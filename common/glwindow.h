#ifndef _GLWINDOW_H_
#define _GLWINDOW_H_

class GLWindow
{
public:
	GLWindow(GLWindow *share);
	virtual ~GLWindow();

	void IncRef()
	{ m_nRef++; }
	void DecRef()
	{ m_nRef--; if (m_nRef == 0) delete this; }

	bool Create(void* data);
	void DestroyContext();

	bool MakeCurrent();
	void SwapBuffers();
	void Redraw();
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
	virtual void OnRightButtonDown(int x, int y, bool bControl, bool bShift) { };
	virtual void OnRightButtonUp(int x, int y, bool bControl, bool bShift) { };
	virtual void OnMouseMove(int x, int y, bool bControl, bool bShift) { };

protected:
	int m_nWidth;
	int m_nHeight;

private:
	void *m_pData;
	GLWindow *m_pShare;
	int m_nRef;
};

#endif // _GLWINDOW_H_
