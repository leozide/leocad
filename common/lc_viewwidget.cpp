#include "lc_global.h"
#include "lc_viewwidget.h"
#include "lc_glextensions.h"
#include "project.h"
#include "lc_library.h"
#include "lc_application.h"
#include "lc_mainwindow.h"
#include "lc_partselectionwidget.h"
#include "lc_context.h"
#include "lc_view.h"
#include "texfont.h"
#include "lc_viewsphere.h"
#include "lc_stringcache.h"
#include "lc_texture.h"
#include "lc_mesh.h"
#include "lc_profile.h"
#include "lc_previewwidget.h"

static QList<lcViewWidget*> gWidgetList;
bool lcViewWidget::mResourcesLoaded;

#ifdef LC_USE_QOPENGLWIDGET
lcViewWidget::lcViewWidget(QWidget* Parent, lcView* View)
	: lcViewWidgetParent(Parent)
#else
lcViewWidget::lcViewWidget(QWidget* Parent, lcView* View)
	: lcViewWidgetParent(Parent, gWidgetList.isEmpty() ? nullptr : gWidgetList.first())
#endif
{
	mWheelAccumulator = 0;
	mView = View;
	mView->SetWidget(this);

	gWidgetList.append(this);

	setMouseTracking(true);

	if (View->GetViewType() == lcViewType::View)
	{
		setFocusPolicy(Qt::StrongFocus);
		setAcceptDrops(true);
	}
}

lcViewWidget::~lcViewWidget()
{
	gWidgetList.removeOne(this);

	if (gWidgetList.isEmpty())
	{
		gStringCache.Reset();
		gTexFont.Reset();

		lcGetPiecesLibrary()->ReleaseBuffers(mView->mContext);
		lcView::DestroyResources(mView->mContext);
		mView->mContext->DestroyResources();
		lcViewSphere::DestroyResources(mView->mContext);

		delete gPlaceholderMesh;
		gPlaceholderMesh = nullptr;

		mResourcesLoaded = false;
	}

	delete mView;
}

QSize lcViewWidget::sizeHint() const
{
	return mPreferredSize.isEmpty() ? lcViewWidgetParent::sizeHint() : mPreferredSize;
}

void lcViewWidget::SetView(lcView* View)
{
	mView = View;

	if (View)
	{
#ifdef LC_USE_QOPENGLWIDGET
		if (context())
			View->mContext->SetGLContext(context());
#endif
		View->SetWidget(this);
		const float Scale = GetDeviceScale();
		View->SetSize(width() * Scale, height() * Scale);

		if (hasFocus())
			View->SetFocus(true);
	}
}

void lcViewWidget::SetPreviewPosition(const QRect& ParentRect)
{
	lcPreferences& Preferences = lcGetPreferences();
	lcPreview* Preview = reinterpret_cast<lcPreview*>(mView);

	setWindowTitle(tr("%1 Preview").arg(Preview->IsModel() ? "Submodel" : "Part"));

	int Size[2] = { 300,200 };
	if (Preferences.mPreviewSize == 400)
	{
		Size[0] = 400; Size[1] = 300;
	}
	mPreferredSize = QSize(Size[0], Size[1]);

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

	if ((pos.x() + width()) > desktop.right())
		pos.setX(desktop.right() - width());
	if ((pos.y() + height()) > desktop.bottom())
		pos.setY(desktop.bottom() - height());
	move(pos);

	setMinimumSize(100,100);
	show();
}

void lcViewWidget::initializeGL()
{
#ifdef LC_USE_QOPENGLWIDGET
	mView->mContext->SetGLContext(context());
#endif

	if (!mResourcesLoaded)
	{
		lcInitializeGLExtensions(context());

		// TODO: Find a better place for the grid texture and font
		gStringCache.Initialize(mView->mContext);
		gTexFont.Initialize(mView->mContext);

		mView->mContext->CreateResources();
		lcView::CreateResources(mView->mContext);
		lcViewSphere::CreateResources(mView->mContext);

		if (!gSupportsShaderObjects && lcGetPreferences().mShadingMode == lcShadingMode::DefaultLights)
			lcGetPreferences().mShadingMode = lcShadingMode::Flat;

		if (!gSupportsFramebufferObject)
			gMainWindow->GetPartSelectionWidget()->DisableIconMode();

		gPlaceholderMesh = new lcMesh;
		gPlaceholderMesh->CreateBox();

		mResourcesLoaded = true;
	}
}

void lcViewWidget::resizeGL(int Width, int Height)
{
	mView->SetSize(Width, Height);
}

void lcViewWidget::paintGL()
{
	mView->OnDraw();
}

void lcViewWidget::focusInEvent(QFocusEvent* FocusEvent)
{
	if (mView)
		mView->SetFocus(true);

	lcViewWidgetParent::focusInEvent(FocusEvent);
}

void lcViewWidget::focusOutEvent(QFocusEvent* FocusEvent)
{
	if (mView)
		mView->SetFocus(false);

	lcViewWidgetParent::focusOutEvent(FocusEvent);
}

void lcViewWidget::keyPressEvent(QKeyEvent* KeyEvent)
{
	if (KeyEvent->key() == Qt::Key_Control || KeyEvent->key() == Qt::Key_Shift)
	{
		mView->SetMouseModifiers(KeyEvent->modifiers());
		mView->UpdateCursor();
	}

	lcViewWidgetParent::keyPressEvent(KeyEvent);
}

void lcViewWidget::keyReleaseEvent(QKeyEvent* KeyEvent)
{
	if (KeyEvent->key() == Qt::Key_Control || KeyEvent->key() == Qt::Key_Shift)
	{
		mView->SetMouseModifiers(KeyEvent->modifiers());
		mView->UpdateCursor();
	}

	lcViewWidgetParent::keyReleaseEvent(KeyEvent);
}

void lcViewWidget::mousePressEvent(QMouseEvent* MouseEvent)
{
	float DeviceScale = GetDeviceScale();

	mView->SetMousePosition(MouseEvent->x() * DeviceScale, mView->GetHeight() - MouseEvent->y() * DeviceScale - 1);
	mView->SetMouseModifiers(MouseEvent->modifiers());

	switch (MouseEvent->button())
	{
	case Qt::LeftButton:
		mView->OnLeftButtonDown();
		break;

	case Qt::MidButton:
		mView->OnMiddleButtonDown();
		break;

	case Qt::RightButton:
		mView->OnRightButtonDown();
		break;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
	case Qt::BackButton:
		mView->OnBackButtonDown();
		break;

	case Qt::ForwardButton:
		mView->OnForwardButtonDown();
		break;
#endif

	default:
		break;
	}
}

void lcViewWidget::mouseReleaseEvent(QMouseEvent* MouseEvent)
{
	float DeviceScale = GetDeviceScale();

	mView->SetMousePosition(MouseEvent->x() * DeviceScale, mView->GetHeight() - MouseEvent->y() * DeviceScale - 1);
	mView->SetMouseModifiers(MouseEvent->modifiers());

	switch (MouseEvent->button())
	{
	case Qt::LeftButton:
		mView->OnLeftButtonUp();
		break;

	case Qt::MidButton:
		mView->OnMiddleButtonUp();
		break;

	case Qt::RightButton:
		mView->OnRightButtonUp();
		break;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
	case Qt::BackButton:
		mView->OnBackButtonUp();
		break;

	case Qt::ForwardButton:
		mView->OnForwardButtonUp();
		break;
#endif

	default:
		break;
	}
}

void lcViewWidget::mouseDoubleClickEvent(QMouseEvent* MouseEvent)
{
	float DeviceScale = GetDeviceScale();

	mView->SetMousePosition(MouseEvent->x() * DeviceScale, mView->GetHeight() - MouseEvent->y() * DeviceScale - 1);
	mView->SetMouseModifiers(MouseEvent->modifiers());

	switch (MouseEvent->button())
	{
	case Qt::LeftButton:
		mView->OnLeftButtonDoubleClick();
		break;
	default:
		break;
	}
}

void lcViewWidget::mouseMoveEvent(QMouseEvent* MouseEvent)
{
	float DeviceScale = GetDeviceScale();

	mView->SetMousePosition(MouseEvent->x() * DeviceScale, mView->GetHeight() - MouseEvent->y() * DeviceScale - 1);
	mView->SetMouseModifiers(MouseEvent->modifiers());

	mView->OnMouseMove();
}

void lcViewWidget::wheelEvent(QWheelEvent* WheelEvent)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 2, 0))
	if (WheelEvent->angleDelta().y() == 0)
#else
	if ((WheelEvent->orientation() & Qt::Vertical) == 0)
#endif
	{
		WheelEvent->ignore();
		return;
	}

	float DeviceScale = GetDeviceScale();

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
	mView->SetMousePosition(WheelEvent->position().x() * DeviceScale, mView->GetHeight() - WheelEvent->position().y() * DeviceScale - 1);
#else
	mView->SetMousePosition(WheelEvent->x() * DeviceScale, mView->GetHeight() - WheelEvent->y() * DeviceScale - 1);
#endif
	mView->SetMouseModifiers(WheelEvent->modifiers());

#if (QT_VERSION >= QT_VERSION_CHECK(5, 2, 0))
	mWheelAccumulator += WheelEvent->angleDelta().y() / 8;
#else
	mWheelAccumulator += WheelEvent->delta() / 8;
#endif
	int numSteps = mWheelAccumulator / 15;

	if (numSteps)
	{
		mView->OnMouseWheel(numSteps);
		mWheelAccumulator -= numSteps * 15;
	}

	WheelEvent->accept();
}

void lcViewWidget::dragEnterEvent(QDragEnterEvent* DragEnterEvent)
{
	const QMimeData* MimeData = DragEnterEvent->mimeData();

	if (MimeData->hasFormat("application/vnd.leocad-part"))
	{
		DragEnterEvent->acceptProposedAction();
		mView->BeginDrag(lcDragState::Piece);
		return;
	}
	else if (MimeData->hasFormat("application/vnd.leocad-color"))
	{
		DragEnterEvent->acceptProposedAction();
		mView->BeginDrag(lcDragState::Color);
		return;
	}

	DragEnterEvent->ignore();
}

void lcViewWidget::dragLeaveEvent(QDragLeaveEvent* DragLeaveEvent)
{
	mView->EndDrag(false);
	DragLeaveEvent->accept();
}

void lcViewWidget::dragMoveEvent(QDragMoveEvent* DragMoveEvent)
{
	const QMimeData* MimeData = DragMoveEvent->mimeData();

	if (MimeData->hasFormat("application/vnd.leocad-part") || MimeData->hasFormat("application/vnd.leocad-color"))
	{
		float DeviceScale = GetDeviceScale();

		mView->SetMousePosition(DragMoveEvent->pos().x() * DeviceScale, mView->GetHeight() - DragMoveEvent->pos().y() * DeviceScale - 1);
		mView->SetMouseModifiers(DragMoveEvent->keyboardModifiers());

		mView->OnMouseMove();

		DragMoveEvent->accept();
		return;
	}

	lcViewWidgetParent::dragMoveEvent(DragMoveEvent);
}

void lcViewWidget::dropEvent(QDropEvent* DropEvent)
{
	const QMimeData* MimeData = DropEvent->mimeData();

	if (MimeData->hasFormat("application/vnd.leocad-part") || MimeData->hasFormat("application/vnd.leocad-color"))
	{
		mView->EndDrag(true);
		setFocus(Qt::MouseFocusReason);

		DropEvent->accept();
		return;
	}

	lcViewWidgetParent::dropEvent(DropEvent);
}
