////////////////////////////////////////////////////////////////
// CFlatToolBar 1997 Microsoft Systems Journal. 
//

#if !defined(_FLATBAR_H__INCLUDED_)
#define _FLATBAR_H__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

//////////////////
// CFlatToolBar fixes the display bug described in the article. It also has
// overridden load functions that modify the style to TBSTYLE_FLAT. If you
// don't create your toolbar by loading it from a resource, you should call
// ModifyStyle(0, TBSTYLE_FLAT) yourself.

class CFlatToolBar : public CToolBar 
{
public:
	BOOL LoadToolBar(LPCTSTR lpszResourceName);
	BOOL LoadToolBar(UINT nIDResource)
		{ return LoadToolBar(MAKEINTRESOURCE(nIDResource)); }
	//{{AFX_VIRTUAL(CFlatToolBar)
	//}}AFX_VIRTUAL

	static int iVerComCtl32;

	virtual CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz);
	virtual CSize CalcDynamicLayout(int nLength, DWORD nMode);

	CSize CalcLayout(DWORD nMode, int nLength = -1);
	CSize CalcSize(TBBUTTON* pData, int nCount);
	int WrapToolBar(TBBUTTON* pData, int nCount, int nWidth);
	void SizeToolBar(TBBUTTON* pData, int nCount, int nLength, BOOL bVert = FALSE);

	virtual CSize GetButtonSize(TBBUTTON* pData, int iButton);
	void GetButton(int nIndex, TBBUTTON* pButton) const;
	void SetButton(int nIndex, TBBUTTON* pButton);

protected:
	DECLARE_DYNAMIC(CFlatToolBar)
	virtual void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler);
	//{{AFX_MSG(CFlatToolBar)
	afx_msg void OnWindowPosChanging(LPWINDOWPOS lpWndPos);
	afx_msg void OnWindowPosChanged(LPWINDOWPOS lpWndPos);
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS FAR* lpncsp);
	afx_msg void OnNcPaint();
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // !defined(_FLATBAR_H__INCLUDED_)
