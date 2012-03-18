#ifndef _MAINFRM_H_
#define _MAINFRM_H_

#include "PieceBar.h"
#include "CADBar.h"
#include "propertiespane.h"

class MainWnd;
class CDynamicSplitterWnd;

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
	//}}AFX_VIRTUAL

	virtual BOOL OnShowPopupMenu(CMFCPopupMenu* pMenuPopup);

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
	CPropertiesPane m_wndProperties;

	CTypedPtrArray<CPtrArray, CDynamicSplitterWnd*> m_SplitterList;

	void UpdateMenuAccelerators();
	void SetStatusBarPane(UINT ID, const char* Text);
	void SetStatusBarMessage(const char* Message)
		{ m_strStatusBar = Message; }

protected:
	CString m_strStatusBar;

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnViewFullscreen();
	afx_msg void OnFilePrintPieceList();
	afx_msg void OnActivateApp(BOOL bActive, DWORD hTask);
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
	afx_msg void OnUpdateCamera(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSnapXY(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSnapZ(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSnapA(CCmdUI* pCmdUI);
	afx_msg void OnUpdateStepNext(CCmdUI* pCmdUI);
	afx_msg void OnUpdateStepPrevious(CCmdUI* pCmdUI);
	afx_msg void OnUpdateStepFirst(CCmdUI* pCmdUI);
	afx_msg void OnUpdateStepLast(CCmdUI* pCmdUI);
	
	afx_msg LONG OnUpdateList(UINT lParam, LONG wParam);
	afx_msg LONG OnPopupClose(UINT lParam, LONG wParam);
	afx_msg LONG OnAddString(UINT lParam, LONG wParam);
	afx_msg LONG OnUpdateInfo(UINT lParam, LONG wParam);
	afx_msg LONG UpdateSettings(UINT lParam, LONG wParam);

	afx_msg void OnViewSplitVertically();
	afx_msg void OnViewSplitHorizontally();
	afx_msg void OnViewDeleteView();
	afx_msg void OnViewResetViews();

	DECLARE_MESSAGE_MAP()
};

#endif // _MAINFRM_H_
