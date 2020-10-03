#pragma once

#include <QGLWidget>
class lcGLWidget;

class lcQGLWidget : public QGLWidget
{
public:
	lcQGLWidget(QWidget* Parent, lcGLWidget* Owner, bool IsView, bool IsPreview = false);
	~lcQGLWidget();

	QSize sizeHint() const override;

	lcGLWidget *widget;
	QSize preferredSize;
	bool mIsView;
	bool mIsPreview;

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
	void initializeGL() override;
	void resizeGL(int Width, int Height) override;
	void paintGL() override;
	void keyPressEvent(QKeyEvent* KeyEvent) override;
	void keyReleaseEvent(QKeyEvent* KeyEvent) override;
	void mousePressEvent(QMouseEvent* MouseEvent) override;
	void mouseReleaseEvent(QMouseEvent* MouseEvent) override;
	void mouseDoubleClickEvent(QMouseEvent* MouseEvent) override;
	void mouseMoveEvent(QMouseEvent* MouseEvent) override;
	void wheelEvent(QWheelEvent* WheelEvent) override;
	void dragEnterEvent(QDragEnterEvent* DragEnterEvent) override;
	void dragLeaveEvent(QDragLeaveEvent* DragLeaveEvent) override;
	void dragMoveEvent(QDragMoveEvent* DragMoveEvent) override;
	void dropEvent(QDropEvent* DropEvent) override;

	int mWheelAccumulator;
};

