#pragma once

#include "lc_context.h"
#include "lc_mesh.h"
#include "lc_math.h"
#include "lc_array.h"
#include "lc_meshloader.h"

class PieceInfo;
class lcZipFile;
class lcLibraryMeshData;

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

enum class lcPrimitiveState
{
	NOT_LOADED,
	LOADING,
	LOADED
};

class lcLibraryPrimitive
{
public:
	explicit lcLibraryPrimitive(QString&& FileName, const char* Name, lcZipFileType ZipFileType, quint32 ZipFileIndex, bool Stud, bool SubFile)
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

	void Unload()
	{
		mState = lcPrimitiveState::NOT_LOADED;
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
	void GetParts(lcArray<PieceInfo*>& Parts) const;

	std::vector<PieceInfo*> GetFavorites() const;
	bool IsFavorite(const PieceInfo* Info) const;
	void AddToFavorites(const PieceInfo* Info) const;
	void RemoveFromFavorites(const PieceInfo* Info) const;

	void GetPrimitiveFile(lcLibraryPrimitive* Primitive, std::function<void(lcFile& File)> Callback);
	void GetPieceFile(const char* FileName, std::function<void(lcFile& File)> Callback);

	bool IsPrimitive(const char* Name) const
	{
		return mPrimitives.find(Name) != mPrimitives.end();
	}

	lcLibraryPrimitive* FindPrimitive(const char* Name) const
	{
		const auto PrimitiveIt = mPrimitives.find(Name);
		return PrimitiveIt != mPrimitives.end() ? PrimitiveIt->second : nullptr;
	}

	bool LoadPrimitive(lcLibraryPrimitive* Primitive);

	void SetStudLogo(int StudLogo, bool Reload);

	int GetStudLogo() const
	{
		return mStudLogo;
	}

	void SetOfficialPieces()
	{
		if (mZipFiles[LC_ZIPFILE_OFFICIAL])
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
	std::map<std::string, lcLibraryPrimitive*> mPrimitives;
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

	bool GetStudLogoFile(lcMemFile& PrimFile, int StudLogo, bool OpenStud);

	QMutex mLoadMutex;
	QList<QFuture<void>> mLoadFutures;
	QList<PieceInfo*> mLoadQueue;

	QMutex mTextureMutex;
	std::vector<lcTexture*> mTextureUploads;

	int mStudLogo;

	QString mCachePath;
	qint64 mArchiveCheckSum[4];
	QString mLibraryFileName;
	QString mUnofficialFileName;
	lcZipFile* mZipFiles[LC_NUM_ZIPFILES];
	bool mHasUnofficial;
	bool mCancelLoading;
};

