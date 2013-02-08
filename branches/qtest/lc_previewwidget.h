#ifndef LC_PREVIEWWIDGET_H
#define LC_PREVIEWWIDGET_H

#include <QGLWidget>

class lcPreviewWidget : public QGLWidget
{
public:
	lcPreviewWidget(QWidget *parent = 0);

protected:
	void initializeGL();
	void resizeGL(int x, int h);
	void paintGL();
};

#endif // LC_PREVIEWWIDGET_H
