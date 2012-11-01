#ifndef _LC_LIBRARY_H_
#define _LC_LIBRARY_H_

#include "lc_mesh.h"
#include "lc_math.h"
#include "array.h"
#include "str.h"

class PieceInfo;
class lcZipFile;

#define LC_CATEGORY_FILE_ID       LC_FOURCC('C', 'A', 'T', 0)
#define LC_CATEGORY_FILE_VERSION  0x0100

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
	ObjArray<lcuint32> mIndices;
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

	void AddLine(int LineType, lcuint32 ColorCode, const lcVector3* Vertices);
	void AddTexturedLine(int LineType, lcuint32 ColorCode, const lcLibraryTextureMap& Map, const lcVector3* Vertices);
	void AddMeshData(const lcLibraryMeshData& Data, const lcMatrix44& Transform, lcuint32 CurrentColorCode, lcLibraryTextureMap* TextureMap);
	void AddMeshDataNoDuplicateCheck(const lcLibraryMeshData& Data, const lcMatrix44& Transform, lcuint32 CurrentColorCode, lcLibraryTextureMap* TextureMap);

	PtrArray<lcLibraryMeshSection> mSections;
	ObjArray<lcVertex> mVertices;
	ObjArray<lcVertexTextured> mTexturedVertices;
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

struct lcLibraryCategory
{
	String Name;
	String Keywords;
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

	lcTexture* FindTexture(const char* TextureName);
	bool LoadTexture(lcTexture* Texture);

	bool PieceInCategory(PieceInfo* Info, const String& CategoryKeywords) const;
	int GetFirstPieceCategory(PieceInfo* Info) const;
	void GetCategoryEntries(int CategoryIndex, bool GroupPieces, PtrArray<PieceInfo>& SinglePieces, PtrArray<PieceInfo>& GroupedPieces);
	void GetPatternedPieces(PieceInfo* Parent, PtrArray<PieceInfo>& Pieces) const;
	int FindCategoryIndex(const String& CategoryName) const;
	void SetCategory(int Index, const String& Name, const String& Keywords);
	void AddCategory(const String& Name, const String& Keywords);
	void RemoveCategory(int Index);
	void ResetCategories();
	bool LoadCategories(const char* FileName);
	bool SaveCategories();
	bool DoSaveCategories(bool AskName);

	PtrArray<PieceInfo> mPieces;
	PtrArray<lcLibraryPrimitive> mPrimitives;
	ObjArray<lcLibraryCategory> mCategories;

	PtrArray<lcTexture> mTextures;

	char mLibraryPath[LC_MAXPATH];

protected:
	bool OpenArchive(const char* FileName, const char* CachePath);
	bool OpenDirectory(const char* Path);
	int FindPrimitiveIndex(const char* Name);
	bool LoadPrimitive(int PrimitiveIndex);
	bool ReadMeshData(lcFile& File, const lcMatrix44& CurrentTransform, lcuint32 CurrentColorCode, ObjArray<lcLibraryTextureMap>& TextureStack, lcLibraryMeshData& MeshData);

	bool mCategoriesModified;
	char mCategoriesFile[LC_MAXPATH];

	char mCacheFileName[LC_MAXPATH];

	lcZipFile* mZipFile;
};

#endif // _LC_LIBRARY_H_
