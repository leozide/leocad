#include "lc_global.h"
#include "lc_previewwidget.h"
#include "pieceinf.h"
#include "piece.h"
#include "project.h"
#include "lc_model.h"
#include "camera.h"
#include "lc_library.h"

#include "lc_qglwidget.h"

lcPreviewDockWidget::lcPreviewDockWidget(QMainWindow* Parent)
	: QMainWindow(Parent)
{
	mPreview = new lcPreviewWidget();
	mViewWidget = new lcQGLWidget(nullptr, mPreview);
	setCentralWidget(mViewWidget);
	setMinimumSize(200, 200);

	mLockAction = new QAction(QIcon(":/resources/action_preview_unlocked.png"),tr("Lock Preview"), this);
	mLockAction->setCheckable(true);
	mLockAction->setChecked(false);
	mLockAction->setShortcut(tr("Ctrl+L"));
	connect(mLockAction, SIGNAL(triggered()), this, SLOT(SetPreviewLock()));
	SetPreviewLock();

	mLabel = new QLabel(QString());

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
	if (mPreview->GetActiveModel()->GetPieces().GetSize())
		mPreview->ClearPreview();
	mLabel->setText(QString());
}

void lcPreviewDockWidget::SetPreviewLock()
{
	bool Locked = mLockAction->isChecked();
	if (Locked && mPreview->GetActiveModel()->GetPieces().IsEmpty())
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

lcPreviewWidget::lcPreviewWidget()
	: lcGLWidget(nullptr), mLoader(new Project(true)), mViewSphere(this)
{
	mLoader->SetActiveModel(0);
	mModel = mLoader->GetActiveModel();

	SetDefaultCamera();
}

lcPreviewWidget::~lcPreviewWidget()
{
	if (mCamera && mCamera->IsSimple())
		delete mCamera;

	delete mLoader;
}

bool lcPreviewWidget::SetCurrentPiece(const QString& PartType, int ColorCode)
{
	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	PieceInfo* Info = Library->FindPiece(PartType.toLatin1().constData(), nullptr, false, false);

	if (Info)
	{
		lcModel* ActiveModel = GetActiveModel();
		for (lcPiece* ModelPiece : ActiveModel->GetPieces())
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

		ActiveModel->SelectAllPieces();
		ActiveModel->DeleteSelectedObjects();

		Library->LoadPieceInfo(Info, false, true);
		Library->WaitForLoadQueue();

		lcStep CurrentStep = 1;
		lcPiece* Piece = new lcPiece(nullptr);

		Piece->SetPieceInfo(Info, PartType, false);
		Piece->Initialize(lcMatrix44Identity(), CurrentStep);
		Piece->SetColorCode(ColorCode);

		ActiveModel->SetPreviewPiece(Piece);
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

void lcPreviewWidget::ClearPreview()
{
	delete mLoader;
	mLoader = new Project(true/*IsPreview*/);
	mLoader->SetActiveModel(0);
	mModel = mLoader->GetActiveModel();
	lcGetPiecesLibrary()->UnloadUnusedParts();
	Redraw();
}

void lcPreviewWidget::UpdatePreview()
{
	QString PartType;
	int ColorCode = -1;
	lcModel* ActiveModel = GetActiveModel();
	for (lcPiece* ModelPiece : ActiveModel->GetPieces())
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

void lcPreviewWidget::StartOrbitTracking() // called by viewSphere
{
	mTrackTool = lcTrackTool::OrbitXY;

	UpdateCursor();

	OnButtonDown(lcTrackButton::Left);
}

void lcPreviewWidget::StopTracking(bool Accept)
{
	if (mTrackButton == lcTrackButton::None)
		return;

	lcTool Tool = GetCurrentTool();
	lcModel* ActiveModel = GetActiveModel();

	switch (Tool)
	{
		case lcTool::Select:
			break;

		case lcTool::Pan:
		case lcTool::RotateView:
			ActiveModel->EndMouseTool(Tool, Accept);
			break;

		case lcTool::Count:
		default:
			break;
	}

	mTrackButton = lcTrackButton::None;

	mTrackTool = lcTrackTool::None;

	UpdateCursor();
}

void lcPreviewWidget::OnButtonDown(lcTrackButton TrackButton)
{
	switch (mTrackTool)
	{
		case lcTrackTool::None:
			break;

		case lcTrackTool::Pan:
			StartTracking(TrackButton);
			break;

		case lcTrackTool::OrbitXY:
			StartTracking(TrackButton);
			break;

		default:
			break;
	}
}

void lcPreviewWidget::OnDraw()
{
	if (!mModel)
		return;

	lcPreferences& Preferences = lcGetPreferences();
	const bool DrawInterface = mWidget != nullptr;

	mScene->SetAllowLOD(Preferences.mAllowLOD && mWidget != nullptr);
	mScene->SetLODDistance(Preferences.mMeshLODDistance);

	mScene->Begin(mCamera->mWorldView);

	mScene->SetDrawInterface(DrawInterface);

	mModel->GetScene(mScene.get(), mCamera, false /*HighlightNewParts*/, false/*mFadeSteps*/);

	mScene->End();

	mContext->SetDefaultState();

	mContext->SetViewport(0, 0, mWidth, mHeight);

	DrawBackground();

	mContext->SetProjectionMatrix(GetProjectionMatrix());

	mContext->SetLineWidth(Preferences.mLineWidth);

	mScene->Draw(mContext);

	if (DrawInterface)
	{
		mContext->SetLineWidth(1.0f);

		if (Preferences.mDrawPreviewAxis)
			DrawAxes();

		if (Preferences.mDrawPreviewViewSphere)
			mViewSphere.Draw();
		DrawViewport();
	}

	mContext->ClearResources();
}

void lcPreviewWidget::OnLeftButtonDown()
{
	if (mTrackButton != lcTrackButton::None)
	{
		StopTracking(false);
		return;
	}

	if (mViewSphere.OnLeftButtonDown())
		return;

	lcTrackTool OverrideTool = lcTrackTool::OrbitXY;

	if (OverrideTool != lcTrackTool::None)
	{
		mTrackTool = OverrideTool;
		UpdateCursor();
	}

	OnButtonDown(lcTrackButton::Left);
}

void lcPreviewWidget::OnLeftButtonUp()
{
	StopTracking(mTrackButton == lcTrackButton::Left);

	if (mViewSphere.OnLeftButtonUp()) {
		ZoomExtents();
		return;
	}
}

void lcPreviewWidget::OnMiddleButtonDown()
{
	if (mTrackButton != lcTrackButton::None)
	{
		StopTracking(false);
		return;
	}

#if (QT_VERSION >= QT_VERSION_CHECK(4, 7, 0))
	lcTrackTool OverrideTool = lcTrackTool::None;

	if (OverrideTool != lcTrackTool::None)
	{
		mTrackTool = OverrideTool;
		UpdateCursor();
	}
#endif
	OnButtonDown(lcTrackButton::Middle);
}

void lcPreviewWidget::OnMiddleButtonUp()
{
	StopTracking(mTrackButton == lcTrackButton::Middle);
}

void lcPreviewWidget::OnLeftButtonDoubleClick()
{
	ZoomExtents();
	Redraw();
}

void lcPreviewWidget::OnRightButtonDown()
{
	if (mTrackButton != lcTrackButton::None)
	{
		StopTracking(false);
		return;
	}

	lcTrackTool OverrideTool = lcTrackTool::Pan;

	if (OverrideTool != lcTrackTool::None)
	{
		mTrackTool = OverrideTool;
		UpdateCursor();
	}

	OnButtonDown(lcTrackButton::Middle);
}

void lcPreviewWidget::OnRightButtonUp()
{
	if (mTrackButton != lcTrackButton::None)
		StopTracking(mTrackButton == lcTrackButton::Right);
}

void lcPreviewWidget::OnMouseMove()
{
	lcModel* ActiveModel = GetActiveModel();

	if (!ActiveModel)
		return;

	if (mTrackButton == lcTrackButton::None)
	{
		if (mViewSphere.OnMouseMove())
		{
			lcTrackTool NewTrackTool = mViewSphere.IsDragging() ? lcTrackTool::OrbitXY : lcTrackTool::None;

			if (NewTrackTool != mTrackTool)
			{
				mTrackTool = NewTrackTool;
				UpdateCursor();
			}

			return;
		}

		return;
	}

	const float MouseSensitivity = 0.5f / (21.0f - lcGetPreferences().mMouseSensitivity);

	switch (mTrackTool)
	{
		case lcTrackTool::None:
			break;

		case lcTrackTool::Pan:
		{
			lcVector3 Points[4] =
			{
				lcVector3((float)mMouseX, (float)mMouseY, 0.0f),
				lcVector3((float)mMouseX, (float)mMouseY, 1.0f),
				lcVector3(mMouseDownX, mMouseDownY, 0.0f),
				lcVector3(mMouseDownX, mMouseDownY, 1.0f)
			};

			UnprojectPoints(Points, 4);

			const lcVector3& CurrentStart = Points[0];
			const lcVector3& CurrentEnd = Points[1];
			const lcVector3& MouseDownStart = Points[2];
			const lcVector3& MouseDownEnd = Points[3];
			lcVector3 Center = ActiveModel->GetSelectionOrModelCenter();

			lcVector3 PlaneNormal(mCamera->mPosition - mCamera->mTargetPosition);
			lcVector4 Plane(PlaneNormal, -lcDot(PlaneNormal, Center));
			lcVector3 Intersection, MoveStart;

			if (!lcLineSegmentPlaneIntersection(&Intersection, CurrentStart, CurrentEnd, Plane) || !lcLineSegmentPlaneIntersection(&MoveStart, MouseDownStart, MouseDownEnd, Plane))
			{
				Center = MouseDownStart + lcNormalize(MouseDownEnd - MouseDownStart) * 10.0f;
				Plane = lcVector4(PlaneNormal, -lcDot(PlaneNormal, Center));

				if (!lcLineSegmentPlaneIntersection(&Intersection, CurrentStart, CurrentEnd, Plane) || !lcLineSegmentPlaneIntersection(&MoveStart, MouseDownStart, MouseDownEnd, Plane))
					break;
			}

			ActiveModel->UpdatePanTool(mCamera, MoveStart - Intersection);
			Redraw();
		}
		break;

		case lcTrackTool::OrbitXY:
			ActiveModel->UpdateOrbitTool(mCamera, 0.1f * MouseSensitivity * (mMouseX - mMouseDownX), 0.1f * MouseSensitivity * (mMouseY - mMouseDownY));
			Redraw();
			break;

		default:
			break;
	}
}
