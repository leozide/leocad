#pragma once

#include <QGLWidget>
class lcGLWidget;

class lcQGLWidget : public QGLWidget
{
public:
	lcQGLWidget(QWidget *parent, lcGLWidget *owner, bool view);
	~lcQGLWidget();

	QSize sizeHint() const;

	lcGLWidget *widget;
	QSize preferredSize;
	bool mIsView;

	float deviceScale()
	{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
		return windowHandle()->devicePixelRatio();
#else
		return 1.0f;
#endif
	}

	QTimer mUpdateTimer;

protected:
	void initializeGL();
	void resizeGL(int x, int h);
	void paintGL();
	void keyPressEvent(QKeyEvent *event);
	void keyReleaseEvent(QKeyEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void mouseDoubleClickEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void wheelEvent(QWheelEvent *event);
	void dragEnterEvent(QDragEnterEvent *event);
	void dragLeaveEvent(QDragLeaveEvent *event);
	void dragMoveEvent(QDragMoveEvent *event);
	void dropEvent(QDropEvent *event);

	int mWheelAccumulator;
};

