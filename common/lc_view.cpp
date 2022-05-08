#include "lc_global.h"
#include "lc_view.h"
#include "lc_viewwidget.h"
#include <stdlib.h>
#include "lc_mainwindow.h"
#include "camera.h"
#include "texfont.h"
#include "lc_texture.h"
#include "piece.h"
#include "pieceinf.h"
#include "lc_synth.h"
#include "lc_scene.h"
#include "lc_context.h"
#include "lc_viewmanipulator.h"
#include "lc_viewsphere.h"
#include "lc_findreplacewidget.h"

lcFindReplaceParams lcView::mFindReplaceParams;
lcFindReplaceWidget* lcView::mFindWidget;

lcView* lcView::mLastFocusedView;
std::vector<lcView*> lcView::mViews;

lcView::lcView(lcViewType ViewType, lcModel* Model)
	: mViewType(ViewType), mScene(new lcScene()), mModel(Model)
{
	mContext = new lcContext();
	mViews.push_back(this);

	mViewManipulator = std::unique_ptr<lcViewManipulator>(new lcViewManipulator(this));
	mViewSphere = std::unique_ptr<lcViewSphere>(new lcViewSphere(this));
	memset(mGridSettings, 0, sizeof(mGridSettings));

	mDragState = lcDragState::None;
	mTrackToolFromOverlay = false;

	lcView* ActiveView = gMainWindow ? gMainWindow->GetActiveView() : nullptr;

	if (ActiveView)
		SetCamera(ActiveView->mCamera, false);
	else
	{
		mCamera = new lcCamera(true);
		mCamera->SetViewpoint(lcViewpoint::Home);
	}
}

lcView::~lcView()
{
	mContext->DestroyVertexBuffer(mGridBuffer);

	if (gMainWindow && mViewType == lcViewType::View)
		gMainWindow->RemoveView(this);

	if (mCamera && mCamera->IsSimple())
		delete mCamera;

	mViews.erase(std::find(mViews.begin(), mViews.end(), this));

	if (mLastFocusedView == this)
		mLastFocusedView = nullptr;

	if (mDeleteContext)
		delete mContext;
}

std::vector<lcView*> lcView::GetModelViews(const lcModel* Model)
{
	std::vector<lcView*> Views;

	for (lcView* View : mViews)
		if (View->GetModel() == Model)
			Views.push_back(View);

	return Views;
}

void lcView::UpdateProjectViews(const Project* Project)
{
	for (lcView* View : mViews)
	{
		const lcModel* ViewModel = View->GetActiveModel();

		if (ViewModel && ViewModel->GetProject() == Project)
			View->Redraw();
	}
}

void lcView::UpdateAllViews()
{
	for (lcView* View : mViews)
		View->Redraw();
}

void lcView::MakeCurrent()
{
	mContext->MakeCurrent();
}

void lcView::Redraw()
{
	if (mWidget)
		mWidget->update();
}

void lcView::SetOffscreenContext()
{
	mContext->SetOffscreenContext();
}

void lcView::SetFocus(bool Focus)
{
	if (Focus)
	{
		if (mLastFocusedView != this)
		{
			delete mFindWidget;
			mFindWidget = nullptr;

			mLastFocusedView = this;
		}

		emit FocusReceived();
	}
}

void lcView::SetMousePosition(int MouseX, int MouseY)
{
	mMouseX = MouseX;
	mMouseY = MouseY;
}

void lcView::SetMouseModifiers(Qt::KeyboardModifiers MouseModifiers)
{
	mMouseModifiers = MouseModifiers;
}

lcModel* lcView::GetActiveModel() const
{
	return !mActiveSubmodelInstance ? mModel : mActiveSubmodelInstance->mPieceInfo->GetModel();
}

void lcView::SetTopSubmodelActive()
{
	lcModel* ActiveModel = GetActiveModel();

	if (mActiveSubmodelInstance)
	{
		ActiveModel->SetActive(false);
		mActiveSubmodelInstance = nullptr;
	}

	GetActiveModel()->UpdateInterface();
}

void lcView::SetSelectedSubmodelActive()
{
	lcModel* ActiveModel = GetActiveModel();
	lcObject* Object = ActiveModel->GetFocusObject();

	if (mActiveSubmodelInstance)
	{
		ActiveModel->SetActive(false);
		mActiveSubmodelInstance = nullptr;
	}

	if (Object && Object->IsPiece())
	{
		lcPiece* Piece = (lcPiece*)Object;

		if (Piece->mPieceInfo->IsModel())
		{
			mActiveSubmodelTransform = lcMatrix44Identity();
			mModel->GetPieceWorldMatrix(Piece, mActiveSubmodelTransform);
			mActiveSubmodelInstance = Piece;
			ActiveModel = mActiveSubmodelInstance->mPieceInfo->GetModel();
			ActiveModel->SetActive(true);
			RemoveCamera();
		}
	}

	GetActiveModel()->UpdateInterface();
}

void lcView::CreateResources(lcContext* Context)
{
	Q_UNUSED(Context);

	gGridTexture = new lcTexture;
	gGridTexture->CreateGridTexture();
}

void lcView::DestroyResources(lcContext* Context)
{
	Q_UNUSED(Context);

	delete gGridTexture;
	gGridTexture = nullptr;
}

void lcView::RemoveCamera()
{
	if (mCamera && mCamera->IsSimple())
		return;

	lcCamera* Camera = mCamera;
	mCamera = new lcCamera(true);

	if (Camera)
		mCamera->CopyPosition(Camera);
	else
		mCamera->SetViewpoint(lcViewpoint::Home);

	emit CameraChanged();
	Redraw();
}

lcVector3 lcView::ProjectPoint(const lcVector3& Point) const
{
	int Viewport[4] = { 0, 0, mWidth, mHeight };
	return lcProjectPoint(Point, mCamera->mWorldView, GetProjectionMatrix(), Viewport);
}

lcVector3 lcView::UnprojectPoint(const lcVector3& Point) const
{
	int Viewport[4] = { 0, 0, mWidth, mHeight };
	return lcUnprojectPoint(Point, mCamera->mWorldView, GetProjectionMatrix(), Viewport);
}

void lcView::UnprojectPoints(lcVector3* Points, int NumPoints) const
{
	int Viewport[4] = { 0, 0, mWidth, mHeight };
	lcUnprojectPoints(Points, NumPoints, mCamera->mWorldView, GetProjectionMatrix(), Viewport);
}

lcMatrix44 lcView::GetProjectionMatrix() const
{
	float AspectRatio = (float)mWidth / (float)mHeight;

	if (mCamera->IsOrtho())
	{
		float OrthoHeight = mCamera->GetOrthoHeight() / 2.0f;
		float OrthoWidth = OrthoHeight * AspectRatio;

		return lcMatrix44Ortho(-OrthoWidth, OrthoWidth, -OrthoHeight, OrthoHeight, mCamera->m_zNear, mCamera->m_zFar * 4);
	}
	else
		return lcMatrix44Perspective(mCamera->m_fovy, AspectRatio, mCamera->m_zNear, mCamera->m_zFar);
}

lcMatrix44 lcView::GetTileProjectionMatrix(int CurrentRow, int CurrentColumn, int CurrentTileWidth, int CurrentTileHeight) const
{
	int ImageWidth = mRenderImage.width();
	int ImageHeight = mRenderImage.height();

	double ImageLeft, ImageRight, ImageBottom, ImageTop, Near, Far;
	double AspectRatio = (double)ImageWidth / (double)ImageHeight;

	if (mCamera->IsOrtho())
	{
		float OrthoHeight = mCamera->GetOrthoHeight() / 2.0f;
		float OrthoWidth = OrthoHeight * AspectRatio;

		ImageLeft = -OrthoWidth;
		ImageRight = OrthoWidth;
		ImageBottom = -OrthoHeight;
		ImageTop = OrthoHeight;
		Near = mCamera->m_zNear;
		Far = mCamera->m_zFar * 4;
	}
	else
	{
		double xmin, xmax, ymin, ymax;
		ymax = mCamera->m_zNear * tan(mCamera->m_fovy * 3.14159265 / 360.0);
		ymin = -ymax;
		xmin = ymin * AspectRatio;
		xmax = ymax * AspectRatio;

		ImageLeft = xmin;
		ImageRight = xmax;
		ImageBottom = ymin;
		ImageTop = ymax;
		Near = mCamera->m_zNear;
		Far = mCamera->m_zFar;
	}

	double Left = ImageLeft + (ImageRight - ImageLeft) * (CurrentColumn * mWidth) / ImageWidth;
	double Right = Left + (ImageRight - ImageLeft) * CurrentTileWidth / ImageWidth;
	double Bottom = ImageBottom + (ImageTop - ImageBottom) * (CurrentRow * mHeight) / ImageHeight;
	double Top = Bottom + (ImageTop - ImageBottom) * CurrentTileHeight / ImageHeight;

	if (mCamera->IsOrtho())
		return lcMatrix44Ortho(Left, Right, Bottom, Top, Near, Far);
	else
		return lcMatrix44Frustum(Left, Right, Bottom, Top, Near, Far);
}

void lcView::ShowContextMenu() const
{
	if (mViewType != lcViewType::View)
		return;

	QAction** Actions = gMainWindow->mActions;

	QMenu* Popup = new QMenu();

	Popup->addAction(Actions[LC_EDIT_CUT]);
	Popup->addAction(Actions[LC_EDIT_COPY]);
	Popup->addAction(Actions[LC_EDIT_PASTE]);
	Popup->addAction(Actions[LC_EDIT_PASTE_STEPS]);
	Popup->addAction(Actions[LC_PIECE_DUPLICATE]);

	Popup->addSeparator();

	Popup->addAction(Actions[LC_PIECE_CONTROL_POINT_INSERT]);
	Popup->addAction(Actions[LC_PIECE_CONTROL_POINT_REMOVE]);

	Popup->addSeparator();

	Popup->addAction(Actions[LC_PIECE_EDIT_SELECTED_SUBMODEL]);
	Popup->addAction(Actions[LC_PIECE_EDIT_END_SUBMODEL]);
	Popup->addAction(Actions[LC_PIECE_VIEW_SELECTED_MODEL]);
	Popup->addAction(Actions[LC_PIECE_INLINE_SELECTED_MODELS]);
	Popup->addAction(Actions[LC_PIECE_MOVE_SELECTION_TO_MODEL]);

	Popup->addSeparator();

	Popup->addMenu(gMainWindow->GetToolsMenu());
	Popup->addMenu(gMainWindow->GetViewpointMenu());
	Popup->addMenu(gMainWindow->GetCameraMenu());
	Popup->addMenu(gMainWindow->GetProjectionMenu());
	Popup->addMenu(gMainWindow->GetShadingMenu());

	Popup->addSeparator();

	Popup->addAction(Actions[LC_VIEW_SPLIT_HORIZONTAL]);
	Popup->addAction(Actions[LC_VIEW_SPLIT_VERTICAL]);
	Popup->addAction(Actions[LC_VIEW_REMOVE_VIEW]);
	Popup->addAction(Actions[LC_VIEW_RESET_VIEWS]);

	Popup->exec(QCursor::pos());
	delete Popup;
}

bool lcView::CloseFindReplaceDialog()
{
	if (mFindWidget && (mWidget->hasFocus() || mFindWidget->focusWidget()))
	{
		delete mFindWidget;
		mFindWidget = nullptr;

		return true;
	}

	return false;
}

void lcView::ShowFindReplaceWidget(bool Replace)
{
	delete mFindWidget;
	mFindReplaceParams = lcFindReplaceParams();
	mFindWidget = new lcFindReplaceWidget(mWidget, GetActiveModel(), Replace);
}

lcVector3 lcView::GetMoveDirection(const lcVector3& Direction) const
{
	if (lcGetPreferences().mFixedAxes)
		return Direction;

	// TODO: rewrite this
	lcVector3 axis = Direction;

	lcVector3 Pts[3] = { lcVector3(5.0f, 5.0f, 0.1f), lcVector3(10.0f, 5.0f, 0.1f), lcVector3(5.0f, 10.0f, 0.1f) };
	UnprojectPoints(Pts, 3);

	float ax, ay;
	lcVector3 vx((Pts[1][0] - Pts[0][0]), (Pts[1][1] - Pts[0][1]), 0);//Pts[1][2] - Pts[0][2] };
	vx.Normalize();
	lcVector3 x(1, 0, 0);
	ax = acosf(lcDot(vx, x));

	lcVector3 vy((Pts[2][0] - Pts[0][0]), (Pts[2][1] - Pts[0][1]), 0);//Pts[2][2] - Pts[0][2] };
	vy.Normalize();
	lcVector3 y(0, -1, 0);
	ay = acosf(lcDot(vy, y));

	if (ax > 135)
		axis[0] = -axis[0];

	if (ay < 45)
		axis[1] = -axis[1];

	if (ax >= 45 && ax <= 135)
	{
		float tmp = axis[0];

		ax = acosf(lcDot(vx, y));
		if (ax > 90)
		{
			axis[0] = -axis[1];
			axis[1] = tmp;
		}
		else
		{
			axis[0] = axis[1];
			axis[1] = -tmp;
		}
	}

	return axis;
}

lcMatrix44 lcView::GetPieceInsertPosition(bool IgnoreSelected, PieceInfo* Info) const
{
	lcModel* ActiveModel = GetActiveModel();

	lcPieceInfoRayTest PieceInfoRayTest = FindPieceInfoUnderPointer(IgnoreSelected);
		
	if (PieceInfoRayTest.Info)
	{
		lcVector3 Position = PieceInfoRayTest.Plane;

		if (Position.x > 0.0f)
			Position.x += fabsf(Info->GetBoundingBox().Min.x);
		else if (Position.x < 0.0f)
			Position.x -= fabsf(Info->GetBoundingBox().Max.x);
		else if (Position.y > 0.0f)
			Position.y += fabsf(Info->GetBoundingBox().Min.y);
		else if (Position.y < 0.0f)
			Position.y -= fabsf(Info->GetBoundingBox().Max.y);
		else if (Position.z > 0.0f)
			Position.z += fabsf(Info->GetBoundingBox().Min.z);
		else if (Position.z < 0.0f)
			Position.z -= fabsf(Info->GetBoundingBox().Max.z);

		if (gMainWindow->GetRelativeTransform())
			Position = lcMul31(ActiveModel->SnapPosition(Position), PieceInfoRayTest.Transform);
		else
			Position = ActiveModel->SnapPosition(lcMul31(Position, PieceInfoRayTest.Transform));

		lcMatrix44 WorldMatrix = PieceInfoRayTest.Transform;
		WorldMatrix.SetTranslation(Position);

		return WorldMatrix;
	}

	std::array<lcVector3, 2> ClickPoints = {{ lcVector3((float)mMouseX, (float)mMouseY, 0.0f), lcVector3((float)mMouseX, (float)mMouseY, 1.0f) }};
	UnprojectPoints(ClickPoints.data(), 2);

	if (ActiveModel != mModel)
	{
		lcMatrix44 InverseMatrix = lcMatrix44AffineInverse(mActiveSubmodelTransform);

		for (lcVector3& Point : ClickPoints)
			Point = lcMul31(Point, InverseMatrix);
	}

	const lcBoundingBox& BoundingBox = Info->GetBoundingBox();
	lcVector3 Intersection;

	if (lcLineSegmentPlaneIntersection(&Intersection, ClickPoints[0], ClickPoints[1], lcVector4(0, 0, 1, BoundingBox.Min.z)))
	{
		Intersection = ActiveModel->SnapPosition(Intersection);
		return lcMatrix44Translation(Intersection);
	}

	lcVector3 Position;

	if (!ActiveModel->GetFocusPosition(Position))
		Position = ActiveModel->GetSelectionOrModelCenter();

	lcVector3 FrontVector(mCamera->mTargetPosition - mCamera->mPosition);

	if (lcLineSegmentPlaneIntersection(&Intersection, ClickPoints[0], ClickPoints[1], lcVector4(FrontVector, -lcDot(FrontVector, Position))))
	{
		Intersection = ActiveModel->SnapPosition(Intersection);
		return lcMatrix44Translation(Intersection);
	}

	return lcMatrix44Translation(UnprojectPoint(lcVector3((float)mMouseX, (float)mMouseY, 0.9f)));
}

lcVector3 lcView::GetCameraLightInsertPosition() const
{
	lcModel* ActiveModel = GetActiveModel();

	std::array<lcVector3, 2> ClickPoints = { { lcVector3((float)mMouseX, (float)mMouseY, 0.0f), lcVector3((float)mMouseX, (float)mMouseY, 1.0f) } };
	UnprojectPoints(ClickPoints.data(), 2);

	if (ActiveModel != mModel)
	{
		lcMatrix44 InverseMatrix = lcMatrix44AffineInverse(mActiveSubmodelTransform);

		for (lcVector3& Point : ClickPoints)
			Point = lcMul31(Point, InverseMatrix);
	}

	lcVector3 Min, Max;
	lcVector3 Center;

	if (ActiveModel->GetVisiblePiecesBoundingBox(Min, Max))
		Center = (Min + Max) / 2.0f;
	else
		Center = lcVector3(0.0f, 0.0f, 0.0f);

	return lcRayPointClosestPoint(Center, ClickPoints[0], ClickPoints[1]);
}

void lcView::GetRayUnderPointer(lcVector3& Start, lcVector3& End) const
{
	lcVector3 StartEnd[2] =
	{
		lcVector3((float)mMouseX, (float)mMouseY, 0.0f),
		lcVector3((float)mMouseX, (float)mMouseY, 1.0f)
	};

	UnprojectPoints(StartEnd, 2);

	Start = StartEnd[0];
	End = StartEnd[1];
}

lcObjectSection lcView::FindObjectUnderPointer(bool PiecesOnly, bool IgnoreSelected) const
{
	lcVector3 StartEnd[2] =
	{
		lcVector3((float)mMouseX, (float)mMouseY, 0.0f),
		lcVector3((float)mMouseX, (float)mMouseY, 1.0f)
	};

	UnprojectPoints(StartEnd, 2);

	lcObjectRayTest ObjectRayTest;

	ObjectRayTest.PiecesOnly = PiecesOnly;
	ObjectRayTest.IgnoreSelected = IgnoreSelected;
	ObjectRayTest.ViewCamera = mCamera;
	ObjectRayTest.Start = StartEnd[0];
	ObjectRayTest.End = StartEnd[1];

	lcModel* ActiveModel = GetActiveModel();

	if (ActiveModel != mModel)
	{
		lcMatrix44 InverseMatrix = lcMatrix44AffineInverse(mActiveSubmodelTransform);

		ObjectRayTest.Start = lcMul31(ObjectRayTest.Start, InverseMatrix);
		ObjectRayTest.End = lcMul31(ObjectRayTest.End, InverseMatrix);
	}

	ActiveModel->RayTest(ObjectRayTest);

	return ObjectRayTest.ObjectSection;
}

lcPieceInfoRayTest lcView::FindPieceInfoUnderPointer(bool IgnoreSelected) const
{
	lcVector3 StartEnd[2] =
	{
		lcVector3((float)mMouseX, (float)mMouseY, 0.0f),
		lcVector3((float)mMouseX, (float)mMouseY, 1.0f)
	};

	UnprojectPoints(StartEnd, 2);

	lcObjectRayTest ObjectRayTest;

	ObjectRayTest.PiecesOnly = true;
	ObjectRayTest.IgnoreSelected = IgnoreSelected;
	ObjectRayTest.ViewCamera = mCamera;
	ObjectRayTest.Start = StartEnd[0];
	ObjectRayTest.End = StartEnd[1];

	lcModel* ActiveModel = GetActiveModel();

	if (ActiveModel != mModel)
	{
		lcMatrix44 InverseMatrix = lcMatrix44AffineInverse(mActiveSubmodelTransform);

		ObjectRayTest.Start = lcMul31(ObjectRayTest.Start, InverseMatrix);
		ObjectRayTest.End = lcMul31(ObjectRayTest.End, InverseMatrix);
	}

	ActiveModel->RayTest(ObjectRayTest);

	return ObjectRayTest.PieceInfoRayTest;
}

lcArray<lcObject*> lcView::FindObjectsInBox(float x1, float y1, float x2, float y2) const
{
	float Left, Top, Bottom, Right;

	if (x1 < x2)
	{
		Left = x1;
		Right = x2;
	}
	else
	{
		Left = x2;
		Right = x1;
	}

	if (y1 > y2)
	{
		Top = y1;
		Bottom = y2;
	}
	else
	{
		Top = y2;
		Bottom = y1;
	}

	std::array<lcVector3, 6> Corners =
	{{
		lcVector3(Left, Top, 0),
		lcVector3(Left, Bottom, 0),
		lcVector3(Right, Bottom, 0),
		lcVector3(Right, Top, 0),
		lcVector3(Left, Top, 1),
		lcVector3(Right, Bottom, 1)
	}};

	UnprojectPoints(Corners.data(), (int)Corners.size());

	lcModel* ActiveModel = GetActiveModel();

	if (ActiveModel != mModel)
	{
		lcMatrix44 InverseMatrix = lcMatrix44AffineInverse(mActiveSubmodelTransform);

		for (lcVector3& Point : Corners)
			Point = lcMul31(Point, InverseMatrix);
	}

	lcVector3 PlaneNormals[6];
	PlaneNormals[0] = lcNormalize(lcCross(Corners[4] - Corners[0], Corners[1] - Corners[0])); // Left
	PlaneNormals[1] = lcNormalize(lcCross(Corners[5] - Corners[2], Corners[3] - Corners[2])); // Right
	PlaneNormals[2] = lcNormalize(lcCross(Corners[3] - Corners[0], Corners[4] - Corners[0])); // Top
	PlaneNormals[3] = lcNormalize(lcCross(Corners[1] - Corners[2], Corners[5] - Corners[2])); // Bottom
	PlaneNormals[4] = lcNormalize(lcCross(Corners[1] - Corners[0], Corners[3] - Corners[0])); // Front
	PlaneNormals[5] = lcNormalize(lcCross(Corners[1] - Corners[2], Corners[3] - Corners[2])); // Back

	lcObjectBoxTest ObjectBoxTest;
	ObjectBoxTest.ViewCamera = mCamera;
	ObjectBoxTest.Planes[0] = lcVector4(PlaneNormals[0], -lcDot(PlaneNormals[0], Corners[0]));
	ObjectBoxTest.Planes[1] = lcVector4(PlaneNormals[1], -lcDot(PlaneNormals[1], Corners[5]));
	ObjectBoxTest.Planes[2] = lcVector4(PlaneNormals[2], -lcDot(PlaneNormals[2], Corners[0]));
	ObjectBoxTest.Planes[3] = lcVector4(PlaneNormals[3], -lcDot(PlaneNormals[3], Corners[5]));
	ObjectBoxTest.Planes[4] = lcVector4(PlaneNormals[4], -lcDot(PlaneNormals[4], Corners[0]));
	ObjectBoxTest.Planes[5] = lcVector4(PlaneNormals[5], -lcDot(PlaneNormals[5], Corners[5]));

	ActiveModel->BoxTest(ObjectBoxTest);

	return ObjectBoxTest.Objects;
}

std::vector<QImage> lcView::GetStepImages(lcStep Start, lcStep End)
{
	std::vector<QImage> Images;

	if (!BeginRenderToImage(mWidth, mHeight))
	{
		QMessageBox::warning(gMainWindow, tr("LeoCAD"), tr("Error creating images."));
		return Images;
	}

	const lcStep CurrentStep = mModel->GetCurrentStep();

	for (lcStep Step = Start; Step <= End; Step++)
	{
		mModel->SetTemporaryStep(Step);

		OnDraw();

		Images.emplace_back(GetRenderImage());
	}

	EndRenderToImage();

	mModel->SetTemporaryStep(CurrentStep);

	if (!mModel->IsActive())
		mModel->CalculateStep(LC_STEP_MAX);

	return Images;
}

void lcView::SaveStepImages(const QString& BaseName, bool AddStepSuffix, lcStep Start, lcStep End, std::function<void(const QString&)> ProgressCallback)
{
	std::vector<QImage> Images = GetStepImages(Start, End);

	for (lcStep Step = Start; Step <= End; Step++)
	{
		QString FileName;

		if (AddStepSuffix)
			FileName = BaseName.arg(Step, 2, 10, QLatin1Char('0'));
		else
			FileName = BaseName;

		QImageWriter Writer(FileName);

		if (Writer.format().isEmpty())
			Writer.setFormat("png");

		if (!Writer.write(Images[Step - Start]))
		{
			QMessageBox::information(gMainWindow, tr("Error"), tr("Error writing to file '%1':\n%2").arg(FileName, Writer.errorString()));
			break;
		}

		if (ProgressCallback)
			ProgressCallback(FileName);
	}
}

bool lcView::BeginRenderToImage(int Width, int Height)
{
	GLint MaxTexture;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &MaxTexture);

	MaxTexture = qMin(MaxTexture, 2048);

	const int Samples = QSurfaceFormat::defaultFormat().samples();
	if (Samples > 1)
		MaxTexture /= Samples;

	int TileWidth = qMin(Width, MaxTexture);
	int TileHeight = qMin(Height, MaxTexture);

	mWidth = TileWidth;
	mHeight = TileHeight;
	mRenderImage = QImage(Width, Height, QImage::Format_ARGB32);

	QOpenGLFramebufferObjectFormat Format;
	Format.setAttachment(QOpenGLFramebufferObject::Depth);

	if (QSurfaceFormat::defaultFormat().samples() > 1)
		Format.setSamples(QSurfaceFormat::defaultFormat().samples());

	mRenderFramebuffer = std::unique_ptr<QOpenGLFramebufferObject>(new QOpenGLFramebufferObject(QSize(TileWidth, TileHeight), Format));

	return mRenderFramebuffer->bind();
}

void lcView::EndRenderToImage()
{
	mRenderFramebuffer.reset();
}

QImage lcView::GetRenderImage() const
{
	return mRenderImage;
}

void lcView::BindRenderFramebuffer()
{
	mRenderFramebuffer->bind();
}

void lcView::UnbindRenderFramebuffer()
{
	mRenderFramebuffer->release();
}

QImage lcView::GetRenderFramebufferImage() const
{
	return mRenderFramebuffer->toImage();
}

void lcView::OnDraw()
{
	if (!mModel)
		return;

//#define LC_PROFILE_DRAW
#ifdef LC_PROFILE_DRAW
	static std::array<GLuint64, 30> QueryResults;
	static std::array<qint64, 30> TimerResults;
	static int FrameNumber;

	QElapsedTimer Timer;
	QOpenGLTimerQuery Query;

	if (!Query.isCreated())
		Query.create();

	Query.begin();
	Timer.start();
#endif

	const lcPreferences& Preferences = lcGetPreferences();
	const bool DrawOverlays = mWidget != nullptr;
	const bool DrawInterface = mWidget != nullptr && mViewType == lcViewType::View;

	lcShadingMode ShadingMode = Preferences.mShadingMode;
	if (ShadingMode == lcShadingMode::Wireframe && !mWidget)
		ShadingMode = lcShadingMode::Flat;

	mScene->SetShadingMode(ShadingMode);
	mScene->SetAllowLOD(Preferences.mAllowLOD && mWidget != nullptr);
	mScene->SetLODDistance(Preferences.mMeshLODDistance);

	mScene->Begin(mCamera->mWorldView);

	mScene->SetActiveSubmodelInstance(mActiveSubmodelInstance, mActiveSubmodelTransform);
	mScene->SetDrawInterface(DrawInterface);

	mModel->GetScene(mScene.get(), mCamera, Preferences.mHighlightNewParts, Preferences.mFadeSteps);

	if (DrawInterface && mTrackTool == lcTrackTool::Insert)
	{
		PieceInfo* Info = gMainWindow->GetCurrentPieceInfo();

		if (Info)
		{
			lcMatrix44 WorldMatrix = GetPieceInsertPosition(false, Info);

			if (GetActiveModel() != mModel)
				WorldMatrix = lcMul(WorldMatrix, mActiveSubmodelTransform);

			Info->AddRenderMeshes(mScene.get(), WorldMatrix, gMainWindow->mColorIndex, lcRenderMeshState::Focused, false);
		}
	}

	if (DrawInterface)
		mScene->SetPreTranslucentCallback([this]() { DrawGrid(); });

	mScene->End();

	int TotalTileRows = 1;
	int TotalTileColumns = 1;

	if (!mRenderImage.isNull())
	{
		int ImageWidth = mRenderImage.width();
		int ImageHeight = mRenderImage.height();

		if (ImageWidth > mWidth || ImageHeight > mHeight)
		{
			TotalTileColumns = (mWidth + ImageWidth - 1) / mWidth;
			TotalTileRows = (mHeight + ImageHeight - 1) / mHeight;
		}
	}

	for (int CurrentTileRow = 0; CurrentTileRow < TotalTileRows; CurrentTileRow++)
	{
		for (int CurrentTileColumn = 0; CurrentTileColumn < TotalTileColumns; CurrentTileColumn++)
		{
			mContext->SetDefaultState();
			mContext->SetViewport(0, 0, mWidth, mHeight);

			int CurrentTileWidth, CurrentTileHeight;

			if (!mRenderImage.isNull() && (TotalTileRows > 1 || TotalTileColumns > 1))
			{
				if (CurrentTileRow < TotalTileRows - 1)
					CurrentTileHeight = mHeight;
				else
					CurrentTileHeight = mRenderImage.height() - (TotalTileRows - 1) * (mHeight);

				if (CurrentTileColumn < TotalTileColumns - 1)
					CurrentTileWidth = mWidth;
				else
					CurrentTileWidth = mRenderImage.width() - (TotalTileColumns - 1) * (mWidth);

				DrawBackground(CurrentTileRow, TotalTileRows, CurrentTileHeight);

				mContext->SetViewport(0, 0, CurrentTileWidth, CurrentTileHeight);
				mContext->SetProjectionMatrix(GetTileProjectionMatrix(CurrentTileRow, CurrentTileColumn, CurrentTileWidth, CurrentTileHeight));
			}
			else
			{
				CurrentTileWidth = mWidth;
				CurrentTileHeight = mHeight;

				DrawBackground(CurrentTileRow, TotalTileRows, CurrentTileHeight);

				mContext->SetProjectionMatrix(GetProjectionMatrix());
			}

			mContext->SetLineWidth(Preferences.mLineWidth);

			mScene->Draw(mContext);

			if (!mRenderImage.isNull())
			{
				UnbindRenderFramebuffer();
				QImage TileImage = GetRenderFramebufferImage();
				BindRenderFramebuffer();
				quint8* Buffer = TileImage.bits();
				uchar* ImageBuffer = mRenderImage.bits();

				quint32 TileY = 0, SrcY = 0;
				if (CurrentTileRow != TotalTileRows - 1)
					TileY = (TotalTileRows - CurrentTileRow - 1) * mHeight - ((mHeight - mRenderImage.height() % mHeight) % mHeight);
				else if (TotalTileRows > 1)
					SrcY = (mHeight - mRenderImage.height() % mHeight) % mHeight;

				quint32 TileStart = ((CurrentTileColumn * mWidth) + (TileY * mRenderImage.width())) * 4;

				for (int y = 0; y < CurrentTileHeight; y++)
				{
					quint8* src = Buffer + (SrcY + y) * mWidth * 4;
					quint8* dst = ImageBuffer + TileStart + y * mRenderImage.width() * 4;

					memcpy(dst, src, CurrentTileWidth * 4);
				}
			}
		}
	}

	if (DrawInterface)
		mScene->DrawInterfaceObjects(mContext);

	if (DrawOverlays)
		DrawAxes();

	if (DrawInterface)
	{
		lcTool Tool = gMainWindow->GetTool();
		lcModel* ActiveModel = GetActiveModel();

		if ((Tool == lcTool::Select || Tool == lcTool::Move) && mTrackButton == lcTrackButton::None && ActiveModel->AnyObjectsSelected())
			mViewManipulator->DrawSelectMove(mTrackButton, mTrackTool);
		else if (GetCurrentTool() == lcTool::Move && mTrackButton != lcTrackButton::None)
			mViewManipulator->DrawSelectMove(mTrackButton, mTrackTool);
		else if ((Tool == lcTool::Rotate || (Tool == lcTool::Select && mTrackButton != lcTrackButton::None && mTrackTool >= lcTrackTool::RotateX && mTrackTool <= lcTrackTool::RotateXYZ)) && ActiveModel->AnyPiecesSelected())
			mViewManipulator->DrawRotate(mTrackButton, mTrackTool);
		else if ((mTrackTool == lcTrackTool::Select || mTrackTool == lcTrackTool::ZoomRegion) && mTrackButton != lcTrackButton::None)
			DrawSelectZoomRegionOverlay();
		else if (Tool == lcTool::RotateView && mTrackButton == lcTrackButton::None)
			DrawRotateViewOverlay();
	}

	if (DrawOverlays)
	{
		mViewSphere->Draw();

		DrawViewport();
	}

#ifdef LC_PROFILE_DRAW
	qint64 TimerElapsed = Timer.nsecsElapsed();
	Query.end();

	GLuint64 QueryElapsed = Query.waitForResult();
	QueryResults[FrameNumber % QueryResults.size()] = QueryElapsed;
	TimerResults[FrameNumber % TimerResults.size()] = TimerElapsed;
	FrameNumber++;

	GLuint64 QueryAverage = 0;
	for (GLuint64 Result : QueryResults)
		QueryAverage += Result;
	QueryAverage /= QueryResults.size();

	GLuint64 TimerAverage = 0;
	for (GLuint64 Result : TimerResults)
		TimerAverage += Result;
	TimerAverage /= TimerResults.size();

	mContext->SetWorldMatrix(lcMatrix44Identity());
	mContext->SetViewMatrix(lcMatrix44Translation(lcVector3(0.375, 0.375, 0.0)));
	mContext->SetProjectionMatrix(lcMatrix44Ortho(0.0f, mWidth, 0.0f, mHeight, -1.0f, 1.0f));

	QString Line = QString("GPU: %1 CPU: %2").arg(QString::number(QueryAverage / 1000000.0, 'f', 2), QString::number(TimerAverage / 1000000.0, 'f', 2));

	mContext->SetMaterial(lcMaterialType::UnlitTextureModulate);
	mContext->SetColor(lcVector4FromColor(lcGetPreferences().mTextColor));
	mContext->BindTexture2D(gTexFont.GetTexture());

	mContext->EnableDepthTest(false);
	mContext->EnableColorBlend(true);

	gTexFont.PrintText(mContext, 3.0f, (float)mHeight - 1.0f - 6.0f, 0.0f, Line.toLatin1().constData());

	mContext->EnableColorBlend(false);
	mContext->EnableDepthTest(true);

	Redraw();
#endif

	mContext->ClearResources();
}

void lcView::DrawBackground(int CurrentTileRow, int TotalTileRows, int CurrentTileHeight) const
{
	if (mOverrideBackgroundColor)
	{
		lcVector4 BackgroundColor(lcVector4FromColor(mBackgroundColor));
		mContext->ClearColorAndDepth(BackgroundColor);
		return;
	}

	const lcPreferences& Preferences = lcGetPreferences();

	if (!Preferences.mBackgroundGradient)
	{
		lcVector4 BackgroundColor(lcVector3FromColor(Preferences.mBackgroundSolidColor), 0.0f);
		mContext->ClearColorAndDepth(BackgroundColor);
		return;
	}

	mContext->ClearDepth();

	mContext->SetDepthWrite(false);
	mContext->EnableDepthTest(false);

	float ViewWidth = (float)mWidth;
	float ViewHeight = (float)mHeight;

	mContext->SetWorldMatrix(lcMatrix44Identity());
	mContext->SetViewMatrix(lcMatrix44Identity());
	mContext->SetProjectionMatrix(lcMatrix44Ortho(0.0f, ViewWidth, 0.0f, ViewHeight, -1.0f, 1.0f));

	const int TotalHeight = TotalTileRows == 1 ? mHeight : mRenderImage.height();
	const quint32 TopY = CurrentTileRow * mHeight + CurrentTileHeight;

	const double t1 = 1.0 - (double)TopY / (double)TotalHeight;
	const double t2 = 1.0 - (double)(TopY - CurrentTileHeight) / (double)TotalHeight;

	const quint32 ColorTop = Preferences.mBackgroundGradientColorTop;
	const quint32 ColorBottom = Preferences.mBackgroundGradientColorBottom;

	double TopRed = LC_RGBA_RED(ColorTop);
	double TopGreen = LC_RGBA_GREEN(ColorTop);
	double TopBlue = LC_RGBA_BLUE(ColorTop);
	double TopAlpha = LC_RGBA_ALPHA(ColorTop);
	double BottomRed = LC_RGBA_RED(ColorBottom);
	double BottomGreen = LC_RGBA_GREEN(ColorBottom);
	double BottomBlue = LC_RGBA_BLUE(ColorBottom);
	double BottomAlpha = LC_RGBA_ALPHA(ColorBottom);
	const double DeltaRed = BottomRed - TopRed;
	const double DeltaGreen = BottomGreen - TopGreen;
	const double DeltaBlue = BottomBlue - TopBlue;
	const double DeltaAlpha = BottomAlpha - TopAlpha;

	BottomRed = TopRed + DeltaRed * t2;
	BottomGreen = TopGreen + DeltaGreen * t2;
	BottomBlue = TopBlue + DeltaBlue * t2;
	BottomAlpha = TopAlpha + DeltaAlpha * t2;

	TopRed = TopRed + DeltaRed * t1;
	TopGreen = TopGreen + DeltaGreen * t1;
	TopBlue = TopBlue + DeltaBlue * t1;
	TopAlpha = TopAlpha + DeltaAlpha * t1;

	const quint32 Color1 = LC_RGBA(TopRed, TopGreen, TopBlue, TopAlpha);
	const quint32 Color2 = LC_RGBA(BottomRed, BottomGreen, BottomBlue, BottomAlpha);

	struct lcBackgroundVertex
	{
		float x, y;
		quint32 Color;
	};

	const lcBackgroundVertex Verts[4] =
	{
		{ ViewWidth, ViewHeight, Color1 }, { 0.0f, ViewHeight, Color1 }, { 0.0f, 0.0f, Color2 }, { ViewWidth, 0.0f, Color2 }
	};

	mContext->SetMaterial(lcMaterialType::UnlitVertexColor);
	mContext->SetVertexBufferPointer(Verts);
	mContext->SetVertexFormat(0, 2, 0, 0, 4, false);

	mContext->DrawPrimitives(GL_TRIANGLE_FAN, 0, 4);

	mContext->EnableDepthTest(true);
	mContext->SetDepthWrite(true);
}

void lcView::DrawViewport() const
{
	mContext->SetWorldMatrix(lcMatrix44Identity());
	mContext->SetViewMatrix(lcMatrix44Translation(lcVector3(0.375, 0.375, 0.0)));
	mContext->SetProjectionMatrix(lcMatrix44Ortho(0.0f, mWidth, 0.0f, mHeight, -1.0f, 1.0f));
	mContext->SetLineWidth(1.0f);

	mContext->SetDepthWrite(false);
	mContext->EnableDepthTest(false);

	mContext->SetMaterial(lcMaterialType::UnlitColor);

	if (mLastFocusedView == this)
		mContext->SetColor(lcVector4FromColor(lcGetPreferences().mActiveViewColor));
	else
		mContext->SetColor(lcVector4FromColor(lcGetPreferences().mInactiveViewColor));

	float Verts[8] = { 0.0f, 0.0f, mWidth - 1.0f, 0.0f, mWidth - 1.0f, mHeight - 1.0f, 0.0f, mHeight - 1.0f };

	mContext->SetVertexBufferPointer(Verts);
	mContext->SetVertexFormatPosition(2);
	mContext->DrawPrimitives(GL_LINE_LOOP, 0, 4);

	QString CameraName = mCamera->GetName();

	if (!CameraName.isEmpty())
	{
		mContext->SetMaterial(lcMaterialType::UnlitTextureModulate);
		mContext->SetColor(lcVector4FromColor(lcGetPreferences().mTextColor));
		mContext->BindTexture2D(gTexFont.GetTexture());

		mContext->EnableColorBlend(true);

		gTexFont.PrintText(mContext, 3.0f, (float)mHeight - 1.0f - 6.0f, 0.0f, CameraName.toLatin1().constData());

		mContext->EnableColorBlend(false);
	}

	mContext->SetDepthWrite(true);
	mContext->EnableDepthTest(true);
}

void lcView::DrawAxes() const
{
	const lcPreferences& Preferences = lcGetPreferences();

	switch (mViewType)
	{
		case lcViewType::View:
			if (!Preferences.mDrawAxes)
				return;
			break;

		case lcViewType::Preview:
			if (!Preferences.mDrawPreviewAxis)
				return;
			break;

		case lcViewType::Minifig:
		case lcViewType::PartsList:
		case lcViewType::Count:
			return;
	}

//	mContext->ClearDepth();

	struct lcAxisVertex
	{
		float x, y, z;
		quint32 Color;
	};

	const quint32 Red = LC_RGBA(204, 0, 0, 255);
	const quint32 Green = LC_RGBA(0, 204, 0, 255);
	const quint32 Blue = LC_RGBA(0, 0, 204, 255);

	const lcAxisVertex Verts[30] =
	{
		{  0.00f,  0.00f,  0.00f, Red }, { 20.00f,  0.00f,  0.00f, Red }, { 12.00f,  3.00f,  0.00f, Red }, { 12.00f,  2.12f,  2.12f, Red }, { 12.00f,  0.00f,  3.00f, Red },
		{ 12.00f, -2.12f,  2.12f, Red }, { 12.00f, -3.00f,  0.00f, Red }, { 12.00f, -2.12f, -2.12f, Red }, { 12.00f,  0.00f, -3.00f, Red }, { 12.00f,  2.12f, -2.12f, Red },
		{  0.00f,  0.00f,  0.00f, Green }, {  0.00f, 20.00f,  0.00f, Green }, {  3.00f, 12.00f,  0.00f, Green }, {  2.12f, 12.00f,  2.12f, Green }, {  0.00f, 12.00f,  3.00f, Green },
		{ -2.12f, 12.00f,  2.12f, Green }, { -3.00f, 12.00f,  0.00f, Green }, { -2.12f, 12.00f, -2.12f, Green }, {  0.00f, 12.00f, -3.00f, Green }, {  2.12f, 12.00f, -2.12f, Green },
		{  0.00f,  0.00f,  0.00f, Blue }, {  0.00f,  0.00f, 20.00f, Blue }, {  0.00f,  3.00f, 12.00f, Blue }, {  2.12f,  2.12f, 12.00f, Blue }, {  3.00f,  0.00f, 12.00f, Blue },
		{  2.12f, -2.12f, 12.00f, Blue }, {  0.00f, -3.00f, 12.00f, Blue }, { -2.12f, -2.12f, 12.00f, Blue }, { -3.00f,  0.00f, 12.00f, Blue }, { -2.12f,  2.12f, 12.00f, Blue }
	};

	const GLushort Indices[78] =
	{
		 0,  1, 10, 11, 20, 21,
		 1,  2,  3,  1,  3,  4,  1,  4,  5,  1,  5,  6,  1,  6,  7,  1,  7,  8,  1,  8,  9,  1,  9,  2,
		11, 12, 13, 11, 13, 14, 11, 14, 15, 11, 15, 16, 11, 16, 17, 11, 17, 18, 11, 18, 19, 11, 19, 12,
		21, 22, 23, 21, 23, 24, 21, 24, 25, 21, 25, 26, 21, 26, 27, 21, 27, 28, 21, 28, 29, 21, 29, 22
	};

	lcMatrix44 TranslationMatrix;

	switch (Preferences.mAxisIconLocation)
	{
	default:
	case lcAxisIconLocation::BottomLeft:
		TranslationMatrix = lcMatrix44Translation(lcVector3(32, 32, 0.0f));
		break;

	case lcAxisIconLocation::BottomRight:
		TranslationMatrix = lcMatrix44Translation(lcVector3(mWidth - 36, 32, 0.0f));
		break;

	case lcAxisIconLocation::TopLeft:
		TranslationMatrix = lcMatrix44Translation(lcVector3(32, mHeight - 36, 0.0f));
		break;

	case lcAxisIconLocation::TopRight:
		TranslationMatrix = lcMatrix44Translation(lcVector3(mWidth - 36, mHeight - 36, 0.0f));
		break;
	}
	lcMatrix44 WorldViewMatrix = mCamera->mWorldView;
	WorldViewMatrix.SetTranslation(lcVector3(0, 0, 0));

	mContext->SetLineWidth(1.0f);
	mContext->SetMaterial(lcMaterialType::UnlitVertexColor);
	mContext->SetWorldMatrix(lcMatrix44Identity());
	mContext->SetViewMatrix(lcMul(WorldViewMatrix, TranslationMatrix));
	mContext->SetProjectionMatrix(lcMatrix44Ortho(0, mWidth, 0, mHeight, -50, 50));

	mContext->SetVertexBufferPointer(Verts);
	mContext->SetVertexFormat(0, 3, 0, 0, 4, false);
	mContext->SetIndexBufferPointer(Indices);

	mContext->DrawIndexedPrimitives(GL_LINES, 6, GL_UNSIGNED_SHORT, 0);
	mContext->DrawIndexedPrimitives(GL_TRIANGLES, 72, GL_UNSIGNED_SHORT, 6 * 2);

	mContext->SetMaterial(lcMaterialType::UnlitTextureModulate);
	mContext->SetViewMatrix(TranslationMatrix);
	mContext->BindTexture2D(gTexFont.GetTexture());
	mContext->EnableColorBlend(true);

	float TextBuffer[6 * 5 * 3];
	lcVector3 PosX = lcMul30(lcVector3(25.0f, 0.0f, 0.0f), WorldViewMatrix);
	gTexFont.GetGlyphTriangles(PosX.x, PosX.y, PosX.z, 'X', TextBuffer);
	lcVector3 PosY = lcMul30(lcVector3(0.0f, 25.0f, 0.0f), WorldViewMatrix);
	gTexFont.GetGlyphTriangles(PosY.x, PosY.y, PosY.z, 'Y', TextBuffer + 5 * 6);
	lcVector3 PosZ = lcMul30(lcVector3(0.0f, 0.0f, 25.0f), WorldViewMatrix);
	gTexFont.GetGlyphTriangles(PosZ.x, PosZ.y, PosZ.z, 'Z', TextBuffer + 5 * 6 * 2);

	mContext->SetVertexBufferPointer(TextBuffer);
	mContext->SetVertexFormat(0, 3, 0, 2, 0, false);

	mContext->SetColor(lcVector4FromColor(lcGetPreferences().mAxesColor));
	mContext->DrawPrimitives(GL_TRIANGLES, 0, 6 * 3);

	mContext->EnableColorBlend(false);
}

void lcView::DrawSelectZoomRegionOverlay()
{
	mContext->SetMaterial(lcMaterialType::UnlitColor);
	mContext->SetWorldMatrix(lcMatrix44Identity());
	mContext->SetViewMatrix(lcMatrix44Translation(lcVector3(0.375, 0.375, 0.0)));
	mContext->SetProjectionMatrix(lcMatrix44Ortho(0.0f, mWidth, 0.0f, mHeight, -1.0f, 1.0f));
	mContext->SetLineWidth(1.0f);

	mContext->EnableDepthTest(false);

	float pt1x = (float)mMouseDownX;
	float pt1y = (float)mMouseDownY;
	float pt2x = (float)mMouseX;
	float pt2y = (float)mMouseY;

	float Left, Right, Bottom, Top;

	if (pt1x < pt2x)
	{
		Left = pt1x;
		Right = pt2x;
	}
	else
	{
		Left = pt2x;
		Right = pt1x;
	}

	if (pt1y < pt2y)
	{
		Bottom = pt1y;
		Top = pt2y;
	}
	else
	{
		Bottom = pt2y;
		Top = pt1y;
	}

	Left = lcMax(Left, 0.0f);
	Right = lcMin(Right, mWidth - 1.0f);
	Bottom = lcMax(Bottom, 0.0f);
	Top = lcMin(Top, mHeight - 1.0f);

	float BorderX = lcMin(2.0f, Right - Left);
	float BorderY = lcMin(2.0f, Top - Bottom);

	float Verts[14][2] =
	{
		{ Left, Bottom },
		{ Left + BorderX, Bottom + BorderY },
		{ Right, Bottom },
		{ Right - BorderX, Bottom + BorderY },
		{ Right, Top },
		{ Right - BorderX, Top - BorderY },
		{ Left, Top },
		{ Left + BorderX, Top - BorderY },
		{ Left, Bottom },
		{ Left + BorderX, Bottom + BorderY },
		{ Left + BorderX, Bottom + BorderY },
		{ Right - BorderX, Bottom + BorderY },
		{ Left + BorderX, Top - BorderY },
		{ Right - BorderX, Top - BorderY },
	};

	mContext->SetVertexBufferPointer(Verts);
	mContext->SetVertexFormatPosition(2);

	const lcPreferences& Preferences = lcGetPreferences();

	mContext->SetColor(lcVector4FromColor(Preferences.mMarqueeBorderColor));
	mContext->DrawPrimitives(GL_TRIANGLE_STRIP, 0, 10);

	if (LC_RGBA_ALPHA(Preferences.mMarqueeFillColor))
	{
		mContext->EnableColorBlend(true);
		mContext->SetColor(lcVector4FromColor(Preferences.mMarqueeFillColor));
		mContext->DrawPrimitives(GL_TRIANGLE_STRIP, 10, 4);
		mContext->EnableColorBlend(false);
	}

	mContext->EnableDepthTest(true);
}

void lcView::DrawRotateViewOverlay()
{
	int x, y, w, h;

	x = 0;
	y = 0;
	w = mWidth;
	h = mHeight;

	mContext->SetMaterial(lcMaterialType::UnlitColor);
	mContext->SetWorldMatrix(lcMatrix44Identity());
	mContext->SetViewMatrix(lcMatrix44Translation(lcVector3(0.375, 0.375, 0.0)));
	mContext->SetProjectionMatrix(lcMatrix44Ortho(0, w, 0, h, -1, 1));
	mContext->SetLineWidth(1.0f);

	mContext->EnableDepthTest(false);
	mContext->SetColor(lcVector4FromColor(lcGetPreferences().mOverlayColor));

	float Verts[32 * 16 * 2];
	float* CurVert = Verts;

	float r = lcMin(w, h) * 0.35f;
	float cx = x + w / 2.0f;
	float cy = y + h / 2.0f;

	for (int i = 0; i < 32; i++)
	{
		*CurVert++ = cosf((float)i / 32.0f * (2.0f * LC_PI)) * r + cx;
		*CurVert++ = sinf((float)i / 32.0f * (2.0f * LC_PI)) * r + cy;
	}

	const float OverlayCameraSquareSize = lcMax(8.0f, (w + h) / 200.0f);

	*CurVert++ = cx + OverlayCameraSquareSize; *CurVert++ = cy + r + OverlayCameraSquareSize;
	*CurVert++ = cx - OverlayCameraSquareSize; *CurVert++ = cy + r + OverlayCameraSquareSize;
	*CurVert++ = cx - OverlayCameraSquareSize; *CurVert++ = cy + r - OverlayCameraSquareSize;
	*CurVert++ = cx + OverlayCameraSquareSize; *CurVert++ = cy + r - OverlayCameraSquareSize;
	*CurVert++ = cx + OverlayCameraSquareSize; *CurVert++ = cy - r + OverlayCameraSquareSize;
	*CurVert++ = cx - OverlayCameraSquareSize; *CurVert++ = cy - r + OverlayCameraSquareSize;
	*CurVert++ = cx - OverlayCameraSquareSize; *CurVert++ = cy - r - OverlayCameraSquareSize;
	*CurVert++ = cx + OverlayCameraSquareSize; *CurVert++ = cy - r - OverlayCameraSquareSize;
	*CurVert++ = cx + r + OverlayCameraSquareSize; *CurVert++ = cy + OverlayCameraSquareSize;
	*CurVert++ = cx + r - OverlayCameraSquareSize; *CurVert++ = cy + OverlayCameraSquareSize;
	*CurVert++ = cx + r - OverlayCameraSquareSize; *CurVert++ = cy - OverlayCameraSquareSize;
	*CurVert++ = cx + r + OverlayCameraSquareSize; *CurVert++ = cy - OverlayCameraSquareSize;
	*CurVert++ = cx - r + OverlayCameraSquareSize; *CurVert++ = cy + OverlayCameraSquareSize;
	*CurVert++ = cx - r - OverlayCameraSquareSize; *CurVert++ = cy + OverlayCameraSquareSize;
	*CurVert++ = cx - r - OverlayCameraSquareSize; *CurVert++ = cy - OverlayCameraSquareSize;
	*CurVert++ = cx - r + OverlayCameraSquareSize; *CurVert++ = cy - OverlayCameraSquareSize;

	mContext->SetVertexBufferPointer(Verts);
	mContext->SetVertexFormatPosition(2);

	GLushort Indices[64 + 32] = 
	{
		0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16,
		17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 23, 24, 24, 25, 25, 26, 26, 27, 27, 28, 28, 29, 29, 30, 30, 31, 31, 0,
		32, 33, 33, 34, 34, 35, 35, 32, 36, 37, 37, 38, 38, 39, 39, 36,
		40, 41, 41, 42, 42, 43, 43, 40, 44, 45, 45, 46, 46, 47, 47, 44
	};

	mContext->SetIndexBufferPointer(Indices);
	mContext->DrawIndexedPrimitives(GL_LINES, 96, GL_UNSIGNED_SHORT, 0);

	mContext->EnableDepthTest(true);
}

void lcView::DrawGrid()
{
	const lcPreferences& Preferences = lcGetPreferences();

	if (!Preferences.mDrawGridStuds && !Preferences.mDrawGridLines && !Preferences.mDrawGridOrigin)
		return;

	if (!Preferences.mGridEnabled)
		return;

	const int Spacing = lcMax(Preferences.mGridLineSpacing, 1);
	int MinX, MaxX, MinY, MaxY;
	lcVector3 Min(FLT_MAX, FLT_MAX, FLT_MAX), Max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	bool GridSizeValid = mModel->GetVisiblePiecesBoundingBox(Min, Max);

	if (mTrackTool == lcTrackTool::Insert)
	{
		PieceInfo* CurPiece = gMainWindow->GetCurrentPieceInfo();

		if (CurPiece)
		{
			lcVector3 Points[8];
			lcGetBoxCorners(CurPiece->GetBoundingBox(), Points);

			lcMatrix44 WorldMatrix = GetPieceInsertPosition(false, CurPiece);

			for (int i = 0; i < 8; i++)
			{
				lcVector3 Point = lcMul31(Points[i], WorldMatrix);

				Min = lcMin(Point, Min);
				Max = lcMax(Point, Max);
			}

			GridSizeValid = true;
		}
	}

	if (GridSizeValid)
	{
		MinX = (int)(floorf(Min[0] / (20.0f * Spacing))) - 1;
		MinY = (int)(floorf(Min[1] / (20.0f * Spacing))) - 1;
		MaxX = (int)(ceilf(Max[0] / (20.0f * Spacing))) + 1;
		MaxY = (int)(ceilf(Max[1] / (20.0f * Spacing))) + 1;

		MinX = lcMin(MinX, -2);
		MinY = lcMin(MinY, -2);
		MaxX = lcMax(MaxX, 2);
		MaxY = lcMax(MaxY, 2);
	}
	else
	{
		MinX = -2;
		MinY = -2;
		MaxX = 2;
		MaxY = 2;
	}

	if (!mGridBuffer.IsValid() || MinX != mGridSettings[0] || MinY != mGridSettings[1] || MaxX != mGridSettings[2] || MaxY != mGridSettings[3] ||
	    Spacing != mGridSettings[4] || (Preferences.mDrawGridStuds ? 1 : 0) != mGridSettings[5] || (Preferences.mDrawGridLines ? 1 : 0) != mGridSettings[6])
	{
		int VertexBufferSize = 0;

		if (Preferences.mDrawGridStuds)
			VertexBufferSize += 4 * 5 * sizeof(float);

		if (Preferences.mDrawGridLines)
			VertexBufferSize += 2 * (MaxX - MinX + MaxY - MinY + 2) * 3 * sizeof(float);

		float* Verts = (float*)malloc(VertexBufferSize);
		if (!Verts)
			return;
		float* CurVert = Verts;

		if (Preferences.mDrawGridStuds)
		{
			float Left = MinX * 20.0f * Spacing;
			float Right = MaxX * 20.0f * Spacing;
			float Top = MinY * 20.0f * Spacing;
			float Bottom = MaxY * 20.0f * Spacing;
			float Z = 0;
			float U = (MaxX - MinX) * Spacing;
			float V = (MaxY - MinY) * Spacing;

			*CurVert++ = Left;
			*CurVert++ = Top;
			*CurVert++ = Z;
			*CurVert++ = 0.0f;
			*CurVert++ = V;

			*CurVert++ = Right;
			*CurVert++ = Top;
			*CurVert++ = Z;
			*CurVert++ = U;
			*CurVert++ = V;

			*CurVert++ = Left;
			*CurVert++ = Bottom;
			*CurVert++ = Z;
			*CurVert++ = 0.0f;
			*CurVert++ = 0.0f;

			*CurVert++ = Right;
			*CurVert++ = Bottom;
			*CurVert++ = Z;
			*CurVert++ = U;
			*CurVert++ = 0.0f;
		}

		if (Preferences.mDrawGridLines)
		{
			float LineSpacing = Spacing * 20.0f;

			for (int Step = MinX; Step < MaxX + 1; Step++)
			{
				*CurVert++ = Step * LineSpacing;
				*CurVert++ = MinY * LineSpacing;
				*CurVert++ = 0.0f;
				*CurVert++ = Step * LineSpacing;
				*CurVert++ = MaxY * LineSpacing;
				*CurVert++ = 0.0f;
			}

			for (int Step = MinY; Step < MaxY + 1; Step++)
			{
				*CurVert++ = MinX * LineSpacing;
				*CurVert++ = Step * LineSpacing;
				*CurVert++ = 0.0f;
				*CurVert++ = MaxX * LineSpacing;
				*CurVert++ = Step * LineSpacing;
				*CurVert++ = 0.0f;
			}
		}

		mGridSettings[0] = MinX;
		mGridSettings[1] = MinY;
		mGridSettings[2] = MaxX;
		mGridSettings[3] = MaxY;
		mGridSettings[4] = Spacing;
		mGridSettings[5] = (Preferences.mDrawGridStuds ? 1 : 0);
		mGridSettings[6] = (Preferences.mDrawGridLines ? 1 : 0);

		mContext->DestroyVertexBuffer(mGridBuffer);
		mGridBuffer = mContext->CreateVertexBuffer(VertexBufferSize, Verts);
		free(Verts);
	}

	int BufferOffset = 0;
	mContext->SetVertexBuffer(mGridBuffer);
	mContext->SetWorldMatrix(lcMatrix44Identity());

	if (Preferences.mDrawGridStuds)
	{
		mContext->BindTexture2D(gGridTexture);
		mContext->SetDepthWrite(false);
		mContext->EnableColorBlend(true);

		mContext->SetMaterial(lcMaterialType::UnlitTextureModulate);
		mContext->SetColor(lcVector4FromColor(Preferences.mGridStudColor));

		mContext->SetVertexFormat(0, 3, 0, 2, 0, false);
		mContext->DrawPrimitives(GL_TRIANGLE_STRIP, 0, 4);

		mContext->EnableColorBlend(false);
		mContext->SetDepthWrite(true);

		BufferOffset = 4 * 5 * sizeof(float);
	}

	if (Preferences.mDrawGridLines)
	{
		mContext->SetLineWidth(1.0f);
		mContext->SetMaterial(lcMaterialType::UnlitColor);
		mContext->SetColor(lcVector4FromColor(Preferences.mGridLineColor));

		int NumVerts = 2 * (MaxX - MinX + MaxY - MinY + 2);

		mContext->SetVertexFormat(BufferOffset, 3, 0, 0, 0, false);
		mContext->DrawPrimitives(GL_LINES, 0, NumVerts);
	}

	if (Preferences.mDrawGridOrigin)
	{
		struct lcGridVertex
		{
			float x, y;
			quint32 Color;
		};

		const quint32 Red = LC_RGBA(204, 0, 0, 255);
		const quint32 Green = LC_RGBA(0, 204, 0, 255);
		const float Scale = 20.0f * Spacing;

		const lcGridVertex Verts[4] =
		{
			{ 0.0f, MinY * Scale, Green }, { 0.0f, MaxY * Scale, Green }, { MinX * Scale, 0.0f, Red }, { MaxX * Scale, 0.0f, Red }
		};

		mContext->SetMaterial(lcMaterialType::UnlitVertexColor);
		mContext->SetVertexBufferPointer(Verts);
		mContext->SetVertexFormat(0, 2, 0, 0, 4, false);

		mContext->DrawPrimitives(GL_LINES, 0, 4);
	}
}

lcTrackTool lcView::GetOverrideTrackTool(Qt::MouseButton Button) const
{
	if (mTrackToolFromOverlay)
		return lcTrackTool::None;

	lcTool OverrideTool = gMouseShortcuts.GetTool(Button, mMouseModifiers);

	if (OverrideTool == lcTool::Count)
		return lcTrackTool::None;

	constexpr lcTrackTool TrackToolFromTool[] =
	{
	    lcTrackTool::Insert,      // lcTool::Insert
	    lcTrackTool::PointLight,  // lcTool::Light
	    lcTrackTool::SpotLight,   // lcTool::SpotLight
	    lcTrackTool::Camera,      // lcTool::Camera
	    lcTrackTool::Select,      // lcTool::Select
	    lcTrackTool::MoveXYZ,     // lcTool::Move
	    lcTrackTool::RotateXYZ,   // lcTool::Rotate
	    lcTrackTool::Eraser,      // lcTool::Eraser
	    lcTrackTool::Paint,       // lcTool::Paint
	    lcTrackTool::ColorPicker, // lcTool::ColorPicker
	    lcTrackTool::Zoom,        // lcTool::Zoom
	    lcTrackTool::Pan,         // lcTool::Pan
	    lcTrackTool::OrbitXY,     // lcTool::RotateView
	    lcTrackTool::Roll,        // lcTool::Roll
	    lcTrackTool::ZoomRegion   // lcTool::ZoomRegion
	};

	LC_ARRAY_SIZE_CHECK(TrackToolFromTool, lcTool::Count);

	return TrackToolFromTool[static_cast<int>(OverrideTool)];
}

float lcView::GetOverlayScale() const
{
	lcVector3 OverlayCenter;
	lcMatrix33 RelativeRotation;
	lcModel* ActiveModel = GetActiveModel();
	ActiveModel->GetMoveRotateTransform(OverlayCenter, RelativeRotation);

	lcMatrix44 WorldMatrix = lcMatrix44(RelativeRotation, OverlayCenter);

	if (ActiveModel != mModel)
		WorldMatrix = lcMul(WorldMatrix, mActiveSubmodelTransform);

	lcVector3 ScreenPos = ProjectPoint(WorldMatrix.GetTranslation());
	ScreenPos[0] += 10.0f;
	lcVector3 Point = UnprojectPoint(ScreenPos);

	lcVector3 Dist(Point - WorldMatrix.GetTranslation());
	return Dist.Length() * 5.0f;
}

void lcView::BeginDrag(lcDragState DragState)
{
	mDragState = DragState;
	UpdateTrackTool();
}

void lcView::EndDrag(bool Accept)
{
	lcModel* ActiveModel = GetActiveModel();

	if (Accept)
	{
		switch (mDragState)
		{
		case lcDragState::None:
			break;

		case lcDragState::Piece:
			{
				PieceInfo* Info = gMainWindow->GetCurrentPieceInfo();
				if (Info)
					ActiveModel->InsertPieceToolClicked(GetPieceInsertPosition(false, Info));
			} break;

		case lcDragState::Color:
			ActiveModel->PaintToolClicked(FindObjectUnderPointer(true, false).Object);
			break;
		}
	}

	mDragState = lcDragState::None;
	UpdateTrackTool();
	ActiveModel->UpdateAllViews();
}

void lcView::SetViewpoint(lcViewpoint Viewpoint)
{
	if (!mCamera || !mCamera->IsSimple())
	{
		lcCamera* OldCamera = mCamera;

		mCamera = new lcCamera(true);

		if (OldCamera)
			mCamera->CopySettings(OldCamera);
	}

	mCamera->SetViewpoint(Viewpoint);
	ZoomExtents();
	Redraw();

	emit CameraChanged();
}

void lcView::SetViewpoint(const lcVector3& Position)
{
	if (!mCamera || !mCamera->IsSimple())
	{
		lcCamera* OldCamera = mCamera;

		mCamera = new lcCamera(true);

		if (OldCamera)
			mCamera->CopySettings(OldCamera);
	}

	mCamera->SetViewpoint(Position);
	ZoomExtents();
	Redraw();

	emit CameraChanged();
}

void lcView::SetViewpoint(const lcVector3& Position, const lcVector3& Target, const lcVector3& Up)
{
	if (!mCamera || !mCamera->IsSimple())
	{
		lcCamera* OldCamera = mCamera;

		mCamera = new lcCamera(true);

		if (OldCamera)
			mCamera->CopySettings(OldCamera);
	}

	mCamera->SetViewpoint(Position, Target, Up);
	Redraw();

	emit CameraChanged();
}

void lcView::SetCameraAngles(float Latitude, float Longitude)
{
	if (!mCamera || !mCamera->IsSimple())
	{
		lcCamera* OldCamera = mCamera;

		mCamera = new lcCamera(true);

		if (OldCamera)
			mCamera->CopySettings(OldCamera);
	}

	mCamera->SetAngles(Latitude, Longitude, 1.0f);
	ZoomExtents();
	Redraw();
}

void lcView::SetDefaultCamera()
{
	if (!mCamera || !mCamera->IsSimple())
		mCamera = new lcCamera(true);

	mCamera->SetViewpoint(lcViewpoint::Home);
	ZoomExtents();
	Redraw();

	emit CameraChanged();
}

void lcView::SetCamera(lcCamera* Camera, bool ForceCopy)
{
	if (Camera->IsSimple() || ForceCopy)
	{
		if (!mCamera || !mCamera->IsSimple())
			mCamera = new lcCamera(true);

		mCamera->CopyPosition(Camera);
	}
	else
	{
		if (mCamera && mCamera->IsSimple())
			delete mCamera;

		mCamera = Camera;
	}
}

void lcView::SetCamera(const QString& CameraName)
{
	const lcArray<lcCamera*>& Cameras = mModel->GetCameras();

	for (int CameraIdx = 0; CameraIdx < Cameras.GetSize(); CameraIdx++)
	{
		if (CameraName.compare(Cameras[CameraIdx]->GetName(), Qt::CaseInsensitive) == 0)
		{
			SetCameraIndex(CameraIdx);
			return;
		}
	}
}

void lcView::SetCameraIndex(int Index)
{
	const lcArray<lcCamera*>& Cameras = mModel->GetCameras();

	if (Index >= Cameras.GetSize())
		return;

	lcCamera* Camera = Cameras[Index];
	SetCamera(Camera, false);

	emit CameraChanged();
	Redraw();
}

void lcView::SetProjection(bool Ortho)
{
	if (mCamera->IsSimple())
	{
		mCamera->SetOrtho(Ortho);
		Redraw();

		if (gMainWindow)
			gMainWindow->UpdatePerspective();
	}
	else
	{
		lcModel* ActiveModel = GetActiveModel();
		if (ActiveModel)
			ActiveModel->SetCameraOrthographic(mCamera, Ortho);
	}
}

void lcView::LookAt()
{
	lcModel* ActiveModel = GetActiveModel();
	if (ActiveModel)
		ActiveModel->LookAt(mCamera);
}

void lcView::MoveCamera(const lcVector3& Direction)
{
	lcModel* ActiveModel = GetActiveModel();
	if (ActiveModel)
		ActiveModel->MoveCamera(mCamera, Direction);
}

void lcView::Zoom(float Amount)
{
	lcModel* ActiveModel = GetActiveModel();
	if (ActiveModel)
		ActiveModel->Zoom(mCamera, Amount);
}

void lcView::ZoomExtents()
{
	lcModel* ActiveModel = GetActiveModel();
	if (ActiveModel)
		ActiveModel->ZoomExtents(mCamera, (float)mWidth / (float)mHeight);
}

lcCursor lcView::GetCursor() const
{
	if (mTrackButton != lcTrackButton::None)
		return lcCursor::Hidden;

	if (mTrackTool == lcTrackTool::Select)
	{
		if (mMouseModifiers & Qt::ControlModifier)
			return lcCursor::SelectAdd;

		if (mMouseModifiers & Qt::ShiftModifier)
			return lcCursor::SelectRemove;
	}

	constexpr lcCursor CursorFromTrackTool[] =
	{
		lcCursor::Select,      // lcTrackTool::None
		lcCursor::Brick,       // lcTrackTool::Insert
		lcCursor::Light,       // lcTrackTool::PointLight
		lcCursor::Spotlight,   // lcTrackTool::SpotLight
		lcCursor::Camera,      // lcTrackTool::Camera
		lcCursor::Select,      // lcTrackTool::Select
		lcCursor::Move,        // lcTrackTool::MoveX
		lcCursor::Move,        // lcTrackTool::MoveY
		lcCursor::Move,        // lcTrackTool::MoveZ
		lcCursor::Move,        // lcTrackTool::MoveXY
		lcCursor::Move,        // lcTrackTool::MoveXZ
		lcCursor::Move,        // lcTrackTool::MoveYZ
		lcCursor::Move,        // lcTrackTool::MoveXYZ
		lcCursor::Rotate,      // lcTrackTool::RotateX
		lcCursor::Rotate,      // lcTrackTool::RotateY
		lcCursor::Rotate,      // lcTrackTool::RotateZ
		lcCursor::Rotate,      // lcTrackTool::RotateXY
		lcCursor::Rotate,      // lcTrackTool::RotateXYZ
		lcCursor::Move,        // lcTrackTool::ScalePlus
		lcCursor::Move,        // lcTrackTool::ScaleMinus
		lcCursor::Delete,      // lcTrackTool::Eraser
		lcCursor::Paint,       // lcTrackTool::Paint
		lcCursor::ColorPicker, // lcTrackTool::ColorPicker
		lcCursor::Zoom,        // lcTrackTool::Zoom
		lcCursor::Pan,         // lcTrackTool::Pan
		lcCursor::RotateX,     // lcTrackTool::OrbitX
		lcCursor::RotateY,     // lcTrackTool::OrbitY
		lcCursor::RotateView,  // lcTrackTool::OrbitXY
		lcCursor::Roll,        // lcTrackTool::Roll
		lcCursor::ZoomRegion   // lcTrackTool::ZoomRegion
	};

	LC_ARRAY_SIZE_CHECK(CursorFromTrackTool, lcTrackTool::Count);

	if (mTrackTool >= lcTrackTool::None && mTrackTool < lcTrackTool::Count)
		return CursorFromTrackTool[static_cast<int>(mTrackTool)];

	return lcCursor::Select;
}

void lcView::SetCursor(lcCursor CursorType)
{
	if (mCursor == CursorType)
		return;

	struct lcCursorInfo
	{
		int x, y;
		const char* Name;
	};

	constexpr lcCursorInfo Cursors[] =
	{
		{  0,  0, "" },                                 // lcCursor::Hidden
		{  0,  0, "" },                                 // lcCursor::Default
		{  8,  3, ":/resources/cursor_insert" },        // lcCursor::Brick
		{ 15, 15, ":/resources/cursor_light" },         // lcCursor::Light
		{  7, 10, ":/resources/cursor_spotlight" },     // lcCursor::Spotlight
		{ 15,  9, ":/resources/cursor_camera" },        // lcCursor::Camera
		{  0,  2, ":/resources/cursor_select" },        // lcCursor::Select
		{  0,  2, ":/resources/cursor_select_add" },    // lcCursor::SelectAdd
		{  0,  2, ":/resources/cursor_select_remove" }, // lcCursor::SelectRemove
		{ 15, 15, ":/resources/cursor_move" },          // lcCursor::Move
		{ 15, 15, ":/resources/cursor_rotate" },        // lcCursor::Rotate
		{ 15, 15, ":/resources/cursor_rotatex" },       // lcCursor::RotateX
		{ 15, 15, ":/resources/cursor_rotatey" },       // lcCursor::RotateY
		{  0, 10, ":/resources/cursor_delete" },        // lcCursor::Delete
		{ 14, 14, ":/resources/cursor_paint" },         // lcCursor::Paint
		{  1, 13, ":/resources/cursor_color_picker" },  // lcCursor::ColorPicker
		{ 15, 15, ":/resources/cursor_zoom" },          // lcCursor::Zoom
		{  9,  9, ":/resources/cursor_zoom_region" },   // lcCursor::ZoomRegion
		{ 15, 15, ":/resources/cursor_pan" },           // lcCursor::Pan
		{ 15, 15, ":/resources/cursor_roll" },          // lcCursor::Roll
		{ 15, 15, ":/resources/cursor_rotate_view" },   // lcCursor::RotateView
	};

	LC_ARRAY_SIZE_CHECK(Cursors, lcCursor::Count);

	if (CursorType == lcCursor::Hidden)
	{
		mWidget->setCursor(Qt::BlankCursor);
		mCursor = CursorType;
	}
	else if (CursorType >= lcCursor::First && CursorType < lcCursor::Count)
	{
		const lcCursorInfo& Cursor = Cursors[static_cast<int>(CursorType)];
		mWidget->setCursor(QCursor(QPixmap(Cursor.Name), Cursor.x, Cursor.y));
		mCursor = CursorType;
	}
	else
	{
		mWidget->unsetCursor();
		mCursor = lcCursor::Default;
	}
}

void lcView::UpdateCursor()
{
	SetCursor(GetCursor());
}

lcTool lcView::GetCurrentTool() const
{
	constexpr lcTool ToolFromTrackTool[] =
	{
		lcTool::Select,      // lcTrackTool::None
		lcTool::Insert,      // lcTrackTool::Insert
		lcTool::Light,       // lcTrackTool::PointLight
		lcTool::SpotLight,   // lcTrackTool::SpotLight
		lcTool::Camera,      // lcTrackTool::Camera
		lcTool::Select,      // lcTrackTool::Select
		lcTool::Move,        // lcTrackTool::MoveX
		lcTool::Move,        // lcTrackTool::MoveY
		lcTool::Move,        // lcTrackTool::MoveZ
		lcTool::Move,        // lcTrackTool::MoveXY
		lcTool::Move,        // lcTrackTool::MoveXZ
		lcTool::Move,        // lcTrackTool::MoveYZ
		lcTool::Move,        // lcTrackTool::MoveXYZ
		lcTool::Rotate,      // lcTrackTool::RotateX
		lcTool::Rotate,      // lcTrackTool::RotateY
		lcTool::Rotate,      // lcTrackTool::RotateZ
		lcTool::Rotate,      // lcTrackTool::RotateXY
		lcTool::Rotate,      // lcTrackTool::RotateXYZ
		lcTool::Move,        // lcTrackTool::ScalePlus
		lcTool::Move,        // lcTrackTool::ScaleMinus
		lcTool::Eraser,      // lcTrackTool::Eraser
		lcTool::Paint,       // lcTrackTool::Paint
		lcTool::ColorPicker, // lcTrackTool::ColorPicker
		lcTool::Zoom,        // lcTrackTool::Zoom
		lcTool::Pan,         // lcTrackTool::Pan
		lcTool::RotateView,  // lcTrackTool::OrbitX
		lcTool::RotateView,  // lcTrackTool::OrbitY
		lcTool::RotateView,  // lcTrackTool::OrbitXY
		lcTool::Roll,        // lcTrackTool::Roll
		lcTool::ZoomRegion   // lcTrackTool::ZoomRegion
	};

	LC_ARRAY_SIZE_CHECK(ToolFromTrackTool, lcTrackTool::Count);

	if (mTrackTool >= lcTrackTool::None && mTrackTool < lcTrackTool::Count)
		return ToolFromTrackTool[static_cast<int>(mTrackTool)];

	return lcTool::Select;
}

void lcView::UpdateTrackTool()
{
	if (mViewType != lcViewType::View)
	{
		mTrackTool = lcTrackTool::None;
		UpdateCursor();
		return;
	}

	lcTool CurrentTool = gMainWindow->GetTool();
	lcTrackTool NewTrackTool = mTrackTool;
	int x = mMouseX;
	int y = mMouseY;
	bool Redraw = false;
	mTrackToolFromOverlay = false;
	lcModel* ActiveModel = GetActiveModel();

	switch (CurrentTool)
	{
	case lcTool::Insert:
		NewTrackTool = lcTrackTool::Insert;
		break;

	case lcTool::Light:
		NewTrackTool = lcTrackTool::PointLight;
		break;

	case lcTool::SpotLight:
		NewTrackTool = lcTrackTool::SpotLight;
		break;

	case lcTool::Camera:
		NewTrackTool = lcTrackTool::Camera;
		break;

	case lcTool::Select:
	case lcTool::Move:
		{
			mMouseDownPiece = nullptr;
			NewTrackTool = mViewManipulator->UpdateSelectMove();
			mTrackToolFromOverlay = NewTrackTool != lcTrackTool::MoveXYZ && NewTrackTool != lcTrackTool::Select;
			Redraw = NewTrackTool != mTrackTool;

			if (CurrentTool == lcTool::Select && NewTrackTool == lcTrackTool::Select && mMouseModifiers == Qt::NoModifier)
			{
				lcObjectSection ObjectSection = FindObjectUnderPointer(false, false);
				lcObject* Object = ObjectSection.Object;

				if (Object && Object->IsPiece() && ObjectSection.Section == LC_PIECE_SECTION_POSITION && Object->IsSelected())
				{
					lcPiece* Piece = (lcPiece*)Object;
					mMouseDownPosition = Piece->mModelWorld.GetTranslation();
					mMouseDownPiece = Piece->mPieceInfo;
					NewTrackTool = lcTrackTool::MoveXYZ;
				}
			}
		}
		break;

	case lcTool::Rotate:
		{
			NewTrackTool = mViewManipulator->UpdateRotate();
			mTrackToolFromOverlay = NewTrackTool != lcTrackTool::RotateXYZ;
			Redraw = NewTrackTool != mTrackTool;
		}
		break;

	case lcTool::Eraser:
		NewTrackTool = lcTrackTool::Eraser;
		break;

	case lcTool::Paint:
		NewTrackTool = lcTrackTool::Paint;
		break;

	case lcTool::ColorPicker:
		NewTrackTool = lcTrackTool::ColorPicker;
		break;

	case lcTool::Zoom:
		NewTrackTool = lcTrackTool::Zoom;
		break;

	case lcTool::Pan:
		NewTrackTool = lcTrackTool::Pan;
		break;

	case lcTool::RotateView:
		{
			int vx, vy, vw, vh;

			vx = 0;
			vy = 0;
			vw = mWidth;
			vh = mHeight;

			int cx = vx + vw / 2;
			int cy = vy + vh / 2;

			float d = sqrtf((float)((cx - x) * (cx - x) + (cy - y) * (cy - y)));
			float r = lcMin(vw, vh) * 0.35f;

			const float SquareSize = lcMax(8.0f, (vw + vh) / 200.0f);

			if ((d < r + SquareSize) && (d > r - SquareSize))
			{
				if ((cx - x < SquareSize) && (cx - x > -SquareSize))
				{
					NewTrackTool = lcTrackTool::OrbitY;
					mTrackToolFromOverlay = true;
				}

				if ((cy - y < SquareSize) && (cy - y > -SquareSize))
				{
					NewTrackTool = lcTrackTool::OrbitX;
					mTrackToolFromOverlay = true;
				}
			}
			else
			{
				if (d < r)
					NewTrackTool = lcTrackTool::OrbitXY;
				else
					NewTrackTool = lcTrackTool::Roll;
			}
		}
		break;

	case lcTool::Roll:
		NewTrackTool = lcTrackTool::Roll;
		break;

	case lcTool::ZoomRegion:
		NewTrackTool = lcTrackTool::ZoomRegion;
		break;

	case lcTool::Count:
		break;
	}

	switch (mDragState)
	{
	case lcDragState::None:
		break;

	case lcDragState::Piece:
		NewTrackTool = lcTrackTool::Insert;
		Redraw = true;
		break;

	case lcDragState::Color:
		NewTrackTool = lcTrackTool::Paint;
		break;
	}

	mTrackTool = NewTrackTool;
	UpdateCursor();

	if (Redraw)
		ActiveModel->UpdateAllViews();
}

void lcView::StartOrbitTracking()
{
	mTrackTool = lcTrackTool::OrbitXY;
	UpdateCursor();
	OnButtonDown(lcTrackButton::Left);
}

void lcView::StartPanGesture()
{
	lcModel* ActiveModel = GetActiveModel();

	StartPan(mWidth / 2, mHeight / 2);
	ActiveModel->BeginMouseTool();
}

void lcView::UpdatePanGesture(int dx, int dy)
{
	UpdatePan(mPanX + dx, mPanY + dy);
}

void lcView::StartPan(int x, int y)
{
	mPanX = x;
	mPanY = y;
}

void lcView::UpdatePan(int x, int y)
{
	if (x == mPanX && y == mPanY)
		return;

	lcModel* ActiveModel = GetActiveModel();

	lcVector3 Points[4] =
	{
	    lcVector3((float)x, (float)y, 0.0f),
	    lcVector3((float)x, (float)y, 1.0f),
	    lcVector3(mPanX, mPanY, 0.0f),
	    lcVector3(mPanX, mPanY, 1.0f)
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
			return;
	}

	mPanX = x;
	mPanY = y;

	ActiveModel->UpdatePanTool(mCamera, MoveStart - Intersection);
}

void lcView::EndPanGesture(bool Accept)
{
	lcModel* ActiveModel = GetActiveModel();

	ActiveModel->EndMouseTool(lcTool::Pan, Accept);
}

void lcView::StartTracking(lcTrackButton TrackButton)
{
	mTrackButton = TrackButton;
	mTrackUpdated = false;
	mMouseDownX = mMouseX;
	mMouseDownY = mMouseY;
	lcTool Tool = GetCurrentTool();
	lcModel* ActiveModel = GetActiveModel();

	switch (Tool)
	{
		case lcTool::Insert:
		case lcTool::Light:
			break;

		case lcTool::SpotLight:
		{
			lcVector3 Position = GetCameraLightInsertPosition();
			lcVector3 Target = Position + lcVector3(0.1f, 0.1f, 0.1f);
			ActiveModel->BeginSpotLightTool(Position, Target);
		}
		break;

		case lcTool::Camera:
		{
			lcVector3 Position = GetCameraLightInsertPosition();
			lcVector3 Target = Position + lcVector3(0.1f, 0.1f, 0.1f);
			ActiveModel->BeginCameraTool(Position, Target);
		}
		break;

		case lcTool::Select:
			break;

		case lcTool::Move:
		case lcTool::Rotate:
			ActiveModel->BeginMouseTool();
			break;

		case lcTool::Eraser:
		case lcTool::Paint:
		case lcTool::ColorPicker:
			break;

	    case lcTool::Pan:
		    StartPan(mMouseX, mMouseY);
			ActiveModel->BeginMouseTool();
		    break;

	    case lcTool::Zoom:
		case lcTool::RotateView:
		case lcTool::Roll:
			ActiveModel->BeginMouseTool();
			break;

		case lcTool::ZoomRegion:
			break;

		case lcTool::Count:
			break;
	}

	UpdateCursor();
}

void lcView::StopTracking(bool Accept)
{
	if (mTrackButton == lcTrackButton::None)
		return;

	lcTool Tool = GetCurrentTool();
	lcModel* ActiveModel = GetActiveModel();

	switch (Tool)
	{
	case lcTool::Insert:
	case lcTool::Light:
		break;

	case lcTool::SpotLight:
	case lcTool::Camera:
		ActiveModel->EndMouseTool(Tool, Accept);
		break;

	case lcTool::Select:
		if (Accept && mMouseDownX != mMouseX && mMouseDownY != mMouseY)
		{
			lcArray<lcObject*> Objects = FindObjectsInBox(mMouseDownX, mMouseDownY, mMouseX, mMouseY);

			if (mMouseModifiers & Qt::ControlModifier)
				ActiveModel->AddToSelection(Objects, true, true);
			else if (mMouseModifiers & Qt::ShiftModifier)
				ActiveModel->RemoveFromSelection(Objects);
			else
				ActiveModel->SetSelectionAndFocus(Objects, nullptr, 0, true);
		}
		break;

	case lcTool::Move:
	case lcTool::Rotate:
		ActiveModel->EndMouseTool(Tool, Accept);
		break;

	case lcTool::Eraser:
	case lcTool::Paint:
	case lcTool::ColorPicker:
		break;

	case lcTool::Zoom:
	case lcTool::Pan:
	case lcTool::RotateView:
	case lcTool::Roll:
		ActiveModel->EndMouseTool(Tool, Accept);
		break;

	case lcTool::ZoomRegion:
		{
			if (mMouseX == mMouseDownX || mMouseY == mMouseDownY)
				break;

			lcVector3 Points[6] =
			{
				lcVector3((mMouseDownX + lcMin(mMouseX, mWidth - 1)) / 2, (mMouseDownY + lcMin(mMouseY, mHeight - 1)) / 2, 0.0f),
				lcVector3((mMouseDownX + lcMin(mMouseX, mWidth - 1)) / 2, (mMouseDownY + lcMin(mMouseY, mHeight - 1)) / 2, 1.0f),
				lcVector3((float)mMouseX, (float)mMouseY, 0.0f),
				lcVector3((float)mMouseX, (float)mMouseY, 1.0f),
				lcVector3(mMouseDownX, mMouseDownY, 0.0f),
				lcVector3(mMouseDownX, mMouseDownY, 1.0f)
			};

			UnprojectPoints(Points, 5);

			lcVector3 Center = ActiveModel->GetSelectionOrModelCenter();

			lcVector3 PlaneNormal(mCamera->mPosition - mCamera->mTargetPosition);
			lcVector4 Plane(PlaneNormal, -lcDot(PlaneNormal, Center));
			lcVector3 Target, Corners[2];

			if (lcLineSegmentPlaneIntersection(&Target, Points[0], Points[1], Plane) && lcLineSegmentPlaneIntersection(&Corners[0], Points[2], Points[3], Plane) && lcLineSegmentPlaneIntersection(&Corners[1], Points[3], Points[4], Plane))
			{
				float AspectRatio = (float)mWidth / (float)mHeight;
				ActiveModel->ZoomRegionToolClicked(mCamera, AspectRatio, Points[0], Target, Corners);
			}
		}
		break;

	case lcTool::Count:
		break;
	}

	mTrackButton = lcTrackButton::None;
	UpdateTrackTool();
	ActiveModel->UpdateAllViews();
}

void lcView::CancelTrackingOrClearSelection()
{
	if (mTrackButton != lcTrackButton::None)
		StopTracking(false);
	else
	{
		lcModel* ActiveModel = GetActiveModel();
		if (ActiveModel)
			ActiveModel->ClearSelection(true);
	}
}

void lcView::OnButtonDown(lcTrackButton TrackButton)
{
	lcModel* ActiveModel = GetActiveModel();
	mToolClicked = false;

	switch (mTrackTool)
	{
	case lcTrackTool::None:
		break;

	case lcTrackTool::Insert:
		{
			PieceInfo* CurPiece = gMainWindow->GetCurrentPieceInfo();

			if (!CurPiece)
				break;

			ActiveModel->InsertPieceToolClicked(GetPieceInsertPosition(false, gMainWindow->GetCurrentPieceInfo()));

			if ((mMouseModifiers & Qt::ControlModifier) == 0)
				gMainWindow->SetTool(lcTool::Select);

			mToolClicked = true;
			UpdateTrackTool();
		}
		break;

	case lcTrackTool::PointLight:
		{
			ActiveModel->PointLightToolClicked(GetCameraLightInsertPosition());

			if ((mMouseModifiers & Qt::ControlModifier) == 0)
				gMainWindow->SetTool(lcTool::Select);

			mToolClicked = true;
			UpdateTrackTool();
		}
		break;

	case lcTrackTool::SpotLight:
	case lcTrackTool::Camera:
		StartTracking(TrackButton);
		break;
		
	case lcTrackTool::Select:
		{
			lcObjectSection ObjectSection = FindObjectUnderPointer(false, false);

			if (mMouseModifiers & Qt::ControlModifier)
				ActiveModel->FocusOrDeselectObject(ObjectSection);
			else if (mMouseModifiers & Qt::ShiftModifier)
				ActiveModel->RemoveFromSelection(ObjectSection);
			else
				ActiveModel->ClearSelectionAndSetFocus(ObjectSection, true);

			StartTracking(TrackButton);
		}
		break;

	case lcTrackTool::MoveX:
	case lcTrackTool::MoveY:
	case lcTrackTool::MoveZ:
	case lcTrackTool::MoveXY:
	case lcTrackTool::MoveXZ:
	case lcTrackTool::MoveYZ:
	case lcTrackTool::MoveXYZ:
		if (ActiveModel->AnyObjectsSelected())
			StartTracking(TrackButton);
		break;

	case lcTrackTool::RotateX:
	case lcTrackTool::RotateY:
	case lcTrackTool::RotateZ:
	case lcTrackTool::RotateXY:
	case lcTrackTool::RotateXYZ:
		if (ActiveModel->AnyPiecesSelected())
			StartTracking(TrackButton);
		break;

	case lcTrackTool::ScalePlus:
	case lcTrackTool::ScaleMinus:
		if (ActiveModel->AnyPiecesSelected())
			StartTracking(TrackButton);
		break;

	case lcTrackTool::Eraser:
		ActiveModel->EraserToolClicked(FindObjectUnderPointer(false, false).Object);
		mToolClicked = true;
		break;

	case lcTrackTool::Paint:
		ActiveModel->PaintToolClicked(FindObjectUnderPointer(true, false).Object);
		mToolClicked = true;
		break;

	case lcTrackTool::ColorPicker:
		ActiveModel->ColorPickerToolClicked(FindObjectUnderPointer(true, false).Object);
		mToolClicked = true;
		break;

	case lcTrackTool::Zoom:
	case lcTrackTool::Pan:
	case lcTrackTool::OrbitX:
	case lcTrackTool::OrbitY:
	case lcTrackTool::OrbitXY:
	case lcTrackTool::Roll:
	case lcTrackTool::ZoomRegion:
		StartTracking(TrackButton);
		break;

	case lcTrackTool::Count:
		break;
	}
}

void lcView::OnLeftButtonDown()
{
	if (mTrackButton != lcTrackButton::None)
	{
		StopTracking(false);
		return;
	}

	if (mViewSphere->OnLeftButtonDown())
		return;

	lcTrackTool OverrideTool = GetOverrideTrackTool(Qt::LeftButton);

	if (OverrideTool != lcTrackTool::None)
	{
		mTrackTool = OverrideTool;
		UpdateCursor();
	}

	OnButtonDown(lcTrackButton::Left);
}

void lcView::OnLeftButtonUp()
{
	StopTracking(mTrackButton == lcTrackButton::Left);

	if (mViewSphere->OnLeftButtonUp())
		return;
}

void lcView::OnLeftButtonDoubleClick()
{
	if (mViewType != lcViewType::View)
	{
		ZoomExtents();
		return;
	}

	lcObjectSection ObjectSection = FindObjectUnderPointer(false, false);
	lcModel* ActiveModel = GetActiveModel();

	if (mMouseModifiers & Qt::ControlModifier)
		ActiveModel->FocusOrDeselectObject(ObjectSection);
	else if (mMouseModifiers & Qt::ShiftModifier)
		ActiveModel->RemoveFromSelection(ObjectSection);
	else
		ActiveModel->ClearSelectionAndSetFocus(ObjectSection, true);
}

void lcView::OnMiddleButtonDown()
{
	if (mTrackButton != lcTrackButton::None)
	{
		StopTracking(false);
		return;
	}

	lcTrackTool OverrideTool = GetOverrideTrackTool(Qt::MiddleButton);

	if (OverrideTool != lcTrackTool::None)
	{
		mTrackTool = OverrideTool;
		UpdateCursor();
	}

	OnButtonDown(lcTrackButton::Middle);
}

void lcView::OnMiddleButtonUp()
{
	StopTracking(mTrackButton == lcTrackButton::Middle);
}

void lcView::OnRightButtonDown()
{
	if (mTrackButton != lcTrackButton::None)
	{
		StopTracking(false);
		return;
	}

	lcTrackTool OverrideTool = GetOverrideTrackTool(Qt::RightButton);

	if (OverrideTool != lcTrackTool::None)
	{
		mTrackTool = OverrideTool;
		UpdateCursor();
	}

	OnButtonDown(lcTrackButton::Right);
}

void lcView::OnRightButtonUp()
{
	bool ShowMenu = !mToolClicked && (mTrackButton == lcTrackButton::None || !mTrackUpdated);

	if (mTrackButton != lcTrackButton::None)
		StopTracking(mTrackButton == lcTrackButton::Right);

	if (ShowMenu)
		ShowContextMenu();
}

void lcView::OnBackButtonDown()
{
}

void lcView::OnBackButtonUp()
{
	gMainWindow->HandleCommand(LC_VIEW_TIME_PREVIOUS);
}

void lcView::OnForwardButtonDown()
{
}

void lcView::OnForwardButtonUp()
{
	gMainWindow->HandleCommand(LC_VIEW_TIME_NEXT);
}

void lcView::OnMouseMove()
{
	lcModel* ActiveModel = GetActiveModel();

	if (!ActiveModel)
		return;

	if (mTrackButton == lcTrackButton::None)
	{
		if (mViewSphere->OnMouseMove())
		{
			lcTrackTool NewTrackTool = mViewSphere->IsDragging() ? lcTrackTool::OrbitXY : lcTrackTool::None;

			if (NewTrackTool != mTrackTool)
			{
				mTrackTool = NewTrackTool;
				UpdateCursor();
			}

			return;
		}

		UpdateTrackTool();

		if (mTrackTool == lcTrackTool::Insert)
			ActiveModel->UpdateAllViews();

		return;
	}

	mTrackUpdated = true;
	const float MouseSensitivity = 0.5f / (21.0f - lcGetPreferences().mMouseSensitivity);

	switch (mTrackTool)
	{
	case lcTrackTool::None:
	case lcTrackTool::Insert:
	case lcTrackTool::PointLight:
		break;

	case lcTrackTool::SpotLight:
		ActiveModel->UpdateSpotLightTool(GetCameraLightInsertPosition());
		break;

	case lcTrackTool::Camera:
		ActiveModel->UpdateCameraTool(GetCameraLightInsertPosition());
		break;

	case lcTrackTool::Select:
		Redraw();
		break;

	case lcTrackTool::MoveX:
	case lcTrackTool::MoveY:
	case lcTrackTool::MoveZ:
	case lcTrackTool::MoveXY:
	case lcTrackTool::MoveXZ:
	case lcTrackTool::MoveYZ:
	case lcTrackTool::MoveXYZ:
	case lcTrackTool::ScalePlus:
	case lcTrackTool::ScaleMinus:
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

			lcVector3 OverlayCenter;
			lcMatrix33 RelativeRotation;
			ActiveModel->GetMoveRotateTransform(OverlayCenter, RelativeRotation);

			lcMatrix44 WorldMatrix = lcMatrix44(RelativeRotation, OverlayCenter);

			if (ActiveModel != mModel)
				WorldMatrix = lcMul(WorldMatrix, mActiveSubmodelTransform);

			const lcVector3 Center = WorldMatrix.GetTranslation();

			if (mTrackTool == lcTrackTool::MoveX || mTrackTool == lcTrackTool::MoveY || mTrackTool == lcTrackTool::MoveZ)
			{
				lcVector3 Direction;
				if (mTrackTool == lcTrackTool::MoveX)
					Direction = lcVector3(1.0f, 0.0f, 0.0f);
				else if (mTrackTool == lcTrackTool::MoveY)
					Direction = lcVector3(0.0f, 1.0f, 0.0f);
				else
					Direction = lcVector3(0.0f, 0.0f, 1.0f);

				Direction = lcMul30(Direction, WorldMatrix);

				lcVector3 Intersection;
				lcClosestPointsBetweenLines(Center, Center + Direction, CurrentStart, CurrentEnd, &Intersection, nullptr);

				lcVector3 MoveStart;
				lcClosestPointsBetweenLines(Center, Center + Direction, MouseDownStart, MouseDownEnd, &MoveStart, nullptr);

				lcVector3 Distance = Intersection - MoveStart;
				Distance = lcMul(Distance, lcMatrix33AffineInverse(lcMatrix33(WorldMatrix)));
				ActiveModel->UpdateMoveTool(Distance, true, mTrackButton != lcTrackButton::Left);
			}
			else if (mTrackTool == lcTrackTool::MoveXY || mTrackTool == lcTrackTool::MoveXZ || mTrackTool == lcTrackTool::MoveYZ)
			{
				lcVector3 PlaneNormal;

				if (mTrackTool == lcTrackTool::MoveXY)
					PlaneNormal = lcVector3(0.0f, 0.0f, 1.0f);
				else if (mTrackTool == lcTrackTool::MoveXZ)
					PlaneNormal = lcVector3(0.0f, 1.0f, 0.0f);
				else
					PlaneNormal = lcVector3(1.0f, 0.0f, 0.0f);

				PlaneNormal = lcMul30(PlaneNormal, WorldMatrix);
				lcVector4 Plane(PlaneNormal, -lcDot(PlaneNormal, Center));
				lcVector3 Intersection;

				if (lcLineSegmentPlaneIntersection(&Intersection, CurrentStart, CurrentEnd, Plane))
				{
					lcVector3 MoveStart;

					if (lcLineSegmentPlaneIntersection(&MoveStart, MouseDownStart, MouseDownEnd, Plane))
					{
						lcVector3 Distance = Intersection - MoveStart;
						Distance = lcMul(Distance, lcMatrix33AffineInverse(lcMatrix33(WorldMatrix)));
						ActiveModel->UpdateMoveTool(Distance, true, mTrackButton != lcTrackButton::Left);
					}
				}
			}
			else if (mTrackTool == lcTrackTool::MoveXYZ && mMouseDownPiece)
			{
				lcMatrix44 NewPosition = GetPieceInsertPosition(true, mMouseDownPiece);
				lcVector3 Distance = NewPosition.GetTranslation() - mMouseDownPosition;
				ActiveModel->UpdateMoveTool(Distance, false, mTrackButton != lcTrackButton::Left);
			}
			else if (mTrackTool == lcTrackTool::ScalePlus || mTrackTool == lcTrackTool::ScaleMinus)
			{
				lcVector3 Direction;
				if (mTrackTool == lcTrackTool::ScalePlus)
					Direction = lcVector3(1.0f, 0.0f, 0.0f);
				else
					Direction = lcVector3(-1.0f, 0.0f, 0.0f);

				Direction = lcMul30(Direction, WorldMatrix);

				lcVector3 Intersection;
				lcClosestPointsBetweenLines(Center, Center + Direction, CurrentStart, CurrentEnd, &Intersection, nullptr);

				lcObject* Focus = ActiveModel->GetFocusObject();
				if (Focus && Focus->IsPiece())
				{
					lcPiece* Piece = (lcPiece*)Focus;
					quint32 Section = Piece->GetFocusSection();

					if (Section >= LC_PIECE_SECTION_CONTROL_POINT_FIRST && Section <= LC_PIECE_SECTION_CONTROL_POINT_LAST)
					{
						const float ScaleMax = 200.0f;
						const float OverlayScale = GetOverlayScale();
						const float ScaleStart = 2.0f * OverlayScale;

						lcVector3 Position = Piece->GetSectionPosition(Section);
						lcVector3 Start = Position + Direction * ScaleStart;

						float Distance = lcLength(Intersection - Start);
						if (lcDot(Direction, Intersection - Start) < 0.0f)
							Distance = 0.0f;

						float Scale = lcClamp(Distance, 0.1f, ScaleMax);

						ActiveModel->UpdateScaleTool(Scale);
					}
				}
			}
			else
			{
				lcVector3 PlaneNormal = lcNormalize(mCamera->mTargetPosition - mCamera->mPosition);
				lcVector4 Plane(PlaneNormal, -lcDot(PlaneNormal, Center));
				lcVector3 Intersection;

				if (lcLineSegmentPlaneIntersection(&Intersection, CurrentStart, CurrentEnd, Plane))
				{
					lcVector3 MoveStart;

					if (lcLineSegmentPlaneIntersection(&MoveStart, MouseDownStart, MouseDownEnd, Plane))
					{
						lcVector3 Distance = Intersection - MoveStart;
						ActiveModel->UpdateMoveTool(Distance, true, mTrackButton != lcTrackButton::Left);
					}
				}
			}
		}
		break;

	case lcTrackTool::RotateX:
	case lcTrackTool::RotateY:
	case lcTrackTool::RotateZ:
		{
			lcVector3 ScreenX = lcNormalize(lcCross(mCamera->mTargetPosition - mCamera->mPosition, mCamera->mUpVector));
			lcVector3 ScreenY = mCamera->mUpVector;
			lcVector3 Dir1;

			switch (mTrackTool)
			{
			case lcTrackTool::RotateX:
				Dir1 = lcVector3(1.0f, 0.0f, 0.0f);
				break;
			case lcTrackTool::RotateY:
				Dir1 = lcVector3(0.0f, 1.0f, 0.0f);
				break;
			case lcTrackTool::RotateZ:
				Dir1 = lcVector3(0.0f, 0.0f, 1.0f);
				break;
			default:
				Dir1 = lcVector3(0.0f, 0.0f, 1.0f);
				break;
			}

			lcVector3 MoveX, MoveY;

			float dx1 = lcDot(ScreenX, Dir1);
			float dy1 = lcDot(ScreenY, Dir1);

			if (fabsf(dx1) > fabsf(dy1))
			{
				if (dx1 >= 0.0f)
					MoveX = Dir1;
				else
					MoveX = -Dir1;

				MoveY = lcVector3(0, 0, 0);
			}
			else
			{
				MoveX = lcVector3(0, 0, 0);

				if (dy1 > 0.0f)
					MoveY = Dir1;
				else
					MoveY = -Dir1;
			}

			MoveX *= 36.0f * (float)(mMouseX - mMouseDownX) * MouseSensitivity;
			MoveY *= 36.0f * (float)(mMouseY - mMouseDownY) * MouseSensitivity;

			ActiveModel->UpdateRotateTool(MoveX + MoveY, mTrackButton != lcTrackButton::Left);
		}
		break;

	case lcTrackTool::RotateXY:
		{
			lcVector3 ScreenZ = lcNormalize(mCamera->mTargetPosition - mCamera->mPosition);
			lcVector3 ScreenX = lcCross(ScreenZ, mCamera->mUpVector);
			lcVector3 ScreenY = mCamera->mUpVector;

			lcVector3 MoveX = 36.0f * (float)(mMouseX - mMouseDownX) * MouseSensitivity * ScreenX;
			lcVector3 MoveY = 36.0f * (float)(mMouseY - mMouseDownY) * MouseSensitivity * ScreenY;
			ActiveModel->UpdateRotateTool(MoveX + MoveY, mTrackButton != lcTrackButton::Left);
		}
		break;

	case lcTrackTool::RotateXYZ:
		{
			lcVector3 ScreenZ = lcNormalize(mCamera->mTargetPosition - mCamera->mPosition);

			ActiveModel->UpdateRotateTool(36.0f * (float)(mMouseY - mMouseDownY) * MouseSensitivity * ScreenZ, mTrackButton != lcTrackButton::Left);
		}
		break;

	case lcTrackTool::Eraser:
	case lcTrackTool::Paint:
	case lcTrackTool::ColorPicker:
		break;

	case lcTrackTool::Zoom:
		ActiveModel->UpdateZoomTool(mCamera, 2.0f * MouseSensitivity * (mMouseY - mMouseDownY));
		break;

	case lcTrackTool::Pan:
		UpdatePan(mMouseX, mMouseY);
		break;

	case lcTrackTool::OrbitX:
		ActiveModel->UpdateOrbitTool(mCamera, 0.1f * MouseSensitivity * (mMouseX - mMouseDownX), 0.0f);
		break;

	case lcTrackTool::OrbitY:
		ActiveModel->UpdateOrbitTool(mCamera, 0.0f, 0.1f * MouseSensitivity * (mMouseY - mMouseDownY));
		break;

	case lcTrackTool::OrbitXY:
		ActiveModel->UpdateOrbitTool(mCamera, 0.1f * MouseSensitivity * (mMouseX - mMouseDownX), 0.1f * MouseSensitivity * (mMouseY - mMouseDownY));
		break;

	case lcTrackTool::Roll:
		ActiveModel->UpdateRollTool(mCamera, 2.0f * MouseSensitivity * (mMouseX - mMouseDownX) * LC_DTOR);
		break;

	case lcTrackTool::ZoomRegion:
		Redraw();
		break;

	case lcTrackTool::Count:
		break;
	}
}

void lcView::OnMouseWheel(float Direction)
{
	float Scale = 10.0f;

	if (mMouseModifiers & Qt::ControlModifier)
		Scale = 100.0f;
	else if (mMouseModifiers & Qt::ShiftModifier)
		Scale = 1.0f;

	mModel->Zoom(mCamera, static_cast<int>(Direction * Scale));
}
