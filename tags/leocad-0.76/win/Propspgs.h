#ifndef _PROPSPGS_H_
#define _PROPSPGS_H_

#include "defines.h"

/////////////////////////////////////////////////////////////////////////////
// CPropertiesGeneral dialog

class CPropertiesGeneral : public CPropertyPage
{
	DECLARE_DYNCREATE(CPropertiesGeneral)

// Construction
public:
	CString m_strFilename;
	CPropertiesGeneral();
	~CPropertiesGeneral();

// Dialog Data
	//{{AFX_DATA(CPropertiesGeneral)
	enum { IDD = IDD_PROPGENERAL };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPropertiesGeneral)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPropertiesGeneral)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

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
	CString	m_strAuthor;
	CString	m_strComments;
	CString	m_strDescription;
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

	int* m_PiecesUsed;
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
