#include "lc_global.h"
#include "lc_previewwidget.h"
#include "pieceinf.h"
#include "piece.h"
#include "project.h"
#include "lc_model.h"
#include "lc_library.h"
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
	connect(mLockAction, SIGNAL(triggered()), this, SLOT(SetPreviewLock()));
	SetPreviewLock();

	mLabel = new QLabel();

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

bool lcPreviewDockWidget::SetCurrentPiece(const QString& PartType, int ColorCode)
{
	if (mLockAction->isChecked())
		return true;

	mLabel->setText(tr("Loading..."));
	if (mPreview->SetCurrentPiece(PartType, ColorCode))
	{
		mLabel->setText(mPreview->GetDescription());
		return true;
	}
	return false;
}

void lcPreviewDockWidget::UpdatePreview()
{
	mPreview->UpdatePreview();
}

void lcPreviewDockWidget::ClearPreview()
{
	if (mPreview->GetModel()->GetPieces().GetSize())
		mPreview->ClearPreview();

	mLabel->setText(QString());
}

void lcPreviewDockWidget::SetPreviewLock()
{
	bool Locked = mLockAction->isChecked();

	if (Locked && mPreview->GetModel()->GetPieces().IsEmpty())
	{
		mLockAction->setChecked(false);
		return;
	}

	QIcon LockIcon(Locked ? ":/resources/action_preview_locked.png" : ":/resources/action_preview_unlocked.png");
	QString State(Locked ? tr("Unlock") : tr("Lock"));
	QString StatusTip(tr("%1 the preview display to %2 updates").arg(State).arg(Locked ? "enable" : "disable"));

	mLockAction->setToolTip(tr("%1 Preview").arg(State));
	mLockAction->setIcon(LockIcon);
	mLockAction->setStatusTip(StatusTip);
}

lcPreview::lcPreview()
	: lcView(lcViewType::Preview, nullptr), mLoader(new Project(true))
{
	mLoader->SetActiveModel(0);
	mModel = mLoader->GetActiveModel();
}

bool lcPreview::SetCurrentPiece(const QString& PartType, int ColorCode)
{
	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	PieceInfo* Info = Library->FindPiece(PartType.toLatin1().constData(), nullptr, false, false);

	if (Info)
	{
		for (lcPiece* ModelPiece : mModel->GetPieces())
		{
			if (Info == ModelPiece->mPieceInfo)
			{
				int ModelColorCode = ModelPiece->mColorCode;
				if (ModelColorCode == ColorCode)
					return true;
			}
		}

		mIsModel = Info->IsModel();
		mDescription = Info->m_strDescription;

		mModel->SelectAllPieces();
		mModel->DeleteSelectedObjects();

		Library->LoadPieceInfo(Info, false, true);
		Library->WaitForLoadQueue();

		lcStep CurrentStep = 1;
		lcPiece* Piece = new lcPiece(nullptr);

		Piece->SetPieceInfo(Info, PartType, false);
		Piece->Initialize(lcMatrix44Identity(), CurrentStep);
		Piece->SetColorCode(ColorCode);

		mModel->SetPreviewPiece(Piece);
	}
	else
	{
		QString ModelPath = QString("%1/%2").arg(QDir::currentPath()).arg(PartType);

		if (!mLoader->Load(ModelPath))
		{
			QMessageBox::warning(nullptr, QMessageBox::tr("Error"), QMessageBox::tr("Failed to load '%1'.").arg(ModelPath));
			return false;
		}

		mLoader->SetActiveModel(0);
		lcGetPiecesLibrary()->RemoveTemporaryPieces();
		mModel = mLoader->GetActiveModel();
		if (!mModel->GetProperties().mDescription.isEmpty())
			mDescription = mModel->GetProperties().mDescription;
		else
			mDescription = PartType;
		mIsModel = true;
	}

	ZoomExtents();

	return true;
}

void lcPreview::ClearPreview()
{
	mLoader = std::unique_ptr<Project>(new Project(true/*IsPreview*/));
	mLoader->SetActiveModel(0);
	mModel = mLoader->GetActiveModel();
	lcGetPiecesLibrary()->UnloadUnusedParts();
	Redraw();
}

void lcPreview::UpdatePreview()
{
	QString PartType;
	int ColorCode = -1;

	for (lcPiece* ModelPiece : mModel->GetPieces())
	{
		if (ModelPiece->mPieceInfo)
		{
			PartType = ModelPiece->mPieceInfo->mFileName;
			ColorCode = ModelPiece->mColorCode;
			break;
		}
	}

	ClearPreview();

	if (!PartType.isEmpty() && ColorCode > -1)
		SetCurrentPiece(PartType, ColorCode);
}
