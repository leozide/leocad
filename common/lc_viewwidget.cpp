#include "lc_global.h"
#include "lc_viewwidget.h"
#include "lc_glextensions.h"
#include "project.h"
#include "lc_library.h"
#include "lc_application.h"
#include "lc_mainwindow.h"
#include "lc_context.h"
#include "lc_view.h"
#include "lc_texture.h"
#include "lc_mesh.h"
#include "lc_profile.h"
#include "lc_previewwidget.h"

lcViewWidget::lcViewWidget(QWidget* Parent, lcView* View)
	: QOpenGLWidget(Parent), mView(View)
{
	mView->SetWidget(this);

	setMouseTracking(true);

	if (View->GetViewType() == lcViewType::View)
	{
		setFocusPolicy(Qt::StrongFocus);
		setAcceptDrops(true);
	}
}

QSize lcViewWidget::sizeHint() const
{
	return mPreferredSize.isEmpty() ? QOpenGLWidget::sizeHint() : mPreferredSize;
}

void lcViewWidget::SetView(lcView* View)
{
	if (View)
	{
		if (context())
		{
			makeCurrent();
			View->mContext->SetGLContext(context(), this);
		}

		View->SetWidget(this);
		const float Scale = GetDeviceScale();
		View->SetSize(width() * Scale, height() * Scale);

		if (hasFocus())
			View->SetFocus(true);
	}

	mView = std::unique_ptr<lcView>(View);
}

void lcViewWidget::initializeGL()
{
	mView->mContext->SetGLContext(context(), this);
}

void lcViewWidget::resizeGL(int Width, int Height)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
	const float Scale = devicePixelRatioF();
#else
	const int Scale = devicePixelRatio();
#endif
	mView->SetSize(Width * Scale, Height * Scale);
}

void lcViewWidget::paintGL()
{
	mView->OnDraw();
}

void lcViewWidget::focusInEvent(QFocusEvent* FocusEvent)
{
	if (mView)
		mView->SetFocus(true);

	QOpenGLWidget::focusInEvent(FocusEvent);
}

void lcViewWidget::focusOutEvent(QFocusEvent* FocusEvent)
{
	if (mView)
		mView->SetFocus(false);

	QOpenGLWidget::focusOutEvent(FocusEvent);
}

void lcViewWidget::keyPressEvent(QKeyEvent* KeyEvent)
{
	if (KeyEvent->key() == Qt::Key_Control || KeyEvent->key() == Qt::Key_Shift)
	{
		mView->SetMouseModifiers(KeyEvent->modifiers());
		mView->UpdateCursor();
	}

	QOpenGLWidget::keyPressEvent(KeyEvent);
}

void lcViewWidget::keyReleaseEvent(QKeyEvent* KeyEvent)
{
	if (KeyEvent->key() == Qt::Key_Control || KeyEvent->key() == Qt::Key_Shift)
	{
		mView->SetMouseModifiers(KeyEvent->modifiers());
		mView->UpdateCursor();
	}

	QOpenGLWidget::keyReleaseEvent(KeyEvent);
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

	case Qt::MiddleButton:
		mView->OnMiddleButtonDown();
		break;

	case Qt::RightButton:
		mView->OnRightButtonDown();
		break;

	case Qt::BackButton:
		mView->OnBackButtonDown();
		break;

	case Qt::ForwardButton:
		mView->OnForwardButtonDown();
		break;

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

	case Qt::MiddleButton:
		mView->OnMiddleButtonUp();
		break;

	case Qt::RightButton:
		mView->OnRightButtonUp();
		break;

	case Qt::BackButton:
		mView->OnBackButtonUp();
		break;

	case Qt::ForwardButton:
		mView->OnForwardButtonUp();
		break;

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
	if (WheelEvent->angleDelta().y() == 0)
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

	mWheelAccumulator += WheelEvent->angleDelta().y() / 8;
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

	QOpenGLWidget::dragMoveEvent(DragMoveEvent);
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

	QOpenGLWidget::dropEvent(DropEvent);
}
