#if !defined(AFX_STEPPOP_H__105A59A6_586A_11D2_8202_D8BEE5C6EEBF__INCLUDED_)
#define AFX_STEPPOP_H__105A59A6_586A_11D2_8202_D8BEE5C6EEBF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// StepPop.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CStepPopup window

class CStepPopup : public CWnd
{
// Construction
public:
	CStepPopup();
	CStepPopup(CPoint pt, CWnd* pParentWnd);

// Attributes
public:
	CSliderCtrl m_Slider;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStepPopup)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CStepPopup();

	// Generated message map functions
protected:
	//{{AFX_MSG(CStepPopup)
	afx_msg void OnNcDestroy();
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnActivateApp(BOOL bActive, DWORD hTask);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STEPPOP_H__105A59A6_586A_11D2_8202_D8BEE5C6EEBF__INCLUDED_)
