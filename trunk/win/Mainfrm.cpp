#include "lc_global.h"
#include "LeoCAD.h"
#include "MainFrm.h"
#include "Camera.h"
#include "project.h"
#include "view.h"
#include "message.h"
#include "globals.h"
#include "mainwnd.h"
#include "cadview.h"
#include "console.h"
#include "keyboard.h"
#include "system.h"
#include "lc_library.h"
#include "lc_application.h"
#include "Print.h"
#include "dynsplit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void mainframe_listener(int message, void *data, void *user)
{
  if (message == LC_MSG_FOCUS_CHANGED)
  {
    CWnd* pFrame = AfxGetMainWnd();
    if (pFrame != NULL)
      pFrame->PostMessage(WM_LC_UPDATE_INFO, (WPARAM)data, 0);
  }
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWndEx)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_WM_SETFOCUS()
	ON_WM_DRAWITEM()
	ON_WM_MEASUREITEM()
	ON_WM_MENUCHAR()
	ON_WM_INITMENUPOPUP()
	ON_COMMAND(ID_VIEW_FULLSCREEN, OnViewFullscreen)
	ON_COMMAND(ID_FILE_PRINTPIECELIST, OnFilePrintPieceList)
	ON_WM_ACTIVATEAPP()
	ON_MESSAGE(WM_SETMESSAGESTRING, OnSetMessageString)
	ON_WM_DROPFILES()
	//}}AFX_MSG_MAP
	ON_COMMAND_RANGE(ID_PIECEBAR_NUMBERS, ID_PIECEBAR_SUBPARTS, OnPieceBar)
	ON_UPDATE_COMMAND_UI_RANGE(ID_PIECEBAR_NUMBERS, ID_PIECEBAR_SUBPARTS, OnUpdatePieceBar)
	// Global help commands
	ON_COMMAND(ID_HELP_FINDER, CFrameWndEx::OnHelpFinder)
	ON_COMMAND(ID_HELP, CFrameWndEx::OnHelp)
	ON_COMMAND(ID_CONTEXT_HELP, CFrameWndEx::OnContextHelp)
	ON_COMMAND(ID_DEFAULT_HELP, CFrameWndEx::OnHelpFinder)
	// User messages
	ON_MESSAGE(WM_LC_UPDATE_LIST, OnUpdateList)
	ON_MESSAGE(WM_LC_POPUP_CLOSE, OnPopupClose)
	ON_MESSAGE(WM_LC_ADD_COMBO_STRING, OnAddString)
	ON_MESSAGE(WM_LC_UPDATE_INFO, OnUpdateInfo)
	ON_MESSAGE(WM_LC_UPDATE_SETTINGS, UpdateSettings)
	ON_REGISTERED_MESSAGE(AFX_WM_RESETTOOLBAR, OnToolbarReset)
	// Toolbar show/hide
	ON_COMMAND_EX(ID_VIEW_ANIMATION_BAR,  &CFrameWndEx::OnPaneCheck)
	ON_COMMAND_EX(ID_VIEW_TOOLS_BAR,  &CFrameWndEx::OnPaneCheck)
	ON_COMMAND_EX(ID_VIEW_PIECES_BAR,  &CFrameWndEx::OnPaneCheck)
	ON_COMMAND_EX(ID_VIEW_PROPERTIES_BAR,  &CFrameWndEx::OnPaneCheck)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ANIMATION_BAR, &CFrameWndEx::OnUpdatePaneMenu)
	ON_UPDATE_COMMAND_UI(ID_VIEW_TOOLS_BAR, &CFrameWndEx::OnUpdatePaneMenu)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PIECES_BAR, &CFrameWndEx::OnUpdatePaneMenu)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PROPERTIES_BAR, &CFrameWndEx::OnUpdatePaneMenu)
	ON_UPDATE_COMMAND_UI_RANGE(ID_ACTION_SELECT, ID_ACTION_ROLL, OnUpdateAction)
	ON_UPDATE_COMMAND_UI_RANGE(ID_SNAP_SNAPX, ID_SNAP_SNAPNONE, OnUpdateSnap)
	ON_UPDATE_COMMAND_UI_RANGE(ID_LOCK_LOCKX, ID_LOCK_UNLOCKALL, OnUpdateLock)
	ON_UPDATE_COMMAND_UI_RANGE(ID_CAMERA_FIRST, ID_CAMERA_LAST, OnUpdateCamera)
	ON_UPDATE_COMMAND_UI_RANGE(ID_SNAP_0, ID_SNAP_9, OnUpdateSnapXY)
	ON_UPDATE_COMMAND_UI_RANGE(ID_SNAP_10, ID_SNAP_19, OnUpdateSnapZ)
	ON_UPDATE_COMMAND_UI_RANGE(ID_SNAP_20, ID_SNAP_29, OnUpdateSnapA)
	ON_UPDATE_COMMAND_UI_RANGE(ID_TRANSFORM_ABSOLUTE_TRANSLATION, ID_TRANSFORM_RELATIVE_ROTATION, OnUpdateTransform)
	ON_UPDATE_COMMAND_UI(ID_VIEW_STEP_NEXT, OnUpdateStepNext)
	ON_UPDATE_COMMAND_UI(ID_VIEW_STEP_PREVIOUS, OnUpdateStepPrevious)
	ON_UPDATE_COMMAND_UI(ID_VIEW_STEP_FIRST, OnUpdateStepFirst)
	ON_UPDATE_COMMAND_UI(ID_VIEW_STEP_LAST, OnUpdateStepLast)
	ON_COMMAND(ID_VIEW_SPLITVERTICALLY, OnViewSplitVertically)
	ON_COMMAND(ID_VIEW_SPLITHORIZONTALLY, OnViewSplitHorizontally)
	ON_COMMAND(ID_VIEW_DELETEVIEW, OnViewDeleteView)
	ON_COMMAND(ID_VIEW_RESETVIEWS, OnViewResetViews)
END_MESSAGE_MAP()

static UINT indicators[] =
	{ ID_SEPARATOR, ID_INDICATOR_POSITION, ID_INDICATOR_SNAP, ID_INDICATOR_STEP };

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	m_bAutoMenuEnable = FALSE;
	mTransformMode = 0;
}

CMainFrame::~CMainFrame()
{
	for (int i = 0; i < m_SplitterList.GetSize(); i++)
		delete m_SplitterList[i];
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows7));

	if (!m_wndMenuBar.Create(this))
	{
		TRACE0("Failed to create menubar\n");
		return -1;      // fail to create
	}

	m_wndMenuBar.SetPaneStyle(m_wndMenuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndMenuBar.EnableDocking(CBRS_ALIGN_ANY);

	if (!m_wndStatusBar.Create(this) || !m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	m_wndStatusBar.SetPaneStyle(0, SBPS_STRETCH|SBPS_NORMAL);

	if (!m_wndStandardBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE) || !m_wndStandardBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	m_wndStandardBar.SetWindowText(_T("Standard"));
	m_wndStandardBar.EnableDocking(CBRS_ALIGN_ANY);

	if (!m_wndToolsBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE, ID_VIEW_TOOLS_BAR) || !m_wndToolsBar.LoadToolBar(IDR_TOOLSBAR))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	m_wndToolsBar.SetWindowText(_T("Drawing"));
	m_wndToolsBar.EnableDocking(CBRS_ALIGN_ANY);

	if (!m_wndAnimationBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE, ID_VIEW_ANIMATION_BAR) || !m_wndAnimationBar.LoadToolBar(IDR_ANIMATORBAR))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	m_wndAnimationBar.SetWindowText(_T("Animation"));
	m_wndAnimationBar.EnableDocking(CBRS_ALIGN_ANY);

	if (!m_wndPiecesBar.Create(_T("Pieces"), this, CRect(0, 0, 200, 200), TRUE, ID_VIEW_PIECES_BAR, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI))
	{
		TRACE0("Failed to create Pieces window\n");
		return -1; // failed to create
	}

	m_wndPiecesBar.EnableDocking(CBRS_ALIGN_ANY);

	EnableDocking(CBRS_ALIGN_ANY);

//	UpdateMenuAccelerators();
	CMFCToolBar::AddToolBarForImageCollection(IDR_INVISIBLE);

	if (!m_wndProperties.Create("Properties", this, CRect(0, 0, 200, 200), TRUE, ID_VIEW_PROPERTIES_BAR, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI))
	{
		TRACE0("Failed to create Properties window\n");
		return FALSE; // failed to create
	}

	m_wndProperties.EnableDocking(CBRS_ALIGN_ANY);
	ShowPane(&m_wndProperties, FALSE, FALSE, FALSE);
	DockPane(&m_wndProperties);

	EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_wndMenuBar);
	DockPane(&m_wndStandardBar);
	DockPane(&m_wndAnimationBar);
	DockPaneLeftOf(&m_wndToolsBar, &m_wndAnimationBar);
	DockPane(&m_wndPiecesBar);

	CDockingManager::SetDockingMode(DT_SMART);
	EnableAutoHidePanes(CBRS_ALIGN_ANY);

	messenger->Listen(&mainframe_listener, this);

	main_window->SetXID(this);

	const int ToolBarVersion = 3;
	if (theApp.GetProfileInt(_T("Settings"), _T("ToolBarVersion"), 0) != ToolBarVersion)
	{
		theApp.CleanState();
		theApp.WriteProfileInt(_T("Settings"), _T("ToolBarVersion"), ToolBarVersion);
	}

//  console.SetWindowCallback(&mainframe_console_func, m_wndSplitter.GetPane(1, 0));

	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG

void CMainFrame::AssertValid() const
{
	CFrameWndEx::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWndEx::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

class CTransformEditCtrl : public CMFCToolBarEditCtrl
{
public:
	CTransformEditCtrl(CMFCToolBarEditBoxButton& edit)
		: CMFCToolBarEditCtrl(edit)
	{
	}

public:
	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
		{
			CMainFrame* MainFrame = (CMainFrame*)GetTopLevelFrame();

			lcVector3 Transform(0.0f, 0.0f, 0.0f);

			for (int Axis = 0; Axis < 3; Axis++)
			{
				CString Text = CMFCToolBarEditBoxButton::GetContentsAll(ID_TRANSFORM_X + Axis);
				sscanf(Text, "%f", &Transform[Axis]);
			}

			if (MainFrame->mTransformMode == LC_TRANSFORM_ABSOLUTE_TRANSLATION || MainFrame->mTransformMode == LC_TRANSFORM_RELATIVE_TRANSLATION)
				lcGetActiveProject()->ConvertFromUserUnits(Transform);
			lcGetActiveProject()->TransformSelectedObjects((LC_TRANSFORM_TYPE)MainFrame->mTransformMode, Transform);

			return TRUE;
		}

		return CMFCToolBarEditCtrl::PreTranslateMessage(pMsg);
	}
};

class CTransformEditButton : public CMFCToolBarEditBoxButton
{
	DECLARE_SERIAL(CTransformEditButton)

public:
	CTransformEditButton()
	{
	}

	CTransformEditButton(UINT uiID, int iImage, DWORD dwStyle = ES_AUTOHSCROLL, int iWidth = 0)
		: CMFCToolBarEditBoxButton(uiID, iImage, dwStyle, iWidth)
	{
	}

	virtual void Serialize(CArchive& ar)
	{
		CMFCToolBarEditBoxButton::Serialize(ar);

		if (ar.IsLoading())
			m_strContents.Empty();
	}

	virtual CEdit* CreateEdit(CWnd* pWndParent, const CRect& rect)
	{
		CTransformEditCtrl* pWndEdit = new CTransformEditCtrl(*this);
		if (!pWndEdit->Create(m_dwStyle, rect, pWndParent, m_nID))
		{
			delete pWndEdit;
			return NULL;
		}

		return pWndEdit;
	}
};

IMPLEMENT_SERIAL(CTransformEditButton, CMFCToolBarEditBoxButton, 1)

LRESULT CMainFrame::OnToolbarReset(WPARAM wParam, LPARAM lParam)
{
	if (wParam == IDR_MAINFRAME)
	{
		CMenu PopupMenus;
		PopupMenus.LoadMenu(IDR_POPUPS);
		CMenu* Popup;

		Popup = PopupMenus.GetSubMenu(8);
		CMFCToolBarMenuButton NewLock(ID_LOCK_ON,Popup->GetSafeHmenu(), GetCmdMgr()->GetCmdImage(ID_LOCK_ON));
		m_wndStandardBar.ReplaceButton(ID_LOCK_ON, NewLock);

		Popup = PopupMenus.GetSubMenu(2);
		CMFCToolBarMenuButton NewSnap(ID_SNAP_ON, Popup->GetSafeHmenu(), GetCmdMgr()->GetCmdImage(ID_SNAP_ON));
		m_wndStandardBar.ReplaceButton(ID_SNAP_ON, NewSnap);

		Popup = PopupMenus.GetSubMenu(9);
		CMFCToolBarMenuButton Transform(ID_TRANSFORM_TYPE, Popup->GetSafeHmenu(), GetCmdMgr()->GetCmdImage(ID_TRANSFORM_TYPE));
		m_wndStandardBar.ReplaceButton(ID_TRANSFORM_TYPE, Transform);

		CTransformEditButton TransformX(ID_TRANSFORM_X, 0, ES_AUTOHSCROLL, 50);
		m_wndStandardBar.ReplaceButton(ID_TRANSFORM_X, TransformX);

		CTransformEditButton TransformY(ID_TRANSFORM_Y, 0, ES_AUTOHSCROLL, 50);
		m_wndStandardBar.ReplaceButton(ID_TRANSFORM_Y, TransformY);

		CTransformEditButton TransformZ(ID_TRANSFORM_Z, 0, ES_AUTOHSCROLL, 50);
		m_wndStandardBar.ReplaceButton(ID_TRANSFORM_Z, TransformZ);
	}

	return 0;
}

void CMainFrame::OnUpdateAction(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(pCmdUI->m_nID == ID_ACTION_SELECT + lcGetActiveProject()->GetCurAction());
}

void CMainFrame::OnUpdateSnap(CCmdUI* pCmdUI)
{
	Project* project = lcGetActiveProject();
	lcuint32 Snap = project->GetSnap();

	switch (pCmdUI->m_nID)
	{
	case ID_SNAP_SNAPX:
		pCmdUI->SetCheck(Snap & LC_DRAW_SNAP_X ? TRUE : FALSE);
		break;

	case ID_SNAP_SNAPY:
		pCmdUI->SetCheck(Snap & LC_DRAW_SNAP_Y ? TRUE : FALSE);
		break;

	case ID_SNAP_SNAPZ:
		pCmdUI->SetCheck(Snap & LC_DRAW_SNAP_Z ? TRUE : FALSE);
		break;

	case ID_SNAP_SNAPNONE:
		pCmdUI->Enable(Snap & LC_DRAW_SNAP_XYZ ? TRUE : FALSE);
		break;

	case ID_SNAP_SNAPALL:
		pCmdUI->Enable((Snap & LC_DRAW_SNAP_XYZ) != LC_DRAW_SNAP_XYZ ? TRUE : FALSE);
		break;
	}
}

void CMainFrame::OnUpdateLock(CCmdUI* pCmdUI)
{
	Project* project = lcGetActiveProject();
	lcuint32 Snap = project->GetSnap();

	switch (pCmdUI->m_nID)
	{
	case ID_LOCK_LOCKX:
		pCmdUI->SetCheck(Snap & LC_DRAW_LOCK_X ? TRUE : FALSE);
		break;

	case ID_LOCK_LOCKY:
		pCmdUI->SetCheck(Snap & LC_DRAW_LOCK_Y ? TRUE : FALSE);
		break;

	case ID_LOCK_LOCKZ:
		pCmdUI->SetCheck(Snap & LC_DRAW_LOCK_Z ? TRUE : FALSE);
		break;

	case ID_LOCK_UNLOCKALL:
		pCmdUI->Enable(Snap & LC_DRAW_LOCK_XYZ ? TRUE : FALSE);
		break;
	}
}

void CMainFrame::OnUpdateCamera(CCmdUI* pCmdUI)
{
	Project* project = lcGetActiveProject();

	if (project->mCameras.GetSize())
	{
		Camera* ActiveCamera = project->GetActiveView()->mCamera;
		Camera* MenuCamera = project->mCameras[pCmdUI->m_nID - ID_CAMERA_FIRST];

		pCmdUI->SetRadio(MenuCamera == ActiveCamera);
	}
}

void CMainFrame::OnUpdateSnapXY(CCmdUI* pCmdUI)
{
	int Snap;
	lcGetActiveProject()->GetSnapIndex(&Snap, NULL, NULL);
	pCmdUI->SetRadio(Snap + ID_SNAP_0 == pCmdUI->m_nID);
}

void CMainFrame::OnUpdateSnapZ(CCmdUI* pCmdUI)
{
	int Snap;
	lcGetActiveProject()->GetSnapIndex(NULL, &Snap, NULL);
	pCmdUI->SetRadio(Snap + ID_SNAP_10 == pCmdUI->m_nID);
}

void CMainFrame::OnUpdateSnapA(CCmdUI* pCmdUI)
{
	int Snap;
	lcGetActiveProject()->GetSnapIndex(NULL, NULL, &Snap);
	pCmdUI->SetRadio(Snap + ID_SNAP_20 == pCmdUI->m_nID);
}

void CMainFrame::OnUpdateTransform(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(mTransformMode + ID_TRANSFORM_ABSOLUTE_TRANSLATION == pCmdUI->m_nID);
}

void CMainFrame::OnUpdateStepNext(CCmdUI* pCmdUI)
{
	int Now, Last;

	lcGetActiveProject()->GetTimeRange(&Now, &Last);
	pCmdUI->Enable(Now < Last);
}

void CMainFrame::OnUpdateStepPrevious(CCmdUI* pCmdUI)
{
	int Now;

	Now = lcGetActiveProject()->GetCurrentTime();
	pCmdUI->Enable(Now > 1);
}

void CMainFrame::OnUpdateStepFirst(CCmdUI* pCmdUI)
{
	int Now;

	Now = lcGetActiveProject()->GetCurrentTime();
	pCmdUI->Enable(Now > 1);
}

void CMainFrame::OnUpdateStepLast(CCmdUI* pCmdUI)
{
	int Now, Last;

	lcGetActiveProject()->GetTimeRange(&Now, &Last);
	pCmdUI->Enable(Now < Last);
}

// lParam = update pieces, wParam = update colors
LRESULT CMainFrame::OnUpdateList(UINT lParam, LONG wParam)
{
	if (wParam != 0)
		m_wndPiecesBar.m_wndColorList.SetColorIndex(wParam - 1);

	return TRUE;
}

// Add a string to the pieces combo
LRESULT CMainFrame::OnAddString(UINT lParam, LONG /*wParam*/)
{
	if (lParam == NULL)
	{
		// Clear list
		m_wndPiecesBar.m_wndPiecesCombo.ResetContent();
		return TRUE;
	}

	// Search if the string is already there
	for (int i = 0; i < m_wndPiecesBar.m_wndPiecesCombo.GetCount();i++)
	{
		char tmp[100];
		m_wndPiecesBar.m_wndPiecesCombo.GetLBText (i, tmp);
		if (strcmp ((char*)lParam, tmp) == 0)
			return TRUE;
	}
	m_wndPiecesBar.m_wndPiecesCombo.AddString ((char*)lParam);

	return TRUE;
}

LRESULT CMainFrame::OnUpdateInfo(WPARAM wParam, LPARAM lParam)
{
	Object* Focus = lcGetActiveProject()->GetFocusObject();

	m_wndProperties.Update(Focus);

	char str[128];
	lcVector3 pos;

	lcGetActiveProject()->GetFocusPosition(pos);
	lcGetActiveProject()->ConvertToUserUnits(pos);

	sprintf (str, "X: %.2f Y: %.2f Z: %.2f", pos[0], pos[1], pos[2]);
	SetStatusBarPane(ID_INDICATOR_POSITION, str);

	return TRUE;
}

// Helper function change the text of a status bar pane and resize it.
void CMainFrame::SetStatusBarPane(UINT ID, const char* Text)
{
	// Set the pane text.
	int Index = m_wndStatusBar.CommandToIndex(ID);
	m_wndStatusBar.SetPaneText(Index, Text);

	// Resize the pane to fit the text.
	UINT nID, nStyle; int cxWidth;
	HFONT hFont = (HFONT)m_wndStatusBar.SendMessage(WM_GETFONT);
	CClientDC dcScreen(NULL);
	HGDIOBJ hOldFont = NULL;

	if (hFont != NULL)
		hOldFont = dcScreen.SelectObject(hFont);

	m_wndStatusBar.GetPaneInfo(Index, nID, nStyle, cxWidth);
	cxWidth = dcScreen.GetTextExtent(Text).cx;
	m_wndStatusBar.SetPaneInfo(Index, nID, nStyle, cxWidth);

	if (hOldFont != NULL)
		dcScreen.SelectObject(hOldFont);
}

LRESULT CMainFrame::OnPopupClose(WPARAM wParam, LPARAM lParam)
{
	m_wndStatusBar.m_pPopup = NULL;
	return TRUE;
}

LRESULT CMainFrame::UpdateSettings(WPARAM wParam, LPARAM lParam)
{
	int i = theApp.GetProfileInt("Settings", "Piecebar Options", 0);
	m_wndPiecesBar.m_bSubParts = (i & PIECEBAR_SUBPARTS) != 0;

	RECT rc;
	m_wndPiecesBar.GetClientRect(&rc);
	m_wndPiecesBar.PostMessage(WM_SIZE, SIZE_RESTORED, MAKELPARAM(rc.right, rc.bottom));
	PostMessage(WM_LC_UPDATE_LIST, 1, 0);

	return TRUE;
}

void CMainFrame::OnClose() 
{
	if (!lcGetActiveProject()->SaveModified())
		return;

	m_wndStandardBar.ResetImages();

	CFrameWndEx::OnClose();
}

void CMainFrame::OnSetFocus(CWnd* pOldWnd) 
{
	CFrameWndEx::OnSetFocus(pOldWnd);
	
	if (m_wndStatusBar.m_pPopup)
		m_wndStatusBar.m_pPopup->DestroyWindow();
}

void CMainFrame::OnPieceBar(UINT nID)
{
	switch (nID)
	{
		case ID_PIECEBAR_SUBPARTS:
		{
			m_wndPiecesBar.m_bSubParts = !m_wndPiecesBar.m_bSubParts;
			m_wndPiecesBar.RefreshPiecesTree();
		} break;
	}

	if (nID != ID_PIECEBAR_SUBPARTS)
	{
		RECT rc;
		m_wndPiecesBar.GetClientRect(&rc);
		m_wndPiecesBar.PostMessage(WM_SIZE, SIZE_RESTORED, MAKELPARAM(rc.right, rc.bottom));
	}

	UINT u = 0;
	if (m_wndPiecesBar.m_bSubParts) u |= PIECEBAR_SUBPARTS;
	theApp.WriteProfileInt("Settings", "Piecebar Options", u);
}

void CMainFrame::OnUpdatePieceBar(CCmdUI* pCmdUI)
{
	switch (pCmdUI->m_nID)
	{
		case ID_PIECEBAR_SUBPARTS:
			pCmdUI->SetCheck(m_wndPiecesBar.m_bSubParts); break;
	}
}

void CMainFrame::OnViewFullscreen() 
{
	EnableFullScreenMode(ID_VIEW_FULLSCREEN);
	ShowFullScreen();
}

void CMainFrame::GetMessageString(UINT nID, CString& rMessage) const
{
	if (nID >= ID_CAMERA_FIRST && nID <= ID_CAMERA_LAST)
	{
		Camera* pCamera = lcGetActiveProject()->mCameras[nID - ID_CAMERA_FIRST];
		rMessage = "Use the camera \"";
		rMessage += pCamera->GetName();
		rMessage += "\"";
	}
	else
		CFrameWndEx::GetMessageString(nID, rMessage);
}

void CMainFrame::OnFilePrintPieceList() 
{
	AfxBeginThread(PrintPiecesFunction, this);
}

BOOL CMainFrame::OnShowPopupMenu(CMFCPopupMenu* pMenuPopup)
{
	if (!CMFCToolBar::IsCustomizeMode() && pMenuPopup != NULL && pMenuPopup->GetHMenu() != NULL)
	{
		HMENU hMenuPop = pMenuPopup->GetHMenu();
		BOOL bIsCamerawMenu = FALSE;

		int iItemMax = ::GetMenuItemCount(hMenuPop);
		for (int iItemPop = 0; !bIsCamerawMenu && iItemPop < iItemMax; iItemPop ++)
		{
			UINT nID = ::GetMenuItemID( hMenuPop, iItemPop);
			bIsCamerawMenu = (nID >= ID_CAMERA_FIRST && nID <= ID_CAMERA_LAST);
		}

		if (bIsCamerawMenu)
		{
			Project* project = lcGetActiveProject();

			pMenuPopup->RemoveAllItems();

			for (int CameraIdx = 0; CameraIdx < project->mCameras.GetSize(); CameraIdx++)
			{
				CMFCToolBarMenuButton newButton(ID_CAMERA_FIRST + CameraIdx, NULL, -1, project->mCameras[CameraIdx]->GetName());
				pMenuPopup->InsertItem(newButton);
			}

			if (project->mCameras.GetSize())
				pMenuPopup->InsertSeparator();

			pMenuPopup->InsertItem(CMFCToolBarMenuButton(ID_VIEW_CAMERAS_RESET, NULL, -1, "Reset"));
		}
	}

	return CFrameWndEx::OnShowPopupMenu(pMenuPopup);
}

// Pass all commands to the project.
BOOL CMainFrame::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	Project* project = lcGetActiveProject();
	int nID = LOWORD(wParam);

	if (nID >= ID_SNAP_0 && nID <= ID_SNAP_9)
	{
		project->HandleCommand((LC_COMMANDS)(LC_EDIT_MOVEXY_SNAP_0 + nID - ID_SNAP_0), 0);
		return TRUE;
	}

	if (nID >= ID_SNAP_10 && nID <= ID_SNAP_19)
	{
		project->HandleCommand((LC_COMMANDS)(LC_EDIT_MOVEZ_SNAP_0 + nID - ID_SNAP_10), 0);
		return TRUE;
	}

	if (nID >= ID_SNAP_20 && nID <= ID_SNAP_29)
	{
		project->HandleCommand((LC_COMMANDS)(LC_EDIT_ANGLE_SNAP_0 + nID - ID_SNAP_20), 0);
		return TRUE;
	}

	if (nID >= ID_CAMERA_FIRST && nID <= ID_CAMERA_LAST)
	{
		project->HandleCommand(LC_VIEW_CAMERA_MENU, nID - ID_CAMERA_FIRST);
		return TRUE;
	}
	
	if (nID >= ID_FILE_MRU_FILE1 && nID <= ID_FILE_MRU_FILE4)
	{
		project->HandleCommand(LC_FILE_RECENT, nID - ID_FILE_MRU_FILE1);
		return TRUE;
	}

	if (nID >= ID_ACTION_SELECT && nID <= ID_ACTION_ROLL)
	{
		project->SetAction(nID - ID_ACTION_SELECT);
		return TRUE;
	}

	if (nID >= ID_TRANSFORM_ABSOLUTE_TRANSLATION && nID <= ID_TRANSFORM_RELATIVE_ROTATION)
	{
		mTransformMode = nID - ID_TRANSFORM_ABSOLUTE_TRANSLATION;
		/*
		CMenu PopupMenus;
		PopupMenus.LoadMenu(IDR_POPUPS);
		CMenu* Popup;
		Popup = PopupMenus.GetSubMenu(9);

		CMFCToolBarMenuButton Transform(ID_TRANSFORM_TYPE, Popup->GetSafeHmenu(), GetCmdMgr()->GetCmdImage(ID_BUTTON_ABSOLUTE_TRANSLATION + mTransformMode));
		m_wndStandardBar.ReplaceButton(ID_TRANSFORM_TYPE, Transform);
		m_wndStandardBar.AdjustLayout();
		*/

		CObList listButtons;
		if (CMFCToolBar::GetCommandButtons(ID_TRANSFORM_TYPE, listButtons) > 0)
		{
			for (POSITION pos = listButtons.GetHeadPosition(); pos != NULL;)
			{
				CMFCToolBarButton* Button = DYNAMIC_DOWNCAST(CMFCToolBarButton, listButtons.GetNext(pos));
				Button->SetImage(GetCmdMgr()->GetCmdImage(ID_BUTTON_ABSOLUTE_TRANSLATION + mTransformMode));
			}
		}

		m_wndStandardBar.AdjustLayout();

		return TRUE;
	}

	switch (nID)
	{
		case ID_FILE_NEW: {
			project->HandleCommand(LC_FILE_NEW, 0);
		} break;

		case ID_FILE_OPEN: {
			project->HandleCommand(LC_FILE_OPEN, 0);
		} break;

		case ID_FILE_MERGE: {
			project->HandleCommand(LC_FILE_MERGE, 0);
		} break;

		case ID_FILE_SAVE: {
			project->HandleCommand(LC_FILE_SAVE, 0);
		} break;

		case ID_FILE_SAVE_AS: {
			project->HandleCommand(LC_FILE_SAVEAS, 0);
		} break;

		case ID_FILE_SAVEPICTURE: {
			project->HandleCommand(LC_FILE_PICTURE, 0);
		} break;

		case ID_FILE_EXPORT_3DSTUDIO: {
			project->HandleCommand(LC_FILE_3DS, 0);
		} break;

		case ID_FILE_EXPORT_HTML: {
			project->HandleCommand(LC_FILE_HTML, 0);
		} break;

		case ID_FILE_EXPORT_POVRAY: {
			project->HandleCommand(LC_FILE_POVRAY, 0);
		} break;

		case ID_FILE_EXPORT_WAVEFRONT: {
			project->HandleCommand(LC_FILE_WAVEFRONT, 0);
		} break;

		case ID_FILE_PROPERTIES: {
			project->HandleCommand(LC_FILE_PROPERTIES, 0);
		} break;

		case ID_FILE_TERRAINEDITOR: {
			project->HandleCommand(LC_FILE_TERRAIN, 0);
		} break;

		case ID_FILE_EDITPIECELIBRARY: {
			project->HandleCommand(LC_FILE_LIBRARY, 0);
		} break;

		case ID_EDIT_REDO: {
			project->HandleCommand(LC_EDIT_REDO, 0);
		} break;

		case ID_EDIT_UNDO: {
			project->HandleCommand(LC_EDIT_UNDO, 0);
		} break;

		case ID_EDIT_CUT: {
			project->HandleCommand(LC_EDIT_CUT, 0);
		} break;
		
		case ID_EDIT_COPY: {
			project->HandleCommand(LC_EDIT_COPY, 0);
		} break;

		case ID_EDIT_PASTE: {
			project->HandleCommand(LC_EDIT_PASTE, 0);
		} break;

		case ID_EDIT_SELECTALL: {
			project->HandleCommand(LC_EDIT_SELECT_ALL, 0);
		} break;

		case ID_EDIT_SELECTNONE: {
			project->HandleCommand(LC_EDIT_SELECT_NONE, 0);
		} break;
		
		case ID_EDIT_SELECTINVERT: {
			project->HandleCommand(LC_EDIT_SELECT_INVERT, 0);
		} break;
		
		case ID_EDIT_SELECTBYNAME: {
			project->HandleCommand(LC_EDIT_SELECT_BYNAME, 0);
		} break;

		case ID_PIECE_INSERT: {
			project->HandleCommand(LC_PIECE_INSERT, 0);
		} break;

		case ID_PIECE_DELETE: {
			project->HandleCommand(LC_PIECE_DELETE, 0);
		} break;

		case ID_PIECE_MINIFIGWIZARD: {
			project->HandleCommand(LC_PIECE_MINIFIG, 0);
		} break;

		case ID_PIECE_ARRAY: {
			project->HandleCommand(LC_PIECE_ARRAY, 0);
		} break;

		case ID_PIECE_COPYKEYS: {
			project->HandleCommand(LC_PIECE_COPYKEYS, 0);
		} break;

		case ID_PIECE_GROUP: {
			project->HandleCommand(LC_PIECE_GROUP, 0);
		} break;
		
		case ID_PIECE_UNGROUP: {
			project->HandleCommand(LC_PIECE_UNGROUP, 0);
		} break;
		
		case ID_PIECE_ATTACH: {
			project->HandleCommand(LC_PIECE_GROUP_ADD, 0);
		} break;

		case ID_PIECE_DETACH: {
			project->HandleCommand(LC_PIECE_GROUP_REMOVE, 0);
		} break;

		case ID_PIECE_EDITGROUPS: {
			project->HandleCommand(LC_PIECE_GROUP_EDIT, 0);
		} break;

		case ID_PIECE_HIDESELECTED: {
			project->HandleCommand(LC_PIECE_HIDE_SELECTED, 0);
		} break;

		case ID_PIECE_HIDEUNSELECTED: {
			project->HandleCommand(LC_PIECE_HIDE_UNSELECTED, 0);
		} break;
		
		case ID_PIECE_UNHIDEALL: {
			project->HandleCommand(LC_PIECE_UNHIDE_ALL, 0);
		} break;

		case ID_PIECE_PREVIOUS: {
			project->HandleCommand(LC_PIECE_PREVIOUS, 0);
		} break;

		case ID_PIECE_NEXT: {
			project->HandleCommand(LC_PIECE_NEXT, 0);
		} break;

		case ID_VIEW_PREFERENCES: {
			project->HandleCommand(LC_VIEW_PREFERENCES, 0);
		} break;

		case ID_VIEW_ZOOMIN: {
			project->HandleCommand(LC_VIEW_ZOOMIN, 0);
		} break;

		case ID_VIEW_ZOOMOUT: {
			project->HandleCommand(LC_VIEW_ZOOMOUT, 0);
		} break;

		case ID_ZOOM_EXTENTS: {
			project->HandleCommand(LC_VIEW_ZOOMEXTENTS, 0);
		} break;

		case ID_VIEW_STEP_NEXT: {
			project->HandleCommand(LC_VIEW_STEP_NEXT, 0);
		} break;

		case ID_VIEW_STEP_PREVIOUS: {
			project->HandleCommand(LC_VIEW_STEP_PREVIOUS, 0);
		} break;

		case ID_VIEW_STEP_FIRST: {
			project->HandleCommand(LC_VIEW_STEP_FIRST, 0);
		} break;

		case ID_VIEW_STEP_LAST: {
			project->HandleCommand(LC_VIEW_STEP_LAST, 0);
		} break;

		case ID_VIEW_STEP_CHOOSE: {
			project->HandleCommand(LC_VIEW_STEP_CHOOSE, 0);
		} break;

		case ID_VIEW_STEP_INSERT: {
			project->HandleCommand(LC_VIEW_STEP_INSERT, 0);
		} break;

		case ID_VIEW_STEP_DELETE: {
			project->HandleCommand(LC_VIEW_STEP_DELETE, 0);
		} break;

		case ID_ANIMATOR_STOP: {
			project->HandleCommand(LC_VIEW_STOP, 0);
		} break;

		case ID_ANIMATOR_PLAY: {
			project->HandleCommand(LC_VIEW_PLAY, 0);
		} break;

		case ID_VIEW_VIEWPOINT_FRONT: {
			project->HandleCommand(LC_VIEW_VIEWPOINT_FRONT, 0);
		} break;

		case ID_VIEW_VIEWPOINT_BACK: {
			project->HandleCommand(LC_VIEW_VIEWPOINT_BACK, 0);
		} break;

		case ID_VIEW_VIEWPOINT_TOP: {
			project->HandleCommand(LC_VIEW_VIEWPOINT_TOP, 0);
		} break;

		case ID_VIEW_VIEWPOINT_BOTTOM: {
			project->HandleCommand(LC_VIEW_VIEWPOINT_BOTTOM, 0);
		} break;

		case ID_VIEW_VIEWPOINT_LEFT: {
			project->HandleCommand(LC_VIEW_VIEWPOINT_LEFT, 0);
		} break;

		case ID_VIEW_VIEWPOINT_RIGHT: {
			project->HandleCommand(LC_VIEW_VIEWPOINT_RIGHT, 0);
		} break;

		case ID_VIEW_VIEWPOINT_HOME: {
			project->HandleCommand(LC_VIEW_VIEWPOINT_HOME, 0);
		} break;

		case ID_VIEW_CAMERAS_RESET: {
			project->HandleCommand(LC_VIEW_CAMERA_RESET, 0);
		} break;

		case ID_ANIMATOR_KEY: {
			project->HandleCommand(LC_TOOLBAR_ADDKEYS, 0);
		} break;

		case ID_ANIMATOR_TOGGLE: {
			project->HandleCommand(LC_TOOLBAR_ANIMATION, 0);
		} break;

		case ID_RENDER_BOX: {
			project->HandleCommand(LC_TOOLBAR_FASTRENDER, 0);
		} break;

		case ID_SNAP_SNAPX: {
			project->HandleCommand(LC_TOOLBAR_SNAPMENU, 0);
		} break;

		case ID_SNAP_SNAPY: {
			project->HandleCommand(LC_TOOLBAR_SNAPMENU, 1);
		} break;

		case ID_SNAP_SNAPZ: {
			project->HandleCommand(LC_TOOLBAR_SNAPMENU, 2);
		} break;

		case ID_SNAP_ON: {
			project->HandleCommand(LC_TOOLBAR_SNAPMENU, 6);
		} break;

		case ID_SNAP_SNAPALL: {
			project->HandleCommand(LC_TOOLBAR_SNAPMENU, 3);
		} break;

		case ID_SNAP_SNAPNONE: {
			project->HandleCommand(LC_TOOLBAR_SNAPMENU, 4);
		} break;

		case ID_LOCK_LOCKX: {
			project->HandleCommand(LC_TOOLBAR_LOCKMENU, 0);
		} break;

		case ID_LOCK_LOCKY: {
			project->HandleCommand(LC_TOOLBAR_LOCKMENU, 1);
		} break;

		case ID_LOCK_LOCKZ: {
			project->HandleCommand(LC_TOOLBAR_LOCKMENU, 2);
		} break;

		case ID_LOCK_ON: {
			project->HandleCommand(LC_TOOLBAR_LOCKMENU, 4);
		} break;

		case ID_LOCK_UNLOCKALL: {
			project->HandleCommand(LC_TOOLBAR_LOCKMENU, 3);
		} break;

		case ID_LOCK_2BUTTONS: {
			project->HandleCommand(LC_TOOLBAR_LOCKMENU, 4);
		} break;

		case ID_LOCK_3DMOVEMENT: {
			project->HandleCommand(LC_TOOLBAR_LOCKMENU, 5);
		} break;

		case ID_SNAP_ANGLE: {
			project->HandleCommand(LC_TOOLBAR_SNAPMENU, 5);
		} break;

		case ID_APP_ABOUT: {
			project->HandleCommand(LC_HELP_ABOUT, 0);
		} break;

		case ID_PIECEBAR_EDITCATEGORY:
		{
			HTREEITEM Item = m_wndPiecesBar.m_PiecesTree.GetSelectedItem();

			if (Item == NULL)
				break;

			lcPiecesLibrary* Lib = lcGetPiecesLibrary();
			CString CategoryName = m_wndPiecesBar.m_PiecesTree.GetItemText(Item);
			int Index = Lib->FindCategoryIndex((const char*)CategoryName);

			if (Index == -1)
				break;

			LC_CATEGORYDLG_OPTS Opts;
			Opts.Name = Lib->mCategories[Index].Name;
			Opts.Keywords = Lib->mCategories[Index].Keywords;

			if (SystemDoDialog(LC_DLG_EDITCATEGORY, &Opts))
			{
				String OldName = Lib->mCategories[Index].Name;
				Lib->SetCategory(Index, Opts.Name, Opts.Keywords);
				m_wndPiecesBar.UpdatePiecesTree(OldName, Opts.Name);
			}

		} break;

		case ID_PIECEBAR_NEWCATEGORY:
		{
			LC_CATEGORYDLG_OPTS Opts;
			Opts.Name = "New Category";
			Opts.Keywords = "";

			if (SystemDoDialog(LC_DLG_EDITCATEGORY, &Opts))
			{
				lcPiecesLibrary* Lib = lcGetPiecesLibrary();
				Lib->AddCategory(Opts.Name, Opts.Keywords);
				m_wndPiecesBar.UpdatePiecesTree(NULL, Opts.Name);
			}

		} break;

		case ID_PIECEBAR_REMOVECATEGORY:
		{
			HTREEITEM Item = m_wndPiecesBar.m_PiecesTree.GetSelectedItem();

			if (Item == NULL)
				break;

			lcPiecesLibrary* Lib = lcGetPiecesLibrary();
			CString CategoryName = m_wndPiecesBar.m_PiecesTree.GetItemText(Item);
			int Index = Lib->FindCategoryIndex((const char*)CategoryName);

			if (Index == -1)
				break;

			char Msg[1024];
			const String& Name = Lib->mCategories[Index].Name;
			sprintf(Msg, "Are you sure you want to remove the '%s' category?", Name);

			if (SystemDoMessageBox(Msg, LC_MB_YESNO | LC_MB_ICONQUESTION) == LC_YES)
			{
				Lib->RemoveCategory(Index);
				m_wndPiecesBar.UpdatePiecesTree(Name, NULL);
			}

		} break;

		default:
			return CFrameWndEx::OnCommand(wParam, lParam);
	}

	return TRUE;
}

void CMainFrame::OnActivateApp(BOOL bActive, DWORD hTask) 
{
	CFrameWndEx::OnActivateApp(bActive, hTask);
	
	// Don't notify if we loose focus while on print preview
	if (m_lpfnCloseProc == NULL)
	{
		Project* project = lcGetActiveProject();

		if (project)
			project->HandleNotify(LC_ACTIVATE, bActive ? 1 : 0);
	}
}

LRESULT CMainFrame::OnSetMessageString(WPARAM wParam, LPARAM lParam)
{
	UINT nIDLast = m_nIDLastMessage;
	m_nFlags &= ~WF_NOPOPMSG;

	CWnd* pMessageBar = GetMessageBar();
	if (pMessageBar != NULL)
	{
		LPCTSTR lpsz = NULL;
		CString strMessage;

		// set the message bar text
		if (lParam != 0)
		{
			ASSERT(wParam == 0);    // can't have both an ID and a string
			lpsz = (LPCTSTR)lParam; // set an explicit string
		}
		else if ((wParam != 0) && !(wParam == AFX_IDS_IDLEMESSAGE && !m_strStatusBar.IsEmpty()))
		{
			// map SC_CLOSE to PREVIEW_CLOSE when in print preview mode
			if (wParam == AFX_IDS_SCCLOSE && m_lpfnCloseProc != NULL)
				wParam = AFX_IDS_PREVIEW_CLOSE;

			// get message associated with the ID indicated by wParam
			GetMessageString(wParam, strMessage);
			lpsz = strMessage;
		}
		else if (!m_strStatusBar.IsEmpty())
		{
			lpsz = m_strStatusBar;
		}

		pMessageBar->SetWindowText(lpsz);

		// update owner of the bar in terms of last message selected
		m_nIDLastMessage = (UINT)wParam;
		m_nIDTracking = (UINT)wParam;
	}

	m_nIDLastMessage = (UINT)wParam;    // new ID (or 0)
	m_nIDTracking = (UINT)wParam;       // so F1 on toolbar buttons work
	return nIDLast;
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg) 
{
	if (pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST)
	{
		CWnd* Focus = GetFocus();

		if (Focus && m_wndPiecesBar.m_wndPiecesCombo.IsChild(Focus))
			return FALSE;
	}

	if (pMsg->message == WM_KEYDOWN)
	{
		if ((HIWORD(pMsg->lParam) & KF_REPEAT) == 0)
		{
			bool Control = GetKeyState(VK_CONTROL) < 0;
			bool Shift = GetKeyState(VK_SHIFT) < 0;

			for (int i = 0; i < KeyboardShortcutsCount; i++)
			{
				LC_KEYBOARD_COMMAND& Cmd = KeyboardShortcuts[i];

				if (pMsg->wParam == Cmd.Key1)
				{
					if ((Shift == ((Cmd.Flags & LC_KEYMOD1_SHIFT) != 0)) &&
					    (Control == ((Cmd.Flags & LC_KEYMOD1_CONTROL) != 0)))
					{
						if (Cmd.Flags & LC_KEYMOD_VIEWONLY)
						{
							if (GetFocus() != GetActiveView())
							{
								break;
							}
						}

						lcGetActiveProject()->HandleCommand(Cmd.ID, 0);
						return TRUE;
					}
				}

				if (pMsg->wParam == Cmd.Key2)
				{
					if ((Shift == ((Cmd.Flags & LC_KEYMOD2_SHIFT) != 0)) &&
					    (Control == ((Cmd.Flags & LC_KEYMOD2_CONTROL) != 0)))
					{
						if (Cmd.Flags & LC_KEYMOD_VIEWONLY)
						{
							if (GetFocus() != GetActiveView())
							{
								break;
							}
						}

						lcGetActiveProject()->HandleCommand(Cmd.ID, 0);
						return TRUE;
					}
				}
			}
		}
	}

	return CFrameWndEx::PreTranslateMessage(pMsg);
}

void CMainFrame::UpdateMenuAccelerators()
{
	WORD CmdToID[] =
	{
		ID_FILE_NEW,               // LC_FILE_NEW
		ID_FILE_OPEN,              // LC_FILE_OPEN
		ID_FILE_MERGE,             // LC_FILE_MERGE
		ID_FILE_SAVE,              // LC_FILE_SAVE
		ID_FILE_SAVE_AS,           // LC_FILE_SAVEAS
		ID_FILE_SAVEPICTURE,       // LC_FILE_PICTURE
		ID_FILE_EXPORT_3DSTUDIO,   // LC_FILE_3DS
		ID_FILE_EXPORT_HTML,       // LC_FILE_HTML
		ID_FILE_EXPORT_POVRAY,     // LC_FILE_POVRAY
		ID_FILE_EXPORT_WAVEFRONT,  // LC_FILE_WAVEFRONT
		ID_FILE_PROPERTIES,        // LC_FILE_PROPERTIES
		ID_FILE_TERRAINEDITOR,     // LC_FILE_TERRAIN
		ID_FILE_EDITPIECELIBRARY,  // LC_FILE_LIBRARY
		0,                         // LC_FILE_RECENT
		ID_EDIT_UNDO,              // LC_EDIT_UNDO
		ID_EDIT_REDO,              // LC_EDIT_REDO
		ID_EDIT_CUT,               // LC_EDIT_CUT
		ID_EDIT_COPY,              // LC_EDIT_COPY
		ID_EDIT_PASTE,             // LC_EDIT_PASTE
		ID_EDIT_SELECTALL,         // LC_EDIT_SELECT_ALL
		ID_EDIT_SELECTNONE,        // LC_EDIT_SELECT_NONE
		ID_EDIT_SELECTINVERT,      // LC_EDIT_SELECT_INVERT
		ID_EDIT_SELECTBYNAME,      // LC_EDIT_SELECT_BYNAME
		ID_PIECE_INSERT,           // LC_PIECE_INSERT
		ID_PIECE_DELETE,           // LC_PIECE_DELETE
		ID_PIECE_MINIFIGWIZARD,    // LC_PIECE_MINIFIG
		ID_PIECE_ARRAY,            // LC_PIECE_ARRAY
		ID_PIECE_COPYKEYS,         // LC_PIECE_COPYKEYS
		ID_PIECE_GROUP,            // LC_PIECE_GROUP
		ID_PIECE_UNGROUP,          // LC_PIECE_UNGROUP
		ID_PIECE_ATTACH,           // LC_PIECE_GROUP_ADD
		ID_PIECE_DETACH,           // LC_PIECE_GROUP_REMOVE
		ID_PIECE_EDITGROUPS,       // LC_PIECE_GROUP_EDIT
		ID_PIECE_HIDESELECTED,     // LC_PIECE_HIDE_SELECTED
		ID_PIECE_HIDEUNSELECTED,   // LC_PIECE_HIDE_UNSELECTED
		ID_PIECE_UNHIDEALL,        // LC_PIECE_UNHIDE_ALL
		ID_PIECE_PREVIOUS,         // LC_PIECE_PREVIOUS
		ID_PIECE_NEXT,             // LC_PIECE_NEXT
		ID_VIEW_PREFERENCES,       // LC_VIEW_PREFERENCES
		0,                         // LC_VIEW_ZOOM
		ID_VIEW_ZOOMIN,            // LC_VIEW_ZOOMIN
		ID_VIEW_ZOOMOUT,           // LC_VIEW_ZOOMOUT
		ID_ZOOM_EXTENTS,           // LC_VIEW_ZOOMEXTENTS
		ID_VIEW_STEP_NEXT,         // LC_VIEW_STEP_NEXT
		ID_VIEW_STEP_PREVIOUS,     // LC_VIEW_STEP_PREVIOUS
		ID_VIEW_STEP_FIRST,        // LC_VIEW_STEP_FIRST
		ID_VIEW_STEP_LAST,         // LC_VIEW_STEP_LAST
		ID_VIEW_STEP_CHOOSE,       // LC_VIEW_STEP_CHOOSE
		0,                         // LC_VIEW_STEP_SET
		ID_VIEW_STEP_INSERT,       // LC_VIEW_STEP_INSERT
		ID_VIEW_STEP_DELETE,       // LC_VIEW_STEP_DELETE
		ID_ANIMATOR_STOP,          // LC_VIEW_STOP
		ID_ANIMATOR_PLAY,          // LC_VIEW_PLAY
		ID_VIEW_VIEWPOINT_FRONT,   // LC_VIEW_VIEWPOINT_FRONT
		ID_VIEW_VIEWPOINT_BACK,    // LC_VIEW_VIEWPOINT_BACK
		ID_VIEW_VIEWPOINT_TOP,     // LC_VIEW_VIEWPOINT_TOP
		ID_VIEW_VIEWPOINT_BOTTOM,  // LC_VIEW_VIEWPOINT_BOTTOM
		ID_VIEW_VIEWPOINT_LEFT,    // LC_VIEW_VIEWPOINT_LEFT
		ID_VIEW_VIEWPOINT_RIGHT,   // LC_VIEW_VIEWPOINT_RIGHT
		ID_VIEW_VIEWPOINT_HOME,    // LC_VIEW_VIEWPOINT_HOME
		0,                         // LC_VIEW_CAMERA_MENU
		ID_VIEW_CAMERAS_RESET,     // LC_VIEW_CAMERA_RESET
		ID_APP_ABOUT,              // LC_HELP_ABOUT
		0,                         // LC_TOOLBAR_ANIMATION
		0,                         // LC_TOOLBAR_ADDKEYS
		0,                         // LC_TOOLBAR_SNAPMENU
		0,                         // LC_TOOLBAR_LOCKMENU
		0,                         // LC_TOOLBAR_FASTRENDER
		0,                         // LC_EDIT_MOVEXY_SNAP_0
		0,                         // LC_EDIT_MOVEXY_SNAP_1
		0,                         // LC_EDIT_MOVEXY_SNAP_2
		0,                         // LC_EDIT_MOVEXY_SNAP_3
		0,                         // LC_EDIT_MOVEXY_SNAP_4
		0,                         // LC_EDIT_MOVEXY_SNAP_5
		0,                         // LC_EDIT_MOVEXY_SNAP_6
		0,                         // LC_EDIT_MOVEXY_SNAP_7
		0,                         // LC_EDIT_MOVEXY_SNAP_8
		0,                         // LC_EDIT_MOVEXY_SNAP_9
		0,                         // LC_EDIT_MOVEZ_SNAP_0
		0,                         // LC_EDIT_MOVEZ_SNAP_1
		0,                         // LC_EDIT_MOVEZ_SNAP_2
		0,                         // LC_EDIT_MOVEZ_SNAP_3
		0,                         // LC_EDIT_MOVEZ_SNAP_4
		0,                         // LC_EDIT_MOVEZ_SNAP_5
		0,                         // LC_EDIT_MOVEZ_SNAP_6
		0,                         // LC_EDIT_MOVEZ_SNAP_7
		0,                         // LC_EDIT_MOVEZ_SNAP_8
		0,                         // LC_EDIT_MOVEZ_SNAP_9
		0,                         // LC_EDIT_ANGLE_SNAP_0
		0,                         // LC_EDIT_ANGLE_SNAP_1
		0,                         // LC_EDIT_ANGLE_SNAP_2
		0,                         // LC_EDIT_ANGLE_SNAP_3
		0,                         // LC_EDIT_ANGLE_SNAP_4
		0,                         // LC_EDIT_ANGLE_SNAP_5
		0,                         // LC_EDIT_ANGLE_SNAP_6
		0,                         // LC_EDIT_ANGLE_SNAP_7
		0,                         // LC_EDIT_ANGLE_SNAP_8
		0,                         // LC_EDIT_ANGLE_SNAP_9
		0,                         // LC_EDIT_ACTION_SELECT
		0,                         // LC_EDIT_ACTION_INSERT
		0,                         // LC_EDIT_ACTION_LIGHT
		0,                         // LC_EDIT_ACTION_SPOTLIGHT
		0,                         // LC_EDIT_ACTION_CAMERA
		0,                         // LC_EDIT_ACTION_MOVE
		0,                         // LC_EDIT_ACTION_ROTATE
		0,                         // LC_EDIT_ACTION_ERASER
		0,                         // LC_EDIT_ACTION_PAINT
		0,                         // LC_EDIT_ACTION_ZOOM
		0,                         // LC_EDIT_ACTION_ZOOM_REGION
		0,                         // LC_EDIT_ACTION_PAN
		0,                         // LC_EDIT_ACTION_ROTATE_VIEW
		0,                         // LC_EDIT_ACTION_ROLL
	};

	LPACCEL AccelTable = new ACCEL[2 * KeyboardShortcutsCount];
	int AccelCount = 0;

	for (int i = 0; i < KeyboardShortcutsCount; i++)
	{
		LC_KEYBOARD_COMMAND& Cmd = KeyboardShortcuts[i];
		WORD ID = CmdToID[Cmd.ID];

		if (ID == 0)
			continue;

		if (Cmd.Key1)
		{
			ACCEL& Accel = AccelTable[AccelCount++];

			Accel.cmd = ID;
			Accel.key = Cmd.Key1;
			Accel.fVirt = FVIRTKEY;

			if (Cmd.Flags & LC_KEYMOD1_SHIFT)
				Accel.fVirt |= FSHIFT;

			if (Cmd.Flags & LC_KEYMOD1_CONTROL)
				Accel.fVirt |= FCONTROL;

			if (Cmd.Key2)
			{
				ACCEL& Accel = AccelTable[AccelCount++];

				Accel.cmd = ID;
				Accel.key = Cmd.Key2;
				Accel.fVirt = FVIRTKEY;

				if (Cmd.Flags & LC_KEYMOD2_SHIFT)
					Accel.fVirt |= FSHIFT;

				if (Cmd.Flags & LC_KEYMOD2_CONTROL)
					Accel.fVirt |= FCONTROL;
			}
		}
	}

	CKeyboardManager* KeyboardManager = theApp.GetKeyboardManager();
	KeyboardManager->UpdateAccelTable(NULL, AccelTable, AccelCount, this);

	delete[] AccelTable;
}

void CMainFrame::OnDropFiles(HDROP hDropInfo) 
{
	SetActiveWindow();

	UINT nFiles = ::DragQueryFile(hDropInfo, (UINT)-1, NULL, 0);

	if (nFiles > 0)
	{
		TCHAR szFileName[_MAX_PATH];
		::DragQueryFile(hDropInfo, 0, szFileName, _MAX_PATH);

		lcGetActiveProject()->OpenProject(szFileName);
	}

	::DragFinish(hDropInfo);
}

void CMainFrame::OnViewSplitVertically()
{
	CView* ActiveView = GetActiveView();

	if (!ActiveView->IsKindOf(RUNTIME_CLASS(CCADView)))
		return;

	RECT rect;
	ActiveView->GetClientRect(&rect);
	int Width = (rect.right - rect.left) / 2;
	int Height = rect.bottom - rect.top;

	CDynamicSplitterWnd* ParentSplitter;
	CDynamicSplitterWnd* NewSplitter = new CDynamicSplitterWnd();
	m_SplitterList.Add(NewSplitter);

	CWnd* Parent = ActiveView->GetParent();

	if (Parent == this)
	{
		ActiveView->SetParent(NULL);

		ParentSplitter = NewSplitter;
		ParentSplitter->CreateStatic(this, 1, 2, WS_CHILD | WS_VISIBLE, AFX_IDW_PANE_FIRST);
	}
	else
	{
		ParentSplitter = (CDynamicSplitterWnd*)Parent;
		int Row, Col;

		ParentSplitter->GetViewRowCol(ActiveView, &Row, &Col);
		ParentSplitter->DetachWindow(Row, Col);

		NewSplitter->CreateStatic(ParentSplitter, 1, 2, WS_CHILD | WS_VISIBLE, ParentSplitter->IdFromRowCol(Row, Col));
		ParentSplitter->AttachWindow(NewSplitter, Row, Col);
	}

	NewSplitter->AttachWindow(ActiveView, 0, 0);
	NewSplitter->SetColumnInfo(0, Width, 0);
	NewSplitter->CreateView(0, 1, RUNTIME_CLASS(CCADView), CSize(Width, Height), NULL);

	CWnd* NewView = NewSplitter->GetPane(0, 1);
	NewView->ModifyStyleEx(0, WS_EX_CLIENTEDGE);

	RecalcLayout();
	ParentSplitter->RecalcLayout();
	ParentSplitter->Invalidate();
}

void CMainFrame::OnViewSplitHorizontally()
{
	CView* ActiveView = GetActiveView();

	if (!ActiveView->IsKindOf(RUNTIME_CLASS(CCADView)))
		return;

	RECT rect;
	ActiveView->GetClientRect(&rect);
	int Width = rect.right - rect.left;
	int Height = (rect.bottom - rect.top) / 2;

	CDynamicSplitterWnd* ParentSplitter;
	CDynamicSplitterWnd* NewSplitter = new CDynamicSplitterWnd();
	m_SplitterList.Add(NewSplitter);

	CWnd* Parent = ActiveView->GetParent();

	if (Parent == this)
	{
		ActiveView->SetParent(NULL);

		ParentSplitter = NewSplitter;
		ParentSplitter->CreateStatic(this, 2, 1, WS_CHILD | WS_VISIBLE, AFX_IDW_PANE_FIRST);
	}
	else
	{
		ParentSplitter = (CDynamicSplitterWnd*)Parent;
		int Row, Col;

		ParentSplitter->GetViewRowCol(ActiveView, &Row, &Col);
		ParentSplitter->DetachWindow(Row, Col);

		NewSplitter->CreateStatic(ParentSplitter, 2, 1, WS_CHILD | WS_VISIBLE, ParentSplitter->IdFromRowCol(Row, Col));
		ParentSplitter->AttachWindow(NewSplitter, Row, Col);
	}

	NewSplitter->AttachWindow(ActiveView, 0, 0);
	NewSplitter->SetRowInfo(0, Height, 0);
	NewSplitter->CreateView(1, 0, RUNTIME_CLASS(CCADView), CSize(Width, Height), NULL);

	CWnd* NewView = NewSplitter->GetPane(1, 0);
	NewView->ModifyStyleEx(0, WS_EX_CLIENTEDGE);

	RecalcLayout();
	ParentSplitter->RecalcLayout();
	ParentSplitter->Invalidate();
}

void CMainFrame::OnViewDeleteView()
{
	CView* ActiveView = GetActiveView();

	if (!ActiveView->IsKindOf(RUNTIME_CLASS(CCADView)))
		return;

	CWnd* Parent = ActiveView->GetParent();

	if (Parent == this)
		return;

	CDynamicSplitterWnd* ParentSplitter = (CDynamicSplitterWnd*)Parent;
	int Row, Col;

	ParentSplitter->GetViewRowCol(ActiveView, &Row, &Col);

	if (ParentSplitter->GetRowCount() == 2)
	{
		ASSERT(ParentSplitter->GetColumnCount() == 1);

		if (Row == 0)
			Row = 1;
		else
			Row = 0;
	}
	else
	{
		ASSERT(ParentSplitter->GetColumnCount() == 2);
		ASSERT(ParentSplitter->GetRowCount() == 1);

		if (Col == 0)
			Col = 1;
		else
			Col = 0;
	}

	// Detach sibling.
	CWnd* Sibling = ParentSplitter->GetPane(Row, Col);
	ParentSplitter->DetachWindow(Row, Col);

	CWnd* ParentParent = Parent->GetParent();
	CDynamicSplitterWnd* ParentParentSplitter = NULL;

	if (ParentParent == this)
	{
		Sibling->SetDlgCtrlID(AFX_IDW_PANE_FIRST);
		Sibling->SetParent(ParentParent);
		Sibling->ShowWindow(SW_SHOW);
		Sibling->InvalidateRect(NULL);
	}
	else
	{
		ParentParentSplitter = (CDynamicSplitterWnd*)ParentParent;

		ParentParentSplitter->GetViewRowCol(ParentSplitter, &Row, &Col);

		ParentParentSplitter->DetachWindow(Row, Col);
		ParentParentSplitter->AttachWindow(Sibling, Row, Col);
	}

	while (!Sibling->IsKindOf(RUNTIME_CLASS(CCADView)))
		Sibling = ((CDynamicSplitterWnd*)Sibling)->GetPane(0, 0);
	SetActiveView((CView*)Sibling);

	Parent->DestroyWindow();

	for (int i = 0; i < m_SplitterList.GetSize(); i++)
	{
		if (m_SplitterList[i] == ParentSplitter)
		{
			delete m_SplitterList[i];
			m_SplitterList.RemoveAt(i);
			break;
		}
	}

	RecalcLayout();
	if (ParentParentSplitter)
		ParentParentSplitter->RecalcLayout();
}

void CMainFrame::OnViewResetViews()
{
	CView* ActiveView = GetActiveView();

	if (!ActiveView->IsKindOf(RUNTIME_CLASS(CCADView)))
		return;

	CWnd* Parent = ActiveView->GetParent();

	if (Parent == this)
		return;

	CDynamicSplitterWnd* ParentSplitter = (CDynamicSplitterWnd*)Parent;
	int Row, Col;

	ParentSplitter->GetViewRowCol(ActiveView, &Row, &Col);
	ParentSplitter->DetachWindow(Row, Col);

	CWnd* TopSplitter = ParentSplitter;
	for (CWnd* NextParent = TopSplitter; NextParent != this; NextParent = NextParent->GetParent())
		TopSplitter = NextParent;

	ParentSplitter->DestroyWindow();

	for (int i = 0; i < m_SplitterList.GetSize(); i++)
	{
		if (m_SplitterList[i] == ParentSplitter)
		{
			delete m_SplitterList[i];
			m_SplitterList.RemoveAt(i);
			break;
		}
	}

	ActiveView->SetDlgCtrlID(AFX_IDW_PANE_FIRST);
	ActiveView->SetParent(this);
	ActiveView->ShowWindow(SW_SHOW);
	ActiveView->InvalidateRect(NULL);

	RecalcLayout();
}
