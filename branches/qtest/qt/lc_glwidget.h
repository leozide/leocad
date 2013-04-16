#ifndef _LC_GLWIDGET_H_
#define _LC_GLWIDGET_H_

#include <QGLWidget>
class GLWindow;

class lcGLWidget : public QGLWidget
{
public:
	lcGLWidget(QWidget *parent, lcGLWidget *share, GLWindow *window, bool view);

	QSize sizeHint() const;

	GLWindow *mWindow;
	QSize preferredSize;
	bool isView;

protected:
	void initializeGL();
	void resizeGL(int x, int h);
	void paintGL();
	void mousePressEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void dragEnterEvent(QDragEnterEvent *event);
	void dragLeaveEvent(QDragLeaveEvent *event);
	void dragMoveEvent(QDragMoveEvent *event);
	void dropEvent(QDropEvent *event);
};

#endif // _LC_VIEWWIDGET_H_
