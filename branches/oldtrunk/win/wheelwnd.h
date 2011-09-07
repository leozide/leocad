#if !defined(AFX_WHEELWND_H__CA6A1DC2_7283_11D2_8203_8EEC36A08D1C__INCLUDED_)
#define AFX_WHEELWND_H__CA6A1DC2_7283_11D2_8203_8EEC36A08D1C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// WheelWnd.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWheelWnd window

class CWheelWnd : public CWnd
{
// Construction
public:
	CWheelWnd(CPoint ptOrigin);
	BOOL Create(CWnd* pParentWnd);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWheelWnd)
	protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWheelWnd();

	// Generated message map functions
protected:
	static LRESULT CALLBACK HookProc(int code, WPARAM wParam, LPARAM lParam);
	CBitmap m_bitmap;
	CWnd* m_pParentWnd;
	CSize m_sizeBitmap;
	CPoint m_ptOrigin;
	CPoint m_ptCursorPrevious;
	CRect m_rcCursorMargin;

	//{{AFX_MSG(CWheelWnd)
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WHEELWND_H__CA6A1DC2_7283_11D2_8203_8EEC36A08D1C__INCLUDED_)
