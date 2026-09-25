# Code instructions

- Keep the file lists in `leocad.pro` alphabetically sorted within their existing groups.
- Get Qt and std includes through the precompiled header, `lc_global.h`.
- Prefer std containers, use Qt containers where a Qt API boundary makes them useful.
- Avoid using `auto` for ordinary variable types. Template iterators, lambdas, and structured bindings may use it for readability.
- Prefer a private static member function over a file-scope helper when the function belongs to a class and does not use instance state.
- Prefer explicit `lock()` and `unlock()` over an extra scope with `QMutexLocker` for short, straight-line critical sections that must end before the following code. Keep `QMutexLocker` for early returns, waits, or nontrivial control flow.
