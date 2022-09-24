#include "lc_global.h"
#include "lc_math.h"
#include "lc_mesh.h"
#include "lc_meshloader.h"
#include "lc_colors.h"
#include "lc_texture.h"
#include "pieceinf.h"
#include "lc_library.h"
#include "lc_application.h"
#include "lc_model.h"
#include "project.h"
#include "lc_scene.h"
#include "lc_synth.h"
#include "lc_file.h"
#include <locale.h>

PieceInfo::PieceInfo()
{
	mZipFileType = lcZipFileType::Count;
	mZipFileIndex = -1;
	mFolderType = -1;
	mFolderIndex = -1;
	mState = lcPieceInfoState::Unloaded;
	mRefCount = 0;
	mType = lcPieceInfoType::Part;
	mMesh = nullptr;
	mModel = nullptr;
	mProject = nullptr;
	mSynthInfo = nullptr;
	mFileName[0] = 0;
	m_strDescription[0] = 0;
}

PieceInfo::~PieceInfo()
{
	delete mSynthInfo;

	if (mState == lcPieceInfoState::Loaded)
		Unload();
}

void PieceInfo::SetMesh(lcMesh* Mesh)
{
	mBoundingBox = Mesh->mBoundingBox;
	ReleaseMesh();
	mMesh = Mesh;
}

void PieceInfo::SetPlaceholder()
{
	lcMesh* Mesh = new lcMesh;
	Mesh->CreateBox();
	SetMesh(Mesh);

	mType = lcPieceInfoType::Placeholder;
	mModel = nullptr;
	mProject = nullptr;
}

void PieceInfo::SetModel(lcModel* Model, bool UpdateMesh, Project* CurrentProject, bool SearchProjectFolder)
{
	if (mModel != Model)
	{
		mType = lcPieceInfoType::Model;
		mModel = Model;
		delete mMesh;
		mMesh = nullptr;
	}

	strncpy(mFileName, Model->GetProperties().mFileName.toLatin1().data(), sizeof(mFileName) - 1);
	mFileName[sizeof(mFileName)-1] = 0;
	strncpy(m_strDescription, Model->GetProperties().mFileName.toLatin1().data(), sizeof(m_strDescription) - 1);
	m_strDescription[sizeof(m_strDescription)-1] = 0;

	const QStringList& MeshLines = Model->GetFileLines();

	if (UpdateMesh && !MeshLines.isEmpty())
	{
		lcMemFile PieceFile;

		for (const QString& Line : MeshLines)
		{
			QByteArray Buffer = Line.toLatin1();
			PieceFile.WriteBuffer(Buffer.constData(), Buffer.size());
			PieceFile.WriteBuffer("\r\n", 2);
		}

		lcLibraryMeshData MeshData;
		lcArray<lcMeshLoaderTextureMap> TextureStack;
		PieceFile.Seek(0, SEEK_SET);

		lcMeshLoader MeshLoader(MeshData, true, CurrentProject, SearchProjectFolder);
		const bool Ret = MeshLoader.LoadMesh(PieceFile, LC_MESHDATA_SHARED);

		if (Ret && !MeshData.IsEmpty())
			SetMesh(MeshData.CreateMesh());
	}
}

void PieceInfo::CreateProject(Project* Project, const char* PieceName)
{
	if (mProject != Project)
	{
		mType = lcPieceInfoType::Project;
		mProject = Project;
		mState = lcPieceInfoState::Loaded;
	}

	strncpy(mFileName, PieceName, sizeof(mFileName) - 1);
	mFileName[sizeof(mFileName) - 1] = 0;
	strncpy(m_strDescription, Project->GetFileName().toLatin1().data(), sizeof(m_strDescription) - 1);
	m_strDescription[sizeof(m_strDescription) - 1] = 0;
}

bool PieceInfo::IsProjectPiece() const
{
	if (mProject)
		return !strcmp(m_strDescription, mProject->GetFileName().toLatin1().data());
	return false;
}

bool PieceInfo::GetPieceWorldMatrix(lcPiece* Piece, lcMatrix44& WorldMatrix) const
{
	if (IsModel())
		return mModel->GetPieceWorldMatrix(Piece, WorldMatrix);

	return false;
}

bool PieceInfo::IncludesModel(const lcModel* Model) const
{
	if (IsModel())
	{
		if (mModel == Model)
			return true;

		return mModel->IncludesModel(Model);
	}

	return false;
}

void PieceInfo::CreatePlaceholder(const char* Name)
{
	strncpy(mFileName, Name, sizeof(mFileName));
	mFileName[sizeof(mFileName) - 1] = 0;
	strncpy(m_strDescription, Name, sizeof(m_strDescription));
	m_strDescription[sizeof(m_strDescription) - 1] = 0;

	SetPlaceholder();
}

void PieceInfo::Load()
{
	if (!IsModel() && !IsProject())
	{
		mState = lcPieceInfoState::Loading; // todo: mutex lock when changing load state

		if (IsPlaceholder())
		{
			if (lcGetPiecesLibrary()->LoadPieceData(this))
				mType = lcPieceInfoType::Part;
		}
		else
			lcGetPiecesLibrary()->LoadPieceData(this);
	}

	mState = lcPieceInfoState::Loaded;
}

void PieceInfo::ReleaseMesh()
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
		mMesh = nullptr;
	}
}

void PieceInfo::Unload()
{
	ReleaseMesh();
	mState = lcPieceInfoState::Unloaded;
	mModel = nullptr;

	if (IsModel())
		lcGetPiecesLibrary()->RemovePiece(this);
	else if (IsProject())
	{
		delete mProject;
		mProject = nullptr;
		lcGetPiecesLibrary()->RemovePiece(this);
	}
}

bool PieceInfo::MinIntersectDist(const lcVector3& Start, const lcVector3& End, float& MinDistance, lcPieceInfoRayTest& PieceInfoRayTest) const
{
	bool Intersect = false;

	if (IsPlaceholder() || IsModel() || IsProject())
	{
		float Distance;
		lcVector3 Plane;

		if (!lcBoundingBoxRayIntersectDistance(mBoundingBox.Min, mBoundingBox.Max, Start, End, &Distance, nullptr, &Plane) || (Distance >= MinDistance))
			return false;

		if (IsPlaceholder())
		{
			PieceInfoRayTest.Info = this;
			PieceInfoRayTest.Transform = lcMatrix44Identity();
			MinDistance = Distance;
			PieceInfoRayTest.Plane = Plane;
			return true;
		}

		if (IsModel())
			Intersect |= mModel->SubModelMinIntersectDist(Start, End, MinDistance, PieceInfoRayTest);
		else if (IsProject())
		{
			const lcModel* const Model = mProject->GetMainModel();
			if (Model)
				Intersect |= Model->SubModelMinIntersectDist(Start, End, MinDistance, PieceInfoRayTest);
		}
	}

	if (mMesh)
	{
		if (mMesh->MinIntersectDist(Start, End, MinDistance, PieceInfoRayTest.Plane))
		{
			PieceInfoRayTest.Info = this;
			PieceInfoRayTest.Transform = lcMatrix44Identity();
			Intersect = true;
		}
	}

	return Intersect;
}

bool PieceInfo::BoxTest(const lcMatrix44& WorldMatrix, const lcVector4 WorldPlanes[6]) const
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

	if (mMesh && mMesh->IntersectsPlanes(LocalPlanes))
		return true;

	if (IsModel())
		return mModel->SubModelBoxTest(LocalPlanes);
	else if (IsProject())
	{
		const lcModel* const Model = mProject->GetMainModel();
		return Model ? Model->SubModelBoxTest(LocalPlanes) : false;
	}

	return false;
}

void PieceInfo::ZoomExtents(float FoV, float AspectRatio, lcMatrix44& ProjectionMatrix, lcMatrix44& ViewMatrix) const
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

void PieceInfo::AddRenderMesh(lcScene& Scene)
{
	if (mMesh)
		Scene.AddMesh(mMesh, lcMatrix44Identity(), gDefaultColor, lcRenderMeshState::Default);
}

void PieceInfo::AddRenderMeshes(lcScene* Scene, const lcMatrix44& WorldMatrix, int ColorIndex, lcRenderMeshState RenderMeshState, bool ParentActive) const
{
	if (mMesh || IsPlaceholder())
		Scene->AddMesh(mMesh, WorldMatrix, ColorIndex, RenderMeshState);

	if (IsModel())
		mModel->AddSubModelRenderMeshes(Scene, WorldMatrix, ColorIndex, RenderMeshState, ParentActive);
	else if (IsProject())
	{
		const lcModel* const Model = mProject->GetMainModel();
		if (Model)
			Model->AddSubModelRenderMeshes(Scene, WorldMatrix, ColorIndex, RenderMeshState, ParentActive);
	}
}

void PieceInfo::GetPartsList(int DefaultColorIndex, bool ScanSubModels, bool AddSubModels, lcPartsList& PartsList) const
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

void PieceInfo::GetModelParts(const lcMatrix44& WorldMatrix, int DefaultColorIndex, std::vector<lcModelPartsEntry>& ModelParts) const
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

void PieceInfo::CompareBoundingBox(const lcMatrix44& WorldMatrix, lcVector3& Min, lcVector3& Max) const
{
	if (!IsModel())
	{
		lcVector3 Points[8];

		if (!mMesh)
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

void PieceInfo::AddSubModelBoundingBoxPoints(const lcMatrix44& WorldMatrix, std::vector<lcVector3>& Points) const
{
	if (!IsModel())
	{
		lcVector3 BoxPoints[8];

		if (!mMesh)
			lcGetBoxCorners(GetBoundingBox(), BoxPoints);
		else
			lcGetBoxCorners(mMesh->mBoundingBox, BoxPoints);

		for (int i = 0; i < 8; i++)
			Points.emplace_back(lcMul31(BoxPoints[i], WorldMatrix));
	}
	else
	{
		mModel->SubModelAddBoundingBoxPoints(WorldMatrix, Points);
	}
}

void PieceInfo::UpdateBoundingBox(std::vector<lcModel*>& UpdatedModels)
{
	if (IsModel())
		mModel->UpdatePieceInfo(UpdatedModels);
	else if (IsProject())
		mProject->UpdatePieceInfo(this);
}
