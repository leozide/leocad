#if !defined(AFX_IMAGEDLG_H__E3BE6801_99E6_11D2_8203_EE720446BF07__INCLUDED_)
#define AFX_IMAGEDLG_H__E3BE6801_99E6_11D2_8203_EE720446BF07__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ImageDlg.h : header file
//

#include "typedefs.h"

/////////////////////////////////////////////////////////////////////////////
// CImageDlg dialog

class CImageDlg : public CDialog
{
// Construction
public:
	CImageDlg(BOOL bHTML, void* param, CWnd* pParent = NULL);   // standard constructor
	LC_IMAGEDLG_OPTS* opts;
	BOOL m_bHTML;

// Dialog Data
	//{{AFX_DATA(CImageDlg)
	enum { IDD = IDD_IMAGE };
	int		m_nFormat;
	BOOL	m_bTransparent;
	BOOL	m_bProgressive;
	UINT	m_nHeight;
	BOOL	m_bHighcolor;
	int		m_nQuality;
	UINT	m_nWidth;
	UINT	m_nFrom;
	float	m_fPause;
	int		m_nSingle;
	UINT	m_nTo;
	CString m_strFilename;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CImageDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CImageDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedImgdlgBrowse();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IMAGEDLG_H__E3BE6801_99E6_11D2_8203_EE720446BF07__INCLUDED_)
