#include "lc_global.h"
#include "lc_viewwidget.h"
#include "view.h"
#include "project.h"
#include "lc_application.h"

lcViewWidget::lcViewWidget(QWidget *parent, lcGLWidget *share)
	: lcGLWidget(parent, share)
{
	Project* project = lcGetActiveProject();

	View* view = new View(project, NULL);
	view->CreateFromWindow(this);

	if (project->GetActiveView())
		view->SetCamera(project->GetActiveView()->mCamera, false);
	else
		view->SetDefaultCamera();

	mWindow = view;
}
