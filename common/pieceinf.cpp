#include "lc_global.h"
#include "lc_math.h"
#include "lc_mesh.h"
#include "lc_colors.h"
#include "lc_texture.h"
#include "opengl.h"
#include "pieceinf.h"
#include "lc_library.h"
#include "lc_application.h"
#include "lc_model.h"
#include "lc_context.h"
#include "camera.h"

PieceInfo::PieceInfo()
{
	mZipFileType = LC_NUM_ZIPFILES;
	mZipFileIndex = -1;
	mFlags = 0;
	mMesh = NULL;
	mRefCount = 0;
	mModel = NULL;
}

PieceInfo::~PieceInfo()
{
	if (mRefCount)
		Unload();
}

void PieceInfo::SetModel(lcModel* Model)
{
	m_strName[0] = 0;
	m_strDescription[0] = 0;

	mFlags = LC_PIECE_MODEL;
	mModel = Model;

	delete mMesh;
	mMesh = NULL;
}

bool PieceInfo::IncludesModel(const lcModel* Model) const
{
	if (mFlags & LC_PIECE_MODEL)
	{
		if (mModel == Model)
			return true;

		return mModel->IncludesModel(Model);
	}

	return false;
}

void PieceInfo::CreatePlaceholder(const char* Name)
{
	strncpy(m_strName, Name, sizeof(m_strName));
	m_strName[sizeof(m_strName)-1] = 0;
	strncpy(m_strDescription, Name, sizeof(m_strDescription));
	m_strDescription[sizeof(m_strDescription)-1] = 0;

	mFlags = LC_PIECE_PLACEHOLDER;
}

void PieceInfo::Load()
{
	if (mFlags & LC_PIECE_PLACEHOLDER)
	{
		mMesh = new lcMesh();
		mMesh->CreateBox();

		mFlags |= LC_PIECE_HAS_DEFAULT | LC_PIECE_HAS_LINES;

		m_fDimensions[0] = 10.0f;
		m_fDimensions[1] = 10.0f;
		m_fDimensions[2] = 4.0f;
		m_fDimensions[3] = -10.0f;
		m_fDimensions[4] = -10.0f;
		m_fDimensions[5] = -24.0f;
	}
	else
		lcGetPiecesLibrary()->LoadPiece(this);
}

void PieceInfo::Unload()
{
	if (mMesh)
	{
		for (int SectionIdx = 0; SectionIdx < mMesh->mNumSections; SectionIdx++)
		{
			lcMeshSection& Section = mMesh->mSections[SectionIdx];

			if (Section.Texture)
				Section.Texture->Release();
		}

		delete mMesh;
		mMesh = NULL;
	}
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
		Position = lcVector3(-250.0f, -250.0f, 75.0f);
	Position += Center;

	lcMatrix44 Projection = lcMatrix44Perspective(30.0f, Aspect, 1.0f, 2500.0f);
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

void PieceInfo::AddRenderMeshes(lcScene& Scene, const lcMatrix44& WorldMatrix, int ColorIndex, bool Focused, bool Selected)
{
	if (mFlags & LC_PIECE_MODEL)
	{
		mModel->AddRenderMeshes(Scene, WorldMatrix, ColorIndex, Focused, Selected);
		return;
	}

	lcRenderMesh RenderMesh;

	RenderMesh.WorldMatrix = WorldMatrix;
	RenderMesh.Mesh = mMesh;
	RenderMesh.ColorIndex = ColorIndex;
	RenderMesh.Focused = Focused;
	RenderMesh.Selected = Selected;

	bool Translucent = lcIsColorTranslucent(ColorIndex);

	if ((mFlags & (LC_PIECE_HAS_SOLID | LC_PIECE_HAS_LINES)) || ((mFlags & LC_PIECE_HAS_DEFAULT) && !Translucent))
		Scene.OpaqueMeshes.Add(RenderMesh);

	if ((mFlags & LC_PIECE_HAS_TRANSLUCENT) || ((mFlags & LC_PIECE_HAS_DEFAULT) && Translucent))
	{
		lcVector3 Pos = lcMul31(WorldMatrix[3], Scene.Camera->mWorldView);

		RenderMesh.Distance = Pos[2];

		Scene.TranslucentMeshes.Add(RenderMesh);
	}
}

void PieceInfo::AddRenderMeshes(const lcMatrix44& ViewMatrix, const lcMatrix44& WorldMatrix, int ColorIndex, bool Focused, bool Selected, lcArray<lcRenderMesh>& OpaqueMeshes, lcArray<lcRenderMesh>& TranslucentMeshes)
{
	lcRenderMesh RenderMesh;

	RenderMesh.WorldMatrix = WorldMatrix;
	RenderMesh.Mesh = mMesh;
	RenderMesh.ColorIndex = ColorIndex;
	RenderMesh.Focused = Focused;
	RenderMesh.Selected = Selected;

	bool Translucent = lcIsColorTranslucent(ColorIndex);

	if ((mFlags & (LC_PIECE_HAS_SOLID | LC_PIECE_HAS_LINES)) || ((mFlags & LC_PIECE_HAS_DEFAULT) && !Translucent))
		OpaqueMeshes.Add(RenderMesh);

	if ((mFlags & LC_PIECE_HAS_TRANSLUCENT) || ((mFlags & LC_PIECE_HAS_DEFAULT) && Translucent))
	{
		lcVector3 Pos = lcMul31(WorldMatrix[3], ViewMatrix);

		RenderMesh.Distance = Pos[2];

		TranslucentMeshes.Add(RenderMesh);
	}
}
