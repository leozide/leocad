#ifndef _LC_QGLWIDGET_H_
#define _LC_QGLWIDGET_H_

#include <QGLWidget>
class GLWindow;

class lcQGLWidget : public QGLWidget
{
public:
	lcQGLWidget(QWidget *parent, lcQGLWidget *share, GLWindow *window, bool view);
	~lcQGLWidget();

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
	void wheelEvent(QWheelEvent *event);
	void dragEnterEvent(QDragEnterEvent *event);
	void dragLeaveEvent(QDragLeaveEvent *event);
	void dragMoveEvent(QDragMoveEvent *event);
	void dropEvent(QDropEvent *event);
};

#endif // _LC_VIEWWIDGET_H_
