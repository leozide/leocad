#if !defined(AFX_CATEGDLG_H__DD3E0D88_ADD7_48A4_9B0E_99637F2010AB__INCLUDED_)
#define AFX_CATEGDLG_H__DD3E0D88_ADD7_48A4_9B0E_99637F2010AB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// categdlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCategoryDlg dialog

class CCategoryDlg : public CDialog
{
// Construction
public:
	CCategoryDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCategoryDlg)
	enum { IDD = IDD_CATEGORY };
	CString	m_Keywords;
	CString	m_Name;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCategoryDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCategoryDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CATEGDLG_H__DD3E0D88_ADD7_48A4_9B0E_99637F2010AB__INCLUDED_)
