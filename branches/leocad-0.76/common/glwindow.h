#ifndef _GLWINDOW_H_
#define _GLWINDOW_H_

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
	virtual void OnLeftButtonDown(int x, int y, bool bControl, bool bShift) { };
	virtual void OnLeftButtonUp(int x, int y, bool bControl, bool bShift) { };
	virtual void OnLeftButtonDoubleClick(int x, int y, bool bControl, bool bShift) { };
	virtual void OnMiddleButtonDown(int x, int y, bool bControl, bool bShift) { };
	virtual void OnMiddleButtonUp(int x, int y, bool bControl, bool bShift) { };
	virtual void OnRightButtonDown(int x, int y, bool bControl, bool bShift) { };
	virtual void OnRightButtonUp(int x, int y, bool bControl, bool bShift) { };
	virtual void OnMouseMove(int x, int y, bool bControl, bool bShift) { };

protected:
	int m_nWidth;
	int m_nHeight;

private:
	void *m_pData;
	GLWindow *m_pShare;
};

#endif // _GLWINDOW_H_
