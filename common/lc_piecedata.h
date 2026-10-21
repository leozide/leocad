#pragma once

#include "lc_assettypes.h"
#include "lc_math.h"

#define LC_PIECE_NAME_LEN 256

struct lcModelPartsEntry
{
	lcMatrix44 WorldMatrix;
	const lcPieceData* PieceData;
	// Borrowed explicit mesh for generated/direct geometry; null uses PieceData->GetMesh().
	lcMesh* Mesh;
	int ColorIndex;
};

class lcPieceData
{
public:
	explicit lcPieceData(bool IsLibraryPiece);
	~lcPieceData();

	lcPieceData(const lcPieceData&) = delete;
	lcPieceData(lcPieceData&&) = delete;
	lcPieceData& operator=(const lcPieceData&) = delete;
	lcPieceData& operator=(lcPieceData&&) = delete;

	const lcBoundingBox& GetBoundingBox() const
	{
		return mBoundingBox;
	}

	void SetBoundingBox(const lcVector3& Min, const lcVector3& Max)
	{
		mBoundingBox.Min = Min;
		mBoundingBox.Max = Max;
	}

	lcSynthInfo* GetSynthInfo() const
	{
		return mSynthInfo.get();
	}

	std::shared_ptr<const lcSynthInfo> GetSynthInfoShared() const
	{
		return mSynthInfo;
	}

	void SetSynthInfo(lcSynthInfo* SynthInfo);

	lcTrainTrackInfo* GetTrainTrackInfo() const
	{
		return mTrainTrackInfo;
	}

	void SetTrainTrackInfo(lcTrainTrackInfo* TrainTrackInfo)
	{
		mTrainTrackInfo = TrainTrackInfo;
	}

	lcMesh* GetMesh() const
	{
		return mMesh;
	}

	lcModel* GetModel() const
	{
		return mModel;
	}

	Project* GetProject() const
	{
		return mProject;
	}

	void SetMesh(lcMesh* Mesh);
	// Interactive fallback geometry is never owned by this piece or returned by GetMesh().
	lcMesh* GetDisplayMesh() const;
	void SetFailed(QString Error);
	void ReleaseMesh();

	int AddRef()
	{
		mRefCount++;
		return mRefCount;
	}

	int Release()
	{
		mRefCount--;
		return mRefCount;
	}

	int GetRefCount() const
	{
		return mRefCount;
	}

	bool IsLibraryPiece() const
	{
		return mIsLibraryPiece;
	}

	bool IsLoading() const
	{
		return mState == lcAssetState::Loading;
	}

	bool IsModel() const
	{
		return mType == Type::Model;
	}

	bool IsProject() const
	{
		return mType == Type::Project;
	}

	void SetZipFile(lcZipFileType ZipFileType, int ZipFileIndex)
	{
		mZipFileType = ZipFileType;
		mZipFileIndex = ZipFileIndex;
	}

	struct CacheData
	{
		std::string Description;
		std::string PatternParentId;
		quint8 Flags;
	};

	// Restores or snapshots the description and classification as one cache record.
	bool InitializeFromCache(const CacheData& CacheData);
	CacheData GetCacheData() const;

	// Inherits the resolved target's decoration and parent without retaining the target.
	void SetAliasTarget(const lcPieceData* Target);

	// Updates local flags from the normalized name and description, returning candidate parent IDs.
	// Resolving those IDs and alias targets belongs to the library.
	std::vector<std::string> Classify(std::string_view Name);

	bool IsDecorated() const
	{
		return IsLibraryPiece() && mFlags.testFlag(Flag::Decorated);
	}

	bool IsAlias() const
	{
		return mFlags.testFlag(Flag::Alias);
	}

	// Normalized undecorated library part ID; empty when no parent could be resolved.
	const std::string& GetPatternParentId() const
	{
		return mPatternParentId;
	}

	void SetPatternParentId(std::string ParentId)
	{
		mPatternParentId = std::move(ParentId);
	}

	bool IsSubPiece() const
	{
		return mFlags.testFlag(Flag::SubPiece);
	}

	bool IsProjectPiece() const;
	void ZoomExtents(float FoV, float AspectRatio, lcMatrix44& ProjectionMatrix, lcMatrix44& ViewMatrix) const;
	void AddRenderMesh(lcScene& Scene);
	void AddRenderMeshes(lcScene* Scene, const lcMatrix44& WorldMatrix, int ColorIndex, lcRenderMeshState RenderMeshState, bool ParentActive) const;

	void CreatePart(const char* Name);

	void DetachContainer();
	void SetModel(lcModel* Model);
	void CreateProject(Project* Project, const char* PieceName);
	bool GetPieceWorldMatrix(lcPiece* Piece, lcMatrix44& WorldMatrix) const;
	bool IncludesModel(const lcModel* Model) const;
	bool MinIntersectDist(const lcVector3& Start, const lcVector3& End, float& MinDistance, lcPieceRayTest& PieceRayTest) const;
	bool BoxTest(const lcMatrix44& WorldMatrix, const lcVector4 Planes[6]) const;
	void GetPartsList(int DefaultColorIndex, bool ScanSubModels, bool AddSubModels, lcPartsList& PartsList) const;
	void GetModelParts(const lcMatrix44& WorldMatrix, int DefaultColorIndex, std::vector<lcModelPartsEntry>& ModelParts) const;
	void CompareBoundingBox(const lcMatrix44& WorldMatrix, lcVector3& Min, lcVector3& Max) const;
	void AddSubModelBoundingBoxPoints(const lcMatrix44& WorldMatrix, std::vector<lcVector3>& Points, lcGeometryBoundsMode Mode) const;
	void UpdateBoundingBox(std::vector<lcModel*>& UpdatedModels);

	void Unload();

public:
	char mFileName[LC_PIECE_NAME_LEN] = {};
	char m_strDescription[256] = {};
	lcZipFileType mZipFileType;
	int mZipFileIndex = -1;
	lcAssetState mState = lcAssetState::Unloaded;

protected:
	enum class Flag
	{
		Decorated = 0x01,
		Alias = 0x02,
		SubPiece = 0x04
	};
	Q_DECLARE_FLAGS(Flags, Flag)

	enum class Type
	{
		Part,
		Model,
		Project
	};

	void SetLoadingBoundingBox();

	// Fixed at creation, independently of type and load state.
	const bool mIsLibraryPiece;
	int mRefCount = 0;
	Type mType = Type::Part;
	lcModel* mModel = nullptr;
	Project* mProject = nullptr;
	lcMesh* mMesh = nullptr;
	lcBoundingBox mBoundingBox;
	Flags mFlags;
	std::string mPatternParentId;
	std::shared_ptr<lcSynthInfo> mSynthInfo;
	lcTrainTrackInfo* mTrainTrackInfo = nullptr;

private:
	static bool HasDecorationDescription(const char* Description);

};
