#include "lc_global.h"
#include "pieceinf.h"

#include "opengl.h"
#include "texture.h"
#include "project.h"
#include "lc_colors.h"
#include "matrix.h"
#include "defines.h"
#include "library.h"
#include "lc_application.h"
#include "lc_mesh.h"
#include "lc_model.h"

#if LC_IPHONE
#define SIDES 8
#else
#define SIDES 16
#endif

static float sintbl[SIDES];
static float costbl[SIDES];

#define LC_MESH		1
#define LC_STUD		2
#define LC_STUD2	3
#define LC_STUD3	4
#define LC_STUD4	5

// measurements (in centimeters)
//#define LC_FLAT_HEIGHT  0.32f
//#define LC_BRICK_HEIGHT (3*LC_FLAT_HEIGHT)
//#define LC_BASEPLATE_HEIGHT (LC_FLAT_HEIGHT/2)
//#define LC_HALF_WIDE  0.4f
//#define LC_ONE_WIDE   0.8f
//#define LC_BRICK_WALL 0.125f
#define LC_STUD_HEIGHT 0.16f
#define LC_STUD_RADIUS 0.24f
#define LC_KNOB_RADIUS 0.32f
//#define LC_STUD_TECH_RADIUS (LC_FLAT_HEIGHT/2)

/////////////////////////////////////////////////////////////////////////////
// PieceInfo construction/destruction

PieceInfo::PieceInfo()
{
	// Do nothing, initialization is done by LoadIndex() or CreateFromModel().
	m_nRef = 0;
	m_nTextureCount = 0;
}

PieceInfo::~PieceInfo()
{
	FreeInformation();
}

/////////////////////////////////////////////////////////////////////////////
// File I/O

void PieceInfo::LoadIndex(File& file)
{
  static bool init = false;
  short sh[6];
  short scale;

  // Initialize sin/cos table
  if (!init)
  {
    for (int i = 0; i < SIDES; i++)
    {
      sintbl[i] = (float)sin((PI2*i)/(SIDES));
      costbl[i] = (float)cos((PI2*i)/(SIDES));
    }
    init = true;
  }

  // TODO: don't change ref. if we're reloading ?
  m_nRef = 0;
  m_nConnectionCount = 0;
  m_pConnections = NULL;
  m_nGroupCount = 0;
  m_pGroups = NULL;
  m_nTextureCount = 0;
  m_pTextures = NULL;
	m_Mesh = NULL;

  file.Read (m_strName, 8);
  m_strName[8] = '\0';
  file.Read (m_strDescription, 64);
  m_strDescription[64] = '\0';
  file.ReadShort (sh, 6);
  file.ReadByte (&m_nFlags, 1);
  u32 Groups; file.ReadLong (&Groups, 1);
  file.ReadLong (&m_nOffset, 1);
  file.ReadLong (&m_nSize, 1);

  if (m_nFlags & LC_PIECE_SMALL)
    scale = 10000;
  else if (m_nFlags & LC_PIECE_MEDIUM)
    scale = 1000;
  else
    scale = 100;

  m_fDimensions[0] = (float)sh[0]/scale;
  m_fDimensions[1] = (float)sh[1]/scale;
  m_fDimensions[2] = (float)sh[2]/scale;
  m_fDimensions[3] = (float)sh[3]/scale;
  m_fDimensions[4] = (float)sh[4]/scale;
  m_fDimensions[5] = (float)sh[5]/scale;

  m_BoundingBox = BoundingBox(Vector3(m_fDimensions[3], m_fDimensions[4], m_fDimensions[5]),
                              Vector3(m_fDimensions[0], m_fDimensions[1], m_fDimensions[2]));
}

void PieceInfo::CreateFromModel(lcModel* Model)
{
	strncpy(m_strDescription, Model->m_Name, sizeof(m_strDescription));
	m_strDescription[sizeof(m_strDescription)-1] = 0;

	m_fDimensions[3] = Model->m_BoundingBox.m_Min[0];
	m_fDimensions[4] = Model->m_BoundingBox.m_Min[1];
	m_fDimensions[5] = Model->m_BoundingBox.m_Min[2];
	m_fDimensions[0] = Model->m_BoundingBox.m_Max[0];
	m_fDimensions[1] = Model->m_BoundingBox.m_Max[1];
	m_fDimensions[2] = Model->m_BoundingBox.m_Max[2];
	m_nOffset = 0;
	m_nSize = 0;

	m_nFlags = LC_PIECE_MODEL;
	m_nConnectionCount = 0;
	m_pConnections = NULL;
	m_nGroupCount = 0;
	m_pGroups = NULL;
	m_nTextureCount = 0;
	m_pTextures = NULL;
	m_Mesh = Model->m_Mesh;
	m_BoundingBox = Model->m_BoundingBox;
	m_Model = Model;

	m_nRef = 0;
}

void PieceInfo::AddRef()
{
	if (m_nRef == 0)
		LoadInformation();
	m_nRef++;

	for (int i = 0; i < m_nTextureCount; i++)
		if (m_pTextures[i].texture != NULL)
			m_pTextures[i].texture->AddRef(false);
// TODO: get correct filter paramenter
}

void PieceInfo::DeRef()
{
	m_nRef--;
	for (int i = 0; i < m_nTextureCount; i++)
		if (m_pTextures[i].texture != NULL)
			m_pTextures[i].texture->DeRef();

	if (m_nRef == 0)
		FreeInformation();
}

void PieceInfo::RenderBox()
{
	float Verts[8][3] =
	{
		{ m_fDimensions[0], m_fDimensions[1], m_fDimensions[2] }, 
		{ m_fDimensions[3], m_fDimensions[1], m_fDimensions[2] },
		{ m_fDimensions[3], m_fDimensions[4], m_fDimensions[2] },
		{ m_fDimensions[0], m_fDimensions[4], m_fDimensions[2] },
		{ m_fDimensions[0], m_fDimensions[1], m_fDimensions[5] },
		{ m_fDimensions[3], m_fDimensions[1], m_fDimensions[5] },
		{ m_fDimensions[3], m_fDimensions[4], m_fDimensions[5] },
		{ m_fDimensions[0], m_fDimensions[4], m_fDimensions[5] }
	};

	unsigned short Indices[] = 
	{
		0, 4, 1, 5, 2, 6, 3, 7,
		1, 2, 0, 3, 4, 7, 5, 6,
	};

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, Verts);

	glDrawElements(GL_TRIANGLE_STRIP, 8, GL_UNSIGNED_SHORT, Indices);
	glDrawElements(GL_TRIANGLE_STRIP, 8, GL_UNSIGNED_SHORT, Indices+8);

	glDisableClientState(GL_VERTEX_ARRAY);
}

inline u16 EndianSwap(u16 Val)
{
	return LCUINT16(Val);
}

inline u32 EndianSwap(u32 Val)
{
	return LCUINT32(Val);
}

template<typename S, typename D>
static void WriteMeshDrawInfo(u8*& Data, lcMeshEditor<D>& MeshEdit, DRAWGROUP* Group, u32* SectionIndices, lcMeshSection** DstSections)
{
	S* SrcPtr = (S*)Data;
	int NumColors = EndianSwap(*SrcPtr);
	SrcPtr++;

	for (int Color = 0; Color < NumColors; Color++)
	{
		int ColorIndex = lcConvertLDrawColor(EndianSwap(*SrcPtr));
		SrcPtr++;

		int NumQuads = EndianSwap(*SrcPtr);
		SrcPtr++;
		int NumTris = EndianSwap(*(SrcPtr + NumQuads));

		if (NumTris || NumQuads)
		{
			Group->NumSections++;
			MeshEdit.SetCurrentSection(DstSections[ColorIndex*2]);

			for (int i = 0; i < NumQuads; i += 4)
			{
				MeshEdit.AddIndex(EndianSwap(SrcPtr[0]));
				MeshEdit.AddIndex(EndianSwap(SrcPtr[1]));
				MeshEdit.AddIndex(EndianSwap(SrcPtr[2]));
				MeshEdit.AddIndex(EndianSwap(SrcPtr[0]));
				MeshEdit.AddIndex(EndianSwap(SrcPtr[2]));
				MeshEdit.AddIndex(EndianSwap(SrcPtr[3]));
				SrcPtr += 4;
			}

			SrcPtr++;

			for (int i = 0; i < NumTris; i++)
			{
				MeshEdit.AddIndex(EndianSwap(*SrcPtr));
				SrcPtr++;
			}

			SectionIndices[ColorIndex*2] -= NumTris + NumQuads / 4 * 6;
			MeshEdit.EndSection(SectionIndices[ColorIndex*2]);
			MeshEdit.CalculateSectionBoundingBox(DstSections[ColorIndex*2]);
		}
		else
			SrcPtr++;

		int NumLines = EndianSwap(*SrcPtr);
		SrcPtr++;

		if (NumLines)
		{
			Group->NumSections++;
			MeshEdit.SetCurrentSection(DstSections[ColorIndex*2+1]);

			for (int i = 0; i < NumLines; i++)
			{
				MeshEdit.AddIndex(EndianSwap(*SrcPtr));
				SrcPtr++;
			}

			SectionIndices[ColorIndex*2+1] -= NumLines;
			MeshEdit.EndSection(SectionIndices[ColorIndex*2+1]);
			MeshEdit.CalculateSectionBoundingBox(DstSections[ColorIndex*2+1]);
		}
	}

	Data = (u8*)SrcPtr;
}

template<class T>
static void WriteStudDrawInfo(int Color, const Matrix44& Mat, lcMeshEditor<T>& MeshEdit, int BaseVertex, DRAWGROUP* Group, float Radius, u32* SectionIndices, lcMeshSection** DstSections)
{
	// Build vertices.
	Vector3 Vert;

	for (int i = 0; i < SIDES; i++)
	{
		Vert = Vector3(Radius * costbl[i], Radius * sintbl[i], 0.0f);
		MeshEdit.AddVertex(Mul31(Vert, Mat));
		Vert = Vector3(Radius * costbl[i], Radius * sintbl[i], LC_STUD_HEIGHT);
		MeshEdit.AddVertex(Mul31(Vert, Mat));
	}

	Vert = Vector3(0.0f, 0.0f, LC_STUD_HEIGHT);
	MeshEdit.AddVertex(Mul31(Vert, Mat));

	// Build indices.
	Vector3 Min = Mul31(Vector3(-LC_STUD_RADIUS, -LC_STUD_RADIUS, 0.0f), Mat);
	Vector3 Max = Mul31(Vector3(LC_STUD_RADIUS, LC_STUD_RADIUS, LC_STUD_HEIGHT), Mat);

	Group->NumSections++;
	lcMeshSection* Section = DstSections[Color*2];
	MeshEdit.SetCurrentSection(Section);

	Section->Box.m_Min = Min;
	Section->Box.m_Max = Max;

	int v0 = BaseVertex + 2 * SIDES;

	// Triangles.
	for (int i = 0; i < SIDES; i++)
	{
		int i1 = BaseVertex + (i % SIDES) * 2;
		int i2 = BaseVertex + ((i + 1) % SIDES) * 2;

		int v1 = i1;
		int v2 = i1 + 1;
		int v3 = i2;
		int v4 = i2 + 1;

		MeshEdit.AddIndex(v0);
		MeshEdit.AddIndex(v2);
		MeshEdit.AddIndex(v4);

		MeshEdit.AddIndex(v1);
		MeshEdit.AddIndex(v3);
		MeshEdit.AddIndex(v2);

		MeshEdit.AddIndex(v3);
		MeshEdit.AddIndex(v4);
		MeshEdit.AddIndex(v2);
	}

	SectionIndices[Color*2] -= 9*SIDES;
	MeshEdit.EndSection(SectionIndices[Color*2]);

	// Lines.
	Group->NumSections++;
	Section = DstSections[LC_COLOR_EDGE*2+1];
	MeshEdit.SetCurrentSection(Section);

	Section->Box.m_Min = Min;
	Section->Box.m_Max = Max;

	for (int i = 0; i < SIDES; i++)
	{
		int i1 = BaseVertex + (i % SIDES) * 2;
		int i2 = BaseVertex + ((i + 1) % SIDES) * 2;

		int v1 = i1;
		int v2 = i1 + 1;
		int v3 = i2;
		int v4 = i2 + 1;

		MeshEdit.AddIndex(v1);
		MeshEdit.AddIndex(v3);

		MeshEdit.AddIndex(v2);
		MeshEdit.AddIndex(v4);
	}

	SectionIndices[LC_COLOR_EDGE*2+1] -= 4*SIDES;
	MeshEdit.EndSection(SectionIndices[Color*2+1]);
}

template<class T>
static void WriteHollowStudDrawInfo(int Color, const Matrix44& Mat, lcMeshEditor<T>& MeshEdit, int BaseVertex, DRAWGROUP* Group, float InnerRadius, float OuterRadius, u32* SectionIndices, lcMeshSection** DstSections)
{
	// Build vertices.
	Vector3 Vert;

	for (int i = 0; i < SIDES; i++)
	{
		// Outside.
		Vert = Vector3(OuterRadius * costbl[i], OuterRadius * sintbl[i], 0.0f);
		MeshEdit.AddVertex(Mul31(Vert, Mat));
		Vert = Vector3(OuterRadius * costbl[i], OuterRadius * sintbl[i], LC_STUD_HEIGHT);
		MeshEdit.AddVertex(Mul31(Vert, Mat));

		// Inside.
		Vert = Vector3(InnerRadius * costbl[i], InnerRadius * sintbl[i], LC_STUD_HEIGHT);
		MeshEdit.AddVertex(Mul31(Vert, Mat));
		Vert = Vector3(InnerRadius * costbl[i], InnerRadius * sintbl[i], 0.0f);
		MeshEdit.AddVertex(Mul31(Vert, Mat));
	}

	// Build indices.
	Vector3 Min = Mul31(Vector3(-LC_STUD_RADIUS, -LC_STUD_RADIUS, 0.0f), Mat);
	Vector3 Max = Mul31(Vector3(LC_STUD_RADIUS, LC_STUD_RADIUS, LC_STUD_HEIGHT), Mat);

	Group->NumSections++;
	lcMeshSection* Section = DstSections[Color*2];
	MeshEdit.SetCurrentSection(Section);

	Section->Box.m_Min = Min;
	Section->Box.m_Max = Max;

	// Triangles.
	for (int i = 0; i < SIDES; i++)
	{
		int i1 = BaseVertex + (i % SIDES) * 4;
		int i2 = BaseVertex + ((i + 1) % SIDES) * 4;

		int v1 = i1;
		int v2 = i1 + 1;
		int v3 = i1 + 2;
		int v4 = i1 + 3;
		int v5 = i2;
		int v6 = i2 + 1;
		int v7 = i2 + 2;
		int v8 = i2 + 3;

		MeshEdit.AddIndex(v1);
		MeshEdit.AddIndex(v5);
		MeshEdit.AddIndex(v2);

		MeshEdit.AddIndex(v5);
		MeshEdit.AddIndex(v6);
		MeshEdit.AddIndex(v2);

		MeshEdit.AddIndex(v2);
		MeshEdit.AddIndex(v6);
		MeshEdit.AddIndex(v3);

		MeshEdit.AddIndex(v6);
		MeshEdit.AddIndex(v7);
		MeshEdit.AddIndex(v3);

		MeshEdit.AddIndex(v3);
		MeshEdit.AddIndex(v7);
		MeshEdit.AddIndex(v4);

		MeshEdit.AddIndex(v7);
		MeshEdit.AddIndex(v8);
		MeshEdit.AddIndex(v4);
	}

	SectionIndices[Color*2] -= 18*SIDES;
	MeshEdit.EndSection(SectionIndices[Color*2]);

	// Lines.
	Group->NumSections++;
	Section = DstSections[LC_COLOR_EDGE*2+1];
	MeshEdit.SetCurrentSection(Section);

	Section->Box.m_Min = Min;
	Section->Box.m_Max = Max;

	for (int i = 0; i < SIDES; i++)
	{
		int i1 = BaseVertex + (i % SIDES) * 4;
		int i2 = BaseVertex + ((i + 1) % SIDES) * 4;

		int v1 = i1;
		int v2 = i1 + 1;
		int v3 = i1 + 2;
		int v4 = i1 + 3;
		int v5 = i2;
		int v6 = i2 + 1;
		int v7 = i2 + 2;
		int v8 = i2 + 3;

		MeshEdit.AddIndex(v1);
		MeshEdit.AddIndex(v5);

		MeshEdit.AddIndex(v2);
		MeshEdit.AddIndex(v6);

		MeshEdit.AddIndex(v3);
		MeshEdit.AddIndex(v7);

		MeshEdit.AddIndex(v4);
		MeshEdit.AddIndex(v8);
	}

	SectionIndices[LC_COLOR_EDGE*2+1] -= 8*SIDES;
	MeshEdit.EndSection(SectionIndices[Color*2+1]);
}

void PieceInfo::LoadInformation()
{
	if (m_nFlags & LC_PIECE_MODEL)
		return;

	FileDisk bin;
	char filename[LC_MAXPATH];
	CONNECTIONINFO* pConnection;
	void* buf;
	u32 verts, *longs, fixverts;
	u16 sh;
	u8 *bytes, *tmp, bt;
	float scale, shift;
	i16* shorts;
	int i;

	// We don't want memory leaks.
	FreeInformation();

	// Open pieces.bin and buffer the information we need.
	strcpy (filename, lcGetPiecesLibrary()->GetLibraryPath());
	strcat (filename, "pieces.bin");
	if (!bin.Open (filename, "rb"))
		return;

	buf = malloc(m_nSize);
	bin.Seek(m_nOffset, SEEK_SET);
	bin.Read(buf, m_nSize);
	bin.Close();

	shift  = 1.0f/(1<<14);
	scale = 0.01f;
	if (m_nFlags & LC_PIECE_MEDIUM) scale = 0.001f;
	if (m_nFlags & LC_PIECE_SMALL)  scale = 0.0001f;
	longs = (u32*)buf;
	fixverts = verts = LCUINT32(*longs);
	bytes = (unsigned char*)(longs + 1);
	bytes += verts * sizeof(i16) * 3;

	// Read connections
	m_nConnectionCount = LCUINT16(*((u16*)bytes));
	bytes += sizeof (u16);
	m_pConnections = (CONNECTIONINFO*)malloc((m_nConnectionCount+1) * sizeof(CONNECTIONINFO));

	sh = m_nConnectionCount;
	for (pConnection = m_pConnections; sh--; pConnection++)
	{
		pConnection->type = *bytes;
		bytes++;

		shorts = (i16*)bytes;
		pConnection->center[0] = (float)(LCINT16(*shorts))*scale;
		shorts++;
		pConnection->center[1] = (float)(LCINT16(*shorts))*scale;
		shorts++;
		pConnection->center[2] = (float)(LCINT16(*shorts))*scale;
		shorts++;
		pConnection->normal[0] = (float)(LCINT16(*shorts))*shift;
		shorts++;
		pConnection->normal[1] = (float)(LCINT16(*shorts))*shift;
		shorts++;
		pConnection->normal[2] = (float)(LCINT16(*shorts))*shift;
		shorts++;

		bytes = (unsigned char*)shorts;
	}

	// Load textures
	m_nTextureCount = *bytes;
	if (m_nTextureCount > 0)
		m_pTextures = (TEXTURE*)malloc(m_nTextureCount*sizeof(TEXTURE));
	bytes++;

	for (sh = 0; sh < m_nTextureCount; sh++)
	{
		char name[9];
		TEXTURE* tex = &m_pTextures[sh];
		tex->color = lcConvertLDrawColor(*bytes);
		bytes++;

		strcpy(name, (char*)bytes);
		tex->texture = lcGetPiecesLibrary()->FindTexture(name);

		shorts = (i16*)(bytes + 8);
		for (i = 0; i < 4; i++)
		{
			tex->vertex[i][0] = (float)LCINT16(shorts[0])*scale;
			tex->vertex[i][1] = (float)LCINT16(shorts[1])*scale;
			tex->vertex[i][2] = (float)LCINT16(shorts[2])*scale;
			shorts += 3;
		}

		for (i = 0; i < 4; i++)
		{
			tex->coords[i][0] = (float)LCINT16(shorts[0]);
			tex->coords[i][1] = (float)LCINT16(shorts[1]);
			shorts += 2;
		}

		bytes += 8 + 20*sizeof(u16);
	}

	// Read groups
	m_nGroupCount = LCUINT16(*((u16*)bytes));
	bytes += sizeof(u16);
	m_pGroups = (DRAWGROUP*)malloc(sizeof(DRAWGROUP)*m_nGroupCount);
	memset(m_pGroups, 0, sizeof(DRAWGROUP)*m_nGroupCount);

	// Calculate the number of vertices, indices and sections.
	tmp = bytes;
	sh = m_nGroupCount;

	u32* SectionIndices = new u32[lcNumColors*2];
	memset(SectionIndices, 0, sizeof(u32)*lcNumColors*2);

	while (sh--)
	{
		bt = *bytes;
		bytes++;
		bytes += bt*sizeof(u16);

		while (*bytes)
		{
			if (*bytes == LC_MESH)
			{
				if (m_nFlags & LC_PIECE_LONGDATA_FILE)
				{
					u32 colors, *p;
					p = (u32*)(bytes + 1);
					colors = LCUINT32(*p);
					p++;

					while (colors--)
					{
						int color = lcConvertLDrawColor(LCUINT32(*p));
						p++;

						SectionIndices[color*2] += LCUINT32(*p) / 4 * 6;
						p += LCUINT32(*p) + 1;

						SectionIndices[color*2] += LCUINT32(*p);
						p += LCUINT32(*p) + 1;

						SectionIndices[color*2+1] += LCUINT32(*p);
						p += LCUINT32(*p) + 1;
					}

					bytes = (unsigned char*)p;
				}
				else
				{
					u16 colors, *p;
					p = (u16*)(bytes + 1);
					colors = LCUINT16(*p);
					p++;

					while (colors--)
					{
						int color = lcConvertLDrawColor(LCUINT16(*p));
						p++;

						SectionIndices[color*2] += LCUINT16(*p) / 4 * 6;
						p += LCUINT16(*p) + 1;

						SectionIndices[color*2] += LCUINT16(*p);
						p += LCUINT16(*p) + 1;

						SectionIndices[color*2+1] += LCUINT16(*p);
						p += LCUINT16(*p) + 1;
					}

					bytes = (unsigned char*)p;
				}
			}

			if ((*bytes == LC_STUD) || (*bytes == LC_STUD3))
			{
				int color = lcConvertLDrawColor(*(bytes+1));
				verts += (2*SIDES)+1;
				SectionIndices[color*2] += 9*SIDES;
				SectionIndices[LC_COLOR_EDGE*2+1] += 4*SIDES;
				bytes += 2*sizeof(unsigned char) + 12*sizeof(float);
			}

			if ((*bytes == LC_STUD2) || (*bytes == LC_STUD4))
			{
				int color = lcConvertLDrawColor(*(bytes+1));
				verts += 4*SIDES;
				SectionIndices[color*2] += 18*SIDES;
				SectionIndices[LC_COLOR_EDGE*2+1] += 8*SIDES;
				bytes += 2*sizeof(unsigned char) + 12*sizeof(float);
			}
		}
		bytes++; // should be 0
	}

	u32 lines = 0, tris = 0, sections = 0;
	for (int c = 0; c < lcNumColors; c++)
	{
		if (SectionIndices[c*2])
		{
			tris += SectionIndices[c*2];
			sections++;
		}

		if (SectionIndices[c*2+1])
		{
			lines += SectionIndices[c*2+1];
			sections++;
		}
	}

	m_Mesh = new lcMesh(sections, tris + lines, verts, NULL);

	if (m_Mesh->m_IndexType == GL_UNSIGNED_SHORT)
		BuildMesh<u16>(buf, tmp, SectionIndices);
	else
		BuildMesh<u32>(buf, tmp, SectionIndices);

	delete[] SectionIndices;
	free(buf);
}

template<typename T>
void PieceInfo::BuildMesh(void* Data, void* MeshStart, u32* SectionIndices)
{
	lcMeshEditor<T> MeshEdit(m_Mesh);

	// Create empty sections.
	lcMeshSection** DstSections = new lcMeshSection*[lcNumColors*2];
	memset(DstSections, 0, sizeof(DstSections[0])*lcNumColors*2);

	for (int c = 0; c < lcNumColors; c++)
	{
		if (SectionIndices[c*2])
		{
			DstSections[c*2] = MeshEdit.StartSection(GL_TRIANGLES, c);
			MeshEdit.EndSection(SectionIndices[c*2]);
		}

		if (SectionIndices[c*2+1])
		{
			DstSections[c*2+1] = MeshEdit.StartSection(GL_LINES, c);
			MeshEdit.EndSection(SectionIndices[c*2+1]);
		}
	}

	// Copy the 'fixed' vertices
	DRAWGROUP* pGroup;
	u32* longs = (u32*)Data;
	i16* shorts = (i16*)(longs + 1);
	u32 verts;
	int i;

	float scale = 0.01f;
	if (m_nFlags & LC_PIECE_MEDIUM)
		scale = 0.001f;
	else if (m_nFlags & LC_PIECE_SMALL)
		scale = 0.0001f;

	for (verts = 0; verts < LCUINT32(*longs); verts++)
	{
		float Vert[3];
		Vert[0] = (float)LCINT16(*shorts) * scale;
		shorts++;
		Vert[1] = (float)LCINT16(*shorts) * scale;
		shorts++;
		Vert[2] = (float)LCINT16(*shorts) * scale;
		shorts++;
		MeshEdit.AddVertex(Vert);
	}

	// Read groups
	u8* bytes = (u8*)MeshStart;
	u16 sh = m_nGroupCount;

	for (pGroup = m_pGroups; sh--; pGroup++)
	{
		u8 bt = *bytes;
		bytes++;

		pGroup->NumSections = 0;
		pGroup->connections[bt] = 0xFFFF;

		while (bt--)
		{
			u16 tmp = LCUINT16(*((u16*)bytes));
			pGroup->connections[bt] = tmp;
			bytes += sizeof(u16);
		}

		switch (*bytes)
		{
		case LC_MESH:
			{
				bytes++;

				if (m_nFlags & LC_PIECE_LONGDATA_FILE)
					WriteMeshDrawInfo<u32, T>(bytes, MeshEdit, pGroup, SectionIndices, DstSections);
				else
					WriteMeshDrawInfo<u16, T>(bytes, MeshEdit, pGroup, SectionIndices, DstSections);
			} break;

		case LC_STUD:
			{
				u16 color = lcConvertLDrawColor(*(bytes+1));
				float* MatFloats = (float*)(bytes+2);

				// Read matrix.
				for (i = 0; i < 12; i++)
					MatFloats[i] = LCFLOAT(MatFloats[i]);

				Matrix44 Mat(Vector4(MatFloats[0], MatFloats[1], MatFloats[2], 0.0f),
				             Vector4(MatFloats[3], MatFloats[4], MatFloats[5], 0.0f),
				             Vector4(MatFloats[6], MatFloats[7], MatFloats[8], 0.0f),
				             Vector4(MatFloats[9], MatFloats[10], MatFloats[11], 1.0f));

				WriteStudDrawInfo<T>(color, Mat, MeshEdit, verts, pGroup, LC_STUD_RADIUS, SectionIndices, DstSections);

				verts += 2*SIDES+1;
				bytes += 2*sizeof(unsigned char) + 12*sizeof(float);
			} break;

		case LC_STUD2:
			{
				u16 color = lcConvertLDrawColor(*(bytes+1));
				float* MatFloats = (float*)(bytes+2);

				// Read matrix.
				for (i = 0; i < 12; i++)
					MatFloats[i] = LCFLOAT(MatFloats[i]);

				Matrix44 Mat(Vector4(MatFloats[0], MatFloats[1], MatFloats[2], 0.0f),
				             Vector4(MatFloats[3], MatFloats[4], MatFloats[5], 0.0f),
				             Vector4(MatFloats[6], MatFloats[7], MatFloats[8], 0.0f),
				             Vector4(MatFloats[9], MatFloats[10], MatFloats[11], 1.0f));

				WriteHollowStudDrawInfo<T>(color, Mat, MeshEdit, verts, pGroup, 0.16f, LC_STUD_RADIUS, SectionIndices, DstSections);

				verts += 4*SIDES;
				bytes += 2*sizeof(unsigned char) + 12*sizeof(float);
			} break;

		case LC_STUD3:
			{
				u16 color = lcConvertLDrawColor(*(bytes+1));
				float* MatFloats = (float*)(bytes+2);

				// Read matrix.
				for (i = 0; i < 12; i++)
					MatFloats[i] = LCFLOAT(MatFloats[i]);

				Matrix44 Mat(Vector4(MatFloats[0], MatFloats[1], MatFloats[2], 0.0f),
				             Vector4(MatFloats[3], MatFloats[4], MatFloats[5], 0.0f),
				             Vector4(MatFloats[6], MatFloats[7], MatFloats[8], 0.0f),
				             Vector4(MatFloats[9], MatFloats[10], MatFloats[11], 1.0f));

				WriteStudDrawInfo<T>(color, Mat, MeshEdit, verts, pGroup, LC_STUD_RADIUS, SectionIndices, DstSections);

				verts += 2*SIDES+1;
				bytes += 2*sizeof(unsigned char) + 12*sizeof(float);
			} break;

		case LC_STUD4:
			{
				u16 color = lcConvertLDrawColor(*(bytes+1));
				float* MatFloats = (float*)(bytes+2);

				// Read matrix.
				for (i = 0; i < 12; i++)
					MatFloats[i] = LCFLOAT(MatFloats[i]);

				Matrix44 Mat(Vector4(MatFloats[0], MatFloats[1], MatFloats[2], 0.0f),
				             Vector4(MatFloats[3], MatFloats[4], MatFloats[5], 0.0f),
				             Vector4(MatFloats[6], MatFloats[7], MatFloats[8], 0.0f),
				             Vector4(MatFloats[9], MatFloats[10], MatFloats[11], 1.0f));

				WriteHollowStudDrawInfo<T>(color, Mat, MeshEdit, verts, pGroup, 0.16f, LC_STUD_RADIUS, SectionIndices, DstSections);

				verts += 4*SIDES;
				bytes += 2*sizeof(unsigned char) + 12*sizeof(float);
			} break;
		}
		bytes++; // should be 0
	}

	delete[] DstSections;
}

void PieceInfo::FreeInformation()
{
	if (m_nFlags & LC_PIECE_MODEL)
		return;

	delete m_Mesh;
	m_Mesh = NULL;

	if (m_pConnections != NULL)
	{
		free(m_pConnections);
		m_pConnections = NULL;
		m_nConnectionCount = 0;
	}

	if (m_pGroups != NULL)
	{
		free(m_pGroups);
		m_pGroups = NULL;
	}

	if (m_pTextures != NULL)
	{
//		while (m_nTextureCount--)
//			if (m_pTextures[m_nTextureCount].texture)
//				m_pTextures[m_nTextureCount].texture->DeRef();

		free(m_pTextures);
		m_pTextures = NULL;
	}
}

// Zoom extents for the preview window and print catalog
void PieceInfo::ZoomExtents(float Fov, float Aspect, float* EyePos) const
{
	float Eye[3] = { -100.0f, -100.0f, 50.0f };

	if (EyePos)
	{
		Eye[0] = EyePos[0];
		Eye[1] = EyePos[1];
		Eye[2] = EyePos[2];
	}

	// Get perspective information.
	float Alpha = Fov / 2.0f;
	float HalfFovY = Fov / 2.0f;
	HalfFovY = HalfFovY * 3.1415f / 180.0f;
	float HalfFovX = (float)atan(tan(HalfFovY) * Aspect);
	HalfFovX = HalfFovX * 180.0f / 3.1415f;
	float Beta = HalfFovX;

	// Get vectors from the position.
	float NonOrthoTop[3] = { 0.0f, 0.0f, 1.0f };
	float Target[3] = { (m_fDimensions[0] + m_fDimensions[3])*0.5f, (m_fDimensions[1] + m_fDimensions[4])*0.5f,
	                    (m_fDimensions[2] + m_fDimensions[5])*0.5f };
	float Front[3] = { Target[0] - Eye[0], Target[1] - Eye[1], Target[2] - Eye[2]};
	float Side[3];
	Side[0] = NonOrthoTop[1]*Front[2] - NonOrthoTop[2]*Front[1];
	Side[1] = NonOrthoTop[2]*Front[0] - NonOrthoTop[0]*Front[2];
	Side[2] = NonOrthoTop[0]*Front[1] - NonOrthoTop[1]*Front[0];
	
	// Make sure the up vector is orthogonal.
	float Top[3];
	Top[0] = Front[1]*Side[2] - Front[2]*Side[1];
	Top[1] = Front[2]*Side[0] - Front[0]*Side[2];
	Top[2] = Front[0]*Side[1] - Front[1]*Side[0];
	
	// Calculate the plane normals.
	Matrix Mat;
	float TopNormal[3] = { -Top[0], -Top[1], -Top[2] };
	Mat.FromAxisAngle(Side, -Alpha);
	Mat.TransformPoints(TopNormal, 1);

	float BottomNormal[3] = { Top[0], Top[1], Top[2] };
	Mat.FromAxisAngle(Side, Alpha);
	Mat.TransformPoints(BottomNormal, 1);

	float RightNormal[3] = { Side[0], Side[1], Side[2] };
	Mat.FromAxisAngle(Top, -Beta);
	Mat.TransformPoints(RightNormal, 1);

	float LeftNormal[3] = { -Side[0], -Side[1], -Side[2] };
	Mat.FromAxisAngle(Top, Beta);
	Mat.TransformPoints(LeftNormal, 1);

	// Calculate the plane offsets from the normals and the eye position.
	float TopD = Eye[0]*-TopNormal[0] + Eye[1]*-TopNormal[1] + Eye[2]*-TopNormal[2];
	float BottomD = Eye[0]*-BottomNormal[0] + Eye[1]*-BottomNormal[1] + Eye[2]*-BottomNormal[2];
	float LeftD = Eye[0]*-LeftNormal[0] + Eye[1]*-LeftNormal[1] + Eye[2]*-LeftNormal[2];
	float RightD = Eye[0]*-RightNormal[0] + Eye[1]*-RightNormal[1] + Eye[2]*-RightNormal[2];
	
	// Now generate the planes
	float Inv;
	Inv = 1.0f/(float)sqrt(TopNormal[0]*TopNormal[0]+TopNormal[1]*TopNormal[1]+TopNormal[2]*TopNormal[2]);
	float TopPlane[4] = { TopNormal[0]*Inv, TopNormal[1]*Inv, TopNormal[2]*Inv, TopD*Inv };
	Inv = 1.0f/(float)sqrt(BottomNormal[0]*BottomNormal[0]+BottomNormal[1]*BottomNormal[1]+BottomNormal[2]*BottomNormal[2]);
	float BottomPlane[4] = { BottomNormal[0]*Inv, BottomNormal[1]*Inv, BottomNormal[2]*Inv, BottomD*Inv };
	Inv = 1.0f/(float)sqrt(LeftNormal[0]*LeftNormal[0]+LeftNormal[1]*LeftNormal[1]+LeftNormal[2]*LeftNormal[2]);
	float LeftPlane[4] = { LeftNormal[0]*Inv, LeftNormal[1]*Inv, LeftNormal[2]*Inv, LeftD*Inv };
	Inv = 1.0f/(float)sqrt(RightNormal[0]*RightNormal[0]+RightNormal[1]*RightNormal[1]+RightNormal[2]*RightNormal[2]);
	float RightPlane[4] = { RightNormal[0]*Inv, RightNormal[1]*Inv, RightNormal[2]*Inv, RightD*Inv };

	float Verts[8][3] = {
		{ m_fDimensions[0], m_fDimensions[1], m_fDimensions[5] },
		{ m_fDimensions[3], m_fDimensions[1], m_fDimensions[5] },
		{ m_fDimensions[0], m_fDimensions[1], m_fDimensions[2] },
		{ m_fDimensions[3], m_fDimensions[4], m_fDimensions[5] },
		{ m_fDimensions[3], m_fDimensions[4], m_fDimensions[2] },
		{ m_fDimensions[0], m_fDimensions[4], m_fDimensions[2] },
		{ m_fDimensions[0], m_fDimensions[4], m_fDimensions[5] },
		{ m_fDimensions[3], m_fDimensions[1], m_fDimensions[2] } };

	float SmallestU = 10000.0f;
	float* PlaneArray[4] = { TopPlane, BottomPlane, LeftPlane, RightPlane };

	for (int i = 0; i < 4; i++)
	{
		float* Plane = PlaneArray[i];

		for (int j = 0; j < 8; j++)
		{
			Plane[3] = Verts[j][0]*-Plane[0] + Verts[j][1]*-Plane[1] + Verts[j][2]*-Plane[2];

			// Intersect the eye line with the plane, NewEye = Eye + u * (Target - Eye)
			float u = Eye[0] * Plane[0] + Eye[1] * Plane[1] + Eye[2] * Plane[2] + Plane[3];
			u /= Front[0] * -Plane[0] + Front[1] * -Plane[1] + Front[2] * -Plane[2];

			if (u < SmallestU)
				SmallestU = u;
		}
	}

	float NewEye[3];
	NewEye[0] = Eye[0] + Front[0] * SmallestU;
	NewEye[1] = Eye[1] + Front[1] * SmallestU;
	NewEye[2] = Eye[2] + Front[2] * SmallestU;

	if (EyePos)
	{
		EyePos[0] = NewEye[0];
		EyePos[1] = NewEye[1];
		EyePos[2] = NewEye[2];
	}

	Vector3 FrontVec, RightVec, UpVec;

	// Calculate view matrix.
	UpVec = Normalize(Vector3(Top[0], Top[1], Top[2]));
	FrontVec = Normalize(Vector3(Front[0], Front[1], Front[2]));
	RightVec = Normalize(Vector3(Side[0], Side[1], Side[2]));

  float ViewMat[16];
  ViewMat[0] = -RightVec[0]; ViewMat[4] = -RightVec[1]; ViewMat[8]  = -RightVec[2]; ViewMat[12] = 0.0;
  ViewMat[1] = UpVec[0];     ViewMat[5] = UpVec[1];     ViewMat[9]  = UpVec[2];     ViewMat[13] = 0.0;
  ViewMat[2] = -FrontVec[0]; ViewMat[6] = -FrontVec[1]; ViewMat[10] = -FrontVec[2]; ViewMat[14] = 0.0;
  ViewMat[3] = 0.0;          ViewMat[7] = 0.0;          ViewMat[11] = 0.0;          ViewMat[15] = 1.0;

  // Load ViewMatrix
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glMultMatrixf(ViewMat);
  glTranslatef(-NewEye[0], -NewEye[1], -NewEye[2]);
}

void PieceInfo::RenderPiece(int nColor)
{
	unsigned short sh;

	for (sh = 0; sh < m_nTextureCount; sh++)
	{
//		if (!m_pTextures[sh].texture->IsLoaded())
//			m_pTextures[sh].texture->Load(false);

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
		m_pTextures[sh].texture->MakeCurrent();

		if (m_pTextures[sh].color == LC_COLOR_DEFAULT)
			lcSetColor(nColor);

		if (LC_COLOR_TRANSLUCENT(nColor))
		{
			glEnable(GL_BLEND);
			glDepthMask(GL_FALSE);
		}
		else
		{
			glDisable(GL_BLEND);
			glDepthMask(GL_TRUE);
		}

		glEnable(GL_TEXTURE_2D);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, m_pTextures[sh].vertex);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, m_pTextures[sh].coords);

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisable(GL_TEXTURE_2D);
	}

	m_Mesh->Render(nColor);

	// if glDepthMask is GL_FALSE then glClearBuffer (GL_DEPTH_BUFFER_BIT) doesn't work
	glDepthMask (GL_TRUE);
}

void PieceInfo::WriteWavefront(FILE* file, unsigned char color, unsigned long* start)
{
	void* indices = m_Mesh->m_IndexBuffer->MapBuffer(GL_READ_ONLY_ARB);

	for (int i = 0; i < m_Mesh->m_SectionCount; i++)
	{
		lcMeshSection* Section = &m_Mesh->m_Sections[i];
		const char* colname;

		// Skip lines.
		if (Section->PrimitiveType != GL_TRIANGLES)
			continue;

		if (Section->ColorIndex == LC_COLOR_DEFAULT)
			colname = g_ColorList[color].Name;
		else
		{
			if (Section->ColorIndex >= lcNumColors)
				continue;

			colname = g_ColorList[Section->ColorIndex].Name;
		}

		char altname[256];
		strcpy(altname, colname);
		while (char* ptr = (char*)strchr(altname, ' '))
			*ptr = '_';

		fprintf(file, "usemtl %s\n", altname);

		if (m_Mesh->m_IndexType == GL_UNSIGNED_INT)
		{
			u32* IndexPtr = (u32*)((char*)indices + Section->IndexOffset);
			for (int c = 0; c < Section->IndexCount; c += 3)
				fprintf(file, "f %ld %ld %ld\n", IndexPtr[c+0] + *start, IndexPtr[c+1] + *start, IndexPtr[c+2] + *start);
		}
		else
		{
			u16* IndexPtr = (u16*)((char*)indices + Section->IndexOffset);
			for (int c = 0; c < Section->IndexCount; c += 3)
				fprintf(file, "f %ld %ld %ld\n", IndexPtr[c+0] + *start, IndexPtr[c+1] + *start, IndexPtr[c+2] + *start);
		}
	}

	m_Mesh->m_IndexBuffer->UnmapBuffer();

	*start += m_Mesh->m_VertexCount;
	fputs("\n", file);
}
