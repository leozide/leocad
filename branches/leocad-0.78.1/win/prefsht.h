// PrefSht.h : header file
//
// This class defines custom modal property sheet 
// CPreferencesSheet.
 
#ifndef __PREFSHT_H__
#define __PREFSHT_H__

#include "PrefPage.h"
#include "DisabTab.h"

/////////////////////////////////////////////////////////////////////////////
// CPreferencesSheet

class CPreferencesSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CPreferencesSheet)

// Construction
public:
	CPreferencesSheet(CWnd* pWndParent = NULL);

// Attributes
public:
	CPreferencesGeneral m_PageGeneral;
	CPreferencesDetail m_PageDetail;
	CPreferencesDrawing m_PageDrawing;
	CPreferencesScene m_PageScene;
	CPreferencesPrint m_PagePrint;
	CPreferencesKeyboard m_PageKeyboard;

	CTabCtrlWithDisable m_tabCtrl;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPreferencesSheet)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CPreferencesSheet();

// Generated message map functions
protected:
	//{{AFX_MSG(CPreferencesSheet)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	afx_msg void OnDefault();

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#endif	// __PREFSHT_H__
