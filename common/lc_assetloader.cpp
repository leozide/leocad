#include "lc_global.h"
#include "lc_assetloader.h"
#include "lc_colors.h"
#include "lc_file.h"
#include "lc_library.h"
#include "lc_mesh.h"
#include "lc_texture.h"
#include "lc_synth.h"
#include "piece.h"

constexpr QEvent::Type lcAssetLoaderCompletionEvent = static_cast<QEvent::Type>(QEvent::User + 1);

// Normal part flow: queue -> build and convert -> finalize sections -> settle textures -> publish -> notify.
// Workers own private build data. Publication and deferred notifications stay on the UI thread.
lcAssetLoader::lcAssetLoader(lcPiecesLibrary* Library)
	: mLibrary(Library)
{
	Q_ASSERT(QThread::currentThread() == Library->thread());
}

lcAssetLoader::~lcAssetLoader()
{
	Q_ASSERT(QThread::currentThread() == thread());

	Shutdown();
}

void lcAssetLoader::RequestPieceLoad(PieceInfo* Info, lcLoadPriority LoadPriority)
{
	Q_ASSERT(QThread::currentThread() == thread());

	if (mStopping)
		return;

	const auto Existing = mPieceRequests.find(Info);

	if (Existing != mPieceRequests.end())
	{
		PromotePiecePriority(Existing->second, LoadPriority);

		return;
	}

	mLibrary->AddPieceReference(Info); // The request owns a separate in-flight hold.

	std::shared_ptr<PieceRequest> RequestedPiece = std::make_shared<PieceRequest>();

	RequestedPiece->Info = Info;
	RequestedPiece->Source = mLibrary->SnapshotPieceSource(Info);
	RequestedPiece->LoadPriority = LoadPriority;
	RequestedPiece->QueuedAt = QDateTime::currentMSecsSinceEpoch();

	mPieceRequests.emplace(Info, RequestedPiece);

	DispatchJobs();

	emit mLibrary->AssetRequestsChanged();
}

void lcAssetLoader::PromotePiecePriority(const std::shared_ptr<PieceRequest>& RequestedPiece, lcLoadPriority LoadPriority)
{
	Q_ASSERT(QThread::currentThread() == thread());

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

void lcAssetLoader::RequestSynthMesh(lcPiece* Piece, PieceInfo* Info, std::shared_ptr<const lcSynthInfo> SynthInfo, std::vector<lcPieceControlPoint> ControlPoints)
{
	Q_ASSERT(QThread::currentThread() == thread());

	CancelSynthMesh(Piece);

	if (!SynthInfo)
		return;

	std::shared_ptr<SynthRequest> RequestedSynth = std::make_shared<SynthRequest>();
	RequestedSynth->Piece = Piece;
	RequestedSynth->Info = Info;
	RequestedSynth->SynthInfo = std::move(SynthInfo);
	RequestedSynth->ControlPoints = std::move(ControlPoints);
	RequestedSynth->LoadPriority = lcLoadPriority::Normal;
	RequestedSynth->QueuedAt = QDateTime::currentMSecsSinceEpoch();

	if (mStopping)
		return;

	mSynthRequests.emplace(Piece, RequestedSynth);

	DispatchJobs();

	emit mLibrary->AssetRequestsChanged();
}

void lcAssetLoader::QueueTextureLoad(lcTexture* Texture, lcFileSource Source, lcLoadPriority LoadPriority)
{
	Q_ASSERT(QThread::currentThread() == thread());

	if (mStopping || Texture->IsReady() || Texture->mState == lcAssetState::Failed)
		return;

	const auto Existing = mTextureRequests.find(Texture);

	if (Existing != mTextureRequests.end())
	{
		if (Existing->second->LoadPriority < LoadPriority)
			Existing->second->LoadPriority = LoadPriority;
		return;
	}

	mLibrary->AddTextureReference(Texture);
	Texture->mState = lcAssetState::Loading;

	std::shared_ptr<TextureRequest> RequestedTexture = std::make_shared<TextureRequest>();

	RequestedTexture->Texture = Texture;
	RequestedTexture->Source = std::move(Source);
	RequestedTexture->LoadPriority = LoadPriority;
	RequestedTexture->QueuedAt = QDateTime::currentMSecsSinceEpoch();

	mTextureRequests.emplace(Texture, RequestedTexture);

	DispatchJobs();
}

void lcAssetLoader::DispatchJobs()
{
	Q_ASSERT(QThread::currentThread() == thread());

	mFutures.erase(std::remove_if(mFutures.begin(), mFutures.end(), [](const QFuture<void>& Future)
	{
		return Future.isFinished();
	}), mFutures.end());

	if (mPaused || mStopping)
		return;

	const int MaxJobs = qMax(1, QThread::idealThreadCount() / 2);

	while (mActiveJobs < MaxJobs)
	{
		const qint64 Now = QDateTime::currentMSecsSinceEpoch();

		const auto Score = [Now](const auto& Item)
		{
			// Each ten seconds of waiting promotes one priority level.
			return qMin<qint64>(static_cast<qint64>(lcLoadPriority::Blocking),
				static_cast<qint64>(Item->LoadPriority) + qMax<qint64>(0, Now - Item->QueuedAt) / 10000);
		};

		const auto Less = [&Score](const auto& Left, const auto& Right)
		{
			return Score(Left) < Score(Right) || (Score(Left) == Score(Right) && Left->QueuedAt > Right->QueuedAt);
		};

		const auto BestQueued = [&Less](const auto& Requests)
		{
			typename std::decay_t<decltype(Requests)>::mapped_type Best;

			for (const auto& [Key, Request] : Requests)
				if (Request->State == RequestState::Queued && (!Best || Less(Best, Request)))
					Best = Request;

			return Best;
		};

		std::shared_ptr<PieceRequest> RequestedPiece = BestQueued(mPieceRequests);
		std::shared_ptr<TextureRequest> RequestedTexture = BestQueued(mTextureRequests);
		std::shared_ptr<SynthRequest> RequestedSynth = BestQueued(mSynthRequests);

		if (RequestedTexture && (!RequestedPiece || !Less(RequestedTexture, RequestedPiece)) &&
			(!RequestedSynth || !Less(RequestedTexture, RequestedSynth)))
		{
			RequestedPiece.reset();
			RequestedSynth.reset();
			RequestedTexture->State = RequestState::AwaitingCompletion;
		}
		else if (RequestedSynth && (!RequestedPiece || !Less(RequestedSynth, RequestedPiece)))
		{
			RequestedPiece.reset();
			RequestedTexture.reset();
			RequestedSynth->State = RequestState::AwaitingCompletion;
		}
		else if (RequestedPiece)
		{
			RequestedTexture.reset();
			RequestedSynth.reset();
			RequestedPiece->State = RequestState::AwaitingCompletion;
		}
		else
			break;

		mActiveJobs++;
		mFutures.emplace_back(QtConcurrent::run([this, RequestedPiece, RequestedTexture, RequestedSynth]()
		{
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

			mCompletionMutex.lock();
			mCompletions.push_back(std::move(Completed));
			mCompletionReady.wakeAll();
			mCompletionMutex.unlock();

			QCoreApplication::postEvent(this, new QEvent(lcAssetLoaderCompletionEvent));
		}));
	}
}

bool lcAssetLoader::ProcessCompletions()
{
	// Bounded UI-thread publication also runs during scoped waits, without public callbacks.
	Q_ASSERT(QThread::currentThread() == thread());

	bool Progress = false;
	constexpr int MaxCompletionsPerPump = 16;

	for (int Processed = 0; Processed < MaxCompletionsPerPump; Processed++)
	{
		Completion Completed;
		std::shared_ptr<TextureRequest> Retry;
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

		{
			QMutexLocker CompletionLock(&mCompletionMutex);

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
					break;

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

		// The build has finished; texture dependencies and retries use no job slot.
		mActiveJobs--;

		if (Completed.RequestedTexture)
			ProcessTextureCompletion(Completed);
		else if (Completed.RequestedSynth)
			ProcessSynthCompletion(Completed);
		else
			ProcessPieceCompletion(Completed);
	}

	DispatchJobs();

	const qint64 Now = QDateTime::currentMSecsSinceEpoch();
	const bool RetryDue = std::any_of(mTextureRequests.begin(), mTextureRequests.end(), [Now](const auto& Entry)
	{
		return Entry.second->RetryAt && Entry.second->RetryAt <= Now;
	});
	mCompletionMutex.lock();
	const bool NeedsPump = !mCompletions.empty() || RetryDue;
	mCompletionMutex.unlock();

	if (NeedsPump)
		QCoreApplication::postEvent(this, new QEvent(lcAssetLoaderCompletionEvent));

	return Progress;
}

void lcAssetLoader::ProcessPieceCompletion(Completion& Completed)
{
	Q_ASSERT(QThread::currentThread() == thread());

	const std::shared_ptr<PieceRequest>& RequestedPiece = Completed.RequestedPiece;
	PieceInfo* Info = RequestedPiece->Info;
	const auto It = mPieceRequests.find(Info);
	const bool Current = It != mPieceRequests.end() && It->second == RequestedPiece;
	const bool Stale = mStopping || !Current;
	RequestedPiece->State = Stale ? RequestState::Cancelled : RequestState::AwaitingPublication;

	const bool HasConsumer = mLibrary->HasPieceConsumers(Info);

	if (Stale || !HasConsumer)
	{
		if (Current)
		{
			mPieceRequests.erase(It);
			mLibrary->PieceLoadCancelled(Info);
		}

		RequestedPiece->State = RequestState::Cancelled;

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
		const bool Ready = ResolveAndQueueMeshTextures(*RequestedPiece->StagedMesh, RequestedPiece->Source,
			RequestedPiece->LoadPriority, RequestedPiece->Error);

		if (RequestedPiece->Error.isEmpty() && !Ready)
			return;
	}

	CompletePieceRequest(RequestedPiece);
}

void lcAssetLoader::ProcessSynthCompletion(Completion& Completed)
{
	Q_ASSERT(QThread::currentThread() == thread());

	const std::shared_ptr<SynthRequest>& RequestedSynth = Completed.RequestedSynth;
	const SynthRequestMap::iterator It = mSynthRequests.find(RequestedSynth->Piece);
	const bool Stale = mStopping || RequestedSynth->Cancelled.load(std::memory_order_relaxed) ||
		It == mSynthRequests.end() || It->second != RequestedSynth;
	if (Stale && It != mSynthRequests.end() && It->second == RequestedSynth)
		mSynthRequests.erase(It);
	RequestedSynth->State = Stale ? RequestState::Cancelled : RequestState::AwaitingPublication;

	if (Stale)
		return;

	if (!Completed.SynthMesh)
	{
		CompleteSynthRequest(RequestedSynth);
		return;
	}

	RequestedSynth->StagedMesh = std::move(Completed.SynthMesh);
	RequestedSynth->StagedMesh->FinalizeSections();

	const bool Ready = ResolveAndQueueMeshTextures(*RequestedSynth->StagedMesh, lcPieceSourceSnapshot(),
		RequestedSynth->LoadPriority, RequestedSynth->Error);

	if (!RequestedSynth->Error.isEmpty() || Ready)
		CompleteSynthRequest(RequestedSynth);
}

void lcAssetLoader::ProcessTextureCompletion(Completion& Completed)
{
	Q_ASSERT(QThread::currentThread() == thread());

	const std::shared_ptr<TextureRequest>& RequestedTexture = Completed.RequestedTexture;
	lcTexture* Texture = RequestedTexture->Texture;
	const auto It = mTextureRequests.find(Texture);
	const bool Stale = mStopping || It == mTextureRequests.end() || It->second != RequestedTexture;
	if (Stale && It != mTextureRequests.end() && It->second == RequestedTexture)
		mTextureRequests.erase(It);
	RequestedTexture->State = Stale ? RequestState::Cancelled : RequestState::AwaitingPublication;

	if (Stale)
	{
		mLibrary->ReleaseTexture(Texture);
		return;
	}

	mLibrary->PublishTexture(Texture, std::move(Completed.TextureResult));

	if (Texture->NeedsUpload())
		UploadTextureRequest(RequestedTexture);
	else
		CompleteTextureRequest(RequestedTexture);
}

bool lcAssetLoader::ResolveAndQueueMeshTextures(lcMesh& Mesh, const lcPieceSourceSnapshot& Source, lcLoadPriority LoadPriority, QString& Error)
{
	Q_ASSERT(QThread::currentThread() == thread());

	for (lcMeshLod& Lod : Mesh.mLods)
		for (int SectionIdx = 0; SectionIdx < Lod.NumSections; SectionIdx++)
		{
			lcMeshSection& Section = Lod.Sections[SectionIdx];

			if (Section.TextureName.isEmpty())
				continue;

			lcFileSource TextureSource;
			Section.Texture = mLibrary->FindTextureDeferred(Section.TextureName.toLatin1().constData(), Source.TextureSearchDirectories, Source.EmbeddedData, TextureSource);

			if (Section.Texture && !Section.Texture->IsReady() && Section.Texture->mState != lcAssetState::Failed)
				QueueTextureLoad(Section.Texture, std::move(TextureSource), LoadPriority);
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
			else if (Texture->mState == lcAssetState::Failed)
				Error = Texture->GetLoadError();
			else if (!Texture->IsReady())
				Ready = false;
		}

	return Ready;
}

void lcAssetLoader::UploadTextureRequest(const std::shared_ptr<TextureRequest>& RequestedTexture)
{
	Q_ASSERT(QThread::currentThread() == thread());

	lcTexture* Texture = RequestedTexture->Texture;

	RequestedTexture->UploadAttempts++;

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
		Texture->mState = lcAssetState::Loading;

		const int Delay = RequestedTexture->UploadAttempts == 1 ? 50 : 200;

		RequestedTexture->RetryAt = QDateTime::currentMSecsSinceEpoch() + Delay;

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

	CompleteTextureRequest(RequestedTexture);
}

void lcAssetLoader::CompleteTextureRequest(const std::shared_ptr<TextureRequest>& RequestedTexture)
{
	Q_ASSERT(QThread::currentThread() == thread());

	const auto It = mTextureRequests.find(RequestedTexture->Texture);
	if (It != mTextureRequests.end() && It->second == RequestedTexture)
		mTextureRequests.erase(It);
	RequestedTexture->State = RequestedTexture->Texture->IsReady() ? RequestState::Succeeded : RequestState::Failed;

	mLibrary->ReleaseTexture(RequestedTexture->Texture);

	FinishPiecesWithSettledTextures();
	FinishSynthsWithSettledTextures();
}

void lcAssetLoader::FinishPiecesWithSettledTextures()
{
	Q_ASSERT(QThread::currentThread() == thread());

	std::vector<std::shared_ptr<PieceRequest>> Waiting;

	for (const PieceRequestMap::value_type& Entry : mPieceRequests)
		if (Entry.second->StagedMesh)
			Waiting.push_back(Entry.second);

	for (const std::shared_ptr<PieceRequest>& RequestedPiece : Waiting)
	{
		const bool Ready = MeshTexturesSettled(*RequestedPiece->StagedMesh, RequestedPiece->Error);

		if (!RequestedPiece->Error.isEmpty() || Ready)
			CompletePieceRequest(RequestedPiece);
	}
}

void lcAssetLoader::FinishSynthsWithSettledTextures()
{
	Q_ASSERT(QThread::currentThread() == thread());

	std::vector<std::shared_ptr<SynthRequest>> Waiting;

	for (const SynthRequestMap::value_type& Entry : mSynthRequests)
		if (Entry.second->StagedMesh)
			Waiting.push_back(Entry.second);

	for (const std::shared_ptr<SynthRequest>& RequestedSynth : Waiting)
	{
		const SynthRequestMap::iterator It = mSynthRequests.find(RequestedSynth->Piece);
		const bool Current = It != mSynthRequests.end() && It->second == RequestedSynth;

		if (!Current)
			continue;

		const bool Ready = MeshTexturesSettled(*RequestedSynth->StagedMesh, RequestedSynth->Error);

		if (!RequestedSynth->Error.isEmpty() || Ready)
			CompleteSynthRequest(RequestedSynth);
	}
}

void lcAssetLoader::CompletePieceRequest(const std::shared_ptr<PieceRequest>& RequestedPiece)
{
	Q_ASSERT(QThread::currentThread() == thread());

	PieceInfo* Info = RequestedPiece->Info;
	QString LoadError = RequestedPiece->Error;

	if (LoadError.isEmpty() && !RequestedPiece->StagedMesh && !RequestedPiece->EmptyGeometry)
		LoadError = tr("Could not load part %1.").arg(RequestedPiece->Source.FileName);

	if (LoadError.isEmpty() && RequestedPiece->SaveCache && RequestedPiece->StagedMesh)
		mLibrary->SaveBuiltPieceCache(RequestedPiece->Source, *RequestedPiece->StagedMesh);

	const bool Loaded = mLibrary->PublishPieceMesh(Info, std::move(RequestedPiece->StagedMesh), LoadError);

	if (!Loaded)
		CancelUnusedTextureRequests();

	const auto It = mPieceRequests.find(Info);
	if (It != mPieceRequests.end() && It->second == RequestedPiece)
		mPieceRequests.erase(It);
	RequestedPiece->State = Loaded ? RequestState::Succeeded : RequestState::Failed;

	mPieceNotifications.push_back({ RequestedPiece, std::move(LoadError) });

	QCoreApplication::postEvent(this, new QEvent(lcAssetLoaderCompletionEvent));
}

void lcAssetLoader::CompleteSynthRequest(const std::shared_ptr<SynthRequest>& RequestedSynth)
{
	Q_ASSERT(QThread::currentThread() == thread());

	lcPiece* Piece = RequestedSynth->Piece;
	PieceInfo* Info = RequestedSynth->Info;
	const bool Loaded = mLibrary->PublishSynthMesh(Piece, std::move(RequestedSynth->StagedMesh), RequestedSynth->Error);

	if (!Loaded)
		CancelUnusedTextureRequests();

	const SynthRequestMap::iterator It = mSynthRequests.find(Piece);

	if (It != mSynthRequests.end() && It->second == RequestedSynth)
		mSynthRequests.erase(It);

	RequestedSynth->State = Loaded ? RequestState::Succeeded : RequestState::Failed;

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

		if (Notification.RequestedPiece->State == RequestState::Cancelled || !mLibrary->HasPieceConsumers(Info))
		{
			mLibrary->ReleasePieceLoadHold(Info);

			continue;
		}

		QPointer<lcAssetLoader> LoaderGuard(this);
		QPointer<lcPiecesLibrary> LibraryGuard(mLibrary);

		if (Notification.RequestedPiece->State == RequestState::Succeeded)
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
	Q_ASSERT(QThread::currentThread() == thread());

	if (Event->type() == lcAssetLoaderCompletionEvent)
	{
		ProcessCompletions();
		DispatchNotifications();

		return true;
	}

	return QObject::event(Event);
}

void lcAssetLoader::WaitForCompletionOrRetry()
{
	Q_ASSERT(QThread::currentThread() == thread());

	qint64 EarliestRetry = -1;

	for (const TextureRequestMap::value_type& Entry : mTextureRequests)
	{
		const qint64 RetryAt = Entry.second->RetryAt;

		if (RetryAt && (EarliestRetry == -1 || RetryAt < EarliestRetry))
			EarliestRetry = RetryAt;
	}

	// Check and wait under the handoff mutex so a worker wakeup cannot be lost.
	QMutexLocker CompletionLock(&mCompletionMutex);

	if (!mCompletions.empty())
		return;

	if (EarliestRetry == -1)
		mCompletionReady.wait(&mCompletionMutex);
	else
	{
		const qint64 Delay = EarliestRetry - QDateTime::currentMSecsSinceEpoch();

		if (Delay > 0)
			mCompletionReady.wait(&mCompletionMutex, static_cast<unsigned long>(Delay));
	}
}

bool lcAssetLoader::WaitForPieceRequest(const std::shared_ptr<PieceRequest>& RequestedPiece)
{
	Q_ASSERT(QThread::currentThread() == thread());

	QOpenGLContext* const PreviousContext = QOpenGLContext::currentContext();
	QSurface* const PreviousSurface = PreviousContext ? PreviousContext->surface() : nullptr;

	for (;;)
	{
		ProcessCompletions();

		if (mStopping || RequestedPiece->IsFinished())
			break;

		WaitForCompletionOrRetry();
	}

	if (PreviousContext && PreviousSurface && (QOpenGLContext::currentContext() != PreviousContext || PreviousContext->surface() != PreviousSurface))
		PreviousContext->makeCurrent(PreviousSurface);

	return RequestedPiece->State == RequestState::Succeeded;
}

bool lcAssetLoader::WaitForPieces(const std::vector<PieceInfo*>& Pieces)
{
	Q_ASSERT(QThread::currentThread() == thread());

	std::vector<PieceInfo*> Required = Pieces;
	std::sort(Required.begin(), Required.end());

	Required.erase(std::unique(Required.begin(), Required.end()), Required.end());

	std::vector<std::shared_ptr<PieceRequest>> Requests;

	for (PieceInfo* Info : Required)
	{
		const auto It = mPieceRequests.find(Info);

		if (It == mPieceRequests.end())
			continue;

		PromotePiecePriority(It->second, lcLoadPriority::Blocking);
		Requests.push_back(It->second);
	}

	DispatchJobs();

	bool Ready = true;

	for (const std::shared_ptr<PieceRequest>& RequestedPiece : Requests)
		Ready = WaitForPieceRequest(RequestedPiece) && Ready;

	return Ready;
}

bool lcAssetLoader::WaitForSynthMeshes(const std::vector<lcPiece*>& Pieces)
{
	Q_ASSERT(QThread::currentThread() == thread());

	std::vector<std::shared_ptr<SynthRequest>> Required;

	for (lcPiece* Piece : Pieces)
	{
		const SynthRequestMap::iterator It = mSynthRequests.find(Piece);

		if (It == mSynthRequests.end())
			continue;

		const std::shared_ptr<SynthRequest>& RequestedSynth = It->second;
		RequestedSynth->LoadPriority = lcLoadPriority::Blocking;
		Required.push_back(RequestedSynth);

		if (RequestedSynth->StagedMesh)
			for (const lcMeshLod& Lod : RequestedSynth->StagedMesh->mLods)
				for (int SectionIdx = 0; SectionIdx < Lod.NumSections; SectionIdx++)
				{
					const TextureRequestMap::iterator TextureIt = mTextureRequests.find(Lod.Sections[SectionIdx].Texture);

					if (TextureIt != mTextureRequests.end())
						TextureIt->second->LoadPriority = lcLoadPriority::Blocking;
				}
	}

	DispatchJobs();

	QOpenGLContext* const PreviousContext = QOpenGLContext::currentContext();
	QSurface* const PreviousSurface = PreviousContext ? PreviousContext->surface() : nullptr;

	bool Ready = true;

	for (const std::shared_ptr<SynthRequest>& RequestedSynth : Required)
	{
		for (;;)
		{
			ProcessCompletions();

			if (mStopping || RequestedSynth->IsFinished())
				break;

			WaitForCompletionOrRetry();
		}

		Ready = RequestedSynth->State == RequestState::Succeeded && Ready;
	}

	if (PreviousContext && PreviousSurface && (QOpenGLContext::currentContext() != PreviousContext || PreviousContext->surface() != PreviousSurface))
		PreviousContext->makeCurrent(PreviousSurface);

	return Ready;
}

bool lcAssetLoader::WaitForTexture(lcTexture* Texture, lcFileSource Source)
{
	Q_ASSERT(QThread::currentThread() == thread());

	if (Texture->IsReady())
		return true;

	QueueTextureLoad(Texture, std::move(Source), lcLoadPriority::Blocking);

	for (;;)
	{
		ProcessCompletions();

		if (Texture->IsReady() || Texture->mState == lcAssetState::Failed)
			break;

		if (mStopping || mTextureRequests.find(Texture) == mTextureRequests.end())
			break;

		WaitForCompletionOrRetry();
	}

	return Texture->IsReady();
}

void lcAssetLoader::WaitForAllWork()
{
	Q_ASSERT(QThread::currentThread() == thread());

	for (;;)
	{
		ProcessCompletions();

		if (mPieceRequests.empty() && mTextureRequests.empty() && mSynthRequests.empty() && mActiveJobs == 0)
			break;

		WaitForCompletionOrRetry();
	}

	for (QFuture<void>& Future : mFutures)
		Future.waitForFinished();

	mFutures.clear();
}

bool lcAssetLoader::HasPendingWork()
{
	Q_ASSERT(QThread::currentThread() == thread());

	return !mPieceRequests.empty() || !mTextureRequests.empty() || !mSynthRequests.empty() || mActiveJobs != 0;
}

void lcAssetLoader::InvalidatePieceLoad(PieceInfo* Info)
{
	Q_ASSERT(QThread::currentThread() == thread());

	for (PieceNotification& Pending : mPieceNotifications)
		if (Pending.RequestedPiece->Info == Info)
			Pending.RequestedPiece->State = RequestState::Cancelled;

	const auto It = mPieceRequests.find(Info);

	if (It != mPieceRequests.end())
		DiscardPieceRequest(It->second);

	emit mLibrary->AssetRequestsChanged();
}

void lcAssetLoader::CancelPieceLoad(PieceInfo* Info)
{
	// Ordinary running builds finish privately; discard them if their last consumer is gone.
	Q_ASSERT(QThread::currentThread() == thread());

	const auto It = mPieceRequests.find(Info);

	if (It == mPieceRequests.end() || It->second->State == RequestState::AwaitingCompletion)
		return;

	mLibrary->PieceLoadCancelled(Info);

	DiscardPieceRequest(It->second);
}

void lcAssetLoader::CancelSynthMesh(lcPiece* Piece)
{
	// Remove queued work immediately; running synth builds notice cancellation cooperatively.
	Q_ASSERT(QThread::currentThread() == thread());

	for (SynthNotification& Pending : mSynthNotifications)
		if (Pending.RequestedSynth->Piece == Piece)
			Pending.RequestedSynth->Cancelled.store(true, std::memory_order_relaxed);

	const SynthRequestMap::iterator It = mSynthRequests.find(Piece);

	if (It == mSynthRequests.end())
		return;

	const std::shared_ptr<SynthRequest> Removed = It->second;
	Removed->Cancelled.store(true, std::memory_order_relaxed);
	const bool HasOutstandingWork = Removed->State == RequestState::AwaitingCompletion;
	Removed->State = RequestState::Cancelled;
	mSynthRequests.erase(It);

	if (!HasOutstandingWork)
	{
		Removed->StagedMesh.reset();
		CancelUnusedTextureRequests();
	}
}

void lcAssetLoader::DiscardPieceRequest(std::shared_ptr<PieceRequest> Request)
{
	Q_ASSERT(QThread::currentThread() == thread());

	const bool HasOutstandingWork = Request->State == RequestState::AwaitingCompletion;

	Request->State = RequestState::Cancelled;
	mPieceRequests.erase(Request->Info);

	// Running builds retain their hold until their stale completion arrives.
	if (!HasOutstandingWork)
	{
		Request->StagedMesh.reset();
		mLibrary->ReleasePieceLoadHold(Request->Info);
	}

	CancelUnusedTextureRequests();
}

void lcAssetLoader::CancelUnusedTextureRequests()
{
	Q_ASSERT(QThread::currentThread() == thread());

	std::vector<std::shared_ptr<TextureRequest>> Cancelled;

	for (auto It = mTextureRequests.begin(); It != mTextureRequests.end();)
	{
		const std::shared_ptr<TextureRequest>& RequestedTexture = It->second;

		// The request owns one reference; all others belong to consumers.
		if (RequestedTexture->State == RequestState::AwaitingCompletion || RequestedTexture->Texture->GetRefCount() > 1)
		{
			++It;
			continue;
		}


		RequestedTexture->State = RequestState::Cancelled;
		Cancelled.push_back(RequestedTexture);

		It = mTextureRequests.erase(It);
	}

	for (const std::shared_ptr<TextureRequest>& RequestedTexture : Cancelled)
	{
		lcTexture* Texture = RequestedTexture->Texture;

		Texture->DiscardDecodedImage();

		mLibrary->ReleaseTexture(Texture);
	}
}

void lcAssetLoader::PauseQueuedWork()
{
	Q_ASSERT(QThread::currentThread() == thread());

	// Pending jobs stay queued until UI-thread dispatch resumes.
	mPaused = true;

	for (;;)
	{
		ProcessCompletions();

		if (mActiveJobs == 0)
			break;

		WaitForCompletionOrRetry();
	}

	// A mesh awaiting textures was built with the previous stud settings.
	// Rebuild it after the settings change instead of publishing stale geometry.
	for (const PieceRequestMap::value_type& Entry : mPieceRequests)
	{
		const std::shared_ptr<PieceRequest>& RequestedPiece = Entry.second;

		if (!RequestedPiece->StagedMesh)
			continue;

		RequestedPiece->StagedMesh.reset();
		RequestedPiece->State = RequestState::Queued;
	}

	// Pending generated meshes are restarted after the new stud style is installed.
	for (const SynthRequestMap::value_type& Entry : mSynthRequests)
	{
		const std::shared_ptr<SynthRequest>& RequestedSynth = Entry.second;

		if (!RequestedSynth->StagedMesh)
			continue;

		RequestedSynth->StagedMesh.reset();
		RequestedSynth->State = RequestState::Queued;
	}
}

void lcAssetLoader::ResumeQueuedWork()
{
	Q_ASSERT(QThread::currentThread() == thread());

	for (const PieceRequestMap::value_type& Entry : mPieceRequests)
		if (Entry.second->State == RequestState::Queued)
			Entry.second->Source = mLibrary->SnapshotPieceSource(Entry.second->Info);

	mPaused = false;
	DispatchJobs();
}

void lcAssetLoader::Shutdown()
{
	// Shutdown/reload joins obsolete workers and releases holds without requiring GL success.
	Q_ASSERT(QThread::currentThread() == thread());

	const bool WasCancelled = mLibrary->mCancelLoading.exchange(true);
	mLibrary->mPrimitiveLoaded.wakeAll();

	std::deque<std::shared_ptr<PieceRequest>> Cancelled;
	std::deque<std::shared_ptr<TextureRequest>> CancelledTextures;

	mStopping = true;

	for (auto It = mPieceRequests.begin(); It != mPieceRequests.end();)
	{
		if (It->second->State == RequestState::AwaitingCompletion)
			++It;
		else
		{
			It->second->State = RequestState::Cancelled;
			mLibrary->PieceLoadCancelled(It->second->Info);

			Cancelled.push_back(It->second);

			It = mPieceRequests.erase(It);
		}
	}

	for (auto It = mTextureRequests.begin(); It != mTextureRequests.end();)
	{
		if (It->second->State == RequestState::AwaitingCompletion)
			++It;
		else
		{
			It->second->State = RequestState::Cancelled;
			CancelledTextures.push_back(It->second);

			It = mTextureRequests.erase(It);
		}
	}

	for (SynthRequestMap::iterator It = mSynthRequests.begin(); It != mSynthRequests.end();)
	{
		if (It->second->State == RequestState::AwaitingCompletion)
		{
			It->second->Cancelled.store(true, std::memory_order_relaxed);
			++It;
		}
		else
		{
			It->second->State = RequestState::Cancelled;
			It = mSynthRequests.erase(It);
		}
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
