#include "lc_global.h"
#include "lc_qglwidget.h"
#include "lc_glwidget.h"
#include "lc_glextensions.h"
#include "project.h"
#include "lc_library.h"
#include "lc_application.h"
#include "lc_mainwindow.h"
#include "lc_partselectionwidget.h"
#include "lc_context.h"
#include "view.h"
#include "texfont.h"
#include "lc_viewsphere.h"
#include "lc_stringcache.h"
#include "lc_texture.h"
#include "lc_mesh.h"
#include "lc_profile.h"

static QList<QGLWidget*> gWidgetList;

void lcGLWidget::MakeCurrent()
{
	QGLWidget* Widget = (QGLWidget*)mWidget;

	Widget->makeCurrent();
}

void lcGLWidget::Redraw()
{
	lcQGLWidget* Widget = (lcQGLWidget*)mWidget;

	Widget->mUpdateTimer.start(0);
}

void lcGLWidget::SetCursor(lcCursor CursorType)
{
	if (mCursor == CursorType)
		return;

	struct lcCursorInfo
	{
		int x, y;
		const char* Name;
	};

	const lcCursorInfo Cursors[] =
	{
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

	static_assert(LC_ARRAY_COUNT(Cursors) == static_cast<int>(lcCursor::Count), "Array size mismatch");

	QGLWidget* widget = (QGLWidget*)mWidget;

	if (CursorType != lcCursor::Default && CursorType < lcCursor::Count)
	{
		const lcCursorInfo& Cursor = Cursors[static_cast<int>(CursorType)];
		widget->setCursor(QCursor(QPixmap(Cursor.Name), Cursor.x, Cursor.y));
		mCursor = CursorType;
	}
	else
	{
		widget->unsetCursor();
		mCursor = lcCursor::Default;
	}
}

lcQGLWidget::lcQGLWidget(QWidget *parent, lcGLWidget *owner, bool view)
	: QGLWidget(parent, gWidgetList.isEmpty() ? nullptr : gWidgetList.first())
{
	mWheelAccumulator = 0;
	widget = owner;
	widget->mWidget = this;

	mUpdateTimer.setSingleShot(true);
	connect(&mUpdateTimer, SIGNAL(timeout()), this, SLOT(updateGL()));

	widget->MakeCurrent();

	if (gWidgetList.isEmpty())
	{
		// TODO: Find a better place for the grid texture and font
		gStringCache.Initialize(widget->mContext);
		gTexFont.Initialize(widget->mContext);

		lcInitializeGLExtensions(context());
		lcContext::CreateResources();
		View::CreateResources(widget->mContext);
		lcViewSphere::CreateResources(widget->mContext);

		if (!gSupportsShaderObjects && lcGetPreferences().mShadingMode == lcShadingMode::DefaultLights)
			lcGetPreferences().mShadingMode = lcShadingMode::Flat;

		if (!gSupportsFramebufferObjectARB && !gSupportsFramebufferObjectEXT)
			gMainWindow->GetPartSelectionWidget()->DisableIconMode();

		gPlaceholderMesh = new lcMesh;
		gPlaceholderMesh->CreateBox();
	}

	gWidgetList.append(this);

	widget->OnInitialUpdate();

	preferredSize = QSize(0, 0);
	setMouseTracking(true);

	mIsView = view;
	if (mIsView)
	{
		setFocusPolicy(Qt::StrongFocus);
		setAcceptDrops(true);
	}
}

lcQGLWidget::~lcQGLWidget()
{
	gWidgetList.removeOne(this);

	if (gWidgetList.isEmpty())
	{
		gStringCache.Reset();
		gTexFont.Reset();

		lcGetPiecesLibrary()->ReleaseBuffers(widget->mContext);
		View::DestroyResources(widget->mContext);
		lcContext::DestroyResources();
		lcViewSphere::DestroyResources(widget->mContext);

		delete gPlaceholderMesh;
		gPlaceholderMesh = nullptr;
	}

	delete widget;
}

QSize lcQGLWidget::sizeHint() const
{
	if (preferredSize.isEmpty())
		return QGLWidget::sizeHint();
	else
		return preferredSize;
}

void lcQGLWidget::initializeGL()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
}

void lcQGLWidget::resizeGL(int width, int height)
{
	widget->mWidth = width;
	widget->mHeight = height;
}

void lcQGLWidget::paintGL()
{
	widget->OnDraw();
}

void lcQGLWidget::keyPressEvent(QKeyEvent *event)
{
	if (mIsView && (event->key() == Qt::Key_Control || event->key() == Qt::Key_Shift))
	{
		widget->mInputState.Modifiers = event->modifiers();
		widget->OnUpdateCursor();
	}

	QGLWidget::keyPressEvent(event);
}

void lcQGLWidget::keyReleaseEvent(QKeyEvent *event)
{
	if (mIsView && (event->key() == Qt::Key_Control || event->key() == Qt::Key_Shift))
	{
		widget->mInputState.Modifiers = event->modifiers();
		widget->OnUpdateCursor();
	}

	QGLWidget::keyReleaseEvent(event);
}

void lcQGLWidget::mousePressEvent(QMouseEvent *event)
{
	float scale = deviceScale();

	widget->mInputState.x = event->x() * scale;
	widget->mInputState.y = widget->mHeight - event->y() * scale - 1;
	widget->mInputState.Modifiers = event->modifiers();

	switch (event->button())
	{
	case Qt::LeftButton:
		widget->OnLeftButtonDown();
		break;

	case Qt::MidButton:
		widget->OnMiddleButtonDown();
		break;

	case Qt::RightButton:
		widget->OnRightButtonDown();
		break;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
	case Qt::BackButton:
		widget->OnBackButtonDown();
		break;

	case Qt::ForwardButton:
		widget->OnForwardButtonDown();
		break;
#endif

	default:
		break;
	}
}

void lcQGLWidget::mouseReleaseEvent(QMouseEvent *event)
{
	float scale = deviceScale();

	widget->mInputState.x = event->x() * scale;
	widget->mInputState.y = widget->mHeight - event->y() * scale - 1;
	widget->mInputState.Modifiers = event->modifiers();

	switch (event->button())
	{
	case Qt::LeftButton:
		widget->OnLeftButtonUp();
		break;

	case Qt::MidButton:
		widget->OnMiddleButtonUp();
		break;

	case Qt::RightButton:
		widget->OnRightButtonUp();
		break;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
	case Qt::BackButton:
		widget->OnBackButtonUp();
		break;

	case Qt::ForwardButton:
		widget->OnForwardButtonUp();
		break;
#endif

	default:
		break;
	}
}

void lcQGLWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
	float scale = deviceScale();

	widget->mInputState.x = event->x() * scale;
	widget->mInputState.y = widget->mHeight - event->y() * scale - 1;
	widget->mInputState.Modifiers = event->modifiers();

	switch (event->button())
	{
	case Qt::LeftButton:
		widget->OnLeftButtonDoubleClick();
		break;
	default:
		break;
	}
}

void lcQGLWidget::mouseMoveEvent(QMouseEvent *event)
{
	float scale = deviceScale();

	widget->mInputState.x = event->x() * scale;
	widget->mInputState.y = widget->mHeight - event->y() * scale - 1;
	widget->mInputState.Modifiers = event->modifiers();

	widget->OnMouseMove();
}

void lcQGLWidget::wheelEvent(QWheelEvent *event)
{
	if ((event->orientation() & Qt::Vertical) == 0)
	{
		event->ignore();
		return;
	}

	float scale = deviceScale();

	widget->mInputState.x = event->x() * scale;
	widget->mInputState.y = widget->mHeight - event->y() * scale - 1;
	widget->mInputState.Modifiers = event->modifiers();

#if (QT_VERSION >= QT_VERSION_CHECK(5, 2, 0))
	mWheelAccumulator += event->angleDelta().y() / 8;
#else
	mWheelAccumulator += event->delta() / 8;
#endif
	int numSteps = mWheelAccumulator / 15;

	if (numSteps)
	{
		widget->OnMouseWheel(numSteps);
		mWheelAccumulator -= numSteps * 15;
	}

	event->accept();
}

void lcQGLWidget::dragEnterEvent(QDragEnterEvent* DragEnterEvent)
{
	if (mIsView)
	{
		const QMimeData* MimeData = DragEnterEvent->mimeData();

		if (MimeData->hasFormat("application/vnd.leocad-part"))
		{
			DragEnterEvent->acceptProposedAction();
			((View*)widget)->BeginDrag(lcDragState::Piece);
		}
		else if (MimeData->hasFormat("application/vnd.leocad-color"))
		{
			DragEnterEvent->acceptProposedAction();
			((View*)widget)->BeginDrag(lcDragState::Color);
		}
	}
	else
		DragEnterEvent->ignore();
}

void lcQGLWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
	if (!mIsView)
		return;

	((View*)widget)->EndDrag(false);

	event->accept();
}

void lcQGLWidget::dragMoveEvent(QDragMoveEvent* DragMoveEvent)
{
	if (mIsView)
	{
		const QMimeData* MimeData = DragMoveEvent->mimeData();

		if (MimeData->hasFormat("application/vnd.leocad-part") || MimeData->hasFormat("application/vnd.leocad-color"))
		{
			float scale = deviceScale();

			widget->mInputState.x = DragMoveEvent->pos().x() * scale;
			widget->mInputState.y = widget->mHeight - DragMoveEvent->pos().y() * scale - 1;
			widget->mInputState.Modifiers = DragMoveEvent->keyboardModifiers();

			widget->OnMouseMove();

			DragMoveEvent->accept();
			return;
		}
	}

	QGLWidget::dragMoveEvent(DragMoveEvent);
}

void lcQGLWidget::dropEvent(QDropEvent* DropEvent)
{
	if (mIsView)
	{
		const QMimeData* MimeData = DropEvent->mimeData();

		if (MimeData->hasFormat("application/vnd.leocad-part") || MimeData->hasFormat("application/vnd.leocad-color"))
		{
			((View*)widget)->EndDrag(true);
			setFocus(Qt::MouseFocusReason);

			DropEvent->accept();
			return;
		}
	}

	QGLWidget::dropEvent(DropEvent);
}
