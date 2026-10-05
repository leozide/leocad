#include "lc_global.h"
#include "lc_assetloader.h"
#include "lc_colors.h"
#include "lc_file.h"
#include "lc_library.h"
#include "lc_mesh.h"
#include "lc_model.h"
#include "lc_texture.h"
#include "lc_synth.h"
#include "piece.h"
#include "pieceinf.h"

constexpr QEvent::Type lcAssetLoaderCompletionEvent = static_cast<QEvent::Type>(QEvent::User + 1);

lcPartBuildResult::~lcPartBuildResult() = default;
lcTextureBuildResult::~lcTextureBuildResult() = default;

lcAssetLoader::lcAssetLoader(lcPiecesLibrary* Library)
	: mLibrary(Library)
{
}

lcAssetLoader::~lcAssetLoader()
{
	CancelAndDrain();
}

bool lcAssetLoader::LoadPieceInfo(PieceInfo* Info, lcPieceLoadFlags Flags)
{
	Q_ASSERT(QThread::currentThread() == thread());

	mLibrary->AddPieceReference(Info);

	if (Info->IsProject() || (Info->IsModel() && !Info->GetModel()->HasDirectGeometry()))
		Info->mState = lcPieceInfoState::Loaded;
	else if (Info->mState == lcPieceInfoState::Unloaded)
		QueuePieceLocked(Info, Flags.testFlag(lcPieceLoadFlag::Visible) ? Priority::Visible : Priority::Background);
	else if (Flags.testFlag(lcPieceLoadFlag::Visible))
	{
		mQueueMutex.lock();
		const auto It = mRequests.find(Info);
		if (It != mRequests.end())
			PromoteRequestPriorityLocked(It->second, Priority::Visible);
		mQueueMutex.unlock();
	}

	return !Flags.testFlag(lcPieceLoadFlag::Wait) || WaitForRequest(Info);
}

void lcAssetLoader::QueuePiece(PieceInfo* Info, bool PriorityHint)
{
	Q_ASSERT(QThread::currentThread() == thread());

	QueuePieceLocked(Info, PriorityHint ? Priority::Visible : Priority::Background);
}

void lcAssetLoader::CancelSynthMesh(lcPiece* Piece)
{
	Q_ASSERT(QThread::currentThread() == thread());

	mSynthPieces.erase(Piece);
	mFailedSynthErrors.erase(Piece);

	for (SynthNotification& Pending : mSynthNotifications)
		if (Pending.RequestedSynth->Piece == Piece)
			Pending.RequestedSynth->Cancelled.store(true, std::memory_order_relaxed);

	std::shared_ptr<SynthRequest> Removed;

	{
		QMutexLocker QueueLock(&mQueueMutex);
		const SynthRequestMap::iterator It = mSynthRequests.find(Piece);

		if (It == mSynthRequests.end())
			return;

		Removed = It->second;
		Removed->Cancelled.store(true, std::memory_order_relaxed);
		Removed->Terminal = true;
		mSynthRequests.erase(It);

		const auto EraseRequest = [&Removed](auto& Queue)
		{
			Queue.erase(std::remove(Queue.begin(), Queue.end(), Removed), Queue.end());
		};

		EraseRequest(mSynthQueue);
		EraseRequest(mPausedSynthQueue);
		mResultReady.wakeAll();
	}

	if (!Removed->Running)
	{
		Removed->StagedMesh.reset();
		CancelUnusedTextureRequests();
	}
}

void lcAssetLoader::QueueSynthMesh(lcPiece* Piece)
{
	Q_ASSERT(QThread::currentThread() == thread());

	CancelSynthMesh(Piece);

	PieceInfo* Info = Piece->mPieceInfo;
	const std::shared_ptr<const lcSynthInfo> SynthInfo = Info ? Info->GetSynthInfoShared() : nullptr;

	if (!SynthInfo)
		return;

	std::shared_ptr<SynthRequest> RequestedSynth = std::make_shared<SynthRequest>();
	RequestedSynth->Piece = Piece;
	RequestedSynth->SynthInfo = SynthInfo;
	RequestedSynth->ControlPoints = Piece->GetControlPoints();
	RequestedSynth->LoadPriority = Priority::Visible;
	RequestedSynth->EnqueuedAt = QDateTime::currentMSecsSinceEpoch();
	RequestedSynth->Generation = mGeneration;

	QMutexLocker QueueLock(&mQueueMutex);

	if (mStopping)
		return;

	mSynthPieces.insert(Piece);
	mSynthRequests.emplace(Piece, RequestedSynth);

	if (mPaused)
		mPausedSynthQueue.push_back(RequestedSynth);
	else
	{
		mSynthQueue.push_back(RequestedSynth);
		StartWorkersLocked();
	}

	QueueLock.unlock();
	emit mLibrary->AssetRequestsChanged();
}

bool lcAssetLoader::EnsureSynthMeshesReady(const std::vector<lcPiece*>& Pieces)
{
	Q_ASSERT(QThread::currentThread() == thread());

	std::vector<std::shared_ptr<SynthRequest>> Required;

	{
		QMutexLocker QueueLock(&mQueueMutex);

		for (lcPiece* Piece : Pieces)
		{
			const SynthRequestMap::iterator It = mSynthRequests.find(Piece);

			if (It == mSynthRequests.end())
				continue;

			const std::shared_ptr<SynthRequest>& RequestedSynth = It->second;
			RequestedSynth->LoadPriority = Priority::Blocking;
			Required.push_back(RequestedSynth);

			if (RequestedSynth->StagedMesh)
				for (const lcMeshLod& Lod : RequestedSynth->StagedMesh->mLods)
					for (int SectionIdx = 0; SectionIdx < Lod.NumSections; SectionIdx++)
					{
						const TextureRequestMap::iterator TextureIt = mTextureRequests.find(Lod.Sections[SectionIdx].Texture);

						if (TextureIt != mTextureRequests.end())
							TextureIt->second->LoadPriority = Priority::Blocking;
					}
		}

		StartWorkersLocked();
	}

	QOpenGLContext* const PreviousContext = QOpenGLContext::currentContext();
	QSurface* const PreviousSurface = PreviousContext ? PreviousContext->surface() : nullptr;

	for (const std::shared_ptr<SynthRequest>& RequestedSynth : Required)
	{
		while (!RequestedSynth->Terminal)
		{
			ProcessCompletions();

			if (RequestedSynth->Terminal)
				break;

			QMutexLocker QueueLock(&mQueueMutex);

			if (mStopping || RequestedSynth->Terminal)
				break;

			WaitForResultsLocked();
		}
	}

	if (PreviousContext && PreviousSurface && (QOpenGLContext::currentContext() != PreviousContext || PreviousContext->surface() != PreviousSurface))
		PreviousContext->makeCurrent(PreviousSurface);

	for (lcPiece* Piece : Pieces)
		if (Piece->mPieceInfo && Piece->mPieceInfo->GetSynthInfo() && !Piece->HasGeneratedMesh())
			return false;

	return true;
}

QString lcAssetLoader::GetSynthMeshError(const lcPiece* Piece) const
{
	const std::unordered_map<const lcPiece*, QString>::const_iterator It = mFailedSynthErrors.find(Piece);

	return It != mFailedSynthErrors.end() ? It->second : QString();
}

void lcAssetLoader::ReloadSynthMeshes()
{
	Q_ASSERT(QThread::currentThread() == thread());

	const std::vector<lcPiece*> Pieces(mSynthPieces.begin(), mSynthPieces.end());

	for (lcPiece* Piece : Pieces)
		Piece->RefreshGeneratedMesh();
}

void lcAssetLoader::PromoteRequestPriorityLocked(const std::shared_ptr<Request>& RequestedPart, Priority LoadPriority)
{
	RequestedPart->LoadPriority = std::max(RequestedPart->LoadPriority, LoadPriority);

	if (!RequestedPart->StagedMesh)
		return;

	for (const lcMeshLod& Lod : RequestedPart->StagedMesh->mLods)
	{
		for (int SectionIdx = 0; SectionIdx < Lod.NumSections; SectionIdx++)
		{
			const auto It = mTextureRequests.find(Lod.Sections[SectionIdx].Texture);

			if (It != mTextureRequests.end())
				It->second->LoadPriority = std::max(It->second->LoadPriority, RequestedPart->LoadPriority);
		}
	}
}

void lcAssetLoader::OnConsumerReleased(PieceInfo* Info)
{
	Q_ASSERT(QThread::currentThread() == thread());

	if (mLibrary->HasPieceConsumers(Info))
		return;

	std::shared_ptr<Request> Removed;
	{
		QMutexLocker QueueLock(&mQueueMutex);

		const auto It = mRequests.find(Info);

		if (It == mRequests.end())
			return;

		const std::shared_ptr<Request>& RequestedPart = It->second;

		if (RequestedPart->Running)
			return;

		RequestedPart->Obsolete = true;

		Removed = RequestedPart;
		Removed->Terminal = true;
		Removed->Succeeded = false;

		Info->mState = lcPieceInfoState::Cancelled;

		mRequests.erase(It);

		auto EraseRequest = [&Removed](auto& Queue)
		{
			Queue.erase(std::remove(Queue.begin(), Queue.end(), Removed), Queue.end());
		};

		EraseRequest(mQueue);
		EraseRequest(mPausedQueue);

		mResultReady.wakeAll();
	}

	Removed->StagedMesh.reset();

	CancelUnusedTextureRequests();

	mLibrary->ReleasePieceLoadHold(Info);
}

void lcAssetLoader::QueuePieceLocked(PieceInfo* Info, Priority LoadPriority)
{
	QMutexLocker QueueLock(&mQueueMutex);

	if (mStopping)
		return;

	const auto Existing = mRequests.find(Info);

	if (Existing != mRequests.end())
	{
		PromoteRequestPriorityLocked(Existing->second, LoadPriority);

		return;
	}

	mLibrary->AddPieceReference(Info); // The request owns a separate in-flight hold.

	Info->mState = lcPieceInfoState::Queued;

	if (!Info->IsModel() && !Info->IsProject())
	{
		const lcBoundingBox& Box = mLibrary->GetLoadingMesh()->mBoundingBox;
		Info->SetBoundingBox(Box.Min, Box.Max);
	}
	else if (Info->IsModel() && Info->GetModel()->HasDirectGeometry())
	{
		const lcBoundingBox& Box = mLibrary->GetLoadingMesh()->mBoundingBox;
		const lcBoundingBox& ModelBox = Info->GetBoundingBox();
		Info->SetBoundingBox(lcMin(ModelBox.Min, Box.Min), lcMax(ModelBox.Max, Box.Max));
	}

	mLibrary->ClearPieceLoadError(Info);

	std::shared_ptr<Request> RequestedPart = std::make_shared<Request>();

	RequestedPart->Info = Info;
	RequestedPart->Id = mNextRequestId++;
	RequestedPart->Generation = mGeneration;
	RequestedPart->Source = mLibrary->SnapshotPieceSource(Info);
	RequestedPart->LoadPriority = LoadPriority;
	RequestedPart->EnqueuedAt = QDateTime::currentMSecsSinceEpoch();

	mRequests.emplace(Info, RequestedPart);

	if (mPaused)
		mPausedQueue.push_back(RequestedPart);
	else
	{
		mQueue.push_back(RequestedPart);

		StartWorkersLocked();
	}

	emit mLibrary->AssetRequestsChanged();
}

void lcAssetLoader::StartWorkersLocked()
{
	mFutures.erase(std::remove_if(mFutures.begin(), mFutures.end(), [](const QFuture<void>& Future)
	{
		return Future.isFinished();
	}), mFutures.end());

	const int MaxWorkers = qMax(1, qMin(4, QThread::idealThreadCount()));
	const int Queued = static_cast<int>(mQueue.size() + mTextureQueue.size() + mSynthQueue.size());
	const int Additional = mPaused || mStopping ? 0 : qMin(MaxWorkers - mActiveWorkers, Queued);

	for (int Worker = 0; Worker < Additional; Worker++)
	{
		mActiveWorkers++;
		mFutures.emplace_back(QtConcurrent::run([this]() { LoadQueuedPieces(); }));
	}
}

void lcAssetLoader::LoadQueuedPieces()
{
	for (;;)
	{
		std::shared_ptr<Request> RequestedPart;
		std::shared_ptr<TextureRequest> RequestedTexture;
		std::shared_ptr<SynthRequest> RequestedSynth;

		{
			QMutexLocker QueueLock(&mQueueMutex);

			if (mStopping || mPaused || (mQueue.empty() && mTextureQueue.empty() && mSynthQueue.empty()))
			{
				mActiveWorkers--;
				mResultReady.wakeAll();

				return;
			}

			const qint64 Now = QDateTime::currentMSecsSinceEpoch();

			const auto Score = [Now](const auto& Item)
			{
				// Each ten seconds of waiting promotes one priority level.
				return qMin<qint64>(static_cast<qint64>(Priority::Blocking),
					static_cast<qint64>(Item->LoadPriority) + qMax<qint64>(0, Now - Item->EnqueuedAt) / 10000);
			};

			const auto Less = [&Score](const auto& Left, const auto& Right)
			{
				return Score(Left) < Score(Right) || (Score(Left) == Score(Right) && Left->EnqueuedAt > Right->EnqueuedAt);
			};

			auto PartBest = std::max_element(mQueue.begin(), mQueue.end(), Less);
			auto TextureBest = std::max_element(mTextureQueue.begin(), mTextureQueue.end(), Less);
			auto SynthBest = std::max_element(mSynthQueue.begin(), mSynthQueue.end(), Less);

			if (TextureBest != mTextureQueue.end() && (PartBest == mQueue.end() || !Less(*TextureBest, *PartBest)) &&
				(SynthBest == mSynthQueue.end() || !Less(*TextureBest, *SynthBest)))
			{
				RequestedTexture = *TextureBest;
				mTextureQueue.erase(TextureBest);
				RequestedTexture->Running = true;
				mStartedTextures.push_back(RequestedTexture);
			}
			else if (SynthBest != mSynthQueue.end() && (PartBest == mQueue.end() || !Less(*SynthBest, *PartBest)))
			{
				RequestedSynth = *SynthBest;
				mSynthQueue.erase(SynthBest);
				RequestedSynth->Running = true;
			}
			else
			{
				RequestedPart = *PartBest;
				mQueue.erase(PartBest);
				RequestedPart->Running = true;
				mStarted.push_back(RequestedPart);
			}
		}

		if (RequestedPart || RequestedTexture || RequestedSynth)
			QCoreApplication::postEvent(this, new QEvent(lcAssetLoaderCompletionEvent));

		Completion Completed;

		if (RequestedPart)
		{
			Completed.RequestedPart = RequestedPart;

			if (RequestedPart->ConvertingMesh)
			{
				Completed.Result.Mesh.reset(RequestedPart->MeshData->CreateMeshResolved(RequestedPart->ColorTranslucency, RequestedPart->DefaultColorIndex, nullptr));
				RequestedPart->MeshData.reset();

				if (!Completed.Result.Mesh)
					Completed.Result.Error = tr("Could not prepare piece %1 for rendering.").arg(RequestedPart->Source.FileName);
			}
			else
			{
				Completed.Result = mLibrary->BuildPieceData(RequestedPart->Source);

				if (Completed.Result.CacheData)
				{
					Completed.Result.Mesh.reset(new lcMesh);

					if (!Completed.Result.Mesh->FileLoad(*Completed.Result.CacheData, Completed.Result.ColorCodes))
						Completed.Result.Mesh.reset();
				}
			}

			Completed.Result.RequestId = RequestedPart->Id;
			Completed.Result.Generation = RequestedPart->Generation;
		}
		else if (RequestedTexture)
		{
			Completed.RequestedTexture = RequestedTexture;
			Completed.TextureResult = mLibrary->BuildTextureData(RequestedTexture->Source);
		}
		else
		{
			Completed.RequestedSynth = RequestedSynth;

			if (RequestedSynth->Cancelled.load(std::memory_order_relaxed))
				RequestedSynth->MeshData.reset();
			else if (RequestedSynth->ConvertingMesh)
			{
				Completed.SynthMesh.reset(RequestedSynth->MeshData->CreateMeshResolved(RequestedSynth->ColorTranslucency, RequestedSynth->DefaultColorIndex, &RequestedSynth->Cancelled));
				RequestedSynth->MeshData.reset();
			}
			else
				Completed.SynthMeshData = RequestedSynth->SynthInfo->BuildMeshData(RequestedSynth->ControlPoints, RequestedSynth->Cancelled);
		}

		{
			QMutexLocker QueueLock(&mQueueMutex);
			mCompletions.push_back(std::move(Completed));
			mResultReady.wakeAll();
		}

		QCoreApplication::postEvent(this, new QEvent(lcAssetLoaderCompletionEvent));
	}
}

void lcAssetLoader::QueueTexture(lcTexture* Texture, Priority LoadPriority)
{
	QMutexLocker QueueLock(&mQueueMutex);

	if (mStopping || Texture->IsReady() || Texture->mState == lcTextureState::Failed)
		return;

	const auto Existing = mTextureRequests.find(Texture);

	if (Existing != mTextureRequests.end())
	{
		if (Existing->second->LoadPriority < LoadPriority)
			Existing->second->LoadPriority = LoadPriority;
		return;
	}

	Texture->AddRef();
	Texture->mState = lcTextureState::Queued;

	std::shared_ptr<TextureRequest> RequestedTexture = std::make_shared<TextureRequest>();

	RequestedTexture->Texture = Texture;
	RequestedTexture->Generation = mGeneration;
	RequestedTexture->Source = mLibrary->SnapshotTextureSource(Texture);
	RequestedTexture->LoadPriority = LoadPriority;
	RequestedTexture->EnqueuedAt = QDateTime::currentMSecsSinceEpoch();

	mTextureRequests.emplace(Texture, RequestedTexture);

	if (mPaused)
		mPausedTextureQueue.push_back(RequestedTexture);
	else
	{
		mTextureQueue.push_back(RequestedTexture);
		StartWorkersLocked();
	}
}

QString lcAssetLoader::TextureFailureMessage(const lcTexture* Texture)
{
	switch (Texture->mLoadFailure)
	{
	case lcTextureLoadError::DecodeFailed:
		return tr("Could not load texture %1.").arg(QString::fromLatin1(Texture->mName));

	case lcTextureLoadError::ContextActivationFailed:
		return tr("Could not make the shared GL context current for texture upload.");

	case lcTextureLoadError::ContextUnavailable:
		return tr("No shared GL context is available for texture upload.");

	case lcTextureLoadError::UploadFailed:
		return tr("Texture upload failed.");

	case lcTextureLoadError::None:
		return tr("Texture %1 failed to load.").arg(QString::fromLatin1(Texture->mName));
	}

	return tr("Texture %1 failed to load.").arg(QString::fromLatin1(Texture->mName));
}

void lcAssetLoader::CancelUnusedTextureRequests()
{
	std::vector<std::shared_ptr<TextureRequest>> Cancelled;

	{
		QMutexLocker QueueLock(&mQueueMutex);

		for (auto It = mTextureRequests.begin(); It != mTextureRequests.end();)
		{
			const std::shared_ptr<TextureRequest>& RequestedTexture = It->second;

			// A queued request owns one reference; all others belong to consumers.
			if (RequestedTexture->Running || RequestedTexture->Texture->GetRefCount() > 1)
			{
				++It;
				continue;
			}

			auto EraseRequest = [&RequestedTexture](auto& Queue)
			{
				Queue.erase(std::remove(Queue.begin(), Queue.end(), RequestedTexture), Queue.end());
			};

			EraseRequest(mTextureQueue);
			EraseRequest(mPausedTextureQueue);

			Cancelled.push_back(RequestedTexture);

			It = mTextureRequests.erase(It);
		}

		if (!Cancelled.empty())
			mResultReady.wakeAll();
	}

	for (const std::shared_ptr<TextureRequest>& RequestedTexture : Cancelled)
	{
		lcTexture* Texture = RequestedTexture->Texture;

		Texture->DiscardDecodedImage();
		Texture->mState = lcTextureState::Unrequested;
		Texture->mLoadFailure = lcTextureLoadError::None;

		mLibrary->ReleaseTexture(Texture);
	}
}

void lcAssetLoader::FinishPart(const std::shared_ptr<Request>& RequestedPart)
{
	PieceInfo* Info = RequestedPart->Info;
	const bool Loaded = RequestedPart->Error.isEmpty() && (RequestedPart->StagedMesh || RequestedPart->EmptyGeometry);
	QString LoadError;

	if (Loaded)
	{
		if (RequestedPart->SaveCache && RequestedPart->StagedMesh)
			mLibrary->SaveBuiltPieceCache(RequestedPart->Source, *RequestedPart->StagedMesh);

		if (RequestedPart->StagedMesh)
			Info->SetMesh(RequestedPart->StagedMesh.release());

		Info->mState = lcPieceInfoState::Loaded;
		mLibrary->ClearPieceLoadError(Info);
	}
	else
	{
		LoadError = RequestedPart->Error.isEmpty() ? tr("Could not load part %1.").arg(RequestedPart->Source.FileName) : RequestedPart->Error;

		Info->SetFailed(LoadError);
	}

	mQueueMutex.lock();
	const auto It = mRequests.find(Info);
	if (It != mRequests.end() && It->second == RequestedPart)
		mRequests.erase(It);
	RequestedPart->Terminal = true;
	RequestedPart->Succeeded = Loaded;
	mResultReady.wakeAll();
	mQueueMutex.unlock();

	mLibrary->ScheduleBufferRepack();
	mNotifications.push_back({ RequestedPart, Loaded, std::move(LoadError) });

	QCoreApplication::postEvent(this, new QEvent(lcAssetLoaderCompletionEvent));
}

void lcAssetLoader::CheckWaitingParts()
{
	std::vector<std::shared_ptr<Request>> Waiting;

	{
		QMutexLocker QueueLock(&mQueueMutex);

		for (const PartRequestMap::value_type& Entry : mRequests)
			if (Entry.second->StagedMesh)
				Waiting.push_back(Entry.second);
	}

	for (const std::shared_ptr<Request>& RequestedPart : Waiting)
	{
		bool Ready = true;

		for (const lcMeshLod& Lod : RequestedPart->StagedMesh->mLods)
		{
			for (int SectionIdx = 0; SectionIdx < Lod.NumSections; SectionIdx++)
			{
				lcTexture* Texture = Lod.Sections[SectionIdx].Texture;

				if (!Texture)
					continue;

				if (Texture->mState == lcTextureState::Failed)
					RequestedPart->Error = TextureFailureMessage(Texture);
				else if (!Texture->IsReady())
					Ready = false;
			}
		}

		if (!RequestedPart->Error.isEmpty() || Ready)
			FinishPart(RequestedPart);
	}
}

void lcAssetLoader::FinishSynth(const std::shared_ptr<SynthRequest>& RequestedSynth)
{
	lcPiece* Piece = RequestedSynth->Piece;
	PieceInfo* Info = Piece->mPieceInfo;
	const bool Loaded = RequestedSynth->Error.isEmpty() && RequestedSynth->StagedMesh;

	Piece->SetGeneratedMesh(Loaded ? RequestedSynth->StagedMesh.release() : nullptr);

	if (Loaded)
		mFailedSynthErrors.erase(Piece);
	else
		mFailedSynthErrors.insert_or_assign(Piece, RequestedSynth->Error);

	if (!Loaded)
	{
		RequestedSynth->StagedMesh.reset();
		CancelUnusedTextureRequests();
	}

	mQueueMutex.lock();
	const SynthRequestMap::iterator It = mSynthRequests.find(Piece);

	if (It != mSynthRequests.end() && It->second == RequestedSynth)
		mSynthRequests.erase(It);

	RequestedSynth->Terminal = true;
	mResultReady.wakeAll();
	mQueueMutex.unlock();

	mLibrary->AddPieceReference(Info);
	mSynthNotifications.push_back({ RequestedSynth, Info });

	QCoreApplication::postEvent(this, new QEvent(lcAssetLoaderCompletionEvent));
}

void lcAssetLoader::CheckWaitingSynths()
{
	std::vector<std::shared_ptr<SynthRequest>> Waiting;

	{
		QMutexLocker QueueLock(&mQueueMutex);

		for (const SynthRequestMap::value_type& Entry : mSynthRequests)
			if (Entry.second->StagedMesh)
				Waiting.push_back(Entry.second);
	}

	for (const std::shared_ptr<SynthRequest>& RequestedSynth : Waiting)
	{
		mQueueMutex.lock();
		const SynthRequestMap::iterator It = mSynthRequests.find(RequestedSynth->Piece);
		const bool Current = It != mSynthRequests.end() && It->second == RequestedSynth;
		mQueueMutex.unlock();

		if (!Current)
			continue;

		bool Ready = true;

		for (const lcMeshLod& Lod : RequestedSynth->StagedMesh->mLods)
			for (int SectionIdx = 0; SectionIdx < Lod.NumSections; SectionIdx++)
			{
				lcTexture* Texture = Lod.Sections[SectionIdx].Texture;

				if (!Texture)
					continue;

				if (Texture->mState == lcTextureState::Failed)
					RequestedSynth->Error = TextureFailureMessage(Texture);
				else if (!Texture->IsReady())
					Ready = false;
			}

		if (!RequestedSynth->Error.isEmpty() || Ready)
			FinishSynth(RequestedSynth);
	}
}

void lcAssetLoader::FinishTextureRequest(const std::shared_ptr<TextureRequest>& RequestedTexture)
{
	mQueueMutex.lock();
	const auto It = mTextureRequests.find(RequestedTexture->Texture);
	if (It != mTextureRequests.end() && It->second == RequestedTexture)
		mTextureRequests.erase(It);
	mResultReady.wakeAll();
	mQueueMutex.unlock();

	mLibrary->ReleaseTexture(RequestedTexture->Texture);

	CheckWaitingParts();
	CheckWaitingSynths();
}

void lcAssetLoader::UploadTextureRequest(const std::shared_ptr<TextureRequest>& RequestedTexture)
{
	lcTexture* Texture = RequestedTexture->Texture;

	RequestedTexture->UploadAttempts++;
	Texture->mState = lcTextureState::Decoded;

	QOpenGLContext* PreviousContext = QOpenGLContext::currentContext();
	QSurface* PreviousSurface = PreviousContext ? PreviousContext->surface() : nullptr;

	if (lcContext* Context = lcContext::GetGlobalOffscreenContext())
	{
		Context->MakeCurrent();

		if (QOpenGLContext::currentContext() == Context->GetGLContext())
			Texture->Upload(Context);
		else
		{
			Texture->mState = lcTextureState::Failed;
			Texture->mLoadFailure = lcTextureLoadError::ContextActivationFailed;
		}
	}
	else
	{
		Texture->mState = lcTextureState::Failed;
		Texture->mLoadFailure = lcTextureLoadError::ContextUnavailable;
	}

	if (PreviousContext && PreviousSurface)
		PreviousContext->makeCurrent(PreviousSurface);
	else if (QOpenGLContext::currentContext())
		QOpenGLContext::currentContext()->doneCurrent();

	constexpr int MaxUploadAttempts = 3;

	if (!Texture->IsReady() && RequestedTexture->UploadAttempts < MaxUploadAttempts)
	{
		// Only the loader may retry. Keep dependent parts awaiting this texture.
		Texture->mState = lcTextureState::RetryPending;

		const int Delay = RequestedTexture->UploadAttempts == 1 ? 50 : 200;

		mQueueMutex.lock();
		RequestedTexture->RetryAt = QDateTime::currentMSecsSinceEpoch() + Delay;
		mResultReady.wakeAll();
		mQueueMutex.unlock();

		QTimer::singleShot(Delay, this, [this]()
		{
			QCoreApplication::postEvent(this, new QEvent(lcAssetLoaderCompletionEvent));
		});

		return;
	}

	if (!Texture->IsReady())
	{
		Texture->mState = lcTextureState::Failed;

		if (Texture->mLoadFailure == lcTextureLoadError::None)
			Texture->mLoadFailure = lcTextureLoadError::UploadFailed;

		Texture->DiscardDecodedImage();
	}

	FinishTextureRequest(RequestedTexture);
}

void lcAssetLoader::WaitForResultsLocked()
{
	if (!mCompletions.empty())
		return;

	qint64 EarliestRetry = -1;

	for (const TextureRequestMap::value_type& Entry : mTextureRequests)
	{
		const qint64 RetryAt = Entry.second->RetryAt;

		if (RetryAt && (EarliestRetry == -1 || RetryAt < EarliestRetry))
			EarliestRetry = RetryAt;
	}

	if (EarliestRetry == -1)
		mResultReady.wait(&mQueueMutex);
	else
	{
		const qint64 Delay = EarliestRetry - QDateTime::currentMSecsSinceEpoch();

		if (Delay > 0)
			mResultReady.wait(&mQueueMutex, static_cast<unsigned long>(Delay));
	}
}

bool lcAssetLoader::ProcessCompletions()
{
	Q_ASSERT(QThread::currentThread() == thread());

	bool Progress = false;

	{
		QMutexLocker QueueLock(&mQueueMutex);

		while (!mStarted.empty())
		{
			const std::shared_ptr<Request> RequestedPart = mStarted.front();

			mStarted.pop_front();

			const auto It = mRequests.find(RequestedPart->Info);

			if (!mStopping && It != mRequests.end() && It->second == RequestedPart && RequestedPart->Info->mState == lcPieceInfoState::Queued)
				RequestedPart->Info->mState = lcPieceInfoState::Loading;
		}

		while (!mStartedTextures.empty())
		{
			const std::shared_ptr<TextureRequest> RequestedTexture = mStartedTextures.front();

			mStartedTextures.pop_front();

			const auto It = mTextureRequests.find(RequestedTexture->Texture);

			if (!mStopping && It != mTextureRequests.end() && It->second == RequestedTexture && RequestedTexture->Texture->mState == lcTextureState::Queued)
				RequestedTexture->Texture->mState = lcTextureState::Decoding;
		}

	}

	constexpr int MaxCompletionsPerPump = 16;

	for (int Processed = 0; Processed < MaxCompletionsPerPump; Processed++)
	{
		Completion Completed;
		std::shared_ptr<TextureRequest> Retry;

		{
			QMutexLocker QueueLock(&mQueueMutex);

			const qint64 Now = QDateTime::currentMSecsSinceEpoch();

			for (const TextureRequestMap::value_type& Entry : mTextureRequests)
			{
				const std::shared_ptr<TextureRequest>& Candidate = Entry.second;

				if (Candidate->RetryAt && Candidate->RetryAt <= Now &&
					(!Retry || Candidate->LoadPriority > Retry->LoadPriority ||
						(Candidate->LoadPriority == Retry->LoadPriority && Candidate->RetryAt < Retry->RetryAt)))
					Retry = Candidate;
			}

			const auto PriorityOf = [](const Completion& Item)
			{
				if (Item.RequestedPart)
					return Item.RequestedPart->LoadPriority;

				return Item.RequestedTexture ? Item.RequestedTexture->LoadPriority : Item.RequestedSynth->LoadPriority;
			};

			const auto Best = std::max_element(mCompletions.begin(), mCompletions.end(), [&PriorityOf](const Completion& Left, const Completion& Right)
			{
				return PriorityOf(Left) < PriorityOf(Right);
			});

			if (Retry && (Best == mCompletions.end() || Retry->LoadPriority >= PriorityOf(*Best)))
				Retry->RetryAt = 0;
			else
			{
				Retry.reset();

				if (Best == mCompletions.end())
					return Progress;

				Completed = std::move(*Best);

				mCompletions.erase(Best);
			}
		}

		Progress = true;

		if (Retry)
		{
			UploadTextureRequest(Retry);

			continue;
		}

		if (Completed.RequestedTexture)
		{
			const std::shared_ptr<TextureRequest>& RequestedTexture = Completed.RequestedTexture;
			lcTexture* Texture = RequestedTexture->Texture;
			bool Stale;

			mQueueMutex.lock();
			const auto It = mTextureRequests.find(Texture);
			Stale = mStopping || RequestedTexture->Generation != mGeneration ||
				It == mTextureRequests.end() || It->second != RequestedTexture;
			if (Stale && It != mTextureRequests.end() && It->second == RequestedTexture)
				mTextureRequests.erase(It);
			RequestedTexture->Running = false;
			mResultReady.wakeAll();
			mQueueMutex.unlock();

			if (Stale)
				mLibrary->ReleaseTexture(Texture);
			else if (Completed.TextureResult.DecodedImage)
			{
				Texture->AdoptDecodedImage(std::move(*Completed.TextureResult.DecodedImage), LC_TEXTURE_MIPMAPS);
				UploadTextureRequest(RequestedTexture);
			}
			else
			{
				Texture->mState = lcTextureState::Failed;
				Texture->mLoadFailure = Completed.TextureResult.Error;

				FinishTextureRequest(RequestedTexture);
			}

			continue;
		}

		if (Completed.RequestedSynth)
		{
			const std::shared_ptr<SynthRequest>& RequestedSynth = Completed.RequestedSynth;
			bool Stale;

			mQueueMutex.lock();
			const SynthRequestMap::iterator It = mSynthRequests.find(RequestedSynth->Piece);
			Stale = mStopping || RequestedSynth->Cancelled.load(std::memory_order_relaxed) || RequestedSynth->Generation != mGeneration ||
				It == mSynthRequests.end() || It->second != RequestedSynth;
			if (Stale && It != mSynthRequests.end() && It->second == RequestedSynth)
				mSynthRequests.erase(It);
			RequestedSynth->Running = false;
			mResultReady.wakeAll();
			mQueueMutex.unlock();

			if (Stale)
			{
				RequestedSynth->Terminal = true;
				continue;
			}

			if (Completed.SynthMeshData)
			{
				Completed.SynthMeshData->ResolveColors();
				RequestedSynth->ColorTranslucency.clear();
				RequestedSynth->ColorTranslucency.reserve(gColorList.size());

				for (const lcColor& Color : gColorList)
					RequestedSynth->ColorTranslucency.push_back(Color.Translucent);

				RequestedSynth->DefaultColorIndex = gDefaultColor;
				RequestedSynth->MeshData = std::move(Completed.SynthMeshData);
				RequestedSynth->ConvertingMesh = true;

				QMutexLocker QueueLock(&mQueueMutex);

				if (mPaused)
					mPausedSynthQueue.push_back(RequestedSynth);
				else
				{
					mSynthQueue.push_back(RequestedSynth);
					StartWorkersLocked();
				}

				continue;
			}

			if (!Completed.SynthMesh)
			{
				RequestedSynth->Error = tr("Could not generate geometry for piece '%1'.").arg(QString::fromLatin1(RequestedSynth->Piece->mPieceInfo->mFileName));
				FinishSynth(RequestedSynth);
				continue;
			}

			RequestedSynth->ConvertingMesh = false;
			RequestedSynth->ColorTranslucency.clear();
			RequestedSynth->StagedMesh = std::move(Completed.SynthMesh);

			bool Ready = true;

			for (lcMeshLod& Lod : RequestedSynth->StagedMesh->mLods)
				for (int SectionIdx = 0; SectionIdx < Lod.NumSections; SectionIdx++)
				{
					lcMeshSection& Section = Lod.Sections[SectionIdx];

					if (Section.TextureName.isEmpty())
						continue;

					Section.Texture = mLibrary->FindTextureDeferred(Section.TextureName.toLatin1().constData(), {});

					if (!Section.Texture)
						RequestedSynth->Error = tr("Missing texture %1.").arg(Section.TextureName);
					else if (Section.Texture->mState == lcTextureState::Failed)
						RequestedSynth->Error = TextureFailureMessage(Section.Texture);
					else if (!Section.Texture->IsReady())
					{
						Ready = false;
						QueueTexture(Section.Texture, RequestedSynth->LoadPriority);
					}
				}

			if (!RequestedSynth->Error.isEmpty() || Ready)
				FinishSynth(RequestedSynth);

			continue;
		}

		const std::shared_ptr<Request>& RequestedPart = Completed.RequestedPart;
		PieceInfo* Info = RequestedPart->Info;
		bool Stale;

		mQueueMutex.lock();
		const auto It = mRequests.find(Info);
		Stale = mStopping || RequestedPart->Obsolete || Completed.Result.Generation != mGeneration || It == mRequests.end() || It->second != RequestedPart;
		if (Stale && It != mRequests.end() && It->second == RequestedPart)
			mRequests.erase(It);
		mQueueMutex.unlock();

		RequestedPart->Running = false;

		const bool HasConsumer = mLibrary->HasPieceConsumers(Info);

		if (Stale || !HasConsumer)
		{
			if (!Stale)
			{
				mQueueMutex.lock();
				mRequests.erase(Info);
				mQueueMutex.unlock();
			}

			RequestedPart->Terminal = true;
			RequestedPart->Succeeded = false;

			if (HasConsumer && !Stale)
				Info->mState = lcPieceInfoState::Cancelled;

			mResultReady.wakeAll();

			mLibrary->ReleasePieceLoadHold(Info);

			continue;
		}

		RequestedPart->Error = Completed.Result.Error;
		RequestedPart->EmptyGeometry = Completed.Result.EmptyGeometry;

		if (RequestedPart->Error.isEmpty())
		{
			const auto TextureLookup = [this, &RequestedPart](const char* Name)
			{
				return mLibrary->FindTextureDeferred(Name, RequestedPart->Source.TextureSearchDirectories);
			};

			if (Completed.Result.CacheData && !Completed.Result.Mesh)
			{
				// Rebuild a corrupt cache privately on a worker, then use the normal commit path.
				RequestedPart->Source.SkipCache = true;
				Info->mState = lcPieceInfoState::Queued;

				QMutexLocker QueueLock(&mQueueMutex);

				if (mPaused)
					mPausedQueue.push_back(RequestedPart);
				else
				{
					mQueue.push_back(RequestedPart);
					StartWorkersLocked();
				}

				continue;
			}
			else if (Completed.Result.MeshData)
			{
				// Register color indices on the UI thread, then queue the expensive mesh conversion.
				Completed.Result.MeshData->ResolveColors();
				RequestedPart->ColorTranslucency.clear();
				RequestedPart->ColorTranslucency.reserve(gColorList.size());

				for (const lcColor& Color : gColorList)
					RequestedPart->ColorTranslucency.push_back(Color.Translucent);

				RequestedPart->DefaultColorIndex = gDefaultColor;
				RequestedPart->MeshData = std::move(Completed.Result.MeshData);
				RequestedPart->ConvertingMesh = true;
				Info->mState = lcPieceInfoState::Queued;

				QMutexLocker QueueLock(&mQueueMutex);

				if (mPaused)
					mPausedQueue.push_back(RequestedPart);
				else
				{
					mQueue.push_back(RequestedPart);
					StartWorkersLocked();
				}

				continue;
			}
			else if (Completed.Result.Mesh)
			{
				RequestedPart->ConvertingMesh = false;
				RequestedPart->ColorTranslucency.clear();
				RequestedPart->StagedMesh = std::move(Completed.Result.Mesh);
				size_t ColorIndex = 0;

				for (lcMeshLod& Lod : RequestedPart->StagedMesh->mLods)
				{
					for (int SectionIdx = 0; SectionIdx < Lod.NumSections; SectionIdx++)
					{
						lcMeshSection& Section = Lod.Sections[SectionIdx];

						if (Completed.Result.CacheData)
							Section.ColorIndex = lcGetColorIndex(Completed.Result.ColorCodes[ColorIndex++]);

						if (!Section.TextureName.isEmpty())
							Section.Texture = TextureLookup(Section.TextureName.toLatin1().constData());
					}
				}

				RequestedPart->SaveCache = !Completed.Result.CacheData;
			}
		}

		if (RequestedPart->StagedMesh)
		{
			bool Ready = true;

			for (const lcMeshLod& Lod : RequestedPart->StagedMesh->mLods)
			{
				for (int SectionIdx = 0; SectionIdx < Lod.NumSections; SectionIdx++)
				{
					const lcMeshSection& Section = Lod.Sections[SectionIdx];

					if (Section.TextureName.isEmpty())
						continue;

					if (!Section.Texture)
					{
						RequestedPart->Error = tr("Missing texture %1.").arg(Section.TextureName);

						continue;
					}

					if (Section.Texture->mState == lcTextureState::Failed)
						RequestedPart->Error = TextureFailureMessage(Section.Texture);
					else if (!Section.Texture->IsReady())
					{
						Ready = false;
						QueueTexture(Section.Texture, RequestedPart->LoadPriority);
					}
				}
			}

			if (RequestedPart->Error.isEmpty() && !Ready)
			{
				Info->mState = lcPieceInfoState::AwaitingTextures;

				continue;
			}
		}

		FinishPart(RequestedPart);
	}

	mQueueMutex.lock();
	const qint64 Now = QDateTime::currentMSecsSinceEpoch();
	const bool RetryDue = std::any_of(mTextureRequests.begin(), mTextureRequests.end(), [Now](const auto& Entry)
	{
		return Entry.second->RetryAt && Entry.second->RetryAt <= Now;
	});
	const bool NeedsPump = !mCompletions.empty() || RetryDue;
	mQueueMutex.unlock();
	if (NeedsPump)
		QCoreApplication::postEvent(this, new QEvent(lcAssetLoaderCompletionEvent));
	return Progress;
}

void lcAssetLoader::DispatchNotifications()
{
	Q_ASSERT(QThread::currentThread() == thread());

	constexpr int MaxNotificationsPerPump = 32;

	for (int Delivered = 0; Delivered < MaxNotificationsPerPump && !mNotifications.empty(); Delivered++)
	{
		Notification Notification = std::move(mNotifications.front());

		mNotifications.pop_front();

		PieceInfo* Info = Notification.RequestedPart->Info;

		if (Notification.RequestedPart->Obsolete || !mLibrary->HasPieceConsumers(Info))
		{
			mLibrary->ReleasePieceLoadHold(Info);

			continue;
		}

		QPointer<lcAssetLoader> LoaderGuard(this);
		QPointer<lcPiecesLibrary> LibraryGuard(mLibrary);

		if (Notification.Loaded)
			emit mLibrary->PartLoaded(Info);
		else
			emit mLibrary->PartLoadFailed(Info, Notification.Error);

		if (!LoaderGuard || !LibraryGuard)
			return;

		mLibrary->ReleasePieceLoadHold(Info);
	}

	std::unordered_set<PieceInfo*> SettledSynths;

	for (int Delivered = 0; Delivered < MaxNotificationsPerPump && !mSynthNotifications.empty(); Delivered++)
	{
		SynthNotification Notification = std::move(mSynthNotifications.front());

		mSynthNotifications.pop_front();

		PieceInfo* Info = Notification.Info;

		if (!Notification.RequestedSynth->Cancelled.load(std::memory_order_relaxed) && mLibrary->HasPieceConsumers(Info) && SettledSynths.insert(Info).second)
		{
			QPointer<lcAssetLoader> LoaderGuard(this);
			QPointer<lcPiecesLibrary> LibraryGuard(mLibrary);

			emit mLibrary->GeneratedMeshSettled(Info);

			if (!LoaderGuard || !LibraryGuard)
				return;
		}

		mLibrary->ReleasePieceLoadHold(Info);
	}

	if (!mNotifications.empty() || !mSynthNotifications.empty())
		QCoreApplication::postEvent(this, new QEvent(lcAssetLoaderCompletionEvent));

	if (!SettledSynths.empty())
		mLibrary->NotifyConsumersChanged();
}

bool lcAssetLoader::WaitForRequest(PieceInfo* Info)
{
	Q_ASSERT(QThread::currentThread() == thread());

	std::shared_ptr<Request> RequestedPart;

	{
		QMutexLocker QueueLock(&mQueueMutex);

		const auto It = mRequests.find(Info);

		if (It != mRequests.end())
		{
			RequestedPart = It->second;

			PromoteRequestPriorityLocked(RequestedPart, Priority::Blocking);
		}
		StartWorkersLocked();
	}

	if (!RequestedPart)
		return Info->mState == lcPieceInfoState::Loaded;

	QOpenGLContext* const PreviousContext = QOpenGLContext::currentContext();
	QSurface* const PreviousSurface = PreviousContext ? PreviousContext->surface() : nullptr;

	for (;;)
	{
		ProcessCompletions();

		if (RequestedPart->Terminal)
			break;

		QMutexLocker QueueLock(&mQueueMutex);

		if (mStopping || RequestedPart->Terminal)
			break;

		WaitForResultsLocked();
	}

	if (PreviousContext && PreviousSurface && (QOpenGLContext::currentContext() != PreviousContext || PreviousContext->surface() != PreviousSurface))
		PreviousContext->makeCurrent(PreviousSurface);

	return RequestedPart->Succeeded;
}

bool lcAssetLoader::EnsurePieceReady(PieceInfo* Info)
{
	return EnsurePiecesReady({ Info });
}

bool lcAssetLoader::EnsurePiecesReady(const std::vector<PieceInfo*>& Parts)
{
	Q_ASSERT(QThread::currentThread() == thread());

	std::vector<PieceInfo*> Required = Parts;
	std::sort(Required.begin(), Required.end());

	Required.erase(std::unique(Required.begin(), Required.end()), Required.end());

	for (PieceInfo* Info : Required)
		mLibrary->AddPieceReference(Info);

	for (PieceInfo* Info : Required)
	{
		if (Info->mState != lcPieceInfoState::Unloaded)
			continue;

		if (Info->IsProject() || (Info->IsModel() && !Info->GetModel()->HasDirectGeometry()))
			Info->mState = lcPieceInfoState::Loaded;
		else
			QueuePieceLocked(Info, Priority::Blocking);
	}

	{
		QMutexLocker QueueLock(&mQueueMutex);

		for (PieceInfo* Info : Required)
		{
			const auto It = mRequests.find(Info);

			if (It == mRequests.end())
				continue;

			PromoteRequestPriorityLocked(It->second, Priority::Blocking);
		}

		StartWorkersLocked();
	}

	bool Ready = true;

	for (PieceInfo* Info : Required)
		Ready = WaitForRequest(Info) && Ready;

	for (PieceInfo* Info : Required)
		mLibrary->ReleasePieceInfo(Info);

	return Ready;
}

bool lcAssetLoader::EnsureTextureReady(lcTexture* Texture)
{
	Q_ASSERT(QThread::currentThread() == thread());

	if (Texture->IsReady())
		return true;

	QueueTexture(Texture, Priority::Blocking);

	for (;;)
	{
		ProcessCompletions();

		if (Texture->IsReady() || Texture->mState == lcTextureState::Failed)
			break;

		QMutexLocker QueueLock(&mQueueMutex);

		if (mStopping || mTextureRequests.find(Texture) == mTextureRequests.end())
			break;

		WaitForResultsLocked();
	}

	return Texture->IsReady();
}

void lcAssetLoader::QueueModelPiece(PieceInfo* Info)
{
	Q_ASSERT(QThread::currentThread() == thread());
	Q_ASSERT(Info->IsModel());

	if (Info->IsLoading())
		WaitForRequest(Info);

	Info->ReleaseMesh();
	mLibrary->ClearPieceLoadError(Info);

	if (!Info->GetModel()->HasDirectGeometry())
	{
		Info->mState = lcPieceInfoState::Loaded;
		emit mLibrary->AssetRequestsChanged();
		return;
	}

	Info->mState = lcPieceInfoState::Unloaded;

	QueuePieceLocked(Info, Priority::Visible);
}

void lcAssetLoader::InvalidatePiece(PieceInfo* Info)
{
	Q_ASSERT(QThread::currentThread() == thread());

	std::shared_ptr<Request> Removed;
	bool ReleaseHold = false;

	{
		QMutexLocker QueueLock(&mQueueMutex);

		const auto It = mRequests.find(Info);

		if (It != mRequests.end())
		{
			Removed = It->second;
			Removed->Obsolete = true;
			Removed->Terminal = true;
			Removed->Succeeded = false;
			ReleaseHold = !Removed->Running;
			mRequests.erase(It);

			const auto EraseRequest = [&Removed](auto& Queue)
			{
				Queue.erase(std::remove(Queue.begin(), Queue.end(), Removed), Queue.end());
			};

			EraseRequest(mQueue);
			EraseRequest(mPausedQueue);
		}

		for (Notification& Pending : mNotifications)
			if (Pending.RequestedPart->Info == Info)
				Pending.RequestedPart->Obsolete = true;

		mResultReady.wakeAll();
	}

	if (Removed && !Removed->Running)
		Removed->StagedMesh.reset();

	Info->mState = lcPieceInfoState::Unloaded;
	mLibrary->ClearPieceLoadError(Info);

	// Running work retains its load hold until its stale completion arrives.
	if (ReleaseHold)
		mLibrary->ReleasePieceLoadHold(Info);

	if (Removed)
		CancelUnusedTextureRequests();

	emit mLibrary->AssetRequestsChanged();
}

void lcAssetLoader::WaitForLoadQueue()
{
	Q_ASSERT(QThread::currentThread() == thread());

	QMutexLocker DrainLock(&mDrainMutex);

	for (;;)
	{
		ProcessCompletions();

		QMutexLocker QueueLock(&mQueueMutex);

		if (mRequests.empty() && mTextureRequests.empty() && mSynthRequests.empty() && mActiveWorkers == 0 && mCompletions.empty())
			break;

		WaitForResultsLocked();
	}

	for (QFuture<void>& Future : mFutures)
		Future.waitForFinished();

	mFutures.clear();
}

bool lcAssetLoader::HasPendingWork()
{
	mQueueMutex.lock();
	const bool Pending = !mRequests.empty() || !mTextureRequests.empty() || !mSynthRequests.empty() || !mCompletions.empty();
	mQueueMutex.unlock();

	return Pending;
}

void lcAssetLoader::CancelAndDrain()
{
	Q_ASSERT(QThread::currentThread() == thread());

	std::deque<std::shared_ptr<Request>> Cancelled;
	std::deque<std::shared_ptr<TextureRequest>> CancelledTextures;

	{
		QMutexLocker QueueLock(&mQueueMutex);

		mStopping = true;
		mGeneration++;
		mQueue.clear();
		mPausedQueue.clear();
		mTextureQueue.clear();
		mPausedTextureQueue.clear();
		mSynthQueue.clear();
		mPausedSynthQueue.clear();
		mSynthPieces.clear();
		mFailedSynthErrors.clear();

		for (auto It = mRequests.begin(); It != mRequests.end();)
		{
			if (It->second->Running)
			{
				It->second->Obsolete = true;
				++It;
			}
			else
			{
				It->second->Terminal = true;
				It->second->Succeeded = false;
				It->second->Info->mState = lcPieceInfoState::Cancelled;

				Cancelled.push_back(It->second);

				It = mRequests.erase(It);
			}
		}

		for (auto It = mTextureRequests.begin(); It != mTextureRequests.end();)
		{
			if (It->second->Running)
				++It;
			else
			{
				CancelledTextures.push_back(It->second);

				It = mTextureRequests.erase(It);
			}
		}

		for (SynthRequestMap::iterator It = mSynthRequests.begin(); It != mSynthRequests.end();)
		{
			if (It->second->Running)
			{
				It->second->Cancelled.store(true, std::memory_order_relaxed);
				++It;
			}
			else
			{
				It->second->Terminal = true;
				It = mSynthRequests.erase(It);
			}
		}

		mResultReady.wakeAll();
	}

	for (const std::shared_ptr<Request>& RequestedPart : Cancelled)
		mLibrary->ReleasePieceLoadHold(RequestedPart->Info);

	for (const std::shared_ptr<TextureRequest>& RequestedTexture : CancelledTextures)
		mLibrary->ReleaseTexture(RequestedTexture->Texture);

	WaitForLoadQueue();

	while (!mNotifications.empty())
	{
		Notification Notification = std::move(mNotifications.front());

		mNotifications.pop_front();

		mLibrary->ReleasePieceLoadHold(Notification.RequestedPart->Info);
	}

	while (!mSynthNotifications.empty())
	{
		SynthNotification Notification = std::move(mSynthNotifications.front());

		mSynthNotifications.pop_front();

		mLibrary->ReleasePieceLoadHold(Notification.Info);
	}
}

void lcAssetLoader::PauseQueuedWork()
{
	Q_ASSERT(QThread::currentThread() == thread());

	mQueueMutex.lock();
	mPaused = true;
	mPausedQueue.swap(mQueue);
	mPausedTextureQueue.swap(mTextureQueue);
	mPausedSynthQueue.swap(mSynthQueue);
	mQueueMutex.unlock();

	for (;;)
	{
		ProcessCompletions();

		QMutexLocker QueueLock(&mQueueMutex);

		if (mActiveWorkers == 0 && mCompletions.empty())
			break;

		WaitForResultsLocked();
	}

	// A mesh awaiting textures was built with the previous stud settings.
	// Rebuild it after the settings change instead of publishing stale geometry.
	for (const PartRequestMap::value_type& Entry : mRequests)
	{
		const std::shared_ptr<Request>& RequestedPart = Entry.second;

		if (RequestedPart->ConvertingMesh)
		{
			RequestedPart->MeshData.reset();
			RequestedPart->ColorTranslucency.clear();
			RequestedPart->ConvertingMesh = false;
			RequestedPart->Info->mState = lcPieceInfoState::Queued;
		}

		if (!RequestedPart->StagedMesh)
			continue;

		RequestedPart->StagedMesh.reset();
		RequestedPart->Info->mState = lcPieceInfoState::Queued;
		RequestedPart->Running = false;
		mPausedQueue.push_back(RequestedPart);
	}

	// Pending generated meshes are restarted after the new stud style is installed.
	for (const SynthRequestMap::value_type& Entry : mSynthRequests)
	{
		const std::shared_ptr<SynthRequest>& RequestedSynth = Entry.second;

		if (!RequestedSynth->StagedMesh && !RequestedSynth->ConvertingMesh)
			continue;

		RequestedSynth->MeshData.reset();
		RequestedSynth->StagedMesh.reset();
		RequestedSynth->ColorTranslucency.clear();
		RequestedSynth->ConvertingMesh = false;

		if (std::find(mPausedSynthQueue.begin(), mPausedSynthQueue.end(), RequestedSynth) == mPausedSynthQueue.end())
			mPausedSynthQueue.push_back(RequestedSynth);
	}
}

void lcAssetLoader::ResumeQueuedWork()
{
	Q_ASSERT(QThread::currentThread() == thread());

	QMutexLocker QueueLock(&mQueueMutex);

	mPaused = false;

	for (const std::shared_ptr<Request>& RequestedPart : mPausedQueue)
	{
		const bool SkipCache = RequestedPart->Source.SkipCache;

		RequestedPart->Source = mLibrary->SnapshotPieceSource(RequestedPart->Info);
		RequestedPart->Source.SkipCache = SkipCache;
	}

	mQueue.swap(mPausedQueue);
	mTextureQueue.swap(mPausedTextureQueue);
	mSynthQueue.swap(mPausedSynthQueue);

	StartWorkersLocked();
}

bool lcAssetLoader::event(QEvent* Event)
{
	if (Event->type() == lcAssetLoaderCompletionEvent)
	{
		ProcessCompletions();
		DispatchNotifications();

		return true;
	}

	return QObject::event(Event);
}
