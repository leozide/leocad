#if !defined(AFX_GROUPDLG_H__DAE91724_6417_11D2_8202_C0F688A2A20E__INCLUDED_)
#define AFX_GROUPDLG_H__DAE91724_6417_11D2_8202_C0F688A2A20E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// GroupDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CGroupDlg dialog

class CGroupDlg : public CDialog
{
// Construction
public:
	CGroupDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CGroupDlg)
	enum { IDD = IDD_GROUP };
	CString	m_strName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGroupDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CGroupDlg)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GROUPDLG_H__DAE91724_6417_11D2_8202_C0F688A2A20E__INCLUDED_)
