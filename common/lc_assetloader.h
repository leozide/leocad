#pragma once

#include "lc_library.h"
#include "piece.h"

class PieceInfo;
class lcPiece;
class lcMesh;
class lcSynthInfo;
class lcMemFile;
class lcLibraryMeshData;
class Image;
class lcTexture;
enum class lcTextureLoadError;

// Copied on the UI thread so workers do not consult live models or project lookup.
struct lcPartSourceSnapshot
{
	QString FileName;
	QString LibraryDirectory;
	QString CachePath;
	std::vector<QString> TextureSearchDirectories;
	std::map<QString, std::shared_ptr<const lcEmbeddedData>> EmbeddedData;
	QStringList InlineMeshLines;
	bool InlineModel = false;
	bool SkipCache = false;
	lcZipFileType ZipFileType = lcZipFileType::Count;
	int ZipFileIndex = -1;
	lcStudStyle StudStyle = lcStudStyle::Plain;
	bool StudCylinderColorEnabled = false;
};

// Private worker output; meshes and dependencies are published only on the UI thread.
struct lcPartBuildResult
{
	lcPartBuildResult() = default;
	~lcPartBuildResult();
	lcPartBuildResult(lcPartBuildResult&&) = default;
	lcPartBuildResult& operator=(lcPartBuildResult&&) = default;
	lcPartBuildResult(const lcPartBuildResult&) = delete;
	lcPartBuildResult& operator=(const lcPartBuildResult&) = delete;

	quint64 RequestId = 0;
	quint64 Generation = 0;
	std::unique_ptr<lcLibraryMeshData> MeshData;
	std::unique_ptr<lcMesh> Mesh;
	std::unique_ptr<lcMemFile> CacheData;
	std::vector<quint32> ColorCodes;
	bool EmptyGeometry = false;
	QStringList TextureDependencies;
	QString Error;
};

struct lcTextureSourceSnapshot
{
	std::shared_ptr<const lcEmbeddedData> EmbeddedData;
	QString FilePath;
	int ArchiveType = -1;
	int ArchiveIndex = -1;
};

struct lcTextureBuildResult
{
	lcTextureBuildResult() = default;
	~lcTextureBuildResult();
	lcTextureBuildResult(lcTextureBuildResult&&) = default;
	lcTextureBuildResult& operator=(lcTextureBuildResult&&) = default;
	lcTextureBuildResult(const lcTextureBuildResult&) = delete;
	lcTextureBuildResult& operator=(const lcTextureBuildResult&) = delete;

	std::unique_ptr<Image> DecodedImage;
	lcTextureLoadError Error{};
	QString ErrorDetails;
};

// Owned by lcPiecesLibrary. UI-thread entry points schedule private worker builds;
// UI-thread completion processing resolves colors, uploads textures, and publishes meshes.
// See docs/asset-loading.md for readiness, ownership, and caller policies.
class lcAssetLoader : public QObject
{
public:
	lcAssetLoader(lcPiecesLibrary* Library);
	~lcAssetLoader() override;

	bool LoadPieceInfo(PieceInfo* Info, lcPieceLoadFlags Flags);
	void QueuePiece(PieceInfo* Info, bool Priority);
	bool EnsurePieceReady(PieceInfo* Info);
	bool EnsurePiecesReady(const std::vector<PieceInfo*>& Parts);
	void QueueSynthMesh(lcPiece* Piece);
	void CancelSynthMesh(lcPiece* Piece);
	bool EnsureSynthMeshesReady(const std::vector<lcPiece*>& Pieces);
	QString GetSynthMeshError(const lcPiece* Piece) const;
	void ReloadSynthMeshes();
	void QueueModelPiece(PieceInfo* Info);
	void InvalidatePiece(PieceInfo* Info);
	bool EnsureTextureReady(lcTexture* Texture);
	void WaitForLoadQueue();
	void CancelAndDrain();
	void PauseQueuedWork();
	void ResumeQueuedWork();
	void OnConsumerReleased(PieceInfo* Info);
	bool HasPendingWork();

private:
	enum class Priority
	{
		// Requests only promote; the priority lasts until completion or cancellation.
		Background, // Ordinary queued asset work.
		Visible,    // Assets requested for the visible model.
		Blocking    // Assets needed by a synchronous wait.
	};

	// Owns one PieceInfo load hold through completion and deferred notification.
	// Parsing and conversion are separate worker phases; textures settle before publication.
	struct Request
	{
		PieceInfo* Info;
		quint64 Id;
		quint64 Generation;
		lcPartSourceSnapshot Source;
		Priority LoadPriority;
		qint64 EnqueuedAt;
		bool Running = false;   // A worker phase is outstanding, including its completion.
		bool Obsolete = false;  // Its result must not publish, even if the build succeeds.
		bool Terminal = false;  // The wait has settled; this does not imply success.
		bool Succeeded = false; // Published successfully, including valid empty geometry.
		bool ConvertingMesh = false;
		std::unique_ptr<lcLibraryMeshData> MeshData;
		std::vector<bool> ColorTranslucency;
		int DefaultColorIndex = 0;
		std::unique_ptr<lcMesh> StagedMesh;
		bool EmptyGeometry = false;
		bool SaveCache = false;
		QString Error;
	};

	// Holds a texture reference through CPU decoding and UI-thread upload/retries.
	struct TextureRequest
	{
		lcTexture* Texture;
		quint64 Generation;
		lcTextureSourceSnapshot Source;
		Priority LoadPriority;
		qint64 EnqueuedAt;
		bool Running = false;
		int UploadAttempts = 0;
		qint64 RetryAt = 0;
	};

	// The instance cancels before destruction; workers use the copied definition/controls.
	// Cancelled is also checked during expensive generation and conversion.
	struct SynthRequest
	{
		lcPiece* Piece;
		std::shared_ptr<const lcSynthInfo> SynthInfo;
		std::vector<lcPieceControlPoint> ControlPoints;
		Priority LoadPriority;
		qint64 EnqueuedAt;
		quint64 Generation;
		bool Running = false;
		std::atomic_bool Cancelled = false;
		bool Terminal = false;
		bool ConvertingMesh = false;
		std::unique_ptr<lcLibraryMeshData> MeshData;
		std::vector<bool> ColorTranslucency;
		int DefaultColorIndex = 0;
		std::unique_ptr<lcMesh> StagedMesh;
		QString Error;
	};

	using PartRequestMap = std::map<PieceInfo*, std::shared_ptr<Request>>;
	using TextureRequestMap = std::map<lcTexture*, std::shared_ptr<TextureRequest>>;
	using SynthRequestMap = std::map<lcPiece*, std::shared_ptr<SynthRequest>>;

	// Workers hand off one private result here; no live asset is published by a worker.
	struct Completion
	{
		std::shared_ptr<Request> RequestedPart;
		lcPartBuildResult Result;
		std::shared_ptr<TextureRequest> RequestedTexture;
		lcTextureBuildResult TextureResult;
		std::shared_ptr<SynthRequest> RequestedSynth;
		std::unique_ptr<lcLibraryMeshData> SynthMeshData;
		std::unique_ptr<lcMesh> SynthMesh;
	};

	// UI-thread delivery retains the load hold after publication, until callbacks finish.
	struct Notification
	{
		std::shared_ptr<Request> RequestedPart;
		bool Loaded;
		QString Error;
	};

	// Retains the base identity; cancellation makes delivery safe after instance deletion.
	struct SynthNotification
	{
		std::shared_ptr<SynthRequest> RequestedSynth;
		PieceInfo* Info;
	};

	// Acquires mQueueMutex internally; callers must not already hold it.
	void EnqueuePiece(PieceInfo* Info, Priority LoadPriority);
	// Caller holds mQueueMutex.
	void PromoteRequestPriorityLocked(const std::shared_ptr<Request>& RequestedPart, Priority LoadPriority);
	void QueueTexture(lcTexture* Texture, Priority LoadPriority);
	static QString TextureFailureMessage(const lcTexture* Texture);
	void CancelUnusedTextureRequests();
	// Caller holds mQueueMutex.
	void StartWorkersLocked();
	void LoadQueuedPieces();
	bool ProcessCompletions();
	void DispatchNotifications();
	void FinishPart(const std::shared_ptr<Request>& RequestedPart);
	void CheckWaitingParts();
	void CheckWaitingSynths();
	void FinishSynth(const std::shared_ptr<SynthRequest>& RequestedSynth);
	void UploadTextureRequest(const std::shared_ptr<TextureRequest>& RequestedTexture);
	void FinishTextureRequest(const std::shared_ptr<TextureRequest>& RequestedTexture);
	// Caller holds mQueueMutex; the condition wait releases it and reacquires it on return.
	void WaitForResultsLocked();
	bool WaitForRequest(PieceInfo* Info);
	bool event(QEvent* Event) override;

	lcPiecesLibrary* const mLibrary;
	// Protects scheduling maps/queues, worker counts, and result handoff.
	// Private build data passes between worker and UI phases through completions.
	QMutex mQueueMutex;
	QWaitCondition mResultReady;
	// Serializes full queue drains; scoped readiness waits do not acquire this mutex.
	QMutex mDrainMutex;
	PartRequestMap mRequests;
	TextureRequestMap mTextureRequests;
	SynthRequestMap mSynthRequests;
	// UI-thread diagnostics and instance tracking; these do not own the pieces.
	std::unordered_map<const lcPiece*, QString> mFailedSynthErrors;
	std::unordered_set<lcPiece*> mSynthPieces;
	std::deque<std::shared_ptr<Request>> mQueue;
	std::deque<std::shared_ptr<Request>> mPausedQueue;
	std::deque<std::shared_ptr<TextureRequest>> mTextureQueue;
	std::deque<std::shared_ptr<TextureRequest>> mPausedTextureQueue;
	std::deque<std::shared_ptr<SynthRequest>> mSynthQueue;
	std::deque<std::shared_ptr<SynthRequest>> mPausedSynthQueue;
	std::deque<Completion> mCompletions;
	std::deque<std::shared_ptr<Request>> mStarted;
	std::deque<std::shared_ptr<TextureRequest>> mStartedTextures;
	// UI-thread only; public signals are deferred until outside synchronous waits.
	std::deque<Notification> mNotifications;
	std::deque<SynthNotification> mSynthNotifications;
	std::vector<QFuture<void>> mFutures;
	int mActiveWorkers = 0;
	quint64 mNextRequestId = 1;
	quint64 mGeneration = 1;
	bool mPaused = false;
	bool mStopping = false;

	Q_DECLARE_TR_FUNCTIONS(lcAssetLoader)
};
