#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include "typedefs.h"
#include "array.h"

// Assert macros.
#ifdef LC_DEBUG

extern bool lcAssert(const char* FileName, int Line, const char* Expression, const char* Description);

#define LC_ASSERT(Expr, Desc) \
do \
{ \
	static bool Ignore = false; \
	if (!Expr && !Ignore) \
		Ignore = lcAssert(__FILE__, __LINE__, #Expr, Desc); \
} while (0)

#define LC_ASSERT_FALSE(Desc) LC_ASSERT(0, Desc)

#else

#define LC_ASSERT(...) do { } while(0)

#define LC_ASSERT_FALSE(Desc) LC_ASSERT(0, Desc)

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

// Misc stuff
bool Sys_KeyDown (int key);
void Sys_GetFileList(const char* Path, ObjArray<String>& FileList);

// User Interface
void SystemUpdateColorList(int nNew);
void SystemUpdateSelected(unsigned long flags, int SelectedCount, class Object* Focus);
void SystemUpdatePlay(bool play, bool stop);
void SystemUpdateFocus(void* p);

void SystemInit();
void SystemFinish();
bool SystemDoDialog(int nMode, void* param);
void SystemDoPopupMenu(int nMenu, int x, int y);

void SystemPieceComboAdd(char* name);

void SystemPumpMessages();
long SystemGetTicks();

#endif // _SYSTEM_H_
