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
}

void lcViewWidget::initializeGL()
{
}

int vw, vh;

void lcViewWidget::resizeGL(int width, int height)
{
	vw = width;
	vh = height;
}

void lcViewWidget::paintGL()
{
	Project* p = lcGetActiveProject();
	if (!p)
		return;

	View* av = p->GetActiveView();
	if (!av)
		return;
	static View v(p,NULL);
	v.OnSize(vw, vh);
	v.mCamera = av->mCamera;
	p->Render(&v, false);
}
