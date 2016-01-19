#include "lc_global.h"
#include "lc_application.h"
#include "lc_qupdatedialog.h"
#include "lc_mainwindow.h"
#include "view.h"
#include "project.h"
#include "lc_colors.h"
#include <QApplication>

#ifdef Q_OS_WIN

#include <dbghelp.h>
#include <direct.h>
#include <shlobj.h>

#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE
#endif
#endif

#include <tchar.h>

static TCHAR minidumpPath[_MAX_PATH];

static LONG WINAPI lcSehHandler(PEXCEPTION_POINTERS exceptionPointers)
{ 
	if (IsDebuggerPresent())
		return EXCEPTION_CONTINUE_SEARCH;

	HMODULE dbgHelp = LoadLibrary(TEXT("dbghelp.dll"));

	if (dbgHelp == NULL)
		return EXCEPTION_EXECUTE_HANDLER;

	HANDLE file = CreateFile(minidumpPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (file == INVALID_HANDLE_VALUE)
		return EXCEPTION_EXECUTE_HANDLER;

	typedef BOOL (WINAPI *LPMINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD ProcessId, HANDLE hFile, MINIDUMP_TYPE DumpType, CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, CONST PMINIDUMP_USER_STREAM_INFORMATION UserEncoderParam, CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);
	LPMINIDUMPWRITEDUMP miniDumpWriteDump = (LPMINIDUMPWRITEDUMP)GetProcAddress(dbgHelp, "MiniDumpWriteDump");
	if (!miniDumpWriteDump)
		return EXCEPTION_EXECUTE_HANDLER;

	MINIDUMP_EXCEPTION_INFORMATION mei;

	mei.ThreadId = GetCurrentThreadId();
	mei.ExceptionPointers = exceptionPointers;
	mei.ClientPointers = TRUE;

	BOOL writeDump = miniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, MiniDumpNormal, exceptionPointers ? &mei : NULL, NULL, NULL);

	CloseHandle(file);
	FreeLibrary(dbgHelp);

	if (writeDump)
	{
		TCHAR message[_MAX_PATH + 256];
		lstrcpy(message, TEXT("LeoCAD just crashed. Crash information was saved to the file '"));
		lstrcat(message, minidumpPath);
		lstrcat(message, TEXT("', please send it to the developers for debugging."));

		MessageBox(NULL, message, TEXT("LeoCAD"), MB_OK);
	}

	return EXCEPTION_EXECUTE_HANDLER;
}

static void lcSehInit()
{
	if (SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, minidumpPath) == S_OK)
	{
		lstrcat(minidumpPath, TEXT("\\LeoCAD\\"));
		_tmkdir(minidumpPath);
		lstrcat(minidumpPath, TEXT("minidump.dmp"));
	}

	SetUnhandledExceptionFilter(lcSehHandler);
}

static void lcRegisterShellFileTypes()
{
	TCHAR modulePath[_MAX_PATH], longModulePath[_MAX_PATH];
	TCHAR temp[2*_MAX_PATH];

	GetModuleFileName(NULL, longModulePath, _MAX_PATH);
	if (GetShortPathName(longModulePath, modulePath, _MAX_PATH) == 0)
		lstrcpy(modulePath, longModulePath);

	if (RegSetValue(HKEY_CLASSES_ROOT, TEXT("LeoCAD.Project"), REG_SZ, TEXT("LeoCAD Project"), lstrlen(TEXT("LeoCAD Project")) * sizeof(TCHAR)) != ERROR_SUCCESS)
		return;

	lstrcpy(temp, modulePath);
	lstrcat(temp, TEXT(",0"));
	if (RegSetValue(HKEY_CLASSES_ROOT, TEXT("LeoCAD.Project\\DefaultIcon"), REG_SZ, temp, lstrlen(temp) * sizeof(TCHAR)) != ERROR_SUCCESS)
		return;

	lstrcpy(temp, modulePath);
	lstrcat(temp, TEXT(" \"%1\""));
	if (RegSetValue(HKEY_CLASSES_ROOT, TEXT("LeoCAD.Project\\shell\\open\\command"), REG_SZ, temp, lstrlen(temp) * sizeof(TCHAR)) != ERROR_SUCCESS)
		return;

	LONG size = 2 * _MAX_PATH;
	LONG result = RegQueryValue(HKEY_CLASSES_ROOT, TEXT(".lcd"), temp, &size);

	if (result != ERROR_SUCCESS || !lstrlen(temp) || lstrcmp(temp, TEXT("LeoCAD.Project")))
	{
		if (RegSetValue(HKEY_CLASSES_ROOT, TEXT(".lcd"), REG_SZ, TEXT("LeoCAD.Project"), lstrlen(TEXT("LeoCAD.Project")) * sizeof(TCHAR)) != ERROR_SUCCESS)
			return;

		HKEY key;
		DWORD disposition = 0;

		if (RegCreateKeyEx(HKEY_CLASSES_ROOT, TEXT(".lcd\\ShellNew"), 0, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_READ, NULL, &key, &disposition) != ERROR_SUCCESS)
			return;

		LONG result = RegSetValueEx(key, TEXT("NullFile"), 0, REG_SZ, (CONST BYTE*)TEXT(""), (lstrlen(TEXT("")) + 1) * sizeof(TCHAR));

		if (RegCloseKey(key) != ERROR_SUCCESS || result != ERROR_SUCCESS)
			return;
	}
}

#endif

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	app.setApplicationDisplayName("LeoCAD");

	QCoreApplication::setOrganizationDomain("leocad.org");
	QCoreApplication::setOrganizationName("LeoCAD Software");
	QCoreApplication::setApplicationName("LeoCAD");
	QCoreApplication::setApplicationVersion(LC_VERSION_TEXT);

	QTranslator Translator;
	Translator.load(QString("leocad_") + QLocale::system().name().section('_', 0, 0) + ".qm", ":/resources");
	app.installTranslator(&Translator);

	g_App = new lcApplication();

#if defined(Q_OS_WIN)
	char libPath[LC_MAXPATH], *ptr;
	strcpy(libPath, argv[0]);
	ptr = strrchr(libPath,'\\');
	if (ptr)
		*(++ptr) = 0;

	lcRegisterShellFileTypes();
	lcSehInit();
#elif defined(Q_OS_MAC)
	QDir bundlePath = QDir(QCoreApplication::applicationDirPath());
	bundlePath.cdUp();
	bundlePath.cdUp();
	bundlePath = QDir::cleanPath(bundlePath.absolutePath() + "/Contents/Resources/");
	QByteArray pathArray = bundlePath.absolutePath().toLocal8Bit();
	const char* libPath = pathArray.data();
#else
	const char* libPath = LC_INSTALL_PREFIX "/share/leocad/";
#endif

#ifdef LC_LDRAW_LIBRARY_PATH
	const char* LDrawPath = LC_LDRAW_LIBRARY_PATH;
#else
	const char* LDrawPath = NULL;
#endif

	if (!g_App->Initialize(argc, argv, libPath, LDrawPath))
		return 1;

	gMainWindow->SetColorIndex(lcGetColorIndex(4));
	gMainWindow->UpdateRecentFiles();
	gMainWindow->show();

#if !LC_DISABLE_UPDATE_CHECK
	lcDoInitialUpdateCheck();
#endif

	int execReturn = app.exec();

	delete gMainWindow;
	gMainWindow = NULL;
	delete g_App;
	g_App = NULL;

	return execReturn;
}
