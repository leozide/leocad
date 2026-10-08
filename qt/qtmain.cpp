#include "lc_global.h"
#include "lc_application.h"
#include "lc_updatedialog.h"
#include "lc_profile.h"
#include "pieceinf.h"
#include <QApplication>
#include <locale.h>

#ifdef Q_OS_WIN

#pragma warning(push)
#pragma warning(disable : 4091)
#include <windows.h>
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

static LONG WINAPI lcSehHandler(PEXCEPTION_POINTERS ExceptionPointers)
{
	if (IsDebuggerPresent())
		return EXCEPTION_CONTINUE_SEARCH;

	HMODULE DbgHelp = LoadLibrary(TEXT("dbghelp.dll"));

	if (DbgHelp == nullptr)
		return EXCEPTION_EXECUTE_HANDLER;

	HANDLE File = CreateFile(gMinidumpPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (File == INVALID_HANDLE_VALUE)
		return EXCEPTION_EXECUTE_HANDLER;

	typedef BOOL (WINAPI *LPMINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD ProcessId, HANDLE hFile, MINIDUMP_TYPE DumpType, CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, CONST PMINIDUMP_USER_STREAM_INFORMATION UserEncoderParam, CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);
	LPMINIDUMPWRITEDUMP MiniDumpWriteDump = (LPMINIDUMPWRITEDUMP)GetProcAddress(DbgHelp, "MiniDumpWriteDump");
	if (!MiniDumpWriteDump)
		return EXCEPTION_EXECUTE_HANDLER;

	MINIDUMP_EXCEPTION_INFORMATION Mei;

	Mei.ThreadId = GetCurrentThreadId();
	Mei.ExceptionPointers = ExceptionPointers;
	Mei.ClientPointers = TRUE;

	BOOL WriteDump = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), File, MiniDumpNormal, ExceptionPointers ? &Mei : nullptr, nullptr, nullptr);

	CloseHandle(File);
	FreeLibrary(DbgHelp);

	if (WriteDump)
	{
		TCHAR Message[_MAX_PATH + 256];
		lstrcpy(Message, TEXT("LeoCAD just crashed. Crash information was saved to the file '"));
		lstrcat(Message, gMinidumpPath);
		lstrcat(Message, TEXT("', please send it to the developers for debugging."));

		MessageBox(nullptr, Message, TEXT("LeoCAD"), MB_OK);
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
	TCHAR ModulePath[_MAX_PATH], LongModulePath[_MAX_PATH];
	TCHAR Temp[2*_MAX_PATH];

	GetModuleFileName(nullptr, LongModulePath, _MAX_PATH);
	if (GetShortPathName(LongModulePath, ModulePath, _MAX_PATH) == 0)
		lstrcpy(ModulePath, LongModulePath);

	if (RegSetValue(HKEY_CLASSES_ROOT, TEXT("LeoCAD.Project"), REG_SZ, TEXT("LeoCAD Project"), lstrlen(TEXT("LeoCAD Project")) * sizeof(TCHAR)) != ERROR_SUCCESS)
		return;

	lstrcpy(Temp, ModulePath);
	lstrcat(Temp, TEXT(",0"));
	if (RegSetValue(HKEY_CLASSES_ROOT, TEXT("LeoCAD.Project\\DefaultIcon"), REG_SZ, Temp, lstrlen(Temp) * sizeof(TCHAR)) != ERROR_SUCCESS)
		return;

	lstrcpy(Temp, ModulePath);
	lstrcat(Temp, TEXT(" \"%1\""));
	if (RegSetValue(HKEY_CLASSES_ROOT, TEXT("LeoCAD.Project\\shell\\open\\command"), REG_SZ, Temp, lstrlen(Temp) * sizeof(TCHAR)) != ERROR_SUCCESS)
		return;

	LONG Size = 2 * _MAX_PATH;
	LONG Result = RegQueryValue(HKEY_CLASSES_ROOT, TEXT(".lcd"), Temp, &Size);

	if (Result != ERROR_SUCCESS || !lstrlen(Temp) || lstrcmp(Temp, TEXT("LeoCAD.Project")))
	{
		if (RegSetValue(HKEY_CLASSES_ROOT, TEXT(".lcd"), REG_SZ, TEXT("LeoCAD.Project"), lstrlen(TEXT("LeoCAD.Project")) * sizeof(TCHAR)) != ERROR_SUCCESS)
			return;

		HKEY Key;
		DWORD Disposition = 0;

		if (RegCreateKeyEx(HKEY_CLASSES_ROOT, TEXT(".lcd\\ShellNew"), 0, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_READ, nullptr, &Key, &Disposition) != ERROR_SUCCESS)
			return;

		Result = RegSetValueEx(Key, TEXT("NullFile"), 0, REG_SZ, (CONST BYTE*)TEXT(""), (lstrlen(TEXT("")) + 1) * sizeof(TCHAR));

		if (RegCloseKey(Key) != ERROR_SUCCESS || Result != ERROR_SUCCESS)
			return;
	}
}

#endif

static void lcInitializeSurfaceFormat(int argc, char* argv[])
{
	QCoreApplication Application(argc, argv);
	const lcCommandLineOptions Options = lcApplication::ParseCommandLineOptions();

	QSurfaceFormat Format = QSurfaceFormat::defaultFormat();
	Format.setDepthBufferSize(24);
	Format.setStencilBufferSize(8);
#ifndef LC_OPENGLES
	Format.setRenderableType(QSurfaceFormat::OpenGL);
#endif

	if (Options.ParseOK && Options.AASamples > 1)
		Format.setSamples(Options.AASamples);

	QSurfaceFormat::setDefaultFormat(Format);
}

int main(int argc, char *argv[])
{
	QCoreApplication::setOrganizationDomain(QLatin1String("leocad.org"));
	QCoreApplication::setOrganizationName(QLatin1String("LeoCAD Software"));
	QCoreApplication::setApplicationName(QLatin1String("LeoCAD"));
	QCoreApplication::setApplicationVersion(QLatin1String(LC_VERSION_TEXT));
	QGuiApplication::setDesktopFileName(QLatin1String("leocad"));

	lcInitializeSurfaceFormat(argc, argv);

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
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

	qRegisterMetaType<PieceInfo*>("PieceInfo*");
	qRegisterMetaType<QList<int> >("QList<int>");
	qRegisterMetaType<lcVector3>("lcVector3");
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	qRegisterMetaTypeStreamOperators<QList<int> >("QList<int>");
	QMetaType::registerComparators<lcVector3>();
#endif

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
	{
		Application.Shutdown();
		return 1;
	}

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
