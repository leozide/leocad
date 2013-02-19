#ifndef LC_PREVIEWWIDGET_H
#define LC_PREVIEWWIDGET_H

#include "lc_glwidget.h"

class lcPreviewWidget : public lcGLWidget
{
public:
	lcPreviewWidget(QWidget *parent, lcGLWidget *share);

	QSize sizeHint() const;
};

#endif // LC_PREVIEWWIDGET_H
