#include "lc_global.h"
#include "LeoCAD.h"

#include "MainFrm.h"
#include "CADDoc.h"
#include "CADView.h"
#include <wininet.h>
#include <direct.h>
#include <process.h>
#include <dbghelp.h>
#include "project.h"
#include "globals.h"
#include "system.h"
#include "pieceinf.h"
#include "mainwnd.h"
#include "lc_library.h"
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
			LPSTR szContents = Contents.GetBuffer(dwFileSize+1);
			szContents[dwFileSize] = 0;
			
			if (InternetReadFile(hHttpFile, szContents, dwFileSize, &dwBytesRead))
			{
				int MajorVersion, MinorVersion, PatchVersion;
				int lib;

				if (sscanf(szContents, "%d.%d.%d %d", &MajorVersion, &MinorVersion, &PatchVersion, &lib) == 4)
				{
					CString str;
					bool Update = false;

					if (MajorVersion > LC_VERSION_MAJOR)
						Update = true;
					else if (MajorVersion == LC_VERSION_MAJOR)
					{
						if (MinorVersion > LC_VERSION_MINOR)
							Update = true;
						else if (MinorVersion == LC_VERSION_MINOR)
					{
							if (PatchVersion > LC_VERSION_PATCH)
						Update = true;
					}
					}

					if (Update)
						str.Format("There's a newer version of LeoCAD available for download (%d.%d.%d).\n", MajorVersion, MinorVersion, PatchVersion);
					else
						str = "You are using the latest version of LeoCAD.\n";

					lcPiecesLibrary* Library = lcGetPiecesLibrary();
					if (Library->mNumOfficialPieces)
					{
						if (lib > Library->mNumOfficialPieces)
						{
							str += "There are new pieces available.\n\n";
							Update = true;
						}
						else
							str += "There are no new pieces available at this time.\n";
					}

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

static char MinidumpPath[MAX_PATH];

LONG WINAPI SehHandler(PEXCEPTION_POINTERS pExceptionPtrs)
{ 
	if (IsDebuggerPresent())
		return EXCEPTION_CONTINUE_SEARCH;

	HMODULE hDbgHelp = LoadLibrary("dbghelp.dll");    

	if (hDbgHelp == NULL)
		return EXCEPTION_EXECUTE_HANDLER;

	HANDLE hFile = CreateFile(MinidumpPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
		return EXCEPTION_EXECUTE_HANDLER;

	typedef BOOL (WINAPI *LPMINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD ProcessId, HANDLE hFile, MINIDUMP_TYPE DumpType, CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, CONST PMINIDUMP_USER_STREAM_INFORMATION UserEncoderParam, CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);
	LPMINIDUMPWRITEDUMP pfnMiniDumpWriteDump = (LPMINIDUMPWRITEDUMP)GetProcAddress(hDbgHelp, "MiniDumpWriteDump");
	if (!pfnMiniDumpWriteDump)
		return EXCEPTION_EXECUTE_HANDLER;

	MINIDUMP_EXCEPTION_INFORMATION mei;

    mei.ThreadId = GetCurrentThreadId();
    mei.ExceptionPointers = pExceptionPtrs;
    mei.ClientPointers = TRUE;

	BOOL bWriteDump = pfnMiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, pExceptionPtrs ? &mei : NULL, NULL, NULL);

	CloseHandle(hFile);
	FreeLibrary(hDbgHelp);

	if (bWriteDump)
	{
		char Message[256];
		sprintf(Message, "LeoCAD just crashed. Crash information was saved to the file '%s', please send it to the developers for debugging.", MinidumpPath);

		MessageBox(NULL, Message, "LeoCAD", MB_OK);
	}

	return EXCEPTION_EXECUTE_HANDLER;
}

/////////////////////////////////////////////////////////////////////////////
// CCADApp

BEGIN_MESSAGE_MAP(CCADApp, CWinAppEx)
	//{{AFX_MSG_MAP(CCADApp)
	ON_COMMAND(ID_HELP_CHECKFORUPDATES, OnHelpUpdates)
	ON_COMMAND(ID_HELP_LEOCADHOMEPAGE, OnHelpHomePage)
	ON_COMMAND(ID_HELP_SENDEMAIL, OnHelpEmail)
	//}}AFX_MSG_MAP
	// Standard print setup command
	ON_UPDATE_COMMAND_UI(ID_FILE_MRU_FILE1, OnUpdateRecentFileMenu)
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinAppEx::OnFilePrintSetup)
END_MESSAGE_MAP()

CCADApp::CCADApp()
{
}

CCADApp theApp;

BOOL CCADApp::InitInstance()
{
	if (SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, MinidumpPath) == S_OK)
	{
		PathAppend(MinidumpPath, "LeoCAD\\");
		_mkdir(MinidumpPath);
		_snprintf(MinidumpPath + strlen(MinidumpPath), MAX_PATH, "%s", "minidump.dmp");
	}

	SetUnhandledExceptionFilter(SehHandler);    

	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinAppEx::InitInstance();

	SetRegistryKey(_T("BT Software"));
	LoadStdProfileSettings();

	InitContextMenuManager();
	InitShellManager();
	InitKeyboardManager();
	InitTooltipManager();
	CMFCToolTipInfo ttParams;
	ttParams.m_bVislManagerTheme = TRUE;
	theApp.GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL, RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);

	InitKeyboardShortcuts();

	char ApplicationPath[LC_MAXPATH], *ptr;
	GetModuleFileName (NULL, ApplicationPath, LC_MAXPATH);
	ptr = strrchr(ApplicationPath,'\\');
	if (ptr)
		*(++ptr) = 0;

	char CacheFilePath[LC_MAXPATH];
	if (SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, CacheFilePath) == S_OK)
	{
		PathAppend(CacheFilePath, "LeoCAD\\");
		_mkdir(CacheFilePath);
	}
	else
		CacheFilePath[0] = 0;

	g_App = new lcApplication();
	main_window = new MainWnd();

	if (!g_App->Initialize(__argc, __targv, ApplicationPath, CacheFilePath))
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
	RegisterLeoCADShellFileTypes();

	UINT cmdshow = m_nCmdShow;
	m_nCmdShow = SW_HIDE;
	pDocTemplate->OpenDocumentFile(NULL);

	GL_EnableVertexBufferObject();
	lcPiecesLibrary* Library = lcGetPiecesLibrary();

	if (lcGetActiveProject()->m_pPieces)
	{
		for (int PieceIdx = 0; PieceIdx < Library->mPieces.GetSize(); PieceIdx++)
		{
			lcMesh* Mesh = Library->mPieces[PieceIdx]->mMesh;

			if (Mesh)
				Mesh->UpdateBuffers();
		}
	}

	CMainFrame* MainFrame = (CMainFrame*)AfxGetMainWnd();
	MainFrame->UpdateMenuAccelerators();

	// Show something in the piece preview window.
	PieceInfo* Info = Library->FindPiece("3005", false);
	if (!Info && Library->mPieces.GetSize())
		Info = Library->mPieces[0];

	if (Info)
	{
		lcGetActiveProject()->SetCurrentPiece(Info);
		MainFrame->m_wndPiecesBar.m_wndPiecePreview.SetPieceInfo(Info);
		MainFrame->m_wndPiecesBar.m_wndPiecePreview.PostMessage(WM_PAINT);
	}

/*
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

//	if (cmdInfo.m_strFileName.IsEmpty())
//		ParseCommandLine(cmdInfo);
*/

	// The one and only window has been initialized, so show and update it.
	m_pMainWnd->ShowWindow(cmdshow);
	m_pMainWnd->UpdateWindow();
	lcGetActiveProject()->HandleNotify(LC_ACTIVATE, 1);
	lcGetActiveProject()->UpdateInterface();

	main_window->UpdateMRU ();

	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();

	lcGetActiveProject()->UpdateAllViews();

	int CheckUpdates = AfxGetApp()->GetProfileInt("Settings", "CheckUpdates", 1);
	if (CheckUpdates)
	{
		struct tm When;
		__time64_t Now, Next;

		if (CheckUpdates == 2)
			CheckUpdates = 7;

		memset(&When, 0, sizeof(When));
		CString LastCheck = GetProfileString("Settings", "LastUpdate", NULL);
		sscanf(LastCheck, "%d %d %d", &When.tm_mday, &When.tm_mon, &When.tm_year);
		When.tm_mday = When.tm_mday + CheckUpdates;
		Next = _mktime64(&When);

		_time64(&Now);

		if (Next < Now)
		{
			When = *_localtime64(&Now);
			_beginthread(CheckForUpdates, 0, NULL);
			LastCheck.Format("%d %d %d", When.tm_mday, When.tm_mon, When.tm_year);
			WriteProfileString("Settings", "LastUpdate", LastCheck);
		}
	}

	return TRUE;
}

int CCADApp::ExitInstance() 
{
	delete main_window;
	main_window = NULL;

	g_App->Shutdown();

	delete g_App;
	g_App = NULL;

#ifdef _DEBUG
	if (__hStdOut != NULL)
		FreeConsole();
#endif

	return CWinAppEx::ExitInstance();
}

void CCADApp::UpdateMRU(char names[4][MAX_PATH])
{
	if (m_pRecentFileList)
	{
		for (int iMRU = 0; iMRU < m_pRecentFileList->m_nSize; iMRU++)
			m_pRecentFileList->m_arrNames[iMRU] = names[iMRU];
	}
}

static BOOL SetRegKey(LPCTSTR lpszKey, LPCTSTR lpszValue, LPCTSTR lpszValueName = NULL)
{
	if (lpszValueName == NULL)
	{
		if (AfxRegSetValue(HKEY_CLASSES_ROOT, lpszKey, REG_SZ, lpszValue, lstrlen(lpszValue) * sizeof(TCHAR)) != ERROR_SUCCESS)
		{
			TRACE(traceAppMsg, 0, _T("Warning: registration database update failed for key '%s'.\n"), lpszKey);
			return FALSE;
		}
		return TRUE;
	}
	else
	{
		HKEY hKey;

		if (AfxRegCreateKey(HKEY_CLASSES_ROOT, lpszKey, &hKey) == ERROR_SUCCESS)
		{
			LONG lResult = ::RegSetValueEx(hKey, lpszValueName, 0, REG_SZ, (CONST BYTE*)lpszValue, (lstrlen(lpszValue) + 1) * sizeof(TCHAR));

			if (::RegCloseKey(hKey) == ERROR_SUCCESS && lResult == ERROR_SUCCESS)
				return TRUE;
		}
		TRACE(traceAppMsg, 0, _T("Warning: registration database update failed for key '%s'.\n"), lpszKey);
		return FALSE;
	}
}

void CCADApp::RegisterLeoCADShellFileTypes()
{
	CString strPathName, strTemp;

	AfxGetModuleShortFileName(AfxGetInstanceHandle(), strPathName);

	// first register the type ID of our server
	if (!SetRegKey(_T("LeoCAD.Project"), _T("LeoCAD Project")))
		return;

	// path\DefaultIcon = path,0
	CString strDefaultIconCommandLine = strPathName;
	strDefaultIconCommandLine += _T(",0");
	if (!SetRegKey(_T("LeoCAD.Project\\DefaultIcon"), strDefaultIconCommandLine))
		return;

	// path\shell\open\command = path filename
	CString strOpenCommandLine = strPathName;
	strOpenCommandLine += _T(" \"%1\"");
	if (!SetRegKey(_T("LeoCAD.Project\\shell\\open\\command"), strOpenCommandLine))
		return;

	// path\shell\print\command = path /p filename
	CString strPrintCommandLine = strPathName;
	strPrintCommandLine += _T(" /p \"%1\"");
	if (!SetRegKey(_T("LeoCAD.Project\\shell\\print\\command"), strPrintCommandLine))
		return;

	// path\shell\printto\command = path /pt filename printer driver port
	CString strPrintToCommandLine = strPathName;
	strPrintToCommandLine += _T(" /pt \"%1\" \"%2\" \"%3\" \"%4\"");
	if (!SetRegKey(_T("LeoCAD.Project\\shell\\printto\\command"), strPrintToCommandLine))
		return;

	LONG lSize = _MAX_PATH * 2;
	LONG lResult = AfxRegQueryValue(HKEY_CLASSES_ROOT, _T(".lcd"), strTemp.GetBuffer(lSize), &lSize);
	strTemp.ReleaseBuffer();

	if (lResult != ERROR_SUCCESS || strTemp.IsEmpty() || strTemp == _T("LeoCAD.Project"))
	{
		// no association for that suffix
		if (!SetRegKey(_T(".lcd"), _T("LeoCAD.Project")))
			return;

		SetRegKey(_T(".lcd\\ShellNew"), _T(""), _T("NullFile"));
	}
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
	ShellExecute(::GetDesktopWindow(), _T("open"), _T("mailto:leozide@gmail.com?subject=LeoCAD"), NULL, NULL, SW_NORMAL);
}
