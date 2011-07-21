#pragma once
#include "afxwin.h"


// CModelListDlg dialog

class CModelListDlg : public CDialog
{
	DECLARE_DYNAMIC(CModelListDlg)

public:
	CModelListDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CModelListDlg();

// Dialog Data
	enum { IDD = IDD_MODEL_LIST };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedNew();
	afx_msg void OnBnClickedDelete();
	afx_msg void OnBnClickedUp();
	afx_msg void OnBnClickedDown();
	afx_msg void OnBnClickedActivate();
	CListBox m_List;
};
