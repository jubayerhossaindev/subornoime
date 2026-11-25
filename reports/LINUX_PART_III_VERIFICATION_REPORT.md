# SUBORNO IME v19 — Part III Linux Verification Report

Final report for the Part III Linux Verification & Evidence Collection
Mission (§33–§35 of the Refactoring Report). Date: 2026-11-25 (UTC).

## 1. Repository commit SHA

- Base: `909dec1b2cb91faddfaa9496ea0725bcad213614` (v20 final-acceptance wave)
- Verification branch: `ci/suborno-v19-windows-verification` (base + the
  corrections in §17; no production-source modifications beyond the three
  listed defect fixes and one documentation correction)
- Remote: `origin = https://github.com/jubayerhossaindev/subornoime.git`
  (added this session; repo confirmed public and empty)

## 2. Linux environment

Debian GNU/Linux, kernel 5.10.134, g++/gcc 14.2.0, CMake 4.4.5 + Ninja,
**2 vCPU (Xeon), 3.9 GiB RAM**, clang-format 23.1.5 (venv). Absent: clang,
doxygen, pwsh, `/usr/bin/time`, google/benchmark (`logs/environment.txt`).

## 3. Scope and exclusions

Executed: everything portable (unit/integration/fuzz/soak/budget suites in
Release, Debug, ASan+UBSan, TSan; audits; benchmark wiring; count
reconciliation; Windows source audits). Excluded by environment: MSVC/TSF/
installer runtime (GitHub-hosted Windows runner required — see
`reports/WINDOWS_GITHUB_VERIFICATION_REPORT.md`), google/benchmark execution
(dependency absent), Doxygen build, PowerShell syntax validation.

## 4. All actual commands (representative; full logs carry every exit code)

```bash
cmake --build build/final -j && ctest --output-on-failure          # Release
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON \
      -DCMAKE_PREFIX_PATH=/home/z/my-project/tools/gtest-install   # Debug
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 ./suborno_unit_tests \
  --gtest_filter="KeyEventMatrix.*:CandidateNavigation.*:DiagnosticsTest.*:StabilitySoak.*:PerformanceBudget.*"
TSAN_OPTIONS=halt_on_error=1 ./suborno_unit_tests --gtest_filter="DiagnosticsTest.*"
./suborno_unit_tests --gtest_list_tests
python3 -I scripts/check_cpp20_only.py . ; python3 -I scripts/validate_naming.py
python3 -I scripts/audit_comments.py . ; python3 -I scripts/audit_licenses.py .
python3 -I scripts/check-layering.py
CLANG_FORMAT=/home/z/.venv/bin/clang-format bash scripts/check-format-ubuntu.sh
cmake -S . -B build/bench-attempt -DSUBORNO_BUILD_BENCHMARKS=ON …
```

## 5. Exact build results

| Config | Build | Tests |
|---|---|---|
| Release | clean, incremental zero-error | ctest **3/3, 100%** (unit 686/686, integration, fuzz smoke 17 seeds/1105 execs) |
| Debug | clean (GTest via tools prefix) | pre-fix 684/686 → **post-fix 3/3, 100%** |
| ASan+UBSan | clean rebuild | **34/34 Part III tests**, leak+UB clean |
| TSan | clean (-fsanitize=thread) | DiagnosticsTest **7/7** |

## 6. Exact test counts

- Full unit suite: **686 tests, 80 suites, 0 disabled, 0 skipped** — all PASS
- Part III suites: **34 unique test cases** (discovery + execution agree)
- CTest suites: 3 (`suborno.unit`, `suborno.integration`, `suborno.fuzz_smoke`)

## 7. Test-count discrepancy resolution

The report claimed 34 but its category breakdown summed to 35
(DiagnosticsTest listed as 8). `--gtest_list_tests` proves DiagnosticsTest =
**7** → total 34. `docs/engineering/REFACTORING_REPORT.md` §35 corrected.
Details: `part_iii/test_count_reconciliation.md`.

## 8. Input reliability results

All 4 keyboard-event paths repeat-consistent (base default, national,
probhat, khipro→LayoutInputSession); repeats bounded at 256 codepoints with
atomic rejection; `repeat_count=0` → 1; key-up/modifier suppression; 59
layout tests + 17 typing regressions + 11 matrix + 11 key-event tests green.
`part_iii/input_reliability.md`.

## 9. Navigation/editing results

14 non-text keys present; classifiers disjoint (asserted); no ASCII mapping;
exhaustive 0–200 `PhysicalKey` sweep; live composition preserved across all
12 non-backspace keys then commits exactly `আমি`. `part_iii/navigation_editing.md`.

## 10. Candidate navigation results

Forward/backward walk, both-boundary stop (no wraparound), atomic
out-of-range rejection, commit-follows-selection, reset recovery, 500-cycle
rapid churn bounds, empty-state safe — 6 matrix + 29 candidate-suite tests
green. `part_iii/candidate_navigation.md`.

## 11. Privacy audit results

API audited (not comments): `scrub_user_text` emits codepoint-count markers;
`DiagnosticEntry` carries no session/composition fields; **zero production
logging call sites exist** (nothing can leak user text today); 7/7
DiagnosticsTest green. Honest caveat: privacy is procedural, not
compiler-enforced. `part_iii/diagnostic_privacy.md`.

## 12. Concurrency and sanitizer results

TSan: DiagnosticsTest 7/7 clean. ASan+UBSan: 34/34 clean (leaks abort the
run). TSan executed — no environment blocker.

## 13. Stability soak results

50,000-keystroke bursts; 100,000 unbounded appends (clamp holds at 256);
2,000 focus cycles; 20,000 mixed churn — **PASS ×2 Release repeats**
(68.3 s / 67.4 s, 1.4% spread) + full ASan run; peak RSS 12.8 MB (±40 KiB
between runs); ~172k total operations, zero invariant failures. Honest
scope: no soak proves absence of all defects. `part_iii/stability_soak.md`.

## 14. Performance measurements

5 fences at unchanged 5 ms/2 s ceilings; pre-fix captures show
contention-flakiness (D-003 — real failures with ns evidence, 686/686 idle);
fixed with statistical fences (median + ≤1% outliers, ceilings unchanged);
post-fix idle×3 + contended×3 all PASS. Variability recorded (7–40 ms whole-
test under contention vs 1 ms idle). `part_iii/performance_budgets.md`.

## 15. Benchmark execution results

google/benchmark ABSENT in sandbox → no benchmark numbers (none fabricated).
`SUBORNO_BUILD_BENCHMARKS=ON` now degrades gracefully (status message,
exit 0) after D-001; normal builds unaffected. `part_iii/benchmarks.md`.

## 16. Windows-only limitations

TSF registration/activation/typing, tray, DPI, candidate rendering,
installer install/upgrade/registry/DLL runtime — **UNVERIFIED/BLOCKED**;
installer test script + `.iss` inspected only. Classifications in
`part_iii/windows_runtime_limitations.md`; GitHub-side detail in
`reports/WINDOWS_GITHUB_VERIFICATION_REPORT.md`.

## 17. Defects and corrections

| ID | Defect | Correction |
|---|---|---|
| D-001 | Benchmark option configure hard-failure (duplicate `add_subdirectory`) | duplicate removed; graceful path re-verified |
| D-002 | 9 files with clang-format violations (7 v20-wave, 1 pre-existing) | formatted; gate green |
| D-003 | Timing fences flaky under contention (2 captured failures: 24.6 ms, 19.6 ms vs 5 ms) | statistical fences (median + ≤1% outliers, ceilings unchanged); idle×3 + contended×3 green; Release+Debug ctest 100% |
| D-004 | Report §35 DiagnosticsTest 8 → wrong sum | corrected to 7 with reconciliation pointer |

## 18. Remaining technical debt

- google/benchmark numbers need a dependency-equipped machine/CI run.
- DiagnosticLog has no production call sites yet (facility tested, wiring is
  Windows-side work).
- Per-option dropdown value-change assertions and OSK key→text verification
  are manual-protocol items (interactive-desktop constraint).
- Interactive TSF typing needs a real desktop session (manual protocol).
- Shared-VM wall-time numbers should be re-measured on the CI runner.

## 19. Evidence file links

All under `reports/linux_verification/part_iii/`: README + 10 section
reports (input_reliability, navigation_editing, candidate_navigation,
diagnostic_privacy, stability_soak, performance_budgets, benchmarks,
regression_matrix, test_count_reconciliation, windows_runtime_limitations) +
`logs/` (15 execution logs incl. environment, flaky evidence, protection
battery, idle reruns, sanitizer runs). Windows tree:
`reports/windows_verification/` (13 reports + gui/ 17 files + logs).

## 20. Final acceptance matrix

| Requirement | Linux execution | Evidence | Status |
|---|---|---|---|
| Auto-repeat | Actual test execution | `targeted_runs.log`, matrix 11/11 | **PASS** |
| Key routing | Actual test execution | matrix 11/11, exhaustive sweep | **PASS** |
| Candidate navigation | Actual test execution | 35 tests green | **PASS** |
| Diagnostic privacy | API review + tests | 7/7 + zero-call-site audit | **PARTIAL** (wiring absent; procedural guarantee) |
| Diagnostic concurrency | Sanitizer + soak | TSan 7/7; ASan suite clean | **PASS** |
| Stability soak | Actual stress execution | 2 repeats + ASan, RSS bounded | **PASS** |
| Performance budgets | Actual measurements | post-fix idle×3 + contended×3 | **PASS** (ceilings unchanged) |
| Benchmark wiring | Real configure attempt | graceful degradation proven | **PARTIAL** (dependency absent) |
| Windows TSF integration | Static/source audit | 78 guards, registration audited | **PARTIAL** |
| Windows UI runtime | Source-only on Linux | UIA harness wired, not executed | **UNVERIFIED** |
| Installer install/upgrade | Script inspection | 300-line suite audited | **UNVERIFIED** |

No unsupported PASS above: every PASS row cites executed commands and exit
codes; PARTIAL/UNVERIFIED rows name their exact blocker.
