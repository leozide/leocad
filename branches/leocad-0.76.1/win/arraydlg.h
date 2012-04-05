#if !defined(AFX_ARRAYDLG_H__4DF55C82_5A43_11D2_8202_B3F55E19C71F__INCLUDED_)
#define AFX_ARRAYDLG_H__4DF55C82_5A43_11D2_8202_B3F55E19C71F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ArrayDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CArrayDlg dialog

class CArrayDlg : public CDialog
{
// Construction
public:
	CArrayDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CArrayDlg)
	enum { IDD = IDD_ARRAY };
	UINT	m_n1DCount;
	UINT	m_n2DCount;
	UINT	m_n3DCount;
	int		m_nArrayDimension;
	UINT	m_nTotal;
	float	m_f2DX;
	float	m_f2DY;
	float	m_f2DZ;
	float	m_f3DX;
	float	m_f3DY;
	float	m_f3DZ;
	float	m_fMoveX;
	float	m_fMoveY;
	float	m_fMoveZ;
	float	m_fRotateX;
	float	m_fRotateY;
	float	m_fRotateZ;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CArrayDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	BOOL m_bInitDone;

	// Generated message map functions
	//{{AFX_MSG(CArrayDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnArrayDimension();
	afx_msg void OnChangeArrayCount();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ARRAYDLG_H__4DF55C82_5A43_11D2_8202_B3F55E19C71F__INCLUDED_)
