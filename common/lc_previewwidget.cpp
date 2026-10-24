#include "lc_global.h"
#include "lc_previewwidget.h"
#include "lc_piecedata.h"
#include "piece.h"
#include "project.h"
#include "lc_model.h"
#include "lc_library.h"
#include "lc_application.h"
#include "lc_viewwidget.h"
#include "lc_view.h"
#include "lc_mainwindow.h"

lcPreviewDockWidget::lcPreviewDockWidget(QMainWindow* Parent)
	: QMainWindow(Parent)
{
	mPreview = new lcPreview();
	mViewWidget = new lcViewWidget(nullptr, mPreview);
	setCentralWidget(mViewWidget);
	mViewWidget->setAccessibleName(tr("Piece Preview"));
	mViewWidget->setToolTip(tr("Inspect the previewed piece without placing it in the model."));
	setMinimumSize(200, 200);

	mLockAction = new QAction(QIcon(":/resources/action_preview_unlocked.png"),tr("Lock Preview"), this);
	mLockAction->setWhatsThis(tr("<p><b>Lock Preview</b></p><p>Keep the current piece in the preview while you browse other pieces. Unlock Preview allows the display to update again. An empty preview cannot be locked.</p>"));
	mLockAction->setCheckable(true);
	mLockAction->setChecked(false);
	mLockAction->setShortcut(tr("Ctrl+L"));
	connect(mLockAction, &QAction::triggered, this, &lcPreviewDockWidget::SetPreviewLock);
	SetPreviewLock();

	mLabel = new QLabel();

	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	connect(Library, &lcPiecesLibrary::PartLoaded, this, [this](lcPieceData*)
	{
		if (mAssetRefreshPending)
			RefreshPendingAssets();
	});
	connect(Library, &lcPiecesLibrary::PartLoadFailed, this, [this](lcPieceData*, const QString&)
	{
		if (mAssetRefreshPending)
			RefreshPendingAssets();
	});
	connect(Library, &lcPiecesLibrary::GeneratedMeshSettled, this, [this](lcPieceData*)
	{
		if (mAssetRefreshPending)
			RefreshPendingAssets();
	});

	mToolBar = addToolBar(tr("Toolbar"));
	mToolBar->setObjectName("Toolbar");
	mToolBar->setStatusTip(tr("Preview Toolbar"));
	mToolBar->setWhatsThis(mLockAction->whatsThis());
	mToolBar->setMovable(false);
	mToolBar->addAction(mLockAction);
	mToolBar->widgetForAction(mLockAction)->setWhatsThis(mLockAction->whatsThis());
	mToolBar->addSeparator();
	mToolBar->addWidget(mLabel);

	QWidget* Spacer = new QWidget(mToolBar);
	Spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	mToolBar->addWidget(Spacer);

	QToolButton* OptionsButton = new QToolButton(mToolBar);
	OptionsButton->setIcon(QIcon(":/resources/gear_in.png"));
	OptionsButton->setToolTip(tr("Configure the pieces preview."));
	OptionsButton->setAccessibleName(tr("Pieces Preview Options"));
	OptionsButton->setPopupMode(QToolButton::InstantPopup);
	mToolBar->addWidget(OptionsButton);

	QMenu* OptionsMenu = new QMenu(this);
	OptionsButton->setMenu(OptionsMenu);
	connect(OptionsMenu, &QMenu::aboutToShow, this, &lcPreviewDockWidget::OptionsMenuAboutToShow);

	if (mToolBar->isHidden())
		mToolBar->show();
}

void lcPreviewDockWidget::OptionsMenuAboutToShow()
{
	QMenu* Menu = qobject_cast<QMenu*>(sender());
	Menu->clear();

	const lcPreferences& Preferences = lcGetPreferences();

	QAction* ViewSphere = Menu->addAction(tr("View Sphere"), this, []()
	{
		gMainWindow->HandleCommand(LC_VIEW_TOGGLE_PREVIEW_VIEW_SPHERE);
	});
	ViewSphere->setCheckable(true);
	ViewSphere->setChecked(Preferences.mPreviewViewSphereEnabled);

	QAction* AxisIcon = Menu->addAction(tr("Axis Icon"), this, []()
	{
		gMainWindow->HandleCommand(LC_VIEW_TOGGLE_PREVIEW_AXIS_ICON);
	});
	AxisIcon->setCheckable(true);
	AxisIcon->setChecked(Preferences.mDrawPreviewAxis);

	Menu->addSeparator();

	gMainWindow->AddRenderTargetActions(Menu, lcRenderTarget::Preview);
}

bool lcPreviewDockWidget::SetCurrentPiece(lcPieceData* PieceData, int ColorCode)
{
	if (mLockAction->isChecked())
		return true;

	mAssetRefreshPending = false;
	mRefitAfterAssetLoad = true;
	mLabel->setText(tr("Loading..."));

	if (mPreview->SetCurrentPiece(PieceData, ColorCode))
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

	const std::vector<lcPieceDataRef> RequiredPieceData = mPreview->GetModel()->GetRequiredPieceData();

	if (std::find(RequiredPieceData.begin(), RequiredPieceData.end(), Model->GetPieceData()) == RequiredPieceData.end())
		return;

	mRefreshingModel = true;

	if (!mAssetRefreshPending)
		mRefitAfterAssetLoad = false;

	lcPiecesLibrary* Library = lcGetPiecesLibrary();

	for (const lcPieceDataRef& RequiredData : RequiredPieceData)
	{
		if (RequiredData->mState != lcAssetState::Unloaded)
			continue;

		Library->RequestPieceDataLoad(RequiredData.Get(), lcLoadPriority::Normal);
	}

	std::vector<lcModel*> UpdatedModels;
	mPreview->GetModel()->UpdatePieceData(UpdatedModels);

	RefreshPendingAssets();

	mRefreshingModel = false;
}

void lcPreviewDockWidget::RefreshAfterAssetChange(const std::vector<lcPieceData*>& ChangedPieces)
{
	if (mPreview->GetModel()->GetPieces().empty())
		return;

	const std::vector<lcPieceDataRef> RequiredPieceData = mPreview->GetModel()->GetRequiredPieceData();
	bool Affected = std::any_of(RequiredPieceData.begin(), RequiredPieceData.end(), [&ChangedPieces](const lcPieceDataRef& RequiredData)
	{
		return std::find(ChangedPieces.begin(), ChangedPieces.end(), RequiredData) != ChangedPieces.end();
	});

	if (!Affected)
	{
		for (const std::unique_ptr<lcPiece>& Piece : mPreview->GetModel()->GetPieces())
		{
			if (std::find(ChangedPieces.begin(), ChangedPieces.end(), Piece->mPieceData) != ChangedPieces.end())
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
	const std::vector<lcPieceDataRef> RequiredPieceData = mPreview->GetModel()->GetRequiredPieceData();

	for (const lcPieceDataRef& RequiredData : RequiredPieceData)
	{
		if (RequiredData->mState == lcAssetState::Failed || RequiredData->mState == lcAssetState::Unloaded)
		{
			mAssetRefreshPending = false;
			mViewWidget->hide();
			mLabel->setText(tr("Preview unavailable"));
			return;
		}

		if (RequiredData->mState != lcAssetState::Loaded)
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

void lcPreviewDockWidget::RebindPieceData(lcPieceData* Previous, lcPieceData* Replacement)
{
	int ColorCode = -1;

	for (const std::unique_ptr<lcPiece>& Piece : mPreview->GetModel()->GetPieces())
	{
		if (Piece->mPieceData == Previous)
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

bool lcPreview::SetCurrentPiece(lcPieceData* PieceData, int ColorCode)
{
	if (!PieceData)
	{
		ClearPreview();
		return false;
	}

	for (const std::unique_ptr<lcPiece>& ModelPiece : mModel->GetPieces())
	{
		if (PieceData == ModelPiece->mPieceData)
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
	const lcPieceDataRef PieceDataRef(PieceData);

	bool Ready = Library->WaitForPieceData(PieceDataRef);

	if (Ready && PieceData->IsModel())
		Ready = static_cast<bool>(PieceData->GetModel()->EnsureAssetsReady());
	else if (Ready && PieceData->IsProject())
		Ready = static_cast<bool>(PieceData->GetProject()->EnsureAssetsReady());

	if (!Ready)
	{
		ClearPreview();

		return false;
	}

	mIsModel = PieceData->IsModel();
	mDescription = PieceData->m_strDescription;

	mModel->SelectAllPiecesAction();
	mModel->DeleteSelectedObjects();
	mModel->SetPreviewPieceData(PieceData, lcGetColorIndex(ColorCode));

	std::vector<lcModel*> UpdatedModels;

	mModel->UpdatePieceData(UpdatedModels);

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

	Library->RemoveTemporaryPieceData();
	Redraw();
}

void lcPreview::UpdatePreview()
{
	lcPieceData* PieceData = nullptr;
	int ColorCode = -1;

	for (const std::unique_ptr<lcPiece>& ModelPiece : mModel->GetPieces())
	{
		if (ModelPiece->mPieceData)
		{
			PieceData = ModelPiece->mPieceData.Get();
			ColorCode = ModelPiece->GetColorCode();
			break;
		}
	}

	lcPieceDataRef PreviousPieceData(PieceData);

	ClearPreview();

	if (PieceData && ColorCode > -1)
		SetCurrentPiece(PieceData, ColorCode);

}

void lcPreview::RefreshDescription()
{
	for (const std::unique_ptr<lcPiece>& Piece : mModel->GetPieces())
	{
		if (!Piece->mPieceData)
			continue;

		mDescription = QString::fromLatin1(Piece->mPieceData->m_strDescription);
		return;
	}

	mDescription.clear();
}
