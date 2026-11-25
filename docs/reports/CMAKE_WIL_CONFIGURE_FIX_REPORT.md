# CMake `WIL::WIL` Configure Failure — Root-Cause Investigation & Fix

**Date:** 2026-11-21
**Failure signature:** `get_target_property() called with non-existent target "WIL::WIL"` at `cmake/modules/SubornoBuildOptions.cmake:66`, called from `tests/CMakeLists.txt:74 (suborno_mark_dependency_headers_external)`
**Status:** Fixed (2 files, 19 insertions). Configure validated in sandbox; `cmake --preset windows-msvc` must be re-run on the Windows build host (MSVC unavailable in the investigation sandbox).

## 1. Root cause

CMake **IMPORTED targets created by `find_package()` are directory-scoped**: they are visible only in the directory where they were created and its subdirectories — not in sibling or parent directories.

Evidence chain:

1. `src/platform/windows/CMakeLists.txt:9` runs `find_package(wil CONFIG REQUIRED)` — **successfully** (configure proceeded past it; the failure came later, from `tests/`). This creates `WIL::WIL` as an IMPORTED target in the `src/platform/windows` **directory scope only**.
2. `src/platform/CMakeLists.txt:8-10` adds `windows/` under `if(WIN32)`; the root `CMakeLists.txt:54` adds `src/platform` **before** `tests` (line 60) — so the target existed *in its own scope* by the time `tests/` was processed.
3. `tests/CMakeLists.txt:63` links `WIL::WIL` — link items are only resolved at generate time, so this did not fail during configure.
4. `tests/CMakeLists.txt:74` → `suborno_mark_dependency_headers_external(suborno_windows_tests WIL::WIL)` → `get_target_property()` **does** require the target at configure time → hard error.

Cross-checks that eliminate the alternative hypotheses:

| Hypothesis | Verdict | Evidence |
| --- | --- | --- |
| Missing `find_package(wil)` | Exists, executed successfully | `src/platform/windows/CMakeLists.txt:9`; vcpkg log shows `wil:x64-windows-static@1.0.260126.7` installed, "All requested installations completed successfully" |
| Wrong exported target name | Name is correct (`WIL::WIL`) | vcpkg's own usage note (`find_package(wil CONFIG REQUIRED)` / `WIL::WIL`); identical usage works in `src/platform/windows/CMakeLists.txt:66,78` |
| Executed too late (ordering) | Not applicable — runs before `tests/` | root `CMakeLists.txt:45-60` ordering |
| **Directory-scope invisibility** | **Confirmed** | A minimal repro (sibling directories, real find_package + unguarded helper) reproduced the exact error string; adding `find_package(wil)` in the consuming directory fixed it |

Why `GTest::gtest` works in `tests/`: it is found **inside** `tests/CMakeLists.txt:5` — same scope. Why `ICU`/`sqlite3` work everywhere: found in the **root** scope (`CMakeLists.txt:28-39`), visible to all subdirectories.

## 2. Files modified

### `tests/CMakeLists.txt` (added `find_package(wil)` inside the `WIN32` block)

```cmake
# BEFORE
if(WIN32)
    add_executable(suborno_windows_tests
```

```cmake
# AFTER
if(WIN32)
    # WIL::WIL is an IMPORTED target created by find_package(wil) inside the
    # src/platform/windows subtree. IMPORTED targets are directory-scoped, so
    # they are invisible to this sibling directory: this tree must run its own
    # find_package before referencing the target (same pattern as
    # find_package(GTest) at the top of this file).
    find_package(wil CONFIG REQUIRED)
    add_executable(suborno_windows_tests
```

### `cmake/modules/SubornoBuildOptions.cmake` (fail-fast guards in the helper)

```cmake
# BEFORE
function(suborno_mark_dependency_headers_external target_name dependency_target)
    get_target_property(dependency_include_directories
        "${dependency_target}" INTERFACE_INCLUDE_DIRECTORIES)
```

```cmake
# AFTER
function(suborno_mark_dependency_headers_external target_name dependency_target)
    if(NOT TARGET "${target_name}")
        message(FATAL_ERROR
            "suborno_mark_dependency_headers_external: '${target_name}' is not a target")
    endif()
    if(NOT TARGET "${dependency_target}")
        message(FATAL_ERROR
            "suborno_mark_dependency_headers_external: dependency target "
            "'${dependency_target}' does not exist in this directory scope. "
            "IMPORTED targets created by find_package() are visible only in "
            "the directory where they were found and its subdirectories: run "
            "find_package() for '${dependency_target}' in this directory (or "
            "a parent) before marking its headers SYSTEM.")
    endif()
    get_target_property(dependency_include_directories
        "${dependency_target}" INTERFACE_INCLUDE_DIRECTORIES)
```

## 3. Why this fix is correct

- **Declares what each build unit links.** `tests/` directly links `WIL::WIL` for `suborno_windows_tests`, so it runs its own `find_package` — exactly the pattern already used for `GTest` (line 5). No new global state; no target moved between modules; the per-directory ownership architecture (root `CMakeLists.txt:41-44` comment) is preserved.
- **Fixes both failure points**, not just the reported one: with the target now in scope, `target_link_libraries(... WIL::WIL)` (generate-time) and `get_target_property` (configure-time) both resolve.
- **Linux/macOS untouched.** The added call is inside `if(WIN32)`; on Linux the branch is skipped and `vcpkg.json` marks `wil` as `"platform": "windows"`.
- **The helper guard is diagnostics, not suppression.** It converts CMake's cryptic `get_target_property() called with non-existent target` into an actionable message naming the scoping rule and remedy, mirroring the existing `suborno_apply_build_options` guard style (lines 7-9). Invalid targets still hard-fail; nothing is silently skipped.
- **SYSTEM marking semantics unchanged:** still `target_include_directories(... SYSTEM PRIVATE ${INTERFACE_INCLUDE_DIRECTORIES})`, still a no-op for dependencies that export no include dirs.

## 4. Validation (Linux sandbox, CMake 4.4.5, GCC 14.2)

| # | Test | Result |
| --- | --- | --- |
| 1 | Minimal repro, pre-fix structure (sibling scopes + original helper) | ✗ failed with the **exact** error: `get_target_property() called with non-existent target "WIL::WIL"` |
| 2 | Same repro, post-fix pattern (own `find_package` + hardened helper) | ✓ configure + full build; generated flags contain `-isystem .../include` (SYSTEM marking verified) |
| 3a | Real repo helper, missing dependency target | ✓ fails fast with the new actionable message (`SubornoBuildOptions.cmake:71`) |
| 3b | Real repo helper, missing consumer target | ✓ fails fast (`'ghost_target' is not a target`) |
| 3c | Real repo helper, happy path + dependency without include dirs (no-op) | ✓ exit 0; `-isystem` flag emitted; no-op branch safe |
| 4 | Full repository configure, `BUILD_TESTING=ON`, `SUBORNO_ENABLE_MSVC_ANALYSIS=ON` | ✓ `Configuring done` / `Generating done` (tests tree parsed; Linux path intact) |
| 5 | Real target build (`suborno_core` full chain, GCC) | ✓ `Built target suborno_core` |
| 6 | CTest registration | ✓ `suborno.unit` present in generated `CTestTestfile.cmake` |

## 5. Remaining steps for the Windows build host

MSVC is not available in the investigation sandbox, so the definitive Windows validation must be re-run:

```cmd
cmake --preset windows-msvc
cmake --build --preset windows-msvc-debug --parallel 2
ctest --test-dir build\windows-msvc -C Debug --output-on-failure
```

Expected: the configure error is gone (the only change on the Windows path is the added `find_package(wil)` resolving `WIL::WIL` from the already-installed `wil:x64-windows-static@1.0.260126.7`). No warnings or errors are introduced by these edits; `/analyze`, sanitizers, and all 26 test files are untouched.
