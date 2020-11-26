#pragma once

#include "lc_context.h"

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
	int x;
	int y;
	Qt::KeyboardModifiers Modifiers;
};

class lcGLWidget
{
public:
	lcGLWidget()
	{
		mCursor = lcCursor::Default;
		mWidget = nullptr;
		mInputState.x = 0;
		mInputState.y = 0;
		mInputState.Modifiers = Qt::NoModifier;
		mWidth = 1;
		mHeight = 1;
		mContext = new lcContext();
		mDeleteContext = true;
	}

	virtual ~lcGLWidget()
	{
		if (mDeleteContext)
			delete mContext;
	}

	void SetContext(lcContext* Context)
	{
		if (mDeleteContext)
			delete mContext;

		mContext = Context;
		mDeleteContext = false;
	}

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
	virtual void BeginDrag(lcDragState DragState) { Q_UNUSED(DragState); };
	virtual void EndDrag(bool Accept) { Q_UNUSED(Accept); };


	lcInputState mInputState;
	int mWidth;
	int mHeight;
	lcCursor mCursor;
	QGLWidget* mWidget;
	lcContext* mContext;
	bool mDeleteContext;
};
