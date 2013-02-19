#include "lc_global.h"
#include "lc_glwidget.h"
#include "glwindow.h"
#include <QtGui>

lcGLWidget::lcGLWidget(QWidget *parent, lcGLWidget *share)
	: QGLWidget(parent, share)
{
	setMouseTracking(true);
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
