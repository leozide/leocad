#pragma once

#include "lc_context.h"
#include "lc_math.h"
#include "lc_array.h"
#include "lc_meshloader.h"

class PieceInfo;
class lcZipFile;
class lcLibraryMeshData;

enum class lcStudStyle
{
	Plain,
	ThinLinesLogo,
	OutlineLogo,
	SharpTopLogo,
	RoundedTopLogo,
	FlattenedLogo,
	HighContrast,
	HighContrastLogo,
	Count
};

constexpr bool lcIsHighContrast(lcStudStyle StudStyle)
{
	return StudStyle == lcStudStyle::HighContrast || StudStyle == lcStudStyle::HighContrastLogo;
}

enum class lcZipFileType
{
	Official,
	Unofficial,
	StudStyle,
	Count
};

enum class lcLibraryFolderType
{
	Official,
	Unofficial,
	Count
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
	explicit lcLibraryPrimitive(QString&& FileName, const char* Name, lcZipFileType ZipFileType, quint32 ZipFileIndex, bool Stud, bool StudStyle, bool SubFile)
		: mFileName(std::move(FileName))
	{
		strncpy(mName, Name, sizeof(mName)-1);
		mName[sizeof(mName) - 1] = 0;

		mZipFileType = ZipFileType;
		mZipFileIndex = ZipFileIndex;
		mState = lcPrimitiveState::NotLoaded;
		mStud = Stud;
		mStudStyle = StudStyle;
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
		mMeshData.Clear();
	}

	QString mFileName;
	char mName[LC_MAXNAME];
	lcZipFileType mZipFileType;
	quint32 mZipFileIndex;
	lcPrimitiveState mState;
	bool mStud;
	bool mStudStyle;
	bool mSubFile;
	lcLibraryMeshData mMeshData;
};

enum class lcLibrarySourceType
{
	Library,
	StudStyle
};

struct lcLibrarySource
{
	lcLibrarySource() = default;

	~lcLibrarySource()
	{
		for (const auto& PrimitiveIt : Primitives)
			delete PrimitiveIt.second;
	}

	lcLibrarySource(const lcLibrarySource&) = delete;
	lcLibrarySource(lcLibrarySource&&) = delete;
	lcLibrarySource& operator=(const lcLibrarySource&) = delete;
	lcLibrarySource& operator=(lcLibrarySource&&) = delete;

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
	void LoadColors();
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

	bool SupportsStudStyle() const;
	void SetStudStyle(lcStudStyle StudStyle, bool Reload, bool StudCylinderColorEnabled);

	lcStudStyle GetStudStyle() const
	{
		return mStudStyle;
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
	void ColorsLoaded();

protected:
	bool OpenArchive(const QString& FileName, lcZipFileType ZipFileType);
	bool OpenArchive(std::unique_ptr<lcFile> File, lcZipFileType ZipFileType);
	bool OpenDirectory(const QDir& LibraryDir, bool ShowProgress);
	void ReadArchiveDescriptions(const QString& OfficialFileName, const QString& UnofficialFileName);
	void ReadDirectoryDescriptions(const QFileInfoList (&FileLists)[static_cast<int>(lcLibraryFolderType::Count)], bool ShowProgress);

	bool ReadArchiveCacheFile(const QString& FileName, lcMemFile& CacheFile);
	bool WriteArchiveCacheFile(const QString& FileName, lcMemFile& CacheFile);
	bool LoadCacheIndex(const QString& FileName);
	bool SaveArchiveCacheIndex(const QString& FileName);
	bool LoadCachePiece(PieceInfo* Info);
	bool SaveCachePiece(PieceInfo* Info);
	bool ReadDirectoryCacheFile(const QString& FileName, lcMemFile& CacheFile);
	bool WriteDirectoryCacheFile(const QString& FileName, lcMemFile& CacheFile);

	static bool IsStudPrimitive(const char* FileName);
	static bool IsStudStylePrimitive(const char* FileName);
	void UpdateStudStyleSource();

	void ReleaseBuffers();

	std::vector<std::unique_ptr<lcLibrarySource>> mSources;

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	QRecursiveMutex mLoadMutex;
#else
	QMutex mLoadMutex;
#endif
	QList<QFuture<void>> mLoadFutures;
	QList<PieceInfo*> mLoadQueue;

	QMutex mTextureMutex;

	lcStudStyle mStudStyle;
	bool mStudCylinderColorEnabled;

	QString mCachePath;
	qint64 mArchiveCheckSum[4];
	std::unique_ptr<lcZipFile> mZipFiles[static_cast<int>(lcZipFileType::Count)];
	bool mHasUnofficial;
	bool mCancelLoading;
};
