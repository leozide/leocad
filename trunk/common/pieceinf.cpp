// Information about how to draw a piece and some more stuff.
//

#include "lc_global.h"
#include "lc_math.h"
#include "lc_mesh.h"
#include "lc_colors.h"
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
#include "library.h"
#include "lc_application.h"

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

static void GetFrustumPlanes (float planes[6][4])
{
  // Storage for the Modelview, Projection and their multiplication (Frustum) matrix.
  float mv[16], pj[16], fm[16];

  glGetFloatv(GL_MODELVIEW_MATRIX, mv);
  glGetFloatv(GL_PROJECTION_MATRIX, pj);

  fm[0]  = pj[0] * mv[0]  + pj[4] * mv[1]  + pj[8]  * mv[2]  + pj[12] * mv[3];
  fm[4]  = pj[0] * mv[4]  + pj[4] * mv[5]  + pj[8]  * mv[6]  + pj[12] * mv[7];
  fm[8]  = pj[0] * mv[8]  + pj[4] * mv[9]  + pj[8]  * mv[10] + pj[12] * mv[11];
  fm[12] = pj[0] * mv[12] + pj[4] * mv[13] + pj[8]  * mv[14] + pj[12] * mv[15];
  fm[1]  = pj[1] * mv[0]  + pj[5] * mv[1]  + pj[9]  * mv[2]  + pj[13] * mv[3];
  fm[5]  = pj[1] * mv[4]  + pj[5] * mv[5]  + pj[9]  * mv[6]  + pj[13] * mv[7];
  fm[9]  = pj[1] * mv[8]  + pj[5] * mv[9]  + pj[9]  * mv[10] + pj[13] * mv[11];
  fm[13] = pj[1] * mv[12] + pj[5] * mv[13] + pj[9]  * mv[14] + pj[13] * mv[15];
  fm[2]  = pj[2] * mv[0]  + pj[6] * mv[1]  + pj[10] * mv[2]  + pj[14] * mv[3];
  fm[6]  = pj[2] * mv[4]  + pj[6] * mv[5]  + pj[10] * mv[6]  + pj[14] * mv[7];
  fm[10] = pj[2] * mv[8]  + pj[6] * mv[9]  + pj[10] * mv[10] + pj[14] * mv[11];
  fm[14] = pj[2] * mv[12] + pj[6] * mv[13] + pj[10] * mv[14] + pj[14] * mv[15];
  fm[3]  = pj[3] * mv[0]  + pj[7] * mv[1]  + pj[11] * mv[2]  + pj[15] * mv[3];
  fm[7]  = pj[3] * mv[4]  + pj[7] * mv[5]  + pj[11] * mv[6]  + pj[15] * mv[7];
  fm[11] = pj[3] * mv[8]  + pj[7] * mv[9]  + pj[11] * mv[10] + pj[15] * mv[11];
  fm[15] = pj[3] * mv[12] + pj[7] * mv[13] + pj[11] * mv[14] + pj[15] * mv[15];

  planes[0][0] = (fm[0] - fm[3]) * -1;
  planes[0][1] = (fm[4] - fm[7]) * -1;
  planes[0][2] = (fm[8] - fm[11]) * -1;
  planes[0][3] = (fm[12] - fm[15]) * -1;
  planes[1][0] = fm[0] + fm[3];
  planes[1][1] = fm[4] + fm[7];
  planes[1][2] = fm[8] + fm[11];
  planes[1][3] = fm[12] + fm[15];
  planes[2][0] = (fm[1] - fm[3]) * -1;
  planes[2][1] = (fm[5] - fm[7]) * -1;
  planes[2][2] = (fm[9] - fm[11]) * -1;
  planes[2][3] = (fm[13] - fm[15]) * -1;
  planes[3][0] = fm[1] + fm[3];
  planes[3][1] = fm[5] + fm[7];
  planes[3][2] = fm[9] + fm[11];
  planes[3][3] = fm[13] + fm[15];
  planes[4][0] = (fm[2] - fm[3]) * -1;
  planes[4][1] = (fm[6] - fm[7]) * -1;
  planes[4][2] = (fm[10] - fm[11]) * -1;
  planes[4][3] = (fm[14] - fm[15]) * -1;
  planes[5][0] = fm[2] + fm[3];
  planes[5][1] = fm[6] + fm[7];
  planes[5][2] = fm[10] + fm[11];
  planes[5][3] = fm[14] + fm[15];
}

bool BoxOutsideFrustum (float Dimensions[6])
{
  float d, planes[6][4], verts[8][3] = {
    { Dimensions[0], Dimensions[1], Dimensions[5] },
    { Dimensions[3], Dimensions[1], Dimensions[5] },
    { Dimensions[0], Dimensions[1], Dimensions[2] },
    { Dimensions[3], Dimensions[4], Dimensions[5] },
    { Dimensions[3], Dimensions[4], Dimensions[2] },
    { Dimensions[0], Dimensions[4], Dimensions[2] },
    { Dimensions[0], Dimensions[4], Dimensions[5] },
    { Dimensions[3], Dimensions[1], Dimensions[2] } };

  GetFrustumPlanes (planes);

  for (int i = 0; i < 6; i++)
    for (int j = 0; j < 8; j++)
    {
      d = verts[j][0]*planes[i][0] + verts[j][1]*planes[i][1] + verts[j][2]*planes[i][2] + planes[i][3];
      if (d < -0.001f)
	return true;
    }
  return false;
}

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
	mMesh = NULL;
}

PieceInfo::~PieceInfo()
{
	FreeInformation();
}

/////////////////////////////////////////////////////////////////////////////
// File I/O

void PieceInfo::LoadIndex(lcFile& file)
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
  m_nVertexCount = 0;
  m_nGroupCount = 0;
  m_pGroups = NULL;
  m_nBoxList = 0;

  file.ReadBuffer(m_strName, LC_PIECE_NAME_LEN);
  file.ReadBuffer(m_strDescription, 64);
  m_strDescription[64] = '\0';
  file.ReadS16(sh, 6);
  file.ReadU8(&m_nFlags, 1);
  lcuint32 Groups; file.ReadU32(&Groups, 1);
  file.ReadU32(&m_nOffset, 1);
  file.ReadU32(&m_nSize, 1);

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
}

void PieceInfo::CreatePlaceholder(const char* Name)
{
	m_nRef = 0;
	m_nVertexCount = 0;
	m_nGroupCount = 0;
	m_pGroups = NULL;
	m_nBoxList = 0;

	strncpy(m_strName, Name, sizeof(m_strName));
	m_strName[sizeof(m_strName)-1] = 0;
	strncpy(m_strDescription, Name, sizeof(m_strDescription));
	m_strDescription[sizeof(m_strDescription)-1] = 0;

	m_nFlags = LC_PIECE_PLACEHOLDER;
	m_nOffset = 0;
	m_nSize = 0;

	m_fDimensions[0] = 0.4f;
	m_fDimensions[1] = 0.4f;
	m_fDimensions[2] = 0.16f;
	m_fDimensions[3] = -0.4f;
	m_fDimensions[4] = -0.4f;
	m_fDimensions[5] = -0.96f;
}

void PieceInfo::AddRef()
{
	if (m_nRef == 0)
		LoadInformation();
	m_nRef++;
}

void PieceInfo::DeRef()
{
	m_nRef--;
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

inline lcuint16 EndianSwap(lcuint16 Val)
{
	return LCUINT16(Val);
}

inline lcuint32 EndianSwap(lcuint32 Val)
{
	return LCUINT32(Val);
}

template<typename SrcType, typename DstType>
static void WriteMeshDrawInfo(lcuint8*& Data, lcMesh* Mesh, float*& OutVertex, int* SectionIndices, lcMeshSection** DstSections)
{
	SrcType* SrcPtr = (SrcType*)Data;
	int NumColors = EndianSwap(*SrcPtr);
	SrcPtr++;

	for (int Color = 0; Color < NumColors; Color++)
	{
		int ColorIdx = lcGetColorIndex(EndianSwap(*SrcPtr));
		SrcPtr++;

		int NumQuads = EndianSwap(*SrcPtr);
		SrcPtr++;
		int NumTris = EndianSwap(*(SrcPtr + NumQuads));

		if (NumTris || NumQuads)
		{
			lcMeshSection* Section = DstSections[ColorIdx * 2 + 0];
			DstType* OutIndex = (DstType*)Mesh->mIndexBuffer.mData + Section->IndexOffset / sizeof(DstType) + Section->NumIndices;

			for (int i = 0; i < NumQuads; i += 4)
			{
				*OutIndex++ = EndianSwap(SrcPtr[0]);
				*OutIndex++ = EndianSwap(SrcPtr[1]);
				*OutIndex++ = EndianSwap(SrcPtr[2]);
				*OutIndex++ = EndianSwap(SrcPtr[0]);
				*OutIndex++ = EndianSwap(SrcPtr[2]);
				*OutIndex++ = EndianSwap(SrcPtr[3]);
				SrcPtr += 4;
			}

			SrcPtr++;

			for (int i = 0; i < NumTris; i++)
			{
				*OutIndex++ = EndianSwap(*SrcPtr);
				SrcPtr++;
			}

			Section->NumIndices += NumQuads / 4 * 6 + NumTris;
		}
		else
			SrcPtr++;

		int NumLines = EndianSwap(*SrcPtr);
		SrcPtr++;

		if (NumLines)
		{
			lcMeshSection* Section = DstSections[ColorIdx * 2 + 1];
			DstType* OutIndex = (DstType*)Mesh->mIndexBuffer.mData + Section->IndexOffset / sizeof(DstType) + Section->NumIndices;

			for (int i = 0; i < NumLines; i++)
			{
				*OutIndex++ = EndianSwap(*SrcPtr);
				SrcPtr++;
			}

			Section->NumIndices += NumLines;
		}
	}

	Data = (lcuint8*)SrcPtr;
}

template<class DstType>
static void WriteStudDrawInfo(int ColorIdx, const Matrix44& Mat, lcMesh* Mesh, float*& OutVertex, float Radius, int* SectionIndices, lcMeshSection** DstSections)
{
	// Build vertices.
	int BaseVertex = (OutVertex - (float*)Mesh->mVertexBuffer.mData) / 3;
	Vector3 Vert;

	for (int i = 0; i < SIDES; i++)
	{
		Vert = Mul31(Vector3(Radius * costbl[i], Radius * sintbl[i], 0.0f), Mat);
		*OutVertex++ = Vert[0];
		*OutVertex++ = Vert[1];
		*OutVertex++ = Vert[2];
		Vert = Mul31(Vector3(Radius * costbl[i], Radius * sintbl[i], LC_STUD_HEIGHT), Mat);
		*OutVertex++ = Vert[0];
		*OutVertex++ = Vert[1];
		*OutVertex++ = Vert[2];
	}

	Vert = Mul31(Vector3(0.0f, 0.0f, LC_STUD_HEIGHT), Mat);
	*OutVertex++ = Vert[0];
	*OutVertex++ = Vert[1];
	*OutVertex++ = Vert[2];

	int v0 = BaseVertex + 2 * SIDES;

	// Triangles.
	lcMeshSection* Section = DstSections[ColorIdx * 2 + 0];
	DstType* OutIndex = (DstType*)Mesh->mIndexBuffer.mData + Section->IndexOffset / sizeof(DstType) + Section->NumIndices;

	for (int i = 0; i < SIDES; i++)
	{
		int i1 = BaseVertex + (i % SIDES) * 2;
		int i2 = BaseVertex + ((i + 1) % SIDES) * 2;

		int v1 = i1;
		int v2 = i1 + 1;
		int v3 = i2;
		int v4 = i2 + 1;

		*OutIndex++ = v0;
		*OutIndex++ = v2;
		*OutIndex++ = v4;

		*OutIndex++ = v1;
		*OutIndex++ = v3;
		*OutIndex++ = v2;

		*OutIndex++ = v3;
		*OutIndex++ = v4;
		*OutIndex++ = v2;
	}

	Section->NumIndices += 9 * SIDES;

	// Lines.
	Section = DstSections[gEdgeColor * 2 + 1];
	OutIndex = (DstType*)Mesh->mIndexBuffer.mData + Section->IndexOffset / sizeof(DstType) + Section->NumIndices;

	for (int i = 0; i < SIDES; i++)
	{
		int i1 = BaseVertex + (i % SIDES) * 2;
		int i2 = BaseVertex + ((i + 1) % SIDES) * 2;

		int v1 = i1;
		int v2 = i1 + 1;
		int v3 = i2;
		int v4 = i2 + 1;

		*OutIndex++ = v1;
		*OutIndex++ = v3;

		*OutIndex++ = v2;
		*OutIndex++ = v4;
	}

	Section->NumIndices += 4 * SIDES;
}

template<class DstType>
static void WriteHollowStudDrawInfo(int ColorIdx, const Matrix44& Mat, lcMesh* Mesh, float*& OutVertex, float InnerRadius, float OuterRadius, int* SectionIndices, lcMeshSection** DstSections)
{
	// Build vertices.
	int BaseVertex = (OutVertex - (float*)Mesh->mVertexBuffer.mData) / 3;
	Vector3 Vert;

	for (int i = 0; i < SIDES; i++)
	{
		// Outside.
		Vert = Mul31(Vector3(OuterRadius * costbl[i], OuterRadius * sintbl[i], 0.0f), Mat);
		*OutVertex++ = Vert[0];
		*OutVertex++ = Vert[1];
		*OutVertex++ = Vert[2];
		Vert = Mul31(Vector3(OuterRadius * costbl[i], OuterRadius * sintbl[i], LC_STUD_HEIGHT), Mat);
		*OutVertex++ = Vert[0];
		*OutVertex++ = Vert[1];
		*OutVertex++ = Vert[2];

		// Inside.
		Vert = Mul31(Vector3(InnerRadius * costbl[i], InnerRadius * sintbl[i], LC_STUD_HEIGHT), Mat);
		*OutVertex++ = Vert[0];
		*OutVertex++ = Vert[1];
		*OutVertex++ = Vert[2];
		Vert = Mul31(Vector3(InnerRadius * costbl[i], InnerRadius * sintbl[i], 0.0f), Mat);
		*OutVertex++ = Vert[0];
		*OutVertex++ = Vert[1];
		*OutVertex++ = Vert[2];
	}

	// Triangles.
	lcMeshSection* Section = DstSections[ColorIdx * 2 + 0];
	DstType* OutIndex = (DstType*)Mesh->mIndexBuffer.mData + Section->IndexOffset / sizeof(DstType) + Section->NumIndices;

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

		*OutIndex++ = v1;
		*OutIndex++ = v5;
		*OutIndex++ = v2;

		*OutIndex++ = v5;
		*OutIndex++ = v6;
		*OutIndex++ = v2;

		*OutIndex++ = v2;
		*OutIndex++ = v6;
		*OutIndex++ = v3;

		*OutIndex++ = v6;
		*OutIndex++ = v7;
		*OutIndex++ = v3;

		*OutIndex++ = v3;
		*OutIndex++ = v7;
		*OutIndex++ = v4;

		*OutIndex++ = v7;
		*OutIndex++ = v8;
		*OutIndex++ = v4;
	}

	Section->NumIndices += 18 * SIDES;

	// Lines.
	Section = DstSections[gEdgeColor * 2 + 1];
	OutIndex = (DstType*)Mesh->mIndexBuffer.mData + Section->IndexOffset / sizeof(DstType) + Section->NumIndices;

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

		*OutIndex++ = v1;
		*OutIndex++ = v5;

		*OutIndex++ = v2;
		*OutIndex++ = v6;

		*OutIndex++ = v3;
		*OutIndex++ = v7;

		*OutIndex++ = v4;
		*OutIndex++ = v8;
	}

	Section->NumIndices += 8 * SIDES;
}

template<typename DstType>
void PieceInfo::BuildMesh(void* Data, int* SectionIndices)
{
	// Create empty sections.
	lcMeshSection** DstSections = new lcMeshSection*[gNumColors * 2];
	memset(DstSections, 0, sizeof(DstSections[0]) * gNumColors * 2);

	int IndexOffset = 0;
	int NumSections = 0;

	for (int ColorIdx = 0; ColorIdx < gNumColors; ColorIdx++)
	{
		if (SectionIndices[ColorIdx * 2 + 0])
		{
			lcMeshSection* Section = &mMesh->mSections[NumSections];
			DstSections[ColorIdx * 2 + 0] = Section;

			Section->PrimitiveType = GL_TRIANGLES;
			Section->ColorIndex = ColorIdx;
			Section->IndexOffset = IndexOffset;
			Section->NumIndices = 0;

			IndexOffset += SectionIndices[ColorIdx * 2 + 0] * sizeof(DstType);
			NumSections++;
		}

		if (SectionIndices[ColorIdx * 2 + 1])
		{
			lcMeshSection* Section = &mMesh->mSections[NumSections];
			DstSections[ColorIdx * 2 + 1] = Section;

			Section->PrimitiveType = GL_LINES;
			Section->ColorIndex = ColorIdx;
			Section->IndexOffset = IndexOffset;
			Section->NumIndices = 0;

			IndexOffset += SectionIndices[ColorIdx * 2 + 1] * sizeof(DstType);
			NumSections++;
		}
	}

	// Read groups
	lcuint32* longs = (lcuint32*)Data;
	int NumVertices = LCUINT32(*longs);
	float* OutVertex = (float*)mMesh->mVertexBuffer.mData + NumVertices * 3;
	lcuint8* bytes = (lcuint8*)(longs + 1);
	bytes += NumVertices * sizeof(lcint16) * 3;
	lcuint16 sh = m_nGroupCount;

	lcuint16 ConnectionCount = LCUINT16(*((lcuint16*)bytes));
	bytes += 2 + (1 + 6 * 2) * ConnectionCount;
	bytes++; // TextureCount 
	bytes += sizeof(lcuint16); // GroupCount

	while (sh--)
	{
		bytes += 1 + 2 * *bytes;

		switch (*bytes)
		{
		case LC_MESH:
			{
				bytes++;

				if (m_nFlags & LC_PIECE_LONGDATA_FILE)
					WriteMeshDrawInfo<lcuint32, DstType>(bytes, mMesh, OutVertex, SectionIndices, DstSections);
				else
					WriteMeshDrawInfo<lcuint16, DstType>(bytes, mMesh, OutVertex, SectionIndices, DstSections);
			} break;

		case LC_STUD:
		case LC_STUD3:
			{
				int ColorIdx = lcGetColorIndex(*(bytes+1));
				float* MatFloats = (float*)(bytes+2);

				for (int i = 0; i < 12; i++)
					MatFloats[i] = LCFLOAT(MatFloats[i]);

				Matrix44 Mat(Vector4(MatFloats[0], MatFloats[1], MatFloats[2], 0.0f),
				             Vector4(MatFloats[3], MatFloats[4], MatFloats[5], 0.0f),
				             Vector4(MatFloats[6], MatFloats[7], MatFloats[8], 0.0f),
				             Vector4(MatFloats[9], MatFloats[10], MatFloats[11], 1.0f));

				WriteStudDrawInfo<DstType>(ColorIdx, Mat, mMesh, OutVertex, LC_STUD_RADIUS, SectionIndices, DstSections);

				bytes += 2 + 12*sizeof(float);
			} break;

		case LC_STUD2:
		case LC_STUD4:
			{
				int ColorIdx = lcGetColorIndex(*(bytes+1));
				float* MatFloats = (float*)(bytes+2);

				for (int i = 0; i < 12; i++)
					MatFloats[i] = LCFLOAT(MatFloats[i]);

				Matrix44 Mat(Vector4(MatFloats[0], MatFloats[1], MatFloats[2], 0.0f),
				             Vector4(MatFloats[3], MatFloats[4], MatFloats[5], 0.0f),
				             Vector4(MatFloats[6], MatFloats[7], MatFloats[8], 0.0f),
				             Vector4(MatFloats[9], MatFloats[10], MatFloats[11], 1.0f));

				WriteHollowStudDrawInfo<DstType>(ColorIdx, Mat, mMesh, OutVertex, 0.16f, LC_STUD_RADIUS, SectionIndices, DstSections);

				bytes += 2 + 12*sizeof(float);
			} break;
		}
		bytes++; // should be 0
	}

	delete[] DstSections;
}

void PieceInfo::LoadInformation()
{
	if (m_nFlags & LC_PIECE_PLACEHOLDER)
	{
		mMesh->CreateBox();
		return;
	}

	lcDiskFile bin;
	char filename[LC_MAXPATH];
	void* buf;
	lcuint32 verts, *longs, fixverts;
	lcuint16 sh;
	lcuint8 *bytes, *tmp, bt;
	float scale, shift;
	lcint16* shorts;

	FreeInformation();

	// Open pieces.bin and buffer the information we need.
	strcpy (filename, lcGetPiecesLibrary()->GetLibraryPath());
	strcat (filename, "pieces.bin");
	if (!bin.Open (filename, "rb"))
		return;

	buf = malloc(m_nSize);
	bin.Seek(m_nOffset, SEEK_SET);
	bin.ReadBuffer(buf, m_nSize);

	shift  = 1.0f/(1<<14);
	scale = 0.01f;
	if (m_nFlags & LC_PIECE_MEDIUM) scale = 0.001f;
	if (m_nFlags & LC_PIECE_SMALL)  scale = 0.0001f;
	longs = (lcuint32*)buf;
	fixverts = verts = LCUINT32(*longs);
	bytes = (unsigned char*)(longs + 1);
	bytes += verts * sizeof(lcint16) * 3;

	lcuint16 ConnectionCount = LCUINT16(*((lcuint16*)bytes));
	bytes += 2 + (1 + 6 * 2) * ConnectionCount;
	bytes++; // TextureCount

	// Read groups.
	m_nGroupCount = LCUINT16(*((lcuint16*)bytes));
	bytes += sizeof(lcuint16);
	m_pGroups = (DRAWGROUP*)malloc(sizeof(DRAWGROUP)*m_nGroupCount);
	memset(m_pGroups, 0, sizeof(DRAWGROUP)*m_nGroupCount);

	// Count sections, vertices and indices.
	tmp = bytes;
	sh = m_nGroupCount;

	int NumSections = 0;
	int NumVertices = fixverts;
	int NumIndices = 0;
	int* SectionIndices = new int[gNumColors * 2];
	memset(SectionIndices, 0, sizeof(int) * gNumColors * 2);

	while (sh--)
	{
		bt = *bytes;
		bytes++;
		bytes += bt*sizeof(lcuint16);

		while (*bytes)
		{
			if (*bytes == LC_MESH)
			{
				if (m_nFlags & LC_PIECE_LONGDATA_FILE)
				{
					lcuint32 colors, *p;
					p = (lcuint32*)(bytes + 1);
					colors = LCUINT32(*p);
					p++;

					while (colors--)
					{
						int ColorIndex = lcGetColorIndex(*p);
						p++;

						int Triangles = LCUINT32(*p) / 4 * 6;
						p += LCUINT32(*p) + 1;
						Triangles += LCUINT32(*p);
						p += LCUINT32(*p) + 1;

						if (Triangles)
						{
							if (!SectionIndices[ColorIndex * 2 + 0])
								NumSections++;

							SectionIndices[ColorIndex * 2 + 0] += Triangles;
							NumIndices += Triangles;
						}

						int Lines = LCUINT32(*p);
						p += LCUINT32(*p) + 1;

						if (Lines)
						{
							if (!SectionIndices[ColorIndex * 2 + 1])
								NumSections++;

							SectionIndices[ColorIndex * 2 + 1] += Lines;
							NumIndices += Lines;
						}
					}

					bytes = (unsigned char*)p;
				}
				else
				{
					lcuint16 colors, *p;
					p = (lcuint16*)(bytes + 1);
					colors = LCUINT16(*p);
					p++;

					while (colors--)
					{
						int ColorIndex = lcGetColorIndex(*p);
						p++;

						int Triangles = LCUINT16(*p) / 4 * 6;
						p += LCUINT16(*p) + 1;
						Triangles += LCUINT16(*p);
						p += LCUINT16(*p) + 1;

						if (Triangles)
						{
							if (!SectionIndices[ColorIndex * 2 + 0])
								NumSections++;

							SectionIndices[ColorIndex * 2 + 0] += Triangles;
							NumIndices += Triangles;
						}

						int Lines = LCUINT16(*p);
						p += LCUINT16(*p) + 1;

						if (Lines)
						{
							if (!SectionIndices[ColorIndex * 2 + 1])
								NumSections++;

							SectionIndices[ColorIndex * 2 + 1] += Lines;
							NumIndices += Lines;
						}
					}

					bytes = (unsigned char*)p;
				}
			}
			else if ((*bytes == LC_STUD) || (*bytes == LC_STUD3))
			{
				int ColorIndex = lcGetColorIndex(*(bytes + 1));

				NumVertices += (2 * SIDES) + 1;

				if (!SectionIndices[ColorIndex * 2 + 0])
					NumSections++;

				SectionIndices[ColorIndex * 2 + 0] += 9 * SIDES;
				NumIndices += 9 * SIDES;

				if (!SectionIndices[gEdgeColor * 2 + 1])
					NumSections++;

				SectionIndices[gEdgeColor * 2 + 1] += 4 * SIDES;
				NumIndices += 4 * SIDES;

				bytes += 2 + 12 * sizeof(float);
			}
			else if ((*bytes == LC_STUD2) || (*bytes == LC_STUD4))
			{
				int ColorIndex = lcGetColorIndex(*(bytes + 1));

				NumVertices += 4 * SIDES;

				if (!SectionIndices[ColorIndex * 2 + 0])
					NumSections++;

				SectionIndices[ColorIndex * 2 + 0] += 18 * SIDES;
				NumIndices += 18 * SIDES;

				if (!SectionIndices[gEdgeColor * 2 + 1])
					NumSections++;

				SectionIndices[gEdgeColor * 2 + 1] += 8 * SIDES;
				NumIndices += 8 * SIDES;

				bytes += 2 + 12 * sizeof(float);
			}
		}
		bytes++; // should be 0
	}

	mMesh = new lcMesh();
	mMesh->Create(NumSections, NumVertices, NumIndices);

	m_nVertexCount = NumVertices;

	float* OutVertex = (float*)mMesh->mVertexBuffer.mData;

	shorts = (lcint16*)(longs + 1);
	for (verts = 0; verts < LCUINT32(*longs); verts++)
	{
		*OutVertex++ = (float)LCINT16(*shorts)*scale;
		shorts++;
		*OutVertex++ = (float)LCINT16(*shorts)*scale;
		shorts++;
		*OutVertex++ = (float)LCINT16(*shorts)*scale;
		shorts++;
	}

	if (NumVertices < 0x10000)
		BuildMesh<GLushort>(buf, SectionIndices);
	else
		BuildMesh<GLuint>(buf, SectionIndices);

	mMesh->UpdateBuffers();

	delete[] SectionIndices;

	free(buf);
}

void PieceInfo::FreeInformation()
{
	delete mMesh;
	mMesh = NULL;

	if (m_nBoxList != 0)
		glDeleteLists(m_nBoxList, 1);
	m_nBoxList = 0;

	if (m_pGroups != NULL)
	{
		while (m_nGroupCount--)
			if (m_pGroups[m_nGroupCount].drawinfo)
				free(m_pGroups[m_nGroupCount].drawinfo);

		free(m_pGroups);
		m_pGroups = NULL;
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

	lcVector3 FrontVec, RightVec, UpVec;

	// Calculate view matrix.
	UpVec = lcVector3(Top[0], Top[1], Top[2]);
	UpVec.Normalize();
	FrontVec = lcVector3(Front[0], Front[1], Front[2]);
	FrontVec.Normalize();
	RightVec = lcVector3(Side[0], Side[1], Side[2]);
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
	mMesh->Render(nColor, false, false);
}
