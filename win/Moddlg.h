// ModDlg.h: interface for the CModifyDialog class. 

#ifndef _MODDLG_H_
#define _MODDLG_H_

#include "ClrPick.h"
#include "sizecbar.h"
#include "scbarg.h"

class Object;

// CModifyPieceDlg dialog
class CModifyPieceDlg : public CDialog
{
	DECLARE_DYNAMIC(CModifyPieceDlg)

public:
	CModifyPieceDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CModifyPieceDlg();

// Dialog Data
	//{{AFX_DATA(CModifyDialog)
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

	void UpdateInfo(class Piece* piece);
	void Apply(class Piece* piece);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	// Generated message map functions
	//{{AFX_MSG(CModifyPieceDlg)
	virtual void OnOK();
	virtual void OnCancel();
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
	//{{AFX_DATA(CModifyDialog)
	enum { IDD = IDD_MODIFY_CAMERA };
	float	m_PosX;
	float	m_PosY;
	float	m_PosZ;
	float	m_TargetX;
	float	m_TargetY;
	float	m_TargetZ;
	float	m_UpX;
	float	m_UpY;
	float	m_UpZ;
	float	m_FOV;
	float	m_Near;
	float	m_Far;
	BOOL	m_Ortho;
	BOOL	m_Hidden;
	//}}AFX_DATA

	void UpdateInfo(class Camera* camera);
	void Apply(class Camera* camera);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	// Generated message map functions
	//{{AFX_MSG(CModifyCameraDlg)
	virtual void OnOK();
	virtual void OnCancel();
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
	enum { IDD = IDD_MODIFY_LIGHT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	// Generated message map functions
	//{{AFX_MSG(CModifyLightDlg)
	virtual void OnOK();
	virtual void OnCancel();
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
	void UpdateInfo(Object* pObject);
	void OnUpdateCmdUI(CFrameWnd * pTarget, BOOL bDisableIfNoHndler);
	BOOL OnInitDialogBar();

protected: 
	void PositionChildren();

	Object* m_pObject;
	int m_CurrentType;

	CModifyPieceDlg m_PieceDlg;
	CModifyCameraDlg m_CameraDlg;
	CModifyLightDlg m_LightDlg;

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

	// Generated message map functions
protected:
	//{{AFX_MSG(CModifyDialogBar)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


#endif // _MODDLG_H_

