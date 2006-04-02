#if !defined(AFX_TEXDLG_H__01AFB4FD_6C04_41FB_8FF6_CC87EAF49D31__INCLUDED_)
#define AFX_TEXDLG_H__01AFB4FD_6C04_41FB_8FF6_CC87EAF49D31__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// texdlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTexturesDlg dialog

class CTexturesDlg : public CDialog
{
// Construction
public:
	CTexturesDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CTexturesDlg)
	enum { IDD = IDD_LIBRARY_TEXTURES };
	CListBox	m_List;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTexturesDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void UpdateList();

	// Generated message map functions
	//{{AFX_MSG(CTexturesDlg)
	virtual void OnOK();
	afx_msg void OnLibtexAdd();
	afx_msg void OnLibtexRemove();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TEXDLG_H__01AFB4FD_6C04_41FB_8FF6_CC87EAF49D31__INCLUDED_)
