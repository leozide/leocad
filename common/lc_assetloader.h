#pragma once

#include "lc_library.h"
#include "lc_texturemanager.h"
#include "piece.h"

class PieceInfo;
class lcPiece;
class lcMesh;
class lcSynthInfo;
class lcMemFile;
class lcLibraryMeshData;
class lcTexture;
enum class lcTextureLoadError;

// Copied on the UI thread so workers do not consult live models or project lookup.
struct lcPieceSourceSnapshot
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
struct lcPieceBuildResult
{
	lcPieceBuildResult() = default;
	~lcPieceBuildResult();
	lcPieceBuildResult(lcPieceBuildResult&&) = default;
	lcPieceBuildResult& operator=(lcPieceBuildResult&&) = default;
	lcPieceBuildResult(const lcPieceBuildResult&) = delete;
	lcPieceBuildResult& operator=(const lcPieceBuildResult&) = delete;

	quint64 Generation = 0;
	std::unique_ptr<lcLibraryMeshData> MeshData;
	std::unique_ptr<lcMesh> Mesh;
	std::unique_ptr<lcMemFile> CacheData;
	std::vector<quint32> ColorCodes;
	bool EmptyGeometry = false;
	QStringList TextureDependencies;
	QString Error;
};

// Owned by lcPiecesLibrary. UI-thread entry points schedule private worker builds;
// UI-thread completion processing resolves colors, uploads textures, and publishes meshes.
// See docs/asset-loading.md for readiness, ownership, and caller policies.
class lcAssetLoader : public QObject
{
public:
	lcAssetLoader(lcPiecesLibrary* Library);
	~lcAssetLoader() override;

	// Start using a piece: acquire a reference and load it if needed. Release it with ReleasePieceInfo().
	// Visible raises its loading priority. Wait blocks until loading settles and returns whether it succeeded;
	// without Wait, this returns true immediately, even if the piece is not ready yet.
	bool LoadPieceInfo(PieceInfo* Info, lcPieceLoadFlags Flags);

	// Queue a background build for a piece the caller already owns, without acquiring a consumer reference or waiting.
	// Reuse any existing request, keeping its priority.
	void QueuePiece(PieceInfo* Info);

	// Build this instance's generated mesh from its current control points in the background.
	// Cancel any older request and keep the previous mesh visible while the replacement is built.
	void QueueSynthMesh(lcPiece* Piece);

	bool EnsurePieceReady(PieceInfo* Info);
	bool EnsurePiecesReady(const std::vector<PieceInfo*>& Pieces);
	bool EnsureSynthMeshesReady(const std::vector<lcPiece*>& Pieces);
	bool EnsureTextureReady(lcTexture* Texture);
	void WaitForLoadQueue();
	bool HasPendingWork();
	QString GetSynthMeshError(const lcPiece* Piece) const;

	void RebuildModelPiece(PieceInfo* Info);
	void InvalidatePiece(PieceInfo* Info);
	void OnConsumerReleased(PieceInfo* Info);
	void CancelSynthMesh(lcPiece* Piece);
	void ReloadSynthMeshes();
	void PauseQueuedWork();
	void ResumeQueuedWork();
	void CancelAndDrain();

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
	struct PieceRequest
	{
		PieceInfo* Info;
		quint64 Generation;
		lcPieceSourceSnapshot Source;
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
		lcFileSource Source;
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

	using PieceRequestMap = std::map<PieceInfo*, std::shared_ptr<PieceRequest>>;
	using TextureRequestMap = std::map<lcTexture*, std::shared_ptr<TextureRequest>>;
	using SynthRequestMap = std::map<lcPiece*, std::shared_ptr<SynthRequest>>;

	// Workers hand off one private result here; no live asset is published by a worker.
	struct Completion
	{
		std::shared_ptr<PieceRequest> RequestedPiece;
		lcPieceBuildResult Result;
		std::shared_ptr<TextureRequest> RequestedTexture;
		lcTextureBuildResult TextureResult;
		std::shared_ptr<SynthRequest> RequestedSynth;
		std::unique_ptr<lcLibraryMeshData> SynthMeshData;
		std::unique_ptr<lcMesh> SynthMesh;
	};

	// UI-thread delivery retains the load hold after publication, until callbacks finish.
	struct Notification
	{
		std::shared_ptr<PieceRequest> RequestedPiece;
		bool Loaded;
		QString Error;
	};

	// Retains the base identity; cancellation makes delivery safe after instance deletion.
	struct SynthNotification
	{
		std::shared_ptr<SynthRequest> RequestedSynth;
		PieceInfo* Info;
	};

	// Enqueue and promote requests. Workers read copied sources and private build data.
	// Acquires mQueueMutex internally; callers must not already hold it.
	void EnqueuePiece(PieceInfo* Info, Priority LoadPriority);
	// Caller holds mQueueMutex.
	void PromoteRequestPriorityLocked(const std::shared_ptr<PieceRequest>& RequestedPiece, Priority LoadPriority);
	void QueueTexture(lcTexture* Texture, Priority LoadPriority);
	// Caller holds mQueueMutex.
	void StartWorkersLocked();
	void RunWorker();

	// UI-thread completion handling: colors, conversion, texture dependencies, publication.
	bool ProcessCompletions();
	void CompletePiece(Completion& Completed);
	void CompleteSynth(Completion& Completed);
	void CompleteTexture(Completion& Completed);
	static void ResolveMeshColors(lcLibraryMeshData& MeshData, std::vector<bool>& ColorTranslucency, int& DefaultColorIndex);
	bool ResolveAndQueueMeshTextures(lcMesh& Mesh, const std::vector<QString>& SearchDirectories,
		const std::map<QString, std::shared_ptr<const lcEmbeddedData>>& EmbeddedData, Priority LoadPriority, QString& Error);
	static bool MeshTexturesSettled(const lcMesh& Mesh, QString& Error);
	static QString TextureFailureMessage(const lcTexture* Texture);
	void UploadTextureRequest(const std::shared_ptr<TextureRequest>& RequestedTexture);
	void FinishTextureRequest(const std::shared_ptr<TextureRequest>& RequestedTexture);
	void FinishPiecesWithSettledTextures();
	void FinishSynthsWithSettledTextures();
	void FinishPiece(const std::shared_ptr<PieceRequest>& RequestedPiece);
	void FinishSynth(const std::shared_ptr<SynthRequest>& RequestedSynth);

	// Notify only from the event handler, after synchronous waits have returned.
	void DispatchNotifications();
	bool event(QEvent* Event) override;
	// Caller holds mQueueMutex; the condition wait releases it and reacquires it on return.
	void WaitForResultsLocked();
	bool WaitForRequest(PieceInfo* Info);

	void CancelUnusedTextureRequests();

	lcPiecesLibrary* const mLibrary;
	// Protects scheduling maps/queues, worker counts, and result handoff.
	// Private build data passes between worker and UI phases through completions.
	QMutex mQueueMutex;
	QWaitCondition mResultReady;
	// Serializes full queue drains; scoped readiness waits do not acquire this mutex.
	QMutex mDrainMutex;
	PieceRequestMap mRequests;
	TextureRequestMap mTextureRequests;
	SynthRequestMap mSynthRequests;
	// UI-thread diagnostics and instance tracking; these do not own the pieces.
	std::unordered_map<const lcPiece*, QString> mFailedSynthErrors;
	std::unordered_set<lcPiece*> mSynthPieces;
	// Pending worker phases remain here while paused; mPaused prevents dispatch.
	std::deque<std::shared_ptr<PieceRequest>> mQueue;
	std::deque<std::shared_ptr<TextureRequest>> mTextureQueue;
	std::deque<std::shared_ptr<SynthRequest>> mSynthQueue;
	std::deque<Completion> mCompletions;
	std::deque<std::shared_ptr<PieceRequest>> mStarted;
	std::deque<std::shared_ptr<TextureRequest>> mStartedTextures;
	// UI-thread only; public signals are deferred until outside synchronous waits.
	std::deque<Notification> mNotifications;
	std::deque<SynthNotification> mSynthNotifications;
	std::vector<QFuture<void>> mFutures;
	int mActiveWorkers = 0;
	quint64 mGeneration = 1;
	bool mPaused = false;
	bool mStopping = false;

	Q_DECLARE_TR_FUNCTIONS(lcAssetLoader)
};
