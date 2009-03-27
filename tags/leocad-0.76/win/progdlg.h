#if !defined(AFX_PROGDLG_H__7DBF6C04_6356_11D2_8202_82A791333B5C__INCLUDED_)
#define AFX_PROGDLG_H__7DBF6C04_6356_11D2_8202_82A791333B5C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ProgDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CProgressDlg dialog

class CProgressDlg : public CDialog
{
// Construction
public:
	void SetStatus(LPCTSTR lpszMessage);
	void SetSubStatus(LPCTSTR lpszMessage);
	BOOL Create(CWnd *pParent);
	void SetRange(int nLower, int nUpper);
	int SetPos(int nPos);
	int StepIt();
	BOOL CheckCancelButton();
	CProgressDlg(LPCTSTR pszTitle);
	 ~CProgressDlg();

// Dialog Data
	//{{AFX_DATA(CProgressDlg)
	enum { IDD = IDD_PROGRESS };
	CProgressCtrl	m_Progress;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CProgressDlg)
	public:
	virtual BOOL DestroyWindow();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	BOOL m_bParentDisabled;
	void ReEnableParent();
	CWnd* m_pParentWnd;
	int m_nMaxValue;
	int m_nMinValue;
	CString m_strTitle;
	int m_nPrevPercent;
	int m_nPrevPos;
	void PumpMessages();
	BOOL m_bCancel;

	// Generated message map functions
	//{{AFX_MSG(CProgressDlg)
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROGDLG_H__7DBF6C04_6356_11D2_8202_82A791333B5C__INCLUDED_)
