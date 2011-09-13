// ModDlg.h: interface for the CModifyDialog class. 
// 
////////////////////////////////////////////////////////////////////// 

#if !defined(AFX_INITDIALOGBAR_H__46B4D2B3_C982_11D1_8902_0060979C2EFD__INCLUDED_) 
#define AFX_INITDIALOGBAR_H__46B4D2B3_C982_11D1_8902_0060979C2EFD__INCLUDED_ 

#if _MSC_VER >= 1000 
#pragma once 
#endif // _MSC_VER >= 1000 

#include "ClrPick.h"

class Object;

///////////////////////////////////////////////////////////////////////////// 
// CModifyDialog window 

class CModifyDialog : public CPaneDialog
{ 
	DECLARE_DYNAMIC(CModifyDialog) 

// Construction / Destruction 
public: 
	CModifyDialog(); 
	virtual ~CModifyDialog(); 

	//{{AFX_DATA(CModifyDialog)
	enum { IDD = IDD_MODIFY };
	CComboBox	m_ctlCombo;
	CColorPicker	m_ctlColor;
	float	m_fRotX;
	float	m_fRotY;
	float	m_fRotZ;
	float	m_fPosX;
	float	m_fPosY;
	float	m_fPosZ;
	BOOL	m_bHidden;
	float	m_fFOV;
	int		m_nFrom;
	int		m_nTo;
	CString	m_strName;
	float	m_fUpX;
	float	m_fUpY;
	float	m_fUpZ;
	float	m_fFar;
	float	m_fNear;
	//}}AFX_DATA

// Attributes 
public: 

// Operations 
public: 

// Overrides 
	// ClassWizard generated virtual function overrides 
	//{{AFX_VIRTUAL(CModifyDialog) 
	protected: 
	virtual void DoDataExchange(CDataExchange* pDX); 
	//}}AFX_VIRTUAL 

// Implementation 
public: 

	void UpdateInfo(Object* pObject);
	void OnUpdateCmdUI(CFrameWnd * pTarget, BOOL bDisableIfNoHndler);
//	BOOL Create(CWnd* pParentWnd, UINT nIDTemplate, UINT nStyle, UINT nID); 
	//BOOL Create(CWnd* pParentWnd, LPCTSTR lpszTemplateName, UINT nStyle, UINT nID); 

protected: 
	virtual BOOL OnInitDialogBar(); 
	Object* m_pObject;
	BYTE m_nType;

protected: 
	void UpdateControls(BYTE nType);
	void OnMenuClick(UINT nID);

	// Generated message map functions 
	//{{AFX_MSG(CModifyDialog)
	afx_msg void OnModdlgPiece();
	afx_msg void OnSelendokModdlgList();
	afx_msg void OnModdlgApply();
	afx_msg void OnDropdownModdlgList();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP() 
}; 

///////////////////////////////////////////////////////////////////////////// 

#endif //!defined(AFX_INITDIALOGBAR_H__46B4D2B3_C982_11D1_8902_0060979C2EFD__INCLUDED_) 
///////////////////////////////////////////////////////////////////////////// 
