#include "lc_global.h"
#include "lc_glwidget.h"
#include "glwindow.h"
#include <QtGui>

lcGLWidget::lcGLWidget(QWidget *parent, lcGLWidget *share, GLWindow *window)
	: QGLWidget(parent, share)
{
	mWindow = window;
	mWindow->CreateFromWindow(this);

	mWindow->MakeCurrent();
	mWindow->OnInitialUpdate();

	preferredSize = QSize(0, 0);
	setMouseTracking(true);
}

QSize lcGLWidget::sizeHint() const
{
	if (preferredSize.isEmpty())
		return QGLWidget::sizeHint();
	else
		return preferredSize;
}

void lcGLWidget::initializeGL()
{
}

void lcGLWidget::resizeGL(int width, int height)
{
	mWindow->OnSize(width, height);
}

void lcGLWidget::paintGL()
{
	mWindow->OnDraw();
}

void lcGLWidget::mousePressEvent(QMouseEvent *event)
{
	bool Control = event->modifiers() & Qt::ControlModifier;
	bool Shift = event->modifiers() & Qt::ShiftModifier;

	switch (event->button())
	{
	case Qt::LeftButton:
		mWindow->OnLeftButtonDown(event->x(), height() - event->y() - 1, Control, Shift);
		break;
	case Qt::MidButton:
		mWindow->OnMiddleButtonDown(event->x(), height() - event->y() - 1, Control, Shift);
		break;
	case Qt::RightButton:
		mWindow->OnRightButtonDown(event->x(), height() - event->y() - 1, Control, Shift);
		break;
	default:
		break;
	}
}

void lcGLWidget::mouseReleaseEvent(QMouseEvent *event)
{
	bool Control = event->modifiers() & Qt::ControlModifier;
	bool Shift = event->modifiers() & Qt::ShiftModifier;

	switch (event->button())
	{
	case Qt::LeftButton:
		mWindow->OnLeftButtonUp(event->x(), height() - event->y() - 1, Control, Shift);
		break;
	case Qt::MidButton:
		mWindow->OnMiddleButtonUp(event->x(), height() - event->y() - 1, Control, Shift);
		break;
	case Qt::RightButton:
		mWindow->OnRightButtonUp(event->x(), height() - event->y() - 1, Control, Shift);
		break;
	default:
		break;
	}
}

void lcGLWidget::mouseMoveEvent(QMouseEvent *event)
{
	bool Control = event->modifiers() & Qt::ControlModifier;
	bool Shift = event->modifiers() & Qt::ShiftModifier;

	mWindow->OnMouseMove(event->x(), height() - event->y() - 1, Control, Shift);
}
