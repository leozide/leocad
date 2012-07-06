#if !defined(AFX_STEPDLG_H__D8E4D902_5BCF_11D2_8202_A3A5E1B1BA04__INCLUDED_)
#define AFX_STEPDLG_H__D8E4D902_5BCF_11D2_8202_A3A5E1B1BA04__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// StepDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CStepDlg dialog

class CStepDlg : public CDialog
{
// Construction
public:
	CStepDlg(CStepDlg** pointer, CWnd* pParent);   // standard constructor
	void UpdateRange(int nTime, int nTotal);

// Dialog Data
	//{{AFX_DATA(CStepDlg)
	enum { IDD = IDD_STEP };
	CSliderCtrl	m_Slider;
	UINT	m_nStep;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStepDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
protected:
	CWnd* m_pView;
	CStepDlg** m_pPointer;

	// Generated message map functions
	//{{AFX_MSG(CStepDlg)
	afx_msg void OnApply();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	virtual void OnOK();
	afx_msg void OnDestroy();
	virtual void OnCancel();
	//}}AFX_MSG

	afx_msg void OnEditChange();

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STEPDLG_H__D8E4D902_5BCF_11D2_8202_A3A5E1B1BA04__INCLUDED_)
