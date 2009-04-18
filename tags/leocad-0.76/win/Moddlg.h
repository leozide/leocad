// ModDlg.h: interface for the CModifyDialog class. 

#ifndef _MODDLG_H_
#define _MODDLG_H_

#include "ClrPick.h"
#include "sizecbar.h"
#include "scbarg.h"
#include "RollUpCtrl.h"
#include "algebra.h"

class lcObject;
class lcCamera;

// CModifyPieceDlg dialog
class CModifyPieceDlg : public CDialog
{
	DECLARE_DYNAMIC(CModifyPieceDlg)

public:
	CModifyPieceDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CModifyPieceDlg();

// Dialog Data
	//{{AFX_DATA(CModifyPieceDlg)
	enum { IDD = IDD_MODIFY_PIECE };
	float	m_PosX;
	float	m_PosY;
	float	m_PosZ;
	float	m_RotX;
	float	m_RotY;
	float	m_RotZ;
	BOOL	m_Hidden;
	int	m_From;
	int	m_To;
	CColorPicker	m_Color;
	//}}AFX_DATA

	void UpdateInfo(class lcPiece* piece);
	void Apply(class lcPiece* piece);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	// Generated message map functions
	//{{AFX_MSG(CModifyPieceDlg)
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnDataChange();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

// CModifyCameraDlg dialog
class CModifyCameraDlg : public CDialog
{
	DECLARE_DYNAMIC(CModifyCameraDlg)

public:
	CModifyCameraDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CModifyCameraDlg();

// Dialog Data
	//{{AFX_DATA(CModifyCameraDlg)
	enum { IDD = IDD_MODIFY_CAMERA };
	float	m_PosX;
	float	m_PosY;
	float	m_PosZ;
	float	m_TargetX;
	float	m_TargetY;
	float	m_TargetZ;
	float	m_Roll;
	float	m_FOV;
	BOOL  m_Clip;
	float	m_Near;
	float	m_Far;
	BOOL	m_Ortho;
	BOOL  m_Cone;
	BOOL	m_Hidden;
	//}}AFX_DATA

	void UpdateInfo(class lcCamera* camera);
	void Apply(class lcCamera* camera);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	// Generated message map functions
	//{{AFX_MSG(CModifyCameraDlg)
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnDataChange();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

// CModifyLightDlg dialog
class CModifyLightDlg : public CDialog
{
	DECLARE_DYNAMIC(CModifyLightDlg)

public:
	CModifyLightDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CModifyLightDlg();

// Dialog Data
	//{{AFX_DATA(CModifyLightDlg)
	enum { IDD = IDD_MODIFY_LIGHT };
	float m_PosX;
	float m_PosY;
	float m_PosZ;
	float m_TargetX;
	float m_TargetY;
	float m_TargetZ;
	CButton m_Ambient;
	CButton m_Diffuse;
	CButton m_Specular;
	float m_Constant;
	float m_Linear;
	float m_Quadratic;
	float m_Cutoff;
	float m_Exponent;
	BOOL m_Hidden;
	//}}AFX_DATA

	Vector4 m_AmbientColor;
	Vector4 m_DiffuseColor;
	Vector4 m_SpecularColor;

	void UpdateInfo(class lcLight* light);
	void Apply(class lcLight* light);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	// Generated message map functions
	//{{AFX_MSG(CModifyLightDlg)
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnDataChange();
	afx_msg void OnAmbient();
	afx_msg void OnDiffuse();
	afx_msg void OnSpecular();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

// CModifyDialog window 
class CModifyDialog : public CDialog
{ 
	DECLARE_DYNAMIC(CModifyDialog) 

// Construction / Destruction 
public: 
	CModifyDialog(CWnd* pParent = NULL); 
	virtual ~CModifyDialog(); 

	//{{AFX_DATA(CModifyDialog)
	enum { IDD = IDD_MODIFY };
	CComboBox	m_ctlCombo;
	CString	m_strName;
	//}}AFX_DATA

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
	void UpdateInfo(lcObject* pObject);
	void OnUpdateCmdUI(CFrameWnd * pTarget, BOOL bDisableIfNoHndler);
	BOOL OnInitDialogBar();

	CRollupCtrl m_RollUp;
	CModifyPieceDlg m_PieceDlg;
	CModifyCameraDlg m_CameraDlg;
	CModifyLightDlg m_LightDlg;

protected: 
	void PositionChildren();

	lcObject* m_pObject;
	int m_CurrentType;

protected: 
	void UpdateControls(int Type);
	void OnMenuClick(UINT nID);

	// Generated message map functions 
	//{{AFX_MSG(CModifyDialog)
	afx_msg void OnModdlgPiece();
	afx_msg void OnSelendokModdlgList();
	afx_msg void OnModdlgApply();
	afx_msg void OnDropdownModdlgList();
	afx_msg void OnMove(int x, int y);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP() 
}; 

class CModifyDialogBar : public CSizingControlBarG
{
// Construction
public:
	CModifyDialogBar();

// Attributes
public:
	CModifyDialog m_ModifyDlg;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CModifyDialogBar)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CModifyDialogBar();

protected:
	CSize CalcDynamicLayout(int nLength, DWORD dwMode);
	CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz);

	// Generated message map functions
protected:
	//{{AFX_MSG(CModifyDialogBar)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void CModifyDialogBar::OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // _MODDLG_H_
