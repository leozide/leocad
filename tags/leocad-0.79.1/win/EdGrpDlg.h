#if !defined(AFX_EDGRPDLG_H__52EEEAC3_525A_11D3_A5D0_989AF4F1FB3F__INCLUDED_)
#define AFX_EDGRPDLG_H__52EEEAC3_525A_11D3_A5D0_989AF4F1FB3F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// EdGrpDlg.h : header file
//

#include "GrpTree.h"

/////////////////////////////////////////////////////////////////////////////
// CEditGroupsDlg dialog

class CEditGroupsDlg : public CDialog
{
// Construction
public:
	CEditGroupsDlg(void* param, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CEditGroupsDlg)
	enum { IDD = IDD_EDIT_GROUPS };
	CGroupEditTree	m_Tree;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditGroupsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CImageList m_TreeImages;

	// Generated message map functions
	//{{AFX_MSG(CEditGroupsDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnEditgrpNewgroup();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EDGRPDLG_H__52EEEAC3_525A_11D3_A5D0_989AF4F1FB3F__INCLUDED_)
