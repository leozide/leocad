# Code instructions

- Keep the file lists in `leocad.pro` alphabetically sorted within their existing groups.
- Get Qt and std includes through the precompiled header, `lc_global.h`.
- Prefer std containers, use Qt containers where a Qt API boundary makes them useful.
- Avoid using `auto` for ordinary variable types. Template iterators, lambdas, and structured bindings may use it for readability.
- Do not nest ternary expressions. Use explicit branches when a choice needs another condition.
- Prefer a private static member function over a file-scope helper when the function belongs to a class and does not use instance state.
- Prefer explicit `lock()` and `unlock()` over an extra scope with `QMutexLocker` for short, straight-line critical sections that must end before the following code. Keep `QMutexLocker` for early returns, waits, or nontrivial control flow.
- Preserve existing blank lines when editing code. Prefer blank lines between declarations and following logic, and between distinct logical steps; do not remove user-added blank lines without a specific reason.
- When adding a parameter to an existing function, update every call site explicitly instead of adding a default argument to preserve old calls.
