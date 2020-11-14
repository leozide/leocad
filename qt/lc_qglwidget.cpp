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

#include "lc_previewwidget.h"

static QList<QGLWidget*> gWidgetList;

void lcGLWidget::MakeCurrent()
{
	QGLWidget* Widget = (QGLWidget*)mWidget;

	Widget->makeCurrent();
}

void lcGLWidget::Redraw()
{
	lcQGLWidget* Widget = (lcQGLWidget*)mWidget;

	Widget->update();
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

lcQGLWidget::lcQGLWidget(QWidget* Parent, lcGLWidget* Owner)
	: QGLWidget(Parent, gWidgetList.isEmpty() ? nullptr : gWidgetList.first())
{
	mWheelAccumulator = 0;
	widget = Owner;
	widget->mWidget = this;

	makeCurrent();

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

	setMouseTracking(true);

	if (dynamic_cast<View*>(Owner))
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
	return mPreferredSize.isNull() ? QGLWidget::sizeHint() : mPreferredSize;
}

void lcQGLWidget::SetPreviewPosition(const QRect& ParentRect)
{
	lcPreferences& Preferences = lcGetPreferences();
	lcPreviewWidget* Preview = reinterpret_cast<lcPreviewWidget*>(widget);

	setWindowTitle(tr("%1 Preview").arg(Preview->IsModel() ? "Submodel" : "Part"));

	int Size[2] = { 300,200 };
	if (Preferences.mPreviewSize == 400)
	{
		Size[0] = 400; Size[1] = 300;
	}
	mPreferredSize = QSize(Size[0], Size[1]);

	float Scale = deviceScale();
	Preview->mWidth = width()  * Scale;
	Preview->mHeight = height() * Scale;

	const QRect desktop = QApplication::desktop()->geometry();

	QPoint pos;
	switch (Preferences.mPreviewLocation)
	{
	case lcPreviewLocation::TopRight:
		pos = mapToGlobal(ParentRect.topRight());
		break;
	case lcPreviewLocation::TopLeft:
		pos = mapToGlobal(ParentRect.topLeft());
		break;
	case lcPreviewLocation::BottomRight:
		pos = mapToGlobal(ParentRect.bottomRight());
		break;
	default:
		pos = mapToGlobal(ParentRect.bottomLeft());
		break;
	}
	if (pos.x() < desktop.left())
		pos.setX(desktop.left());
	if (pos.y() < desktop.top())
		pos.setY(desktop.top());

	if ((pos.x() + width()) > desktop.width())
		pos.setX(desktop.width() - width());
	if ((pos.y() + height()) > desktop.bottom())
		pos.setY(desktop.bottom() - height());
	move(pos);

	setMinimumSize(100,100);
	show();
	setFocus();
}

void lcQGLWidget::resizeGL(int Width, int Height)
{
	widget->mWidth = Width;
	widget->mHeight = Height;
}

void lcQGLWidget::paintGL()
{
	widget->OnDraw();
}

void lcQGLWidget::keyPressEvent(QKeyEvent* KeyEvent)
{
	if (KeyEvent->key() == Qt::Key_Control || KeyEvent->key() == Qt::Key_Shift)
	{
		widget->mInputState.Modifiers = KeyEvent->modifiers();
		widget->OnUpdateCursor();
	}

	QGLWidget::keyPressEvent(KeyEvent);
}

void lcQGLWidget::keyReleaseEvent(QKeyEvent* KeyEvent)
{
	if (KeyEvent->key() == Qt::Key_Control || KeyEvent->key() == Qt::Key_Shift)
	{
		widget->mInputState.Modifiers = KeyEvent->modifiers();
		widget->OnUpdateCursor();
	}

	QGLWidget::keyReleaseEvent(KeyEvent);
}

void lcQGLWidget::mousePressEvent(QMouseEvent* MouseEvent)
{
	float scale = deviceScale();

	widget->mInputState.x = MouseEvent->x() * scale;
	widget->mInputState.y = widget->mHeight - MouseEvent->y() * scale - 1;
	widget->mInputState.Modifiers = MouseEvent->modifiers();

	switch (MouseEvent->button())
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

void lcQGLWidget::mouseReleaseEvent(QMouseEvent* MouseEvent)
{
	float scale = deviceScale();

	widget->mInputState.x = MouseEvent->x() * scale;
	widget->mInputState.y = widget->mHeight - MouseEvent->y() * scale - 1;
	widget->mInputState.Modifiers = MouseEvent->modifiers();

	switch (MouseEvent->button())
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

void lcQGLWidget::mouseDoubleClickEvent(QMouseEvent* MouseEvent)
{
	float scale = deviceScale();

	widget->mInputState.x = MouseEvent->x() * scale;
	widget->mInputState.y = widget->mHeight - MouseEvent->y() * scale - 1;
	widget->mInputState.Modifiers = MouseEvent->modifiers();

	switch (MouseEvent->button())
	{
	case Qt::LeftButton:
		widget->OnLeftButtonDoubleClick();
		break;
	default:
		break;
	}
}

void lcQGLWidget::mouseMoveEvent(QMouseEvent* MouseEvent)
{
	float scale = deviceScale();

	widget->mInputState.x = MouseEvent->x() * scale;
	widget->mInputState.y = widget->mHeight - MouseEvent->y() * scale - 1;
	widget->mInputState.Modifiers = MouseEvent->modifiers();

	widget->OnMouseMove();
}

void lcQGLWidget::wheelEvent(QWheelEvent* WheelEvent)
{
	if ((WheelEvent->orientation() & Qt::Vertical) == 0)
	{
		WheelEvent->ignore();
		return;
	}

	float scale = deviceScale();

	widget->mInputState.x = WheelEvent->x() * scale;
	widget->mInputState.y = widget->mHeight - WheelEvent->y() * scale - 1;
	widget->mInputState.Modifiers = WheelEvent->modifiers();

#if (QT_VERSION >= QT_VERSION_CHECK(5, 2, 0))
	mWheelAccumulator += WheelEvent->angleDelta().y() / 8;
#else
	mWheelAccumulator += event->delta() / 8;
#endif
	int numSteps = mWheelAccumulator / 15;

	if (numSteps)
	{
		widget->OnMouseWheel(numSteps);
		mWheelAccumulator -= numSteps * 15;
	}

	WheelEvent->accept();
}

void lcQGLWidget::dragEnterEvent(QDragEnterEvent* DragEnterEvent)
{
	const QMimeData* MimeData = DragEnterEvent->mimeData();

	if (MimeData->hasFormat("application/vnd.leocad-part"))
	{
		DragEnterEvent->acceptProposedAction();
		widget->BeginDrag(lcDragState::Piece);
		return;
	}
	else if (MimeData->hasFormat("application/vnd.leocad-color"))
	{
		DragEnterEvent->acceptProposedAction();
		widget->BeginDrag(lcDragState::Color);
		return;
	}

	DragEnterEvent->ignore();
}

void lcQGLWidget::dragLeaveEvent(QDragLeaveEvent* DragLeaveEvent)
{
	widget->EndDrag(false);
	DragLeaveEvent->accept();
}

void lcQGLWidget::dragMoveEvent(QDragMoveEvent* DragMoveEvent)
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

	QGLWidget::dragMoveEvent(DragMoveEvent);
}

void lcQGLWidget::dropEvent(QDropEvent* DropEvent)
{
	const QMimeData* MimeData = DropEvent->mimeData();

	if (MimeData->hasFormat("application/vnd.leocad-part") || MimeData->hasFormat("application/vnd.leocad-color"))
	{
		widget->EndDrag(true);
		setFocus(Qt::MouseFocusReason);

		DropEvent->accept();
		return;
	}

	QGLWidget::dropEvent(DropEvent);
}
