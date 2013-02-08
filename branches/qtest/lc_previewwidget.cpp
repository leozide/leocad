#include "lc_previewwidget.h"
#include <QtGui>
#include <QtOpenGL>
#include "preview.h"
#include "opengl.h"

lcPreviewWidget::lcPreviewWidget(QWidget *parent) :
	QGLWidget(parent)
{
}

void lcPreviewWidget::initializeGL()
{
}
int qpw,qph;

void lcPreviewWidget::resizeGL(int width, int height)
{
	glViewport(0,0,width,height);
	qpw = width;
	qph = height;
}

void lcPreviewWidget::paintGL()
{
	extern PiecePreview* preview;
	extern int qtest;
	qtest = 1;
	GL_DisableVertexBufferObject();
	preview->OnDraw();
	qtest = 0;
}
