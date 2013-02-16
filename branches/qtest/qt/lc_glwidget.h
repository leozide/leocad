#ifndef _LC_GLWIDGET_H_
#define _LC_GLWIDGET_H_

#include <QGLWidget>
class GLWindow;

class lcGLWidget : public QGLWidget
{
public:
	lcGLWidget(QWidget *parent);

	GLWindow* mWindow;

protected:
	void initializeGL();
	void resizeGL(int x, int h);
	void paintGL();
	void mousePressEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
};

#endif // _LC_VIEWWIDGET_H_
