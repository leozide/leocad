#include "lc_global.h"
#include "lc_math.h"
#include "lc_mesh.h"
#include "lc_colors.h"
#include "lc_texture.h"
#include "opengl.h"
#include "pieceinf.h"
#include "lc_library.h"
#include "lc_application.h"

PieceInfo::PieceInfo(int ZipFileIndex)
{
	mZipFileIndex = ZipFileIndex;
	mFlags = 0;
	mMesh = NULL;
	mRefCount = 0;
	m_nBoxList = 0;
}

PieceInfo::~PieceInfo()
{
	Unload();
}

void PieceInfo::CreatePlaceholder(const char* Name)
{
	strncpy(m_strName, Name, sizeof(m_strName));
	m_strName[sizeof(m_strName)-1] = 0;
	strncpy(m_strDescription, Name, sizeof(m_strDescription));
	m_strDescription[sizeof(m_strDescription)-1] = 0;

	mFlags = LC_PIECE_PLACEHOLDER;
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

void PieceInfo::Load()
{
	if (mFlags & LC_PIECE_PLACEHOLDER)
	{
		mMesh = new lcMesh();
		mMesh->CreateBox();

		mFlags |= LC_PIECE_HAS_DEFAULT | LC_PIECE_HAS_LINES;

		m_fDimensions[0] = 0.4f;
		m_fDimensions[1] = 0.4f;
		m_fDimensions[2] = 0.16f;
		m_fDimensions[3] = -0.4f;
		m_fDimensions[4] = -0.4f;
		m_fDimensions[5] = -0.96f;
	}
	else
	{
		lcGetPiecesLibrary()->LoadPiece(this);
	}
}

void PieceInfo::Unload()
{
	for (int SectionIdx = 0; SectionIdx < mMesh->mNumSections; SectionIdx++)
	{
		lcMeshSection& Section = mMesh->mSections[SectionIdx];

		if (Section.Texture)
			Section.Texture->Release();
	}

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
