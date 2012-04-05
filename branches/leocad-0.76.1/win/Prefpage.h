// PrefPage.h : header file
//

#ifndef __PREFPAGE_H__
#define __PREFPAGE_H__

#include "keyedit.h"

/////////////////////////////////////////////////////////////////////////////
// CPreferencesGeneral dialog

class CPreferencesGeneral : public CPropertyPage
{
	DECLARE_DYNCREATE(CPreferencesGeneral)

// Construction
public:
	void SetOptions(int nSaveInterval, int nMouse, const char* strFolder, const char* strUser);
	void GetOptions(int* nSaveTime, int* nMouse, char* strFolder, char* strUser);
	CPreferencesGeneral();
	~CPreferencesGeneral();

// Dialog Data
	//{{AFX_DATA(CPreferencesGeneral)
	enum { IDD = IDD_PREFGENERAL };
	CSliderCtrl	m_ctlMouse;
	BOOL	m_bSubparts;
	int		m_nSaveTime;
	CString	m_strFolder;
	BOOL	m_bAutoSave;
	CString	m_bUser;
	CString	m_strUser;
	BOOL	m_Updates;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPreferencesGeneral)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	int m_nMouse;

	// Generated message map functions
	//{{AFX_MSG(CPreferencesGeneral)
	afx_msg void OnFolderBrowse();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};


/////////////////////////////////////////////////////////////////////////////
// CPreferencesDetail dialog

class CPreferencesDetail : public CPropertyPage
{
	DECLARE_DYNCREATE(CPreferencesDetail)

// Construction
public:
	void SetOptions(DWORD dwDetail, float fLine);
	void GetOptions(DWORD* dwDetail, float* fLine);
	CPreferencesDetail();
	~CPreferencesDetail();

// Dialog Data
	//{{AFX_DATA(CPreferencesDetail)
	enum { IDD = IDD_PREFDETAIL };
	BOOL	m_bAntialiasing;
	BOOL	m_bEdges;
	BOOL	m_bLighting;
	BOOL	m_bSmooth;
	float	m_fLineWidth;
	BOOL	m_bFast;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPreferencesDetail)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPreferencesDetail)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CPreferencesDrawing dialog

class CPreferencesDrawing : public CPropertyPage
{
	DECLARE_DYNCREATE(CPreferencesDrawing)

// Construction
public:
	void SetOptions(unsigned long dwSnap, unsigned short nAngle, unsigned short nGrid);
	void GetOptions(unsigned long* dwSnap, unsigned short* nAngle, unsigned short* nGrid);
	CPreferencesDrawing();
	~CPreferencesDrawing();

// Dialog Data
	//{{AFX_DATA(CPreferencesDrawing)
	enum { IDD = IDD_PREFDRAWING };
	int		m_nAngle;
	BOOL	m_bAxis;
	BOOL	m_bCentimeters;
	BOOL	m_bFixed;
	BOOL	m_bGrid;
	int		m_nGridSize;
	BOOL	m_bLockX;
	BOOL	m_bLockY;
	BOOL	m_bLockZ;
	BOOL	m_bMove;
	BOOL	m_bSnapA;
	BOOL	m_bSnapX;
	BOOL	m_bSnapY;
	BOOL	m_bSnapZ;
	BOOL	m_bGlobal;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPreferencesDrawing)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPreferencesDrawing)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};


/////////////////////////////////////////////////////////////////////////////
// CPreferencesScene dialog

class CPreferencesScene : public CPropertyPage
{
	DECLARE_DYNCREATE(CPreferencesScene)

// Construction
public:
	void SetOptions(unsigned long nScene, float fDensity, char* strBackground, float* fBackground, float* fFog, float* fAmbient, float* fGrad1, float* fGrad2);
	void GetOptions(unsigned long* nScene, float* fDensity, char* strBackground, float* fBackground, float* fFog, float* fAmbient, float* fGrad1, float* fGrad2);
	CPreferencesScene();
	~CPreferencesScene();
	COLORREF m_crBackground;
	COLORREF m_crAmbient;
	COLORREF m_crFog;
	COLORREF m_crGrad1;
	COLORREF m_crGrad2;

// Dialog Data
	//{{AFX_DATA(CPreferencesScene)
	enum { IDD = IDD_PREFSCENE };
	CButton	m_btnGrad1;
	CButton	m_btnGrad2;
	CButton	m_btnAmbient;
	CButton	m_btnFog;
	CButton	m_btnBackground;
	CString	m_strBackground;
	BOOL	m_bTile;
	BOOL	m_bFog;
	BYTE	m_nFogDensity;
	BOOL	m_bFloor;
	int		m_nBackground;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPreferencesScene)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPreferencesScene)
	afx_msg void OnBackgroundBrowse();
	afx_msg void OnBackgroundColor();
	afx_msg void OnAmbientLight();
	afx_msg void OnFogColor();
	virtual BOOL OnInitDialog();
	afx_msg void OnGradColor1();
	afx_msg void OnGradColor2();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};


/////////////////////////////////////////////////////////////////////////////
// CPreferencesPrint dialog

class CPreferencesPrint : public CPropertyPage
{
	DECLARE_DYNCREATE(CPreferencesPrint)

// Construction
public:
	void SetOptions(CString strHeader, CString strFooter);
	void GetOptions(char* strHeader, char* strFooter);
	CPreferencesPrint();
	~CPreferencesPrint();

// Dialog Data
	//{{AFX_DATA(CPreferencesPrint)
	enum { IDD = IDD_PREFPRINT };
	float	m_fBottom;
	float	m_fLeft;
	float	m_fRight;
	float	m_fTop;
	BOOL	m_bNumbers;
	CString	m_strHeader;
	CString	m_strFooter;
	BOOL	m_bBorder;
	int		m_nInstCols;
	int		m_nInstRows;
	int		m_nCatCols;
	int		m_nCatRows;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPreferencesPrint)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void OnHeaderClick(UINT nID);
	// Generated message map functions
	//{{AFX_MSG(CPreferencesPrint)
	virtual BOOL OnInitDialog();
	afx_msg void OnFooterButton();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CPreferencesKeyboard dialog

class CPreferencesKeyboard : public CPropertyPage
{
	DECLARE_DYNCREATE(CPreferencesKeyboard)

// Construction
public:
	void SetOptions();
	void GetOptions();
	CPreferencesKeyboard();
	~CPreferencesKeyboard();

// Dialog Data
	//{{AFX_DATA(CPreferencesKeyboard)
	enum { IDD = IDD_PREFKEYBOARD };
	CKeyEdit	m_Edit;
	CButton	m_Assign;
	CButton	m_Remove;
	CListBox	m_List;
	CComboBox	m_Combo;
	CString	m_strFileName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPreferencesKeyboard)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPreferencesKeyboard)
	virtual BOOL OnInitDialog();
	afx_msg void OnKeydlgRemove();
	afx_msg void OnKeydlgAssign();
	afx_msg void OnKeydlgReset();
	afx_msg void OnSelchangeKeydlgCmdlist();
	afx_msg void OnChangeKeydlgKeyedit();
	afx_msg void OnKeydlgSave();
	afx_msg void OnKeydlgLoad();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

#endif // __PREFPAGE_H__
