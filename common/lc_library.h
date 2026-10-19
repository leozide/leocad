#pragma once

#include "lc_context.h"
#include "lc_math.h"
#include "lc_meshloader.h"
#include "lc_result.h"
#include "lc_assettypes.h"

class lcPieceData;
class lcPiece;
class lcMesh;
class lcTrainTrackInfo;
struct lcTrainTrackConnectionType;
class lcZipFile;
class lcLibraryMeshData;
class lcThumbnailManager;
class lcAssetLoader;
class lcTextureManager;
struct lcEmbeddedData;

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
using lcLibraryLoadMutex = QRecursiveMutex;
#else
// Qt 5 has no QRecursiveMutex; library release paths can reenter this lock.
class lcLibraryLoadMutex : public QMutex
{
public:
	lcLibraryLoadMutex()
		: QMutex(QMutex::Recursive)
	{
	}
};
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
		mLoadError.clear();
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
	QString mLoadError;
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

// Owns catalog/local identities, sources, references, and shared render buffers.
// Its asset loader owns pending work; workers build privately and the UI thread publishes.
// See docs/asset-loading.md for the loading and lifetime contracts.
class lcPiecesLibrary : public QObject
{
	Q_OBJECT

public:
	lcPiecesLibrary(lcStudStyle StudStyle, bool StudCylinderColorEnabled);
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
	void RemoveTemporaryPieceData();
	void ReleaseProjectPieceData(Project* OwnerProject);
	void TransferProjectPieceData(Project* Source, Project* Destination);

	void SetModelPieceDataName(lcPieceData* PieceData, const char* Name);
	bool RenamePieceData(lcPieceData* PieceData, const char* NewName);
	static std::string NormalizePieceDataName(const char* PieceName);
	// Returns one scoped model reference, ready to transfer to the model.
	lcPieceDataRef AcquireModelPieceData(const char* PieceName, Project* Project, bool& Reused);
	// Borrowed registered lookup: project names, then the shared catalog. No I/O or creation.
	lcPieceData* FindPieceData(const char* PieceName, Project* Project);
	// Resolves or creates an identity and returns one scoped reference; does not ensure readiness.
	lcPieceDataRef AcquirePieceData(const char* PieceName, Project* Project, bool CreateMissing, bool SearchProjectFolder);
	bool RemapProjectPieceData(lcPieceData* PieceData, const QString& ProjectDirectory, bool IsPreview);

	// Asset scheduling, waits, invalidation, and consumer releases below run on the UI thread.
	// Start or promote loading asynchronously. Adds no consumer references and never waits.
	// Priority controls scheduling only; the caller retains the identity through completion or cancellation.
	void RequestPieceDataLoad(lcPieceData* PieceData, lcLoadPriority LoadPriority);
	void NotifyAssetStatusChanged();
	// Start or promote loading, then wait. Adds no consumer references.
	// Returns false for failed or cancelled pieces.
	// Does not traverse container children or wait for per-instance synth geometry.
	bool WaitForPieceData(const lcPieceDataRef& PieceDataRef);
	bool WaitForPieceData(const std::vector<lcPieceDataRef>& PieceDataRefs);
	// Also settles container children/synths and updates bounds; caller retains the identity.
	bool EnsurePieceDataAssetsReady(const lcPieceDataRef& PieceData);

	// Obsoletes pending work and resets readiness without acquiring a reference.
	void InvalidatePieceDataLoad(lcPieceData* PieceData);

	// Replaces any pending generation for this instance; snapshots control points for workers.
	void RequestSynthMesh(lcPiece* Piece);
	void CancelSynthMesh(lcPiece* Piece);
	// Promote and wait for queued generations; false means missing or outdated generated geometry.
	// Call RequestSynthMesh() to start a generation; keep the instances alive through the wait.
	bool WaitForSynthMeshes(const std::vector<lcPiece*>& Pieces);
	// Last generation error, or an empty string; an empty error does not imply readiness.
	QString GetSynthMeshError(const lcPiece* Piece) const;
	void RebuildModelPieceData(lcPieceData* PieceData);
	// Loader helper: assumes a pending request/notification owns one in-flight hold.
	bool HasPieceDataConsumers(const lcPieceData* PieceData);
	void SetPieceDataLoadError(const lcPieceData* PieceData, QString Error);
	QString GetPieceDataLoadError(const lcPieceData* PieceData) const;
	void ClearPieceDataLoadError(const lcPieceData* PieceData);
	bool LoadBuiltinPieceLibrary();
	lcPieceSourceSnapshot SnapshotPieceDataSource(const lcPieceData* PieceData) const;
	// Worker build service: returns a converted private mesh or an error without publishing asset state.
	lcPieceBuildResult BuildPieceData(const lcPieceSourceSnapshot& Source);
	void SaveBuiltPieceDataCache(const lcPieceSourceSnapshot& Source, lcMesh& Mesh);

	// Borrowed registered lookup by resolved source. No I/O, creation, or acquisition.
	lcTexture* FindTexture(const lcFileSource& Source);
	// Resolve/create and return one scoped reference without decoding, uploading, or waiting.
	lcTextureRef AcquireTexture(const char* TextureName, Project* CurrentProject, bool SearchProjectFolder);
	lcTextureRef AcquireTexture(const char* TextureName, const std::vector<QString>& SearchDirectories, const std::map<QString, std::shared_ptr<const lcEmbeddedData>>& EmbeddedData, lcFileSource& Source);
	// Priority controls scheduling only. Both operations require the caller to retain the texture.
	void RequestTextureLoad(lcTexture* Texture, lcLoadPriority LoadPriority);
	bool WaitForTexture(const lcTextureRef& TextureRef);

	bool PieceDataInCategory(lcPieceData* PieceData, const char* CategoryKeywords) const;
	void GetCategoryEntries(int CategoryIndex, bool GroupPieces, std::vector<lcPieceData*>& SinglePieces, std::vector<lcPieceData*>& GroupedPieces);
	void GetCategoryEntries(const char* CategoryKeywords, bool GroupPieces, std::vector<lcPieceData*>& SinglePieces, std::vector<lcPieceData*>& GroupedPieces);
	void GetParts(std::vector<lcPieceData*>& Parts) const;
	std::vector<lcPieceData*> GetVisibleTrainTrackParts(const lcTrainTrackConnectionType& ConnectionType) const;

	std::vector<lcPieceData*> GetPartsFromSet(const std::vector<std::string>& PartIds) const;
	std::string GetPartId(const lcPieceData* PieceData) const;

	// Opens a resolved source on the reading thread, positioned at zero.
	// Caller owns the file; archive replacement requires draining workers first.
	lcResult<std::unique_ptr<lcFile>> OpenFile(const lcFileSource& Source);
	void GetPrimitiveFile(lcLibraryPrimitive* Primitive, std::function<void(lcFile& File)> Callback);
	void GetPieceDataFile(const char* FileName, std::function<void(lcFile& File)> Callback);

	bool IsPrimitive(const char* Name) const;
	lcLibraryPrimitive* FindPrimitive(const char* Name) const;
	lcResult<void> LoadPrimitive(lcLibraryPrimitive* Primitive);

	bool SupportsStudStyle() const;

	// Drain running jobs before changing settings; queue replacement meshes for asynchronous loading.
	void UpdateStudStyleAndColors(lcStudStyle StudStyle, bool StudCylinderColorEnabled);

	lcStudStyle GetStudStyle() const
	{
		return mStudStyle;
	}

	void SetOfficialPieceLibrary()
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

	std::map<std::string, lcPieceData*> mPieces;
	int mNumOfficialPieces;

	QDir mLibraryDir;

	bool mBuffersDirty;
	lcVertexBuffer mVertexBuffer;
	lcIndexBuffer mIndexBuffer;

signals:
	void AssetStatusChanged();
	void PartLoaded(lcPieceData* PieceData);
	void PartLoadFailed(lcPieceData* PieceData, const QString& Error);
	void GeneratedMeshSettled(lcPieceData* PieceData);
	void ColorsLoaded();

protected:
	friend class lcAssetLoader;
	template<typename T> friend class lcAssetRef;

	// Internal lifetime primitive for scoped acquisitions and loader holds. Never starts work.
	void AddPieceDataReference(lcPieceData* PieceData);

	// Releases one reference and lets the loader cancel unused queued work.
	void ReleasePieceData(lcPieceData* PieceData);

	// The loader calls this only when cancelling the current request, before releasing its hold.
	void PieceDataLoadCancelled(lcPieceData* PieceData);

	// Publish current results on the UI thread; the loader rejects stale requests first.
	// A successful piece result may have no mesh when its geometry is empty.
	bool PublishPieceDataMesh(lcPieceData* PieceData, std::unique_ptr<lcMesh> Mesh, QString Error);
	bool PublishSynthMesh(lcPiece* Piece, std::unique_ptr<lcMesh> Mesh, QString Error);
	// Apply decoded pixels or a decode failure; upload scheduling and retries stay in the loader.
	void PublishTexture(lcTexture* Texture, lcTextureBuildResult Result);

	lcResult<Project*> LoadExternalProject(const QFileInfo& FileInfo, bool Preview, bool DeferModelMeshRequests);
	lcResult<void> LoadPrimitiveData(lcLibraryPrimitive* Primitive);

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

	static bool IsStudPrimitive(const char* FileName);
	static bool IsStudStylePrimitive(const char* FileName);
	static qint32 MeshCacheSettingsKey(lcStudStyle StudStyle, bool StudCylinderColorEnabled);
	void UpdateLoadingMeshColors();
	void UpdateStudStyleSource();
	void RemovePieceData(lcPieceData* PieceData);
	void UnloadPieceData(lcPieceData* PieceData);
	void RegisterProjectPieceData(Project* Project, const std::string& Name, lcPieceData* PieceData);

	// Worker decode service: returns private pixels/errors without GL upload or publication.
	lcTextureBuildResult BuildTextureData(const lcFileSource& Source);
	void AddTextureReference(lcTexture* Texture);
	void ReleaseTexture(lcTexture* Texture);
	static QString FindProjectTextureFile(const QString& ProjectPath, const QString& TextureName);
	lcFileSource ResolveTextureSource(const char* TextureName, const std::vector<QString>& SearchDirectories, const std::map<QString, std::shared_ptr<const lcEmbeddedData>>& EmbeddedData, QString& ProjectPath);

	void ReleaseBuffers();

	std::vector<std::unique_ptr<lcLibrarySource>> mSources;
	// Each project maps its local filenames to lcPieceData pointers. This map tracks the
	// project for each local lcPieceData, or nullptr after that project is destroyed.
	std::unordered_map<lcPieceData*, Project*> mProjectPieces;

	// UI-thread live synth instances and their last generation errors (empty on success or retry).
	// Instances unregister before destruction; the library does not own them.
	std::unordered_map<lcPiece*, QString> mSynthPieces;

	// Protects identity/reference bookkeeping.
	// Recursive because library release and lookup paths can reenter this lock.
	lcLibraryLoadMutex mLoadMutex;

	// Active dependency edges include both nested loads and waits on other workers.
	QMutex mPrimitiveMutex;
	QWaitCondition mPrimitiveLoaded;
	std::unordered_map<QThread*, std::vector<lcLibraryPrimitive*>> mPrimitiveLoadStacks;
	std::unordered_map<lcLibraryPrimitive*, lcLibraryPrimitive*> mPrimitiveDependencies;

	// Sparse errors avoid storing a QString in every catalog identity.
	mutable QMutex mPieceErrorMutex;
	std::unordered_map<const lcPieceData*, QString> mFailedPartErrors;

	lcStudStyle mStudStyle;
	bool mStudCylinderColorEnabled;

	std::unique_ptr<lcAssetLoader> mAssetLoader;
	std::unique_ptr<lcThumbnailManager> mThumbnailManager;

	std::unique_ptr<lcTextureManager> mTextureManager;
	std::map<QString, QString> mTextureDiskEntries;
	std::map<QString, int> mTextureArchiveEntries[static_cast<int>(lcZipFileType::Count)];

	std::unique_ptr<lcMesh> mLoadingMesh;
	bool mStreamingBuffersPending = false;
	bool mBufferRepackScheduled = false;
	QString mCachePath;
	qint64 mArchiveCheckSum[4];
	std::unique_ptr<lcZipFile> mZipFiles[static_cast<int>(lcZipFileType::Count)];
	bool mHasUnofficial;
	std::atomic_bool mCancelLoading{ false };
};
