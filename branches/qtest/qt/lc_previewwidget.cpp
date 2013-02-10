#include "lc_global.h"
#include "lc_previewwidget.h"
#include <QtGui>
#include <QtOpenGL>
#include "preview.h"
#include "opengl.h"

lcPreviewWidget::lcPreviewWidget(QWidget *parent) :
	QGLWidget(parent)
{
	mPreview = new PiecePreview(NULL);
	mPreview->CreateFromWindow(this);
}

void lcPreviewWidget::initializeGL()
{
}

void lcPreviewWidget::resizeGL(int width, int height)
{
	mPreview->OnSize(width, height);
}

void lcPreviewWidget::paintGL()
{
	mPreview->OnDraw();
}
