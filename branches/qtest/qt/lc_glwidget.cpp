#include "lc_global.h"
#include "lc_glwidget.h"
#include "glwindow.h"
#include <QtGui>

lcGLWidget::lcGLWidget(QWidget *parent)
	: QGLWidget(parent)
{
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
		mWindow->OnLeftButtonDown(event->x(), event->y(), Control, Shift);
		break;
	case Qt::MidButton:
		mWindow->OnMiddleButtonDown(event->x(), event->y(), Control, Shift);
		break;
	case Qt::RightButton:
		mWindow->OnRightButtonDown(event->x(), event->y(), Control, Shift);
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
		mWindow->OnLeftButtonUp(event->x(), event->y(), Control, Shift);
		break;
	case Qt::MidButton:
		mWindow->OnMiddleButtonUp(event->x(), event->y(), Control, Shift);
		break;
	case Qt::RightButton:
		mWindow->OnRightButtonUp(event->x(), event->y(), Control, Shift);
		break;
	default:
		break;
	}
}

void lcGLWidget::mouseMoveEvent(QMouseEvent *event)
{
	bool Control = event->modifiers() & Qt::ControlModifier;
	bool Shift = event->modifiers() & Qt::ShiftModifier;

	mWindow->OnMouseMove(event->x(), event->y(), Control, Shift);
}
