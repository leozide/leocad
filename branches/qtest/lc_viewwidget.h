#ifndef LC_VIEWWIDGET_H
#define LC_VIEWWIDGET_H

#include <QGLWidget>

class lcViewWidget : public QGLWidget
{
public:
	lcViewWidget(QWidget *parent);

protected:
	void initializeGL();
	void resizeGL(int x, int h);
	void paintGL();
};

#endif // LC_VIEWWIDGET_H
