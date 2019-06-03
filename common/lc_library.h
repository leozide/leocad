#pragma once

#include "lc_context.h"
#include "lc_mesh.h"
#include "lc_math.h"
#include "lc_array.h"

class PieceInfo;
class lcZipFile;

enum lcZipFileType
{
	LC_ZIPFILE_OFFICIAL,
	LC_ZIPFILE_UNOFFICIAL,
	LC_NUM_ZIPFILES
};

enum lcLibraryFolderType
{
	LC_FOLDER_UNOFFICIAL,
	LC_FOLDER_OFFICIAL,
	LC_NUM_FOLDERTYPES
};

#define LC_LIBRARY_VERTEX_UNTEXTURED 0x1
#define LC_LIBRARY_VERTEX_TEXTURED   0x2

struct lcLibraryMeshVertex
{
	lcVector3 Position;
	lcVector3 Normal;
	float NormalWeight;
	lcVector2 TexCoord;
	quint32 Usage;
};

class lcLibraryMeshSection
{
public:
	lcLibraryMeshSection(lcMeshPrimitiveType PrimitiveType, quint32 Color, lcTexture* Texture)
		: mIndices(1024, 1024)
	{
		mPrimitiveType = PrimitiveType;
		mColor = Color;
		mTexture = Texture;
	}

	~lcLibraryMeshSection()
	{
	}

	lcMeshPrimitiveType mPrimitiveType;
	quint32 mColor;
	lcTexture* mTexture;
	lcArray<quint32> mIndices;
};

enum class lcLibraryTextureMapType
{
	PLANAR,
	CYLINDRICAL,
	SPHERICAL
};

struct lcLibraryTextureMap
{
	lcTexture* Texture;

	union lcTextureMapParams
	{
		lcTextureMapParams()
		{
		}

		struct lcTextureMapPlanarParams
		{
			lcVector4 Planes[2];
		} Planar;

		struct lcTextureMapCylindricalParams
		{
			lcVector4 FrontPlane;
			float UpLength;
			lcVector4 Plane1;
			lcVector4 Plane2;
		} Cylindrical;

		struct lcTextureMapSphericalParams
		{
			lcVector4 FrontPlane;
			lcVector3 Center;
			lcVector4 Plane1;
			lcVector4 Plane2;
		} Spherical;
	} Params;

	float Angle1;
	float Angle2;
	lcLibraryTextureMapType Type;
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

enum class lcPrimitiveState
{
	NOT_LOADED,
	LOADING,
	LOADED
};

class lcLibraryMeshData
{
public:
	lcLibraryMeshData()
	{
		mHasTextures = false;

		for (int MeshDataIdx = 0; MeshDataIdx < LC_NUM_MESHDATA_TYPES; MeshDataIdx++)
			mVertices[MeshDataIdx].SetGrow(1024);
	}

	~lcLibraryMeshData()
	{
		for (int MeshDataIdx = 0; MeshDataIdx < LC_NUM_MESHDATA_TYPES; MeshDataIdx++)
			mSections[MeshDataIdx].DeleteAll();
	}

	bool IsEmpty() const
	{
		for (int MeshDataIdx = 0; MeshDataIdx < LC_NUM_MESHDATA_TYPES; MeshDataIdx++)
			if (!mSections[MeshDataIdx].IsEmpty())
				return false;

		return true;
	}

	lcLibraryMeshSection* AddSection(lcMeshDataType MeshDataType, lcMeshPrimitiveType PrimitiveType, quint32 ColorCode, lcTexture* Texture);
	quint32 AddVertex(lcMeshDataType MeshDataType, const lcVector3& Position, bool Optimize);
	quint32 AddVertex(lcMeshDataType MeshDataType, const lcVector3& Position, const lcVector3& Normal, bool Optimize);
	quint32 AddTexturedVertex(lcMeshDataType MeshDataType, const lcVector3& Position, const lcVector2& TexCoord, bool Optimize);
	quint32 AddTexturedVertex(lcMeshDataType MeshDataType, const lcVector3& Position, const lcVector3& Normal, const lcVector2& TexCoord, bool Optimize);
	void AddVertices(lcMeshDataType MeshDataType, int VertexCount, int* BaseVertex, lcLibraryMeshVertex** VertexBuffer);
	void AddIndices(lcMeshDataType MeshDataType, lcMeshPrimitiveType PrimitiveType, quint32 ColorCode, int IndexCount, quint32** IndexBuffer);
	void AddLine(lcMeshDataType MeshDataType, int LineType, quint32 ColorCode, bool WindingCCW, const lcVector3* Vertices, bool Optimize);
	void AddTexturedLine(lcMeshDataType MeshDataType, int LineType, quint32 ColorCode, bool WindingCCW, const lcLibraryTextureMap& Map, const lcVector3* Vertices, bool Optimize);
	void AddMeshData(const lcLibraryMeshData& Data, const lcMatrix44& Transform, quint32 CurrentColorCode, bool InvertWinding, bool InvertNormals, lcLibraryTextureMap* TextureMap, lcMeshDataType OverrideDestIndex);
	void AddMeshDataNoDuplicateCheck(const lcLibraryMeshData& Data, const lcMatrix44& Transform, quint32 CurrentColorCode, bool InvertWinding, bool InvertNormals, lcLibraryTextureMap* TextureMap, lcMeshDataType OverrideDestIndex);
	void TestQuad(int* QuadIndices, const lcVector3* Vertices);
	void ResequenceQuad(int* QuadIndices, int a, int b, int c, int d);

	lcArray<lcLibraryMeshSection*> mSections[LC_NUM_MESHDATA_TYPES];
	lcArray<lcLibraryMeshVertex> mVertices[LC_NUM_MESHDATA_TYPES];
	bool mHasTextures;
};

class lcLibraryPrimitive
{
public:
	explicit lcLibraryPrimitive(QString&& FileName, const char* Name, lcZipFileType ZipFileType,quint32 ZipFileIndex, bool Stud, bool SubFile)
		: mFileName(std::move(FileName))
	{
		strncpy(mName, Name, sizeof(mName));
		mName[sizeof(mName) - 1] = 0;

		mZipFileType = ZipFileType;
		mZipFileIndex = ZipFileIndex;
		mState = lcPrimitiveState::NOT_LOADED;
		mStud = Stud;
		mSubFile = SubFile;
	}

	void SetZipFile(lcZipFileType ZipFileType, quint32 ZipFileIndex)
	{
		mZipFileType = ZipFileType;
		mZipFileIndex = ZipFileIndex;
	}

	QString mFileName;
	char mName[LC_MAXNAME];
	lcZipFileType mZipFileType;
	quint32 mZipFileIndex;
	lcPrimitiveState mState;
	bool mStud;
	bool mSubFile;
	lcLibraryMeshData mMeshData;
};

class lcPiecesLibrary : public QObject
{
	Q_OBJECT

public:
	lcPiecesLibrary();
	~lcPiecesLibrary();

	bool Load(const QString& LibraryPath, bool ShowProgress);
	void Unload();
	void RemoveTemporaryPieces();
	void RemovePiece(PieceInfo* Info);

	void RenamePiece(PieceInfo* Info, const char* NewName);
	PieceInfo* FindPiece(const char* PieceName, Project* Project, bool CreatePlaceholder, bool SearchProjectFolder);
	void LoadPieceInfo(PieceInfo* Info, bool Wait, bool Priority);
	void ReleasePieceInfo(PieceInfo* Info);
	bool LoadBuiltinPieces();
	bool LoadPieceData(PieceInfo* Info);
	void LoadQueuedPiece();
	void WaitForLoadQueue();

	lcTexture* FindTexture(const char* TextureName, Project* CurrentProject, bool SearchProjectFolder);
	bool LoadTexture(lcTexture* Texture);
	void ReleaseTexture(lcTexture* Texture);
	void QueueTextureUpload(lcTexture* Texture);
	void UploadTextures(lcContext* Context);

	bool PieceInCategory(PieceInfo* Info, const char* CategoryKeywords) const;
	void GetCategoryEntries(int CategoryIndex, bool GroupPieces, lcArray<PieceInfo*>& SinglePieces, lcArray<PieceInfo*>& GroupedPieces);
	void GetCategoryEntries(const char* CategoryKeywords, bool GroupPieces, lcArray<PieceInfo*>& SinglePieces, lcArray<PieceInfo*>& GroupedPieces);
	void GetPatternedPieces(PieceInfo* Parent, lcArray<PieceInfo*>& Pieces) const;
	void GetParts(lcArray<PieceInfo*>& Parts);

	bool IsPrimitive(const char* Name) const
	{
		return mPrimitives.find(Name) != mPrimitives.end();
	}

	void SetOfficialPieces()
	{
		if (mZipFiles[LC_ZIPFILE_OFFICIAL])
			mNumOfficialPieces = (int)mPieces.size();
	}

	bool ReadMeshData(lcFile& File, const lcMatrix44& CurrentTransform, quint32 CurrentColorCode, bool InvertWinding, lcArray<lcLibraryTextureMap>& TextureStack, lcLibraryMeshData& MeshData, lcMeshDataType MeshDataType, bool Optimize, Project* CurrentProject, bool SearchProjectFolder);
	lcMesh* CreateMesh(PieceInfo* Info, lcLibraryMeshData& MeshData);
	void ReleaseBuffers(lcContext* Context);
	void UpdateBuffers(lcContext* Context);
	void UnloadUnusedParts();

	std::map<std::string, PieceInfo*> mPieces;
	std::map<std::string, lcLibraryPrimitive*> mPrimitives;
	int mNumOfficialPieces;

	lcArray<lcTexture*> mTextures;

	QDir mLibraryDir;

	bool mBuffersDirty;
	lcVertexBuffer mVertexBuffer;
	lcIndexBuffer mIndexBuffer;

signals:
	void PartLoaded(PieceInfo* Info);

protected:
	bool OpenArchive(const QString& FileName, lcZipFileType ZipFileType);
	bool OpenArchive(lcFile* File, const QString& FileName, lcZipFileType ZipFileType);
	bool OpenDirectory(const QDir& LibraryDir, bool ShowProgress);
	void ReadArchiveDescriptions(const QString& OfficialFileName, const QString& UnofficialFileName);
	void ReadDirectoryDescriptions(const QFileInfoList (&FileLists)[LC_NUM_FOLDERTYPES], bool ShowProgress);

	bool ReadArchiveCacheFile(const QString& FileName, lcMemFile& CacheFile);
	bool WriteArchiveCacheFile(const QString& FileName, lcMemFile& CacheFile);
	bool LoadCacheIndex(const QString& FileName);
	bool SaveArchiveCacheIndex(const QString& FileName);
	bool LoadCachePiece(PieceInfo* Info);
	bool SaveCachePiece(PieceInfo* Info);
	bool ReadDirectoryCacheFile(const QString& FileName, lcMemFile& CacheFile);
	bool WriteDirectoryCacheFile(const QString& FileName, lcMemFile& CacheFile);

	lcLibraryPrimitive* FindPrimitive(const char* Name) const
	{
		const auto PrimitiveIt = mPrimitives.find(Name);
		return PrimitiveIt != mPrimitives.end() ? PrimitiveIt->second : nullptr;
	}

	bool LoadPrimitive(lcLibraryPrimitive* Primitive);

	QMutex mLoadMutex;
	QList<QFuture<void>> mLoadFutures;
	QList<PieceInfo*> mLoadQueue;

	QMutex mTextureMutex;
	std::vector<lcTexture*> mTextureUploads;

	QString mCachePath;
	qint64 mArchiveCheckSum[4];
	QString mLibraryFileName;
	QString mUnofficialFileName;
	lcZipFile* mZipFiles[LC_NUM_ZIPFILES];
	bool mHasUnofficial;
	bool mCancelLoading;
};

