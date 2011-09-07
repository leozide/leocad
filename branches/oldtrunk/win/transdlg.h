#if !defined(AFX_TRANSDLG_H__FF6D1B48_BEEA_4A5F_9A00_48FD3ABE72C4__INCLUDED_)
#define AFX_TRANSDLG_H__FF6D1B48_BEEA_4A5F_9A00_48FD3ABE72C4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// transdlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTransformDlg dialog

class CTransformDlg : public CDialog
{
// Construction
public:
	CTransformDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CTransformDlg)
	enum { IDD = IDD_TRANSFORM };
	float	m_GlobalX;
	float	m_GlobalY;
	float	m_GlobalZ;
	float	m_OffsetX;
	float	m_OffsetY;
	float	m_OffsetZ;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTransformDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CTransformDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TRANSDLG_H__FF6D1B48_BEEA_4A5F_9A00_48FD3ABE72C4__INCLUDED_)
