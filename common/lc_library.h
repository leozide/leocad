#pragma once

#include "lc_context.h"
#include "lc_mesh.h"
#include "lc_math.h"
#include "lc_array.h"
#include "lc_meshloader.h"

class PieceInfo;
class lcZipFile;
class lcLibraryMeshData;

enum class lcZipFileType
{
	Official,
	Unofficial,
	StudLogo,
	Count
};

enum lcLibraryFolderType
{
	LC_FOLDER_UNOFFICIAL,
	LC_FOLDER_OFFICIAL,
	LC_NUM_FOLDERTYPES
};

enum class lcPrimitiveState
{
	NotLoaded,
	Loading,
	Loaded
};

class lcLibraryPrimitive
{
public:
	explicit lcLibraryPrimitive(QString&& FileName, const char* Name, lcZipFileType ZipFileType, quint32 ZipFileIndex, bool Stud, bool SubFile)
		: mFileName(std::move(FileName))
	{
		strncpy(mName, Name, sizeof(mName)-1);
		mName[sizeof(mName) - 1] = 0;

		mZipFileType = ZipFileType;
		mZipFileIndex = ZipFileIndex;
		mState = lcPrimitiveState::NotLoaded;
		mStud = Stud;
		mSubFile = SubFile;
	}

	void SetZipFile(lcZipFileType ZipFileType, quint32 ZipFileIndex)
	{
		mZipFileType = ZipFileType;
		mZipFileIndex = ZipFileIndex;
	}

	void Unload()
	{
		mState = lcPrimitiveState::NotLoaded;
		mMeshData.RemoveAll();
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

enum class lcLibrarySourceType
{
	Library,
	StudLogo
};

struct lcLibrarySource
{
	~lcLibrarySource()
	{
		for (const auto& PrimitiveIt : Primitives)
			delete PrimitiveIt.second;
	}

	lcLibrarySourceType Type;
	std::map<std::string, lcLibraryPrimitive*> Primitives;
};

class lcPiecesLibrary : public QObject
{
	Q_OBJECT

public:
	lcPiecesLibrary();
	~lcPiecesLibrary();

	lcPiecesLibrary(const lcPiecesLibrary&) = delete;
	lcPiecesLibrary(lcPiecesLibrary&&) = delete;
	lcPiecesLibrary& operator=(const lcPiecesLibrary&) = delete;
	lcPiecesLibrary& operator=(lcPiecesLibrary&&) = delete;

	bool Load(const QString& LibraryPath, bool ShowProgress);
	void LoadColors(bool Update);
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
	void GetParts(lcArray<PieceInfo*>& Parts) const;

	std::vector<PieceInfo*> GetPartsFromSet(const std::vector<std::string>& PartIds) const;
	std::string GetPartId(const PieceInfo* Info) const;

	void GetPrimitiveFile(lcLibraryPrimitive* Primitive, std::function<void(lcFile& File)> Callback);
	void GetPieceFile(const char* FileName, std::function<void(lcFile& File)> Callback);

	bool IsPrimitive(const char* Name) const;
	lcLibraryPrimitive* FindPrimitive(const char* Name) const;
	bool LoadPrimitive(lcLibraryPrimitive* Primitive);

	bool SupportsStudLogo() const;
	void SetStudLogo(int StudLogo, bool Reload);

	int GetStudLogo() const
	{
		return mStudLogo;
	}

	void SetOfficialPieces()
	{
		if (mZipFiles[static_cast<int>(lcZipFileType::Official)])
			mNumOfficialPieces = (int)mPieces.size();
	}

	bool ShouldCancelLoading() const
	{
		return mCancelLoading;
	}

	void ReleaseBuffers(lcContext* Context);
	void UpdateBuffers(lcContext* Context);
	void UnloadUnusedParts();

	std::map<std::string, PieceInfo*> mPieces;
	int mNumOfficialPieces;

	std::vector<lcTexture*> mTextures;

	QDir mLibraryDir;

	bool mBuffersDirty;
	lcVertexBuffer mVertexBuffer;
	lcIndexBuffer mIndexBuffer;

signals:
	void PartLoaded(PieceInfo* Info);

protected:
	bool OpenArchive(const QString& FileName, lcZipFileType ZipFileType);
	bool OpenArchive(std::unique_ptr<lcFile> File, lcZipFileType ZipFileType);
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

	void UpdateStudLogoSource();

	std::vector<std::unique_ptr<lcLibrarySource>> mSources;

	QMutex mLoadMutex;
	QList<QFuture<void>> mLoadFutures;
	QList<PieceInfo*> mLoadQueue;

	QMutex mTextureMutex;
	std::vector<lcTexture*> mTextureUploads;

	int mStudLogo;

	QString mCachePath;
	qint64 mArchiveCheckSum[4];
	std::unique_ptr<lcZipFile> mZipFiles[static_cast<int>(lcZipFileType::Count)];
	bool mHasUnofficial;
	bool mCancelLoading;
};

