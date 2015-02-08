#ifndef _PIECEINF_H_
#define _PIECEINF_H_

#include <stdio.h>
#include "lc_math.h"
#include "lc_array.h"
#include "lc_mesh.h"

#define LC_PIECE_HAS_DEFAULT        0x01 // Piece has triangles using the default color
#define LC_PIECE_HAS_SOLID          0x02 // Piece has triangles using a solid color
#define LC_PIECE_HAS_TRANSLUCENT    0x04 // Piece has triangles using a translucent color
#define LC_PIECE_HAS_LINES          0x08 // Piece has lines
#define LC_PIECE_PLACEHOLDER        0x10 // Placeholder for a piece not in the library
#define LC_PIECE_CACHED             0x20 // Piece is saved in the library cache
#define LC_PIECE_MODEL              0x40 // Piece if a model

#define LC_PIECE_NAME_LEN 256

class PieceInfo
{
public:
	PieceInfo();
	~PieceInfo();

	QString GetSaveID() const;

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

	int AddRef()
	{
		mRefCount++;

		if (mRefCount == 1)
			Load();

		return mRefCount;
	}

	int Release()
	{
		mRefCount--;

		if (!mRefCount)
			Unload();

		return mRefCount;
	}

	bool IsLoaded() const
	{
		return mRefCount != 0;
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

	lcVector3 GetCenter() const
	{
		return lcVector3((m_fDimensions[0] + m_fDimensions[3]) * 0.5f,
		                 (m_fDimensions[1] + m_fDimensions[4]) * 0.5f,
		                 (m_fDimensions[2] + m_fDimensions[5]) * 0.5f);
	}

	void ZoomExtents(const lcMatrix44& ProjectionMatrix, lcMatrix44& ViewMatrix, float* EyePos = NULL) const;
	void AddRenderMeshes(lcScene& Scene, const lcMatrix44& WorldMatrix, int ColorIndex, bool Focused, bool Selected);

	void CreatePlaceholder(const char* Name);

	void SetPlaceholder();
	void SetModel(lcModel* Model);
	bool IncludesModel(const lcModel* Model) const;
	bool MinIntersectDist(const lcMatrix44& WorldMatrix, const lcVector3& WorldStart, const lcVector3& WorldEnd, float& MinDistance) const;
	bool BoxTest(const lcMatrix44& WorldMatrix, const lcVector4 Planes[6]) const;
	void GetPartsList(int DefaultColorIndex, lcArray<lcPartsListEntry>& PartsList) const;
	void GetModelParts(const lcMatrix44& WorldMatrix, int DefaultColorIndex, lcArray<lcModelPartsEntry>& ModelParts) const;
	void UpdateBoundingBox(lcArray<lcModel*>& UpdatedModels);

public:
	// Attributes
	char m_strName[LC_PIECE_NAME_LEN];
	char m_strDescription[128];
	float m_fDimensions[6];
	int mZipFileType;
	int mZipFileIndex;
	lcuint32 mFlags;

protected:
	int mRefCount;
	lcModel* mModel;
	lcMesh* mMesh;

	void Load();
	void Unload();
};

#endif // _PIECEINF_H_
