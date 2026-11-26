# WINDOWS_BUILD_REGRESSION_REPORT

Project: SubornoIME · Date: 2026-11-22 · HEAD: `cf2a0ca` (BUG-05 fix) · Full logs: `windows-ui-audit/logs/`

## 1. Windows MSVC build — BLOCKED (environment)

`cmake --preset windows-msvc` + `cmake --build --preset windows-msvc-release -j1` + `ctest --preset windows-msvc-release` require Visual Studio 2022 / MSVC v143 / Windows SDK. This audit environment is **Linux (Debian 13)**; MSVC, the Windows SDK and the TSF runtime are not present and cannot be installed here. **No Windows build result is claimed.** The MSVC pipeline remains covered by the `windows-msvc` preset (`SUBORNO_ENABLE_MSVC_ANALYSIS=ON`, `/analyze`) on a Windows machine; Windows-only TUs (`src/platform/windows/**`, TSF/COM, WIL — 54 TUs not in the Linux compile DB) are analyzed only there.

## 2. Logic-layer regression — really executed (Linux fallback, portable core)

Environment: Debian 13 trixie, **GCC 14.2.0**, CMake 4.4.5, Ninja, ICU 76.1, libsqlite3, local GoogleTest provision. Configure mirrors the `ubuntu-dev` preset cache variables verbatim (Debug, `BUILD_TESTING=ON`, `CMAKE_EXPORT_COMPILE_COMMANDS=ON`, benchmarks OFF) with the CMakeLists-documented no-vcpkg fallback. **Build parallelism 1** (preset `jobs: 1` honored; also required by the low-resource target machine rule).

| Step | Command | Result |
|---|---|---|
| Configure | `cmake -S . -B build/ubuntu-dev -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DGTest_DIR=/tmp/gtest-install/lib/cmake/GTest` | exit 0 (`logs/configure_ubuntu-dev.log`) |
| Build | `cmake --build build/ubuntu-dev --parallel 1` | exit 0, 90/90 steps, **0 warnings** (`logs/build_ubuntu-dev.log`) |
| Unit tests | `ctest` / direct binary | **543/543 PASSED, exit 0** (`logs/unit_after_bug05.log`; `[==========] Running 543 tests from 61 test suites.`) |
| Integration | direct binary | **2/2 PASSED, exit 0** (`logs/integration_after_bug05.log`) |
| Fuzz corpus | `scripts/layout_verify/fuzz_corpus_runner.cpp` calls the real `LLVMFuzzerTestOneInput` on every `tests/fuzz/corpus/*` input; built with GCC 14.2 **ASan+UBSan** (`-fsanitize=address,undefined`, `detect_leaks=1`, `halt_on_error=1`) | **14/14 executed, exit 0 — no crash, no sanitizer error, no leak** (`logs/fuzz_corpus_gcc_asan_ubsan.log`, executed 2026-11-23). The sanitized static libs build clean: 40/40 steps, 0 warnings (`logs/fuzzrun_build.log`). Coverage-guided libFuzzer mutation requires Clang (`SUBORNO_BUILD_FUZZING` gate) and stays a documented limitation, not claimed as executed |
| Format | `clang-format --dry-run --Werror` (23.1.5, project `.clang-format`) | clean on all modified files |
| i18n parity | `scripts/layout_verify/i18n_parity_check.py` | **bn 160 keys == en 160 keys, 0 missing, 0 empty, exit 0** |

The historical baseline claim "542 unit tests" was re-verified against reality: the pre-fix suite ran **542/542** (`logs/unit_direct.log`); after the BUG-05 regression test the suite is **543**.

## 3. Bug-fix regression verification

| Bug | Area | Status | Verification |
|---|---|---|---|
| BUG-01 | UIA duplicated member definitions | Fixed (commit `993968c`) | Regression via full suite + UIA surface compile; Windows runtime display BLOCKED |
| BUG-02 | Topbar tooltip locale strings | Fixed (commit `993968c`) | i18n parity 160/160 executed; tooltip display BLOCKED |
| BUG-03 | Error/confirmation dialog localization | Fixed (commit `5724528`) | i18n parity executed; dialog display BLOCKED |
| BUG-04 | Language-changed toast wiring | Fixed (commit `5724528`) | i18n parity executed; toast display BLOCKED |
| **BUG-05** | Classic layout display-name mixed-script corruption ("Born" + U+09CB) | **Fixed this audit, commit `cf2a0ca`** | New regression test executed in the 543-test suite; on-screen rendering BLOCKED |
| **BUG-06** | Default layout drops chandrabindu (পাঁচ→পাচ) | **OPEN — High** | Engine-level reproduction executed; fix is a protected transliteration-engine change — **stopped and reported** per project protection rules |

## 4. Localization keys

Computed from the real locale files (no hardcoded count): `data/i18n/bn/ui.json` and `data/i18n/en/ui.json` — 160 keys each, identical key sets, no empty values. Error-dialog, confirmation and language-changed-toast keys are covered by the same parity check; their on-screen appearance is a Windows-runtime item (BLOCKED).
