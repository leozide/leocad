#pragma once

#include "lc_context.h"
#include "lc_math.h"
#include "lc_meshloader.h"
#include "lc_result.h"
#include "lc_assettypes.h"
#include "lc_string.h"

class lcPieceData;
class lcPiece;
class lcMesh;
class lcTrainTrackInfo;
struct lcTrainTrackConnectionType;
class lcZipFile;
class lcLibraryMeshData;
struct lcEmbeddedData;

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
		lcstrcpy(mName, Name);

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

// Coordinates asset sources, loading, settings and shared render buffers.
// Piece/texture managers own identities and references; the loader owns pending work.
// Acquisition, scheduling and publication run on the UI thread; workers use source snapshots.
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

	// Replaces the current library with an archive or directory and initializes its metadata.
	// OfficialLibrary enables archive update checks; directories remain excluded.
	bool Load(const QString& LibraryPath, bool ShowProgress, bool OfficialLibrary);

	// Loads the bundled fallback library and its default settings.
	bool LoadBuiltinPieceLibrary();

	// Returns the official archive count for update checks, or zero for custom/directory libraries.
	int GetOfficialPieceCount() const
	{
		return mOfficialPieceCount;
	}

	// Reloads the configured colors and updates the loading mesh.
	void LoadColors();

	bool SupportsStudStyle() const;

	lcStudStyle GetStudStyle() const
	{
		return mStudStyle;
	}

	// Drains running workers before changing settings; queues replacement meshes asynchronously.
	void UpdateStudStyleAndColors(lcStudStyle StudStyle, bool StudCylinderColorEnabled);

	// Normalizes case and slashes, preserving the piece name length limit.
	static std::string NormalizePieceDataName(const char* PieceName);

	// Returns a borrowed project identity, falling back to the library. No I/O or creation.
	lcPieceData* FindPieceData(const char* PieceName, Project* Project);

	// Resolves or creates an identity and returns one scoped reference; does not ensure readiness.
	lcPieceDataRef AcquirePieceData(const char* PieceName, Project* Project, bool CreateMissing, bool SearchProjectFolder);

	// Creates or reuses a local identity and returns one scoped reference for the model.
	// Reused data is invalidated before returning.
	lcPieceDataRef AcquireModelPieceData(const char* PieceName, Project* Project, bool& Reused);

	// Updates the model filename and description without changing its index key.
	void SetModelPieceDataName(lcPieceData* PieceData, const char* Name);

	// Renames the model identity and its index key, rejecting collisions in the same scope.
	bool RenameModelPieceData(lcPieceData* PieceData, const char* NewName);

	// Rechecks an external/missing identity against the project directory; returns whether it changed.
	bool RemapProjectPieceData(lcPieceData* PieceData, const QString& ProjectDirectory, bool IsPreview);

	// Removes unreferenced non-library identities.
	void RemoveTemporaryPieceData();

	// Detaches the project's borrowed index and deletes its unreferenced local identities.
	void ReleaseProjectPieceData(Project* OwnerProject);

	// Transfers registrations and borrowed indexes, preserving destination name collisions.
	void TransferProjectPieceData(Project* Source, Project* Destination);

	// Starts or promotes asynchronous loading. Adds no consumer references and never waits.
	// Priority controls scheduling only; the caller retains the identity while it needs the result.
	void RequestPieceDataLoad(lcPieceData* PieceData, lcLoadPriority LoadPriority);

	// Starts or promotes loading, then waits. Adds no consumer references; false means failure/cancellation.
	// Does not traverse container children or wait for per-instance synth geometry.
	bool WaitForPieceData(const lcPieceDataRef& PieceDataRef);
	bool WaitForPieceData(const std::vector<lcPieceDataRef>& PieceDataRefs);

	// Also settles container children/synths and updates bounds; the caller retains the identity.
	bool EnsurePieceDataAssetsReady(const lcPieceDataRef& PieceData);

	// Obsoletes pending work and resets readiness without acquiring a reference.
	void InvalidatePieceDataLoad(lcPieceData* PieceData);

	// Invalidates the model mesh, releases old geometry and requests an asynchronous rebuild.
	void RebuildModelPieceData(lcPieceData* PieceData);

	// Sparse diagnostics; an empty error does not imply readiness.
	void SetPieceDataLoadError(const lcPieceData* PieceData, QString Error);
	QString GetPieceDataLoadError(const lcPieceData* PieceData) const;
	void ClearPieceDataLoadError(const lcPieceData* PieceData);

	// Returns a borrowed registered texture for a resolved source. No I/O or acquisition.
	lcTexture* FindTexture(const lcFileSource& Source);

	// Resolves or creates a texture and returns one scoped reference without decoding or uploading.
	lcTextureRef AcquireTexture(const char* TextureName, Project* CurrentProject, bool SearchProjectFolder);
	lcTextureRef AcquireTexture(const char* TextureName, const std::vector<QString>& SearchDirectories, const std::map<QString, std::shared_ptr<const lcEmbeddedData>>& EmbeddedData, lcFileSource& Source);

	// Starts or promotes asynchronous loading; the caller retains the texture. Never waits.
	void RequestTextureLoad(lcTexture* Texture, lcLoadPriority LoadPriority);

	// Starts or promotes loading, then waits for settlement; the caller retains the texture.
	bool WaitForTexture(const lcTextureRef& TextureRef);

	// Replaces pending generation for this instance; snapshots control points for workers.
	void RequestSynthMesh(lcPiece* Piece);

	// Cancels pending generation and removes the instance's diagnostic entry.
	void CancelSynthMesh(lcPiece* Piece);

	// Promotes and waits for queued generations; false means missing or outdated generated geometry.
	// Call RequestSynthMesh() to start a generation; keep the instances alive through the wait.
	bool WaitForSynthMeshes(const std::vector<lcPiece*>& Pieces);

	// Returns the last generation error; an empty error does not imply readiness.
	QString GetSynthMeshError(const lcPiece* Piece) const;

	// Returns a borrowed library snapshot in name order. Caller prevents identity teardown.
	std::vector<lcPieceData*> GetParts() const;

	// Matches library-part descriptions against category keywords.
	bool PieceDataInCategory(lcPieceData* PieceData, const char* CategoryKeywords) const;

	// Fills borrowed category entries, optionally grouping patterned pieces under their parent.
	void GetCategoryEntries(int CategoryIndex, bool GroupPieces, std::vector<lcPieceData*>& SinglePieces, std::vector<lcPieceData*>& GroupedPieces);
	void GetCategoryEntries(const char* CategoryKeywords, bool GroupPieces, std::vector<lcPieceData*>& SinglePieces, std::vector<lcPieceData*>& GroupedPieces);

	// Returns borrowed visible track parts compatible with the connection type.
	std::vector<lcPieceData*> GetVisibleTrainTrackParts(const lcTrainTrackConnectionType& ConnectionType) const;

	// Resolves normalized library IDs, preserving input order and duplicates; missing IDs are skipped.
	std::vector<lcPieceData*> GetPartsFromSet(const std::vector<std::string>& PartIds) const;

	// Returns the library index key, or an empty string for an unregistered/local identity.
	std::string GetPartId(const lcPieceData* PieceData) const;

	// Queries primitive sources; FindPrimitive() returns a borrowed entry.
	bool IsPrimitive(const char* Name) const;
	lcLibraryPrimitive* FindPrimitive(const char* Name) const;

	// Loads shared primitive data, waiting for another worker when necessary.
	lcResult<void> LoadPrimitive(lcLibraryPrimitive* Primitive);

	// Opens an include for the synchronous callback on the reading thread.
	void GetPrimitiveFile(lcLibraryPrimitive* Primitive, std::function<void(lcFile& File)> Callback);
	void GetPieceDataFile(const char* FileName, std::function<void(lcFile& File)> Callback);

	// Worker builds poll this shutdown flag without consulting live UI state.
	bool ShouldCancelLoading() const
	{
		return mCancelLoading.load();
	}

	// Notifies consumers to refresh pending/failed asset status.
	void NotifyAssetStatusChanged();

	// Returns the library-owned asynchronous thumbnail service.
	lcThumbnailManager* GetThumbnailManager() const
	{
		return mThumbnailManager.get();
	}

	// Rebuilds shared mesh buffers when dirty, using the supplied render context.
	void UpdateBuffers(lcContext* Context);

	// Checks unused library parts under the existing zero-reference release policy.
	void UnloadUnusedParts();

	// Returns the borrowed placeholder mesh for pending or failed piece geometry.
	lcMesh* GetLoadingMesh() const;

	QDir mLibraryDir;

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

	// Drains the loader and clears pieces before sources, textures and archives.
	// Consumers must release references before teardown; the library remains alive during callbacks.
	void Unload();

	// Internal retain/release primitives for scoped references. Retaining never starts work.
	// Piece release also lets the loader cancel unused queued work.
	void AddPieceDataReference(lcPieceData* PieceData);
	void ReleasePieceData(lcPieceData* PieceData);
	void AddTextureReference(lcTexture* Texture);
	void ReleaseTexture(lcTexture* Texture);

	// Snapshots live piece/model sources on the UI thread for a private worker build.
	lcPieceSourceSnapshot SnapshotPieceDataSource(const lcPieceData* PieceData) const;

	// Returns a converted private mesh or an error without publishing asset state.
	lcPieceBuildResult BuildPieceData(const lcPieceSourceSnapshot& Source);

	// Saves geometry using the settings and cache location captured for the build.
	void SaveBuiltPieceDataCache(const lcPieceSourceSnapshot& Source, lcMesh& Mesh);

	// Returns private decoded pixels/errors without GL upload or publication.
	lcTextureBuildResult BuildTextureData(const lcFileSource& Source);

	// The loader calls this only when cancelling the current request, before releasing its hold.
	void PieceDataLoadCancelled(lcPieceData* PieceData);

	// Publishes current results on the UI thread; the loader rejects stale requests first.
	// A successful piece result may have no mesh when its geometry is empty.
	bool PublishPieceDataMesh(lcPieceData* PieceData, std::unique_ptr<lcMesh> Mesh, QString Error);
	bool PublishSynthMesh(lcPiece* Piece, std::unique_ptr<lcMesh> Mesh, QString Error);

	// Applies decoded pixels or a decode failure; upload scheduling and retries stay in the loader.
	void PublishTexture(lcTexture* Texture, lcTextureBuildResult Result);

	// Opens a resolved source on the reading thread, positioned at zero.
	// Caller owns the file; archive replacement requires draining workers first.
	lcResult<std::unique_ptr<lcFile>> OpenFile(const lcFileSource& Source);

	// Loads an external project, rejecting recursive include cycles.
	lcResult<Project*> LoadExternalProject(const QFileInfo& FileInfo, bool Preview, bool DeferModelMeshRequests);

	// Discovers source entries before workers consume their metadata.
	bool OpenArchive(const QString& FileName, lcZipFileType ZipFileType);
	bool OpenArchive(std::unique_ptr<lcFile> File, lcZipFileType ZipFileType);
	bool OpenDirectory(const QDir& LibraryDir, bool ShowProgress);

	// Initializes descriptions from source files and compatible description caches.
	void ReadArchiveDescriptions(const QString& OfficialFileName, const QString& UnofficialFileName);
	void ReadDirectoryDescriptions(const QFileInfoList (&FileLists)[static_cast<int>(lcLibraryFolderType::Count)], bool ShowProgress);

	// Reads/writes compressed archive caches validated against archive checksums.
	bool ReadArchiveCacheFile(const QString& FileName, lcMemFile& CacheFile);
	bool WriteArchiveCacheFile(const QString& FileName, lcMemFile& CacheFile);

	// Restores/saves descriptions in library index order.
	bool LoadCacheIndex(const QString& FileName);
	bool SaveArchiveCacheIndex(const QString& FileName);

	// Reads/writes compressed directory caches with version and size checks.
	bool ReadDirectoryCacheFile(const QString& FileName, lcMemFile& CacheFile);
	bool WriteDirectoryCacheFile(const QString& FileName, lcMemFile& CacheFile);

	// Builds primitive data after dependency tracking selects this worker as its loader.
	lcResult<void> LoadPrimitiveData(lcLibraryPrimitive* Primitive);

	static bool IsStudPrimitive(const char* FileName);
	static bool IsStudStylePrimitive(const char* FileName);
	static qint32 MeshCacheSettingsKey(lcStudStyle StudStyle, bool StudCylinderColorEnabled);

	// Updates placeholder colors and the active stud-style source after settings change.
	void UpdateLoadingMeshColors();
	void UpdateStudStyleSource();

	// Resolves embedded, project and library texture sources on the UI thread.
	static QString FindProjectTextureFile(const QString& ProjectPath, const QString& TextureName);
	lcFileSource ResolveTextureSource(const char* TextureName, const std::vector<QString>& SearchDirectories, const std::map<QString, std::shared_ptr<const lcEmbeddedData>>& EmbeddedData, QString& ProjectPath);

	// Defers repacking until work settles, then marks buffers dirty and refreshes views.
	void ScheduleBufferRepack();

	void ReleaseBuffers();

	// Archive readers and discovered primitive sources; workers drain before replacement or teardown.
	std::unique_ptr<lcZipFile> mZipFiles[static_cast<int>(lcZipFileType::Count)];
	std::vector<std::unique_ptr<lcLibrarySource>> mSources;
	bool mHasUnofficial;

	// Captured before unofficial additions/overrides; custom libraries skip update checks.
	int mOfficialPieceCount = 0;

	// Settings captured by worker source snapshots and mesh cache keys.
	lcStudStyle mStudStyle;
	bool mStudCylinderColorEnabled;

	// Identity managers and asynchronous services. Unload() clears them in dependency order.
	std::unique_ptr<lcPieceManager> mPieceManager;
	std::unique_ptr<lcTextureManager> mTextureManager;
	std::unique_ptr<lcAssetLoader> mAssetLoader;
	std::unique_ptr<lcThumbnailManager> mThumbnailManager;

	// UI-thread live synth instances and their last generation errors (empty on success or retry).
	// Instances unregister before destruction; the library does not own them.
	std::unordered_map<lcPiece*, QString> mSynthPieces;

	// Active dependency edges include both nested loads and waits on other workers.
	QMutex mPrimitiveMutex;
	QWaitCondition mPrimitiveLoaded;
	std::unordered_map<QThread*, std::vector<lcLibraryPrimitive*>> mPrimitiveLoadStacks;
	std::unordered_map<lcLibraryPrimitive*, lcLibraryPrimitive*> mPrimitiveDependencies;

	// Normalized texture names mapped to disk paths or archive entry indices.
	std::map<QString, QString> mTextureDiskEntries;
	std::map<QString, int> mTextureArchiveEntries[static_cast<int>(lcZipFileType::Count)];

	// Cache location and official/unofficial archive size/timestamp validation data.
	QString mCachePath;
	qint64 mArchiveCheckSum[4];

	// Placeholder geometry and deferred shared-buffer repacking state.
	bool mBuffersDirty;
	std::unique_ptr<lcMesh> mLoadingMesh;
	bool mStreamingBuffersPending = false;
	bool mBufferRepackScheduled = false;

	// Shutdown flag polled by workers while the UI thread drains outstanding builds.
	std::atomic_bool mCancelLoading{ false };
};
