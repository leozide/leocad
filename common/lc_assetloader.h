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

struct lcPartSourceSnapshot
{
	QString FileName;
	QString LibraryDirectory;
	QString CachePath;
	std::vector<QString> TextureSearchDirectories;
	QStringList InlineMeshLines;
	bool InlineModel = false;
	bool SkipCache = false;
	lcZipFileType ZipFileType = lcZipFileType::Count;
	int ZipFileIndex = -1;
	lcStudStyle StudStyle = lcStudStyle::Plain;
	bool StudCylinderColorEnabled = false;
};

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
	QString Name;
	QString FilePath;
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
};

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

	struct Request
	{
		PieceInfo* Info;
		quint64 Id;
		quint64 Generation;
		lcPartSourceSnapshot Source;
		Priority LoadPriority;
		qint64 EnqueuedAt;
		bool Running = false;
		bool Obsolete = false;
		bool Terminal = false;
		bool Succeeded = false;
		bool ConvertingMesh = false;
		std::unique_ptr<lcLibraryMeshData> MeshData;
		std::vector<bool> ColorTranslucency;
		int DefaultColorIndex = 0;
		std::unique_ptr<lcMesh> StagedMesh;
		bool EmptyGeometry = false;
		bool SaveCache = false;
		QString Error;
	};

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

	struct Notification
	{
		std::shared_ptr<Request> RequestedPart;
		bool Loaded;
		QString Error;
	};

	struct SynthNotification
	{
		std::shared_ptr<SynthRequest> RequestedSynth;
		PieceInfo* Info;
	};

	void QueuePieceLocked(PieceInfo* Info, Priority LoadPriority);
	void PromoteRequestPriorityLocked(const std::shared_ptr<Request>& RequestedPart, Priority LoadPriority);
	void QueueTexture(lcTexture* Texture, Priority LoadPriority);
	static QString TextureFailureMessage(const lcTexture* Texture);
	void CancelUnusedTextureRequests();
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
	void WaitForResultsLocked();
	bool WaitForRequest(PieceInfo* Info);
	bool event(QEvent* Event) override;

	lcPiecesLibrary* const mLibrary;
	QMutex mQueueMutex;
	QWaitCondition mResultReady;
	QMutex mDrainMutex;
	PartRequestMap mRequests;
	TextureRequestMap mTextureRequests;
	SynthRequestMap mSynthRequests;
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
