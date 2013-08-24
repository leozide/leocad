#ifndef _SYSTEM_H_
#define _SYSTEM_H_

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

void SystemUpdatePlay(bool play, bool stop);
void SystemPieceComboAdd(char* name);

#endif // _SYSTEM_H_
