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
#include "lc_library.h"
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

/////////////////////////////////////////////////////////////////////////////
// PieceInfo construction/destruction

PieceInfo::PieceInfo()
{
	m_nRef = 0;
	m_nBoxList = 0;
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
      sintbl[i] = (float)sin((LC_2PI*i)/(SIDES));
      costbl[i] = (float)cos((LC_2PI*i)/(SIDES));
    }
    init = true;
  }

  file.ReadBuffer(m_strName, LC_PIECE_NAME_LEN);
  file.ReadBuffer(m_strDescription, 64);
  m_strDescription[64] = '\0';
  file.ReadS16(sh, 6);
  lcuint8 Flags;
  file.ReadU8(&Flags, 1);
  m_nFlags = Flags;
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

void PieceInfo::LoadInformation()
{
	if (m_nFlags & LC_PIECE_PLACEHOLDER)
	{
		mMesh = new lcMesh();
		mMesh->CreateBox();
	}
	else
	{
		FreeInformation();
		lcGetPiecesLibrary()->LoadPiece(m_strName);
	}
}

void PieceInfo::FreeInformation()
{
	delete mMesh;
	mMesh = NULL;

	if (m_nBoxList != 0)
		glDeleteLists(m_nBoxList, 1);
	m_nBoxList = 0;
}

// Zoom extents for the preview window and print catalog
void PieceInfo::ZoomExtents(float Fov, float Aspect, float* EyePos) const
{
	lcVector3 Points[8] =
	{
		lcVector3(m_fDimensions[0], m_fDimensions[1], m_fDimensions[5]),
		lcVector3(m_fDimensions[3], m_fDimensions[1], m_fDimensions[5]),
		lcVector3(m_fDimensions[0], m_fDimensions[1], m_fDimensions[2]),
		lcVector3(m_fDimensions[3], m_fDimensions[4], m_fDimensions[5]),
		lcVector3(m_fDimensions[3], m_fDimensions[4], m_fDimensions[2]),
		lcVector3(m_fDimensions[0], m_fDimensions[4], m_fDimensions[2]),
		lcVector3(m_fDimensions[0], m_fDimensions[4], m_fDimensions[5]),
		lcVector3(m_fDimensions[3], m_fDimensions[1], m_fDimensions[2])
	};

	lcVector3 Center(GetCenter());
	lcVector3 Position;

	if (EyePos)
		Position = lcVector3(EyePos[0], EyePos[1], EyePos[2]);
	else
		Position = lcVector3(-10.0f, -10.0f, 5.0f);
	Position += Center;

	lcMatrix44 Projection = lcMatrix44Perspective(30.0f, Aspect, 1.0f, 100.0f);
	lcMatrix44 ModelView = lcMatrix44LookAt(Position, Center, lcVector3(0, 0, 1));
	Position = lcZoomExtents(Position, ModelView, Projection, Points, 8);

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(Projection);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(lcMatrix44LookAt(Position, Center, lcVector3(0, 0, 1)));

	if (EyePos)
	{
		EyePos[0] = Position[0];
		EyePos[1] = Position[1];
		EyePos[2] = Position[2];
	}
}

void PieceInfo::RenderPiece(int nColor)
{
	mMesh->Render(nColor, false, false);
}
