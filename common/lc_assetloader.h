#pragma once

#include "lc_assettypes.h"

class lcPieceData;
class lcPiecesLibrary;
class lcPiece;
struct lcPieceControlPoint;
class lcMesh;
class lcSynthInfo;
class lcTexture;

// Owned by lcPiecesLibrary. Instance methods run on the UI thread and schedule private worker builds;
// UI-thread completion processing resolves colors, uploads textures, and publishes meshes.
// See docs/asset-loading.md for readiness, ownership, and caller policies.
class lcAssetLoader : public QObject
{
public:
	lcAssetLoader(lcPiecesLibrary* Library);
	~lcAssetLoader() override;

	// Queue or promote a piece the library has marked Loading. Adds no consumer reference.
	// Priority controls scheduling only; the library handles blocking waits and asset readiness.
	void RequestPieceLoad(lcPieceData* PieceData, lcLoadPriority LoadPriority);

	// Promote and wait for requests active at entry; false means a request failed or was cancelled.
	// The library checks final asset readiness, including identities with no active request.
	// Adds no consumer references and does not traverse container children or generated meshes.
	bool WaitForPieces(const std::vector<lcPieceData*>& Pieces);

	// Called when the last consumer releases the piece. Cancels queued work and reports it to the library;
	// running work finishes privately and is discarded if the piece is still unused.
	void CancelPieceLoad(lcPieceData* PieceData);

	// Discard pending load results and notifications after the library invalidates readiness.
	// Asset meshes, readiness, and errors belong to the library.
	void InvalidatePieceLoad(lcPieceData* PieceData);

	// Build this instance's generated mesh from definition/control-point snapshots supplied by the library.
	// Cancel any older request and keep the previous mesh visible while the replacement is built.
	void RequestSynthMesh(lcPiece* Piece, lcPieceData* PieceData, std::shared_ptr<const lcSynthInfo> SynthInfo, std::vector<lcPieceControlPoint> ControlPoints);

	// Promote and wait for generations active at entry; false means a generation failed or was cancelled.
	// The library checks final generated-mesh readiness. The caller keeps the instances alive through the wait.
	bool WaitForSynthMeshes(const std::vector<lcPiece*>& Pieces);

	// Cancel this instance's generation and suppress its deferred notifications.
	// Running jobs stop cooperatively; any published mesh is kept.
	void CancelSynthMesh(lcPiece* Piece);

	// Start or promote decoding as needed, then wait through upload and retries. The caller retains the texture.
	bool WaitForTexture(lcTexture* Texture, lcFileSource Source);

	// Queue or promote decoding without waiting or acquiring a consumer reference.
	void RequestTextureLoad(lcTexture* Texture, lcFileSource Source, lcLoadPriority LoadPriority);

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
		lcLoadPriority LoadPriority;
		qint64 QueuedAt;
		RequestState State = RequestState::Queued;

		bool IsFinished() const
		{
			return State == RequestState::Succeeded || State == RequestState::Failed || State == RequestState::Cancelled;
		}
	};

	// Owns one definition reference through completion and deferred notification.
	// Reset on the UI thread before worker-shared request storage can be destroyed.
	// One worker job builds and converts geometry; textures settle before publication.
	struct PieceRequest : RequestBase
	{
		lcPieceDataRef PieceData;
		lcPieceSourceSnapshot Source;
		std::unique_ptr<lcMesh> StagedMesh;
		bool EmptyGeometry = false;
		bool SaveCache = false;
		QString Error;
	};

	// Owns a texture reference through CPU decoding and UI-thread upload/retries.
	// Reset on the UI thread before worker-shared request storage can be destroyed.
	struct TextureRequest : RequestBase
	{
		lcTextureRef Texture;
		lcFileSource Source;
		int UploadAttempts = 0;
		qint64 RetryAt = 0;
	};

	// The instance cancels before destruction; workers use the copied definition/controls.
	// Cancelled is also checked during expensive generation and conversion.
	struct SynthRequest : RequestBase
	{
		lcPiece* Piece;
		lcPieceData* PieceData; // Borrowed from the instance until cancellation or publication.
		std::shared_ptr<const lcSynthInfo> SynthInfo;
		std::vector<lcPieceControlPoint> ControlPoints;
		std::atomic_bool Cancelled = false;
		std::unique_ptr<lcMesh> StagedMesh;
		QString Error;
	};

	using PieceRequestMap = std::map<lcPieceData*, std::shared_ptr<PieceRequest>>;
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
		lcPieceDataRef PieceData;
	};

	// Promote requests on the UI thread.
	void PromotePiecePriority(const std::shared_ptr<PieceRequest>& RequestedPiece, lcLoadPriority LoadPriority);

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
	bool ResolveAndQueueMeshTextures(lcMesh& Mesh, const lcPieceSourceSnapshot& Source, lcLoadPriority LoadPriority, QString& Error);
	static bool MeshTexturesSettled(const lcMesh& Mesh, QString& Error);

	// Texture upload, bounded retries, and dependency settlement.
	void UploadTextureRequest(const std::shared_ptr<TextureRequest>& RequestedTexture);
	void CompleteTextureRequest(const std::shared_ptr<TextureRequest>& RequestedTexture);

	// Publish pieces and generated meshes after their texture dependencies settle.
	void FinishPiecesWithSettledTextures();
	void FinishSynthsWithSettledTextures();
	void CompletePieceRequest(const std::shared_ptr<PieceRequest>& RequestedPiece);
	void CompleteSynthRequest(const std::shared_ptr<SynthRequest>& RequestedSynth);

	// Wait for a captured request to settle without delivering consumer signals.
	bool WaitForPieceRequest(const std::shared_ptr<PieceRequest>& RequestedPiece);

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

	Q_DECLARE_TR_FUNCTIONS(lcAssetLoader)
};
