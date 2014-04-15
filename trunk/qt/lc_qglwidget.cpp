#include "lc_global.h"
#include "lc_qglwidget.h"
#include "lc_glwidget.h"
#include "project.h"
#include "lc_library.h"
#include "lc_application.h"
#include "lc_qmainwindow.h"
#include "lc_mainwindow.h"
#include "lc_context.h"

void lcGLWidget::MakeCurrent()
{
	QGLWidget* Widget = (QGLWidget*)mWidget;

	Widget->makeCurrent();
}

void lcGLWidget::Redraw()
{
	QGLWidget* widget = (QGLWidget*)mWidget;

	widget->updateGL();
}

void lcGLWidget::CaptureMouse()
{
	QGLWidget* widget = (QGLWidget*)mWidget;

	widget->grabMouse();
}

void lcGLWidget::ReleaseMouse()
{
	QGLWidget* widget = (QGLWidget*)mWidget;

	widget->releaseMouse();
}

void* lcGLWidget::GetExtensionAddress(const char* FunctionName)
{
	QGLWidget* Widget = (QGLWidget*)mWidget;

	return (void*)Widget->context()->getProcAddress(FunctionName);
}

void lcGLWidget::ShowPopupMenu()
{
	QGLWidget* Widget = (QGLWidget*)mWidget;

	lcQMainWindow *mainWindow = (lcQMainWindow*)gMainWindow->mHandle;
	QAction **actions = mainWindow->actions;

	QMenu *popup = new QMenu(Widget);

	QMenu *tools = new QMenu("Tools");
	popup->addMenu(tools);
	for (int actionIdx = LC_EDIT_ACTION_FIRST; actionIdx <= LC_EDIT_ACTION_LAST; actionIdx++)
		tools->addAction(actions[actionIdx]);

	QMenu *cameras = new QMenu("Cameras");
	popup->addMenu(cameras);
	cameras->addAction(actions[LC_VIEW_CAMERA_NONE]);

	for (int actionIdx = LC_VIEW_CAMERA_FIRST; actionIdx <= LC_VIEW_CAMERA_LAST; actionIdx++)
		cameras->addAction(actions[actionIdx]);

	cameras->addSeparator();
	cameras->addAction(actions[LC_VIEW_CAMERA_RESET]);

	popup->addSeparator();
	popup->addAction(actions[LC_VIEW_SPLIT_HORIZONTAL]);
	popup->addAction(actions[LC_VIEW_SPLIT_VERTICAL]);
	popup->addAction(actions[LC_VIEW_REMOVE_VIEW]);
	popup->addAction(actions[LC_VIEW_RESET_VIEWS]);

	popup->exec(QCursor::pos());
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
		{  0,  0, "" },                                   // LC_CURSOR_DEFAULT
		{  8,  3, ":/resources/cursor_insert" },          // LC_CURSOR_BRICK
		{ 15, 15, ":/resources/cursor_light" },           // LC_CURSOR_LIGHT
		{  7, 10, ":/resources/cursor_spotlight" },       // LC_CURSOR_SPOTLIGHT
		{ 15,  9, ":/resources/cursor_camera" },          // LC_CURSOR_CAMERA
		{  0,  2, ":/resources/cursor_select" },          // LC_CURSOR_SELECT
		{  0,  2, ":/resources/cursor_select_multiple" }, // LC_CURSOR_SELECT_GROUP
		{ 15, 15, ":/resources/cursor_move" },            // LC_CURSOR_MOVE
		{ 15, 15, ":/resources/cursor_rotate" },          // LC_CURSOR_ROTATE
		{ 15, 15, ":/resources/cursor_rotatex" },         // LC_CURSOR_ROTATEX
		{ 15, 15, ":/resources/cursor_rotatey" },         // LC_CURSOR_ROTATEY
		{  0, 10, ":/resources/cursor_delete" },          // LC_CURSOR_DELETE
		{ 14, 14, ":/resources/cursor_paint" },           // LC_CURSOR_PAINT
		{ 15, 15, ":/resources/cursor_zoom" },            // LC_CURSOR_ZOOM
		{  9,  9, ":/resources/cursor_zoom_region" },     // LC_CURSOR_ZOOM_REGION
		{ 15, 15, ":/resources/cursor_pan" },             // LC_CURSOR_PAN
		{ 15, 15, ":/resources/cursor_roll" },            // LC_CURSOR_ROLL
		{ 15, 15, ":/resources/cursor_rotate_view" },     // LC_CURSOR_ROTATE_VIEW
	};

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

lcQGLWidget::lcQGLWidget(QWidget *parent, lcQGLWidget *share, lcGLWidget *owner, bool view)
	: QGLWidget(parent, share)
{
	widget = owner;
	widget->mWidget = this;
	owner->mContext = new lcContext();

	widget->MakeCurrent();
	GL_InitializeSharedExtensions(widget);
	widget->OnInitialUpdate();

	preferredSize = QSize(0, 0);
	setMouseTracking(true);

	isView = view;
	if (isView)
	{
		setFocusPolicy(Qt::StrongFocus);
		setAcceptDrops(true);
	}
}

lcQGLWidget::~lcQGLWidget()
{
	delete widget->mContext;
	widget->mContext = NULL;

	if (isView)
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
	if (isView && event->key() == Qt::Key_Control)
	{
		widget->mInputState.Control = true;
		widget->OnUpdateCursor();
	}

	QGLWidget::keyPressEvent(event);
}

void lcQGLWidget::keyReleaseEvent(QKeyEvent *event)
{
	if (isView && event->key() == Qt::Key_Control)
	{
		widget->mInputState.Control = false;
		widget->OnUpdateCursor();
	}

	QGLWidget::keyReleaseEvent(event);
}

void lcQGLWidget::mousePressEvent(QMouseEvent *event)
{
	float scale = deviceScale();

	widget->mInputState.x = event->x() * scale;
	widget->mInputState.y = widget->mHeight - event->y() * scale - 1;
	widget->mInputState.Control = (event->modifiers() & Qt::ControlModifier) != 0;
	widget->mInputState.Shift = (event->modifiers() & Qt::ShiftModifier) != 0;
	widget->mInputState.Alt = (event->modifiers() & Qt::AltModifier) != 0;

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
	default:
		break;
	}
}

void lcQGLWidget::mouseReleaseEvent(QMouseEvent *event)
{
	float scale = deviceScale();

	widget->mInputState.x = event->x() * scale;
	widget->mInputState.y = widget->mHeight - event->y() * scale - 1;
	widget->mInputState.Control = (event->modifiers() & Qt::ControlModifier) != 0;
	widget->mInputState.Shift = (event->modifiers() & Qt::ShiftModifier) != 0;
	widget->mInputState.Alt = (event->modifiers() & Qt::AltModifier) != 0;

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
	default:
		break;
	}
}

void lcQGLWidget::mouseMoveEvent(QMouseEvent *event)
{
	float scale = deviceScale();

	widget->mInputState.x = event->x() * scale;
	widget->mInputState.y = widget->mHeight - event->y() * scale - 1;
	widget->mInputState.Control = (event->modifiers() & Qt::ControlModifier) != 0;
	widget->mInputState.Shift = (event->modifiers() & Qt::ShiftModifier) != 0;
	widget->mInputState.Alt = (event->modifiers() & Qt::AltModifier) != 0;

	widget->OnMouseMove();
}

void lcQGLWidget::wheelEvent(QWheelEvent *event)
{
	if (event->orientation() != Qt::Vertical)
	{
		event->ignore();
		return;
	}

	float scale = deviceScale();

	widget->mInputState.x = event->x() * scale;
	widget->mInputState.y = widget->mHeight - event->y() * scale - 1;
	widget->mInputState.Control = (event->modifiers() & Qt::ControlModifier) != 0;
	widget->mInputState.Shift = (event->modifiers() & Qt::ShiftModifier) != 0;
	widget->mInputState.Alt = (event->modifiers() & Qt::AltModifier) != 0;

	int numDegrees = event->delta() / 8;
	int numSteps = numDegrees / 15;

	widget->OnMouseWheel(numSteps);

	event->accept();
}

void lcQGLWidget::dragEnterEvent(QDragEnterEvent *event)
{
	if (isView && event->mimeData()->hasFormat("application/vnd.leocad-part"))
	{
		event->acceptProposedAction();

		QByteArray pieceData = event->mimeData()->data("application/vnd.leocad-part");
		QDataStream dataStream(&pieceData, QIODevice::ReadOnly);
		QString id;

		dataStream >> id;

		lcGetActiveProject()->BeginPieceDrop(lcGetPiecesLibrary()->FindPiece(id.toLocal8Bit().data(), false));
	}
	else
		event->ignore();
}

void lcQGLWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
	if (!isView)
		return;

	lcGetActiveProject()->EndPieceDrop(false);

	event->accept();
}

void lcQGLWidget::dragMoveEvent(QDragMoveEvent *event)
{
	if (!isView || !event->mimeData()->hasFormat("application/vnd.leocad-part"))
		return;

	float scale = deviceScale();

	lcGetActiveProject()->OnPieceDropMove(event->pos().x() * scale, widget->mHeight - event->pos().y() * scale - 1);

	event->accept();
}

void lcQGLWidget::dropEvent(QDropEvent *event)
{
	if (!isView || !event->mimeData()->hasFormat("application/vnd.leocad-part"))
		return;

	lcGetActiveProject()->EndPieceDrop(true);

	event->accept();
}
