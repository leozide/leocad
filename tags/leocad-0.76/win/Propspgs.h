#ifndef _PROPSPGS_H_
#define _PROPSPGS_H_

#include "defines.h"
#include "typedefs.h"
#include "lc_array.h"

/////////////////////////////////////////////////////////////////////////////
// CPropertiesSummary dialog

class CPropertiesSummary : public CPropertyPage
{
	DECLARE_DYNCREATE(CPropertiesSummary)

// Construction
public:
	CPropertiesSummary();
	~CPropertiesSummary();

// Dialog Data
	//{{AFX_DATA(CPropertiesSummary)
	enum { IDD = IDD_PROPSUMMARY };
	CString m_Name;
	CString	m_Author;
	CString	m_Comments;
	CString	m_Description;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPropertiesSummary)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPropertiesSummary)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CPropertiesScene dialog

class CPropertiesScene : public CPropertyPage
{
	DECLARE_DYNCREATE(CPropertiesScene)

// Construction
public:
	CPropertiesScene();
	~CPropertiesScene();
	COLORREF m_crBackground;
	COLORREF m_crAmbient;
	COLORREF m_crFog;
	COLORREF m_crGrad1;
	COLORREF m_crGrad2;

// Dialog Data
	//{{AFX_DATA(CPropertiesScene)
	enum { IDD = IDD_PROPSCENE };
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
	//{{AFX_VIRTUAL(CPropertiesScene)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPropertiesScene)
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
// CPropertiesPieces dialog

class CPropertiesPieces : public CPropertyPage
{
	DECLARE_DYNCREATE(CPropertiesPieces)

// Construction
public:
	CPropertiesPieces();
	~CPropertiesPieces();

// Dialog Data
	//{{AFX_DATA(CPropertiesPieces)
	enum { IDD = IDD_PROPPIECES };
	CListCtrl	m_List;
	//}}AFX_DATA

	lcObjArray<lcPiecesUsedEntry> m_PiecesUsed;
	int* m_ColorColumn;
	int m_SortColumn;
	bool m_SortAscending;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPropertiesPieces)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPropertiesPieces)
	virtual BOOL OnInitDialog();
	afx_msg void OnColumnclickPropPiecesList(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // _PROPSPGS_H_
