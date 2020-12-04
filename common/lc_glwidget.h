#pragma once

enum class lcDragState
{
	None,
	Piece,
	Color
};

enum class lcCursor
{
	Default,
	Brick,
	Light,
	Spotlight,
	Camera,
	Select,
	SelectAdd,
	SelectRemove,
	Move,
	Rotate,
	RotateX,
	RotateY,
	Delete,
	Paint,
	ColorPicker,
	Zoom,
	ZoomRegion,
	Pan,
	Roll,
	RotateView,
	Count
};

struct lcInputState
{
	int x = 0;
	int y = 0;
	Qt::KeyboardModifiers Modifiers = Qt::NoModifier;
};

class lcGLWidget
{
public:
	lcGLWidget();
	virtual ~lcGLWidget();

	lcGLWidget(const lcGLWidget&) = delete;
	lcGLWidget& operator=(const lcGLWidget&) = delete;

	void SetContext(lcContext* Context);
	void MakeCurrent();
	void Redraw();
	void SetCursor(lcCursor Cursor);
	void DrawBackground() const;

	virtual void OnDraw() { }
	virtual void OnInitialUpdate() { }
	virtual void OnUpdateCursor() { }
	virtual void OnLeftButtonDown() { }
	virtual void OnLeftButtonUp() { }
	virtual void OnLeftButtonDoubleClick() { }
	virtual void OnMiddleButtonDown() { }
	virtual void OnMiddleButtonUp() { }
	virtual void OnRightButtonDown() { }
	virtual void OnRightButtonUp() { }
	virtual void OnBackButtonDown() { }
	virtual void OnBackButtonUp() { }
	virtual void OnForwardButtonDown() { }
	virtual void OnForwardButtonUp() { }
	virtual void OnMouseMove() { }
	virtual void OnMouseWheel(float Direction) { Q_UNUSED(Direction); }
	virtual void BeginDrag(lcDragState DragState) { Q_UNUSED(DragState); }
	virtual void EndDrag(bool Accept) { Q_UNUSED(Accept); }

	lcInputState mInputState;
	int mWidth = 1;
	int mHeight = 1;
	lcCursor mCursor = lcCursor::Default;
	QGLWidget* mWidget = nullptr;
	lcContext* mContext = nullptr;
	bool mDeleteContext = true;
};
