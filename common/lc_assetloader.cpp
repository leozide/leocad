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

lcPieceBuildResult::~lcPieceBuildResult() = default;

// Normal part flow: enqueue -> build and convert -> finalize sections -> settle textures -> publish -> notify.
// Workers own private build data. Publication and deferred notifications stay on the UI thread.
lcAssetLoader::lcAssetLoader(lcPiecesLibrary* Library)
	: mLibrary(Library)
{
}

lcAssetLoader::~lcAssetLoader()
{
	CancelAndDrain();
}

bool lcAssetLoader::RequestPieceLoad(PieceInfo* Info, lcPieceLoadPriority LoadPriority)
{
	Q_ASSERT(QThread::currentThread() == thread());

	if (Info->IsProject() || (Info->IsModel() && !Info->GetModel()->HasDirectGeometry()))
		Info->mState = lcPieceInfoState::Loaded;
	else if (Info->mState == lcPieceInfoState::Unloaded)
		QueuePiece(Info, LoadPriority);
	else if (LoadPriority != lcPieceLoadPriority::Low)
	{
		mQueueMutex.lock();
		const auto It = mPieceRequests.find(Info);
		if (It != mPieceRequests.end())
			PromotePiecePriorityLocked(It->second, LoadPriority);
		mQueueMutex.unlock();
	}

	return LoadPriority != lcPieceLoadPriority::Blocking || WaitForPieceRequest(Info);
}

void lcAssetLoader::QueuePiece(PieceInfo* Info, lcPieceLoadPriority LoadPriority)
{
	QMutexLocker QueueLock(&mQueueMutex);

	if (mStopping)
		return;

	const auto Existing = mPieceRequests.find(Info);

	if (Existing != mPieceRequests.end())
	{
		PromotePiecePriorityLocked(Existing->second, LoadPriority);

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

	std::shared_ptr<PieceRequest> RequestedPiece = std::make_shared<PieceRequest>();

	RequestedPiece->Info = Info;
	RequestedPiece->Source = mLibrary->SnapshotPieceSource(Info);
	RequestedPiece->LoadPriority = LoadPriority;
	RequestedPiece->EnqueuedAt = QDateTime::currentMSecsSinceEpoch();

	mPieceRequests.emplace(Info, RequestedPiece);

	mPieceQueue.push_back(RequestedPiece);
	StartWorkersLocked();

	emit mLibrary->AssetRequestsChanged();
}

void lcAssetLoader::PromotePiecePriorityLocked(const std::shared_ptr<PieceRequest>& RequestedPiece, lcPieceLoadPriority LoadPriority)
{
	RequestedPiece->LoadPriority = std::max(RequestedPiece->LoadPriority, LoadPriority);

	if (!RequestedPiece->StagedMesh)
		return;

	for (const lcMeshLod& Lod : RequestedPiece->StagedMesh->mLods)
	{
		for (int SectionIdx = 0; SectionIdx < Lod.NumSections; SectionIdx++)
		{
			const auto It = mTextureRequests.find(Lod.Sections[SectionIdx].Texture);

			if (It != mTextureRequests.end())
				It->second->LoadPriority = std::max(It->second->LoadPriority, RequestedPiece->LoadPriority);
		}
	}
}

void lcAssetLoader::RequestSynthMesh(lcPiece* Piece)
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
	RequestedSynth->LoadPriority = lcPieceLoadPriority::Normal;
	RequestedSynth->EnqueuedAt = QDateTime::currentMSecsSinceEpoch();

	QMutexLocker QueueLock(&mQueueMutex);

	if (mStopping)
		return;

	mSynthPieces.insert(Piece);
	mSynthRequests.emplace(Piece, RequestedSynth);

	mSynthQueue.push_back(RequestedSynth);
	StartWorkersLocked();

	QueueLock.unlock();
	emit mLibrary->AssetRequestsChanged();
}

void lcAssetLoader::QueueTexture(lcTexture* Texture, lcPieceLoadPriority LoadPriority)
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

	mLibrary->AddTextureReference(Texture);
	Texture->mState = lcTextureState::Queued;

	std::shared_ptr<TextureRequest> RequestedTexture = std::make_shared<TextureRequest>();

	RequestedTexture->Texture = Texture;
	RequestedTexture->Source = Texture->mSource;
	RequestedTexture->LoadPriority = LoadPriority;
	RequestedTexture->EnqueuedAt = QDateTime::currentMSecsSinceEpoch();

	mTextureRequests.emplace(Texture, RequestedTexture);

	mTextureQueue.push_back(RequestedTexture);
	StartWorkersLocked();
}

void lcAssetLoader::StartWorkersLocked()
{
	mFutures.erase(std::remove_if(mFutures.begin(), mFutures.end(), [](const QFuture<void>& Future)
	{
		return Future.isFinished();
	}), mFutures.end());

	const int MaxWorkers = qMax(1, qMin(4, QThread::idealThreadCount()));
	const int Queued = static_cast<int>(mPieceQueue.size() + mTextureQueue.size() + mSynthQueue.size());
	const int Additional = mPaused || mStopping ? 0 : qMin(MaxWorkers - mActiveWorkers, Queued);

	for (int Worker = 0; Worker < Additional; Worker++)
	{
		mActiveWorkers++;
		mFutures.emplace_back(QtConcurrent::run([this]() { RunWorker(); }));
	}
}

void lcAssetLoader::RunWorker()
{
	for (;;)
	{
		std::shared_ptr<PieceRequest> RequestedPiece;
		std::shared_ptr<TextureRequest> RequestedTexture;
		std::shared_ptr<SynthRequest> RequestedSynth;

		{
			QMutexLocker QueueLock(&mQueueMutex);

			if (mStopping || mPaused || (mPieceQueue.empty() && mTextureQueue.empty() && mSynthQueue.empty()))
			{
				mActiveWorkers--;
				mResultReady.wakeAll();

				return;
			}

			const qint64 Now = QDateTime::currentMSecsSinceEpoch();

			const auto Score = [Now](const auto& Item)
			{
				// Each ten seconds of waiting promotes one priority level.
				return qMin<qint64>(static_cast<qint64>(lcPieceLoadPriority::Blocking),
					static_cast<qint64>(Item->LoadPriority) + qMax<qint64>(0, Now - Item->EnqueuedAt) / 10000);
			};

			const auto Less = [&Score](const auto& Left, const auto& Right)
			{
				return Score(Left) < Score(Right) || (Score(Left) == Score(Right) && Left->EnqueuedAt > Right->EnqueuedAt);
			};

			auto PieceBest = std::max_element(mPieceQueue.begin(), mPieceQueue.end(), Less);
			auto TextureBest = std::max_element(mTextureQueue.begin(), mTextureQueue.end(), Less);
			auto SynthBest = std::max_element(mSynthQueue.begin(), mSynthQueue.end(), Less);

			if (TextureBest != mTextureQueue.end() && (PieceBest == mPieceQueue.end() || !Less(*TextureBest, *PieceBest)) &&
				(SynthBest == mSynthQueue.end() || !Less(*TextureBest, *SynthBest)))
			{
				RequestedTexture = *TextureBest;
				mTextureQueue.erase(TextureBest);
				RequestedTexture->AwaitingCompletion = true;
			}
			else if (SynthBest != mSynthQueue.end() && (PieceBest == mPieceQueue.end() || !Less(*SynthBest, *PieceBest)))
			{
				RequestedSynth = *SynthBest;
				mSynthQueue.erase(SynthBest);
				RequestedSynth->AwaitingCompletion = true;
			}
			else
			{
				RequestedPiece = *PieceBest;
				mPieceQueue.erase(PieceBest);
				RequestedPiece->AwaitingCompletion = true;
			}
		}

		if (RequestedPiece || RequestedTexture || RequestedSynth)
			QCoreApplication::postEvent(this, new QEvent(lcAssetLoaderCompletionEvent));

		Completion Completed;

		if (RequestedPiece)
		{
			Completed.RequestedPiece = RequestedPiece;

			Completed.PieceResult = mLibrary->BuildPieceData(RequestedPiece->Source);
		}
		else if (RequestedTexture)
		{
			Completed.RequestedTexture = RequestedTexture;
			Completed.TextureResult = mLibrary->BuildTextureData(RequestedTexture->Source);
		}
		else
		{
			Completed.RequestedSynth = RequestedSynth;

			Completed.SynthMesh = RequestedSynth->SynthInfo->BuildMesh(RequestedSynth->ControlPoints, RequestedSynth->Cancelled);
		}

		{
			QMutexLocker QueueLock(&mQueueMutex);
			mCompletions.push_back(std::move(Completed));
			mResultReady.wakeAll();
		}

		QCoreApplication::postEvent(this, new QEvent(lcAssetLoaderCompletionEvent));
	}
}

bool lcAssetLoader::ProcessCompletions()
{
	// Bounded UI-thread publication also runs during scoped waits, without public callbacks.
	Q_ASSERT(QThread::currentThread() == thread());

	bool Progress = false;

	{
		QMutexLocker QueueLock(&mQueueMutex);

		// Reflect worker starts in asset states on the UI thread.
		if (!mStopping)
		{
			for (const PieceRequestMap::value_type& Entry : mPieceRequests)
			{
				const std::shared_ptr<PieceRequest>& RequestedPiece = Entry.second;

				if (RequestedPiece->AwaitingCompletion && RequestedPiece->Info->mState == lcPieceInfoState::Queued)
					RequestedPiece->Info->mState = lcPieceInfoState::Loading;
			}

			for (const TextureRequestMap::value_type& Entry : mTextureRequests)
			{
				const std::shared_ptr<TextureRequest>& RequestedTexture = Entry.second;

				if (RequestedTexture->AwaitingCompletion && RequestedTexture->Texture->mState == lcTextureState::Queued)
					RequestedTexture->Texture->mState = lcTextureState::Decoding;
			}
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
				if (Item.RequestedPiece)
					return Item.RequestedPiece->LoadPriority;

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
			ProcessTextureCompletion(Completed);
		else if (Completed.RequestedSynth)
			ProcessSynthCompletion(Completed);
		else
			ProcessPieceCompletion(Completed);
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

void lcAssetLoader::ProcessPieceCompletion(Completion& Completed)
{
	const std::shared_ptr<PieceRequest>& RequestedPiece = Completed.RequestedPiece;
	PieceInfo* Info = RequestedPiece->Info;
	bool Stale;

	mQueueMutex.lock();
	const auto It = mPieceRequests.find(Info);
	Stale = mStopping || RequestedPiece->Obsolete || It == mPieceRequests.end() || It->second != RequestedPiece;
	if (Stale && It != mPieceRequests.end() && It->second == RequestedPiece)
		mPieceRequests.erase(It);
	mQueueMutex.unlock();

	RequestedPiece->AwaitingCompletion = false;

	const bool HasConsumer = mLibrary->HasPieceConsumers(Info);

	if (Stale || !HasConsumer)
	{
		if (!Stale)
		{
			mQueueMutex.lock();
			mPieceRequests.erase(Info);
			mQueueMutex.unlock();
		}

		RequestedPiece->Terminal = true;
		RequestedPiece->Succeeded = false;

		if (HasConsumer && !Stale)
			Info->mState = lcPieceInfoState::Cancelled;

		mResultReady.wakeAll();

		mLibrary->ReleasePieceLoadHold(Info);

		return;
	}

	RequestedPiece->Error = Completed.PieceResult.Error;
	RequestedPiece->EmptyGeometry = Completed.PieceResult.EmptyGeometry;

	if (RequestedPiece->Error.isEmpty() && Completed.PieceResult.Mesh)
	{
		RequestedPiece->StagedMesh = std::move(Completed.PieceResult.Mesh);

		RequestedPiece->StagedMesh->FinalizeSections();

		RequestedPiece->SaveCache = !Completed.PieceResult.FromCache;
	}

	// A private mesh cannot be published until all its declared textures settle.
	if (RequestedPiece->StagedMesh)
	{
		const bool Ready = ResolveAndQueueMeshTextures(*RequestedPiece->StagedMesh, RequestedPiece->Source.TextureSearchDirectories,
			RequestedPiece->Source.EmbeddedData, RequestedPiece->LoadPriority, RequestedPiece->Error);

		if (RequestedPiece->Error.isEmpty() && !Ready)
		{
			Info->mState = lcPieceInfoState::AwaitingTextures;

			return;
		}
	}

	FinishPiece(RequestedPiece);
}

void lcAssetLoader::ProcessSynthCompletion(Completion& Completed)
{
	const std::shared_ptr<SynthRequest>& RequestedSynth = Completed.RequestedSynth;
	bool Stale;

	mQueueMutex.lock();
	const SynthRequestMap::iterator It = mSynthRequests.find(RequestedSynth->Piece);
	Stale = mStopping || RequestedSynth->Cancelled.load(std::memory_order_relaxed) ||
		It == mSynthRequests.end() || It->second != RequestedSynth;
	if (Stale && It != mSynthRequests.end() && It->second == RequestedSynth)
		mSynthRequests.erase(It);
	RequestedSynth->AwaitingCompletion = false;
	mResultReady.wakeAll();
	mQueueMutex.unlock();

	if (Stale)
	{
		RequestedSynth->Terminal = true;
		return;
	}

	if (!Completed.SynthMesh)
	{
		RequestedSynth->Error = tr("Could not generate geometry for piece '%1'.").arg(QString::fromLatin1(RequestedSynth->Piece->mPieceInfo->mFileName));
		FinishSynth(RequestedSynth);
		return;
	}

	RequestedSynth->StagedMesh = std::move(Completed.SynthMesh);
	RequestedSynth->StagedMesh->FinalizeSections();

	const bool Ready = ResolveAndQueueMeshTextures(*RequestedSynth->StagedMesh, std::vector<QString>(),
		std::map<QString, std::shared_ptr<const lcEmbeddedData>>(), RequestedSynth->LoadPriority, RequestedSynth->Error);

	if (!RequestedSynth->Error.isEmpty() || Ready)
		FinishSynth(RequestedSynth);
}

void lcAssetLoader::ProcessTextureCompletion(Completion& Completed)
{
	const std::shared_ptr<TextureRequest>& RequestedTexture = Completed.RequestedTexture;
	lcTexture* Texture = RequestedTexture->Texture;
	bool Stale;

	mQueueMutex.lock();
	const auto It = mTextureRequests.find(Texture);
	Stale = mStopping || It == mTextureRequests.end() || It->second != RequestedTexture;
	if (Stale && It != mTextureRequests.end() && It->second == RequestedTexture)
		mTextureRequests.erase(It);
	RequestedTexture->AwaitingCompletion = false;
	mResultReady.wakeAll();
	mQueueMutex.unlock();

	if (Stale)
		mLibrary->ReleaseTexture(Texture);
	else if (!Completed.TextureResult.DecodedImage.isNull())
	{
		Texture->SetImage(std::move(Completed.TextureResult.DecodedImage), LC_TEXTURE_MIPMAPS);
		if (Texture->NeedsUpload())
			UploadTextureRequest(RequestedTexture);
		else
			FinishTexture(RequestedTexture);
	}
	else
	{
		Texture->SetFailed(Completed.TextureResult.Error, std::move(Completed.TextureResult.ErrorDetails));

		FinishTexture(RequestedTexture);
	}
}

bool lcAssetLoader::ResolveAndQueueMeshTextures(lcMesh& Mesh, const std::vector<QString>& SearchDirectories,
	const std::map<QString, std::shared_ptr<const lcEmbeddedData>>& EmbeddedData, lcPieceLoadPriority LoadPriority, QString& Error)
{
	for (lcMeshLod& Lod : Mesh.mLods)
		for (int SectionIdx = 0; SectionIdx < Lod.NumSections; SectionIdx++)
		{
			lcMeshSection& Section = Lod.Sections[SectionIdx];

			if (Section.TextureName.isEmpty())
				continue;

			Section.Texture = mLibrary->FindTextureDeferred(Section.TextureName.toLatin1().constData(), SearchDirectories, EmbeddedData);

			if (Section.Texture && !Section.Texture->IsReady() && Section.Texture->mState != lcTextureState::Failed)
				QueueTexture(Section.Texture, LoadPriority);
		}

	return MeshTexturesSettled(Mesh, Error);
}

bool lcAssetLoader::MeshTexturesSettled(const lcMesh& Mesh, QString& Error)
{
	bool Ready = true;

	for (const lcMeshLod& Lod : Mesh.mLods)
		for (int SectionIdx = 0; SectionIdx < Lod.NumSections; SectionIdx++)
		{
			const lcMeshSection& Section = Lod.Sections[SectionIdx];
			lcTexture* Texture = Section.Texture;

			if (!Texture)
			{
				if (!Section.TextureName.isEmpty())
					Error = tr("Missing texture %1.").arg(Section.TextureName);
			}
			else if (Texture->mState == lcTextureState::Failed)
				Error = TextureFailureMessage(Texture);
			else if (!Texture->IsReady())
				Ready = false;
		}

	return Ready;
}

QString lcAssetLoader::TextureFailureMessage(const lcTexture* Texture)
{
	if (!Texture->mLoadFailureDetails.isEmpty())
		return Texture->mLoadFailureDetails;

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
			Texture->SetFailed(lcTextureLoadError::ContextActivationFailed, QString());
		}
	}
	else
	{
		Texture->SetFailed(lcTextureLoadError::ContextUnavailable, QString());
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
		lcTextureLoadError Error = Texture->mLoadFailure;

		if (Error == lcTextureLoadError::None)
			Error = lcTextureLoadError::UploadFailed;

		Texture->SetFailed(Error, Texture->mLoadFailureDetails);
		Texture->DiscardDecodedImage();
	}

	FinishTexture(RequestedTexture);
}

void lcAssetLoader::FinishTexture(const std::shared_ptr<TextureRequest>& RequestedTexture)
{
	mQueueMutex.lock();
	const auto It = mTextureRequests.find(RequestedTexture->Texture);
	if (It != mTextureRequests.end() && It->second == RequestedTexture)
		mTextureRequests.erase(It);
	mResultReady.wakeAll();
	mQueueMutex.unlock();

	mLibrary->ReleaseTexture(RequestedTexture->Texture);

	FinishPiecesWithSettledTextures();
	FinishSynthsWithSettledTextures();
}

void lcAssetLoader::FinishPiecesWithSettledTextures()
{
	std::vector<std::shared_ptr<PieceRequest>> Waiting;

	{
		QMutexLocker QueueLock(&mQueueMutex);

		for (const PieceRequestMap::value_type& Entry : mPieceRequests)
			if (Entry.second->StagedMesh)
				Waiting.push_back(Entry.second);
	}

	for (const std::shared_ptr<PieceRequest>& RequestedPiece : Waiting)
	{
		const bool Ready = MeshTexturesSettled(*RequestedPiece->StagedMesh, RequestedPiece->Error);

		if (!RequestedPiece->Error.isEmpty() || Ready)
			FinishPiece(RequestedPiece);
	}
}

void lcAssetLoader::FinishSynthsWithSettledTextures()
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

		const bool Ready = MeshTexturesSettled(*RequestedSynth->StagedMesh, RequestedSynth->Error);

		if (!RequestedSynth->Error.isEmpty() || Ready)
			FinishSynth(RequestedSynth);
	}
}

void lcAssetLoader::FinishPiece(const std::shared_ptr<PieceRequest>& RequestedPiece)
{
	PieceInfo* Info = RequestedPiece->Info;
	const bool Loaded = RequestedPiece->Error.isEmpty() && (RequestedPiece->StagedMesh || RequestedPiece->EmptyGeometry);
	QString LoadError;

	if (Loaded)
	{
		if (RequestedPiece->SaveCache && RequestedPiece->StagedMesh)
			mLibrary->SaveBuiltPieceCache(RequestedPiece->Source, *RequestedPiece->StagedMesh);

		if (RequestedPiece->StagedMesh)
			Info->SetMesh(RequestedPiece->StagedMesh.release());

		Info->mState = lcPieceInfoState::Loaded;
		mLibrary->ClearPieceLoadError(Info);
	}
	else
	{
		LoadError = RequestedPiece->Error.isEmpty() ? tr("Could not load part %1.").arg(RequestedPiece->Source.FileName) : RequestedPiece->Error;

		Info->SetFailed(LoadError);
	}

	mQueueMutex.lock();
	const auto It = mPieceRequests.find(Info);
	if (It != mPieceRequests.end() && It->second == RequestedPiece)
		mPieceRequests.erase(It);
	RequestedPiece->Terminal = true;
	RequestedPiece->Succeeded = Loaded;
	mResultReady.wakeAll();
	mQueueMutex.unlock();

	mLibrary->ScheduleBufferRepack();
	mPieceNotifications.push_back({ RequestedPiece, std::move(LoadError) });

	QCoreApplication::postEvent(this, new QEvent(lcAssetLoaderCompletionEvent));
}

void lcAssetLoader::FinishSynth(const std::shared_ptr<SynthRequest>& RequestedSynth)
{
	lcPiece* Piece = RequestedSynth->Piece;
	PieceInfo* Info = Piece->mPieceInfo;
	const bool Loaded = RequestedSynth->Error.isEmpty() && RequestedSynth->StagedMesh;

	Piece->SetGeneratedMesh(Loaded ? RequestedSynth->StagedMesh.release() : nullptr);

	if (Loaded)
		mSynthErrors.erase(Piece);
	else
		mSynthErrors.insert_or_assign(Piece, RequestedSynth->Error);

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

void lcAssetLoader::DispatchNotifications()
{
	// Event-handler delivery avoids reentering consumers inside a synchronous readiness wait.
	Q_ASSERT(QThread::currentThread() == thread());

	constexpr int MaxNotificationsPerPump = 32;

	for (int Delivered = 0; Delivered < MaxNotificationsPerPump && !mPieceNotifications.empty(); Delivered++)
	{
		PieceNotification Notification = std::move(mPieceNotifications.front());

		mPieceNotifications.pop_front();

		PieceInfo* Info = Notification.RequestedPiece->Info;

		if (Notification.RequestedPiece->Obsolete || !mLibrary->HasPieceConsumers(Info))
		{
			mLibrary->ReleasePieceLoadHold(Info);

			continue;
		}

		QPointer<lcAssetLoader> LoaderGuard(this);
		QPointer<lcPiecesLibrary> LibraryGuard(mLibrary);

		if (Notification.RequestedPiece->Succeeded)
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

	if (!mPieceNotifications.empty() || !mSynthNotifications.empty())
		QCoreApplication::postEvent(this, new QEvent(lcAssetLoaderCompletionEvent));

	if (!SettledSynths.empty())
		mLibrary->NotifyConsumersChanged();
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

bool lcAssetLoader::WaitForPieceRequest(PieceInfo* Info)
{
	Q_ASSERT(QThread::currentThread() == thread());

	std::shared_ptr<PieceRequest> RequestedPiece;

	{
		QMutexLocker QueueLock(&mQueueMutex);

		const auto It = mPieceRequests.find(Info);

		if (It != mPieceRequests.end())
		{
			RequestedPiece = It->second;

			PromotePiecePriorityLocked(RequestedPiece, lcPieceLoadPriority::Blocking);
		}
		StartWorkersLocked();
	}

	if (!RequestedPiece)
		return Info->mState == lcPieceInfoState::Loaded;

	QOpenGLContext* const PreviousContext = QOpenGLContext::currentContext();
	QSurface* const PreviousSurface = PreviousContext ? PreviousContext->surface() : nullptr;

	for (;;)
	{
		ProcessCompletions();

		if (RequestedPiece->Terminal)
			break;

		QMutexLocker QueueLock(&mQueueMutex);

		if (mStopping || RequestedPiece->Terminal)
			break;

		WaitForResultsLocked();
	}

	if (PreviousContext && PreviousSurface && (QOpenGLContext::currentContext() != PreviousContext || PreviousContext->surface() != PreviousSurface))
		PreviousContext->makeCurrent(PreviousSurface);

	return RequestedPiece->Succeeded;
}

bool lcAssetLoader::WaitForPieces(const std::vector<PieceInfo*>& Pieces)
{
	Q_ASSERT(QThread::currentThread() == thread());

	std::vector<PieceInfo*> Required = Pieces;
	std::sort(Required.begin(), Required.end());

	Required.erase(std::unique(Required.begin(), Required.end()), Required.end());

	for (PieceInfo* Info : Required)
	{
		if (Info->mState != lcPieceInfoState::Unloaded)
			continue;

		if (Info->IsProject() || (Info->IsModel() && !Info->GetModel()->HasDirectGeometry()))
			Info->mState = lcPieceInfoState::Loaded;
		else
			QueuePiece(Info, lcPieceLoadPriority::Blocking);
	}

	{
		QMutexLocker QueueLock(&mQueueMutex);

		for (PieceInfo* Info : Required)
		{
			const auto It = mPieceRequests.find(Info);

			if (It == mPieceRequests.end())
				continue;

			PromotePiecePriorityLocked(It->second, lcPieceLoadPriority::Blocking);
		}

		StartWorkersLocked();
	}

	bool Ready = true;

	for (PieceInfo* Info : Required)
		Ready = WaitForPieceRequest(Info) && Ready;

	return Ready;
}

bool lcAssetLoader::WaitForSynthMeshes(const std::vector<lcPiece*>& Pieces)
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
			RequestedSynth->LoadPriority = lcPieceLoadPriority::Blocking;
			Required.push_back(RequestedSynth);

			if (RequestedSynth->StagedMesh)
				for (const lcMeshLod& Lod : RequestedSynth->StagedMesh->mLods)
					for (int SectionIdx = 0; SectionIdx < Lod.NumSections; SectionIdx++)
					{
						const TextureRequestMap::iterator TextureIt = mTextureRequests.find(Lod.Sections[SectionIdx].Texture);

						if (TextureIt != mTextureRequests.end())
							TextureIt->second->LoadPriority = lcPieceLoadPriority::Blocking;
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

bool lcAssetLoader::WaitForTexture(lcTexture* Texture)
{
	Q_ASSERT(QThread::currentThread() == thread());

	if (Texture->IsReady())
		return true;

	QueueTexture(Texture, lcPieceLoadPriority::Blocking);

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

void lcAssetLoader::WaitForAllWork()
{
	Q_ASSERT(QThread::currentThread() == thread());

	for (;;)
	{
		ProcessCompletions();

		QMutexLocker QueueLock(&mQueueMutex);

		if (mPieceRequests.empty() && mTextureRequests.empty() && mSynthRequests.empty() && mActiveWorkers == 0 && mCompletions.empty())
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
	const bool Pending = !mPieceRequests.empty() || !mTextureRequests.empty() || !mSynthRequests.empty() || !mCompletions.empty();
	mQueueMutex.unlock();

	return Pending;
}

QString lcAssetLoader::GetSynthMeshError(const lcPiece* Piece) const
{
	const std::unordered_map<const lcPiece*, QString>::const_iterator It = mSynthErrors.find(Piece);

	return It != mSynthErrors.end() ? It->second : QString();
}

void lcAssetLoader::RebuildModelPiece(PieceInfo* Info)
{
	Q_ASSERT(QThread::currentThread() == thread());
	Q_ASSERT(Info->IsModel());

	if (Info->IsLoading())
		WaitForPieceRequest(Info);

	Info->ReleaseMesh();
	mLibrary->ClearPieceLoadError(Info);

	if (!Info->GetModel()->HasDirectGeometry())
	{
		Info->mState = lcPieceInfoState::Loaded;
		emit mLibrary->AssetRequestsChanged();
		return;
	}

	Info->mState = lcPieceInfoState::Unloaded;

	QueuePiece(Info, lcPieceLoadPriority::Normal);
}

void lcAssetLoader::InvalidatePiece(PieceInfo* Info)
{
	Q_ASSERT(QThread::currentThread() == thread());

	std::shared_ptr<PieceRequest> Removed;
	bool ReleaseHold = false;

	{
		QMutexLocker QueueLock(&mQueueMutex);

		const auto It = mPieceRequests.find(Info);

		if (It != mPieceRequests.end())
		{
			Removed = It->second;
			Removed->Obsolete = true;
			Removed->Terminal = true;
			Removed->Succeeded = false;
			ReleaseHold = !Removed->AwaitingCompletion;
			mPieceRequests.erase(It);

			mPieceQueue.erase(std::remove(mPieceQueue.begin(), mPieceQueue.end(), Removed), mPieceQueue.end());
		}

		for (PieceNotification& Pending : mPieceNotifications)
			if (Pending.RequestedPiece->Info == Info)
				Pending.RequestedPiece->Obsolete = true;

		mResultReady.wakeAll();
	}

	if (Removed && !Removed->AwaitingCompletion)
		Removed->StagedMesh.reset();

	Info->mState = lcPieceInfoState::Unloaded;
	mLibrary->ClearPieceLoadError(Info);

	// Work awaiting completion retains its load hold until its stale completion arrives.
	if (ReleaseHold)
		mLibrary->ReleasePieceLoadHold(Info);

	if (Removed)
		CancelUnusedTextureRequests();

	emit mLibrary->AssetRequestsChanged();
}

void lcAssetLoader::CancelPieceLoad(PieceInfo* Info)
{
	// Ordinary running builds finish privately; discard them if their last consumer is gone.
	Q_ASSERT(QThread::currentThread() == thread());

	std::shared_ptr<PieceRequest> Removed;
	{
		QMutexLocker QueueLock(&mQueueMutex);

		const auto It = mPieceRequests.find(Info);

		if (It == mPieceRequests.end())
			return;

		const std::shared_ptr<PieceRequest>& RequestedPiece = It->second;

		if (RequestedPiece->AwaitingCompletion)
			return;

		RequestedPiece->Obsolete = true;

		Removed = RequestedPiece;
		Removed->Terminal = true;
		Removed->Succeeded = false;

		Info->mState = lcPieceInfoState::Cancelled;

		mPieceRequests.erase(It);

		mPieceQueue.erase(std::remove(mPieceQueue.begin(), mPieceQueue.end(), Removed), mPieceQueue.end());

		mResultReady.wakeAll();
	}

	Removed->StagedMesh.reset();

	CancelUnusedTextureRequests();

	mLibrary->ReleasePieceLoadHold(Info);
}

void lcAssetLoader::CancelSynthMesh(lcPiece* Piece)
{
	// Remove queued work immediately; running synth builds notice cancellation cooperatively.
	Q_ASSERT(QThread::currentThread() == thread());

	mSynthPieces.erase(Piece);
	mSynthErrors.erase(Piece);

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

		mSynthQueue.erase(std::remove(mSynthQueue.begin(), mSynthQueue.end(), Removed), mSynthQueue.end());
		mResultReady.wakeAll();
	}

	if (!Removed->AwaitingCompletion)
	{
		Removed->StagedMesh.reset();
		CancelUnusedTextureRequests();
	}
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
			if (RequestedTexture->AwaitingCompletion || RequestedTexture->Texture->GetRefCount() > 1)
			{
				++It;
				continue;
			}

			mTextureQueue.erase(std::remove(mTextureQueue.begin(), mTextureQueue.end(), RequestedTexture), mTextureQueue.end());

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
		Texture->mLoadFailureDetails.clear();

		mLibrary->ReleaseTexture(Texture);
	}
}

void lcAssetLoader::RebuildSynthMeshes()
{
	Q_ASSERT(QThread::currentThread() == thread());

	const std::vector<lcPiece*> Pieces(mSynthPieces.begin(), mSynthPieces.end());

	for (lcPiece* Piece : Pieces)
		Piece->RefreshGeneratedMesh();
}

void lcAssetLoader::PauseQueuedWork()
{
	Q_ASSERT(QThread::currentThread() == thread());

	mQueueMutex.lock();
	// Jobs stay queued; workers stop taking them until resume.
	mPaused = true;
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
	for (const PieceRequestMap::value_type& Entry : mPieceRequests)
	{
		const std::shared_ptr<PieceRequest>& RequestedPiece = Entry.second;

		if (!RequestedPiece->StagedMesh)
			continue;

		RequestedPiece->StagedMesh.reset();
		RequestedPiece->Info->mState = lcPieceInfoState::Queued;
		mPieceQueue.push_back(RequestedPiece);
	}

	// Pending generated meshes are restarted after the new stud style is installed.
	for (const SynthRequestMap::value_type& Entry : mSynthRequests)
	{
		const std::shared_ptr<SynthRequest>& RequestedSynth = Entry.second;

		if (!RequestedSynth->StagedMesh)
			continue;

		RequestedSynth->StagedMesh.reset();

		if (std::find(mSynthQueue.begin(), mSynthQueue.end(), RequestedSynth) == mSynthQueue.end())
			mSynthQueue.push_back(RequestedSynth);
	}
}

void lcAssetLoader::ResumeQueuedWork()
{
	Q_ASSERT(QThread::currentThread() == thread());

	QMutexLocker QueueLock(&mQueueMutex);

	for (const std::shared_ptr<PieceRequest>& RequestedPiece : mPieceQueue)
		RequestedPiece->Source = mLibrary->SnapshotPieceSource(RequestedPiece->Info);

	mPaused = false;
	StartWorkersLocked();
}

void lcAssetLoader::CancelAndDrain()
{
	// Shutdown/reload joins obsolete workers and releases holds without requiring GL success.
	Q_ASSERT(QThread::currentThread() == thread());

	const bool WasCancelled = mLibrary->mCancelLoading.exchange(true);
	mLibrary->mPrimitiveLoaded.wakeAll();

	std::deque<std::shared_ptr<PieceRequest>> Cancelled;
	std::deque<std::shared_ptr<TextureRequest>> CancelledTextures;

	{
		QMutexLocker QueueLock(&mQueueMutex);

		mStopping = true;
		mPieceQueue.clear();
		mTextureQueue.clear();
		mSynthQueue.clear();
		mSynthPieces.clear();
		mSynthErrors.clear();

		for (auto It = mPieceRequests.begin(); It != mPieceRequests.end();)
		{
			if (It->second->AwaitingCompletion)
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

				It = mPieceRequests.erase(It);
			}
		}

		for (auto It = mTextureRequests.begin(); It != mTextureRequests.end();)
		{
			if (It->second->AwaitingCompletion)
				++It;
			else
			{
				CancelledTextures.push_back(It->second);

				It = mTextureRequests.erase(It);
			}
		}

		for (SynthRequestMap::iterator It = mSynthRequests.begin(); It != mSynthRequests.end();)
		{
			if (It->second->AwaitingCompletion)
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

	for (const std::shared_ptr<PieceRequest>& RequestedPiece : Cancelled)
		mLibrary->ReleasePieceLoadHold(RequestedPiece->Info);

	for (const std::shared_ptr<TextureRequest>& RequestedTexture : CancelledTextures)
		mLibrary->ReleaseTexture(RequestedTexture->Texture);

	WaitForAllWork();

	while (!mPieceNotifications.empty())
	{
		PieceNotification Notification = std::move(mPieceNotifications.front());

		mPieceNotifications.pop_front();

		mLibrary->ReleasePieceLoadHold(Notification.RequestedPiece->Info);
	}

	while (!mSynthNotifications.empty())
	{
		SynthNotification Notification = std::move(mSynthNotifications.front());

		mSynthNotifications.pop_front();

		mLibrary->ReleasePieceLoadHold(Notification.Info);
	}

	mLibrary->mCancelLoading.store(WasCancelled);
}
