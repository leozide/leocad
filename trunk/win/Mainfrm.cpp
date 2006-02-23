// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include <afxrich.h>
#include <afxpriv.h>
#include "LeoCAD.h"
#include "MainFrm.h"
#include "Camera.h"
#include "project.h"
#include "message.h"
#include "globals.h"
#include "mainwnd.h"
#include "cadview.h"
#include "console.h"
#include "keyboard.h"
#include "system.h"
#include "library.h"
#include "lc_application.h"
#include "Print.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TOOLBAR_VERSION 1

void mainframe_listener (int message, void *data, void *user)
{
  if (message == LC_MSG_FOCUS_CHANGED)
  {
    CWnd* pFrame = AfxGetMainWnd();
    if (pFrame != NULL)
      pFrame->PostMessage(WM_LC_UPDATE_INFO, (WPARAM)data, 0);
  }
}

static void mainframe_console_func (LC_CONSOLE_LEVEL level, const char* text, void* user_data)
{
	CRichEditCtrl& ctrl = ((CRichEditView *) user_data)->GetRichEditCtrl ();
	CHARFORMAT cf;

	cf.cbSize = sizeof (cf);
	cf.dwMask = CFM_COLOR;
	cf.dwEffects = 0;

	switch (level)
	{
	case LC_CONSOLE_ERROR:
		cf.crTextColor = RGB (255, 0, 0);
		break;

	case LC_CONSOLE_WARNING:
		cf.crTextColor = RGB (0, 0, 255);
		break;

	case LC_CONSOLE_DEBUG:
		cf.crTextColor = RGB (0, 128, 0);
		break;

	case LC_CONSOLE_MISC:
	default:
		cf.crTextColor = RGB (0, 0, 0);
		break;
	}

	ctrl.SetRedraw(FALSE);

	// Go to the end of the window text.
	int TextStart = ctrl.GetWindowTextLength();
	ctrl.SetSel(TextStart, -1);

	// Change color.
	ctrl.SetSelectionCharFormat(cf);

	// Append new text.
	ctrl.ReplaceSel(text);

	ctrl.SetRedraw(TRUE);
	ctrl.InvalidateRect(NULL, FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_WM_SETFOCUS()
	ON_WM_MEASUREITEM()
	ON_WM_MENUCHAR()
	ON_WM_INITMENUPOPUP()
	ON_COMMAND(ID_VIEW_FULLSCREEN, OnViewFullscreen)
	ON_WM_GETMINMAXINFO()
	ON_COMMAND(ID_FILE_PRINTPIECELIST, OnFilePrintPieceList)
	ON_WM_ACTIVATEAPP()
	ON_COMMAND(ID_VIEW_NEWVIEW, OnViewNewView)
	ON_MESSAGE(WM_SETMESSAGESTRING, OnSetMessageString)
	ON_WM_DROPFILES()
	//}}AFX_MSG_MAP
	ON_COMMAND_RANGE(ID_PIECEBAR_NUMBERS, ID_PIECEBAR_SUBPARTS, OnPieceBar)
	ON_UPDATE_COMMAND_UI_RANGE(ID_PIECEBAR_NUMBERS, ID_PIECEBAR_SUBPARTS, OnUpdatePieceBar)
	// Global help commands
	ON_COMMAND(ID_HELP_FINDER, CFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_HELP, CFrameWnd::OnHelp)
	ON_COMMAND(ID_CONTEXT_HELP, CFrameWnd::OnContextHelp)
	ON_COMMAND(ID_DEFAULT_HELP, CFrameWnd::OnHelpFinder)
	// User messages
	ON_MESSAGE(WM_LC_UPDATE_LIST, OnUpdateList)
	ON_MESSAGE(WM_LC_POPUP_CLOSE, OnPopupClose)
	ON_MESSAGE(WM_LC_ADD_COMBO_STRING, OnAddString)
	ON_MESSAGE(WM_LC_UPDATE_INFO, OnUpdateInfo)
	ON_MESSAGE(WM_LC_UPDATE_SETTINGS, UpdateSettings)
	// Toolbar show/hide
	ON_COMMAND_EX(ID_VIEW_ANIMATION_BAR, OnBarCheck)
	ON_COMMAND_EX(ID_VIEW_TOOLS_BAR, OnBarCheck)
	ON_COMMAND_EX(ID_VIEW_PIECES_BAR, OnBarCheck)
	ON_COMMAND_EX(ID_VIEW_MODIFY_BAR, OnBarCheck)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ANIMATION_BAR, OnUpdateControlBarMenu)
	ON_UPDATE_COMMAND_UI(ID_VIEW_TOOLS_BAR, OnUpdateControlBarMenu)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PIECES_BAR, OnUpdateControlBarMenu)
	ON_UPDATE_COMMAND_UI(ID_VIEW_MODIFY_BAR, OnUpdateControlBarMenu)
END_MESSAGE_MAP()

static UINT indicators[] =
	{ ID_SEPARATOR, ID_INDICATOR_POSITION, ID_INDICATOR_SNAP, ID_INDICATOR_STEP };

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	m_pwndFullScrnBar = NULL;
	m_bAutoMenuEnable = FALSE;
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
	m_wndStatusBar.SetPaneStyle(0, SBPS_STRETCH|SBPS_NORMAL);

	if (!m_wndStandardBar.Create(this) ||
		!m_wndStandardBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	m_wndStandardBar.SetBarStyle(m_wndStandardBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndStandardBar.SetWindowText (_T("Standard"));
	m_wndStandardBar.SendMessage(TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS);
	m_wndStandardBar.SetButtonStyle(m_wndStandardBar.CommandToIndex(ID_LOCK_ON), m_wndStandardBar.GetButtonStyle(m_wndStandardBar.CommandToIndex(ID_LOCK_ON)) | TBSTYLE_DROPDOWN);
	m_wndStandardBar.SetButtonStyle(m_wndStandardBar.CommandToIndex(ID_SNAP_ON), m_wndStandardBar.GetButtonStyle(m_wndStandardBar.CommandToIndex(ID_SNAP_ON)) | TBSTYLE_DROPDOWN);
	m_wndStandardBar.EnableDocking(CBRS_ALIGN_ANY);

	if (!m_wndToolsBar.Create(this, WS_CHILD | WS_VISIBLE | CBRS_TOP, ID_VIEW_TOOLS_BAR) ||
		!m_wndToolsBar.LoadToolBar(IDR_TOOLSBAR))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	m_wndToolsBar.SetBarStyle(m_wndToolsBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndToolsBar.SetWindowText (_T("Drawing"));
	m_wndToolsBar.EnableDocking(CBRS_ALIGN_ANY);

	if (!m_wndAnimationBar.Create(this, WS_CHILD | WS_VISIBLE | CBRS_TOP, ID_VIEW_ANIMATION_BAR) ||
		!m_wndAnimationBar.LoadToolBar(IDR_ANIMATORBAR))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	m_wndAnimationBar.SetBarStyle(m_wndAnimationBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndAnimationBar.SetWindowText (_T("Animation"));
	m_wndAnimationBar.EnableDocking(CBRS_ALIGN_ANY);

	if (!m_wndPiecesBar.Create(_T("Pieces"), this, CSize(200, 100),
		TRUE, ID_VIEW_PIECES_BAR))
	{
		TRACE0("Failed to create pieces bar\n");
		return -1;      // fail to create
	}

	if (!m_wndModifyDlg.Create(this, IDD_MODIFY,
	    CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_HIDE_INPLACE,
		ID_VIEW_MODIFY_BAR))
	{
		TRACE0("Failed to create modify dialog bar\n");
		return -1;		// fail to create
	}

	EnableDocking(CBRS_ALIGN_ANY);
	m_wndModifyDlg.SetWindowText (_T("Modify"));
	m_wndModifyDlg.EnableDocking(0);
	ShowControlBar(&m_wndModifyDlg, FALSE, FALSE);
	FloatControlBar(&m_wndModifyDlg, CPoint(10,10));

	// To change the resizable control bar sizes, just change the following
	// members. If you need to change them later, don't forget to call
	// RecalcLayout() or DelayRecalcLayout() after you set the size.
	// You can use this technique to load/save the size of the control bar.
//	m_wndPiecesBar.m_sizeVert = CSize(226, -1); // y size ignored (stretched)
//	m_wndPiecesBar.m_sizeFloat = CSize(226, 270);
	m_wndPiecesBar.SetBarStyle(m_wndPiecesBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndPiecesBar.EnableDocking(CBRS_ALIGN_LEFT|CBRS_ALIGN_RIGHT);

	DockControlBar(&m_wndStandardBar);
	DockControlBar(&m_wndToolsBar);
	DockControlBar(&m_wndPiecesBar, AFX_IDW_DOCKBAR_RIGHT);
		
	CRect rect;
	RecalcLayout(TRUE);
	m_wndToolsBar.GetWindowRect(&rect);
	rect.OffsetRect(1,0);
	DockControlBar(&m_wndAnimationBar, AFX_IDW_DOCKBAR_TOP, &rect);

	if (theApp.GetProfileInt(_T("Settings"), _T("ToolBarVersion"), 0) == TOOLBAR_VERSION)
		LoadBarState("Toolbars");

	// Bitmap menus are cool !
	CMenu* pMenu = GetMenu();
	if (pMenu)
		pMenu->DestroyMenu();
	HMENU hMenu = NewMenu();
	pMenu = CMenu::FromHandle (hMenu);
	SetMenu (pMenu);
	m_hMenuDefault = hMenu;

	UpdateMenuAccelerators();

  messenger->Listen (&mainframe_listener, this);

  main_window->SetXID (this);

  console.SetWindowCallback (&mainframe_console_func, m_wndSplitter.GetPane (1, 0));

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	int status = theApp.GetProfileInt("Settings", "Window Status", -1);
	cs.style &= ~WS_VISIBLE;

	if (status != -1)
	{
		int r,l,b,t;
		char szBuf[60];
		strcpy (szBuf, theApp.GetProfileString("Settings","Window Position"));
		sscanf(szBuf,"%d, %d, %d, %d", &t, &r, &b, &l);

		cs.cx = r - l;
		cs.cy = b - t;
		
		RECT workArea;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
		l += workArea.left;
		t += workArea.top;

		cs.x = min(l, GetSystemMetrics(SM_CXSCREEN) - GetSystemMetrics(SM_CXICON));
		cs.y = min(t, GetSystemMetrics(SM_CYSCREEN) - GetSystemMetrics(SM_CYICON));
	}

	return CFrameWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

// lParam = update pieces, wParam = update colors
LONG CMainFrame::OnUpdateList(UINT lParam, LONG wParam)
{
	if (wParam != 0)
	{
		int x = wParam-1;
		if (x < 14)
		    x *= 2;
		else
			x = ((x-14)*2)+1;

		m_wndPiecesBar.m_wndColorsList.SetCurSel(x);
	}

	return TRUE;
}

// Add a string to the pieces combo
LONG CMainFrame::OnAddString(UINT lParam, LONG /*wParam*/)
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

HMENU CMainFrame::NewMenu()
{
	m_bmpMenu.LoadMenu(IDR_MAINFRAME);
	m_bmpMenu.AddFromToolBar(&m_wndStandardBar, IDR_MAINFRAME);

	// The first parameter is the menu option text. If it's NULL, keep the current text
	// The second option is the ID of the menu option, or the menu
	// option text (use this for adding bitmaps to popup options) to change.
	// The third option is the resource ID of the bitmap.This can also be a
	// toolbar ID in which case the class searches the toolbar for the
	// appropriate bitmap. Only Bitmap and Toolbar resources are supported.
	m_bmpMenu.ModifyODMenu(NULL, ID_FILE_PROPERTIES, IDB_INFO);
	m_bmpMenu.ModifyODMenu(NULL, ID_FILE_SAVEPICTURE, IDB_PHOTO);
	m_bmpMenu.ModifyODMenu(NULL, ID_PIECE_DELETE, IDB_DELETE);
	m_bmpMenu.ModifyODMenu(NULL, ID_PIECE_GROUP, IDB_GROUP);
	m_bmpMenu.ModifyODMenu(NULL, ID_PIECE_UNGROUP, IDB_UNGROUP);
	m_bmpMenu.ModifyODMenu(NULL, ID_VIEW_PREFERENCES, IDB_PREFERENCES);
	m_bmpMenu.ModifyODMenu(NULL, ID_VIEW_ZOOMOUT, IDB_ZOOMOUT);
	m_bmpMenu.ModifyODMenu(NULL, ID_VIEW_ZOOMIN, IDB_ZOOMIN);
	m_bmpMenu.ModifyODMenu(NULL, ID_VIEW_FULLSCREEN, IDB_FULLSCREEN);
	m_bmpMenu.ModifyODMenu(NULL, ID_HELP_FINDER, IDB_HELP);
	m_bmpMenu.ModifyODMenu(NULL, ID_HELP_LEOCADHOMEPAGE, IDB_HOME);
	m_bmpMenu.ModifyODMenu(NULL, ID_HELP_SENDEMAIL, IDB_MAIL);

	m_bmpMenu.ModifyODMenu(NULL, _T("Cameras"), IDB_CAMERA);

/*
	m_menubar.ModifyODMenu(NULL,"Step", IDB_STEP);
*/
	return m_bmpMenu.Detach();
}

void CMainFrame::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct) 
{
	if(lpMeasureItemStruct->CtlType == ODT_MENU)
	{
		if(IsMenu((HMENU)lpMeasureItemStruct->itemID))
		{
			CMenu* cmenu = CMenu::FromHandle((HMENU)lpMeasureItemStruct->itemID);
			if(m_bmpMenu.IsMenu(cmenu))
			{
				m_bmpMenu.MeasureItem(lpMeasureItemStruct);
				return;
			}
		}
	}

	CFrameWnd::OnMeasureItem(nIDCtl, lpMeasureItemStruct);
}

LRESULT CMainFrame::OnMenuChar(UINT nChar, UINT nFlags, CMenu* pMenu) 
{
	if (m_bmpMenu.IsMenu(pMenu))
		return CBMPMenu::FindKeyboardShortcut(nChar, nFlags, pMenu);
	else
		return CFrameWnd::OnMenuChar(nChar, nFlags, pMenu);
}

void CMainFrame::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu) 
{
	CFrameWnd::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

	if(!bSysMenu)
	{
		if(m_bmpMenu.IsMenu(pPopupMenu))
			CBMPMenu::UpdateMenu(pPopupMenu);
	}
}

LONG CMainFrame::OnUpdateInfo(UINT lParam, LONG wParam)
{
	m_wndModifyDlg.UpdateInfo((Object*)lParam);

	char str[128];
	Vector3 pos;

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

LONG CMainFrame::OnPopupClose(UINT /*lParam*/, LONG /*wParam*/)
{
	m_wndStatusBar.m_pPopup = NULL;
	return TRUE;
}

LONG CMainFrame::UpdateSettings(UINT /*lParam*/, LONG /*wParam*/)
{
	int i = theApp.GetProfileInt("Settings", "Piecebar Options", 0);
	m_wndPiecesBar.m_bSubParts = (i & PIECEBAR_SUBPARTS) != 0;
	m_wndPiecesBar.m_bNumbers = (i & PIECEBAR_PARTNUMBERS) != 0;

	RECT rc;
	m_wndPiecesBar.GetClientRect(&rc);
	m_wndPiecesBar.PostMessage(WM_SIZE, SIZE_RESTORED, MAKELPARAM(rc.right, rc.bottom));
	PostMessage(WM_LC_UPDATE_LIST, 1, 0);

	return TRUE;
}

void CMainFrame::OnClose() 
{
	if (m_lpfnCloseProc != NULL && !(*m_lpfnCloseProc)(this))
		return;

	if (!lcGetActiveProject()->SaveModified())
		return;

	if (GetStyle() & WS_VISIBLE)
	{
		// save window size and position when destroyed
		WINDOWPLACEMENT wp;
		char szBuf[60];

		if (m_pwndFullScrnBar)
		{
			m_pwndFullScrnBar->DestroyWindow();
			delete m_pwndFullScrnBar;
			m_wndStatusBar.ShowWindow(SW_SHOWNORMAL);
			wp = m_wpPrev;
		}
		else
		{
			wp.length = sizeof(wp);
			GetWindowPlacement(&wp);
		}

		wsprintf(szBuf,"%d, %d, %d, %d", wp.rcNormalPosition.top, wp.rcNormalPosition.right, 
			wp.rcNormalPosition.bottom, wp.rcNormalPosition.left);
		theApp.WriteProfileString("Settings","Window Position", szBuf);
		theApp.WriteProfileInt("Settings", "Window Status", wp.showCmd);

		SaveBarState("Toolbars");
		theApp.WriteProfileInt(_T("Settings"), _T("ToolBarVersion"), TOOLBAR_VERSION);
	}

	AfxGetApp()->HideApplication();
	GetActiveDocument()->OnCloseDocument();
	AfxPostQuitMessage(0);
}

void CMainFrame::OnSetFocus(CWnd* pOldWnd) 
{
	CFrameWnd::OnSetFocus(pOldWnd);
	
	if (m_wndStatusBar.m_pPopup)
		m_wndStatusBar.m_pPopup->DestroyWindow();
}

void CMainFrame::OnPieceBar(UINT nID)
{
	switch (nID)
	{
		case ID_PIECEBAR_NUMBERS:
		{
			m_wndPiecesBar.m_bNumbers = !m_wndPiecesBar.m_bNumbers;
		} break;
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
		
		if (nID == ID_PIECEBAR_NUMBERS)
			PostMessage(WM_LC_UPDATE_LIST, 1, 0);
	}

	UINT u = 0;
	if (m_wndPiecesBar.m_bSubParts) u |= PIECEBAR_SUBPARTS;
	if (m_wndPiecesBar.m_bNumbers) u |= PIECEBAR_PARTNUMBERS;
	theApp.WriteProfileInt("Settings", "Piecebar Options", u);
}

void CMainFrame::OnUpdatePieceBar(CCmdUI* pCmdUI)
{
	switch (pCmdUI->m_nID)
	{
		case ID_PIECEBAR_NUMBERS:
			pCmdUI->SetCheck(m_wndPiecesBar.m_bNumbers); break;
		case ID_PIECEBAR_SUBPARTS:
			pCmdUI->SetCheck(m_wndPiecesBar.m_bSubParts); break;
	}
}

void CMainFrame::OnViewFullscreen() 
{
	RECT rectDesktop;
	WINDOWPLACEMENT wpNew;
	
	if (m_pwndFullScrnBar == NULL)
	{
		m_wndStatusBar.ShowWindow(SW_HIDE);
		GetWindowPlacement (&m_wpPrev);
		m_wpPrev.length = sizeof m_wpPrev;
		
		//Adjust RECT to new size of window
		::GetWindowRect (::GetDesktopWindow(), &rectDesktop);
		::AdjustWindowRectEx(&rectDesktop, GetStyle(), TRUE, GetExStyle());

		// Remember this for OnGetMinMaxInfo()
		m_FullScreenWindowRect = rectDesktop;
		
		wpNew = m_wpPrev;
		wpNew.showCmd =  SW_SHOWNORMAL;
		wpNew.rcNormalPosition = rectDesktop;
		
		m_pwndFullScrnBar = new CToolBar;
		
		if(!m_pwndFullScrnBar->Create(this,CBRS_SIZE_DYNAMIC|CBRS_FLOATING)
			|| !m_pwndFullScrnBar->LoadToolBar(IDR_FULLSCREEN))
		{
			TRACE0("Failed to create toolbar\n");
			return; 	 // fail to create
		}
		
		//don't allow the toolbar to dock
		m_pwndFullScrnBar->EnableDocking(0);
		m_pwndFullScrnBar->SetWindowPos(0,30,30,
			0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_SHOWWINDOW);
		m_pwndFullScrnBar->SetWindowText(_T("Full Screen"));
		m_pwndFullScrnBar->GetToolBarCtrl().CheckButton(ID_VIEW_FULLSCREEN, TRUE);
		FloatControlBar(m_pwndFullScrnBar, CPoint(30,30));
	}
	else
	{
		m_pwndFullScrnBar->DestroyWindow();
		delete m_pwndFullScrnBar;
		m_pwndFullScrnBar = NULL;
		m_wndStatusBar.ShowWindow(SW_SHOWNORMAL);
		wpNew = m_wpPrev;
	}

	SetWindowPlacement (&wpNew);
}

void CMainFrame::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI) 
{
	if (m_pwndFullScrnBar != NULL)
	{
		lpMMI->ptMaxSize.y = m_FullScreenWindowRect.Height();
		lpMMI->ptMaxTrackSize.y = lpMMI->ptMaxSize.y;
		lpMMI->ptMaxSize.x = m_FullScreenWindowRect.Width();
		lpMMI->ptMaxTrackSize.x = lpMMI->ptMaxSize.x;
	}
	else
		CFrameWnd::OnGetMinMaxInfo(lpMMI);
}

void CMainFrame::GetMessageString(UINT nID, CString& rMessage) const
{
	if (nID >= ID_CAMERA_FIRST && nID <= ID_CAMERA_LAST)
	{
		Camera* pCamera = lcGetActiveProject()->GetCamera(nID-ID_CAMERA_FIRST);
		rMessage = "Use the camera \"";
		rMessage += pCamera->GetName();
		rMessage += "\"";
	}
	else
		CFrameWnd::GetMessageString(nID, rMessage);
}

void CMainFrame::OnFilePrintPieceList() 
{
	AfxBeginThread(PrintPiecesFunction, this);
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

	if (nID >= ID_VIEWPORT01 && nID <= ID_VIEWPORT14)
	{
		project->HandleCommand(LC_VIEW_VIEWPORTS, nID - ID_VIEWPORT01);
		return TRUE;
	}

	if (nID >= ID_ACTION_SELECT && nID <= ID_ACTION_ROLL)
	{
		project->SetAction(nID - ID_ACTION_SELECT);
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

		case ID_RENDER_BACKGROUND: {
			project->HandleCommand(LC_TOOLBAR_BACKGROUND, 0);
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

		case ID_SNAP_ON:
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

			PiecesLibrary* Lib = lcGetPiecesLibrary();
			CString CategoryName = m_wndPiecesBar.m_PiecesTree.GetItemText(Item);
			int Index = Lib->FindCategoryIndex((const char*)CategoryName);

			if (Index == -1)
				break;

			LC_CATEGORYDLG_OPTS Opts;
			Opts.Name = Lib->GetCategoryName(Index);
			Opts.Keywords = Lib->GetCategoryKeywords(Index);

			if (SystemDoDialog(LC_DLG_EDITCATEGORY, &Opts))
			{
				String OldName = Lib->GetCategoryName(Index);
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
				PiecesLibrary* Lib = lcGetPiecesLibrary();
				Lib->AddCategory(Opts.Name, Opts.Keywords);
				m_wndPiecesBar.UpdatePiecesTree(NULL, Opts.Name);
			}

		} break;

		case ID_PIECEBAR_REMOVECATEGORY:
		{
			HTREEITEM Item = m_wndPiecesBar.m_PiecesTree.GetSelectedItem();

			if (Item == NULL)
				break;

			PiecesLibrary* Lib = lcGetPiecesLibrary();
			CString CategoryName = m_wndPiecesBar.m_PiecesTree.GetItemText(Item);
			int Index = Lib->FindCategoryIndex((const char*)CategoryName);

			if (Index == -1)
				break;

			char Msg[1024];
			String Name = Lib->GetCategoryName(Index);
			sprintf(Msg, "Are you sure you want to remove the %s category?", Name);

			if (SystemDoMessageBox(Msg, LC_MB_YESNO | LC_MB_ICONQUESTION) == LC_YES)
			{
				Lib->RemoveCategory(Index);
				m_wndPiecesBar.UpdatePiecesTree(Name, NULL);
			}

		} break;

		default:
			return CFrameWnd::OnCommand(wParam, lParam);
	}

	return TRUE;
}

void CMainFrame::OnActivateApp(BOOL bActive, ACTIVATEAPPPARAM hTask) 
{
	CFrameWnd::OnActivateApp(bActive, hTask);
	
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
		if (!m_strStatusBar.IsEmpty())
		{
			lpsz = m_strStatusBar;
		}
		else if (lParam != 0)
		{
			ASSERT(wParam == 0);    // can't have both an ID and a string
			lpsz = (LPCTSTR)lParam; // set an explicit string
		}
		else if (wParam != 0)
		{
			// map SC_CLOSE to PREVIEW_CLOSE when in print preview mode
			if (wParam == AFX_IDS_SCCLOSE && m_lpfnCloseProc != NULL)
				wParam = AFX_IDS_PREVIEW_CLOSE;

			// get message associated with the ID indicated by wParam
			GetMessageString(wParam, strMessage);
			lpsz = strMessage;
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

#include "view.h"
LRESULT CALLBACK GLWindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void CMainFrame::OnViewNewView() 
{
	HINSTANCE hInst = AfxGetInstanceHandle();
	WNDCLASS wndcls;

#define OPENGL_CLASSNAME _T("LeoCADOpenGLClass")
#define FLOATING_CLASSNAME _T("LeoCADFloatingOpenGLClass")

  // check if our class is registered
	if(!(GetClassInfo (hInst, FLOATING_CLASSNAME, &wndcls)))
	{
  	if (GetClassInfo (hInst, OPENGL_CLASSNAME, &wndcls))
	  {
      // set our class name
	  	wndcls.lpszClassName = FLOATING_CLASSNAME;
      wndcls.lpfnWndProc = GLWindowProc;
      wndcls.hIcon = LoadIcon (hInst, MAKEINTRESOURCE (IDR_MAINFRAME));

  		// register class
	  	if (!AfxRegisterClass (&wndcls))
		  	AfxThrowResourceException();
  	}
		else
			AfxThrowResourceException();
  }

  View *view = new View (lcGetActiveProject(), NULL);

  CreateWindowEx (0, FLOATING_CLASSNAME, "LeoCAD",
    WS_VISIBLE | WS_POPUPWINDOW | WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT, CW_USEDEFAULT, 200, 100,
    m_hWnd, (HMENU)0, hInst, view);
}

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
  m_wndSplitter.CreateStatic (this, 2, 1, WS_CHILD | WS_VISIBLE, AFX_IDW_PANE_FIRST);

  m_wndSplitter.CreateView (0, 0, RUNTIME_CLASS (CCADView), CSize (0, 1000), pContext);
  m_wndSplitter.CreateView (1, 0, RUNTIME_CLASS (CRichEditView), CSize (0, 0), pContext);
  m_wndSplitter.SetRowInfo (1, 50, 0);

	// Setup the console.
	CRichEditCtrl& Edit = ((CRichEditView*) m_wndSplitter.GetPane(1, 0))->GetRichEditCtrl();
	Edit.SetReadOnly (TRUE);

	CHARFORMAT cf;
	memset(&cf, 0, sizeof(cf));
	cf.dwMask = CFM_BOLD | CFM_FACE;

	NONCLIENTMETRICS nm;
	nm.cbSize = sizeof (NONCLIENTMETRICS);
	VERIFY (SystemParametersInfo(SPI_GETNONCLIENTMETRICS,nm.cbSize,&nm,0)); 
	strcpy(cf.szFaceName, nm.lfStatusFont.lfFaceName);

	Edit.SetDefaultCharFormat(cf);

  return TRUE;
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg) 
{
	// Check if the user pressed any accelerator.
	if (pMsg->message == WM_KEYDOWN)
	{
		if ((HIWORD(pMsg->lParam) & KF_REPEAT) == 0)
		{
			bool Control = GetKeyState(VK_CONTROL) < 0;
			bool Shift = GetKeyState(VK_SHIFT) < 0;

			// Don't process key presses if the user is typing text.
			if (!Control)
			{
				CWnd* Focus = GetFocus();

				if (Focus != NULL)
				{
					if (m_wndPiecesBar.m_wndPiecesCombo.IsChild(Focus))
					{
						return CFrameWnd::PreTranslateMessage(pMsg);
					}

					char Name[256];
					GetClassName(Focus->m_hWnd, Name, sizeof(Name));
					if (!strcmp(Name, "Edit"))
					{
						return CFrameWnd::PreTranslateMessage(pMsg);
					}
				}
			}

			if (m_wndPiecesBar.m_wndPiecesCombo.IsChild(GetFocus()))
			{
				if (!Control && (((pMsg->wParam >= 'A') && (pMsg->wParam <= 'Z')) || ((pMsg->wParam >= '0') && (pMsg->wParam <= '9'))))
				{
					return CFrameWnd::PreTranslateMessage(pMsg);
				}
			}

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
						return true;
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
						return true;
					}
				}
			}
		}
	}

	return CFrameWnd::PreTranslateMessage(pMsg);
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
		0,                         // LC_VIEW_VIEWPORTS
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
		ID_CAMERA_FIRST + 0,       // LC_VIEW_CAMERA_FRONT,
		ID_CAMERA_FIRST + 1,       // LC_VIEW_CAMERA_BACK,
		ID_CAMERA_FIRST + 2,       // LC_VIEW_CAMERA_TOP,
		ID_CAMERA_FIRST + 3,       // LC_VIEW_CAMERA_BOTTOM,
		ID_CAMERA_FIRST + 4,       // LC_VIEW_CAMERA_LEFT,
		ID_CAMERA_FIRST + 5,       // LC_VIEW_CAMERA_RIGHT,
		ID_CAMERA_FIRST + 6,       // LC_VIEW_CAMERA_MAIN,
		0,                         // LC_VIEW_CAMERA_MENU
		ID_VIEW_CAMERAS_RESET,     // LC_VIEW_CAMERA_RESET
		0,                         // LC_VIEW_AUTOPAN
		ID_APP_ABOUT,              // LC_HELP_ABOUT
		0,                         // LC_TOOLBAR_ANIMATION
		0,                         // LC_TOOLBAR_ADDKEYS
		0,                         // LC_TOOLBAR_SNAPMENU
		0,                         // LC_TOOLBAR_LOCKMENU
		0,                         // LC_TOOLBAR_FASTRENDER
		0,                         // LC_TOOLBAR_BACKGROUND
		0,                         // LC_EDIT_MOVE_SNAP_0
		0,                         // LC_EDIT_MOVE_SNAP_1
		0,                         // LC_EDIT_MOVE_SNAP_2
		0,                         // LC_EDIT_MOVE_SNAP_3
		0,                         // LC_EDIT_MOVE_SNAP_4
		0,                         // LC_EDIT_MOVE_SNAP_5
		0,                         // LC_EDIT_MOVE_SNAP_6
		0,                         // LC_EDIT_MOVE_SNAP_7
		0,                         // LC_EDIT_MOVE_SNAP_8
		0,                         // LC_EDIT_MOVE_SNAP_9
		0,                         // LC_EDIT_ANGLE_SNAP_0
		0,                         // LC_EDIT_ANGLE_SNAP_1
		0,                         // LC_EDIT_ANGLE_SNAP_2
		0,                         // LC_EDIT_ANGLE_SNAP_3
		0,                         // LC_EDIT_ANGLE_SNAP_4
		0,                         // LC_EDIT_ANGLE_SNAP_5
		0,                         // LC_EDIT_ANGLE_SNAP_6
		0,                         // LC_EDIT_ANGLE_SNAP_7
		0,                         // LC_EDIT_ANGLE_SNAP_8
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

	m_bmpMenu.Attach(m_hMenuDefault);

	for (int i = 0; i < KeyboardShortcutsCount; i++)
	{
		LC_KEYBOARD_COMMAND& Cmd = KeyboardShortcuts[i];
		WORD ID = CmdToID[Cmd.ID];
		String str;

		if (ID == 0)
			continue;

		if (Cmd.Key1)
		{
			if (Cmd.Flags & LC_KEYMOD1_SHIFT)
				str += "Shift+";

			if (Cmd.Flags & LC_KEYMOD1_CONTROL)
				str += "Ctrl+";

			str += GetKeyName(Cmd.Key1);

			if (Cmd.Key2)
			{
				str += ", ";
				if (Cmd.Flags & LC_KEYMOD2_SHIFT)
					str += "Shift+";

				if (Cmd.Flags & LC_KEYMOD2_CONTROL)
					str += "Ctrl+";

				str += GetKeyName(Cmd.Key2);
			}
		}

		m_bmpMenu.ChangeMenuItemShortcut(str, ID);
	}

	m_bmpMenu.Detach();
}

void CMainFrame::OnDropFiles(HDROP hDropInfo) 
{
	SetActiveWindow();      // activate us first !
	UINT nFiles = ::DragQueryFile(hDropInfo, (UINT)-1, NULL, 0);

	if (nFiles > 0)
	{
		TCHAR szFileName[_MAX_PATH];
		::DragQueryFile(hDropInfo, 0, szFileName, _MAX_PATH);

		lcGetActiveProject()->OpenProject(szFileName);
	}
	::DragFinish(hDropInfo);
}
