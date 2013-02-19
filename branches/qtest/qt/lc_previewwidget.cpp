#include "lc_global.h"
#include "lc_previewwidget.h"
#include "preview.h"

lcPreviewWidget::lcPreviewWidget(QWidget *parent, lcGLWidget *share)
	: lcGLWidget(parent, share)
{
	mWindow = new PiecePreview(NULL);
	mWindow->CreateFromWindow(this);
}

 QSize lcPreviewWidget::sizeHint() const
 {
	 return QSize(100, 100);
 }
