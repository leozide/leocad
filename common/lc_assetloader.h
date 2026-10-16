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

// Owned by lcPiecesLibrary. UI-thread entry points schedule private worker builds;
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

	// Build this instance's generated mesh from its current control points in the background.
	// Cancel any older request and keep the previous mesh visible while the replacement is built.
	void RequestSynthMesh(lcPiece* Piece);

	// Promote and wait for queued generations. Returns true if every required generated mesh is current.
	// Call RequestSynthMesh() to start a generation; the caller keeps the instances alive through the wait.
	bool WaitForSynthMeshes(const std::vector<lcPiece*>& Pieces);

	// Start or promote decoding as needed, then wait through upload and retries. The caller retains the texture.
	bool WaitForTexture(lcTexture* Texture);

	// Drain all pending work, including unrelated assets. Completion does not imply success.
	void WaitForAllWork();

	// True while piece, texture, or generated-mesh requests or worker completions remain unsettled.
	// Includes paused work and upload retries; deferred notifications alone do not count.
	bool HasPendingWork();

	// Returns this instance's recorded generation error, or an empty string if none is recorded.
	// An empty string does not establish readiness; check the generated mesh separately.
	QString GetSynthMeshError(const lcPiece* Piece) const;

	void RebuildModelPiece(PieceInfo* Info);
	void InvalidatePiece(PieceInfo* Info);
	void CancelSynthMesh(lcPiece* Piece);
	void RebuildSynthMeshes();
	void PauseQueuedWork();
	void ResumeQueuedWork();

private:
	// Request lifecycle; obsolete results and cooperative cancellation are tracked separately.
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
		bool Obsolete = false;  // Its result must not publish, even if the build succeeds.
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

	// Queue and promote requests. Workers read copied sources and private build data.
	// Acquires mRequestMutex internally; callers must not already hold it.
	void QueuePiece(PieceInfo* Info, lcPieceLoadPriority LoadPriority);
	// Caller holds mRequestMutex.
	void PromotePiecePriorityLocked(const std::shared_ptr<PieceRequest>& RequestedPiece, lcPieceLoadPriority LoadPriority);
	void QueueTexture(lcTexture* Texture, lcPieceLoadPriority LoadPriority);
	// Caller holds mRequestMutex.
	void StartWorkersLocked();
	void RunWorker();

	// UI-thread completion handling: colors, texture dependencies, publication.
	bool ProcessCompletions();
	void ProcessPieceCompletion(Completion& Completed);
	void ProcessSynthCompletion(Completion& Completed);
	void ProcessTextureCompletion(Completion& Completed);
	bool ResolveAndQueueMeshTextures(lcMesh& Mesh, const lcPieceSourceSnapshot& Source, lcPieceLoadPriority LoadPriority, QString& Error);
	static bool MeshTexturesSettled(const lcMesh& Mesh, QString& Error);
	static QString TextureFailureMessage(const lcTexture* Texture);
	void UploadTextureRequest(const std::shared_ptr<TextureRequest>& RequestedTexture);
	void FinishTexture(const std::shared_ptr<TextureRequest>& RequestedTexture);
	void FinishPiecesWithSettledTextures();
	void FinishSynthsWithSettledTextures();
	void FinishPiece(const std::shared_ptr<PieceRequest>& RequestedPiece);
	void FinishSynth(const std::shared_ptr<SynthRequest>& RequestedSynth);

	// Notify only from the event handler, after synchronous waits have returned.
	void DispatchNotifications();
	bool event(QEvent* Event) override;
	// Caller holds mRequestMutex; the condition wait releases it and reacquires it on return.
	void WaitForResultsLocked();
	bool WaitForPieceRequest(PieceInfo* Info);

	void CancelUnusedTextureRequests();
	void CancelAndDrain();

	lcPiecesLibrary* const mLibrary;

	// Protects request maps/states, worker counts, and result handoff.
	// Private build data passes between worker and UI phases through completions.
	QMutex mRequestMutex;
	QWaitCondition mResultReady;

	PieceRequestMap mPieceRequests;
	TextureRequestMap mTextureRequests;
	SynthRequestMap mSynthRequests;

	// UI-thread diagnostics and instance tracking; these do not own the pieces.
	std::unordered_map<const lcPiece*, QString> mSynthErrors;
	std::unordered_set<lcPiece*> mSynthPieces;

	std::deque<Completion> mCompletions;

	// UI-thread only; public signals are deferred until outside synchronous waits.
	std::deque<PieceNotification> mPieceNotifications;
	std::deque<SynthNotification> mSynthNotifications;

	std::vector<QFuture<void>> mFutures;
	int mActiveWorkers = 0;
	bool mPaused = false;
	bool mStopping = false;

	Q_DECLARE_TR_FUNCTIONS(lcAssetLoader)
};
