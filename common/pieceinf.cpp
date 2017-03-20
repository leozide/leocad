#include "lc_global.h"
#include "lc_math.h"
#include "lc_mesh.h"
#include "lc_colors.h"
#include "lc_texture.h"
#include "pieceinf.h"
#include "lc_library.h"
#include "lc_application.h"
#include "lc_model.h"
#include "project.h"
#include "lc_context.h"
#include "lc_synth.h"
#include "lc_file.h"
#include <locale.h>

PieceInfo::PieceInfo()
{
	mZipFileType = LC_NUM_ZIPFILES;
	mZipFileIndex = -1;
	mFlags = 0;
	mState = LC_PIECEINFO_UNLOADED;
	mRefCount = 0;
	mMesh = NULL;
	mModel = NULL;
	mProject = NULL;
	mSynthInfo = NULL;
}

PieceInfo::~PieceInfo()
{
	delete mSynthInfo;

	if (mState == LC_PIECEINFO_LOADED)
		Unload();
}

QString PieceInfo::GetSaveID() const
{
	if (mFlags & (LC_PIECE_MODEL | LC_PIECE_PROJECT))
		return QString::fromLatin1(m_strName);

	return QString::fromLatin1(m_strName) + QLatin1String(".DAT");
}

void PieceInfo::SetMesh(lcMesh* Mesh)
{
	mBoundingBox = Mesh->mBoundingBox;
	mMesh = Mesh;
}

void PieceInfo::SetPlaceholder()
{
	mBoundingBox.Min = lcVector3(-10.0f, -10.0f, -24.0f);
	mBoundingBox.Max = lcVector3(10.0f, 10.0f, 4.0f);

	mFlags = LC_PIECE_PLACEHOLDER | LC_PIECE_HAS_DEFAULT | LC_PIECE_HAS_LINES;
	mModel = NULL;
	mProject = NULL;

	delete mMesh;
	mMesh = NULL;
}

void PieceInfo::SetModel(lcModel* Model, bool UpdateMesh)
{
	if (mModel != Model)
	{
		mFlags = LC_PIECE_MODEL;
		mModel = Model;
	}

	strncpy(m_strName, Model->GetProperties().mName.toUpper().toLatin1().data(), sizeof(m_strName));
	m_strName[sizeof(m_strName)-1] = 0;
	strncpy(m_strDescription, Model->GetProperties().mName.toLatin1().data(), sizeof(m_strDescription));
	m_strDescription[sizeof(m_strDescription)-1] = 0;

	const QStringList& MeshLines = Model->GetFileLines();

	if (UpdateMesh && !MeshLines.isEmpty())
	{
		lcMemFile PieceFile;

		foreach(const QString& Line, MeshLines)
		{
			QByteArray Buffer = Line.toLatin1();
			PieceFile.WriteBuffer(Buffer.constData(), Buffer.size());
			PieceFile.WriteBuffer("\r\n", 2);
		}

		lcLibraryMeshData MeshData;
		lcArray<lcLibraryTextureMap> TextureStack;
		PieceFile.Seek(0, SEEK_SET);

		bool Ret = lcGetPiecesLibrary()->ReadMeshData(PieceFile, lcMatrix44Identity(), 16, false, TextureStack, MeshData, LC_MESHDATA_SHARED, true);

		if (Ret && !MeshData.IsEmpty())
			lcGetPiecesLibrary()->CreateMesh(this, MeshData);
	}
}

void PieceInfo::SetProject(Project* Project, const char* PieceName)
{
	if (mProject != Project)
	{
		mFlags = LC_PIECE_PROJECT;
		mProject = Project;
		mState = LC_PIECEINFO_LOADED;
	}

	strncpy(m_strName, PieceName, sizeof(m_strName));
	m_strName[sizeof(m_strName)-1] = 0;
	strncpy(m_strDescription, Project->GetFileName().toLatin1().data(), sizeof(m_strDescription));
	m_strDescription[sizeof(m_strDescription)-1] = 0;
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

	SetPlaceholder();
}

void PieceInfo::Load()
{
	if ((mFlags & (LC_PIECE_MODEL | LC_PIECE_PROJECT)) == 0)
	{
		mState = LC_PIECEINFO_LOADING; // todo: mutex lock when changing load state

		if (mFlags & LC_PIECE_PLACEHOLDER)
		{
			if (lcGetPiecesLibrary()->LoadPieceData(this))
				mFlags &= ~LC_PIECE_PLACEHOLDER;
			else
			{
				mFlags |= LC_PIECE_HAS_DEFAULT | LC_PIECE_HAS_LINES;
				mBoundingBox = gPlaceholderMesh->mBoundingBox;
			}
		}
		else
			lcGetPiecesLibrary()->LoadPieceData(this);
	}

	mState = LC_PIECEINFO_LOADED;
}

void PieceInfo::Unload()
{
	if (mMesh)
	{
		for (int LodIdx = 0; LodIdx < LC_NUM_MESH_LODS; LodIdx++)
		{
			for (int SectionIdx = 0; SectionIdx < mMesh->mLods[LodIdx].NumSections; SectionIdx++)
			{
				lcMeshSection& Section = mMesh->mLods[LodIdx].Sections[SectionIdx];

				if (Section.Texture)
					Section.Texture->Release();
			}
		}

		delete mMesh;
		mMesh = NULL;
	}

	mState = LC_PIECEINFO_UNLOADED;
	mModel = NULL;

	if (IsModel())
		lcGetPiecesLibrary()->RemovePiece(this);
	else if (IsProject())
	{
		delete mProject;
		mProject = NULL;
		lcGetPiecesLibrary()->RemovePiece(this);
	}
}

bool PieceInfo::MinIntersectDist(const lcVector3& Start, const lcVector3& End, float& MinDistance) const
{
	bool Intersect = false;

	if (mFlags & (LC_PIECE_PLACEHOLDER | LC_PIECE_MODEL | LC_PIECE_PROJECT))
	{
		float Distance;
		if (!lcBoundingBoxRayIntersectDistance(mBoundingBox.Min, mBoundingBox.Max, Start, End, &Distance, NULL) || (Distance >= MinDistance))
			return false;

		if (mFlags & LC_PIECE_PLACEHOLDER)
			return true;

		if (mFlags & LC_PIECE_MODEL)
			Intersect |= mModel->SubModelMinIntersectDist(Start, End, MinDistance);
		else if (mFlags & LC_PIECE_PROJECT)
		{
			lcModel* Model = mProject->GetMainModel();
			if (Model)
				Intersect |= Model->SubModelMinIntersectDist(Start, End, MinDistance);
		}
	}

	if (mMesh)
		Intersect = mMesh->MinIntersectDist(Start, End, MinDistance);

	return Intersect;
}

bool PieceInfo::BoxTest(const lcMatrix44& WorldMatrix, const lcVector4 WorldPlanes[6]) const
{
	lcMatrix44 InverseWorldMatrix = lcMatrix44AffineInverse(WorldMatrix);

	const int NumCorners = 8;
	const int NumPlanes = 6;
	lcVector4 LocalPlanes[NumPlanes];

	for (int PlaneIdx = 0; PlaneIdx < NumPlanes; PlaneIdx++)
	{
		lcVector3 PlaneNormal = lcMul30(WorldPlanes[PlaneIdx], InverseWorldMatrix);
		LocalPlanes[PlaneIdx] = lcVector4(PlaneNormal, WorldPlanes[PlaneIdx][3] - lcDot3(InverseWorldMatrix[3], PlaneNormal));
	}

	lcVector3 Box[NumCorners];
	lcGetBoxCorners(mBoundingBox, Box);

	int Outcodes[NumCorners];

	for (int CornerIdx = 0; CornerIdx < NumCorners; CornerIdx++)
	{
		Outcodes[CornerIdx] = 0;

		for (int PlaneIdx = 0; PlaneIdx < NumPlanes; PlaneIdx++)
		{
			if (lcDot3(Box[CornerIdx], LocalPlanes[PlaneIdx]) + LocalPlanes[PlaneIdx][3] > 0)
				Outcodes[CornerIdx] |= 1 << PlaneIdx;
		}
	}

	int OutcodesOR = 0, OutcodesAND = 0x3f;

	for (int CornerIdx = 0; CornerIdx < NumCorners; CornerIdx++)
	{
		OutcodesAND &= Outcodes[CornerIdx];
		OutcodesOR |= Outcodes[CornerIdx];
	}

	if (OutcodesAND != 0)
		return false;

	if (OutcodesOR == 0)
		return true;

	if (mFlags & LC_PIECE_PLACEHOLDER)
		return gPlaceholderMesh->IntersectsPlanes(LocalPlanes);

	if (mMesh && mMesh->IntersectsPlanes(LocalPlanes))
		return true;

	if (mFlags & LC_PIECE_MODEL)
		return mModel->SubModelBoxTest(LocalPlanes);
	else if (mFlags & LC_PIECE_PROJECT)
	{
		lcModel* Model = mProject->GetMainModel();
		return Model ? Model->SubModelBoxTest(LocalPlanes) : false;
	}

	return false;
}

// Zoom extents for the preview window and print catalog
void PieceInfo::ZoomExtents(const lcMatrix44& ProjectionMatrix, lcMatrix44& ViewMatrix, float* EyePos) const
{
	lcVector3 Points[8];
	lcGetBoxCorners(mBoundingBox, Points);

	lcVector3 Center = (mBoundingBox.Min + mBoundingBox.Max) / 2.0f;
	lcVector3 Position;

	if (EyePos)
		Position = lcVector3(EyePos[0], EyePos[1], EyePos[2]);
	else
		Position = lcVector3(-250.0f, -250.0f, 75.0f);
	Position += Center;

	lcMatrix44 ModelView = lcMatrix44LookAt(Position, Center, lcVector3(0, 0, 1));
	Position = lcZoomExtents(Position, ModelView, ProjectionMatrix, Points, 8);
	ViewMatrix = lcMatrix44LookAt(Position, Center, lcVector3(0, 0, 1));

	if (EyePos)
	{
		EyePos[0] = Position[0];
		EyePos[1] = Position[1];
		EyePos[2] = Position[2];
	}
}

void PieceInfo::AddRenderMesh(lcScene& Scene)
{
	if (!mMesh)
		return;

	lcRenderMesh& RenderMesh = Scene.mRenderMeshes.Add();

	RenderMesh.WorldMatrix = lcMatrix44Identity();
	RenderMesh.Mesh = mMesh;
	RenderMesh.ColorIndex = gDefaultColor;
	RenderMesh.State = LC_RENDERMESH_NONE;
	RenderMesh.Distance = fabsf(Scene.mViewMatrix.r[3].z);
	RenderMesh.LodIndex = mMesh->GetLodIndex(RenderMesh.Distance);

	if (mFlags & (LC_PIECE_HAS_SOLID | LC_PIECE_HAS_DEFAULT | LC_PIECE_HAS_LINES))
		Scene.mOpaqueMeshes.Add(Scene.mRenderMeshes.GetSize() - 1);

	if (mFlags & LC_PIECE_HAS_TRANSLUCENT)
		Scene.mTranslucentMeshes.Add(Scene.mRenderMeshes.GetSize() - 1);
}

void PieceInfo::AddRenderMeshes(lcScene& Scene, const lcMatrix44& WorldMatrix, int ColorIndex, bool Focused, bool Selected) const
{
	if (mMesh || (mFlags & LC_PIECE_PLACEHOLDER))
	{
		lcRenderMesh& RenderMesh = Scene.mRenderMeshes.Add();

		RenderMesh.WorldMatrix = WorldMatrix;
		RenderMesh.Mesh = (mFlags & LC_PIECE_PLACEHOLDER) ? gPlaceholderMesh : mMesh;
		RenderMesh.ColorIndex = ColorIndex;
		RenderMesh.State = Focused ? LC_RENDERMESH_FOCUSED : (Selected ? LC_RENDERMESH_SELECTED : LC_RENDERMESH_NONE);
		RenderMesh.Distance = fabsf(lcMul31(WorldMatrix[3], Scene.mViewMatrix).z);
		RenderMesh.LodIndex = RenderMesh.Mesh->GetLodIndex(RenderMesh.Distance);

		bool Translucent = lcIsColorTranslucent(ColorIndex);

		if ((mFlags & (LC_PIECE_HAS_SOLID | LC_PIECE_HAS_LINES)) || ((mFlags & LC_PIECE_HAS_DEFAULT) && !Translucent))
			Scene.mOpaqueMeshes.Add(Scene.mRenderMeshes.GetSize() - 1);

		if ((mFlags & LC_PIECE_HAS_TRANSLUCENT) || ((mFlags & LC_PIECE_HAS_DEFAULT) && Translucent))
			Scene.mTranslucentMeshes.Add(Scene.mRenderMeshes.GetSize() - 1);
	}

	if (mFlags & LC_PIECE_MODEL)
		mModel->SubModelAddRenderMeshes(Scene, WorldMatrix, ColorIndex, Focused, Selected);
	else if (mFlags & LC_PIECE_PROJECT)
	{
		lcModel* Model = mProject->GetMainModel();
		if (Model)
			Model->SubModelAddRenderMeshes(Scene, WorldMatrix, ColorIndex, Focused, Selected);
	}
}

void PieceInfo::GetPartsList(int DefaultColorIndex, lcPartsList& PartsList) const
{
	if (mFlags & LC_PIECE_MODEL)
		mModel->GetPartsList(DefaultColorIndex, PartsList);
	else if (mFlags & LC_PIECE_PROJECT)
	{
		lcModel* Model = mProject->GetMainModel();
		if (Model)
			Model->GetPartsList(DefaultColorIndex, PartsList);
	}
	else if (DefaultColorIndex < gNumUserColors)
		PartsList[this][DefaultColorIndex]++;
}

void PieceInfo::GetModelParts(const lcMatrix44& WorldMatrix, int DefaultColorIndex, lcArray<lcModelPartsEntry>& ModelParts) const
{
	if (mFlags & LC_PIECE_MODEL)
	{
		mModel->GetModelParts(WorldMatrix, DefaultColorIndex, ModelParts);
		return;
	}
	else if (mFlags & LC_PIECE_PROJECT)
	{
		lcModel* Model = mProject->GetMainModel();
		if (Model)
			Model->GetModelParts(WorldMatrix, DefaultColorIndex, ModelParts);
		return;
	}

	lcModelPartsEntry& ModelPartsEntry = ModelParts.Add();
	ModelPartsEntry.WorldMatrix = WorldMatrix;
	ModelPartsEntry.ColorIndex = DefaultColorIndex;
	ModelPartsEntry.Info = const_cast<PieceInfo*>(this);
}

void PieceInfo::UpdateBoundingBox(lcArray<lcModel*>& UpdatedModels)
{
	if (mFlags & LC_PIECE_MODEL)
		mModel->UpdatePieceInfo(UpdatedModels);
	else if (mFlags & LC_PIECE_PROJECT)
		mProject->UpdatePieceInfo(this);
}
