#pragma once

#include <stdio.h>
#include "lc_math.h"
#include "lc_array.h"

enum class lcPieceInfoType
{
	Part,
	Placeholder,
	Model,
	Project
};

#define LC_PIECE_NAME_LEN 256

enum lcPieceInfoState
{
	LC_PIECEINFO_UNLOADED,
	LC_PIECEINFO_LOADING,
	LC_PIECEINFO_LOADED
};

struct lcModelPartsEntry
{
	lcMatrix44 WorldMatrix;
	const PieceInfo* Info;
	lcMesh* Mesh;
	int ColorIndex;
};

class lcSynthInfo;

class PieceInfo
{
public:
	PieceInfo();
	~PieceInfo();

	PieceInfo(const PieceInfo&) = delete;
	PieceInfo(PieceInfo&&) = delete;
	PieceInfo& operator=(const PieceInfo&) = delete;
	PieceInfo& operator=(PieceInfo&&) = delete;

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
		return mSynthInfo;
	}

	void SetSynthInfo(lcSynthInfo* SynthInfo)
	{
		mSynthInfo = SynthInfo;
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

	bool IsPlaceholder() const
	{
		return mType == lcPieceInfoType::Placeholder;
	}

	bool IsModel() const
	{
		return mType == lcPieceInfoType::Model;
	}

	bool IsProject() const
	{
		return mType == lcPieceInfoType::Project;
	}

	bool IsTemporary() const
	{
		return mType != lcPieceInfoType::Part;
	}

	void SetZipFile(int ZipFileType, int ZipFileIndex)
	{
		mZipFileType = ZipFileType;
		mZipFileIndex = ZipFileIndex;
	}

	bool IsPatterned() const
	{
		if (mType != lcPieceInfoType::Part)
			return false;

		const char* Name = mFileName;

		// Heuristic: Names matching '^[Uu]?[0-9]*[A-Za-z][^.][^.]' are patterned.

		if (*Name == 'U' || *Name == 'u')
			Name++;

		while (*Name)
		{
			if (*Name < '0' || *Name > '9')
				break;

			Name++;
		}

		if (!*Name || !((*Name >= 'A' && *Name <= 'Z') || (*Name >= 'a' && *Name <= 'z')))
			return false;

		if (Name[1] && Name[1] != '.' && Name[2] && Name[2] != '.')
			return true;

		return false;
	}

	bool IsSubPiece() const
	{
		return (m_strDescription[0] == '~');
	}

	void ZoomExtents(float FoV, float AspectRatio, lcMatrix44& ProjectionMatrix, lcMatrix44& ViewMatrix) const;
	void AddRenderMesh(lcScene& Scene);
	void AddRenderMeshes(lcScene& Scene, const lcMatrix44& WorldMatrix, int ColorIndex, lcRenderMeshState RenderMeshState, bool ParentActive) const;

	void CreatePlaceholder(const char* Name);

	void SetPlaceholder();
	void SetModel(lcModel* Model, bool UpdateMesh, Project* CurrentProject, bool SearchProjectFolder);
	void CreateProject(Project* Project, const char* PieceName);
	bool GetPieceWorldMatrix(lcPiece* Piece, lcMatrix44& WorldMatrix) const;
	bool IncludesModel(const lcModel* Model) const;
	bool MinIntersectDist(const lcVector3& Start, const lcVector3& End, float& MinDistance) const;
	bool BoxTest(const lcMatrix44& WorldMatrix, const lcVector4 Planes[6]) const;
	void GetPartsList(int DefaultColorIndex, bool ScanSubModels, bool AddSubModels, lcPartsList& PartsList) const;
	void GetModelParts(const lcMatrix44& WorldMatrix, int DefaultColorIndex, std::vector<lcModelPartsEntry>& ModelParts) const;
	void CompareBoundingBox(const lcMatrix44& WorldMatrix, lcVector3& Min, lcVector3& Max) const;
	void UpdateBoundingBox(std::vector<lcModel*>& UpdatedModels);

	void Load();
	void Unload();

public:
	char mFileName[LC_PIECE_NAME_LEN];
	char m_strDescription[128];
	int mZipFileType;
	int mZipFileIndex;
	lcPieceInfoState mState;
	int mFolderType;
	int mFolderIndex;

protected:
	void ReleaseMesh();

	int mRefCount;
	lcPieceInfoType mType;
	lcModel* mModel;
	Project* mProject;
	lcMesh* mMesh;
	lcBoundingBox mBoundingBox;
	lcSynthInfo* mSynthInfo;
};

