#ifndef _LC_CONFIG_H_
#define _LC_CONFIG_H_

#include <QtGlobal>
#include <QWidget>
#include <QtOpenGL>
#include <QGLWidget>
#include <QtGui>

#define LC_MAXPATH 1024

#define LC_POINTER_TO_INT(p) ((lcint32) (quintptr) (p))

typedef qint8 lcint8;
typedef quint8 lcuint8;
typedef qint16 lcint16;
typedef quint16 lcuint16;
typedef qint32 lcint32;
typedef quint32 lcuint32;
typedef qint64 lcint64;
typedef quint64 lcuint64;
typedef quintptr lcuintptr;

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
#define LC_LITTLE_ENDIAN
#else
#define LC_BIG_ENDIAN
#endif

#ifdef Q_OS_WIN
#ifdef QT_NO_DEBUG
#define LC_ASSERT(Expr)
#else
#define LC_ASSERT_STR(x) #x
#define LC_ASSERT_TOSTR(x) LC_ASSERT_STR(x)
#define LC_ASSERT(Expr) \
do \
{ \
	static bool Ignore = false; \
	if (!(Expr) && !Ignore) \
	{ \
		TCHAR* text = TEXT("Expression: ' ") TEXT(LC_ASSERT_STR(Expr)) TEXT("'\nLocation: ") TEXT(__FILE__) TEXT(":") TEXT(LC_ASSERT_TOSTR(__LINE__)); \
		if (MessageBox(NULL, text, TEXT("Assertion failed"), MB_OKCANCEL | MB_ICONERROR) == IDCANCEL) \
			Ignore = true; \
		else \
			DebugBreak(); \
	} \
} while (0)
#endif
#else
#define LC_ASSERT Q_ASSERT
#endif

#if _MSC_VER >= 1600
#define LC_CASSERT(x) static_assert(x, "Assertion failed: " #x)
#else
#define LC_CASSERT_CONCAT(arg1, arg2)   LC_CASSERT_CONCAT_(arg1, arg2)
#define LC_CASSERT_CONCAT_(arg1, arg2)  arg1##arg2

#define LC_CASSERT(expression)\
struct LC_CASSERT_CONCAT(__assertion_at_line_, __LINE__) \
{ \
	lcStaticAssert<static_cast<bool>((expression))> LC_CASSERT_CONCAT(LC_CASSERT_CONCAT(_ASSERTION_FAILED_AT_LINE_, __LINE__), _); \
}; \
typedef lcStaticAssertTest<sizeof(LC_CASSERT_CONCAT(__assertion_at_line_, __LINE__))> LC_CASSERT_CONCAT(__assertion_test_at_line_, __LINE__)

template<bool> struct lcStaticAssert;
template<> struct lcStaticAssert<true> { };
template<int i> struct lcStaticAssertTest { };
#endif

#ifdef Q_OS_WIN
#define isnan _isnan
#define strcasecmp stricmp
#define strncasecmp strnicmp
char* strcasestr(const char *s, const char *find);
#else
char* strupr(char* string);
char* strlwr(char* string);
int stricmp(const char* str1, const char* str2);
#endif

#endif // _LC_CONFIG_H_
