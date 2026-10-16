#pragma once

#include "lc_library.h"
#include "lc_texturemanager.h"
#include "piece.h"

class PieceInfo;
class lcPiece;
class lcMesh;
class lcSynthInfo;
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

	std::unique_ptr<lcMesh> Mesh;
	bool FromCache = false;
	bool EmptyGeometry = false;
	QString Error;
};

// Owned by lcPiecesLibrary. Instance methods run on the UI thread and schedule private worker builds;
// UI-thread completion processing resolves colors, uploads textures, and publishes meshes.
// See docs/asset-loading.md for readiness, ownership, and caller policies.
class lcAssetLoader : public QObject
{
public:
	lcAssetLoader(lcPiecesLibrary* Library);
	~lcAssetLoader() override;

	// Load a piece the library has already acquired for the caller. Adds no consumer reference.
	// Low and Normal return immediately; true does not mean the piece is ready yet.
	// Blocking promotes urgent work, waits until loading settles, and returns whether it succeeded.
	bool RequestPieceLoad(PieceInfo* Info, lcPieceLoadPriority LoadPriority);

	// Start or promote loading, then wait and returns true if all pieces are ready. Adds no consumer references.
	// Waits for these identities only, without traversing container children or generated meshes.
	bool WaitForPieces(const std::vector<PieceInfo*>& Pieces);

	// Called when the last consumer releases the piece. Cancels queued work;
	// running work finishes privately and is discarded if the piece is still unused.
	void CancelPieceLoad(PieceInfo* Info);

	// Discard pending load results and notifications, reset the state to Unloaded, and clear errors.
	// Keeps the current mesh; the caller decides whether to unload it or request another load.
	void InvalidatePieceLoad(PieceInfo* Info);

	// Build this instance's generated mesh from its current control points in the background.
	// Cancel any older request and keep the previous mesh visible while the replacement is built.
	void RequestSynthMesh(lcPiece* Piece);

	// Promote and wait for queued generations. Returns true if every required generated mesh is current.
	// Call RequestSynthMesh() to start a generation; the caller keeps the instances alive through the wait.
	bool WaitForSynthMeshes(const std::vector<lcPiece*>& Pieces);

	// Cancel this instance's generation and suppress its deferred notifications.
	// Running jobs stop cooperatively; any published mesh is kept.
	void CancelSynthMesh(lcPiece* Piece);

	// Returns this instance's recorded generation error, or an empty string if none is recorded.
	// An empty string does not establish readiness; check the generated mesh separately.
	QString GetSynthMeshError(const lcPiece* Piece) const;

	// Start or promote decoding as needed, then wait through upload and retries. The caller retains the texture.
	bool WaitForTexture(lcTexture* Texture);

	// True while piece, texture, or generated-mesh requests or worker completions remain unsettled.
	// Includes paused work and upload retries; deferred notifications alone do not count.
	bool HasPendingWork();

	// Pause dispatch and drain assigned jobs before shared library data changes.
	// Discard meshes still awaiting textures so they can be rebuilt with the new settings.
	void PauseQueuedWork();

	// Refresh queued piece source snapshots and resume dispatch with the current settings.
	// Does not wait for pending work to finish.
	void ResumeQueuedWork();

private:
	// UI-thread request lifecycle; synth workers use a separate cooperative cancellation token.
	enum class RequestState
	{
		Queued,
		AwaitingCompletion, // A worker job or its completion is outstanding.
		AwaitingPublication, // UI-thread completion, texture dependencies, or upload retries.
		Succeeded,
		Failed,
		Cancelled
	};

	struct RequestBase
	{
		lcPieceLoadPriority LoadPriority;
		qint64 QueuedAt;
		RequestState State = RequestState::Queued;

		bool IsFinished() const
		{
			return State == RequestState::Succeeded || State == RequestState::Failed || State == RequestState::Cancelled;
		}
	};

	// Owns one PieceInfo load hold through completion and deferred notification.
	// One worker job builds and converts geometry; textures settle before publication.
	struct PieceRequest : RequestBase
	{
		PieceInfo* Info;
		lcPieceSourceSnapshot Source;
		std::unique_ptr<lcMesh> StagedMesh;
		bool EmptyGeometry = false;
		bool SaveCache = false;
		QString Error;
	};

	// Holds a texture reference through CPU decoding and UI-thread upload/retries.
	struct TextureRequest : RequestBase
	{
		lcTexture* Texture;
		lcFileSource Source;
		int UploadAttempts = 0;
		qint64 RetryAt = 0;
	};

	// The instance cancels before destruction; workers use the copied definition/controls.
	// Cancelled is also checked during expensive generation and conversion.
	struct SynthRequest : RequestBase
	{
		lcPiece* Piece;
		std::shared_ptr<const lcSynthInfo> SynthInfo;
		std::vector<lcPieceControlPoint> ControlPoints;
		std::atomic_bool Cancelled = false;
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
		lcPieceBuildResult PieceResult;
		std::shared_ptr<TextureRequest> RequestedTexture;
		lcTextureBuildResult TextureResult;
		std::shared_ptr<SynthRequest> RequestedSynth;
		std::unique_ptr<lcMesh> SynthMesh;
	};

	// UI-thread delivery retains the load hold after publication, until callbacks finish.
	struct PieceNotification
	{
		std::shared_ptr<PieceRequest> RequestedPiece;
		QString Error;
	};

	// Retains the base identity; cancellation makes delivery safe after instance deletion.
	struct SynthNotification
	{
		std::shared_ptr<SynthRequest> RequestedSynth;
		PieceInfo* Info;
	};

	// Queue and promote requests on the UI thread.
	void QueuePieceLoad(PieceInfo* Info, lcPieceLoadPriority LoadPriority);
	void QueueTextureLoad(lcTexture* Texture, lcPieceLoadPriority LoadPriority);
	void PromotePiecePriority(const std::shared_ptr<PieceRequest>& RequestedPiece, lcPieceLoadPriority LoadPriority);

	// Fill available job slots using queued request priority and age.
	void DispatchJobs();

	// Process a bounded batch of results and upload retries, then start more jobs.
	// Returns whether any result or retry was processed; consumer signals remain deferred.
	bool ProcessCompletions();

	// Handle worker results on the UI thread.
	void ProcessPieceCompletion(Completion& Completed);
	void ProcessSynthCompletion(Completion& Completed);
	void ProcessTextureCompletion(Completion& Completed);

	// Mesh texture dependencies: true means settled; failures are reported in Error.
	bool ResolveAndQueueMeshTextures(lcMesh& Mesh, const lcPieceSourceSnapshot& Source, lcPieceLoadPriority LoadPriority, QString& Error);
	static bool MeshTexturesSettled(const lcMesh& Mesh, QString& Error);

	// Texture upload, bounded retries, and dependency settlement.
	void UploadTextureRequest(const std::shared_ptr<TextureRequest>& RequestedTexture);
	void CompleteTextureRequest(const std::shared_ptr<TextureRequest>& RequestedTexture);
	static QString TextureFailureMessage(const lcTexture* Texture);

	// Publish pieces and generated meshes after their texture dependencies settle.
	void FinishPiecesWithSettledTextures();
	void FinishSynthsWithSettledTextures();
	void CompletePieceRequest(const std::shared_ptr<PieceRequest>& RequestedPiece);
	void CompleteSynthRequest(const std::shared_ptr<SynthRequest>& RequestedSynth);

	// Promote this piece and wait for its request to settle without delivering consumer signals.
	bool WaitForPieceRequest(PieceInfo* Info);

	// Sleep until a worker completion arrives or the next texture upload retry is due.
	void WaitForCompletionOrRetry();

	// Process completion events and deliver deferred consumer signals outside synchronous waits.
	bool event(QEvent* Event) override;
	void DispatchNotifications();

	// Remove a request; outstanding worker completions retain their load hold.
	void DiscardPieceRequest(std::shared_ptr<PieceRequest> Request);

	// Drop queued texture work and upload retries that have no remaining consumers.
	void CancelUnusedTextureRequests();

	// Drain pending work and join workers during shutdown; completion does not imply success.
	void WaitForAllWork();

	// Stop dispatch, cancel pending work, join workers, and release loader-owned references.
	void Shutdown();

	lcPiecesLibrary* const mLibrary;

	// UI-thread scheduling state.
	PieceRequestMap mPieceRequests;
	TextureRequestMap mTextureRequests;
	SynthRequestMap mSynthRequests;

	// UI-thread job bookkeeping; futures may include finished jobs awaiting cleanup.
	std::vector<QFuture<void>> mFutures;
	int mActiveJobs = 0; // Includes completed jobs until the UI consumes their results.

	// Temporarily suspend dispatch while shared library data changes.
	bool mPaused = false;

	// Permanently stop accepting work and discard results during shutdown.
	bool mStopping = false;

	// Worker-to-UI result handoff; the mutex protects the completion queue.
	// Workers wake synchronous UI waits when they enqueue a completion.
	QMutex mCompletionMutex;
	QWaitCondition mCompletionReady;
	std::deque<Completion> mCompletions;

	// UI-thread only; public signals are deferred until outside synchronous waits.
	std::deque<PieceNotification> mPieceNotifications;
	std::deque<SynthNotification> mSynthNotifications;

	// UI-thread errors for failed synth generations.
	std::unordered_map<const lcPiece*, QString> mSynthErrors;

	Q_DECLARE_TR_FUNCTIONS(lcAssetLoader)
};
