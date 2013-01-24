#include "lc_global.h"
#include <dlgs.h>
#include <direct.h>
#include "leocad.h"
#include "system.h"
#include "camera.h"
#include "tools.h"
#include "lc_file.h"
#include "image.h"
#include "PieceBar.h"
#include "PropsSht.h"
#include "PrefSht.h"
#include "arraydlg.h"
#include "imagedlg.h"
#include "groupdlg.h"
#include "figdlg.h"
#include "seldlg.h"
#include "htmldlg.h"
#include "stepdlg.h"
#include "povdlg.h"
#include "terrdlg.h"
#include "LibDlg.h"
#include "EdGrpDlg.h"
#include "AboutDlg.h"
#include "categdlg.h"
#include "cadbar.h"
#include "mainfrm.h"
#include "project.h"
#include "globals.h"
#include "lc_application.h"
#include "piece.h"
#include "pieceinf.h"

// Display a message box when an assert happens.
bool lcAssert(const char* FileName, int Line, const char* Expression, const char* Description)
{
	char buf[1024];
	sprintf(buf, "Assertion failed on line %d of file %s.\n%s\nDo you want to debug this?", Line, FileName, Description);

	int ret = AfxMessageBox(buf, MB_YESNOCANCEL|MB_ICONERROR);

	if (ret == IDYES)
		DebugBreak();
	else if (ret == IDCANCEL)
		return true; // Disable this assert.

	return false;
}

static CStepDlg* StepModeless = NULL;
static UINT ClipboardFormat = 0;

/////////////////////////////////////////////////////////////////////////////
// Static functions

static void ShowLastError()
{
	LPVOID lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf, 0, NULL);

	MessageBox( NULL, (char*)lpMsgBuf, "GetLastError", MB_OK|MB_ICONINFORMATION );
	LocalFree( lpMsgBuf );
}

static CMenu* GetMainMenu(int nIndex)
{
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();

	if (pFrame == NULL)
		return NULL;

	CMenu* pMenu =  CMenu::FromHandle(pFrame->m_wndMenuBar.GetHMenu());

	if (pMenu == NULL)
		return NULL;

	return pMenu->GetSubMenu(nIndex);
}

UINT APIENTRY OFNOpenHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uiMsg)
	{
/*
		case WM_HELP: 
		{
			LPHELPINFO lphi = (LPHELPINFO)lParam;
			if (lphi->iContextType == HELPINFO_WINDOW)   // must be for a control
				WinHelp ((HWND)lphi->hItemHandle, AfxGetApp()->m_pszHelpFilePath, HELP_WM_HELP, (DWORD)(LPVOID)HelpIDs);
			return TRUE;
		} break;

		case WM_CONTEXTMENU:
		{
			WinHelp ((HWND)wParam, AfxGetApp()->m_pszHelpFilePath, HELP_CONTEXTMENU, (DWORD)(LPVOID)HelpIDs);
			return TRUE;
		} break;

*/
		case WM_INITDIALOG: 
		{
			RECT rc1, rc2;
			HWND h = GetParent(hdlg);
			GetWindowRect(GetDlgItem(h, lst1), &rc1);
			ScreenToClient(h, (LPPOINT)&rc1);
			ScreenToClient(h, ((LPPOINT)&rc1)+1);
			GetWindowRect(hdlg, &rc2);
			SetWindowPos(hdlg, NULL, 0, 0, 122 + rc1.left, rc2.bottom - rc2.top, SWP_NOMOVE);
			HBITMAP hbm = CreateColorBitmap(120, 100, GetSysColor(COLOR_BTNFACE));
			SendDlgItemMessage(hdlg, IDC_OPENDLG_PREVIEW, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbm);
			GetWindowRect(GetDlgItem(hdlg, IDC_OPENDLG_PREVIEW), &rc1);
			GetWindowRect(GetDlgItem(hdlg, IDC_OPENDLG_TEXT), &rc2);
			SetWindowPos(GetDlgItem(hdlg, IDC_OPENDLG_TEXT), NULL, 0, 0, rc1.right - rc1.left, rc2.bottom - rc2.top, SWP_NOMOVE);
		} break;


		case WM_NOTIFY: 
		{
			LPNMHDR pnmh = (LPNMHDR) lParam;
			if (pnmh->code == CDN_FILEOK)
			{
//					HBITMAP hbmold = (HBITMAP)SendDlgItemMessage(hdlg, IDC_OPENDLG_BITMAP, STM_GETIMAGE, IMAGE_BITMAP, 0);
//					if (hbmold)
//						DeleteObject(hbmold);

				// This avoids an assert
				_AFX_THREAD_STATE* pThreadState = AfxGetThreadState();
				pThreadState->m_pAlternateWndInit = NULL;
				return FALSE;
			}

			if (pnmh->code == CDN_SELCHANGE)
			{
				char filename[_MAX_PATH];
				SendMessage(GetParent(hdlg), CDM_GETFILEPATH, _MAX_PATH, (LPARAM)filename);
				HBITMAP hbm = NULL;
				char *p = strrchr(filename, '.');

				if ((p && (_stricmp (p+1, "lcd"))) ||
					((GetFileAttributes(filename) & FILE_ATTRIBUTE_DIRECTORY)))
				{
					hbm = CreateColorBitmap (120, 100, GetSysColor (COLOR_BTNFACE));
					HBITMAP hbmold = (HBITMAP)SendDlgItemMessage(hdlg, IDC_OPENDLG_PREVIEW, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbm);
					if (hbmold)
						DeleteObject(hbmold);
					return FALSE;
				}

				float fv;
				char id[32];
				lcDiskFile file;
				file.Open(filename, "rb");
				file.ReadBuffer(id, 32);
				sscanf(strchr(id, ' '), "%f", &fv);

				if (fv > 0.4f)
				{
					file.ReadFloats(&fv, 1);

					if (fv > 0.7f)
					{
						lcuint32 dwPosition;
						file.Seek(-4, SEEK_END);
						file.ReadU32(&dwPosition, 1);
						file.Seek(dwPosition, SEEK_SET);

						if (dwPosition != 0)
						{
							if (fv < 1.0f)
							{
								BITMAPFILEHEADER bmfHeader;
								file.ReadBuffer((LPSTR)&bmfHeader, sizeof(bmfHeader));
								DWORD nPackedDIBLen = sizeof(BITMAPINFOHEADER) + 36000;
								HGLOBAL hDIB = ::GlobalAlloc(GMEM_FIXED, nPackedDIBLen);
								file.ReadBuffer((LPSTR)hDIB, nPackedDIBLen);
								BITMAPINFOHEADER &bmiHeader = *(LPBITMAPINFOHEADER)hDIB;
								BITMAPINFO &bmInfo = *(LPBITMAPINFO)hDIB;
								int nColors = bmiHeader.biClrUsed ? bmiHeader.biClrUsed : 1 << bmiHeader.biBitCount;
								LPVOID lpDIBBits;
								if (bmInfo.bmiHeader.biBitCount > 8)
									lpDIBBits = (LPVOID)((LPDWORD)(bmInfo.bmiColors + bmInfo.bmiHeader.biClrUsed) + 
									((bmInfo.bmiHeader.biCompression == BI_BITFIELDS) ? 3 : 0));
								else
									lpDIBBits = (LPVOID)(bmInfo.bmiColors + nColors);
								
								CClientDC dc(NULL);
								hbm = CreateDIBitmap(dc.m_hDC, &bmiHeader, CBM_INIT, lpDIBBits, &bmInfo, DIB_RGB_COLORS);
								::GlobalFree(hDIB);
							}
							else
							{
								Image image;

								if (image.FileLoad (file))
								{
									HWND hwndDesktop = GetDesktopWindow(); 
									HDC hdcDesktop = GetDC(hwndDesktop); 
									HDC hdcMem = CreateCompatibleDC(hdcDesktop); 
									hbm = CreateCompatibleBitmap(hdcDesktop, 120, 100);
									HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbm); 

									for (int y = 0; y < 100; y++)
										for (int x = 0; x < 120; x++)
										{
											unsigned char* b = image.GetData () + (y*120+x)*3;
											SetPixelV(hdcMem, x, y, RGB(b[0], b[1], b[2]));
										}

										// Clean up
										SelectObject(hdcMem, hbmOld); 
										DeleteDC(hdcMem); 
										ReleaseDC(hwndDesktop, hdcDesktop); 
								}
							}
						}
					}
				}

				if (!hbm)
					hbm = CreateColorBitmap (120, 100, GetSysColor(COLOR_BTNFACE));

				HBITMAP hbmold = (HBITMAP)SendDlgItemMessage(hdlg, IDC_OPENDLG_PREVIEW, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbm);
				if (hbmold)
					DeleteObject(hbmold);

				InvalidateRect(GetDlgItem(hdlg, IDC_OPENDLG_PREVIEW), NULL, TRUE);
			}
		} break;

		case WM_DESTROY: 
		{
			HBITMAP hbmold = (HBITMAP)SendDlgItemMessage(hdlg, IDC_OPENDLG_PREVIEW, STM_GETIMAGE, IMAGE_BITMAP, 0);
			if (hbmold)
				DeleteObject(hbmold);
		} break;
	}

	return FALSE;
}

static UINT APIENTRY OFNSaveHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uiMsg)
	{
		case WM_INITDIALOG: 
		{
			int i = theApp.GetProfileInt ("Default", "Save Preview", 0);
			if (i != 0) 
				CheckDlgButton(hdlg, IDC_SAVEDLG_PREVIEW, BST_CHECKED);
		}

		case WM_NOTIFY: 
		{
			LPNMHDR pnmh = (LPNMHDR) lParam;
			if (pnmh->code == CDN_FILEOK)
			{
				int i = 0;
				if (IsDlgButtonChecked(hdlg, IDC_SAVEDLG_PREVIEW))
					i = 1;
				theApp.WriteProfileInt ("Default", "Save Preview", i);

				// This avoids an assert
				_AFX_THREAD_STATE* pThreadState = AfxGetThreadState();
				pThreadState->m_pAlternateWndInit = NULL;
				return FALSE;
			}
		}
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Wait cursor

static int g_nWaitCursorCount;          // for wait cursor (>0 => waiting)
static HCURSOR g_hcurWaitCursorRestore; // old cursor to restore after wait cursor

// 0 => restore, 1=> begin, -1=> end
void SystemDoWaitCursor(int nCode)
{
	g_nWaitCursorCount += nCode;
	if (g_nWaitCursorCount > 0)
	{
		HCURSOR hcurWait = ::LoadCursor(NULL, IDC_WAIT);
		HCURSOR hcurPrev = ::SetCursor(hcurWait);
		if (nCode > 0 && g_nWaitCursorCount == 1)
			g_hcurWaitCursorRestore = hcurPrev;
	}
	else
	{
		// turn everything off
		g_nWaitCursorCount = 0;     // prevent underflow
		::SetCursor(g_hcurWaitCursorRestore);
	}
}

void Sys_BeginWait ()
{
	SystemDoWaitCursor (1);
}

void Sys_EndWait ()
{
	SystemDoWaitCursor (-1);
}

/////////////////////////////////////////////////////////////////////////////
// Profile Access

// returns the store value or default
int Sys_ProfileLoadInt(const char* section, const char* entry, const int defaultvalue)
{
	return theApp.GetProfileInt(section, entry, defaultvalue);
}

// returns true if successful
bool Sys_ProfileSaveInt(const char* section, const char* entry, const int value)
{
	return theApp.WriteProfileInt(section, entry, value) ? true : false;
}

char* Sys_ProfileLoadString(const char* section, const char* entry, const char* defaultvalue)
{
	static CString str;
	str = theApp.GetProfileString(section, entry, defaultvalue);
	return (char*)(LPCSTR)str;
}

bool Sys_ProfileSaveString(const char* section, const char* entry, const char* value)
{
	return theApp.WriteProfileString(section, entry, value) ? true : false;
}

/////////////////////////////////////////////////////////////////////////////
// User Interface

static HBITMAP hbmMenuDot;
static const BYTE rgbDot[] =
	{ 0x6, 0xF, 0xF, 0xF, 0x6 }; // simple byte bitmap, 1=> bit on
#define DOT_WIDTH   4
#define DOT_HEIGHT  5

void SystemFinish()
{
	DeleteObject(hbmMenuDot);
}

void SystemInit()
{
	ClipboardFormat = RegisterClipboardFormat(_T("LeoCAD_Data"));

	// initialize wait cursor state
	g_nWaitCursorCount = 0;
	g_hcurWaitCursorRestore = NULL;
}

static void CheckToolBarButton(CMFCToolBar& ToolBar, int ID, bool Check)
{
	int Index = ToolBar.CommandToIndex(ID);
	UINT NewStyle = ToolBar.GetButtonStyle(Index) & ~(TBBS_CHECKED | TBBS_INDETERMINATE);
	if (Check)
		NewStyle |= TBBS_CHECKED;
	ToolBar.SetButtonStyle(Index, NewStyle | TBBS_CHECKBOX);
}

static void EnableToolBarButton(CMFCToolBar& ToolBar, int ID, bool Enable)
{
	int Index = ToolBar.CommandToIndex(ID);
	UINT NewStyle = ToolBar.GetButtonStyle(Index) & ~TBBS_DISABLED;

	if (!Enable)
		NewStyle |= TBBS_DISABLED;
	ToolBar.SetButtonStyle(Index, NewStyle);
}

// Action toolbar, popup menu and cursor.
void SystemUpdateAction(int nNew, int nOld)
{
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	if (!pFrame)
		return;

//	CheckToolBarButton(pFrame->m_wndToolsBar, ID_ACTION_SELECT+nOld, FALSE);
//	CheckToolBarButton(pFrame->m_wndToolsBar, ID_ACTION_SELECT+nNew, TRUE);

	// TODO: make sure this works if loading a file from the cmd line.
	CView* pView = pFrame->GetActiveView();
	if (pView)
		pView->SendMessage(WM_LC_SET_CURSOR, nNew);

	// TODO: update popup context menu
	// TODO: disable lights if count > 8
}

// Current color in the listbox;
void SystemUpdateColorList(int nNew)
{
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	if (!pFrame)
		return;

	pFrame->PostMessage (WM_LC_UPDATE_LIST, 0, nNew+1);
}

void SystemUpdateRenderingMode(bool bFast)
{
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	if (!pFrame)
		return;

	CheckToolBarButton(pFrame->m_wndStandardBar, ID_RENDER_BOX, bFast);
}

void SystemUpdateUndoRedo(char* undo, char* redo)
{
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	if (!pFrame)
		return;

	EnableToolBarButton(pFrame->m_wndStandardBar, ID_EDIT_UNDO, undo != NULL);
	EnableToolBarButton(pFrame->m_wndStandardBar, ID_EDIT_REDO, redo != NULL);

	CMFCMenuBar& MenuBar = pFrame->m_wndMenuBar;
	CMFCToolBarButton* pEditButton = MenuBar.GetButton(1);
	CMFCToolBarMenuButton* pEditMenuButton = DYNAMIC_DOWNCAST(CMFCToolBarMenuButton, pEditButton);

	const CObList& editCommands = pEditMenuButton->GetCommands();

	for (POSITION pos = editCommands.GetHeadPosition (); pos != NULL;)
	{
		CMFCToolBarButton* pSubButton = (CMFCToolBarButton*)editCommands.GetNext(pos);
		ASSERT_VALID(pSubButton);

		UINT Style = pSubButton->m_nStyle;

		switch (pSubButton->m_nID)
		{
		case ID_EDIT_UNDO:
			if (undo)
			{
				pSubButton->SetStyle(Style & ~TBBS_DISABLED);
				pSubButton->m_strText = "Undo " + CString(undo);
			}
			else
			{
				pSubButton->SetStyle(Style | TBBS_DISABLED);
				pSubButton->m_strText = "Undo";
			}
			break;

		case ID_EDIT_REDO:
			if (redo)
			{
				pSubButton->SetStyle(Style & ~TBBS_DISABLED);
				pSubButton->m_strText = "Redo " + CString(redo);
			}
			else
			{
				pSubButton->SetStyle(Style | TBBS_DISABLED);
				pSubButton->m_strText = "Redo";
			}
			break;
		}
	}
}

void SystemUpdateSnap(const unsigned long nSnap)
{
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	if (!pFrame)
		return;

	CheckToolBarButton(pFrame->m_wndStandardBar, ID_SNAP_ANGLE, (nSnap & LC_DRAW_SNAP_A) != 0);
}

void SystemUpdateSelected(unsigned long flags, int SelectedCount, Object* Focus)
{
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	if (!pFrame)
		return;

	CMFCMenuBar& MenuBar = pFrame->m_wndMenuBar;
	CMFCToolBarButton* pEditButton = MenuBar.GetButton(1);
	CMFCToolBarMenuButton* pEditMenuButton = DYNAMIC_DOWNCAST(CMFCToolBarMenuButton, pEditButton);

	const CObList& editCommands = pEditMenuButton->GetCommands();

	for (POSITION pos = editCommands.GetHeadPosition (); pos != NULL;)
	{
		CMFCToolBarButton* pSubButton = (CMFCToolBarButton*)editCommands.GetNext(pos);
		ASSERT_VALID(pSubButton);

		UINT Style = pSubButton->m_nStyle;

		switch (pSubButton->m_nID)
		{
		case ID_EDIT_CUT:
		case ID_EDIT_COPY:
			if (flags & (LC_SEL_PIECE|LC_SEL_CAMERA|LC_SEL_LIGHT))
				pSubButton->SetStyle(Style & ~TBBS_DISABLED);
			else
				pSubButton->SetStyle(Style | TBBS_DISABLED);
			break;

		case ID_EDIT_SELECTINVERT:
		case ID_EDIT_SELECTBYNAME:
			if (flags & LC_SEL_NO_PIECES)
				pSubButton->SetStyle(Style | TBBS_DISABLED);
			else
				pSubButton->SetStyle(Style & ~TBBS_DISABLED);
			break;

		case ID_EDIT_SELECTNONE:
			if (flags & (LC_SEL_PIECE|LC_SEL_CAMERA|LC_SEL_LIGHT))
				pSubButton->SetStyle(Style & ~TBBS_DISABLED);
			else
				pSubButton->SetStyle(Style | TBBS_DISABLED);
			break;

		case ID_EDIT_SELECTALL:
			if (flags & LC_SEL_UNSELECTED)
				pSubButton->SetStyle(Style & ~TBBS_DISABLED);
			else
				pSubButton->SetStyle(Style | TBBS_DISABLED);
			break;
		};

	}

	EnableToolBarButton(pFrame->m_wndStandardBar, ID_EDIT_CUT, flags & (LC_SEL_PIECE|LC_SEL_CAMERA|LC_SEL_LIGHT) ? TRUE : FALSE);
	EnableToolBarButton(pFrame->m_wndStandardBar, ID_EDIT_COPY, flags & (LC_SEL_PIECE|LC_SEL_CAMERA|LC_SEL_LIGHT) ? TRUE : FALSE);

	CMFCToolBarButton* pPieceButton = MenuBar.GetButton(3);
	CMFCToolBarMenuButton* pPieceMenuButton = DYNAMIC_DOWNCAST(CMFCToolBarMenuButton, pPieceButton);

	const CObList& pieceCommands = pPieceMenuButton->GetCommands();

	for (POSITION pos = pieceCommands.GetHeadPosition (); pos != NULL;)
	{
		CMFCToolBarButton* pSubButton = (CMFCToolBarButton*)pieceCommands.GetNext(pos);
		ASSERT_VALID(pSubButton);

		UINT Style = pSubButton->m_nStyle;

		switch (pSubButton->m_nID)
		{
		case ID_PIECE_DELETE:
		case ID_PIECE_COPYKEYS:
			if (flags & (LC_SEL_PIECE|LC_SEL_CAMERA|LC_SEL_LIGHT))
				pSubButton->SetStyle(Style & ~TBBS_DISABLED);
			else
				pSubButton->SetStyle(Style | TBBS_DISABLED);
			break;

		case ID_PIECE_ARRAY:
		case ID_PIECE_MIRROR:
		case ID_PIECE_HIDESELECTED:
			if (flags & LC_SEL_PIECE)
				pSubButton->SetStyle(Style & ~TBBS_DISABLED);
			else
				pSubButton->SetStyle(Style | TBBS_DISABLED);
			break;

		case ID_PIECE_UNHIDEALL:
			if (flags & LC_SEL_HIDDEN)
				pSubButton->SetStyle(Style & ~TBBS_DISABLED);
			else
				pSubButton->SetStyle(Style | TBBS_DISABLED);
			break;

		case ID_PIECE_HIDEUNSELECTED:
			if (flags & LC_SEL_UNSELECTED)
				pSubButton->SetStyle(Style & ~TBBS_DISABLED);
			else
				pSubButton->SetStyle(Style | TBBS_DISABLED);
			break;

		case ID_PIECE_GROUP:
			if (flags & LC_SEL_CANGROUP)
				pSubButton->SetStyle(Style & ~TBBS_DISABLED);
			else
				pSubButton->SetStyle(Style | TBBS_DISABLED);
			break;

		case ID_PIECE_UNGROUP:
			if (flags & LC_SEL_GROUP)
				pSubButton->SetStyle(Style & ~TBBS_DISABLED);
			else
				pSubButton->SetStyle(Style | TBBS_DISABLED);
			break;

		case ID_PIECE_ATTACH:
			if ((flags & (LC_SEL_GROUP|LC_SEL_FOCUSGROUP)) == LC_SEL_GROUP)
				pSubButton->SetStyle(Style & ~TBBS_DISABLED);
			else
				pSubButton->SetStyle(Style | TBBS_DISABLED);
			break;

		case ID_PIECE_DETACH:
			if (flags & LC_SEL_UNSELECTED)
				pSubButton->SetStyle(Style & ~TBBS_DISABLED);
			else
				pSubButton->SetStyle(Style | TBBS_DISABLED);
			break;

		case ID_PIECE_EDITGROUPS:
			if (flags & LC_SEL_NO_PIECES)
				pSubButton->SetStyle(Style | TBBS_DISABLED);
			else
				pSubButton->SetStyle(Style & ~TBBS_DISABLED);
			break;
		}
	}

	EnableToolBarButton(pFrame->m_wndToolsBar, ID_PIECE_PREVIOUS, flags & LC_SEL_PIECE ? TRUE : FALSE); // FIXME: disable if current step is 1
	EnableToolBarButton(pFrame->m_wndToolsBar, ID_PIECE_NEXT, flags & LC_SEL_PIECE ? TRUE : FALSE);

	// Status bar text.
	if (SelectedCount == 0)
	{
		pFrame->SetStatusBarMessage("");
		pFrame->SetMessageText(AFX_IDS_IDLEMESSAGE);
	}
	else if ((SelectedCount == 1) && (Focus != NULL))
	{
		char Message[256];

		if (Focus->IsPiece())
			sprintf(Message, "%s (ID: %s)", Focus->GetName(), ((Piece*)Focus)->mPieceInfo->m_strName);
		else
			strcpy(Message, Focus->GetName());

		pFrame->SetStatusBarMessage(Message);
		pFrame->SetMessageText(Message);
	}
	else
	{
		char Message[256];
		if (SelectedCount == 1)
			strcpy(Message, "1 Object selected.");
		else
			sprintf(Message, "%d Objects selected.", SelectedCount);

		pFrame->SetStatusBarMessage(Message);
		pFrame->SetMessageText(Message);
	}
}

// Changed current step/frame
void SystemUpdateTime(bool bAnimation, int nTime, int nTotal)
{
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	if (!pFrame)
		return;

	// Status bar
	char szStep[11];
	if (bAnimation)
		sprintf(szStep, "%i/%i", nTime, nTotal);
	else
		sprintf(szStep, " Step %i ", nTime);

	pFrame->m_wndStatusBar.SetPaneText(pFrame->m_wndStatusBar.CommandToIndex(ID_INDICATOR_STEP), LPCSTR(szStep));

	// Choose step dialog
	if (StepModeless != NULL)
		StepModeless->UpdateRange(nTime, nTotal);
}

void SystemUpdateSnap(unsigned short MoveSnap, unsigned short RotateSnap)
{
	char Text[256], xy[32], z[32];

	lcGetActiveProject()->GetSnapDistanceText(xy, z);

	sprintf(Text, " M: %s %s R: %d ", xy, z, RotateSnap);

	if (AfxGetMainWnd())
		((CMainFrame*)AfxGetMainWnd())->SetStatusBarPane(ID_INDICATOR_SNAP, Text);
}

void SystemUpdatePaste(bool enable)
{
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	if (!pFrame)
		return;

	CMFCMenuBar& MenuBar = pFrame->m_wndMenuBar;
	CMFCToolBarButton* pEditButton = MenuBar.GetButton(1);
	CMFCToolBarMenuButton* pEditMenuButton = DYNAMIC_DOWNCAST(CMFCToolBarMenuButton, pEditButton);

	const CObList& editCommands = pEditMenuButton->GetCommands();

	for (POSITION pos = editCommands.GetHeadPosition (); pos != NULL;)
	{
		CMFCToolBarButton* pSubButton = (CMFCToolBarButton*)editCommands.GetNext(pos);
		ASSERT_VALID(pSubButton);

		UINT Style = pSubButton->m_nStyle;

		switch (pSubButton->m_nID)
		{
		case ID_EDIT_PASTE:
			if (enable)
				pSubButton->SetStyle(Style & ~TBBS_DISABLED);
			else
				pSubButton->SetStyle(Style | TBBS_DISABLED);
			break;
		}
	}

	EnableToolBarButton(pFrame->m_wndStandardBar, ID_EDIT_PASTE, enable ? TRUE : FALSE);
}

void SystemUpdatePlay(bool play, bool stop)
{
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();

	EnableToolBarButton(pFrame->m_wndAnimationBar, ID_ANIMATOR_PLAY, play ? TRUE : FALSE);
	EnableToolBarButton(pFrame->m_wndAnimationBar, ID_ANIMATOR_STOP, stop ? TRUE : FALSE);
}

void SystemUpdateAnimation(bool bAnimation, bool bAddKeys)
{
	// Toolbar
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	if (!pFrame)
		return;

	CheckToolBarButton(pFrame->m_wndAnimationBar, ID_ANIMATOR_TOGGLE, bAnimation ? TRUE : FALSE);
	CheckToolBarButton(pFrame->m_wndAnimationBar, ID_ANIMATOR_KEY, bAddKeys ? TRUE : FALSE);
	EnableToolBarButton(pFrame->m_wndAnimationBar, ID_ANIMATOR_PLAY, bAnimation ? TRUE : FALSE);
	EnableToolBarButton(pFrame->m_wndAnimationBar, ID_ANIMATOR_STOP, FALSE);

	// Menu
	char* txt;
	CMenu* pMenu = GetMainMenu(3);
	if (!pMenu)
		return;

	UINT nState = pMenu->GetMenuState(ID_PIECE_COPYKEYS, MF_BYCOMMAND);
	nState &= ~(MF_BITMAP|MF_OWNERDRAW|MF_SEPARATOR);

	if (bAnimation)
		txt = "Copy Keys from Instructions";
	else
		txt = "Copy Keys from Animation";
	
	pMenu->ModifyMenu(ID_PIECE_COPYKEYS, MF_BYCOMMAND | MF_STRING | nState, ID_PIECE_COPYKEYS, txt);
}

void SystemUpdateCurrentCamera(Camera* pOld, Camera* pNew, const PtrArray<Camera>& Cameras)
{
}

void SystemUpdateCameraMenu(const PtrArray<Camera>& Cameras)
{
}

void SystemUpdateCategories(bool SearchOnly)
{
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();

	if (!pFrame)
		return;

	pFrame->m_wndPiecesBar.UpdatePiecesTree(SearchOnly);
}

void SystemUpdateRecentMenu(char names[4][MAX_PATH])
{
	theApp.UpdateMRU(names);
}

// if x = -1, get cursor pos 
void SystemDoPopupMenu(int nMenu, int x, int y)
{
	CMenu PopupMenus;
	PopupMenus.LoadMenu(IDR_POPUPS);

	POINT pt;

	if (x != -1)
	{
		pt.x = x;
		pt.y = y;
	}
	else
		GetCursorPos(&pt);

	CMFCPopupMenu* Popup = new CMFCPopupMenu();
	Popup->Create(AfxGetMainWnd(), pt.x, pt.y, PopupMenus.GetSubMenu(nMenu)->Detach());
}

// Private MFC function only sets the title if it's different
extern void AFXAPI AfxSetWindowText(HWND, LPCTSTR);

void SystemSetWindowCaption(char* caption)
{
	if (!AfxGetMainWnd())
		return;

	AfxSetWindowText(AfxGetMainWnd()->m_hWnd, caption);
}

int SystemDoMessageBox(const char* prompt, int nMode)
{
	return AfxMessageBox(prompt, nMode);
}

int Sys_MessageBox (const char* text, const char* caption, int type)

{

	return AfxMessageBox(text, type);

}



extern BOOL AFXAPI AfxFullPath(LPTSTR lpszPathOut, LPCTSTR lpszFileIn);

bool SystemDoDialog(int nMode, void* param)
{
	switch (nMode)
	{
		case LC_DLG_FILE_OPEN_PROJECT:
		{
			CFileDialog dlg(TRUE, "*.lcd", NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
				"Supported Formats (*.lcd;*.dat;*.ldr;*.mpd)|*.lcd;*.dat;*.ldr;*.mpd|LeoCAD Projects (*.lcd)|*.lcd|LDraw Files (*.dat;*.ldr;*.mpd)|*.dat;*.ldr;*.mpd|All Files (*.*)|*.*||", AfxGetMainWnd());
			dlg.m_ofn.Flags |= (OFN_ENABLETEMPLATE|OFN_HIDEREADONLY|OFN_FILEMUSTEXIST|OFN_ENABLEHOOK|OFN_EXPLORER);
			dlg.m_ofn.hInstance = AfxGetInstanceHandle();
			dlg.m_ofn.lpfnHook = OFNOpenHookProc;
			dlg.m_ofn.lpTemplateName = MAKEINTRESOURCE(IDD_OPENDLG_TEMPLATE);

			char *defdir = (char*)param;
			if (strlen(defdir))
				dlg.m_ofn.lpstrInitialDir = defdir;

			if (dlg.DoModal() == IDOK)
			{
				char szFullPath[LC_MAXPATH];
				AfxFullPath(szFullPath, dlg.GetPathName());
				strcpy((char*)param, szFullPath);
				return true;
			}
		} break;
		
		case LC_DLG_FILE_SAVE_PROJECT:
		{
			CFileDialog dlg(FALSE, "*.lcd", (char*)param, OFN_HIDEREADONLY|OFN_PATHMUSTEXIST|OFN_OVERWRITEPROMPT|OFN_ENABLEHOOK|OFN_ENABLETEMPLATE,
				"LeoCAD Projects (*.lcd)|*.lcd|LDraw Files (*.dat;*.ldr)|*.dat;*.ldr|All Files (*.*)|*.*||");

			dlg.m_ofn.lpfnHook = OFNSaveHookProc;
			dlg.m_ofn.hInstance = AfxGetInstanceHandle();
			dlg.m_ofn.lpTemplateName = MAKEINTRESOURCE(IDD_SAVEDLG_TEMPLATE);

			if (dlg.DoModal() == IDOK)
			{
				char szFullPath[LC_MAXPATH];
				AfxFullPath(szFullPath, dlg.GetPathName());
				strcpy((char*)param, szFullPath);
				return true;
			}
		} break;

		case LC_DLG_FILE_MERGE_PROJECT:
		{
			CFileDialog dlg(TRUE, "*.lcd", NULL, OFN_HIDEREADONLY|OFN_ENABLETEMPLATE|OFN_FILEMUSTEXIST|OFN_ENABLEHOOK|OFN_EXPLORER,
				"LeoCAD Projects (*.lcd)|*.lcd|All Files (*.*)|*.*||", AfxGetMainWnd());
	
			dlg.m_ofn.hInstance = AfxGetInstanceHandle();
			dlg.m_ofn.lpfnHook = OFNOpenHookProc;
			dlg.m_ofn.lpTemplateName = MAKEINTRESOURCE(IDD_OPENDLG_TEMPLATE);
			dlg.m_ofn.lpstrTitle = "Merge";

			char *defdir = (char*)param;
			if (strlen(defdir))
				dlg.m_ofn.lpstrInitialDir = defdir;

			if (dlg.DoModal() == IDOK)
			{
				strcpy((char*)param, dlg.GetPathName());
				return true;
			}
		} break;

		case LC_DLG_FILE_OPEN:
		{
			LC_FILEOPENDLG_OPTS* opts = (LC_FILEOPENDLG_OPTS*)param;

			if (opts->type == LC_FILEOPENDLG_LCF)
			{
				const char *ext, *filter;

				ext = ".lcf";
				filter = "LeoCAD Category Files (*.lcf)|*.lcf|All Files (*.*)|*.*||";

				CFileDialog dlg(TRUE, ext, NULL,OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, filter, NULL);

				if (dlg.DoModal() == IDOK)
				{
					opts->numfiles = 1;
					opts->filenames = (char**)malloc(LC_MAXPATH);
					strcpy((char*)opts->filenames, dlg.GetPathName ());

					// Get the file path.
					strcpy(opts->path, (char*)opts->filenames);
					if (strlen (opts->path) > 0)
					{
						char* ptr = strrchr(opts->path, '/');
						if (ptr == NULL)
							ptr = strrchr(opts->path, '\\');
						if (ptr)
						{
							ptr++;
							*ptr = 0;
						}
					}

					return true;
				}

				return false;
			}

		} break;

		case LC_DLG_FILE_SAVE:
		{
			LC_FILESAVEDLG_OPTS* opts = (LC_FILESAVEDLG_OPTS*)param;

			if (opts->type == LC_FILESAVEDLG_LCF)
			{
				CFileDialog dlg(FALSE, ".lcf", NULL, OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
				                "LeoCAD Category Files (*.lcf)|*.lcf|All Files (*.*)|*.*||", NULL);

				if (dlg.DoModal() == IDOK)
				{
					strcpy(opts->path, dlg.GetPathName ());
					return true;
				}
			}

			return false;
		} break;

		case LC_DLG_PICTURE_SAVE:
		{
			LC_IMAGEDLG_OPTS* opts = (LC_IMAGEDLG_OPTS*)param;
			CImageDlg dlg(FALSE, param);

			if (dlg.DoModal() == IDOK)
				return true;
		} break;

		case LC_DLG_DIRECTORY_BROWSE:
		{
			LC_DLG_DIRECTORY_BROWSE_OPTS* Opts = (LC_DLG_DIRECTORY_BROWSE_OPTS*)param;

			strcpy(Opts->Path, "");

			LPMALLOC ShellMalloc;
			if (SHGetMalloc(&ShellMalloc) == NOERROR)
			{
				BROWSEINFO bi;
				LPITEMIDLIST pidl;
		
				if (AfxGetMainWnd())
					bi.hwndOwner = AfxGetMainWnd()->GetSafeHwnd();
				else
					bi.hwndOwner = ::GetDesktopWindow();
				bi.pidlRoot = NULL;
				bi.pszDisplayName = Opts->Path;
				bi.lpszTitle = Opts->Title;
				bi.ulFlags = BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
				bi.lpfn = NULL;
				bi.lParam = 0;
		
				pidl = SHBrowseForFolder(&bi);
				if (pidl != NULL)
				{
					if (SHGetPathFromIDList(pidl, Opts->Path))
					{ 
						if (Opts->Path[strlen(Opts->Path)-1] != '\\') 
							strcat(Opts->Path, "\\");
						return true;
					}
					ShellMalloc->Free(pidl);
				}
				ShellMalloc->Release();
			}

			return false;

		} break;

		case LC_DLG_HTML:
		{
			LC_HTMLDLG_OPTS* opts = (LC_HTMLDLG_OPTS*)param;
			CHTMLDlg dlg(&opts->imdlg);

			dlg.m_nLayout = opts->singlepage ? 0 : 1;
			dlg.m_bIndex = opts->index;
			dlg.m_bImages = opts->images;
			dlg.m_bListEnd = opts->listend;
			dlg.m_bListStep = opts->liststep;
			dlg.m_bHighlight = opts->highlight;

			if (dlg.DoModal() == IDOK)
			{
				strcpy(opts->path, dlg.m_strFolder);
				opts->singlepage = (dlg.m_nLayout == 0);
				opts->index = dlg.m_bIndex == TRUE;
				opts->images = dlg.m_bImages == TRUE;
				opts->listend = dlg.m_bListEnd == TRUE;
				opts->liststep = dlg.m_bListStep == TRUE;
				opts->highlight = dlg.m_bHighlight == TRUE;
				return true;
			}
		} break;

		case LC_DLG_BRICKLINK:
		{
			CFileDialog dlg(FALSE, "*.xml", NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
				"XML Files (*.xml)|*.xml|All Files (*.*)|*.*||", AfxGetMainWnd());
			if (dlg.DoModal() == IDOK)
			{
				strcpy((char*)param, dlg.GetPathName());
				return true;
			}
		} break;

		case LC_DLG_POVRAY:
		{
			CPOVDlg dlg;
			if (dlg.DoModal() == IDOK)
			{
				LC_POVRAYDLG_OPTS* opts = (LC_POVRAYDLG_OPTS*)param;
				opts->render = dlg.m_bRender != 0;
				strcpy(opts->povpath, dlg.m_strPOV);
				strcpy(opts->outpath, dlg.m_strOut);
				strcpy(opts->libpath, dlg.m_strLGEO);

				return true;
			}
		} break;

		case LC_DLG_WAVEFRONT:
		{
			CFileDialog dlg(FALSE, "*.obj", NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
				"Wavefront Files (*.obj)|*.obj|All Files (*.*)|*.*||", AfxGetMainWnd());
			if (dlg.DoModal() == IDOK)
			{
				strcpy((char*)param, dlg.GetPathName());
				return true;
			}
		} break;

		case LC_DLG_MINIFIG:
		{
			CMinifigDlg dlg(param);

			if (dlg.DoModal() == IDOK)
				return true;
		} break;

		case LC_DLG_ARRAY:
		{
			CArrayDlg dlg;
			if (dlg.DoModal() == IDOK)
			{
				LC_ARRAYDLG_OPTS* opts = (LC_ARRAYDLG_OPTS*)param;

				opts->n1DCount = dlg.m_n1DCount;
				opts->n2DCount = dlg.m_n2DCount;
				opts->n3DCount = dlg.m_n3DCount;
				opts->f2D[0] = dlg.m_f2DX;
				opts->f2D[1] = dlg.m_f2DY;
				opts->f2D[2] = dlg.m_f2DZ;
				opts->f3D[0] = dlg.m_f3DX;
				opts->f3D[1] = dlg.m_f3DY;
				opts->f3D[2] = dlg.m_f3DZ;
				opts->fMove[0] = dlg.m_fMoveX;
				opts->fMove[1] = dlg.m_fMoveY;
				opts->fMove[2] = dlg.m_fMoveZ;
				opts->fRotate[0] = dlg.m_fRotateX;
				opts->fRotate[1] = dlg.m_fRotateY;
				opts->fRotate[2] = dlg.m_fRotateZ;
				opts->nArrayDimension = dlg.m_nArrayDimension;

				return true;
			}
		} break;
	
		case LC_DLG_PREFERENCES:
		{
			CPreferencesSheet ps;
			LC_PREFERENCESDLG_OPTS* opts = (LC_PREFERENCESDLG_OPTS*)param;

			ps.m_PageGeneral.SetOptions(opts->nSaveInterval, opts->nMouse, opts->strPath, opts->strUser);
			ps.m_PageDetail.SetOptions(opts->nDetail, opts->fLineWidth, opts->AASamples);
			ps.m_PageDrawing.SetOptions(opts->nSnap, opts->nAngleSnap, opts->nGridSize);
			ps.m_PageScene.SetOptions(opts->nScene, opts->fDensity, opts->strBackground, opts->fBackground, opts->fFog, opts->fAmbient, opts->fGrad1, opts->fGrad2);
			ps.m_PagePrint.SetOptions(opts->strHeader, opts->strFooter);
			ps.m_PageKeyboard.SetOptions();

			if (ps.DoModal() == IDOK)
			{
				ps.m_PageGeneral.GetOptions(&opts->nSaveInterval, &opts->nMouse, opts->strPath, opts->strUser);
				ps.m_PageDetail.GetOptions(&opts->nDetail, &opts->fLineWidth, &opts->AASamples);
				ps.m_PageDrawing.GetOptions(&opts->nSnap, &opts->nAngleSnap, &opts->nGridSize);
				ps.m_PageScene.GetOptions(&opts->nScene, &opts->fDensity, opts->strBackground, opts->fBackground, opts->fFog, opts->fAmbient, opts->fGrad1, opts->fGrad2);
				ps.m_PagePrint.GetOptions(opts->strHeader, opts->strFooter);
				ps.m_PageKeyboard.GetOptions();
				AfxGetMainWnd()->PostMessage(WM_LC_UPDATE_SETTINGS);
				AfxGetApp()->WriteProfileInt("Settings", "Autosave", opts->nSaveInterval);
				AfxGetApp()->WriteProfileInt("Default", "Mouse", opts->nMouse);
				AfxGetApp()->WriteProfileString("Default", "Projects", opts->strPath);

				if (opts->AASamples != Sys_ProfileLoadInt("Default", "AASamples", 1))
				{
					AfxGetApp()->WriteProfileInt("Default", "AASamples", opts->AASamples);
					AfxMessageBox("Anti-aliasing changes will only take effect next time you start LeoCAD.", MB_OK);
				}

				return true;
			}
		} break;

		case LC_DLG_PROPERTIES:
		{
			CPropertiesSheet ps;
			LC_PROPERTIESDLG_OPTS* opts = (LC_PROPERTIESDLG_OPTS*)param;

			ps.SetTitle(opts->strTitle, PSH_PROPTITLE);
			ps.m_PageSummary.m_strAuthor = opts->strAuthor;
			ps.m_PageSummary.m_strDescription = opts->strDescription;
			ps.m_PageSummary.m_strComments = opts->strComments;
			ps.m_PageGeneral.m_strFilename = opts->strFilename;
			ps.m_PagePieces.mPieceNames = opts->PieceNames;
			ps.m_PagePieces.mNumPieces = opts->NumPieces;
			ps.m_PagePieces.mPieceColorCount = opts->PieceColorCount;
			ps.m_PagePieces.mNumColors = opts->NumColors;

			if (ps.DoModal() == IDOK)
			{
				strcpy(opts->strAuthor, ps.m_PageSummary.m_strAuthor);
				strcpy(opts->strDescription, ps.m_PageSummary.m_strDescription);
				strcpy(opts->strComments, ps.m_PageSummary.m_strComments);

				return true;
			}
		} break;

		case LC_DLG_TERRAIN:
		{
			CTerrainDlg dlg((Terrain*)param, false);
			if (dlg.DoModal() == IDOK)
				return true;
		} break;

		case LC_DLG_LIBRARY:
		{
			CLibraryDlg dlg;
			dlg.DoModal();

			CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
			pFrame->m_wndPiecesBar.UpdatePiecesTree(false);

			return true;
		} break;

		case LC_DLG_SELECTBYNAME:
		{
			CSelectDlg dlg(param);
			if (dlg.DoModal() == IDOK)
				return true;
		} break;

		case LC_DLG_STEPCHOOSE:
		{
			if (StepModeless == NULL)
			{
				CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
				CView* pView = pFrame->GetActiveView();
				StepModeless = new CStepDlg(&StepModeless, pView);
				StepModeless->Create(IDD_STEP, pView);

				int t, l;
				char buf[30];
				strcpy (buf, theApp.GetProfileString("Settings", "Step Dialog"));
				if (sscanf(buf, "%d, %d", &t, &l) == 2)
				{
					CRect rc;
					StepModeless->GetWindowRect(&rc);
					StepModeless->SetWindowPos(NULL, 
						min(l, GetSystemMetrics(SM_CXSCREEN)-rc.Width()),
						min(t, GetSystemMetrics(SM_CYSCREEN)-rc.Height()),
						0, 0, SWP_NOZORDER|SWP_NOSIZE);
				}
				StepModeless->ShowWindow(SW_SHOW);
			}
			else
				StepModeless->SetActiveWindow();
		} break;

		case LC_DLG_EDITGROUPS:
		{
			CEditGroupsDlg dlg((LC_GROUPEDITDLG_OPTS*)param);

			if (dlg.DoModal() == IDOK)
				return true;
		} break;

		case LC_DLG_GROUP:
		{
			CGroupDlg dlg;
			dlg.m_strName = (char*)param;

			if (dlg.DoModal() == IDOK)
			{
				strcpy((char*)param, dlg.m_strName);

				return true;
			}
		} break;

		case LC_DLG_EDITCATEGORY:
		{
			CCategoryDlg Dlg;
			LC_CATEGORYDLG_OPTS* Opts = (LC_CATEGORYDLG_OPTS*)param;

			Dlg.m_Keywords = Opts->Keywords;
			Dlg.m_Name = Opts->Name;

			if (Dlg.DoModal() == IDOK)
			{
				Opts->Keywords = Dlg.m_Keywords;
				Opts->Name = Dlg.m_Name;

				return true;
			}
		} break;

		case LC_DLG_ABOUT:
		{
			CAboutDlg dlg;
			dlg.m_hViewDC = wglGetCurrentDC();
			dlg.DoModal();
		} break;
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////
// Memory rendering functions

typedef struct
{
	HDC hdc;
	HDC oldhdc;
	HGLRC hrc;
	HGLRC oldhrc;
	HBITMAP hbm;
	HBITMAP oldhbm;
} LC_RENDER;

void* Sys_StartMemoryRender(int width, int height)
{
	LC_RENDER* render = (LC_RENDER*)malloc(sizeof(LC_RENDER));
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	CView* pView = pFrame->GetActiveView();
	CDC* pDC = pView->GetDC();
	render->oldhdc = wglGetCurrentDC();
	render->oldhrc = wglGetCurrentContext();
	render->hdc = CreateCompatibleDC(pDC->m_hDC);

	// Preparing bitmap header for DIB section
	BITMAPINFO bi;
	ZeroMemory(&bi, sizeof(BITMAPINFO));
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = width;
	bi.bmiHeader.biHeight = height;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 24;
	bi.bmiHeader.biCompression = BI_RGB;
	bi.bmiHeader.biSizeImage = width * height * 3;
	bi.bmiHeader.biXPelsPerMeter = 2925;
	bi.bmiHeader.biYPelsPerMeter = 2925;

	// Creating a DIB surface
	LPVOID lpBits;
	render->hbm = CreateDIBSection(pDC->GetSafeHdc(), &bi, DIB_RGB_COLORS, (void**)&lpBits, NULL, (DWORD)0);
	render->oldhbm = (HBITMAP)::SelectObject(render->hdc, render->hbm);

	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR), 1, PFD_DRAW_TO_BITMAP|PFD_SUPPORT_OPENGL,
		PFD_TYPE_RGBA, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16,
		0, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 };

	int pixelformat = ChoosePixelFormat(render->hdc, &pfd);
	DescribePixelFormat(render->hdc, pixelformat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
	SetPixelFormat(render->hdc, pixelformat, &pfd);
	render->hrc = wglCreateContext(render->hdc);
	wglMakeCurrent(render->hdc, render->hrc);
	GL_DisableVertexBufferObject();

	return render;
}

void Sys_FinishMemoryRender(void* param)
{
	LC_RENDER* render = (LC_RENDER*)param;

	GL_EnableVertexBufferObject();
	wglMakeCurrent (render->oldhdc, render->oldhrc);
	wglDeleteContext(render->hrc);
	SelectObject(render->hdc, render->oldhbm);
	DeleteObject(render->hbm);
	DeleteDC(render->hdc);
	free(render);
}

/////////////////////////////////////////////////////////////////////////////
// Main window functions

void SystemPieceComboAdd(char* name)
{
	CWnd* pWnd = AfxGetMainWnd();
	if (pWnd != NULL)
		pWnd->PostMessage(WM_LC_ADD_COMBO_STRING, (LPARAM)name);
}

void SystemCaptureMouse()
{
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	CView* pView = pFrame->GetActiveView();
	pView->SetCapture();
}

void SystemReleaseMouse()
{
	ReleaseCapture();
}

void SystemExportClipboard(lcFile* clip)
{
	if (clip == NULL)
		return;

	HGLOBAL hData = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE, clip->GetLength());
	void* lpBuffer = GlobalLock(hData);
	clip->Seek(0, SEEK_SET);
	clip->ReadBuffer(lpBuffer, clip->GetLength());
	GlobalUnlock(hData);

	if (OpenClipboard(NULL))
	{
		SetClipboardData(ClipboardFormat, hData);
		CloseClipboard();
	}
//	else
//		AfxMessageBox(IDS_CANNOT_OPEN_CLIPBOARD);
}

lcFile* SystemImportClipboard()
{
	lcFile* clip = NULL;

	if (ClipboardFormat != 0)
	if (OpenClipboard(NULL))
	{
		HANDLE hData = ::GetClipboardData(ClipboardFormat);
		if (hData != NULL)
		{
			clip = new lcMemFile();

			BYTE* lpBuffer = (BYTE*)::GlobalLock(hData);
			long nBufferSize = ::GlobalSize(hData);
			clip->WriteBuffer(lpBuffer, nBufferSize);
			GlobalUnlock(hData);
		}
//		else
//			AfxMessageBox(IDS_CANNOT_GET_CLIPBOARD_DATA);
		CloseClipboard();
	}
//	else
//		AfxMessageBox(IDS_CANNOT_OPEN_CLIPBOARD);

	return clip;
}

bool Sys_KeyDown(int key)
{
	return GetKeyState(key) < 0;
}

void Sys_GetFileList(const char* Path, ObjArray<String>& FileList)
{
	FileList.RemoveAll();

	WIN32_FIND_DATA FindData;
	HANDLE Find = INVALID_HANDLE_VALUE;
	char Dir[MAX_PATH], FindPath[MAX_PATH];

	strcpy(Dir, Path);
	int Len = strlen(Dir);

	if (Dir[Len-1] != '\\' && Dir[Len-1] != '/')
		strcat(Dir, "\\");

	strcpy(FindPath, Dir);
	strcat(FindPath, "*");

	Find = FindFirstFile(FindPath, &FindData);

	if (Find == INVALID_HANDLE_VALUE) 
		return;

	do
	{
		if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;

		char File[MAX_PATH];
		strcpy(File, Dir);
		strcat(File, FindData.cFileName);
		FileList.Add(File);
	}
	while (FindNextFile(Find, &FindData) != 0);

	FindClose(Find);
}

void SystemPumpMessages()
{
	MSG msg;

	while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

long SystemGetTicks()
{
	return GetTickCount();
}

void SystemStartProgressBar(int nLower, int nUpper, int nStep, const char* Text)
{
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	if (!pFrame)
		return;
	CCADStatusBar* pStatusBar = &pFrame->m_wndStatusBar;

	pStatusBar->ShowProgressBar(TRUE);
	pStatusBar->SetProgressBarRange(nLower, nUpper);
	pStatusBar->SetProgressBarStep(nStep);
	pStatusBar->SetProgressBarPos(0);

	pFrame->SetStatusBarMessage(Text); 
	pFrame->SetMessageText(Text);
}

void SytemEndProgressBar()
{
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	if (!pFrame)
		return;

	pFrame->m_wndStatusBar.ShowProgressBar(FALSE);

	pFrame->SetStatusBarMessage(""); 
	pFrame->SetMessageText(AFX_IDS_IDLEMESSAGE);
}

void SytemStepProgressBar()
{
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	if (!pFrame)
		return;

	pFrame->m_wndStatusBar.StepProgressBar();
}
