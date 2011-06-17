#if !defined(AFX_COLORLST_H__533899A7_4008_11D2_8202_D2B1707B2D1B__INCLUDED_)
#define AFX_COLORLST_H__533899A7_4008_11D2_8202_D2B1707B2D1B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ColorLst.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CColorsList window

class CColorsList : public CListBox
{
// Construction
public:
	CColorsList();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CColorsList)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDIS);
	//}}AFX_VIRTUAL

// Implementation
public:
	BOOL m_bLowRes;
	virtual ~CColorsList();

	// Generated message map functions
protected:
	//{{AFX_MSG(CColorsList)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG

	CToolTipCtrl   m_ToolTip;

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COLORLST_H__533899A7_4008_11D2_8202_D2B1707B2D1B__INCLUDED_)
