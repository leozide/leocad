// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__195E1F4E_3FF2_11D2_8202_D2B1707B2D1B__INCLUDED_)
#define AFX_MAINFRM_H__195E1F4E_3FF2_11D2_8202_D2B1707B2D1B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "FlatBar.h"
#include "PieceBar.h"
#include "CADBar.h"
#include "BMPMenu.h"
#include "ModDlg.h"

class MainWnd;

class CMainFrame : public CFrameWnd
{
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
public:
	HMENU NewMenu();
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

  // control bar embedded members
	CCADStatusBar m_wndStatusBar;
	CFlatToolBar  m_wndStandardBar;
	CFlatToolBar  m_wndToolsBar;
	CFlatToolBar  m_wndAnimationBar;
	CPiecesBar    m_wndPiecesBar;

protected:
	CModifyDialog	m_wndModifyDlg;

	CBMPMenu m_bmpMenu;
	WINDOWPLACEMENT m_wpPrev;
	CToolBar* m_pwndFullScrnBar;
	CRect m_FullScreenWindowRect;

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	afx_msg LRESULT OnMenuChar(UINT nChar, UINT nFlags, CMenu* pMenu);
	afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
	afx_msg void OnViewFullscreen();
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg void OnFilePrintPieceList();
	afx_msg void OnActivateApp(BOOL bActive, HTASK hTask);
	afx_msg void OnViewNewView();
	//}}AFX_MSG

	// Status bar
	void GetMessageString(UINT nID, CString& rMessage) const;

	afx_msg void OnPieceBar(UINT nID);
	afx_msg void OnUpdatePieceBar(CCmdUI* pCmdUI);
	
	afx_msg LONG OnUpdateList(UINT lParam, LONG wParam);
	afx_msg LONG OnPopupClose(UINT lParam, LONG wParam);
	afx_msg LONG OnAddString(UINT lParam, LONG wParam);
	afx_msg LONG OnUpdateInfo(UINT lParam, LONG wParam);
	afx_msg LONG UpdateSettings(UINT lParam, LONG wParam);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__195E1F4E_3FF2_11D2_8202_D2B1707B2D1B__INCLUDED_)
