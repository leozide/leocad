#if !defined(AFX_PIECELST_H__533899A4_4008_11D2_8202_D2B1707B2D1B__INCLUDED_)
#define AFX_PIECELST_H__533899A4_4008_11D2_8202_D2B1707B2D1B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// PieceLst.h : header file
//

#include "TitleTip.h"
#include "SortHead.h"

/////////////////////////////////////////////////////////////////////////////
// CPiecesList window

class CPiecesList : public CListCtrl
{
// Construction
public:
	CPiecesList();

// Attributes
public:

// Operations
public:
	void SubclassHeader();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPiecesList)
	//}}AFX_VIRTUAL

// Implementation
public:
	void UpdateList();
	virtual ~CPiecesList();

	// Generated message map functions
protected:
	UINT m_nLastPieces[32];
	int CellRectFromPoint(CPoint & point, RECT * cellrect, int * col) const;

	BOOL m_bAscending;
	int m_nSortedCol;

	CTitleTip m_TitleTip;
	CSortHeaderCtrl m_HeaderCtrl;

	//{{AFX_MSG(CPiecesList)
	afx_msg void OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PIECELST_H__533899A4_4008_11D2_8202_D2B1707B2D1B__INCLUDED_)
