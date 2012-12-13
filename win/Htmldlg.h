#if !defined(AFX_HTMLDLG_H__6E9D4B64_61DB_11D2_8202_9B4C5CA8F71F__INCLUDED_)
#define AFX_HTMLDLG_H__6E9D4B64_61DB_11D2_8202_9B4C5CA8F71F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// HTMLDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CHTMLDlg dialog

class CHTMLDlg : public CDialog
{
// Construction
public:
	CHTMLDlg(void* param, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CHTMLDlg)
	enum { IDD = IDD_HTML };
	int		m_nLayout;
	BOOL	m_bIndex;
	CString	m_strFolder;
	BOOL	m_bImages;
	BOOL	m_bListEnd;
	BOOL	m_bListStep;
	BOOL	m_bHighlight;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHTMLDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void* imopts;

	// Generated message map functions
	//{{AFX_MSG(CHTMLDlg)
	afx_msg void OnImageOptions();
	afx_msg void OnListClick();
	afx_msg void OnLayoutClick();
	afx_msg void OnHtmdlgBrowsefolder();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HTMLDLG_H__6E9D4B64_61DB_11D2_8202_9B4C5CA8F71F__INCLUDED_)
