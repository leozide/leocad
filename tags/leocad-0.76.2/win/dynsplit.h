#ifndef _DYNSPLIT_H_
#define _DYNSPLIT_H_

class CDynamicSplitterWnd : public CSplitterWndEx
{
public:
	BOOL AttachWindow(CWnd* Wnd, int Row, int Col);
	BOOL DetachWindow(int Row, int Col);
	void GetViewRowCol(CWnd* Window, int* Row, int* Col);
};

#endif // _DYNSPLIT_H_
