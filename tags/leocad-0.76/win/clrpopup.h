#if !defined(AFX_COLOURPOPUP_H__D0B75902_9830_11D1_9C0F_00A0243D1382__INCLUDED_)
#define AFX_COLOURPOPUP_H__D0B75902_9830_11D1_9C0F_00A0243D1382__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// ColorPopup.h : header file
//

// CColorPopup messages
#define CPN_SELENDOK        WM_USER + 1001 // Color Picker end OK
#define CPN_SELENDCANCEL    WM_USER + 1002 // Color Picker end (cancelled)

/////////////////////////////////////////////////////////////////////////////
// CColorPopup window

class CColorPopup : public CWnd
{
	// Construction
public:
	CColorPopup();
	CColorPopup(CPoint p, int ColorIndex, CWnd* pParentWnd);
	void Initialise();

	// Attributes
public:

	// Operations
public:
	BOOL Create(CPoint p, int ColorIndex, CWnd* pParentWnd);

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CColorPopup)
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CColorPopup();

protected:
	BOOL GetCellRect(int nIndex, const LPRECT& rect);
	void SetWindowSize();
	void CreateToolTips();
	void ChangeSelection(int nIndex);
	void EndSelection(int nMessage);
	void DrawCell(CDC* pDC, int nIndex);

	int  GetIndex(int row, int col) const;
	int  GetRow(int nIndex) const;
	int  GetColumn(int nIndex) const;

// protected attributes
protected:
	int            m_nNumColumns, m_nNumRows;
	int            m_nBoxSize, m_nMargin;
	int            m_nCurrentSel;
	int            m_nChosenColorSel;
	int            m_nInitialColor, m_nColor;
	CRect          m_WindowRect;
	CFont          m_Font;
	CToolTipCtrl   m_ToolTip;
	CWnd*          m_pParent;

	// Generated message map functions
protected:
	//{{AFX_MSG(CColorPopup)
	afx_msg void OnNcDestroy();
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnPaint();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnActivateApp(BOOL bActive, ACTIVATEAPPPARAM hTask);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COLOURPOPUP_H__D0B75902_9830_11D1_9C0F_00A0243D1382__INCLUDED_)
