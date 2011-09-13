// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__195E1F4E_3FF2_11D2_8202_D2B1707B2D1B__INCLUDED_)
#define AFX_MAINFRM_H__195E1F4E_3FF2_11D2_8202_D2B1707B2D1B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "PieceBar.h"
#include "CADBar.h"
#include "ModDlg.h"
#include "propertiespane.h"

class MainWnd;

class CMFCToolBarNoUpdate : public CMFCToolBar
{
public:
	virtual void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler)
	{
		CMFCToolBar::OnUpdateCmdUI(pTarget, FALSE);
	}
};

class CMainFrame : public CFrameWndEx
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
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Control bar embedded members
	CMFCMenuBar m_wndMenuBar;
	CCADStatusBar m_wndStatusBar;
	CMFCToolBarNoUpdate m_wndStandardBar;
	CMFCToolBarNoUpdate m_wndToolsBar;
	CMFCToolBarNoUpdate m_wndAnimationBar;
	CMFCToolBar m_wndInvisibleToolBar;
	CPiecesBar m_wndPiecesBar;
	CSplitterWnd m_wndSplitter;

	void UpdateMenuAccelerators();
	void SetStatusBarPane(UINT ID, const char* Text);
	void SetStatusBarMessage(const char* Message)
		{ m_strStatusBar = Message; }

protected:
	CModifyDialog	m_wndModifyDlg;
	CPropertiesPane m_wndProperties;

	WINDOWPLACEMENT m_wpPrev;
	CToolBar* m_pwndFullScrnBar;
	CRect m_FullScreenWindowRect;
	CString m_strStatusBar;

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	afx_msg void CMainFrame::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnViewFullscreen();
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg void OnFilePrintPieceList();
	afx_msg void OnActivateApp(BOOL bActive, DWORD hTask);
	afx_msg void OnViewNewView();
	afx_msg LRESULT OnSetMessageString(WPARAM wParam, LPARAM lParam);
	afx_msg void OnDropFiles(HDROP hDropInfo);
	//}}AFX_MSG

	// Status bar
	void GetMessageString(UINT nID, CString& rMessage) const;

	afx_msg void OnPieceBar(UINT nID);
	afx_msg void OnUpdatePieceBar(CCmdUI* pCmdUI);
	afx_msg void OnUpdateAction(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSnap(CCmdUI* pCmdUI);
	afx_msg void OnUpdateLock(CCmdUI* pCmdUI);
	
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
