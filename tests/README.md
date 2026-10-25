# LeoCAD tests

Core tests run in one Qt Test executable, `leocad_tests`. These tests cover
object history, property-key comparisons, and the final animation key after
step insertion, removal, and key deletion, including 8,000 deterministic
randomized undo/redo and merge transitions.

The executable creates a `QCoreApplication`; it does not open windows or require
an OpenGL context, the parts library, or network access. It uses the same Qt
headers as the application, plus Qt Test, and supports Qt 5 and Qt 6.

## Build and run

From the repository root, with the intended Qt installation's `qmake` on PATH:

```sh
mkdir -p build/tests
cd build/tests
qmake ../../tests/tests.pro CONFIG+=release CONFIG-=debug
make -j
make check
```

On Windows, use the make tool for the selected Qt kit, such as `nmake`.

To list test functions or run one function directly:

```sh
./leocad_tests -functions
./leocad_tests HistoryStoresOnlyChangedObjects
```

On Windows, the executable is `leocad_tests.exe` and may be in `release/`.
`make check` handles the configured executable location.

To write both console output and a JUnit report:

```sh
make check TESTARGS="-o test-results.xml,junitxml -o -,txt"
```

## Add tests

1. Declare a named `void` function under `private slots` in `lc_tests.h`.
2. Define it in the relevant test source file as `void lcTests::FunctionName()`.
3. If adding a new source file, list it in `tests.pro`.

Use `QVERIFY`, `QCOMPARE`, or other Qt Test checks so assertions remain active in
release builds and failures identify the function and source line. Each test
must create its own state. Use temporary directories for writable fixtures and
fixed seeds for randomized tests; include the seed and iteration in failures.
Tests should call production code directly.

Application-level tests needing more source files should declare those sources
and dependencies in `tests.pro`; shared source lists can move into `.pri` files
when needed. Keep application startup and packaging out of the test target.

## GitHub CI

`.github/workflows/continuous.yml` builds and runs this target on Linux (Qt 5),
macOS (Qt 6), and Windows x64 (Qt 6 with MSVC 2022), on pull requests to `master`,
pushes to `master`, and manual runs. JUnit reports are uploaded as artifacts,
including on test failure. Adding a test does not require changing the workflow
or `main.cpp`.

The Windows job uses `nmake` and a single release configuration. To reproduce its
commands in an x64 MSVC developer command prompt with Qt on PATH:

```bat
mkdir build\tests
cd build\tests
qmake ..\..\tests\tests.pro -spec win32-msvc CONFIG+=release CONFIG-=debug CONFIG-=debug_and_release
nmake /NOLOGO
nmake /NOLOGO check "TESTARGS=-o test-results.xml,junitxml -o -,txt"
```

## Model instance integration test

`model_instances.pro` builds a separate Qt Test executable using the application
sources and resources, with its own entry point. It creates temporary models and
an in-memory parts library; it requires no installed LDraw library, network
access, or OpenGL rendering. It checks repeated and nested instances, step
visibility, animated transforms, highlighting, clicks, box selection, shared
edits, undo/redo, and tab switching. Camera and light checks cover exclusion
from in-place picking, box selection, and the Select Objects dialog, while
preserving normal selection in a model tab. Every scene build checks that rendering
leaves editing state and cached matrices unchanged, including when caches hold
a different step from the one being rendered.

```sh
mkdir -p build/model-instance-tests
cd build/model-instance-tests
qmake ../../tests/model_instances.pro
make -j
QT_QPA_PLATFORM=offscreen make check
```

Use the make tool and executable location provided by the selected Qt kit.
The existing CI job runs the core target; this integration target is separate.
