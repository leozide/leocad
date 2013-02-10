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
