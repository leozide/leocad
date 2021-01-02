#include "lc_global.h"
#include "lc_application.h"
#include "lc_qupdatedialog.h"
#include "lc_profile.h"
#include <QApplication>
#include <locale.h>

#ifdef Q_OS_WIN

#pragma warning(push)
#pragma warning(disable : 4091)
#include <dbghelp.h>
#include <direct.h>
#include <shlobj.h>
#pragma warning(pop)

#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE
#endif
#endif

#include <tchar.h>

static TCHAR gMinidumpPath[_MAX_PATH];

static LONG WINAPI lcSehHandler(PEXCEPTION_POINTERS exceptionPointers)
{ 
	if (IsDebuggerPresent())
		return EXCEPTION_CONTINUE_SEARCH;

	HMODULE dbgHelp = LoadLibrary(TEXT("dbghelp.dll"));

	if (dbgHelp == nullptr)
		return EXCEPTION_EXECUTE_HANDLER;

	HANDLE file = CreateFile(gMinidumpPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

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

	BOOL writeDump = miniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, MiniDumpNormal, exceptionPointers ? &mei : nullptr, nullptr, nullptr);

	CloseHandle(file);
	FreeLibrary(dbgHelp);

	if (writeDump)
	{
		TCHAR message[_MAX_PATH + 256];
		lstrcpy(message, TEXT("LeoCAD just crashed. Crash information was saved to the file '"));
		lstrcat(message, gMinidumpPath);
		lstrcat(message, TEXT("', please send it to the developers for debugging."));

		MessageBox(nullptr, message, TEXT("LeoCAD"), MB_OK);
	}

	return EXCEPTION_EXECUTE_HANDLER;
}

static void lcSehInit()
{
	if (GetTempPath(LC_ARRAY_COUNT(gMinidumpPath), gMinidumpPath))
		lstrcat(gMinidumpPath, TEXT("leocad.dmp"));

	SetUnhandledExceptionFilter(lcSehHandler);
}

static void lcRegisterShellFileTypes()
{
	TCHAR modulePath[_MAX_PATH], longModulePath[_MAX_PATH];
	TCHAR temp[2*_MAX_PATH];

	GetModuleFileName(nullptr, longModulePath, _MAX_PATH);
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

		if (RegCreateKeyEx(HKEY_CLASSES_ROOT, TEXT(".lcd\\ShellNew"), 0, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_READ, nullptr, &key, &disposition) != ERROR_SUCCESS)
			return;

		result = RegSetValueEx(key, TEXT("NullFile"), 0, REG_SZ, (CONST BYTE*)TEXT(""), (lstrlen(TEXT("")) + 1) * sizeof(TCHAR));

		if (RegCloseKey(key) != ERROR_SUCCESS || result != ERROR_SUCCESS)
			return;
	}
}

#endif

int main(int argc, char *argv[])
{
#ifdef LC_USE_QOPENGLWIDGET
	QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
#endif

	lcApplication Application(argc, argv);

	QString Language = lcGetProfileString(LC_PROFILE_LANGUAGE);
	QLocale Locale;

	if (!Language.isEmpty())
		Locale = QLocale(Language);

	QTranslator QtTranslator;
	if (QtTranslator.load(Locale, "qt", "_", QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
		Application.installTranslator(&QtTranslator);
#ifdef Q_OS_WIN
	else if (QtTranslator.load(Locale, "qt", "_", qApp->applicationDirPath() + "/translations"))
		Application.installTranslator(&QtTranslator);
#endif

	QTranslator QtBaseTranslator;
	if (QtBaseTranslator.load("qtbase_" + Locale.name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
		Application.installTranslator(&QtBaseTranslator);
#ifdef Q_OS_WIN
	else if (QtBaseTranslator.load("qtbase_" + Locale.name(), qApp->applicationDirPath() + "/translations"))
		Application.installTranslator(&QtBaseTranslator);
#endif

	QTranslator Translator;
	if (Translator.load("leocad_" + Locale.name(), ":/resources"))
		Application.installTranslator(&Translator);

	qRegisterMetaTypeStreamOperators<QList<int> >("QList<int>");

	QList<QPair<QString, bool>> LibraryPaths;

#ifdef Q_OS_WIN
	lcRegisterShellFileTypes();
	lcSehInit();

	LibraryPaths += qMakePair(QDir::cleanPath(QCoreApplication::applicationDirPath() + "/library.bin"), true);
#endif

#ifdef Q_OS_LINUX
	LibraryPaths += qMakePair(QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../share/leocad/library.bin"), true);
#endif

#ifdef Q_OS_MAC
	LibraryPaths += qMakePair(QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../../Contents/Resources/library.bin"), true);
#endif

#ifdef LC_LDRAW_LIBRARY_PATH
	LibraryPaths += qMakePair(QString::fromLatin1(LC_LDRAW_LIBRARY_PATH), false);
#endif
	
	setlocale(LC_NUMERIC, "C");

	lcStartupMode StartupMode = Application.Initialize(LibraryPaths);

	if (StartupMode == lcStartupMode::Error)
		return 1;

	int ExecReturn = 0;

	if (StartupMode == lcStartupMode::ShowWindow)
	{
#if !LC_DISABLE_UPDATE_CHECK
		lcDoInitialUpdateCheck();
#endif

		ExecReturn = Application.exec();
	}

	Application.Shutdown();

	return ExecReturn;
}
