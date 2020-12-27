#pragma once

#ifdef LC_USE_QOPENGLWIDGET
typedef QOpenGLWidget lcViewWidgetParent;
#else
typedef QGLWidget lcViewWidgetParent;
#endif

class lcViewWidget : public lcViewWidgetParent
{
	Q_OBJECT

public:
	lcViewWidget(QWidget* Parent, lcView* View);
	~lcViewWidget();

	lcView* GetView() const
	{
		return mView;
	}

	void SetView(lcView* View);
	void SetPreviewPosition(const QRect& ParentRect);

	QSize sizeHint() const override;

protected:
	float GetDeviceScale() const
	{
#ifdef LC_USE_QOPENGLWIDGET
		return devicePixelRatio();
#elif (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
		return windowHandle()->devicePixelRatio();
#else
		return 1.0f;
#endif
	}

	void initializeGL() override;
	void resizeGL(int Width, int Height) override;
	void paintGL() override;
	void focusInEvent(QFocusEvent* FocusEvent) override;
	void focusOutEvent(QFocusEvent* FocusEvent) override;
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

	lcView* mView;
	QSize mPreferredSize;
	int mWheelAccumulator;

	static bool mResourcesLoaded;
};
