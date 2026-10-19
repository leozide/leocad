#include "lc_global.h"
#include "lc_math.h"
#include "lc_mesh.h"
#include "lc_colors.h"
#include "lc_texture.h"
#include "lc_piecedata.h"
#include "lc_string.h"
#include "lc_library.h"
#include "lc_application.h"
#include "lc_model.h"
#include "project.h"
#include "lc_scene.h"
#include "lc_synth.h"
#include "lc_traintrack.h"
#include <locale.h>

lcPieceData::lcPieceData(bool IsLibraryPiece)
	: mIsLibraryPiece(IsLibraryPiece)
{
	mZipFileType = lcZipFileType::Count;
	mZipFileIndex = -1;
	mFolderType = -1;
	mFolderIndex = -1;
	mState = lcAssetState::Unloaded;
	SetLoadingBoundingBox();
	mFileName[0] = 0;
	m_strDescription[0] = 0;
}

void lcPieceData::SetLoadingBoundingBox()
{
	// Keep these bounds in sync with lcMesh::CreateBox().
	mBoundingBox.Min = lcVector3(-10.0f, -10.0f, -24.0f);
	mBoundingBox.Max = lcVector3(10.0f, 10.0f, 4.0f);
}

lcPieceData::~lcPieceData()
{
	delete mTrainTrackInfo;

	Unload();
}

void lcPieceData::SetSynthInfo(lcSynthInfo* SynthInfo)
{
	mSynthInfo.reset(SynthInfo);
}

void lcPieceData::SetMesh(lcMesh* Mesh)
{
	mBoundingBox = Mesh->mBoundingBox;
	ReleaseMesh();
	mMesh = Mesh;
}

lcMesh* lcPieceData::GetDisplayMesh() const
{
	if (mState == lcAssetState::Loaded)
		return mMesh;

	if ((IsLoading() || mState == lcAssetState::Failed) &&
		((!IsModel() && !IsProject()) || (IsModel() && mModel->HasDirectGeometry())))
		return lcGetPiecesLibrary()->GetLoadingMesh();

	return nullptr;
}

void lcPieceData::SetFailed(QString Error)
{
	ReleaseMesh();
	if (!IsModel() && !IsProject())
		SetLoadingBoundingBox();

	mState = lcAssetState::Failed;

	if (IsModel())
	{
		if (const lcMesh* Mesh = GetDisplayMesh())
		{
			mBoundingBox.Min = lcMin(mBoundingBox.Min, Mesh->mBoundingBox.Min);
			mBoundingBox.Max = lcMax(mBoundingBox.Max, Mesh->mBoundingBox.Max);
		}
	}

	lcGetPiecesLibrary()->SetPieceDataLoadError(this, std::move(Error));
}

void lcPieceData::DetachContainer()
{
	Project* PreviousProject = IsProject() ? mProject : nullptr;
	mType = Type::Part;
	mModel = nullptr;
	mProject = nullptr;
	lcstrcpy(m_strDescription, mFileName);
	delete PreviousProject;

	ReleaseMesh();
	SetLoadingBoundingBox();
}

void lcPieceData::SetModel(lcModel* Model)
{
	if (mModel != Model)
	{
		// A submodel can take the name of a local external project. Keep this
		// lcPieceData so existing placements continue to refer to the same name.
		Project* PreviousProject = IsProject() ? mProject : nullptr;
		mType = Type::Model;
		mModel = Model;
		mProject = nullptr;
		delete PreviousProject;
		delete mMesh;
		mMesh = nullptr;
		mState = lcAssetState::Unloaded;
		lcGetPiecesLibrary()->ClearPieceDataLoadError(this);
	}

	lcGetPiecesLibrary()->SetModelPieceDataName(this, Model->GetProperties().mFileName.toLatin1().data());
}

void lcPieceData::CreateProject(Project* NewProject, const char* PieceName)
{
	if (mProject != NewProject)
	{
		lcGetPiecesLibrary()->ClearPieceDataLoadError(this);
		Project* PreviousProject = IsProject() ? mProject : nullptr;
		mType = Type::Project;
		mModel = nullptr;
		mProject = NewProject;
		mState = lcAssetState::Loaded;
		ReleaseMesh();
		delete PreviousProject;
	}

	lcstrcpy(mFileName, PieceName);
	lcstrcpy(m_strDescription, NewProject->GetFileName().toLatin1().data());
}

bool lcPieceData::IsProjectPiece() const
{
	if (mProject)
		return !strcmp(m_strDescription, mProject->GetFileName().toLatin1().data());
	return false;
}

bool lcPieceData::GetPieceWorldMatrix(lcPiece* Piece, lcMatrix44& WorldMatrix) const
{
	if (IsModel())
		return mModel->GetPieceWorldMatrix(Piece, WorldMatrix);

	return false;
}

bool lcPieceData::IncludesModel(const lcModel* Model) const
{
	if (IsModel())
	{
		if (mModel == Model)
			return true;

		return mModel->IncludesModel(Model);
	}

	return false;
}

void lcPieceData::CreatePart(const char* Name)
{
	lcstrcpy(mFileName, Name);
	lcstrcpy(m_strDescription, Name);
}

void lcPieceData::ReleaseMesh()
{
	if (mMesh)
	{
		delete mMesh;
		mMesh = nullptr;
	}
}

void lcPieceData::Unload()
{
	lcGetPiecesLibrary()->ClearPieceDataLoadError(this);

	ReleaseMesh();
	mState = lcAssetState::Unloaded;
	SetLoadingBoundingBox();
	mModel = nullptr;

	delete mProject;
	mProject = nullptr;
}

bool lcPieceData::MinIntersectDist(const lcVector3& Start, const lcVector3& End, float& MinDistance, lcPieceRayTest& PieceRayTest) const
{
	bool Intersect = false;
	lcMesh* Mesh = GetDisplayMesh();

	if (Mesh == lcGetPiecesLibrary()->GetLoadingMesh() || IsModel() || IsProject())
	{
		float Distance;
		lcVector3 Plane;

		if (!lcBoundingBoxRayIntersectDistance(mBoundingBox.Min, mBoundingBox.Max, Start, End, &Distance, nullptr, &Plane) || (Distance >= MinDistance))
			return false;

		if (Mesh == lcGetPiecesLibrary()->GetLoadingMesh() && !IsModel() && !IsProject())
		{
			PieceRayTest.PieceData = this;
			PieceRayTest.Transform = lcMatrix44Identity();
			MinDistance = Distance;
			PieceRayTest.Plane = Plane;
			return true;
		}

		if (IsModel())
			Intersect |= mModel->SubModelMinIntersectDist(Start, End, MinDistance, PieceRayTest);
		else if (IsProject())
		{
			const lcModel* const Model = mProject->GetMainModel();
			if (Model && Model->GetPieceData())
				Intersect |= Model->GetPieceData()->MinIntersectDist(Start, End, MinDistance, PieceRayTest);
		}
	}

	if (Mesh)
	{
		if (Mesh->MinIntersectDist(Start, End, MinDistance, PieceRayTest.Plane))
		{
			PieceRayTest.PieceData = this;
			PieceRayTest.Transform = lcMatrix44Identity();
			Intersect = true;
		}
	}

	return Intersect;
}

bool lcPieceData::BoxTest(const lcMatrix44& WorldMatrix, const lcVector4 WorldPlanes[6]) const
{
	lcMatrix44 InverseWorldMatrix = lcMatrix44AffineInverse(WorldMatrix);

	constexpr int NumCorners = 8;
	constexpr int NumPlanes = 6;
	lcVector4 LocalPlanes[NumPlanes];

	for (int PlaneIdx = 0; PlaneIdx < NumPlanes; PlaneIdx++)
	{
		const lcVector3 PlaneNormal = lcMul30(WorldPlanes[PlaneIdx], InverseWorldMatrix);
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

	lcMesh* Mesh = GetDisplayMesh();

	if (Mesh && Mesh->IntersectsPlanes(LocalPlanes))
		return true;

	if (IsModel())
		return mModel->SubModelBoxTest(LocalPlanes);
	else if (IsProject())
	{
		const lcModel* const Model = mProject->GetMainModel();
		return Model && Model->GetPieceData() ? Model->GetPieceData()->BoxTest(lcMatrix44Identity(), LocalPlanes) : false;
	}

	return false;
}

void lcPieceData::ZoomExtents(float FoV, float AspectRatio, lcMatrix44& ProjectionMatrix, lcMatrix44& ViewMatrix) const
{
	lcVector3 Points[8];
	lcGetBoxCorners(mBoundingBox, Points);

	const lcVector3 Center = (mBoundingBox.Min + mBoundingBox.Max) / 2.0f;
	lcVector3 Position = Center + lcVector3(100.0f, -100.0f, 75.0f);

	ProjectionMatrix = lcMatrix44Perspective(FoV, AspectRatio, 1.0f, 12500.0f);
	const lcMatrix44 ModelView = lcMatrix44LookAt(Position, Center, lcVector3(0, 0, 1));
	float FarDistance;
	std::tie(Position, FarDistance) = lcZoomExtents(Position, ModelView, ProjectionMatrix, Points, 8);
	ViewMatrix = lcMatrix44LookAt(Position, Center, lcVector3(0, 0, 1));
	ProjectionMatrix = lcMatrix44Perspective(FoV, AspectRatio, 1.0f, FarDistance);
}

void lcPieceData::AddRenderMesh(lcScene& Scene)
{
	if (mState != lcAssetState::Loaded)
	{
		if (Scene.GetRequireCompleteAssets())
			Scene.MarkMissingAssets();
		else if (lcMesh* Mesh = GetDisplayMesh())
			Scene.AddMesh(Mesh, lcMatrix44Identity(), gDefaultColor, lcRenderMeshState::Default);
	}
	else if (mMesh)
		Scene.AddMesh(mMesh, lcMatrix44Identity(), gDefaultColor, lcRenderMeshState::Default);
}

void lcPieceData::AddRenderMeshes(lcScene* Scene, const lcMatrix44& WorldMatrix, int ColorIndex, lcRenderMeshState RenderMeshState, bool ParentActive) const
{
	if (mState != lcAssetState::Loaded)
	{
		if (Scene->GetRequireCompleteAssets())
			Scene->MarkMissingAssets();
		else if (lcMesh* Mesh = GetDisplayMesh())
			Scene->AddMesh(Mesh, WorldMatrix, ColorIndex, RenderMeshState);
	}
	else if (mMesh)
		Scene->AddMesh(mMesh, WorldMatrix, ColorIndex, RenderMeshState);

	if (IsModel())
		mModel->AddSubModelRenderMeshes(Scene, WorldMatrix, ColorIndex, RenderMeshState, ParentActive);
	else if (IsProject())
	{
		const lcModel* const Model = mProject->GetMainModel();
		if (Model && Model->GetPieceData())
			Model->GetPieceData()->AddRenderMeshes(Scene, WorldMatrix, ColorIndex, RenderMeshState, ParentActive);
	}
}

void lcPieceData::GetPartsList(int DefaultColorIndex, bool ScanSubModels, bool AddSubModels, lcPartsList& PartsList) const
{
	if (IsModel())
	{
		if (ScanSubModels)
			mModel->GetPartsList(DefaultColorIndex, ScanSubModels, AddSubModels, PartsList);

		if (AddSubModels)
			PartsList[this][DefaultColorIndex]++;
	}
	else if (IsProject() && !IsProjectPiece())
	{
		const lcModel* const Model = mProject->GetMainModel();
		if (Model)
			Model->GetPartsList(DefaultColorIndex, ScanSubModels, AddSubModels, PartsList);
	}
	else
		PartsList[this][DefaultColorIndex]++;
}

void lcPieceData::GetModelParts(const lcMatrix44& WorldMatrix, int DefaultColorIndex, std::vector<lcModelPartsEntry>& ModelParts) const
{
	if (IsModel())
	{
		mModel->GetModelParts(WorldMatrix, DefaultColorIndex, ModelParts);
		return;
	}
	else if (IsProject())
	{
		const lcModel* const Model = mProject->GetMainModel();
		if (Model)
			Model->GetModelParts(WorldMatrix, DefaultColorIndex, ModelParts);
		return;
	}

	ModelParts.emplace_back(lcModelPartsEntry{ WorldMatrix, this, nullptr, DefaultColorIndex });
}

void lcPieceData::CompareBoundingBox(const lcMatrix44& WorldMatrix, lcVector3& Min, lcVector3& Max) const
{
	if (!IsModel())
	{
		lcVector3 Points[8];

		if (!mMesh || IsLoading())
			lcGetBoxCorners(GetBoundingBox(), Points);
		else
			lcGetBoxCorners(mMesh->mBoundingBox, Points);

		for (int i = 0; i < 8; i++)
		{
			const lcVector3 Point = lcMul31(Points[i], WorldMatrix);

			Min = lcMin(Point, Min);
			Max = lcMax(Point, Max);
		}
	}
	else
	{
		mModel->SubModelCompareBoundingBox(WorldMatrix, Min, Max);
	}
}

void lcPieceData::AddSubModelBoundingBoxPoints(const lcMatrix44& WorldMatrix, std::vector<lcVector3>& Points, lcGeometryBoundsMode Mode) const
{
	if (Mode == lcGeometryBoundsMode::AvailableOnly)
	{
		if (IsModel())
		{
			mModel->SubModelAddBoundingBoxPoints(WorldMatrix, Points, Mode);
			return;
		}

		if (IsProject())
		{
			if (const lcModel* Model = mProject->GetMainModel())
				Model->SubModelAddBoundingBoxPoints(WorldMatrix, Points, Mode);

			return;
		}

		if (mState != lcAssetState::Loaded || !mMesh)
			return;
	}

	if (!IsModel())
	{
		lcVector3 BoxPoints[8];

		if (!mMesh || IsLoading())
			lcGetBoxCorners(GetBoundingBox(), BoxPoints);
		else
			lcGetBoxCorners(mMesh->mBoundingBox, BoxPoints);

		for (int i = 0; i < 8; i++)
			Points.emplace_back(lcMul31(BoxPoints[i], WorldMatrix));
	}
	else
	{
		mModel->SubModelAddBoundingBoxPoints(WorldMatrix, Points, Mode);
	}
}

void lcPieceData::UpdateBoundingBox(std::vector<lcModel*>& UpdatedModels)
{
	if (IsModel())
		mModel->UpdatePieceData(UpdatedModels);
	else if (IsProject())
		mProject->UpdatePieceData(this);
}
