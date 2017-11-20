#pragma once

#include "lc_context.h"

enum LC_CURSOR_TYPE
{
	LC_CURSOR_DEFAULT,
	LC_CURSOR_BRICK,
	LC_CURSOR_LIGHT,
	LC_CURSOR_SPOTLIGHT,
	LC_CURSOR_CAMERA,
	LC_CURSOR_SELECT,
	LC_CURSOR_SELECT_ADD,
	LC_CURSOR_SELECT_REMOVE,
	LC_CURSOR_MOVE,
	LC_CURSOR_ROTATE,
	LC_CURSOR_ROTATEX,
	LC_CURSOR_ROTATEY,
	LC_CURSOR_DELETE,
	LC_CURSOR_PAINT,
	LC_CURSOR_ZOOM,
	LC_CURSOR_ZOOM_REGION,
	LC_CURSOR_PAN,
	LC_CURSOR_ROLL,
	LC_CURSOR_ROTATE_VIEW,
	LC_CURSOR_COUNT
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
		mCursorType = LC_CURSOR_DEFAULT;
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
	void SetCursor(LC_CURSOR_TYPE Cursor);

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

	lcInputState mInputState;
	int mWidth;
	int mHeight;
	int mCursorType;
	void* mWidget;
	lcContext* mContext;
	bool mDeleteContext;
};

