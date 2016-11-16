#ifndef _PIECEINF_H_
#define _PIECEINF_H_

#include <stdio.h>
#include "lc_math.h"
#include "lc_array.h"

#define LC_PIECE_HAS_DEFAULT        0x01 // Piece has triangles using the default color
#define LC_PIECE_HAS_SOLID          0x02 // Piece has triangles using a solid color
#define LC_PIECE_HAS_TRANSLUCENT    0x04 // Piece has triangles using a translucent color
#define LC_PIECE_HAS_LINES          0x08 // Piece has lines
#define LC_PIECE_PLACEHOLDER        0x10 // Placeholder for a piece not in the library
#define LC_PIECE_MODEL              0x20 // Piece is a model

#define LC_PIECE_NAME_LEN 256

class lcSynthInfo;

class PieceInfo
{
public:
	PieceInfo();
	~PieceInfo();

	QString GetSaveID() const;

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

	void SetMesh(lcMesh* Mesh)
	{
		mMesh = Mesh;
	}

	void AddRef()
	{
		mRefCount++;

		if (!mLoaded)
			Load();
	}

	void Release(bool AllowUnload)
	{
		mRefCount--;

		if (!mRefCount && AllowUnload)
			Unload();
	}

	void UnloadIfUnused()
	{
		if (!mRefCount && mLoaded)
			Unload();
	}

	bool IsLoaded() const
	{
		return mLoaded;
	}

	bool IsPlaceholder() const
	{
		return (mFlags & LC_PIECE_PLACEHOLDER) != 0;
	}

	bool IsModel() const
	{
		return (mFlags & LC_PIECE_MODEL) != 0;
	}

	bool IsTemporary() const
	{
		return (mFlags & (LC_PIECE_PLACEHOLDER | LC_PIECE_MODEL)) != 0;
	}

	void SetZipFile(int ZipFileType, int ZipFileIndex)
	{
		mZipFileType = ZipFileType;
		mZipFileIndex = ZipFileIndex;
	}

	bool IsPatterned() const
	{
		const char* Name = m_strName;

		while (*Name)
		{
			if (*Name < '0' || *Name > '9')
				break;

			Name++;
		}

		if (*Name == 'P')
			return true;

		return false;
	}

	bool IsSubPiece() const
	{
		return (m_strDescription[0] == '~');
	}

	void ZoomExtents(const lcMatrix44& ProjectionMatrix, lcMatrix44& ViewMatrix, float* EyePos = NULL) const;
	void AddRenderMesh(lcScene& Scene);
	void AddRenderMeshes(lcScene& Scene, const lcMatrix44& WorldMatrix, int ColorIndex, bool Focused, bool Selected) const;

	void CreatePlaceholder(const char* Name);

	void SetPlaceholder();
	void SetModel(lcModel* Model, bool UpdateMesh);
	bool IncludesModel(const lcModel* Model) const;
	bool MinIntersectDist(const lcVector3& Start, const lcVector3& End, float& MinDistance) const;
	bool BoxTest(const lcMatrix44& WorldMatrix, const lcVector4 Planes[6]) const;
	void GetPartsList(int DefaultColorIndex, lcPartsList& PartsList) const;
	void GetModelParts(const lcMatrix44& WorldMatrix, int DefaultColorIndex, lcArray<lcModelPartsEntry>& ModelParts) const;
	void UpdateBoundingBox(lcArray<lcModel*>& UpdatedModels);

public:
	// Attributes
	char m_strName[LC_PIECE_NAME_LEN];
	char m_strDescription[128];
	int mZipFileType;
	int mZipFileIndex;
	lcuint32 mFlags;

protected:
	bool mLoaded;
	int mRefCount;
	lcModel* mModel;
	lcMesh* mMesh;
	lcBoundingBox mBoundingBox;
	lcSynthInfo* mSynthInfo;

	void Load();
	void Unload();
};

#endif // _PIECEINF_H_
