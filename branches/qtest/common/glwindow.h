#ifndef _GLWINDOW_H_
#define _GLWINDOW_H_

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

class GLWindow
{
public:
	GLWindow(GLWindow* Share);
	virtual ~GLWindow();

	bool CreateFromWindow(void* Data);
	bool CreateFromBitmap(void* Data);
	void DestroyContext();

	bool MakeCurrent();
	void SwapBuffers();
	void Redraw(bool ForceRedraw = false);
	void CaptureMouse();
	void ReleaseMouse();
	void SetCursor(LC_CURSOR_TYPE Cursor);

	int GetWidth() const
	{ return m_nWidth; }
	int GetHeight() const
	{ return m_nHeight; }
	void* GetData() const
	{ return m_pData; }

	virtual void OnDraw() { };
	virtual void OnSize(int cx, int cy)
	{ m_nWidth = cx; m_nHeight = cy; };
	virtual void OnInitialUpdate();
	virtual void OnLeftButtonDown(int x, int y, bool Control, bool Shift) { };
	virtual void OnLeftButtonUp(int x, int y, bool Control, bool Shift) { };
	virtual void OnLeftButtonDoubleClick(int x, int y, bool Control, bool Shift) { };
	virtual void OnMiddleButtonDown(int x, int y, bool Control, bool Shift) { };
	virtual void OnMiddleButtonUp(int x, int y, bool Control, bool Shift) { };
	virtual void OnRightButtonDown(int x, int y, bool Control, bool Shift) { };
	virtual void OnRightButtonUp(int x, int y, bool Control, bool Shift) { };
	virtual void OnMouseMove(int x, int y, bool Control, bool Shift) { };
	virtual void OnMouseWheel(int x, int y, float Direction, bool Control, bool Shift) { };

protected:
	int m_nWidth;
	int m_nHeight;

private:
	void *m_pData;
	GLWindow *m_pShare;
};

#endif // _GLWINDOW_H_
