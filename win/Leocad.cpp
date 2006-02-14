// LeoCAD.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "LeoCAD.h"

#include "MainFrm.h"
#include "CADDoc.h"
#include "CADView.h"
#include <wininet.h>
#include <process.h>
#include "project.h"
#include "globals.h"
#include "system.h"
#include "pieceinf.h" // TODO: remove
#include "config.h"
#include "mainwnd.h"
#include "library.h"
#include "keyboard.h"
#include "lc_application.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef _DEBUG
static HANDLE __hStdOut = NULL;

// Use wprintf like TRACE0, TRACE1, ... (The arguments are the same as printf)
void wprintf(char *fmt, ...)
{
	if(!__hStdOut)
	{
		AllocConsole();
		SetConsoleTitle("Debug Window");
		__hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
		COORD co = {80, 25};
		SetConsoleScreenBufferSize(__hStdOut, co);
	}

	char s[300];
	va_list argptr;
	va_start(argptr, fmt);
	vsprintf(s, fmt, argptr);
	va_end(argptr);
	strcat(s, "\n");
	DWORD cCharsWritten;
	WriteConsole(__hStdOut, s, strlen(s), &cCharsWritten, NULL);
}
#endif

// If Data is NULL this function will only display a message if there are updates.
static void CheckForUpdates(void* Data)
{
	HINTERNET session = InternetOpen("LeoCAD", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0) ;
	
	char szSizeBuffer[32];
	DWORD dwLengthSizeBuffer = sizeof(szSizeBuffer); 
	DWORD dwFileSize;
	DWORD dwBytesRead;
	CString Contents;

	HINTERNET hHttpFile = InternetOpenUrl(session, "http://www.leocad.org/updates.txt", NULL, 0, 0, 0);

	if (hHttpFile)
	{
		if(HttpQueryInfo(hHttpFile,HTTP_QUERY_CONTENT_LENGTH, szSizeBuffer, &dwLengthSizeBuffer, NULL))
		{	 
			dwFileSize = atol(szSizeBuffer);
			LPSTR szContents = Contents.GetBuffer(dwFileSize);
			
			if (InternetReadFile(hHttpFile, szContents, dwFileSize, &dwBytesRead))
			{
				float ver;
				int lib;

				if (sscanf (szContents, "%f %d", &ver, &lib) == 2)
				{
					CString str;
					bool Update = false;

					if (ver > LC_VERSION_MAJOR + (float)LC_VERSION_MINOR/100 + (float)LC_VERSION_PATCH/1000)
					{
						str.Format("There's a newer version of LeoCAD available for download (%0.3f).\n", ver);
						Update = true;
					}
					else
						str = "You are using the latest version of LeoCAD.\n";

					if (lib > lcGetPiecesLibrary()->GetPieceCount ())
					{
						str += "There are new pieces available.\n\n";
						Update = true;
					}
					else
						str += "There are no new pieces available at this time.\n";

					if (Data || Update)
					{
						if (Update)
						{
							str += "Would you like to visit the LeoCAD website now?\n";

							if (AfxMessageBox(str, MB_YESNO | MB_ICONQUESTION) == IDYES)
							{
								ShellExecute(::GetDesktopWindow(), _T("open"), _T("http://www.leocad.org"), NULL, NULL, SW_NORMAL); 
							}
						}
						else
							AfxMessageBox(str, MB_OK | MB_ICONINFORMATION);
					}
				}
				else if (Data != NULL)
					AfxMessageBox("Unknown file information.");
			}
			InternetCloseHandle(hHttpFile);
		}
		else if (Data != NULL)
			AfxMessageBox("Could not connect.");
	}
	else if (Data != NULL)
		AfxMessageBox("Could not connect.");

	InternetCloseHandle(session);
}

/////////////////////////////////////////////////////////////////////////////
// CCADApp

BEGIN_MESSAGE_MAP(CCADApp, CWinApp)
	//{{AFX_MSG_MAP(CCADApp)
	ON_COMMAND(ID_HELP_CHECKFORUPDATES, OnHelpUpdates)
	ON_COMMAND(ID_HELP_LEOCADHOMEPAGE, OnHelpHomePage)
	ON_COMMAND(ID_HELP_SENDEMAIL, OnHelpEmail)
	//}}AFX_MSG_MAP
	// Standard print setup command
	ON_UPDATE_COMMAND_UI(ID_FILE_MRU_FILE1, OnUpdateRecentFileMenu)
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCADApp construction

CCADApp::CCADApp()
{
	m_hMutex = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CCADApp object

CCADApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CCADApp initialization

BOOL CCADApp::InitInstance()
{
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#if _MFC_VER < 0x0710
#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif
#endif

	SetRegistryKey(_T("BT Software"));
//	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	InitKeyboardShortcuts();

	char app[LC_MAXPATH], *ptr;
	GetModuleFileName (NULL, app, LC_MAXPATH);
	ptr = strrchr(app,'\\');
	if (ptr)
		*(++ptr) = 0;

	g_App = new lcApplication();
	main_window = new MainWnd();

	if (!g_App->Initialize(__argc, __targv, app))
		return false;

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CCADDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CCADView));
	AddDocTemplate(pDocTemplate);

  EnableShellOpen();
	RegisterShellFileTypes(TRUE);

	UINT cmdshow = m_nCmdShow;
	m_nCmdShow = SW_HIDE;
	pDocTemplate->OpenDocumentFile(NULL);

	// Show something in the piece preview window.
	PieceInfo* Info = lcGetPiecesLibrary()->FindPieceInfo("3005");
	if (!Info)
		Info = lcGetPiecesLibrary()->GetPieceInfo(0);

	if (Info)
	{
		lcGetActiveProject()->SetCurrentPiece(Info);
		((CMainFrame*)(AfxGetMainWnd()))->m_wndPiecesBar.m_wndPiecePreview.SetPieceInfo(Info);
		((CMainFrame*)(AfxGetMainWnd()))->m_wndPiecesBar.m_wndPiecePreview.PostMessage(WM_PAINT);
	}

/*
	m_hMutex = CreateMutex(NULL, FALSE, _T("LeoCAD_Mutex"));
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
//		ParseCommandLine(cmdInfo);
	}
	else
	{
		char out[_MAX_PATH];
		GetTempPath (_MAX_PATH, out);
		strcat (out, "~LC*.lcd");

		WIN32_FIND_DATA fd;
		HANDLE fh = FindFirstFile(out, &fd);
		if (fh != INVALID_HANDLE_VALUE)
		{
			if (char *ptr = strrchr (out, '\\')) *(ptr+1) = 0;
			strcat (out, fd.cFileName);
			if (AfxMessageBox (_T("LeoCAD found a file that was being edited while the program exited unexpectdly. Do you want to load it ?"), MB_YESNO) == IDNO)
			{
				if (AfxMessageBox (_T("Delete file ?"), MB_YESNO) == IDYES)
					DeleteFile (out);
			}
			else
			{
				cmdInfo.m_nShellCommand = CCommandLineInfo::FileOpen;
				cmdInfo.m_strFileName = out;
			}
		}

//		if (cmdInfo.m_strFileName.IsEmpty())
//			ParseCommandLine(cmdInfo);
	}
*/

	// The one and only window has been initialized, so show and update it.
	int status = theApp.GetProfileInt("Settings", "Window Status", -1);
	if (status != -1)
		m_pMainWnd->ShowWindow(status);
	else
		m_pMainWnd->ShowWindow(cmdshow);
	m_pMainWnd->UpdateWindow();
	lcGetActiveProject()->HandleNotify(LC_ACTIVATE, 1);
	lcGetActiveProject()->UpdateInterface();

  main_window->UpdateMRU ();

	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();

  lcGetActiveProject()->UpdateAllViews (NULL);

	if (AfxGetApp()->GetProfileInt("Settings", "CheckUpdates", 1))
		_beginthread(CheckForUpdates, 0, NULL);

  return TRUE;
}

int CCADApp::ExitInstance() 
{
	if (m_hMutex != NULL)
		ReleaseMutex(m_hMutex);

	delete main_window;
	main_window = NULL;

	g_App->Shutdown();

	delete g_App;
	g_App = NULL;

#ifdef _DEBUG
	if (__hStdOut != NULL)
		FreeConsole();
#endif

	return CWinApp::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// CCADApp commands

void CCADApp::OnHelpUpdates() 
{
	CheckForUpdates(this);
}

void CCADApp::OnHelpHomePage() 
{
	ShellExecute(::GetDesktopWindow(), _T("open"), _T("http://www.leocad.org"), NULL, NULL, SW_NORMAL); 
}

void CCADApp::OnHelpEmail() 
{
	ShellExecute(::GetDesktopWindow(), _T("open"), _T("mailto:leonardo@centroin.com.br?subject=LeoCAD"), NULL, NULL, SW_NORMAL);
}
