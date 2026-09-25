#include "lc_global.h"
#include "lc_assetloader.h"
#include "lc_library.h"
#include "lc_mesh.h"
#include "pieceinf.h"

constexpr QEvent::Type lcAssetLoaderCompletionEvent = static_cast<QEvent::Type>(QEvent::User + 1);

// TODO(stage 2): Remove this helper when LoadPieceInfo() uses per-request
// terminal-result waits instead of polling PieceInfo::mState.
class lcAssetLoaderSleeper : public QThread
{
public:
	static void msleep(unsigned long Msecs)
	{
		QThread::msleep(Msecs);
	}
};

lcPartBuildResult::~lcPartBuildResult() = default;

lcAssetLoader::lcAssetLoader(lcPiecesLibrary* Library, lcLibraryLoadMutex& LoadMutex)
	: mLibrary(Library), mLoadMutex(LoadMutex)
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	, mDrainMutex(QMutex::Recursive)
#endif
{
}

lcAssetLoader::~lcAssetLoader()
{
	CancelAndDrain();
}

void lcAssetLoader::LoadPieceInfo(PieceInfo* Info, bool Wait, bool Priority)
{
	QMutexLocker LoadLock(&mLoadMutex);

	if (Wait)
	{
		if (Info->AddRef() == 1)
			Info->Load();
		else
		{
			if (Info->mState == lcPieceInfoState::Unloaded)
			{
				Info->Load();
				emit mLibrary->PartLoaded(Info);
			}
			else
			{
				LoadLock.unlock();

				while (Info->mState != lcPieceInfoState::Loaded)
					lcAssetLoaderSleeper::msleep(10);
			}
		}
	}
	else if (Info->AddRef() == 1)
		QueuePieceLocked(Info, Priority);
}

void lcAssetLoader::QueuePiece(PieceInfo* Info, bool Priority)
{
	QMutexLocker LoadLock(&mLoadMutex);

	QueuePieceLocked(Info, Priority);
}

void lcAssetLoader::QueuePieceLocked(PieceInfo* Info, bool Priority)
{
	QMutexLocker QueueLock(&mQueueMutex);

	if (mStopping)
		return;

	Info->AddRef();

	Request Queued{ Info, mNextRequestId++, mGeneration };
	QList<Request>& Pending = mPaused ? mPausedQueue : mQueue;

	if (Priority)
		Pending.prepend(Queued);
	else
		Pending.append(Queued);

	if (!mPaused)
		mFutures.append(QtConcurrent::run([this]() { LoadQueuedPiece(); }));
}

void lcAssetLoader::LoadQueuedPiece()
{
	PieceInfo* Info = nullptr;
	QList<Completion> Completed;

	// Reference counts and queue removal use the same lock order as
	// LoadPieceInfo(). The request hold is released on the UI thread.
	{
		QMutexLocker LoadLock(&mLoadMutex);
		QMutexLocker QueueLock(&mQueueMutex);

		while (!mQueue.isEmpty())
		{
			Request Queued = mQueue.takeFirst();
			PieceInfo* Candidate = Queued.Info;

			if (!mStopping && Queued.Generation == mGeneration && Candidate->mState == lcPieceInfoState::Unloaded && Candidate->GetRefCount() > 1)
			{
				Candidate->mState = lcPieceInfoState::Loading;
				Info = Candidate;
				break;
			}

			Completed.append({ Candidate, false });
		}
	}

	if (Info)
	{
		Info->Load();
		Completed.append({ Info, true });
	}

	if (!Completed.isEmpty())
	{
		mQueueMutex.lock();
		for (const Completion& Result : Completed)
			mCompletions.append(Result);
		mQueueMutex.unlock();

		QCoreApplication::postEvent(this, new QEvent(lcAssetLoaderCompletionEvent));
	}
}

void lcAssetLoader::WaitForLoadQueue()
{
	QMutexLocker DrainLock(&mDrainMutex);

	for (;;)
	{
		QList<QFuture<void>> Futures;

		mQueueMutex.lock();
		Futures.swap(mFutures);
		mQueueMutex.unlock();

		if (Futures.isEmpty())
			break;

		for (QFuture<void>& Future : Futures)
			Future.waitForFinished();
	}

	if (QThread::currentThread() == thread())
		ProcessCompletions();
}

void lcAssetLoader::CancelAndDrain()
{
	Q_ASSERT(QThread::currentThread() == thread());

	QList<Request> Queued;
	QList<Request> Paused;

	{
		QMutexLocker LoadLock(&mLoadMutex);
		QMutexLocker QueueLock(&mQueueMutex);

		mStopping = true;
		mGeneration++;
		Queued.swap(mQueue);
		Paused.swap(mPausedQueue);
	}

	for (const Request& Request : Queued)
		mLibrary->ReleasePieceInfo(Request.Info);

	for (const Request& Request : Paused)
		mLibrary->ReleasePieceInfo(Request.Info);

	WaitForLoadQueue();
}

void lcAssetLoader::PauseQueuedWork()
{
	Q_ASSERT(QThread::currentThread() == thread());

	mQueueMutex.lock();
	mPaused = true;
	mPausedQueue.swap(mQueue);
	mQueueMutex.unlock();

	// Futures for paused requests still run, but find no queued work. Already
	// running requests finish before the library changes shared source data.
	WaitForLoadQueue();
}

void lcAssetLoader::ResumeQueuedWork()
{
	Q_ASSERT(QThread::currentThread() == thread());

	QMutexLocker QueueLock(&mQueueMutex);

	mPaused = false;
	mQueue.swap(mPausedQueue);

	for (int Index = 0; Index < mQueue.size(); Index++)
		mFutures.append(QtConcurrent::run([this]() { LoadQueuedPiece(); }));
}

void lcAssetLoader::ProcessCompletions()
{
	Q_ASSERT(QThread::currentThread() == thread());

	for (;;)
	{
		Completion Result;
		bool MayNotify;

		{
			QMutexLocker QueueLock(&mQueueMutex);

			if (mCompletions.isEmpty())
				return;

			Result = mCompletions.takeFirst();
			MayNotify = !mStopping && Result.Notify;
		}

		bool Notify;

		{
			QMutexLocker LoadLock(&mLoadMutex);
			Notify = MayNotify && Result.Info->GetRefCount() > 1;
			mLibrary->ReleasePieceInfo(Result.Info);
		}

		if (Notify)
		{
			QPointer<lcAssetLoader> Guard(this);
			emit mLibrary->PartLoaded(Result.Info);
			if (!Guard)
				return;
		}
	}
}

bool lcAssetLoader::event(QEvent* Event)
{
	if (Event->type() == lcAssetLoaderCompletionEvent)
	{
		ProcessCompletions();

		return true;
	}

	return QObject::event(Event);
}
