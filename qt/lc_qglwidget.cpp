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

void lcGLWidget::SetCursor(LC_CURSOR_TYPE CursorType)
{
	if (mCursorType == CursorType)
		return;

	struct lcCursorInfo
	{
		int x, y;
		const char* Name;
	};

	const lcCursorInfo Cursors[LC_CURSOR_COUNT] =
	{
		{  0,  0, "" },                                 // LC_CURSOR_DEFAULT
		{  8,  3, ":/resources/cursor_insert" },        // LC_CURSOR_BRICK
		{ 15, 15, ":/resources/cursor_light" },         // LC_CURSOR_LIGHT
		{  7, 10, ":/resources/cursor_spotlight" },     // LC_CURSOR_SPOTLIGHT
		{ 15,  9, ":/resources/cursor_camera" },        // LC_CURSOR_CAMERA
		{  0,  2, ":/resources/cursor_select" },        // LC_CURSOR_SELECT
		{  0,  2, ":/resources/cursor_select_add" },    // LC_CURSOR_SELECT_ADD
		{  0,  2, ":/resources/cursor_select_remove" }, // LC_CURSOR_SELECT_REMOVE
		{ 15, 15, ":/resources/cursor_move" },          // LC_CURSOR_MOVE
		{ 15, 15, ":/resources/cursor_rotate" },        // LC_CURSOR_ROTATE
		{ 15, 15, ":/resources/cursor_rotatex" },       // LC_CURSOR_ROTATEX
		{ 15, 15, ":/resources/cursor_rotatey" },       // LC_CURSOR_ROTATEY
		{  0, 10, ":/resources/cursor_delete" },        // LC_CURSOR_DELETE
		{ 14, 14, ":/resources/cursor_paint" },         // LC_CURSOR_PAINT
		{ 15, 15, ":/resources/cursor_zoom" },          // LC_CURSOR_ZOOM
		{  9,  9, ":/resources/cursor_zoom_region" },   // LC_CURSOR_ZOOM_REGION
		{ 15, 15, ":/resources/cursor_pan" },           // LC_CURSOR_PAN
		{ 15, 15, ":/resources/cursor_roll" },          // LC_CURSOR_ROLL
		{ 15, 15, ":/resources/cursor_rotate_view" },   // LC_CURSOR_ROTATE_VIEW
	};

	static_assert(sizeof(Cursors) / sizeof(Cursors[0]) == LC_CURSOR_COUNT, "Array size mismatch");

	QGLWidget* widget = (QGLWidget*)mWidget;

	if (CursorType != LC_CURSOR_DEFAULT && CursorType < LC_CURSOR_COUNT)
	{
		const lcCursorInfo& Cursor = Cursors[CursorType];
		widget->setCursor(QCursor(QPixmap(Cursor.Name), Cursor.x, Cursor.y));
		mCursorType = CursorType;
	}
	else
	{
		widget->unsetCursor();
		mCursorType = LC_CURSOR_DEFAULT;
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

	// TODO: Find a better place for the grid texture and font
	gStringCache.AddRef(widget->mContext);
	gTexFont.Load(widget->mContext);
	if (gWidgetList.isEmpty())
	{
		lcInitializeGLExtensions(context());
		lcContext::CreateResources();
		View::CreateResources(widget->mContext);
		lcViewSphere::CreateResources(widget->mContext);

		if (!gSupportsShaderObjects && lcGetPreferences().mShadingMode == LC_SHADING_DEFAULT_LIGHTS)
			lcGetPreferences().mShadingMode = LC_SHADING_FLAT;

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
	gStringCache.Release(widget->mContext);
	gTexFont.Release();
	makeCurrent();
	if (gWidgetList.isEmpty())
	{
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
	glDepthMask(GL_TRUE);
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
			((View*)widget)->BeginDrag(lcDragState::PIECE);
		}
		else if (MimeData->hasFormat("application/vnd.leocad-color"))
		{
			DragEnterEvent->acceptProposedAction();
			((View*)widget)->BeginDrag(lcDragState::COLOR);
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
