// CADBar.h: interface for the CCADStatusBar class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CADBAR_H__105A59A3_586A_11D2_8202_D8BEE5C6EEBF__INCLUDED_)
#define AFX_CADBAR_H__105A59A3_586A_11D2_8202_D8BEE5C6EEBF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CCADStatusBar : public CMFCStatusBar  
{
public:
	CCADStatusBar();
	virtual ~CCADStatusBar();
	virtual BOOL Create(CWnd *pParentWnd, DWORD dwStyle = WS_CHILD | WS_VISIBLE | CBRS_BOTTOM, UINT nID = AFX_IDW_STATUS_BAR);

	BOOL IsProgressBarVisible() const
		{ return m_bProgressVisible; }
	void SetProgressBarWidth(int nWidth)
		{ m_nProgressWidth = nWidth;  }
	BOOL ShowProgressBar(BOOL bShow = TRUE);

	void SetProgressBarRange(int nLower, int nUpper)
		{ m_Progress.SetRange(nLower,nUpper); }
	int SetProgressBarPos(int nPos)
		{ return m_Progress.SetPos(nPos); }
	int SetProgressBarStep(int nStep)
		{ return m_Progress.SetStep(nStep); }
	int StepProgressBar()
		{ return m_Progress.StepIt(); }

protected:
	void AdjustProgressBarPosition();

public:
	CWnd* m_pPopup;

protected:
	// Progress Bar variables.
	CProgressCtrl m_Progress;
	int m_nProgressWidth;
	BOOL m_bProgressVisible;

protected:
	//{{AFX_MSG(CCADStatusBar)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

#endif // !defined(AFX_CADBAR_H__105A59A3_586A_11D2_8202_D8BEE5C6EEBF__INCLUDED_)
