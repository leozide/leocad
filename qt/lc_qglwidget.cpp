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

lcQGLWidget::lcQGLWidget(QWidget* Parent, lcGLWidget* Owner)
	: QGLWidget(Parent, gWidgetList.isEmpty() ? nullptr : gWidgetList.first())
{
	mWheelAccumulator = 0;
	mWidget = Owner;
	mWidget->mWidget = this;

	makeCurrent();

	if (gWidgetList.isEmpty())
	{
		// TODO: Find a better place for the grid texture and font
		gStringCache.Initialize(mWidget->mContext);
		gTexFont.Initialize(mWidget->mContext);

		lcInitializeGLExtensions(context());
		lcContext::CreateResources();
		View::CreateResources(mWidget->mContext);
		lcViewSphere::CreateResources(mWidget->mContext);

		if (!gSupportsShaderObjects && lcGetPreferences().mShadingMode == lcShadingMode::DefaultLights)
			lcGetPreferences().mShadingMode = lcShadingMode::Flat;

		if (!gSupportsFramebufferObjectARB && !gSupportsFramebufferObjectEXT)
			gMainWindow->GetPartSelectionWidget()->DisableIconMode();

		gPlaceholderMesh = new lcMesh;
		gPlaceholderMesh->CreateBox();
	}

	gWidgetList.append(this);

	mWidget->OnInitialUpdate();

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

		lcGetPiecesLibrary()->ReleaseBuffers(mWidget->mContext);
		View::DestroyResources(mWidget->mContext);
		lcContext::DestroyResources();
		lcViewSphere::DestroyResources(mWidget->mContext);

		delete gPlaceholderMesh;
		gPlaceholderMesh = nullptr;
	}

	delete mWidget;
}

QSize lcQGLWidget::sizeHint() const
{
	return mPreferredSize.isNull() ? QGLWidget::sizeHint() : mPreferredSize;
}

void lcQGLWidget::SetPreviewPosition(const QRect& ParentRect)
{
	lcPreferences& Preferences = lcGetPreferences();
	lcPreviewWidget* Preview = reinterpret_cast<lcPreviewWidget*>(mWidget);

	setWindowTitle(tr("%1 Preview").arg(Preview->IsModel() ? "Submodel" : "Part"));

	int Size[2] = { 300,200 };
	if (Preferences.mPreviewSize == 400)
	{
		Size[0] = 400; Size[1] = 300;
	}
	mPreferredSize = QSize(Size[0], Size[1]);

	float Scale = GetDeviceScale();
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
	mWidget->mWidth = Width;
	mWidget->mHeight = Height;
}

void lcQGLWidget::paintGL()
{
	mWidget->OnDraw();
}

void lcQGLWidget::keyPressEvent(QKeyEvent* KeyEvent)
{
	if (KeyEvent->key() == Qt::Key_Control || KeyEvent->key() == Qt::Key_Shift)
	{
		mWidget->SetMouseModifiers(KeyEvent->modifiers());
		mWidget->UpdateCursor();
	}

	QGLWidget::keyPressEvent(KeyEvent);
}

void lcQGLWidget::keyReleaseEvent(QKeyEvent* KeyEvent)
{
	if (KeyEvent->key() == Qt::Key_Control || KeyEvent->key() == Qt::Key_Shift)
	{
		mWidget->SetMouseModifiers(KeyEvent->modifiers());
		mWidget->UpdateCursor();
	}

	QGLWidget::keyReleaseEvent(KeyEvent);
}

void lcQGLWidget::mousePressEvent(QMouseEvent* MouseEvent)
{
	float DeviceScale = GetDeviceScale();

	mWidget->SetMousePosition(MouseEvent->x() * DeviceScale, mWidget->mHeight - MouseEvent->y() * DeviceScale - 1);
	mWidget->SetMouseModifiers(MouseEvent->modifiers());

	switch (MouseEvent->button())
	{
	case Qt::LeftButton:
		mWidget->OnLeftButtonDown();
		break;

	case Qt::MidButton:
		mWidget->OnMiddleButtonDown();
		break;

	case Qt::RightButton:
		mWidget->OnRightButtonDown();
		break;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
	case Qt::BackButton:
		mWidget->OnBackButtonDown();
		break;

	case Qt::ForwardButton:
		mWidget->OnForwardButtonDown();
		break;
#endif

	default:
		break;
	}
}

void lcQGLWidget::mouseReleaseEvent(QMouseEvent* MouseEvent)
{
	float DeviceScale = GetDeviceScale();

	mWidget->SetMousePosition(MouseEvent->x() * DeviceScale, mWidget->mHeight - MouseEvent->y() * DeviceScale - 1);
	mWidget->SetMouseModifiers(MouseEvent->modifiers());

	switch (MouseEvent->button())
	{
	case Qt::LeftButton:
		mWidget->OnLeftButtonUp();
		break;

	case Qt::MidButton:
		mWidget->OnMiddleButtonUp();
		break;

	case Qt::RightButton:
		mWidget->OnRightButtonUp();
		break;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
	case Qt::BackButton:
		mWidget->OnBackButtonUp();
		break;

	case Qt::ForwardButton:
		mWidget->OnForwardButtonUp();
		break;
#endif

	default:
		break;
	}
}

void lcQGLWidget::mouseDoubleClickEvent(QMouseEvent* MouseEvent)
{
	float DeviceScale = GetDeviceScale();

	mWidget->SetMousePosition(MouseEvent->x() * DeviceScale, mWidget->mHeight - MouseEvent->y() * DeviceScale - 1);
	mWidget->SetMouseModifiers(MouseEvent->modifiers());

	switch (MouseEvent->button())
	{
	case Qt::LeftButton:
		mWidget->OnLeftButtonDoubleClick();
		break;
	default:
		break;
	}
}

void lcQGLWidget::mouseMoveEvent(QMouseEvent* MouseEvent)
{
	float DeviceScale = GetDeviceScale();

	mWidget->SetMousePosition(MouseEvent->x() * DeviceScale, mWidget->mHeight - MouseEvent->y() * DeviceScale - 1);
	mWidget->SetMouseModifiers(MouseEvent->modifiers());

	mWidget->OnMouseMove();
}

void lcQGLWidget::wheelEvent(QWheelEvent* WheelEvent)
{
	if ((WheelEvent->orientation() & Qt::Vertical) == 0)
	{
		WheelEvent->ignore();
		return;
	}

	float DeviceScale = GetDeviceScale();

	mWidget->SetMousePosition(WheelEvent->x() * DeviceScale, mWidget->mHeight - WheelEvent->y() * DeviceScale - 1);
	mWidget->SetMouseModifiers(WheelEvent->modifiers());

#if (QT_VERSION >= QT_VERSION_CHECK(5, 2, 0))
	mWheelAccumulator += WheelEvent->angleDelta().y() / 8;
#else
	mWheelAccumulator += WheelEvent->delta() / 8;
#endif
	int numSteps = mWheelAccumulator / 15;

	if (numSteps)
	{
		mWidget->OnMouseWheel(numSteps);
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
		mWidget->BeginDrag(lcDragState::Piece);
		return;
	}
	else if (MimeData->hasFormat("application/vnd.leocad-color"))
	{
		DragEnterEvent->acceptProposedAction();
		mWidget->BeginDrag(lcDragState::Color);
		return;
	}

	DragEnterEvent->ignore();
}

void lcQGLWidget::dragLeaveEvent(QDragLeaveEvent* DragLeaveEvent)
{
	mWidget->EndDrag(false);
	DragLeaveEvent->accept();
}

void lcQGLWidget::dragMoveEvent(QDragMoveEvent* DragMoveEvent)
{
	const QMimeData* MimeData = DragMoveEvent->mimeData();

	if (MimeData->hasFormat("application/vnd.leocad-part") || MimeData->hasFormat("application/vnd.leocad-color"))
	{
		float DeviceScale = GetDeviceScale();

		mWidget->SetMousePosition(DragMoveEvent->pos().x() * DeviceScale, mWidget->mHeight - DragMoveEvent->pos().y() * DeviceScale - 1);
		mWidget->SetMouseModifiers(DragMoveEvent->keyboardModifiers());

		mWidget->OnMouseMove();

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
		mWidget->EndDrag(true);
		setFocus(Qt::MouseFocusReason);

		DropEvent->accept();
		return;
	}

	QGLWidget::dropEvent(DropEvent);
}
