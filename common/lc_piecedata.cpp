#include "lc_global.h"
#include "lc_math.h"
#include "lc_mesh.h"
#include "lc_colors.h"
#include "lc_piecedata.h"
#include "lc_string.h"
#include "lc_library.h"
#include "lc_application.h"
#include "lc_model.h"
#include "project.h"
#include "lc_scene.h"
#include "lc_synth.h"
#include "lc_traintrack.h"

lcPieceData::lcPieceData(bool IsLibraryPiece)
	: mIsLibraryPiece(IsLibraryPiece)
{
	mZipFileType = lcZipFileType::Count;
	SetLoadingBoundingBox();
}

bool lcPieceData::HasDecorationDescription(const char* Description)
{
	// Assemblies and intrinsically printed parts may have no pattern suffix.
	std::string PreviousWord;
	bool With = false;

	for (const char* Position = Description; *Position;)
	{
		std::string Word;

		while ((*Position >= 'A' && *Position <= 'Z') || (*Position >= 'a' && *Position <= 'z'))
		{
			char Character = *Position++;

			if (Character >= 'a' && Character <= 'z')
				Character += 'A' - 'a';

			Word.push_back(Character);
		}

		if (Word.empty())
		{
			Position++;
			continue;
		}

		if (PreviousWord != "WITHOUT" && (Word == "PATTERN" || Word == "PATTERNS" || Word == "PATTERNED" || (With && (Word == "STICKER" || Word == "STICKERS"))))
			return true;

		if (Word == "WITH")
			With = true;
		else if (Word == "WITHOUT")
			With = false;

		PreviousWord = std::move(Word);
	}

	return false;
}

std::vector<std::string> lcPieceData::Classify(std::string_view Name)
{
	mPatternParentId.clear();
	mFlags = Flags();

	if (!IsLibraryPiece())
		return std::vector<std::string>();

	if (m_strDescription[0] == '=')
		mFlags |= Flag::Alias;
	else if (m_strDescription[0] == '~')
		mFlags |= Flag::SubPiece;

	if (HasDecorationDescription(m_strDescription))
		mFlags |= Flag::Decorated;

	if (IsAlias())
		return std::vector<std::string>();

	if (Name.size() <= 4 || Name.compare(Name.size() - 4, 4, ".DAT") != 0)
		return std::vector<std::string>();

	const std::string_view Stem = Name.substr(0, Name.size() - 4);

	const size_t NumberStart = Stem[0] == 'U' || Stem[0] == 'T' || Stem[0] == 'S' ? 1 : 0;
	const size_t NumberEnd = Stem.find_first_not_of("0123456789", NumberStart);

	if (NumberStart == NumberEnd)
		return std::vector<std::string>();

	std::vector<std::string> ParentIds;

	// P denotes a printed pattern; D denotes a sticker shortcut. Preserve base variants.
	for (size_t Position = Stem.find_last_of("PD"); Position != std::string_view::npos; Position = Position ? Stem.find_last_of("PD", Position - 1) : std::string_view::npos)
	{
		if (Position == 0 || Position + 1 == Stem.size())
			continue;

		if (Stem.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789", Position + 1) != std::string_view::npos)
			continue;

		mFlags |= Flag::Decorated;

		ParentIds.emplace_back(Stem.substr(0, Position));
		ParentIds.back() += ".DAT";
	}

	return ParentIds;
}

bool lcPieceData::InitializeFromCache(const CacheData& CacheData)
{
	const int KnownFlags = static_cast<int>(Flag::Decorated) | static_cast<int>(Flag::Alias) | static_cast<int>(Flag::SubPiece);

	if (CacheData.Description.size() >= sizeof(m_strDescription) || CacheData.PatternParentId.size() >= LC_PIECE_NAME_LEN || (CacheData.Flags & ~KnownFlags) != 0)
		return false;

	lcstrcpy(m_strDescription, CacheData.Description.c_str());
	mPatternParentId = CacheData.PatternParentId;
	mFlags = Flags(static_cast<Flag>(CacheData.Flags));

	return true;
}

lcPieceData::CacheData lcPieceData::GetCacheData() const
{
	return { m_strDescription, mPatternParentId, static_cast<quint8>(static_cast<int>(mFlags)) };
}

void lcPieceData::SetAliasTarget(const lcPieceData* Target)
{
	if (!IsAlias())
		return;

	mPatternParentId = Target ? Target->GetPatternParentId() : std::string();
	mFlags.setFlag(Flag::Decorated, HasDecorationDescription(m_strDescription) || (Target && Target->IsDecorated()));
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

	const QByteArray Name = Model->GetProperties().mFileName.toLatin1();
	lcstrcpy(mFileName, Name.constData());
	lcstrcpy(m_strDescription, Name.constData());
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
			PieceRayTest.Normal = Plane;
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
		if (Mesh->MinIntersectDist(Start, End, MinDistance, PieceRayTest.Plane, PieceRayTest.Normal))
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
