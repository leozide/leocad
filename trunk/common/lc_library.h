#ifndef _LC_LIBRARY_H_
#define _LC_LIBRARY_H_

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

class lcLibraryMeshData
{
public:
	lcLibraryMeshData()
		: mVertices(1024, 1024)
	{
	}

	~lcLibraryMeshData()
	{
		for (int SectionIdx = 0; SectionIdx < mSections.GetSize(); SectionIdx++)
			delete mSections[SectionIdx];
	}

	void AddLine(int LineType, lcuint32 ColorCode, lcVector3* Vertices);
	void AddTexturedLine(int LineType, lcuint32 ColorCode, const lcLibraryTextureMap& Map, lcVector3* Vertices);
	void AddMeshData(const lcLibraryMeshData& Data, const lcMatrix44& Transform, lcuint32 CurrentColorCode, lcLibraryTextureMap* TextureMap);
	void AddMeshDataNoDuplicateCheck(const lcLibraryMeshData& Data, const lcMatrix44& Transform, lcuint32 CurrentColorCode, lcLibraryTextureMap* TextureMap);
	void TestQuad(lcVector3* Vertices);
	void ResequenceQuad(lcVector3* Vertices, int a, int b, int c, int d);

	lcArray<lcLibraryMeshSection*> mSections;
	lcArray<lcVertex> mVertices;
	lcArray<lcVertexTextured> mTexturedVertices;
};

class lcLibraryPrimitive
{
public:
	lcLibraryPrimitive(const char* Name, lcuint32 ZipFileIndex, bool Stud, bool SubFile)
	{
		strncpy(mName, Name, sizeof(mName));
		mName[sizeof(mName) - 1] = 0;

		mZipFileIndex = ZipFileIndex;
		mLoaded = false;
		mStud = Stud;
		mSubFile = SubFile;
	}

	char mName[LC_MAXPATH];
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

	bool Load(const char* LibraryPath, const char* CachePath);
	void Unload();

	PieceInfo* FindPiece(const char* PieceName, bool CreatePlaceholderIfMissing);
	PieceInfo* CreatePlaceholder(const char* PieceName);
	bool LoadPiece(PieceInfo* Info);
	bool GeneratePiece(PieceInfo* Info);
	void CreateBuiltinPieces();

	lcTexture* FindTexture(const char* TextureName);
	bool LoadTexture(lcTexture* Texture);

	bool OpenCache();
	void CloseCache();

	bool PieceInCategory(PieceInfo* Info, const String& CategoryKeywords) const;
	void SearchPieces(const String& CategoryKeywords, bool GroupPieces, lcArray<PieceInfo*>& SinglePieces, lcArray<PieceInfo*>& GroupedPieces);
	void GetCategoryEntries(int CategoryIndex, bool GroupPieces, lcArray<PieceInfo*>& SinglePieces, lcArray<PieceInfo*>& GroupedPieces);
	void GetPatternedPieces(PieceInfo* Parent, lcArray<PieceInfo*>& Pieces) const;

	void SetOfficialPieces()
	{
		if (mZipFile)
			mNumOfficialPieces = mPieces.GetSize();
	}

	lcArray<PieceInfo*> mPieces;
	lcArray<lcLibraryPrimitive*> mPrimitives;
	int mNumOfficialPieces;

	lcArray<lcTexture*> mTextures;

	char mLibraryPath[LC_MAXPATH];

protected:
	bool OpenArchive(const char* FileName, const char* CachePath);
	bool OpenDirectory(const char* Path);

	bool LoadCacheIndex(lcZipFile& CacheFile);
	bool LoadCachePiece(PieceInfo* Info);
	void SaveCacheFile();

	int FindPrimitiveIndex(const char* Name);
	bool LoadPrimitive(int PrimitiveIndex);
	bool ReadMeshData(lcFile& File, const lcMatrix44& CurrentTransform, lcuint32 CurrentColorCode, lcArray<lcLibraryTextureMap>& TextureStack, lcLibraryMeshData& MeshData);

	char mCacheFileName[LC_MAXPATH];
	lcuint64 mCacheFileModifiedTime;
	lcZipFile* mCacheFile;
	bool mSaveCache;

	char mLibraryFileName[LC_MAXPATH];
	lcZipFile* mZipFile;
};

#endif // _LC_LIBRARY_H_
