// CADBar.h: interface for the CCADStatusBar class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CADBAR_H__105A59A3_586A_11D2_8202_D8BEE5C6EEBF__INCLUDED_)
#define AFX_CADBAR_H__105A59A3_586A_11D2_8202_D8BEE5C6EEBF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CCADStatusBar : public CStatusBar  
{
public:
	CWnd* m_pPopup;
	CCADStatusBar();
	virtual ~CCADStatusBar();


protected:
	//{{AFX_MSG(CCADStatusBar)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

#endif // !defined(AFX_CADBAR_H__105A59A3_586A_11D2_8202_D8BEE5C6EEBF__INCLUDED_)
