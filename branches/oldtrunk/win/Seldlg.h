#if !defined(AFX_SELDLG_H__655F3924_5536_11D2_8202_85459B83C718__INCLUDED_)
#define AFX_SELDLG_H__655F3924_5536_11D2_8202_85459B83C718__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SelDlg.h : header file
//

#include "typedefs.h"

/////////////////////////////////////////////////////////////////////////////
// CSelectDlg dialog

class CSelectDlg : public CDialog
{
// Construction
public:
	CSelectDlg(void* pData, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSelectDlg)
	enum { IDD = IDD_SELECT_OBJECTS };
	CListBox	m_List;
	BOOL	m_bCameras;
	BOOL	m_bGroups;
	BOOL	m_bLights;
	BOOL	m_bPieces;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSelectDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void UpdateList(BOOL bFirst);
	LC_SEL_DATA* m_pData;

	// Generated message map functions
	//{{AFX_MSG(CSelectDlg)
	afx_msg void OnSeldlgAll();
	afx_msg void OnSeldlgNone();
	afx_msg void OnSeldlgInvert();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnSelChange();
	//}}AFX_MSG
	afx_msg void OnSeldlgToggle();
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SELDLG_H__655F3924_5536_11D2_8202_85459B83C718__INCLUDED_)
