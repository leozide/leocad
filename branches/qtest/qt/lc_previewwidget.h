#ifndef LC_PREVIEWWIDGET_H
#define LC_PREVIEWWIDGET_H

#include <QGLWidget>
class PiecePreview;

class lcPreviewWidget : public QGLWidget
{
public:
	lcPreviewWidget(QWidget *parent = 0);

	PiecePreview* mPreview;

protected:
	void initializeGL();
	void resizeGL(int x, int h);
	void paintGL();
};

#endif // LC_PREVIEWWIDGET_H
