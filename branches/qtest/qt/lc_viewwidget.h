#ifndef LC_VIEWWIDGET_H
#define LC_VIEWWIDGET_H

#include <QGLWidget>
class View;

class lcViewWidget : public QGLWidget
{
public:
	lcViewWidget(QWidget *parent);

	View* mView;

protected:
	void initializeGL();
	void resizeGL(int x, int h);
	void paintGL();
	void mousePressEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
};

#endif // LC_VIEWWIDGET_H
