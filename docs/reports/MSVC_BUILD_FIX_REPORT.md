# MSVC Release-Build Fix Report — suborno_app + TSF warnings

Date: 2026-11-21
Input: user-supplied MSVC 14.44 / Windows SDK 10.0.26100 release-build log
(`cmake --build --preset windows-msvc-release`).

## Root cause

The dark-shell UI sources (`application.cpp`, `menu_popup.cpp`, `topbar.cpp`)
and the app/TSF platform layer are Windows-only: the Ubuntu CI/sandbox never
compiles them, so these defects were invisible to every Linux-side gate until
the first real MSVC build.

## Fixes (29 diagnostics: 22 errors + 7 warnings)

| Diagnostic | File:line | Fix |
|---|---|---|
| C2228 `.icon` on pointer | application.cpp:604 | `state.icon` → `state->icon` |
| C2662 `release()` on `const unique_ptr` | application.cpp:678/641 | dropped `const` from the `make_unique` local in `show_about_dialog()` |
| C2672 `std::max` LONG/int ambiguity + C2737 | application.cpp:757–760 | `std::max(0, …)` → `std::max(0L, …)` |
| C2065 `font_message_` ×4 | application.cpp:1531–1542 | renamed to the real member `message_font_` |
| C2589/C2660/C2059/C2373 `min`/`max` macros ×8 | menu_popup.cpp:113, 238–241 | NOMINMAX guard at the top (project idiom, matches `application.cpp`) |
| C2248 `Topbar::Impl` private ×9 + C2064 cascade | topbar.cpp:41–278 | `struct Impl;` moved to the public section of `topbar.hpp` (incomplete handle; completed in the .cpp) |
| C2589/C2059/C2737 `max` macros ×6 | topbar.cpp:336–339 | NOMINMAX guard at the top |
| C28252/C28253 SAL mismatch ×4 | dll_main.cpp:21, windows_build_smoke_test.cpp:17 | `_Check_return_` → the SDK's `__control_entrypoint(DllExport)` form for `DllCanUnloadNow` |
| LNK4104 ×5 | suborno_windows_tsf.def | exports stay non-PRIVATE (`suborno_windows_tests` imports `Dll*` directly) and the target silences the warning with `/IGNORE:4104` |

The def-file `PRIVATE` alternative was rejected deliberately: marking the
five `Dll*` exports PRIVATE removes them from the import library, and
`suborno_windows_tests` links `suborno_windows_tsf` and calls
`DllCanUnloadNow()`/`DllGetClassObject()`/`DllRegisterServer()`/
`DllUnregisterServer()` directly — PRIVATE would have traded five warnings
for five LNK2019 unresolved externals.

## Verification matrix

| Gate | Command | Result |
|---|---|---|
| Format | `CLANG_FORMAT=… ./scripts/check-format-ubuntu.sh` (clang-format 23.1.5, repo `.clang-format`, all C++ incl. Windows sources) | PASS |
| Lint | `CLANG_TIDY=… ./scripts/check-tidy-ubuntu.sh build/verify2` (clang-tidy 23.1.4) | PASS — 0 findings in project code (751 suppressed, all non-user code) |
| Configure | `cmake -S . -B build/verify2` | exit 0 (validates the `/IGNORE:4104` CMake edit) |
| Build | `cmake --build build/verify2 -j4` | exit 0, 0 warnings |
| Test | `ctest` in `build/verify2` | 2/2 passed (unit + integration binaries green) |

MSVC itself cannot run in the Ubuntu sandbox; every fix above maps 1:1 to an
MSVC diagnostic from the supplied log, and the next
`cmake --build --preset windows-msvc-release --parallel 1` is expected to
complete the `suborno_app` target with zero errors and zero warnings.
