#pragma once

#include "lc_context.h"
#include "lc_math.h"
#include "lc_meshloader.h"
#include "lc_result.h"

class PieceInfo;
class lcPiece;
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
	void ReleaseProjectPieces(Project* OwnerProject);
	void TransferProjectPieces(Project* Source, Project* Destination);
	void RemovePiece(PieceInfo* Info);

	void SetModelPieceName(PieceInfo* Info, const char* Name);
	bool RenamePiece(PieceInfo* Info, const char* NewName);
	static std::string NormalizePieceName(const char* PieceName);
	// Returns one acquired model reference, paired with ReleasePieceInfo().
	PieceInfo* CreateModelPiece(const char* PieceName, Project* Project, bool& Reused);
	// Returns a borrowed identity; lookup does not acquire a reference or ensure readiness.
	PieceInfo* FindPiece(const char* PieceName, Project* Project, bool CreateMissing, bool SearchProjectFolder);
	bool RemapProjectPiece(PieceInfo* Info, const QString& ProjectDirectory, bool IsPreview);

	// Asset scheduling, waits, invalidation, and consumer releases below run on the UI thread.
	// Acquires one reference even on failure; always pair with ReleasePieceInfo().
	// Without Wait, true does not establish readiness. See docs/asset-loading.md.
	bool LoadPieceInfo(PieceInfo* Info, lcPieceLoadFlags Flags);
	void NotifyConsumersChanged();
	// Wait for these identities only, with temporary holds; false means failure/cancellation.
	// Does not traverse container children or wait for per-instance synth geometry.
	bool EnsurePieceReady(PieceInfo* Info);
	bool EnsurePiecesReady(const std::vector<PieceInfo*>& Parts);
	// Replaces any pending generation for this instance; snapshots control points for workers.
	void QueueSynthMesh(lcPiece* Piece);
	void CancelSynthMesh(lcPiece* Piece);
	// Caller keeps the instances alive through the wait; false means missing generated geometry.
	bool EnsureSynthMeshesReady(const std::vector<lcPiece*>& Pieces);
	QString GetSynthMeshError(const lcPiece* Piece) const;
	void QueueModelPiece(PieceInfo* Info);
	// Obsoletes pending work and resets readiness without acquiring a reference.
	void InvalidatePiece(PieceInfo* Info);
	// Waits through GPU upload; caller retains the texture, and false means not ready.
	bool EnsureTextureReady(lcTexture* Texture);
	// Releases one reference and lets the loader cancel unused queued work.
	void ReleasePieceInfo(PieceInfo* Info);
	// Lifetime hold only: does not start loading or establish readiness.
	void AddPieceReference(PieceInfo* Info);
	// Loader helper: assumes a pending request/notification owns one in-flight hold.
	bool HasPieceConsumers(const PieceInfo* Info);
	// Releases that in-flight hold without triggering consumer-release cancellation.
	void ReleasePieceLoadHold(PieceInfo* Info);
	void SetPieceLoadError(const PieceInfo* Info, QString Error);
	QString GetPieceLoadError(const PieceInfo* Info) const;
	void ClearPieceLoadError(const PieceInfo* Info);
	bool LoadBuiltinPieces();
	lcPartSourceSnapshot SnapshotPieceSource(const PieceInfo* Info) const;
	// Worker build service: returns private CPU data/errors without publishing asset state.
	lcPartBuildResult BuildPieceData(const lcPartSourceSnapshot& Source);
	void SaveBuiltPieceCache(const lcPartSourceSnapshot& Source, lcMesh& Mesh);
	// UI-thread drain of all work, including unrelated assets; not a success check.
	void WaitForLoadQueue();

	// UI-thread lookup and upload wait; returns a ready reference or nullptr on failure.
	// Release a returned reference with ReleaseTexture().
	lcTexture* FindTexture(const char* TextureName, Project* CurrentProject, bool SearchProjectFolder);
	// UI-thread lookup/acquisition only, without decoding or upload; caller releases the reference.
	lcTexture* FindTextureDeferred(const char* TextureName, const std::vector<QString>& SearchDirectories);
	lcTextureSourceSnapshot SnapshotTextureSource(const lcTexture* Texture) const;
	// Worker decode service: returns private pixels/errors without GL upload or publication.
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
	lcResult<void> LoadPrimitive(lcLibraryPrimitive* Primitive);

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
	void GeneratedMeshSettled(PieceInfo* Info);
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
	void RegisterProjectPiece(Project* Project, const std::string& Name, PieceInfo* Info);

	void ReleaseBuffers();

	std::vector<std::unique_ptr<lcLibrarySource>> mSources;
	// Each project maps its local filenames to PieceInfo pointers. This map tracks the
	// project for each local PieceInfo, or nullptr after that project is destroyed.
	std::unordered_map<PieceInfo*, Project*> mProjectPieces;

	// Protects identity/reference bookkeeping and shared primitive loading state.
	// Recursive because library release and lookup paths can reenter this lock.
	lcLibraryLoadMutex mLoadMutex;

	// Serializes texture source access, including archive extraction by workers.
	QMutex mTextureMutex;
	// Sparse errors avoid storing a QString in every catalog identity.
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
