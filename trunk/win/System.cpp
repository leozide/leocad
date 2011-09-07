// System user interface.
//

#include "stdafx.h"
#include <dlgs.h>
#include <direct.h>
#include "leocad.h"
#include "bmpmenu.h"
#include "system.h"
#include "defines.h"
#include "camera.h"
#include "tools.h"
#include "file.h"
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


static CMenu menuPopups;
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
	CWnd* pFrame = AfxGetMainWnd();

	if (pFrame == NULL)
		return NULL;

	CMenu* pMenu = pFrame->GetMenu();

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
				FileDisk file;
				file.Open(filename, "rb");
				file.Read(id, 32);
				sscanf(strchr(id, ' '), "%f", &fv);

				if (fv > 0.4f)
				{
					file.Read(&fv, 4);

					if (fv > 0.7f)
					{
						unsigned long dwPosition;
						file.Seek(-4, SEEK_END);
						file.Read(&dwPosition, 4);
						file.Seek(dwPosition, SEEK_SET);

						if (dwPosition != 0)
						{
							if (fv < 1.0f)
							{
								BITMAPFILEHEADER bmfHeader;
								file.Read((LPSTR)&bmfHeader, sizeof(bmfHeader));
								DWORD nPackedDIBLen = sizeof(BITMAPINFOHEADER) + 36000;
								HGLOBAL hDIB = ::GlobalAlloc(GMEM_FIXED, nPackedDIBLen);
								file.Read((LPSTR)hDIB, nPackedDIBLen);
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
				file.Close();

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

static UINT APIENTRY OFNSavePictureHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uiMsg)
	{
		case WM_INITDIALOG:
		{
			OPENFILENAME* ofn = (OPENFILENAME*)lParam;
			SetWindowLong(hdlg, GWL_USERDATA, ofn->lCustData);
		} break;

		case WM_COMMAND:
		{
			if (wParam == IDC_SAVEPICTURE_OPTIONS)
			{
				LC_IMAGEDLG_OPTS* opts = (LC_IMAGEDLG_OPTS*)GetWindowLong(hdlg, GWL_USERDATA);
				CImageDlg dlg(FALSE, opts);
				dlg.DoModal();
			}
		} break;

		case WM_NOTIFY: 
		{
			LPNMHDR pnmh = (LPNMHDR) lParam;
			if (pnmh->code == CDN_FILEOK)
			{
				// This avoids an assert
				_AFX_THREAD_STATE* pThreadState = AfxGetThreadState();
				pThreadState->m_pAlternateWndInit = NULL;
				return FALSE;
			}
		}
	}
	return FALSE;
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

	menuPopups.LoadMenu(IDR_POPUPS);

	// attempt to load special bitmap, else default to arrow
	CSize size = GetMenuCheckMarkDimensions();
	ASSERT(size.cx > 4 && size.cy > 5); // not too small please
	if (size.cx > 32)
		size.cx = 32;
	int iwRow = (size.cx + 15) >> 4;    // # of WORDs per raster line
	int nShift = (size.cx - DOT_WIDTH) / 2;     // # of bits to shift over
	nShift += ((iwRow * 16) - size.cx); // padding for word alignment
	if (nShift > 16 - DOT_WIDTH)
		nShift = 16 - DOT_WIDTH;    // maximum shift for 1 word

	if (size.cy > 32)
		size.cy = 32;

	// bitmap 2/4/4/4/2 pixels wide - centered (0 => black)
	BYTE rgbBitmap[32 * 2 * sizeof(WORD)];
	memset(rgbBitmap, 0xff, sizeof(rgbBitmap));

	BYTE* pbOut = &rgbBitmap[iwRow * sizeof(WORD) *
							((size.cy - (DOT_HEIGHT+1)) >> 1)];
	const BYTE* pbIn = rgbDot;
	for (int y = 0; y < DOT_HEIGHT; y++)
	{
		WORD w = (WORD)~(((DWORD)*pbIn++) << nShift);
		// bitmaps are always hi-lo
		pbOut[0] = HIBYTE(w);
		pbOut[1] = LOBYTE(w);
		pbOut += iwRow * sizeof(WORD);
	}

	hbmMenuDot = CreateBitmap(size.cx, size.cy, 1, 1, (LPVOID)&rgbBitmap);
	if (hbmMenuDot == NULL)
	{
		#define OBM_MNARROW         32739
		hbmMenuDot = LoadBitmap(NULL, MAKEINTRESOURCE(OBM_MNARROW));
	}
}

// Viewport menu.
void SystemUpdateViewport(int nNew, int nOld)
{
	CMenu* pMenu = GetMainMenu(2);
	if (!pMenu)
		return;
	pMenu = pMenu->GetSubMenu(13);
	pMenu->CheckMenuItem(nOld + ID_VIEWPORT01, MF_BYCOMMAND | MF_UNCHECKED);
	pMenu->CheckMenuItem(nNew + ID_VIEWPORT01, MF_BYCOMMAND | MF_CHECKED);
}

// Action toolbar, popup menu and cursor.
void SystemUpdateAction(int nNew, int nOld)
{
	CFrameWnd* pFrame = (CFrameWnd*)AfxGetMainWnd();
	if (!pFrame)
		return;
	CToolBar* pBar = (CToolBar*)pFrame->GetControlBar(ID_VIEW_TOOLS_BAR);
	CToolBarCtrl* pCtrl = &pBar->GetToolBarCtrl();
	CView* pView = pFrame->GetActiveView();

	pCtrl->CheckButton(ID_ACTION_SELECT+nOld, FALSE);
	pCtrl->CheckButton(ID_ACTION_SELECT+nNew, TRUE);

	// TODO: make sure this works if loading a file from the cmd line.
	if (pView)
		pView->SendMessage(WM_LC_SET_CURSOR, nNew);

	// TODO: update popup context menu
	// TODO: disable lights if count > 8
}

// Current color in the listbox;
void SystemUpdateColorList(int nNew)
{
	if (AfxGetMainWnd())
		AfxGetMainWnd()->PostMessage (WM_LC_UPDATE_LIST, 0, nNew+1);
}

void SystemUpdateRenderingMode(bool bBackground, bool bFast)
{
	CFrameWnd* pFrame = (CFrameWnd*)AfxGetMainWnd();
	if (!pFrame)
		return;
	CToolBar* pBar = (CToolBar*)pFrame->GetControlBar(AFX_IDW_TOOLBAR);
	CToolBarCtrl* pCtrl = &pBar->GetToolBarCtrl();

//	if (bFast)
//	{
//		pCtrl->EnableButton(ID_RENDER_BACKGROUND, TRUE);
//		pCtrl->CheckButton(ID_RENDER_BACKGROUND, bBackground);
//	}
//	else
	{
		pCtrl->CheckButton(ID_RENDER_BACKGROUND, FALSE);
		pCtrl->EnableButton(ID_RENDER_BACKGROUND, FALSE);
	}

	pCtrl->CheckButton(ID_RENDER_BOX, bFast);
}

void SystemUpdateUndoRedo(char* undo, char* redo)
{
	CFrameWnd* pFrame = (CFrameWnd*)AfxGetMainWnd();
	if (!pFrame)
		return;
	CToolBar* pBar = (CToolBar*)pFrame->GetControlBar(AFX_IDW_TOOLBAR);
	CToolBarCtrl* pCtrl = &pBar->GetToolBarCtrl();
	CMenu* pMenu = GetMainMenu(1);
	char txt[50];
	UINT nState;

	if (pMenu == NULL)
		return;

	strcpy(txt, "Undo ");
	if (undo != NULL)
		strcat(txt, undo);
	strcat(txt, "\tCtrl+Z");

	nState = pMenu->GetMenuState(ID_EDIT_UNDO, MF_BYCOMMAND);
	nState &= ~(MF_BITMAP|MF_OWNERDRAW|MF_SEPARATOR);
	pMenu->ModifyMenu(ID_EDIT_UNDO, MF_BYCOMMAND |
        MF_STRING | nState, ID_EDIT_UNDO, txt);

	strcpy(txt, "Redo ");
	if (redo != NULL)
		strcat(txt, redo);
	strcat(txt, "\tCtrl+Y");

	nState = pMenu->GetMenuState(ID_EDIT_REDO, MF_BYCOMMAND);
	nState &= ~(MF_BITMAP|MF_OWNERDRAW|MF_SEPARATOR);
	pMenu->ModifyMenu(ID_EDIT_REDO, MF_BYCOMMAND |
        MF_STRING | nState, ID_EDIT_REDO, txt);

	pMenu->EnableMenuItem(ID_EDIT_UNDO, MF_BYCOMMAND | 
		(undo ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	pMenu->EnableMenuItem(ID_EDIT_REDO, MF_BYCOMMAND | 
		(redo ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));

	pCtrl->EnableButton(ID_EDIT_UNDO, undo ? TRUE : FALSE);
	pCtrl->EnableButton(ID_EDIT_REDO, redo ? TRUE : FALSE);
}

// Snap menu & toolbar icon
void SystemUpdateSnap(const unsigned long nSnap)
{
	CFrameWnd* pFrame = (CFrameWnd*)AfxGetMainWnd();
	if (!pFrame)
		return;
	CToolBar* pBar = (CToolBar*)pFrame->GetControlBar(AFX_IDW_TOOLBAR);
	CToolBarCtrl* pCtrl = &pBar->GetToolBarCtrl();
	pCtrl->CheckButton(ID_SNAP_ANGLE, (nSnap & LC_DRAW_SNAP_A) != 0);

	CMenu* pMenu = menuPopups.GetSubMenu(2);
	pMenu->CheckMenuItem(ID_SNAP_SNAPX, MF_BYCOMMAND | 
		(nSnap & LC_DRAW_SNAP_X ? MF_CHECKED : MF_UNCHECKED));
	pMenu->CheckMenuItem(ID_SNAP_SNAPY, MF_BYCOMMAND | 
		(nSnap & LC_DRAW_SNAP_Y ? MF_CHECKED : MF_UNCHECKED));
	pMenu->CheckMenuItem(ID_SNAP_SNAPZ, MF_BYCOMMAND | 
		(nSnap & LC_DRAW_SNAP_Z ? MF_CHECKED : MF_UNCHECKED));

	pMenu = menuPopups.GetSubMenu(8);
	pMenu->CheckMenuItem(ID_LOCK_LOCKX, MF_BYCOMMAND | 
		(nSnap & LC_DRAW_LOCK_X ? MF_CHECKED : MF_UNCHECKED));
	pMenu->CheckMenuItem(ID_LOCK_LOCKY, MF_BYCOMMAND | 
		(nSnap & LC_DRAW_LOCK_Y ? MF_CHECKED : MF_UNCHECKED));
	pMenu->CheckMenuItem(ID_LOCK_LOCKZ, MF_BYCOMMAND | 
		(nSnap & LC_DRAW_LOCK_Z ? MF_CHECKED : MF_UNCHECKED));

	SetMenuItemBitmaps(pMenu->m_hMenu, ID_LOCK_2BUTTONS, MF_BYCOMMAND, NULL, hbmMenuDot);
	SetMenuItemBitmaps(pMenu->m_hMenu, ID_LOCK_3DMOVEMENT, MF_BYCOMMAND, NULL, hbmMenuDot);

	// TODO: change Snap None & All (or maybe not ?)
}

void SystemUpdateSelected(unsigned long flags, int SelectedCount, Object* Focus)
{
	CMenu* pMenu;
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	if (!pFrame)
		return;
	CToolBar* pBar = (CToolBar*)pFrame->GetControlBar(AFX_IDW_TOOLBAR);
	CToolBarCtrl* pCtrl = &pBar->GetToolBarCtrl();

	// select all/none/invert/by name
	pMenu = GetMainMenu(1);
	if (flags & LC_SEL_NO_PIECES)
	{
		pMenu->EnableMenuItem(ID_EDIT_SELECTINVERT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		pMenu->EnableMenuItem(ID_EDIT_SELECTBYNAME, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	}
	else
	{
		pMenu->EnableMenuItem(ID_EDIT_SELECTINVERT, MF_BYCOMMAND | MF_ENABLED);
		pMenu->EnableMenuItem(ID_EDIT_SELECTBYNAME, MF_BYCOMMAND | MF_ENABLED);
	}
	pMenu->EnableMenuItem(ID_EDIT_SELECTNONE, MF_BYCOMMAND | 
		(flags & (LC_SEL_PIECE|LC_SEL_CAMERA|LC_SEL_LIGHT) ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	pMenu->EnableMenuItem(ID_EDIT_SELECTALL, MF_BYCOMMAND | 
		(flags & LC_SEL_UNSELECTED ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));

	// cut, copy
	pMenu->EnableMenuItem(ID_EDIT_CUT, MF_BYCOMMAND | 
		(flags & (LC_SEL_PIECE|LC_SEL_CAMERA|LC_SEL_LIGHT) ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	pMenu->EnableMenuItem(ID_EDIT_COPY, MF_BYCOMMAND | 
		(flags & (LC_SEL_PIECE|LC_SEL_CAMERA|LC_SEL_LIGHT) ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	pCtrl->EnableButton(ID_EDIT_CUT, flags & (LC_SEL_PIECE|LC_SEL_CAMERA|LC_SEL_LIGHT) ? TRUE : FALSE);
	pCtrl->EnableButton(ID_EDIT_COPY, flags & (LC_SEL_PIECE|LC_SEL_CAMERA|LC_SEL_LIGHT) ? TRUE : FALSE);

	// mirror/array, hide sel/unsel, unhideall
	pBar = (CToolBar*)pFrame->GetControlBar(ID_VIEW_TOOLS_BAR);
	pCtrl = &pBar->GetToolBarCtrl();
	pCtrl->EnableButton(ID_PIECE_ARRAY, flags & LC_SEL_PIECE ? TRUE : FALSE);
	pCtrl->EnableButton(ID_PIECE_MIRROR, flags & LC_SEL_PIECE ? TRUE : FALSE);
	pMenu = GetMainMenu(3);
	pMenu->EnableMenuItem(ID_PIECE_DELETE, MF_BYCOMMAND | 
		(flags & (LC_SEL_PIECE|LC_SEL_CAMERA|LC_SEL_LIGHT) ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	pMenu->EnableMenuItem(ID_PIECE_COPYKEYS, MF_BYCOMMAND | 
		(flags & (LC_SEL_PIECE|LC_SEL_CAMERA|LC_SEL_LIGHT) ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	pMenu->EnableMenuItem(ID_PIECE_ARRAY, MF_BYCOMMAND | 
		(flags & LC_SEL_PIECE ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	pMenu->EnableMenuItem(ID_PIECE_MIRROR, MF_BYCOMMAND | 
		(flags & LC_SEL_PIECE ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	pMenu->EnableMenuItem(ID_PIECE_UNHIDEALL, MF_BYCOMMAND | 
		(flags & LC_SEL_HIDDEN ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	pMenu->EnableMenuItem(ID_PIECE_HIDESELECTED, MF_BYCOMMAND | 
		(flags & LC_SEL_PIECE ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	pMenu->EnableMenuItem(ID_PIECE_HIDEUNSELECTED, MF_BYCOMMAND | 
		(flags & LC_SEL_UNSELECTED ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));

	// group
	pMenu->EnableMenuItem(ID_PIECE_GROUP, MF_BYCOMMAND | 
		(flags & LC_SEL_CANGROUP ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	pMenu->EnableMenuItem(ID_PIECE_UNGROUP, MF_BYCOMMAND | 
		(flags & LC_SEL_GROUP ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	pMenu->EnableMenuItem(ID_PIECE_ATTACH, MF_BYCOMMAND | 
		((flags & (LC_SEL_GROUP|LC_SEL_FOCUSGROUP)) == (LC_SEL_GROUP) ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	pMenu->EnableMenuItem(ID_PIECE_DETACH, MF_BYCOMMAND | 
		(flags & LC_SEL_FOCUSGROUP ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	pMenu->EnableMenuItem(ID_PIECE_EDITGROUPS, MF_BYCOMMAND | 
		((flags & LC_SEL_NO_PIECES) == 0 ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));

	pCtrl->EnableButton(ID_PIECE_PREVIOUS, flags & LC_SEL_PIECE ? TRUE : FALSE);
	pCtrl->EnableButton(ID_PIECE_NEXT, flags & LC_SEL_PIECE ? TRUE : FALSE);

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
			sprintf(Message, "%s (ID: %s)", Focus->GetName(), ((Piece*)Focus)->GetPieceInfo()->m_strName);
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
	// Toolbar
	CFrameWnd* pFrame = (CFrameWnd*)AfxGetMainWnd();
	if (!pFrame)
		return;
	CToolBar* pBar = (CToolBar*)pFrame->GetControlBar(ID_VIEW_ANIMATION_BAR);
	CToolBarCtrl* pCtrl = &pBar->GetToolBarCtrl();

	pCtrl->EnableButton(ID_VIEW_STEP_NEXT, nTime < nTotal ? TRUE : FALSE);
	pCtrl->EnableButton(ID_VIEW_STEP_PREVIOUS, nTime > 1 ? TRUE : FALSE);
	pCtrl->EnableButton(ID_VIEW_STEP_FIRST, nTime != 1 ? TRUE : FALSE);
	pCtrl->EnableButton(ID_VIEW_STEP_LAST, nTime != nTotal ? TRUE : FALSE);

	// Main menu
	CBMPMenu* pMainMenu = (CBMPMenu*)GetMainMenu(2)->GetSubMenu(14);

	pMainMenu->EnableMenuItem(ID_VIEW_STEP_NEXT, MF_BYCOMMAND | 
		(nTime < nTotal ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	pMainMenu->EnableMenuItem(ID_VIEW_STEP_PREVIOUS, MF_BYCOMMAND | 
		(nTime > 1 ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	pMainMenu->EnableMenuItem(ID_VIEW_STEP_FIRST, MF_BYCOMMAND | 
		(nTime != 1 ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	pMainMenu->EnableMenuItem(ID_VIEW_STEP_LAST, MF_BYCOMMAND | 
		(nTime != nTotal ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));

	// Status bar
	char szStep[11];
	CStatusBar* pStatusBar = (CStatusBar*)pFrame->GetControlBar(AFX_IDW_STATUS_BAR);

	if (bAnimation)
		sprintf(szStep, "%i/%i", nTime, nTotal);
	else
		sprintf(szStep, " Step %i ", nTime);

	pStatusBar->SetPaneText(pStatusBar->CommandToIndex(ID_INDICATOR_STEP), LPCSTR(szStep));

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
	CMenu* pMenu = GetMainMenu(1);
	CFrameWnd* pFrame = (CFrameWnd*)AfxGetMainWnd();
	CToolBar* pBar = (CToolBar*)pFrame->GetControlBar(AFX_IDW_TOOLBAR);
	CToolBarCtrl* pCtrl = &pBar->GetToolBarCtrl();

	if (pMenu != NULL)
		pMenu->EnableMenuItem(ID_EDIT_PASTE, MF_BYCOMMAND | (enable ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));

	if (pCtrl)
		pCtrl->EnableButton(ID_EDIT_PASTE, enable ? TRUE : FALSE);
}

void SystemUpdatePlay(bool play, bool stop)
{
	CFrameWnd* pFrame = (CFrameWnd*)AfxGetMainWnd();
	CToolBar* pBar = (CToolBar*)pFrame->GetControlBar(ID_VIEW_ANIMATION_BAR);
	CToolBarCtrl* pCtrl = &pBar->GetToolBarCtrl();

	pCtrl->EnableButton(ID_ANIMATOR_PLAY, play ? TRUE : FALSE);
	pCtrl->EnableButton(ID_ANIMATOR_STOP, stop ? TRUE : FALSE);
}

void SystemUpdateAnimation(bool bAnimation, bool bAddKeys)
{
	// Toolbar
	CFrameWnd* pFrame = (CFrameWnd*)AfxGetMainWnd();
	if (!pFrame)
		return;
	CToolBar* pBar = (CToolBar*)pFrame->GetControlBar(ID_VIEW_ANIMATION_BAR);
	CToolBarCtrl* pCtrl = &pBar->GetToolBarCtrl();

	pCtrl->CheckButton(ID_ANIMATOR_TOGGLE, bAnimation ? TRUE : FALSE);
	pCtrl->CheckButton(ID_ANIMATOR_KEY, bAddKeys ? TRUE : FALSE);
	pCtrl->EnableButton(ID_ANIMATOR_PLAY, bAnimation ? TRUE : FALSE);
	pCtrl->EnableButton(ID_ANIMATOR_STOP, FALSE);

	// Menu
	char* txt;
	CMenu* pMenu = GetMainMenu(3);
	UINT nState = pMenu->GetMenuState(ID_PIECE_COPYKEYS, MF_BYCOMMAND);
	nState &= ~(MF_BITMAP|MF_OWNERDRAW|MF_SEPARATOR);

	if (bAnimation)
		txt = "Copy Keys from Instructions";
	else
		txt = "Copy Keys from Animation";
	
	pMenu->ModifyMenu(ID_PIECE_COPYKEYS, MF_BYCOMMAND |
        MF_STRING | nState, ID_PIECE_COPYKEYS, txt);
}

void SystemUpdateCurrentCamera(Camera* pOld, Camera* pNew, Camera* pCamera)
{
	CMenu* Menu = GetMainMenu(2);
	if (!Menu)
		return;
	CBMPMenu* pMainMenu = (CBMPMenu*)Menu->GetSubMenu(13);
	CMenu* pPopupMenu = menuPopups.GetSubMenu(1)->GetSubMenu(3);
	int i;

	for (i = 0; pCamera; i++, pCamera = pCamera->m_pNext)
	{
		if (pOld == pCamera)
		{
			pPopupMenu->CheckMenuItem(i + ID_CAMERA_FIRST, MF_BYCOMMAND | MF_UNCHECKED);
			pMainMenu->CheckMenuItem(i + ID_CAMERA_FIRST, MF_BYCOMMAND | MF_UNCHECKED);
		}

		if (pNew == pCamera)
		{
			pPopupMenu->CheckMenuItem(i + ID_CAMERA_FIRST, MF_BYCOMMAND | MF_CHECKED);
			pMainMenu->CheckMenuItem(i + ID_CAMERA_FIRST, MF_BYCOMMAND | MF_CHECKED);
			SetMenuItemBitmaps(pPopupMenu->m_hMenu, i + ID_CAMERA_FIRST, MF_BYCOMMAND, NULL, hbmMenuDot);
			SetMenuItemBitmaps(pMainMenu->m_hMenu, i + ID_CAMERA_FIRST, MF_BYCOMMAND, NULL, hbmMenuDot);
		}
	}
}

// Update the list of cameras
void SystemUpdateCameraMenu(Camera* pCamera)
{
	CMenu* Menu = GetMainMenu(2);
	if (!Menu)
		return;
	CBMPMenu* pMainMenu = (CBMPMenu*)Menu->GetSubMenu(13);
	CMenu* pPopupMenu = menuPopups.GetSubMenu(1)->GetSubMenu(3);
	Camera* pFirst = pCamera;
	int i;

	while (pMainMenu->GetMenuItemCount())
		pMainMenu->DeleteMenu(0, MF_BYPOSITION);
	while (pPopupMenu->GetMenuItemCount())
		pPopupMenu->DeleteMenu(0, MF_BYPOSITION);

	for (i = 0; pCamera; i++, pCamera = pCamera->m_pNext)
		if (i > 6)
		{
			pMainMenu->AppendODMenu(pCamera->GetName(), MF_ENABLED, i + ID_CAMERA_FIRST);
			pPopupMenu->AppendMenu(MF_STRING, i + ID_CAMERA_FIRST, pCamera->GetName());
		}

	if (i > 7)
	{
		pMainMenu->AppendODMenu("", MF_SEPARATOR);
		pPopupMenu->AppendMenu(MF_SEPARATOR);
	}

	pCamera = pFirst;
	for (i = 0; pCamera && (i < 7); i++, pCamera = pCamera->m_pNext)
	{
		pMainMenu->AppendODMenu(pCamera->GetName(), MF_ENABLED, i + ID_CAMERA_FIRST);
		pPopupMenu->AppendMenu(MF_STRING, i + ID_CAMERA_FIRST, pCamera->GetName());

		pMainMenu->ChangeMenuItemShortcut("str", i + ID_CAMERA_FIRST);
	}

	pMainMenu->AppendODMenu("", MF_SEPARATOR);
	pPopupMenu->AppendMenu(MF_SEPARATOR);
	pMainMenu->AppendODMenu("Reset", MF_ENABLED, ID_VIEW_CAMERAS_RESET);
	pPopupMenu->AppendMenu(MF_STRING, ID_VIEW_CAMERAS_RESET, "Reset");
//	pMainMenu->AppendODMenu("Adjust...\t", MF_ENABLED, ID_VIEW_VIEWPOINT);
//	pPopupMenu->AppendODMenu("Adjust...\t", MF_ENABLED, ID_VIEW_VIEWPOINT);

  ((CMainFrame*)AfxGetMainWnd())->UpdateMenuAccelerators(); 
}

void SystemUpdateCategories(bool SearchOnly)
{
	CFrameWnd* pFrame = (CFrameWnd*)AfxGetMainWnd();

	if (!pFrame)
		return;

	CPiecesBar* pBar = (CPiecesBar*)pFrame->GetControlBar(ID_VIEW_PIECES_BAR);
	pBar->UpdatePiecesTree(SearchOnly);
}

extern UINT AFXAPI AfxGetFileTitle(LPCTSTR lpszPathName, LPTSTR lpszTitle, UINT nMax);
extern UINT AFXAPI AfxGetFileName(LPCTSTR lpszPathName, LPTSTR lpszTitle, UINT nMax);

/////////////////////////////////////////////////////////////////////////////
// lpszCanon = C:\MYAPP\DEBUGS\C\TESWIN.C
//
// cchMax   b   Result
// ------   -   ---------
//  1- 7    F   <empty>
//  1- 7    T   TESWIN.C
//  8-14    x   TESWIN.C
// 15-16    x   C:\...\TESWIN.C
// 17-23    x   C:\...\C\TESWIN.C
// 24-25    x   C:\...\DEBUGS\C\TESWIN.C
// 26+      x   C:\MYAPP\DEBUGS\C\TESWIN.C

#ifndef _MAC
static void AbbreviateName(LPTSTR lpszCanon, int cchMax, BOOL bAtLeastName)
{
	int cchFullPath, cchFileName, cchVolName;
	const TCHAR* lpszCur;
	const TCHAR* lpszBase;
	const TCHAR* lpszFileName;

	lpszBase = lpszCanon;
	cchFullPath = lstrlen(lpszCanon);

	cchFileName = AfxGetFileName(lpszCanon, NULL, 0) - 1;
	lpszFileName = lpszBase + (cchFullPath-cchFileName);

	// If cchMax is more than enough to hold the full path name, we're done.
	// This is probably a pretty common case, so we'll put it first.
	if (cchMax >= cchFullPath)
		return;

	// If cchMax isn't enough to hold at least the basename, we're done
	if (cchMax < cchFileName)
	{
		lstrcpy(lpszCanon, (bAtLeastName) ? lpszFileName : _T(""));
		return;
	}

	// Calculate the length of the volume name.  Normally, this is two characters
	// (e.g., "C:", "D:", etc.), but for a UNC name, it could be more (e.g.,
	// "\\server\share").
	//
	// If cchMax isn't enough to hold at least <volume_name>\...\<base_name>, the
	// result is the base filename.

	lpszCur = lpszBase + 2;                 // Skip "C:" or leading "\\"

	if (lpszBase[0] == '\\' && lpszBase[1] == '\\') // UNC pathname
	{
		// First skip to the '\' between the server name and the share name,
		while (*lpszCur != '\\')
		{
			lpszCur = _tcsinc(lpszCur);
			ASSERT(*lpszCur != '\0');
		}
	}
	// if a UNC get the share name, if a drive get at least one directory
	ASSERT(*lpszCur == '\\');
	// make sure there is another directory, not just c:\filename.ext
	if (cchFullPath - cchFileName > 3)
	{
		lpszCur = _tcsinc(lpszCur);
		while (*lpszCur != '\\')
		{
			lpszCur = _tcsinc(lpszCur);
			ASSERT(*lpszCur != '\0');
		}
	}
	ASSERT(*lpszCur == '\\');

	cchVolName = lpszCur - lpszBase;
	if (cchMax < cchVolName + 5 + cchFileName)
	{
		lstrcpy(lpszCanon, lpszFileName);
		return;
	}

	// Now loop through the remaining directory components until something
	// of the form <volume_name>\...\<one_or_more_dirs>\<base_name> fits.
	//
	// Assert that the whole filename doesn't fit -- this should have been
	// handled earlier.

	ASSERT(cchVolName + (int)lstrlen(lpszCur) > cchMax);
	while (cchVolName + 4 + (int)lstrlen(lpszCur) > cchMax)
	{
		do
		{
			lpszCur = _tcsinc(lpszCur);
			ASSERT(*lpszCur != '\0');
		}
		while (*lpszCur != '\\');
	}

	// Form the resultant string and we're done.
	lpszCanon[cchVolName] = '\0';
	lstrcat(lpszCanon, _T("\\..."));
	lstrcat(lpszCanon, lpszCur);
}
#endif

static BOOL GetDisplayName(char* filename, CString& strName, LPCTSTR lpszCurDir, int nCurDir, BOOL bAtLeastName)
{
	LPTSTR lpch = strName.GetBuffer(_MAX_PATH);
#ifndef _MAC
	lstrcpy(lpch, filename);
	// nLenDir is the length of the directory part of the full path
	int nLenDir = lstrlen(lpch) - (AfxGetFileName(lpch, NULL, 0) - 1);
	BOOL bSameDir = FALSE;
	if (nLenDir == nCurDir)
	{
		TCHAR chSave = lpch[nLenDir];
		lpch[nCurDir] = 0;  // terminate at same location as current dir
		bSameDir = lstrcmpi(lpszCurDir, lpch) == 0;
		lpch[nLenDir] = chSave;
	}
	// copy the full path, otherwise abbreviate the name
	if (bSameDir)
	{
		// copy file name only since directories are same
		char szTemp[_MAX_PATH];
		AfxGetFileTitle(lpch+nCurDir, szTemp, sizeof(szTemp));
		lstrcpyn(lpch, szTemp, _MAX_PATH);
	}
	else
	{
		// strip the extension if the system calls for it
		char szTemp[_MAX_PATH];
		AfxGetFileTitle(lpch+nLenDir, szTemp, sizeof(szTemp));
		lstrcpyn(lpch+nLenDir, szTemp, _MAX_PATH-nLenDir);

		// abbreviate name based on what will fit in limited space
		AbbreviateName(lpch, 30, bAtLeastName);
	}
#else
	// for Mac just show the file title name without path
	AfxGetFileTitle(filename, lpch, _MAX_PATH);
#endif
	strName.ReleaseBuffer();
	return TRUE;
}

void SystemUpdateRecentMenu(char names[4][LC_MAXPATH])
{
	CBMPMenu* pMenu = (CBMPMenu*)GetMainMenu(0);
	if (!pMenu)
		return;
	UINT nState;

	pMenu->DeleteMenu(ID_FILE_MRU_FILE2, MF_BYCOMMAND);
	pMenu->DeleteMenu(ID_FILE_MRU_FILE3, MF_BYCOMMAND);
	pMenu->DeleteMenu(ID_FILE_MRU_FILE4, MF_BYCOMMAND);

	if (strlen(names[0]) == 0)
	{
		nState = pMenu->GetMenuState(ID_FILE_MRU_FILE1, MF_BYCOMMAND);
		nState &= ~(MF_BITMAP|MF_OWNERDRAW|MF_SEPARATOR);
		pMenu->ModifyMenu(ID_FILE_MRU_FILE1, MF_BYCOMMAND |
		    MF_STRING | nState, ID_FILE_MRU_FILE1, "Recent File");
		pMenu->EnableMenuItem(ID_FILE_MRU_FILE1, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		return;
	}

#ifndef _MAC
	TCHAR szCurDir[_MAX_PATH];
	GetCurrentDirectory(_MAX_PATH, szCurDir);
	int nCurDir = lstrlen(szCurDir);
	ASSERT(nCurDir >= 0);
	szCurDir[nCurDir] = '\\';
	szCurDir[++nCurDir] = '\0';
#endif

	CString strName;
	CString strTemp;
	for (int i = 0; i < 4; i++)
	{
		if (strlen(names[i]) == 0)
			break;
		if (!GetDisplayName(names[i], strName, szCurDir, nCurDir, TRUE))
			break;

		// double up any '&' characters so they are not underlined
		LPCTSTR lpszSrc = strName;
		LPTSTR lpszDest = strTemp.GetBuffer(strName.GetLength()*2);
		while (*lpszSrc != 0)
		{
			if (*lpszSrc == '&')
				*lpszDest++ = '&';
			if (_istlead(*lpszSrc))
				*lpszDest++ = *lpszSrc++;
			*lpszDest++ = *lpszSrc++;
		}
		*lpszDest = 0;
		strTemp.ReleaseBuffer();

		// insert mnemonic + the file name
		char buf[200];
		sprintf(buf, "&%d %s", i+1, strTemp);

		if (i != 0)
		{
			UINT x = pMenu->GetMenuItemCount() - 2;
			pMenu->InsertMenu(x, MF_BYPOSITION|MF_STRING, ID_FILE_MRU_FILE1 + i, buf);
		}
		else
		{
			nState = pMenu->GetMenuState(ID_FILE_MRU_FILE1, MF_BYCOMMAND);
			nState &= ~(MF_BITMAP|MF_OWNERDRAW|MF_SEPARATOR);
			pMenu->ModifyMenu(ID_FILE_MRU_FILE1, MF_BYCOMMAND |
			    MF_STRING | nState, ID_FILE_MRU_FILE1 + i, buf);
			pMenu->EnableMenuItem(ID_FILE_MRU_FILE1, MF_BYCOMMAND | MF_ENABLED);
		}
	}
}

// if x = -1, get cursor pos 
void SystemDoPopupMenu(int nMenu, int x, int y)
{
	CMenu* pPopup;
	POINT pt;

	if (x != -1)
	{
		pt.x = x;
		pt.y = y;
	}
	else
		GetCursorPos(&pt);

	pPopup = menuPopups.GetSubMenu(nMenu);
	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, pt.x, pt.y, AfxGetMainWnd());
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
				"LeoCAD Projects (*.lcd)|*.lcd|LDraw Files (*.dat;*.ldr;*.mpd)|*.dat;*.ldr;*.mpd|All Files (*.*)|*.*||", AfxGetMainWnd());
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

			if (opts->type == LC_FILEOPENDLG_DAT)
			{
				CString filename;

				CFileDialog dlg(TRUE, ".dat\0", NULL,OFN_ALLOWMULTISELECT | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_FILEMUSTEXIST,
					"LDraw Files (*.dat)|*.dat|All Files (*.*)|*.*||",NULL);
				dlg.m_ofn.lpstrFile = filename.GetBuffer(_MAX_PATH * 32);
		    dlg.m_ofn.nMaxFile = _MAX_PATH;
				dlg.m_ofn.lpstrInitialDir = opts->path;

	      if (dlg.DoModal() == IDOK)
				{
		      POSITION pos = dlg.GetStartPosition ();
					int count = 0;

		      while (pos != NULL)
					{
						dlg.GetNextPathName (pos);
						count++;
					}

					opts->filenames = (char**)malloc(count*sizeof(char*));
					opts->numfiles = count;

					pos = dlg.GetStartPosition ();
					count = 0;

		      while (pos != NULL)
				  {
		        CString str = dlg.GetNextPathName (pos);
						opts->filenames[count] = (char*)malloc(LC_MAXPATH);
						strcpy (opts->filenames[count], str);
						count++;
					}

					// Get the file path.
					strcpy (opts->path, opts->filenames[0]);
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
			else
			{
				const char *ext, *filter;

				if (opts->type == LC_FILEOPENDLG_LCF)
				{
					ext = ".lcf\0";
					filter = "LeoCAD Category Files (*.lcf)|*.lcf|All Files (*.*)|*.*||";
				}
				else
				{
					ext = ".lup\0";
					filter = "LeoCAD Library Updates (*.lup)|*.lup|All Files (*.*)|*.*||";
				}

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
			CFileDialog dlg(FALSE, NULL, NULL, OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_ENABLEHOOK|OFN_ENABLETEMPLATE|OFN_EXPLORER,
				"GIF Files (*.gif)|*.gif|JPEG Files (*.jpg;*.jpeg)|*.jpg;*.jpeg|Bitmap Files (*.bmp)|*.bmp|PNG Files (*.png)|*.png|AVI Files (*.avi)|*.avi|All Files (*.*)|*.*||");

			DWORD dwImage = theApp.GetProfileInt ("Default", "Image Options", 1|LC_IMAGE_TRANSPARENT);
			LC_IMAGEDLG_OPTS* opts = (LC_IMAGEDLG_OPTS*)param;
			opts->width = theApp.GetProfileInt("Default", "Image Width", GetSystemMetrics(SM_CXSCREEN));
			opts->height = theApp.GetProfileInt("Default", "Image Height", GetSystemMetrics(SM_CYSCREEN));
			opts->imopts.quality = theApp.GetProfileInt("Default", "JPEG Quality", 70);
			opts->imopts.interlaced = (dwImage & LC_IMAGE_PROGRESSIVE) != 0;
			opts->imopts.transparent = (dwImage & LC_IMAGE_TRANSPARENT) != 0;
			opts->imopts.truecolor = (dwImage & LC_IMAGE_HIGHCOLOR) != 0;
			opts->imopts.pause = (float)theApp.GetProfileInt("Default", "AVI Pause", 100)/100;
			opts->imopts.format = (unsigned char)(dwImage & ~(LC_IMAGE_MASK));

			dlg.m_ofn.hInstance = AfxGetInstanceHandle();
			dlg.m_ofn.lpTemplateName = MAKEINTRESOURCE(IDD_SAVEPICTUREDLG_TEMPLATE);
			dlg.m_ofn.lpfnHook = OFNSavePictureHookProc;
			dlg.m_ofn.lCustData= (LONG)opts;

			if (dlg.DoModal() == IDOK)
			{
				strcpy(opts->filename, (LPCSTR)dlg.GetPathName());
				char ext[5];
				if (strlen(opts->filename) == 0)
					return false;
				char *p = strrchr(opts->filename, '.');
				if (p != NULL)
				{
					strcpy(ext, p+1);
					strlwr(ext);

					if ((strcmp(ext, "jpg") == 0) || (strcmp(ext, "jpeg") == 0) ||
						(strcmp(ext, "bmp") == 0) || (strcmp(ext, "gif") == 0) ||
						(strcmp(ext, "png") == 0) || (strcmp(ext, "avi") == 0))
						return true;
				}

				switch (opts->imopts.format)
				{
				case LC_IMAGE_BMP: strcat(opts->filename, ".bmp"); break;
				case LC_IMAGE_GIF: strcat(opts->filename, ".gif"); break;
				case LC_IMAGE_JPG: strcat(opts->filename, ".jpg"); break;
				case LC_IMAGE_PNG: strcat(opts->filename, ".png"); break;
				case LC_IMAGE_AVI: strcat(opts->filename, ".avi"); break;
				}

				return true;
			}
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
      dlg.m_bHtmlExt = opts->htmlext;

      if (dlg.DoModal() == IDOK)
			{
				strcpy(opts->path, dlg.m_strFolder);
				opts->singlepage = (dlg.m_nLayout == 0);
				opts->index = dlg.m_bIndex == TRUE;
				opts->images = dlg.m_bImages == TRUE;
				opts->listend = dlg.m_bListEnd == TRUE;
				opts->liststep = dlg.m_bListStep == TRUE;
				opts->highlight = dlg.m_bHighlight == TRUE;
        opts->htmlext = dlg.m_bHtmlExt == TRUE;
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
			ps.m_PageDetail.SetOptions(opts->nDetail, opts->fLineWidth);
			ps.m_PageDrawing.SetOptions(opts->nSnap, opts->nAngleSnap, opts->nGridSize);
			ps.m_PageScene.SetOptions(opts->nScene, opts->fDensity, opts->strBackground, opts->fBackground, opts->fFog, opts->fAmbient, opts->fGrad1, opts->fGrad2);
			ps.m_PagePrint.SetOptions(opts->strHeader, opts->strFooter);
			ps.m_PageKeyboard.SetOptions();

			if (ps.DoModal() == IDOK)
			{
				ps.m_PageGeneral.GetOptions(&opts->nSaveInterval, &opts->nMouse, opts->strPath, opts->strUser);
				ps.m_PageDetail.GetOptions(&opts->nDetail, &opts->fLineWidth);
				ps.m_PageDrawing.GetOptions(&opts->nSnap, &opts->nAngleSnap, &opts->nGridSize);
				ps.m_PageScene.GetOptions(&opts->nScene, &opts->fDensity, opts->strBackground, opts->fBackground, opts->fFog, opts->fAmbient, opts->fGrad1, opts->fGrad2);
				ps.m_PagePrint.GetOptions(opts->strHeader, opts->strFooter);
				ps.m_PageKeyboard.GetOptions();
				AfxGetMainWnd()->PostMessage(WM_LC_UPDATE_SETTINGS);
				AfxGetApp()->WriteProfileInt("Settings", "Autosave", opts->nSaveInterval);
				AfxGetApp()->WriteProfileInt("Default", "Mouse", opts->nMouse);
				AfxGetApp()->WriteProfileString("Default", "Projects", opts->strPath);

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
			ps.m_PagePieces.names = opts->names;
			ps.m_PagePieces.count = opts->count;
			ps.m_PagePieces.lines = opts->lines;

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

			CPiecesBar* pBar = (CPiecesBar*)((CFrameWnd*)AfxGetMainWnd())->GetControlBar(ID_VIEW_PIECES_BAR);
			pBar->UpdatePiecesTree(false);

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
				CFrameWnd* pFrame = (CFrameWnd*)AfxGetMainWnd();
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
			dlg.m_hViewDC = pfnwglGetCurrentDC();
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
	CFrameWnd* pFrame = (CFrameWnd*)AfxGetMainWnd();
	CView* pView = pFrame->GetActiveView();
	CDC* pDC = pView->GetDC();
	render->oldhdc = pfnwglGetCurrentDC();
	render->oldhrc = pfnwglGetCurrentContext();
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

	int pixelformat = OpenGLChoosePixelFormat(render->hdc, &pfd);
	OpenGLDescribePixelFormat(render->hdc, pixelformat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
	OpenGLSetPixelFormat(render->hdc, pixelformat, &pfd);
  render->hrc = pfnwglCreateContext(render->hdc);
	pfnwglMakeCurrent(render->hdc, render->hrc);

	return render;
}

void Sys_FinishMemoryRender(void* param)
{
	LC_RENDER* render = (LC_RENDER*)param;

	pfnwglMakeCurrent (render->oldhdc, render->oldhrc);
	pfnwglDeleteContext(render->hrc);
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
	CFrameWnd* pFrame = (CFrameWnd*)AfxGetMainWnd();
	CView* pView = pFrame->GetActiveView();
	pView->SetCapture();
}

void SystemReleaseMouse()
{
	ReleaseCapture();
}

void SystemExportClipboard(File* clip)
{
	if (clip == NULL)
		return;

	HGLOBAL hData = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE, clip->GetLength());
	void* lpBuffer = GlobalLock(hData);
	clip->Seek(0, SEEK_SET);
	clip->Read(lpBuffer, clip->GetLength());
	GlobalUnlock(hData);

	if (OpenClipboard(NULL))
	{
		SetClipboardData(ClipboardFormat, hData);
		CloseClipboard();
	}
//	else
//		AfxMessageBox(IDS_CANNOT_OPEN_CLIPBOARD);
}

File* SystemImportClipboard()
{
	File* clip = NULL;

	if (ClipboardFormat != 0)
	if (OpenClipboard(NULL))
	{
		HANDLE hData = ::GetClipboardData(ClipboardFormat);
		if (hData != NULL)
		{
			clip = new FileMem();

			BYTE* lpBuffer = (BYTE*)::GlobalLock(hData);
			long nBufferSize = ::GlobalSize(hData);
			clip->Write(lpBuffer, nBufferSize);
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

bool Sys_KeyDown (int key)
{
  return GetKeyState (KEY_CONTROL) < 0;
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
	CFrameWnd* pFrame = (CFrameWnd*)AfxGetMainWnd();
	if (!pFrame)
		return;
	CCADStatusBar* pStatusBar = (CCADStatusBar*)pFrame->GetControlBar(AFX_IDW_STATUS_BAR);

	pStatusBar->ShowProgressBar(TRUE);
	pStatusBar->SetProgressBarRange(nLower, nUpper);
	pStatusBar->SetProgressBarStep(nStep);
	pStatusBar->SetProgressBarPos(0);

  ((CMainFrame*)AfxGetMainWnd())->SetStatusBarMessage(Text); 
	((CMainFrame*)AfxGetMainWnd())->SetMessageText(Text);
}

void SytemEndProgressBar()
{
	CFrameWnd* pFrame = (CFrameWnd*)AfxGetMainWnd();
	if (!pFrame)
		return;
	CCADStatusBar* pStatusBar = (CCADStatusBar*)pFrame->GetControlBar(AFX_IDW_STATUS_BAR);

	pStatusBar->ShowProgressBar(FALSE);

  ((CMainFrame*)AfxGetMainWnd())->SetStatusBarMessage(""); 
	((CMainFrame*)AfxGetMainWnd())->SetMessageText(AFX_IDS_IDLEMESSAGE);
}

void SytemStepProgressBar()
{
	CFrameWnd* pFrame = (CFrameWnd*)AfxGetMainWnd();
	if (!pFrame)
		return;
	CCADStatusBar* pStatusBar = (CCADStatusBar*)pFrame->GetControlBar(AFX_IDW_STATUS_BAR);

	pStatusBar->StepProgressBar();
}
