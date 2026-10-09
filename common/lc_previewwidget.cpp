#include "lc_global.h"
#include "lc_previewwidget.h"
#include "pieceinf.h"
#include "piece.h"
#include "project.h"
#include "lc_model.h"
#include "lc_library.h"
#include "lc_application.h"
#include "lc_viewwidget.h"
#include "lc_view.h"

lcPreviewDockWidget::lcPreviewDockWidget(QMainWindow* Parent)
	: QMainWindow(Parent)
{
	mPreview = new lcPreview();
	mViewWidget = new lcViewWidget(nullptr, mPreview);
	setCentralWidget(mViewWidget);
	setMinimumSize(200, 200);

	mLockAction = new QAction(QIcon(":/resources/action_preview_unlocked.png"),tr("Lock Preview"), this);
	mLockAction->setCheckable(true);
	mLockAction->setChecked(false);
	mLockAction->setShortcut(tr("Ctrl+L"));
	connect(mLockAction, &QAction::triggered, this, &lcPreviewDockWidget::SetPreviewLock);
	SetPreviewLock();

	mLabel = new QLabel();

	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	connect(Library, &lcPiecesLibrary::PartLoaded, this, [this](PieceInfo*)
	{
		if (mAssetRefreshPending)
			RefreshPendingAssets();
	});
	connect(Library, &lcPiecesLibrary::PartLoadFailed, this, [this](PieceInfo*, const QString&)
	{
		if (mAssetRefreshPending)
			RefreshPendingAssets();
	});
	connect(Library, &lcPiecesLibrary::GeneratedMeshSettled, this, [this](PieceInfo*)
	{
		if (mAssetRefreshPending)
			RefreshPendingAssets();
	});

	mToolBar = addToolBar(tr("Toolbar"));
	mToolBar->setObjectName("Toolbar");
	mToolBar->setStatusTip(tr("Preview Toolbar"));
	mToolBar->setMovable(false);
	mToolBar->addAction(mLockAction);
	mToolBar->addSeparator();
	mToolBar->addWidget(mLabel);
	if (mToolBar->isHidden())
		mToolBar->show();
}

bool lcPreviewDockWidget::SetCurrentPiece(PieceInfo* Info, int ColorCode)
{
	if (mLockAction->isChecked())
		return true;

	mAssetRefreshPending = false;
	mRefitAfterAssetLoad = true;
	mLabel->setText(tr("Loading..."));

	if (mPreview->SetCurrentPiece(Info, ColorCode))
	{
		RefreshPendingAssets();
		return true;
	}

	mViewWidget->show();
	mLabel->setText(tr("Preview unavailable"));

	return false;
}

void lcPreviewDockWidget::UpdatePreview()
{
	mAssetRefreshPending = false;
	mRefitAfterAssetLoad = true;
	mPreview->UpdatePreview();
	RefreshPendingAssets();
}

void lcPreviewDockWidget::RefreshDescription()
{
	mPreview->RefreshDescription();
	mLabel->setText(mPreview->GetDescription());
}

void lcPreviewDockWidget::RefreshModel(const lcModel* Model)
{
	if (mLockAction->isChecked() || mRefreshingModel)
		return;

	const std::vector<PieceInfo*> Required = mPreview->GetModel()->GetRequiredPieces();

	if (std::find(Required.begin(), Required.end(), Model->GetPieceInfo()) == Required.end())
		return;

	mRefreshingModel = true;

	if (!mAssetRefreshPending)
		mRefitAfterAssetLoad = false;

	lcPiecesLibrary* Library = lcGetPiecesLibrary();

	for (PieceInfo* Info : Required)
	{
		if (Info->mState != lcPieceInfoState::Unloaded)
			continue;

		Library->LoadPieceInfo(Info, lcPieceLoadFlag::Visible);
		Library->ReleasePieceInfo(Info);
	}

	std::vector<lcModel*> UpdatedModels;
	mPreview->GetModel()->UpdatePieceInfo(UpdatedModels);

	RefreshPendingAssets();

	mRefreshingModel = false;
}

void lcPreviewDockWidget::RefreshAfterAssetChange(const std::vector<PieceInfo*>& ChangedPieces)
{
	if (mPreview->GetModel()->GetPieces().empty())
		return;

	const std::vector<PieceInfo*> Required = mPreview->GetModel()->GetRequiredPieces();
	bool Affected = std::any_of(Required.begin(), Required.end(), [&ChangedPieces](PieceInfo* Info)
	{
		return std::find(ChangedPieces.begin(), ChangedPieces.end(), Info) != ChangedPieces.end();
	});

	if (!Affected)
	{
		for (const std::unique_ptr<lcPiece>& Piece : mPreview->GetModel()->GetPieces())
		{
			if (std::find(ChangedPieces.begin(), ChangedPieces.end(), Piece->mPieceInfo) != ChangedPieces.end())
			{
				Affected = true;
				break;
			}
		}
	}

	if (!Affected)
		return;

	RefreshPendingAssets();
}

void lcPreviewDockWidget::RefreshPendingAssets()
{
	const std::vector<PieceInfo*> Required = mPreview->GetModel()->GetRequiredPieces();

	for (const PieceInfo* Info : Required)
	{
		if (Info->mState == lcPieceInfoState::Failed || Info->mState == lcPieceInfoState::Cancelled)
		{
			mAssetRefreshPending = false;
			mViewWidget->hide();
			mLabel->setText(tr("Preview unavailable"));
			return;
		}

		if (Info->mState != lcPieceInfoState::Loaded)
		{
			mAssetRefreshPending = true;
			mViewWidget->hide();
			mLabel->setText(tr("Loading..."));
			return;
		}
	}

	for (const lcPiece* Piece : mPreview->GetModel()->GetRequiredSynthPieces())
	{
		if (Piece->IsGeneratedMeshPending())
		{
			mAssetRefreshPending = true;
			mViewWidget->hide();
			mLabel->setText(tr("Loading..."));
			return;
		}

		if (!Piece->HasGeneratedMesh())
		{
			mAssetRefreshPending = false;
			mViewWidget->hide();
			mLabel->setText(tr("Preview unavailable"));
			return;
		}
	}

	if (mAssetRefreshPending && mRefitAfterAssetLoad)
		mPreview->ZoomExtents(lcGeometryBoundsMode::IncludeFallback, false);

	mAssetRefreshPending = false;
	mRefitAfterAssetLoad = true;
	mViewWidget->show();
	mPreview->RefreshDescription();
	mPreview->Redraw();
	mLabel->setText(mPreview->GetDescription());
}

void lcPreviewDockWidget::RebindPieceInfo(PieceInfo* Previous, PieceInfo* Replacement)
{
	int ColorCode = -1;

	for (const std::unique_ptr<lcPiece>& Piece : mPreview->GetModel()->GetPieces())
	{
		if (Piece->mPieceInfo == Previous)
		{
			ColorCode = Piece->GetColorCode();
			break;
		}
	}

	if (ColorCode == -1)
		return;

	mAssetRefreshPending = false;
	mRefitAfterAssetLoad = true;

	if (mPreview->SetCurrentPiece(Replacement, ColorCode))
		RefreshPendingAssets();
	else
	{
		mViewWidget->show();
		mLabel->setText(tr("Preview unavailable"));
	}
}

void lcPreviewDockWidget::ClearPreview()
{
	mAssetRefreshPending = false;
	mRefitAfterAssetLoad = true;
	mViewWidget->show();
	if (mPreview->GetModel()->GetPieces().size())
		mPreview->ClearPreview();

	mLabel->setText(QString());
}

void lcPreviewDockWidget::SetPreviewLock()
{
	bool Locked = mLockAction->isChecked();

	if (Locked && mPreview->GetModel()->GetPieces().empty())
	{
		mLockAction->setChecked(false);
		return;
	}

	QIcon LockIcon(Locked ? ":/resources/action_preview_locked.png" : ":/resources/action_preview_unlocked.png");
	QString StatusTip(Locked
		? tr("Unlock the preview display to enable updates")
		: tr("Lock the preview display to disable updates"));

	mLockAction->setToolTip(Locked ? tr("Unlock Preview") : tr("Lock Preview"));
	mLockAction->setIcon(LockIcon);
	mLockAction->setStatusTip(StatusTip);
}

lcPreview::lcPreview()
	: lcView(lcViewType::Preview, nullptr), mLoader(new Project(true))
{
	mLoader->SetActiveModel(0, false);
	mModel = mLoader->GetActiveModel();
}

bool lcPreview::SetCurrentPiece(PieceInfo* Info, int ColorCode)
{
	if (!Info)
	{
		ClearPreview();
		return false;
	}

	for (const std::unique_ptr<lcPiece>& ModelPiece : mModel->GetPieces())
	{
		if (Info == ModelPiece->mPieceInfo)
		{
			int ModelColorCode = ModelPiece->GetColorCode();

			if (ModelColorCode == ColorCode)
			{
			RefreshDescription();
				return true;
			}
		}
	}

	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	bool Ready = Library->LoadPieceInfo(Info, lcPieceLoadFlag::Wait | lcPieceLoadFlag::Visible);

	if (Ready && Info->IsModel())
		Ready = static_cast<bool>(Info->GetModel()->EnsureAssetsReady());
	else if (Ready && Info->IsProject())
		Ready = static_cast<bool>(Info->GetProject()->EnsureAssetsReady());

	if (!Ready)
	{
		Library->ReleasePieceInfo(Info);
		ClearPreview();

		return false;
	}

	mIsModel = Info->IsModel();
	mDescription = Info->m_strDescription;

	mModel->SelectAllPiecesAction();
	mModel->DeleteSelectedObjects();
	mModel->SetPreviewPieceInfo(Info, lcGetColorIndex(ColorCode));

	std::vector<lcModel*> UpdatedModels;

	mModel->UpdatePieceInfo(UpdatedModels);

	Library->ReleasePieceInfo(Info);
	ZoomExtents(lcGeometryBoundsMode::IncludeFallback, true);

	return true;
}

void lcPreview::ClearPreview()
{
	mDescription.clear();
	mIsModel = false;
	mLoader = std::unique_ptr<Project>(new Project(true/*IsPreview*/));
	mLoader->SetActiveModel(0, false);
	mModel = mLoader->GetActiveModel();

	lcPiecesLibrary* Library = lcGetPiecesLibrary();

	Library->RemoveTemporaryPieces();
	Library->UnloadUnusedParts();
	Redraw();
}

void lcPreview::UpdatePreview()
{
	PieceInfo* Info = nullptr;
	int ColorCode = -1;

	for (const std::unique_ptr<lcPiece>& ModelPiece : mModel->GetPieces())
	{
		if (ModelPiece->mPieceInfo)
		{
			Info = ModelPiece->mPieceInfo;
			ColorCode = ModelPiece->GetColorCode();
			break;
		}
	}

	lcPiecesLibrary* Library = lcGetPiecesLibrary();

	if (Info)
		Library->AddPieceReference(Info);

	ClearPreview();

	if (Info && ColorCode > -1)
		SetCurrentPiece(Info, ColorCode);

	if (Info)
		Library->ReleasePieceInfo(Info);
}

void lcPreview::RefreshDescription()
{
	for (const std::unique_ptr<lcPiece>& Piece : mModel->GetPieces())
	{
		if (!Piece->mPieceInfo)
			continue;

		mDescription = QString::fromLatin1(Piece->mPieceInfo->m_strDescription);
		return;
	}

	mDescription.clear();
}
