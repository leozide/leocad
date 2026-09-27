#pragma once

#include "lc_context.h"
#include "lc_math.h"
#include "lc_meshloader.h"

class PieceInfo;
class lcMesh;
class lcTrainTrackInfo;
struct lcTrainTrackConnectionType;
class lcZipFile;
class lcLibraryMeshData;
class lcThumbnailManager;
class lcAssetLoader;
struct lcPartSourceSnapshot;
struct lcPartBuildResult;
struct lcTextureSourceSnapshot;
struct lcTextureBuildResult;

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
using lcLibraryLoadMutex = QRecursiveMutex;
#else
using lcLibraryLoadMutex = QMutex;
#endif

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

enum class lcPieceLoadFlag
{
	None = 0,
	Wait = 1 << 0,
	Visible = 1 << 1
};

Q_DECLARE_FLAGS(lcPieceLoadFlags, lcPieceLoadFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(lcPieceLoadFlags)

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

	lcThumbnailManager* GetThumbnailManager() const
	{
		return mThumbnailManager.get();
	}

	bool Load(const QString& LibraryPath, bool ShowProgress);
	void LoadColors();
	void Unload();
	void RemoveTemporaryPieces();
	std::vector<PieceInfo*> CaptureMappedPieces(const std::vector<PieceInfo*>& Pieces);
	void RestorePieceMappings(const std::vector<PieceInfo*>& Pieces);
	void RemovePiece(PieceInfo* Info);

	void SetModelPieceName(PieceInfo* Info, const char* Name);
	void RenamePiece(PieceInfo* Info, const char* NewName);
	PieceInfo* FindPiece(const char* PieceName, Project* Project, bool CreatePlaceholder, bool SearchProjectFolder);
	bool LoadPieceInfo(PieceInfo* Info, lcPieceLoadFlags Flags);
	void NotifyConsumersChanged();
	bool EnsurePieceReady(PieceInfo* Info);
	bool EnsurePiecesReady(const std::vector<PieceInfo*>& Parts);
	void QueueModelPiece(PieceInfo* Info);
	void SetPieceRequestsVisible(const std::vector<PieceInfo*>& Parts, bool Visible);
	bool EnsureTextureReady(lcTexture* Texture);
	void ReleasePieceInfo(PieceInfo* Info);
	void AddPieceReference(PieceInfo* Info);
	bool HasPieceConsumers(const PieceInfo* Info);
	void ReleasePieceLoadHold(PieceInfo* Info);
	void SetPieceLoadError(const PieceInfo* Info, QString Error);
	QString GetPieceLoadError(const PieceInfo* Info) const;
	void ClearPieceLoadError(const PieceInfo* Info);
	bool LoadBuiltinPieces();
	lcPartSourceSnapshot SnapshotPieceSource(const PieceInfo* Info) const;
	lcPartBuildResult BuildPieceData(const lcPartSourceSnapshot& Source);
	void SaveBuiltPieceCache(const lcPartSourceSnapshot& Source, lcMesh& Mesh);
	void WaitForLoadQueue();

	// Returns a texture reference that the caller must release.
	lcTexture* FindTexture(const char* TextureName, Project* CurrentProject, bool SearchProjectFolder);
	lcTexture* FindTextureDeferred(const char* TextureName, const QString& ProjectPath);
	lcTextureSourceSnapshot SnapshotTextureSource(const lcTexture* Texture) const;
	lcTextureBuildResult BuildTextureData(const lcTextureSourceSnapshot& Source);
	bool LoadTexture(lcTexture* Texture);
	void ReleaseTexture(lcTexture* Texture);

	bool PieceInCategory(PieceInfo* Info, const char* CategoryKeywords) const;
	void GetCategoryEntries(int CategoryIndex, bool GroupPieces, std::vector<PieceInfo*>& SinglePieces, std::vector<PieceInfo*>& GroupedPieces);
	void GetCategoryEntries(const char* CategoryKeywords, bool GroupPieces, std::vector<PieceInfo*>& SinglePieces, std::vector<PieceInfo*>& GroupedPieces);
	void GetParts(std::vector<PieceInfo*>& Parts) const;
	std::vector<PieceInfo*> GetVisibleTrainTrackParts(const lcTrainTrackConnectionType& ConnectionType) const;

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
		return mCancelLoading.load();
	}

	void UpdateBuffers(lcContext* Context);
	void ScheduleBufferRepack();
	void UnloadUnusedParts();
	lcMesh* GetLoadingMesh() const;

	std::map<std::string, PieceInfo*> mPieces;
	int mNumOfficialPieces;

	std::vector<lcTexture*> mTextures;

	QDir mLibraryDir;

	bool mBuffersDirty;
	lcVertexBuffer mVertexBuffer;
	lcIndexBuffer mIndexBuffer;

signals:
	void AssetRequestsChanged();
	void PartLoaded(PieceInfo* Info);
	void PartLoadFailed(PieceInfo* Info, const QString& Error);
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
	bool ReadDirectoryCacheFile(const QString& FileName, lcMemFile& CacheFile);
	bool WriteDirectoryCacheFile(const QString& FileName, lcMemFile& CacheFile);

	static QString FindProjectTextureFile(const QString& ProjectPath, const QString& TextureName);
	static bool IsStudPrimitive(const char* FileName);
	static bool IsStudStylePrimitive(const char* FileName);
	static qint32 MeshCacheSettingsKey(lcStudStyle StudStyle, bool StudCylinderColorEnabled);
	void UpdateLoadingMeshColors();
	void UpdateStudStyleSource();
	void UnloadPieceInfo(PieceInfo* Info);
	void DetachPiece(PieceInfo* Info, const std::string& Name);
	void RestoreDetachedPiece(const std::string& Name);

	void ReleaseBuffers();

	std::vector<std::unique_ptr<lcLibrarySource>> mSources;
	struct DetachedPiece
	{
		std::string Name;
		quint64 Order;
	};

	// Entries displaced by a project-local piece with the same name.
	std::unordered_map<PieceInfo*, DetachedPiece> mDetachedPieces;
	quint64 mNextDetachedPieceOrder = 0;

	lcLibraryLoadMutex mLoadMutex;

	QMutex mTextureMutex;
	mutable QMutex mPieceErrorMutex;
	std::unordered_map<const PieceInfo*, QString> mFailedPartErrors;

	lcStudStyle mStudStyle;
	bool mStudCylinderColorEnabled;

	std::unique_ptr<lcAssetLoader> mAssetLoader;
	std::unique_ptr<lcThumbnailManager> mThumbnailManager;
	std::unique_ptr<lcMesh> mLoadingMesh;
	bool mStreamingBuffersPending = false;
	bool mBufferRepackScheduled = false;
	QString mCachePath;
	qint64 mArchiveCheckSum[4];
	std::unique_ptr<lcZipFile> mZipFiles[static_cast<int>(lcZipFileType::Count)];
	bool mHasUnofficial;
	std::atomic_bool mCancelLoading{ false };
};
