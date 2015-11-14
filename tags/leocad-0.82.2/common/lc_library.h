#ifndef _LC_LIBRARY_H_
#define _LC_LIBRARY_H_

#include "lc_context.h"
#include "lc_mesh.h"
#include "lc_math.h"
#include "lc_array.h"
#include "str.h"

class PieceInfo;
class lcZipFile;

enum LC_MESH_PRIMITIVE_TYPE
{
	LC_MESH_LINES,
	LC_MESH_TRIANGLES,
	LC_MESH_TEXTURED_LINES,
	LC_MESH_TEXTURED_TRIANGLES,
	LC_MESH_NUM_PRIMITIVE_TYPES
};

enum lcZipFileType
{
	LC_ZIPFILE_OFFICIAL,
	LC_ZIPFILE_UNOFFICIAL,
	LC_NUM_ZIPFILES
};

class lcLibraryMeshSection
{
public:
	lcLibraryMeshSection(LC_MESH_PRIMITIVE_TYPE PrimitiveType, lcuint32 Color, lcTexture* Texture)
		: mIndices(1024, 1024)
	{
		mPrimitiveType = PrimitiveType;
		mColor = Color;
		mTexture = Texture;
	}

	~lcLibraryMeshSection()
	{
	}

	LC_MESH_PRIMITIVE_TYPE mPrimitiveType;
	lcuint32 mColor;
	lcTexture* mTexture;
	lcArray<lcuint32> mIndices;
};

struct lcLibraryTextureMap
{
	lcVector4 Params[2];
	lcTexture* Texture;
	bool Fallback;
	bool Next;
};

enum lcMeshDataType
{
	LC_MESHDATA_HIGH,
	LC_MESHDATA_LOW,
	LC_MESHDATA_SHARED,
	LC_NUM_MESHDATA_TYPES
};

class lcLibraryMeshData
{
public:
	lcLibraryMeshData()
	{
		for (int MeshDataIdx = 0; MeshDataIdx < LC_NUM_MESHDATA_TYPES; MeshDataIdx++)
			mVertices[MeshDataIdx].SetGrow(1024);
	}

	~lcLibraryMeshData()
	{
		for (int MeshDataIdx = 0; MeshDataIdx < LC_NUM_MESHDATA_TYPES; MeshDataIdx++)
			mSections[MeshDataIdx].DeleteAll();
	}

	void AddLine(lcMeshDataType MeshDataType, int LineType, lcuint32 ColorCode, const lcVector3* Vertices);
	void AddTexturedLine(lcMeshDataType MeshDataType, int LineType, lcuint32 ColorCode, const lcLibraryTextureMap& Map, const lcVector3* Vertices);
	void AddMeshData(const lcLibraryMeshData& Data, const lcMatrix44& Transform, lcuint32 CurrentColorCode, lcLibraryTextureMap* TextureMap, lcMeshDataType OverrideDestIndex);
	void AddMeshDataNoDuplicateCheck(const lcLibraryMeshData& Data, const lcMatrix44& Transform, lcuint32 CurrentColorCode, lcLibraryTextureMap* TextureMap, lcMeshDataType OverrideDestIndex);
	void TestQuad(int* QuadIndices, const lcVector3* Vertices);
	void ResequenceQuad(int* QuadIndices, int a, int b, int c, int d);

	lcArray<lcLibraryMeshSection*> mSections[LC_NUM_MESHDATA_TYPES];
	lcArray<lcVertex> mVertices[LC_NUM_MESHDATA_TYPES];
	lcArray<lcVertexTextured> mTexturedVertices[LC_NUM_MESHDATA_TYPES];
};

class lcLibraryPrimitive
{
public:
	lcLibraryPrimitive(const char* Name, lcZipFileType ZipFileType,lcuint32 ZipFileIndex, bool Stud, bool SubFile)
	{
		strncpy(mName, Name, sizeof(mName));
		mName[sizeof(mName) - 1] = 0;

		mZipFileType = ZipFileType;
		mZipFileIndex = ZipFileIndex;
		mLoaded = false;
		mStud = Stud;
		mSubFile = SubFile;
	}

	void SetZipFile(lcZipFileType ZipFileType,lcuint32 ZipFileIndex)
	{
		mZipFileType = ZipFileType;
		mZipFileIndex = ZipFileIndex;
	}

	char mName[LC_MAXPATH];
	lcZipFileType mZipFileType;
	lcuint32 mZipFileIndex;
	bool mLoaded;
	bool mStud;
	bool mSubFile;
	lcLibraryMeshData mMeshData;
};

class lcPiecesLibrary
{
public:
	lcPiecesLibrary();
	~lcPiecesLibrary();

	bool Load(const char* LibraryPath);
	void Unload();
	void RemoveTemporaryPieces();
	void RemovePiece(PieceInfo* Info);

	PieceInfo* FindPiece(const char* PieceName, Project* Project, bool CreatePlaceholder);
	bool LoadPiece(PieceInfo* Info);
	bool LoadBuiltinPieces();

	lcTexture* FindTexture(const char* TextureName);
	bool LoadTexture(lcTexture* Texture);

	bool PieceInCategory(PieceInfo* Info, const String& CategoryKeywords) const;
	void SearchPieces(const char* Keyword, lcArray<PieceInfo*>& Pieces) const;
	void GetCategoryEntries(int CategoryIndex, bool GroupPieces, lcArray<PieceInfo*>& SinglePieces, lcArray<PieceInfo*>& GroupedPieces);
	void GetCategoryEntries(const String& CategoryKeywords, bool GroupPieces, lcArray<PieceInfo*>& SinglePieces, lcArray<PieceInfo*>& GroupedPieces);
	void GetPatternedPieces(PieceInfo* Parent, lcArray<PieceInfo*>& Pieces) const;

	bool IsPrimitive(const char* Name) const
	{
		return FindPrimitiveIndex(Name) != -1;
	}

	void SetOfficialPieces()
	{
		if (mZipFiles[LC_ZIPFILE_OFFICIAL])
			mNumOfficialPieces = mPieces.GetSize();
	}

	bool ReadMeshData(lcFile& File, const lcMatrix44& CurrentTransform, lcuint32 CurrentColorCode, lcArray<lcLibraryTextureMap>& TextureStack, lcLibraryMeshData& MeshData, lcMeshDataType MeshDataType);
	void CreateMesh(PieceInfo* Info, lcLibraryMeshData& MeshData);
	void UpdateBuffers(lcContext* Context);

	lcArray<PieceInfo*> mPieces;
	lcArray<lcLibraryPrimitive*> mPrimitives;
	int mNumOfficialPieces;

	lcArray<lcTexture*> mTextures;

	char mLibraryPath[LC_MAXPATH];

	bool mBuffersDirty;
	lcVertexBuffer mVertexBuffer;
	lcIndexBuffer mIndexBuffer;

protected:
	bool OpenArchive(const char* FileName, lcZipFileType ZipFileType);
	bool OpenArchive(lcFile* File, const char* FileName, lcZipFileType ZipFileType);
	bool OpenDirectory(const char* Path);
	void ReadArchiveDescriptions(const QString& OfficialFileName, const QString& UnofficialFileName);

	bool ReadCacheFile(const QString& FileName, lcMemFile& CacheFile);
	bool WriteCacheFile(const QString& FileName, lcMemFile& CacheFile);
	bool LoadCacheIndex(const QString& FileName);
	bool SaveCacheIndex(const QString& FileName);
	bool LoadCachePiece(PieceInfo* Info);
	bool SaveCachePiece(PieceInfo* Info);

	int FindPrimitiveIndex(const char* Name) const;
	bool LoadPrimitive(int PrimitiveIndex);

	QString mCachePath;
	qint64 mArchiveCheckSum[4];
	char mLibraryFileName[LC_MAXPATH];
	char mUnofficialFileName[LC_MAXPATH];
	lcZipFile* mZipFiles[LC_NUM_ZIPFILES];
};

#endif // _LC_LIBRARY_H_
