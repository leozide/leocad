#if !defined(AFX_POVDLG_H__17968F81_BCE9_11D1_A742_444553540000__INCLUDED_)
#define AFX_POVDLG_H__17968F81_BCE9_11D1_A742_444553540000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// POVDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPOVDlg dialog

class CPOVDlg : public CDialog
{
// Construction
public:
	CPOVDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CPOVDlg)
	enum { IDD = IDD_EXPORTPOV };
	BOOL	m_bRender;
	CString	m_strPOV;
	CString	m_strOut;
	CString	m_strLGEO;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPOVDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	static const DWORD m_nHelpIDs[];

	// Generated message map functions
	//{{AFX_MSG(CPOVDlg)
	virtual void OnOK();
	afx_msg void OnPovbrowse();
	afx_msg void OnPovoutbrowse();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnPovdlgLgeobrowse();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_POVDLG_H__17968F81_BCE9_11D1_A742_444553540000__INCLUDED_)
