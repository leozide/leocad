#pragma once

#include <memory>
#include "lc_library.h"

class PieceInfo;
class lcMesh;

// The worker result will be populated by private mesh builds in the next stage.
// Request identity will let the next stage reject a result from a previous
// library generation after a source reload.
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
	std::unique_ptr<lcMesh> Mesh;
	lcBoundingBox Bounds;
	QStringList TextureDependencies;
	QString Error;
};

class lcAssetLoader : public QObject
{
public:
	lcAssetLoader(lcPiecesLibrary* Library, lcLibraryLoadMutex& LoadMutex);
	~lcAssetLoader() override;

	void LoadPieceInfo(PieceInfo* Info, bool Wait, bool Priority);
	void QueuePiece(PieceInfo* Info, bool Priority);
	void WaitForLoadQueue();
	void CancelAndDrain();
	void PauseQueuedWork();
	void ResumeQueuedWork();

private:
	struct Request
	{
		PieceInfo* Info;
		quint64 Id;
		quint64 Generation;
	};

	struct Completion
	{
		PieceInfo* Info;
		bool Notify;
	};

	// Call with the library load lock held. The queued request keeps an extra
	// reference until a worker finishes or cancellation removes it.
	void QueuePieceLocked(PieceInfo* Info, bool Priority);
	void LoadQueuedPiece();
	void ProcessCompletions();
	bool event(QEvent* Event) override;

	lcPiecesLibrary* const mLibrary;
	lcLibraryLoadMutex& mLoadMutex;
	QMutex mQueueMutex;
	// Completion notifications may reenter a wait on the UI thread.
	lcLibraryLoadMutex mDrainMutex;
	QList<Request> mQueue;
	QList<Request> mPausedQueue;
	QList<Completion> mCompletions;
	QList<QFuture<void>> mFutures;
	quint64 mNextRequestId = 1;
	quint64 mGeneration = 1;
	bool mPaused = false;
	bool mStopping = false;
};
