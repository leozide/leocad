// Information about how to draw a piece and some more stuff.
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "opengl.h"
#include "texture.h"
#include "pieceinf.h"
#include "project.h"
#include "globals.h"
#include "matrix.h"
#include "defines.h"
#include "config.h"
#include "library.h"
#include "lc_application.h"
#include "lc_mesh.h"

#define SIDES 16
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

// Convert a color from LDraw to LeoCAD
unsigned char ConvertColor(int c)
{
	if (c > 255) c -= 256;
	switch (c)
	{
	case 0: return 9;	// black		(black)
	case 1: return 4;	// blue			(blue)
	case 2: return 2;	// green		(green)
	case 3: return 5;	// dark cyan
	case 4: return 0;	// red			(red)
	case 5: return 11;	// magenta
	case 6: return 10;	// brown		(brown)
	case 7: return 22;	// gray			(gray)
	case 8: return 8;	// dark gray	(dark gray)
	case 9: return 5;	// light blue	()
	case 10: return 3;	// light green	(light green)
	case 11: return 5;	// cyan			(light blue)
	case 12: return 1;	// light red
	case 13: return 11;	// pink			(pink)
	case 14: return 6;	// yellow		(yellow)
	case 15: return 7;	// white		(white)
	case 16: return LC_COL_DEFAULT; // special case
	case 24: return LC_COL_EDGES; // edge
	case 32: return 9;	// black
	case 33: return 18;	// clear blue
	case 34: return 16;	// clear green
	case 35: return 5;	// dark cyan
	case 36: return 14;	// clear red
	case 37: return 11;	// magenta
	case 38: return 10;	// brown
	case 39: return 21;	// clear white (clear gray)
	case 40: return 8;	// dark gray
	case 41: return 19;	// clear light blue
	case 42: return 17;	// clear light green
	case 43: return 19;	// clear cyan			(clear light blue)
	case 44: return 15;	// clear light red ??
	case 45: return 11;	// pink
	case 46: return 20;	// clear yellow
	case 47: return 21;	// clear white
	case 70: return 10; // maroon (326)
	case 78: return 13;	// gold (334)
	case 110: return 1; // orange (366 from fire logo pattern)
	case 126: return 23;// tan (382)
	case 127: return 27;// silver/chrome (383)
	case 175: return 3;	// mint green (431)
	case 206: return 1;	// orange (462)
	case 238: return 6;	// light yellow (494 eletric contacts)
	case 239: return 6;	// light yellow (495)
	case 247: return 27;// 503 chrome
	case 250: return 3; // 506 mint (Belville)
	case 253: return 11;// 509 rose (e.g. in Paradisa)

	// taken from l2p.doc but not verified
	case 178: return 11;// 434 dark cyan (e.g. in New Technic Models)
	case 254: return 6; // 510 light yellow (e.g. in Belville)
	}
	return 9; // black
}

/////////////////////////////////////////////////////////////////////////////
// PieceInfo construction/destruction

PieceInfo::PieceInfo()
{
  // Do nothing, initialization is done by LoadIndex()
}

PieceInfo::~PieceInfo()
{
  FreeInformation();
}

/////////////////////////////////////////////////////////////////////////////
// File I/O

void PieceInfo::LoadIndex (File& file)
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
	m_nBoxList = 0;
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

	Vector3 p1(m_fDimensions[0], m_fDimensions[1], m_fDimensions[2]);
	Vector3 p2(m_fDimensions[3], m_fDimensions[4], m_fDimensions[5]);

	m_Center = p1 + (p2 - p1) / 2;
	m_Dimensions = Vector4(p2 - m_Center, Length(p2 - p1) / 2);
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

void PieceInfo::CreateBoxDisplayList()
{
	if (m_nBoxList)
		return;

	// Create a display for the bounding box.
	m_nBoxList = glGenLists(1);
	glNewList(m_nBoxList, GL_COMPILE);
	glEnableClientState(GL_VERTEX_ARRAY);

	float box[24][3] =
	{
		{ m_fDimensions[0], m_fDimensions[1], m_fDimensions[2] }, 
		{ m_fDimensions[3], m_fDimensions[1], m_fDimensions[2] },
		{ m_fDimensions[3], m_fDimensions[4], m_fDimensions[2] },
		{ m_fDimensions[0], m_fDimensions[4], m_fDimensions[2] },
		{ m_fDimensions[0], m_fDimensions[1], m_fDimensions[5] },
		{ m_fDimensions[0], m_fDimensions[4], m_fDimensions[5] },
		{ m_fDimensions[3], m_fDimensions[4], m_fDimensions[5] },
		{ m_fDimensions[3], m_fDimensions[1], m_fDimensions[5] },
		{ m_fDimensions[3], m_fDimensions[4], m_fDimensions[2] }, 
		{ m_fDimensions[3], m_fDimensions[1], m_fDimensions[2] },
		{ m_fDimensions[3], m_fDimensions[1], m_fDimensions[5] },
		{ m_fDimensions[3], m_fDimensions[4], m_fDimensions[5] },
		{ m_fDimensions[0], m_fDimensions[4], m_fDimensions[5] },
		{ m_fDimensions[0], m_fDimensions[1], m_fDimensions[5] },
		{ m_fDimensions[0], m_fDimensions[1], m_fDimensions[2] },
		{ m_fDimensions[0], m_fDimensions[4], m_fDimensions[2] }, 
		{ m_fDimensions[0], m_fDimensions[1], m_fDimensions[5] },
		{ m_fDimensions[3], m_fDimensions[1], m_fDimensions[5] },
		{ m_fDimensions[3], m_fDimensions[1], m_fDimensions[2] },
		{ m_fDimensions[0], m_fDimensions[1], m_fDimensions[2] },
		{ m_fDimensions[0], m_fDimensions[4], m_fDimensions[2] },
		{ m_fDimensions[3], m_fDimensions[4], m_fDimensions[2] },
		{ m_fDimensions[3], m_fDimensions[4], m_fDimensions[5] },
		{ m_fDimensions[0], m_fDimensions[4], m_fDimensions[5] }
	};

	glVertexPointer(3, GL_FLOAT, 0, box);
	glDrawArrays(GL_QUADS, 0, 24);
	glEndList();
}

void PieceInfo::LoadInformation()
{
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
	m_pConnections = (CONNECTIONINFO*)malloc (m_nConnectionCount * sizeof(CONNECTIONINFO));

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
		tex->color = ConvertColor(*bytes);
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
	u32 lines = 0, tris = 0, quads = 0, fixquads = 0, sections = 0;

	while (sh--)
	{
		bt = *bytes;
		bytes++;
		bytes += bt*sizeof(u16);

		while (*bytes)
		{
			if (*bytes == LC_MESH)
			{
				if ((fixverts > 65535) || (m_nFlags & LC_PIECE_LONGDATA))
				{
					u32 colors, *p;
					p = (u32*)(bytes + 1);
					colors = LCUINT32(*p);
					p++;

					while (colors--)
					{
						p++; // color code
						fixquads += LCUINT32(*p);

						quads += LCUINT32(*p);
						if (LCUINT32(*p)) sections++;
						p += LCUINT32(*p) + 1;

						tris += LCUINT32(*p);
						if (LCUINT32(*p)) sections++;
						p += LCUINT32(*p) + 1;

						lines += LCUINT32(*p);
						if (LCUINT32(*p)) sections++;
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
						p++; // color code
						fixquads += LCUINT16(*p);

						quads += LCUINT16(*p);
						if (LCUINT16(*p)) sections++;
						p += LCUINT16(*p) + 1;

						tris += LCUINT16(*p);
						if (LCUINT16(*p)) sections++;
						p += LCUINT16(*p) + 1;

						lines += LCUINT16(*p);
						if (LCUINT16(*p)) sections++;
						p += LCUINT16(*p) + 1;
					}

					bytes = (unsigned char*)p;
				}
			}

			if (*bytes == LC_STUD)
			{
				verts += (2*SIDES)+1;
				quads += 4*SIDES;
				sections++;
				tris += 3*SIDES;
				sections++;
				lines += 4*SIDES;
				sections++;
				bytes += 2*sizeof(unsigned char) + 12*sizeof(float);
			}

			if (*bytes == LC_STUD2)
			{
				verts += 4*SIDES;
				quads += 12*SIDES;
				sections++;
				lines += 8*SIDES;
				sections++;
				bytes += 2*sizeof(unsigned char) + 12*sizeof(float);
			}

			if (*bytes == LC_STUD3)
			{
				verts += (2*SIDES)+1;
				quads += 4*SIDES;
				sections++;
				tris += 3*SIDES;
				sections++;
				lines += 4*SIDES;
				sections++;
				bytes += 2*sizeof(unsigned char) + 12*sizeof(float);
			}

			if (*bytes == LC_STUD4)
			{
				verts += 4*SIDES;
				quads += 12*SIDES;
				sections++;
				lines += 8*SIDES;
				sections++;
				bytes += 2*sizeof(unsigned char) + 12*sizeof(float);
			}
		}
		bytes++; // should be 0
	}

	if ((verts > 65535) || (quads > 65535) || (fixquads > 65535))
	{
		if ((m_nFlags & LC_PIECE_LONGDATA) == 0)
		{
			m_nFlags |= LC_PIECE_LONGDATA | LC_PIECE_LONGDATA_RUNTIME;
		}
	}
	else
		m_nFlags &= ~(LC_PIECE_LONGDATA | LC_PIECE_LONGDATA_RUNTIME);

	m_Mesh = new lcMesh(sections, quads + tris + lines, verts, NULL);

	if (m_Mesh->m_IndexType == GL_UNSIGNED_SHORT)
		BuildMesh(TypeToType<u16>(), buf, tmp, (fixverts > 65535) || (fixquads > 65535));
	else
		BuildMesh(TypeToType<u32>(), buf, tmp, (fixverts > 65535) || (fixquads > 65535));

	free(buf);
}

template<typename T>
void PieceInfo::BuildMesh(TypeToType<T>, void* Data, void* MeshStart, bool LongData)
{
	lcMeshEditor<T> MeshEdit(m_Mesh);

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

		// Currently there's only one type of drawinfo (mesh or stud)
		// per group but this will change in the future.
		switch (*bytes)
		{
		case LC_MESH:
			if (LongData)
			{
				u32 colors, *p;
				bytes++;
				p = (u32*)bytes;
				*p = LCUINT32(*p);
				colors = *p;
				p++;

				while (colors--)
				{
					*p = ConvertColor(LCUINT32(*p));
					int ColorIndex = *p;
					p++; // color code

					int PrimTypes[] = { GL_QUADS, GL_TRIANGLES, GL_LINES };
					for (int PrimType = 0; PrimType < 3; PrimType++)
					{
						int NumPrims = LCUINT32(*p);
						p++;

						if (NumPrims)
						{
							pGroup->NumSections++;
							MeshEdit.StartSection(PrimTypes[PrimType], ColorIndex);

#ifdef LC_BIG_ENDIAN
							while (NumPrims--)
							{
								MeshEdit.AddIndex(LCUINT32(*p));
								p++;
							}
#else
							MeshEdit.AddIndices32(p, NumPrims);
							p += NumPrims;
#endif
							MeshEdit.EndSection();
						}
					}
				}

				bytes = (unsigned char*)p;
			}
			else
			{
				u16 colors, *p;
				bytes++;
				p = (u16*)bytes;
				*p = LCUINT16(*p);
				colors = *p;
				p++;

				while (colors--)
				{
					*p = ConvertColor(LCUINT16(*p));
					int ColorIndex = *p;
					p++; // color code

					int PrimTypes[] = { GL_QUADS, GL_TRIANGLES, GL_LINES };
					for (int PrimType = 0; PrimType < 3; PrimType++)
					{
						int NumPrims = LCUINT16(*p);
						p++;

						if (NumPrims)
						{
							pGroup->NumSections++;
							MeshEdit.StartSection(PrimTypes[PrimType], ColorIndex);

#ifdef LC_BIG_ENDIAN
							while (NumPrims--)
							{
								MeshEdit.AddIndex(LCUINT16(*p));
								p++;
							}
#else
							MeshEdit.AddIndices16(p, NumPrims);
							p += NumPrims;
#endif
							MeshEdit.EndSection();
						}
					}
				}

				bytes = (unsigned char*)p;
			}
			break;

		case LC_STUD:
			{
				u16 color = ConvertColor(*(bytes+1));
				float* MatFloats = (float*)(bytes+2);

				// Read matrix.
				for (i = 0; i < 12; i++)
					MatFloats[i] = LCFLOAT(MatFloats[i]);

				Matrix44 Mat(Vector4(MatFloats[0], MatFloats[1], MatFloats[2], 0.0f),
				             Vector4(MatFloats[3], MatFloats[4], MatFloats[5], 0.0f),
				             Vector4(MatFloats[6], MatFloats[7], MatFloats[8], 0.0f),
				             Vector4(MatFloats[9], MatFloats[10], MatFloats[11], 1.0f));
				Vector3 Verts[2*SIDES+1];

				// Create the vertices.
				for (i = 0; i < SIDES; i++)
				{
					Verts[i] = Vector3(LC_STUD_RADIUS * costbl[i], LC_STUD_RADIUS * sintbl[i], 0.0f);
					Verts[i+SIDES] = Vector3(LC_STUD_RADIUS * costbl[i], LC_STUD_RADIUS * sintbl[i], LC_STUD_HEIGHT);
				}
				Verts[2*SIDES] = Vector3(0, 0, LC_STUD_HEIGHT);

				for (i = 0; i < 2*SIDES+1; i++)
				{
					Vector3 tmp = Mul31(Verts[i], Mat);
					MeshEdit.AddVertex(tmp);
				}

				pGroup->NumSections++;
				MeshEdit.StartSection(GL_QUADS, color);

				for (i = 0; i < SIDES; i++)
				{
					MeshEdit.AddIndex(verts + i);
					if (i == SIDES-1)
					{
						MeshEdit.AddIndex(verts);
						MeshEdit.AddIndex(verts + SIDES);
					}
					else
					{
						MeshEdit.AddIndex(verts + i + 1);
						MeshEdit.AddIndex(verts + SIDES + i + 1);
					}
					MeshEdit.AddIndex(verts + SIDES + i);
				}

				MeshEdit.EndSection();
				pGroup->NumSections++;
				MeshEdit.StartSection(GL_TRIANGLES, color);

				for (i = 0; i < SIDES; i++)
				{
					MeshEdit.AddIndex(verts + 2*SIDES);
					MeshEdit.AddIndex(verts + SIDES + i);
					if (i == SIDES-1)
						MeshEdit.AddIndex(verts + SIDES);
					else
						MeshEdit.AddIndex(verts + SIDES + i + 1);
				}

				MeshEdit.EndSection();
				pGroup->NumSections++;
				MeshEdit.StartSection(GL_LINES, LC_COL_EDGES);

				for (i = 0; i < SIDES; i++)
				{
					MeshEdit.AddIndex(verts + i);
					if (i == SIDES-1)
					{
						MeshEdit.AddIndex(verts);
						MeshEdit.AddIndex(verts + i + SIDES);
						MeshEdit.AddIndex(verts + SIDES);
					}
					else
					{
						MeshEdit.AddIndex(verts + i + 1);
						MeshEdit.AddIndex(verts + i + SIDES);
						MeshEdit.AddIndex(verts + i + 1 + SIDES);
					}
				}
				MeshEdit.EndSection();

				verts += 2*SIDES+1;
				bytes += 2*sizeof(unsigned char) + 12*sizeof(float);
		} break;

		case LC_STUD2:
			{
				u16 color = ConvertColor(*(bytes+1));
				float* MatFloats = (float*)(bytes+2);

				// Read matrix.
				for (i = 0; i < 12; i++)
					MatFloats[i] = LCFLOAT(MatFloats[i]);

				Matrix44 Mat(Vector4(MatFloats[0], MatFloats[1], MatFloats[2], 0.0f),
				             Vector4(MatFloats[3], MatFloats[4], MatFloats[5], 0.0f),
				             Vector4(MatFloats[6], MatFloats[7], MatFloats[8], 0.0f),
				             Vector4(MatFloats[9], MatFloats[10], MatFloats[11], 1.0f));
				Vector3 Verts[4*SIDES];

				// Create the vertices.
				for (i = 0; i < SIDES; i++)
				{
					// Outside.
					Verts[i] = Vector3(LC_STUD_RADIUS * costbl[i], LC_STUD_RADIUS * sintbl[i], LC_STUD_HEIGHT);
					Verts[i+SIDES] = Vector3(LC_STUD_RADIUS * costbl[i], LC_STUD_RADIUS * sintbl[i], 0.0f);

					// Inside.
					Verts[i+2*SIDES] = Vector3(0.16f * costbl[i], 0.16f * sintbl[i], 0.0f);
					Verts[i+3*SIDES] = Vector3(0.16f * costbl[i], 0.16f * sintbl[i], LC_STUD_HEIGHT);
				}

				for (i = 0; i < 4*SIDES; i++)
				{
					Vector3 tmp = Mul31(Verts[i], Mat);
					MeshEdit.AddVertex(tmp);
				}

				pGroup->NumSections++;
				MeshEdit.StartSection(GL_QUADS, color);

				// Outside.
				for (i = 0; i < SIDES; i++)
				{
					MeshEdit.AddIndex(verts + SIDES + i);
					if (i == SIDES-1)
					{
						MeshEdit.AddIndex(verts + SIDES);
						MeshEdit.AddIndex(verts);
					}
					else
					{
						MeshEdit.AddIndex(verts + SIDES + i + 1);
						MeshEdit.AddIndex(verts + i + 1);
					}
					MeshEdit.AddIndex(verts + i);
				}

				// Inside.
				for (i = 0; i < SIDES; i++)
				{
					MeshEdit.AddIndex(verts + 2*SIDES + i);
					if (i == SIDES-1)
					{
						MeshEdit.AddIndex(verts + 2*SIDES);
						MeshEdit.AddIndex(verts + 3*SIDES);
					}
					else
					{
						MeshEdit.AddIndex(verts + 2*SIDES + i + 1);
						MeshEdit.AddIndex(verts + 3*SIDES + i + 1);
					}
					MeshEdit.AddIndex(verts + 3*SIDES + i);
				}

				// ring
				for (i = 0; i < SIDES; i++)
				{
					MeshEdit.AddIndex(verts + i);
					if (i == SIDES-1)
					{
						MeshEdit.AddIndex(verts);
						MeshEdit.AddIndex(verts + 3*SIDES);
					}
					else
					{
						MeshEdit.AddIndex(verts + i + 1);
						MeshEdit.AddIndex(verts + 3*SIDES + i + 1);
					}
					MeshEdit.AddIndex(verts + 3*SIDES + i);
				}

				MeshEdit.EndSection();
				pGroup->NumSections++;
				MeshEdit.StartSection(GL_LINES, LC_COL_EDGES);

				// outside
				for (i = 0; i < SIDES; i++)
				{
					MeshEdit.AddIndex(verts + i);
					if (i == SIDES-1)
					{
						MeshEdit.AddIndex(verts);
						MeshEdit.AddIndex(verts + i + SIDES);
						MeshEdit.AddIndex(verts + SIDES);
					}
					else
					{
						MeshEdit.AddIndex(verts + i + 1);
						MeshEdit.AddIndex(verts + i + SIDES);
						MeshEdit.AddIndex(verts + i + 1 + SIDES);
					}
				}

				// inside
				for (i = 0; i < SIDES; i++)
				{
					MeshEdit.AddIndex(verts + 2*SIDES + i);
					if (i == SIDES-1)
					{
						MeshEdit.AddIndex(verts + 2*SIDES);
						MeshEdit.AddIndex(verts + 2*SIDES + i + SIDES);
						MeshEdit.AddIndex(verts + 2*SIDES + SIDES);
					}
					else
					{
						MeshEdit.AddIndex(verts + 2*SIDES + i + 1);
						MeshEdit.AddIndex(verts + 2*SIDES + i + SIDES);
						MeshEdit.AddIndex(verts + 2*SIDES + i + 1 + SIDES);
					}
				}

				MeshEdit.EndSection();

				verts += 4*SIDES;
				bytes += 2*sizeof(unsigned char) + 12*sizeof(float);
		} break;

		case LC_STUD3:
			{
				u16 color = ConvertColor(*(bytes+1));
				float* MatFloats = (float*)(bytes+2);

				// Read matrix.
				for (i = 0; i < 12; i++)
					MatFloats[i] = LCFLOAT(MatFloats[i]);

				Matrix44 Mat(Vector4(MatFloats[0], MatFloats[1], MatFloats[2], 0.0f),
				             Vector4(MatFloats[3], MatFloats[4], MatFloats[5], 0.0f),
				             Vector4(MatFloats[6], MatFloats[7], MatFloats[8], 0.0f),
				             Vector4(MatFloats[9], MatFloats[10], MatFloats[11], 1.0f));
				Vector3 Verts[2*SIDES+1];

				// Create the vertices.
				for (i = 0; i < SIDES; i++)
				{
					Verts[i] = Vector3(0.16f * costbl[i], 0.16f * sintbl[i], 0.0f);
					Verts[i+SIDES] = Vector3(0.16f * costbl[i], 0.16f * sintbl[i], LC_STUD_HEIGHT);
				}
				Verts[2*SIDES] = Vector3(0.0f, 0.0f, LC_STUD_HEIGHT);

				for (i = 0; i < 2*SIDES+1; i++)
				{
					Vector3 tmp = Mul31(Verts[i], Mat);
					MeshEdit.AddVertex(tmp);
				}

				pGroup->NumSections++;
				MeshEdit.StartSection(GL_QUADS, color);

				for (i = 0; i < SIDES; i++)
				{
					MeshEdit.AddIndex(verts + SIDES + i);
					if (i == SIDES-1)
					{
						MeshEdit.AddIndex(verts + SIDES);
						MeshEdit.AddIndex(verts);
					}
					else
					{
						MeshEdit.AddIndex(verts + SIDES + i + 1);
						MeshEdit.AddIndex(verts + i + 1);
					}
					MeshEdit.AddIndex(verts + i);
				}

				MeshEdit.EndSection();
				pGroup->NumSections++;
				MeshEdit.StartSection(GL_TRIANGLES, color);

				for (i = 0; i < SIDES; i++)
				{
					if (i == SIDES-1)
						MeshEdit.AddIndex(verts + SIDES);
					else
						MeshEdit.AddIndex(verts + SIDES + i + 1);
					MeshEdit.AddIndex(verts + SIDES + i);
					MeshEdit.AddIndex(verts + 2*SIDES);
				}

				MeshEdit.EndSection();
				pGroup->NumSections++;
				MeshEdit.StartSection(GL_LINES, LC_COL_EDGES);

				for (i = 0; i < SIDES; i++)
				{
					MeshEdit.AddIndex(verts + i);
					if (i == SIDES-1)
					{
						MeshEdit.AddIndex(verts);
						MeshEdit.AddIndex(verts + i + SIDES);
						MeshEdit.AddIndex(verts + SIDES);
					}
					else
					{
						MeshEdit.AddIndex(verts + i + 1);
						MeshEdit.AddIndex(verts + i + SIDES);
						MeshEdit.AddIndex(verts + i + 1 + SIDES);
					}
				}

				MeshEdit.EndSection();

				verts += 2*SIDES+1;
				bytes += 2*sizeof(unsigned char) + 12*sizeof(float);
			} break;

		case LC_STUD4:
			{
				u16 color = ConvertColor(*(bytes+1));
				float* MatFloats = (float*)(bytes+2);

				// Read matrix.
				for (i = 0; i < 12; i++)
					MatFloats[i] = LCFLOAT(MatFloats[i]);

				Matrix44 Mat(Vector4(MatFloats[0], MatFloats[1], MatFloats[2], 0.0f),
				             Vector4(MatFloats[3], MatFloats[4], MatFloats[5], 0.0f),
				             Vector4(MatFloats[6], MatFloats[7], MatFloats[8], 0.0f),
				             Vector4(MatFloats[9], MatFloats[10], MatFloats[11], 1.0f));
				Vector3 Verts[4*SIDES];

				// Create the vertices.
				for (i = 0; i < SIDES; i++)
				{
					// outside
					Verts[i] = Vector3(LC_KNOB_RADIUS * costbl[i], LC_KNOB_RADIUS * sintbl[i], LC_STUD_HEIGHT);
					Verts[i+SIDES] = Vector3(LC_KNOB_RADIUS * costbl[i], LC_KNOB_RADIUS * sintbl[i], 0.0f);

					// inside
					Verts[i+2*SIDES] = Vector3(LC_STUD_RADIUS * costbl[i], LC_STUD_RADIUS * sintbl[i], 0.0f);
					Verts[i+3*SIDES] = Vector3(LC_STUD_RADIUS * costbl[i], LC_STUD_RADIUS * sintbl[i], LC_STUD_HEIGHT);
				}

				for (i = 0; i < 4*SIDES; i++)
				{
					Vector3 tmp = Mul31(Verts[i], Mat);
					MeshEdit.AddVertex(tmp);
				}

				pGroup->NumSections++;
				MeshEdit.StartSection(GL_QUADS, color);

				// outside
				for (i = 0; i < SIDES; i++)
				{
					MeshEdit.AddIndex(verts + i);
					if (i == SIDES-1)
					{
						MeshEdit.AddIndex(verts);
						MeshEdit.AddIndex(verts + SIDES);
					}
					else
					{
						MeshEdit.AddIndex(verts + i + 1);
						MeshEdit.AddIndex(verts + SIDES + i + 1);
					}
					MeshEdit.AddIndex(verts + SIDES + i);
				}

				// inside
				for (i = 0; i < SIDES; i++)
				{
					MeshEdit.AddIndex(verts + 3*SIDES + i);
					if (i == SIDES-1)
					{
						MeshEdit.AddIndex(verts + 3*SIDES);
						MeshEdit.AddIndex(verts + 2*SIDES);
					}
					else
					{
						MeshEdit.AddIndex(verts + 3*SIDES + i + 1);
						MeshEdit.AddIndex(verts + 2*SIDES + i + 1);
					}
					MeshEdit.AddIndex(verts + 2*SIDES + i);
				}

				// ring
				for (i = 0; i < SIDES; i++)
				{
					MeshEdit.AddIndex(verts + 3*SIDES + i);
					if (i == SIDES-1)
					{
						MeshEdit.AddIndex(verts + 3*SIDES);
						MeshEdit.AddIndex(verts);
					}
					else
					{
						MeshEdit.AddIndex(verts + 3*SIDES + i + 1);
						MeshEdit.AddIndex(verts + i + 1);
					}
					MeshEdit.AddIndex(verts + i);
				}

				MeshEdit.EndSection();
				pGroup->NumSections++;
				MeshEdit.StartSection(GL_LINES, LC_COL_EDGES);

				// outside
				for (i = 0; i < SIDES; i++)
				{
					MeshEdit.AddIndex(verts + i);
					if (i == SIDES-1)
					{
						MeshEdit.AddIndex(verts);
						MeshEdit.AddIndex(verts + i + SIDES);
						MeshEdit.AddIndex(verts + SIDES);
					}
					else
					{
						MeshEdit.AddIndex(verts + i + 1);
						MeshEdit.AddIndex(verts + i + SIDES);
						MeshEdit.AddIndex(verts + i + 1 + SIDES);
					}
				}

				// inside
				for (i = 0; i < SIDES; i++)
				{
					MeshEdit.AddIndex(verts + 2*SIDES + i);
					if (i == SIDES-1)
					{
						MeshEdit.AddIndex(verts + 2*SIDES);
						MeshEdit.AddIndex(verts + 2*SIDES + i + SIDES);
						MeshEdit.AddIndex(verts + 2*SIDES + SIDES);
					}
					else
					{
						MeshEdit.AddIndex(verts + 2*SIDES + i + 1);
						MeshEdit.AddIndex(verts + 2*SIDES + i + SIDES);
						MeshEdit.AddIndex(verts + 2*SIDES + i + 1 + SIDES);
					}
				}
				MeshEdit.EndSection();

				verts += 4*SIDES;
				bytes += 2*sizeof(unsigned char) + 12*sizeof(float);
			} break;
		}
		bytes++; // should be 0
	}
}

void PieceInfo::FreeInformation()
{
  if (m_nBoxList != 0)
	glDeleteLists(m_nBoxList, 1);
	m_nBoxList = 0;

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

	if (m_nFlags & LC_PIECE_LONGDATA_RUNTIME)
	{
		m_nFlags &= ~(LC_PIECE_LONGDATA | LC_PIECE_LONGDATA_RUNTIME);
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

	for (int i = 0; i < 4; i++)
	{
		float* Plane;

		switch (i)
		{
		case 0: Plane = TopPlane; break;
		case 1: Plane = BottomPlane; break;
		case 2: Plane = LeftPlane; break;
		case 3: Plane = RightPlane; break;
		}

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
	UpVec = Vector3(Top[0], Top[1], Top[2]);
	UpVec.Normalize();
	FrontVec = Vector3(Front[0], Front[1], Front[2]);
	FrontVec.Normalize();
	RightVec = Vector3(Side[0], Side[1], Side[2]);
	RightVec.Normalize();

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

// Used by the print catalog and HTML instructions functions.
void PieceInfo::RenderOnce(int nColor)
{
	AddRef();
	RenderPiece(nColor);
	DeRef();
}

// Called by the piece preview and from RenderOnce()
void PieceInfo::RenderPiece(int nColor)
{
	unsigned short sh;

	for (sh = 0; sh < m_nTextureCount; sh++)
	{
//		if (!m_pTextures[sh].texture->IsLoaded())
//			m_pTextures[sh].texture->Load(false);

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
		m_pTextures[sh].texture->MakeCurrent();

		if (m_pTextures[sh].color == LC_COL_DEFAULT)
			glColor3ubv(FlatColorArray[nColor]);
		if (nColor > 13 && nColor < 22)
		{
			glEnable (GL_BLEND);
			glDepthMask (GL_FALSE);
		}
		else
		{
			glDepthMask (GL_TRUE);
			glDisable (GL_BLEND);
		}

		glEnable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
		glTexCoord2fv(m_pTextures[sh].coords[0]);
		glVertex3fv(m_pTextures[sh].vertex[0]);
		glTexCoord2fv(m_pTextures[sh].coords[1]);
		glVertex3fv(m_pTextures[sh].vertex[1]);
		glTexCoord2fv(m_pTextures[sh].coords[2]);
		glVertex3fv(m_pTextures[sh].vertex[2]);
		glTexCoord2fv(m_pTextures[sh].coords[3]);
		glVertex3fv(m_pTextures[sh].vertex[3]);
		glEnd();
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
		if (Section->PrimitiveType == GL_LINES)
			continue;

		if (Section->ColorIndex == LC_COL_DEFAULT)
			colname = altcolornames[color];
		else
		{
			if (Section->ColorIndex >= LC_MAXCOLORS)
				continue;

			colname = altcolornames[Section->ColorIndex];
		}

		fprintf(file, "usemtl %s\n", colname);

		if (Section->PrimitiveType == GL_QUADS)
		{
			if (m_nFlags & LC_PIECE_LONGDATA)
			{
				u32* IndexPtr = (u32*)((char*)indices + Section->IndexOffset);
				for (int c = 0; c < Section->IndexCount; c += 4)
					fprintf(file, "f %ld %ld %ld %ld\n", IndexPtr[c+0] + *start, IndexPtr[c+1] + *start, IndexPtr[c+2] + *start, IndexPtr[c+3] + *start);
			}
			else
			{
				u16* IndexPtr = (u16*)((char*)indices + Section->IndexOffset);
				for (int c = 0; c < Section->IndexCount; c += 4)
					fprintf(file, "f %ld %ld %ld %ld\n", IndexPtr[c+0] + *start, IndexPtr[c+1] + *start, IndexPtr[c+2] + *start, IndexPtr[c+3] + *start);
			}
		}
		else if (Section->PrimitiveType == GL_TRIANGLES)
		{
			if (m_nFlags & LC_PIECE_LONGDATA)
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
	}

	m_Mesh->m_IndexBuffer->UnmapBuffer();

	*start += m_Mesh->m_VertexCount;
	fputs("\n", file);
}

void PieceInfo::WritePOV(FILE* f)
{
	char name[32], *ptr;
	strcpy(name, m_strName);
	while ((ptr = strchr(name, '-')))
		*ptr = '_';
	fprintf(f, "#declare lc_%s = union {\n", name);

	float* VertexPtr = (float*)m_Mesh->m_VertexBuffer->MapBuffer(GL_READ_ONLY_ARB);
	void* indices = m_Mesh->m_IndexBuffer->MapBuffer(GL_READ_ONLY_ARB);

	for (int i = 0; i < m_Mesh->m_SectionCount; i++)
	{
		lcMeshSection* Section = &m_Mesh->m_Sections[i];

		// Skip lines.
		if (Section->PrimitiveType == GL_LINES)
			continue;

		fputs(" mesh {\n", f);

		if (Section->PrimitiveType == GL_QUADS)
		{
			if (m_nFlags & LC_PIECE_LONGDATA)
			{
				u32* IndexPtr = (u32*)((char*)indices + Section->IndexOffset);
				for (int c = 0; c < Section->IndexCount; c += 4)
				{
					fprintf(f, "  triangle { <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f> }\n",
						-VertexPtr[IndexPtr[0]*3+1], -VertexPtr[IndexPtr[0]*3], VertexPtr[IndexPtr[0]*3+2],
						-VertexPtr[IndexPtr[1]*3+1], -VertexPtr[IndexPtr[1]*3], VertexPtr[IndexPtr[1]*3+2],
						-VertexPtr[IndexPtr[2]*3+1], -VertexPtr[IndexPtr[2]*3], VertexPtr[IndexPtr[2]*3+2]);
					fprintf(f, "  triangle { <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f> }\n",
						-VertexPtr[IndexPtr[2]*3+1], -VertexPtr[IndexPtr[2]*3], VertexPtr[IndexPtr[2]*3+2],
						-VertexPtr[IndexPtr[3]*3+1], -VertexPtr[IndexPtr[3]*3], VertexPtr[IndexPtr[3]*3+2],
						-VertexPtr[IndexPtr[0]*3+1], -VertexPtr[IndexPtr[0]*3], VertexPtr[IndexPtr[0]*3+2]);
					IndexPtr += 4;
				}
			}
			else
			{
				u16* IndexPtr = (u16*)((char*)indices + Section->IndexOffset);
				for (int c = 0; c < Section->IndexCount; c += 4)
				{
					fprintf(f, "  triangle { <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f> }\n",
						-VertexPtr[IndexPtr[0]*3+1], -VertexPtr[IndexPtr[0]*3], VertexPtr[IndexPtr[0]*3+2],
						-VertexPtr[IndexPtr[1]*3+1], -VertexPtr[IndexPtr[1]*3], VertexPtr[IndexPtr[1]*3+2],
						-VertexPtr[IndexPtr[2]*3+1], -VertexPtr[IndexPtr[2]*3], VertexPtr[IndexPtr[2]*3+2]);
					fprintf(f, "  triangle { <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f> }\n",
						-VertexPtr[IndexPtr[2]*3+1], -VertexPtr[IndexPtr[2]*3], VertexPtr[IndexPtr[2]*3+2],
						-VertexPtr[IndexPtr[3]*3+1], -VertexPtr[IndexPtr[3]*3], VertexPtr[IndexPtr[3]*3+2],
						-VertexPtr[IndexPtr[0]*3+1], -VertexPtr[IndexPtr[0]*3], VertexPtr[IndexPtr[0]*3+2]);
					IndexPtr += 4;
				}
			}
		}
		else if (Section->PrimitiveType == GL_TRIANGLES)
		{
			if (m_nFlags & LC_PIECE_LONGDATA)
			{
				u32* IndexPtr = (u32*)((char*)indices + Section->IndexOffset);
				for (int c = 0; c < Section->IndexCount; c += 3)
				{
					fprintf(f, "  triangle { <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f> }\n",
						-VertexPtr[IndexPtr[0]*3+1], -VertexPtr[IndexPtr[0]*3], VertexPtr[IndexPtr[0]*3+2],
						-VertexPtr[IndexPtr[1]*3+1], -VertexPtr[IndexPtr[1]*3], VertexPtr[IndexPtr[1]*3+2],
						-VertexPtr[IndexPtr[2]*3+1], -VertexPtr[IndexPtr[2]*3], VertexPtr[IndexPtr[2]*3+2]);
					IndexPtr += 3;
				}
			}
			else
			{
				u16* IndexPtr = (u16*)((char*)indices + Section->IndexOffset);
				for (int c = 0; c < Section->IndexCount; c += 3)
				{
					fprintf(f, "  triangle { <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f> }\n",
						-VertexPtr[IndexPtr[0]*3+1], -VertexPtr[IndexPtr[0]*3], VertexPtr[IndexPtr[0]*3+2],
						-VertexPtr[IndexPtr[1]*3+1], -VertexPtr[IndexPtr[1]*3], VertexPtr[IndexPtr[1]*3+2],
						-VertexPtr[IndexPtr[2]*3+1], -VertexPtr[IndexPtr[2]*3], VertexPtr[IndexPtr[2]*3+2]);
					IndexPtr += 3;
				}
			}
		}

		if (Section->ColorIndex != LC_COL_DEFAULT && Section->ColorIndex != LC_COL_EDGES)
			fprintf (f, "  texture { lg_%s }\n", lg_colors[Section->ColorIndex]);
		fputs(" }\n", f);
	}

	m_Mesh->m_VertexBuffer->UnmapBuffer();
	m_Mesh->m_IndexBuffer->UnmapBuffer();

	fputs("}\n\n", f);
}
