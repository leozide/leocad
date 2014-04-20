#ifndef _LC_GLWIDGET_H_
#define _LC_GLWIDGET_H_

#include "lc_context.h"

enum LC_CURSOR_TYPE
{
	LC_CURSOR_DEFAULT,
	LC_CURSOR_BRICK,
	LC_CURSOR_LIGHT,
	LC_CURSOR_SPOTLIGHT,
	LC_CURSOR_CAMERA,
	LC_CURSOR_SELECT,
	LC_CURSOR_SELECT_GROUP,
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
	bool Control;
	bool Shift;
	bool Alt;
};

class lcGLWidget
{
public:
	lcGLWidget()
	{
		mCursorType = LC_CURSOR_DEFAULT;
		mWidget = NULL;
		mInputState.x = 0;
		mInputState.y = 0;
		mInputState.Control = false;
		mInputState.Shift = false;
		mInputState.Alt = false;
		mContext = new lcContext();
	}

	virtual ~lcGLWidget()
	{
		delete mContext;
	}

	void* GetExtensionAddress(const char* FunctionName);
	void ShowPopupMenu();

	void MakeCurrent();
	void Redraw();
	void CaptureMouse();
	void ReleaseMouse();
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
	virtual void OnMouseMove() { }
	virtual void OnMouseWheel(float Direction) { }

	lcInputState mInputState;
	int mWidth;
	int mHeight;
	int mCursorType;
	void* mWidget;
	lcContext* mContext;
};

#endif // _LC_GLWIDGET_H_
