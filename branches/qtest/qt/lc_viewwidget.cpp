#include "lc_viewwidget.h"
#include <QtGui>
#include <QtOpenGL>
#include "lc_global.h"
#include "view.h"
#include "lc_file.h"
#include "project.h"
#include "lc_application.h"

lcViewWidget::lcViewWidget(QWidget *parent)
	: QGLWidget(parent)
{
	Project* project = lcGetActiveProject();

	mView = new View(project, NULL);
	mView->CreateFromWindow(this);

	if (project->GetActiveView())
		mView->SetCamera(project->GetActiveView()->mCamera, false);
	else
		mView->SetDefaultCamera();
}

void lcViewWidget::initializeGL()
{
}

void lcViewWidget::resizeGL(int width, int height)
{
	mView->OnSize(width, height);
}

void lcViewWidget::paintGL()
{
	mView->OnDraw();
}

void lcViewWidget::mousePressEvent(QMouseEvent *event)
{
	bool Control = event->modifiers() & Qt::ControlModifier;
	bool Shift = event->modifiers() & Qt::ShiftModifier;

	switch (event->button())
	{
	case Qt::LeftButton:
		mView->OnLeftButtonDown(event->x(), event->y(), Control, Shift);
		break;
	case Qt::MidButton:
		mView->OnMiddleButtonDown(event->x(), event->y(), Control, Shift);
		break;
	case Qt::RightButton:
		mView->OnRightButtonDown(event->x(), event->y(), Control, Shift);
		break;
	default:
		break;
	}
}

void lcViewWidget::mouseReleaseEvent(QMouseEvent *event)
{
	bool Control = event->modifiers() & Qt::ControlModifier;
	bool Shift = event->modifiers() & Qt::ShiftModifier;

	switch (event->button())
	{
	case Qt::LeftButton:
		mView->OnLeftButtonUp(event->x(), event->y(), Control, Shift);
		break;
	case Qt::MidButton:
		mView->OnMiddleButtonUp(event->x(), event->y(), Control, Shift);
		break;
	case Qt::RightButton:
		mView->OnRightButtonUp(event->x(), event->y(), Control, Shift);
		break;
	default:
		break;
	}
}

void lcViewWidget::mouseMoveEvent(QMouseEvent *event)
{
	bool Control = event->modifiers() & Qt::ControlModifier;
	bool Shift = event->modifiers() & Qt::ShiftModifier;

	mView->OnMouseMove(event->x(), event->y(), Control, Shift);
}
