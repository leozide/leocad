#pragma once

#include <stdio.h>
#include "lc_math.h"
#include "lc_array.h"

#define LC_PIECE_HAS_DEFAULT        0x01 // Piece has triangles using the default color
#define LC_PIECE_HAS_SOLID          0x02 // Piece has triangles using a solid color
#define LC_PIECE_HAS_TRANSLUCENT    0x04 // Piece has triangles using a translucent color
#define LC_PIECE_HAS_LINES          0x08 // Piece has lines
#define LC_PIECE_PLACEHOLDER        0x10 // Placeholder for a piece not in the library
#define LC_PIECE_MODEL              0x20 // Piece is a model
#define LC_PIECE_PROJECT            0x40 // Piece is a project
#define LC_PIECE_HAS_TEXTURE        0x80 // Piece has sections using textures

#define LC_PIECE_NAME_LEN 256

enum lcPieceInfoState
{
	LC_PIECEINFO_UNLOADED,
	LC_PIECEINFO_LOADING,
	LC_PIECEINFO_LOADED
};

class lcSynthInfo;

class PieceInfo
{
public:
	PieceInfo();
	~PieceInfo();

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
		return (mFlags & LC_PIECE_PLACEHOLDER) != 0;
	}

	bool IsModel() const
	{
		return (mFlags & LC_PIECE_MODEL) != 0;
	}

	bool IsProject() const
	{
		return (mFlags & LC_PIECE_PROJECT) != 0;
	}

	bool IsTemporary() const
	{
		return (mFlags & (LC_PIECE_PLACEHOLDER | LC_PIECE_MODEL | LC_PIECE_PROJECT)) != 0;
	}

	void SetZipFile(int ZipFileType, int ZipFileIndex)
	{
		mZipFileType = ZipFileType;
		mZipFileIndex = ZipFileIndex;
	}

	bool IsPatterned() const
	{
		const char* Name = mFileName;

		while (*Name)
		{
			if (*Name < '0' || *Name > '9')
				break;

			Name++;
		}

		if (*Name == 'P' || *Name == 'p')
			return true;

		return false;
	}

	bool IsSubPiece() const
	{
		return (m_strDescription[0] == '~');
	}

	void ZoomExtents(float FoV, float AspectRatio, lcMatrix44& ProjectionMatrix, lcMatrix44& ViewMatrix) const;
	void AddRenderMesh(lcScene& Scene);
	void AddRenderMeshes(lcScene& Scene, const lcMatrix44& WorldMatrix, int ColorIndex, bool Focused, bool Selected, bool Disabled, bool Highlight, lcPiece* ActiveSubmodelInstance) const;

	void CreatePlaceholder(const char* Name);

	void SetPlaceholder();
	void SetModel(lcModel* Model, bool UpdateMesh, Project* CurrentProject, bool SearchProjectFolder);
	void CreateProject(Project* Project, const char* PieceName);
	bool GetPieceWorldMatrix(lcPiece* Piece, lcMatrix44& WorldMatrix) const;
	bool IncludesModel(const lcModel* Model) const;
	bool MinIntersectDist(const lcVector3& Start, const lcVector3& End, float& MinDistance) const;
	bool BoxTest(const lcMatrix44& WorldMatrix, const lcVector4 Planes[6]) const;
	void GetPartsList(int DefaultColorIndex, bool IncludeSubmodels, lcPartsList& PartsList) const;
	void GetModelParts(const lcMatrix44& WorldMatrix, int DefaultColorIndex, lcArray<lcModelPartsEntry>& ModelParts) const;
	void UpdateBoundingBox(lcArray<lcModel*>& UpdatedModels);

	void Load();
	void Unload();

public:
	char mFileName[LC_PIECE_NAME_LEN];
	char m_strDescription[128];
	int mZipFileType;
	int mZipFileIndex;
	quint32 mFlags;
	lcPieceInfoState mState;
	int mFolderType;
	int mFolderIndex;

protected:
	void ReleaseMesh();

	int mRefCount;
	lcModel* mModel;
	Project* mProject;
	lcMesh* mMesh;
	lcBoundingBox mBoundingBox;
	lcSynthInfo* mSynthInfo;
};

