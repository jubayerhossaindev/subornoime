# Part III — Regression Matrix (Release / Debug / ASan+UBSan / TSan)

Mission §8. All configurations executed in this session against commit
`909dec1` + verification corrections (D-001…D-004 below). Machine: 2 vCPU
Xeon, 3.9 GiB — every wall time carries that context.

## Release (`build/final`, -O2, Ninja, g++ 14.2)

| Run | Result | Exit | Log |
|---|---|---|---|
| ctest pre-format-fix | 3/3 suites, 44.9 s | 0 | `release_ctest.log` |
| ctest post-format-fix (contended) | 1 failed (`suborno.unit` = the 2 timing fences — D-003, see below) | recorded | `release_ctest.log` |
| Full unit suite idle (pre-D-003) | **686/686 PASSED, 33.9 s** | 0 | `idle_rerun.log` |
| ctest post-D-003 | **3/3 suites, 100%** | 0 | `release_ctest.log` |

## Debug (`build/debug`, -g, assertions on)

| Run | Result | Exit | Log |
|---|---|---|---|
| Full build + ctest pre-D-003 (contended: ASan soak + Release ctest running) | **684/686 passed**; failures = `PerformanceBudget.SingleKeystroke…` (24.6 ms vs 5 ms) + `PerformanceBudget.AutoRepeatLoop…` (19.6 ms) — scheduler descheduling, exact ns evidence in `flaky_evidence.log` | recorded | `debug_build_and_ctest.log`, `LastTest.log` extract |
| Rebuild + ctest post-D-003 | **3/3 suites, 100%, 605.8 s (contended)** | 0 | `debug_postfix_ctest.log` |

## ASan + UBSan (`build/sanitizers`, detect_leaks=1, halt_on_error=1)

| Run | Result | Exit | Log |
|---|---|---|---|
| Part III suites (34 tests, incl. all 4 soak workloads under ASan) | **34/34 PASSED** — no leaks, no UB, no crashes, 1886.2 s | 0 | `asan_ubsan.log` |

Notes: `halt_on_error=1` + `detect_leaks=1` make any leak or UB abort the
run — passing means clean. The earlier partial ASan run (28 tests) also
passed; its log was overwritten by the final complete run documented here.

## TSan (`build/tsan`, -fsanitize=thread, O1)

| Run | Result | Exit | Log |
|---|---|---|---|
| `DiagnosticsTest.*` (concurrency producers, ring contention, sink) | **7/7 PASSED** | 0 | `tsan.log` |

`libtsan` present in this environment (gcc 14) — TSan executed, no blocker.
The soak/session tests are single-threaded by design; the concurrency-sensitive
surface is the DiagnosticsLog producer suite (plus the OS-level tests on
Windows, which the CI runs).

## Defects and corrections driving these numbers

- **D-001**: `SUBORNO_BUILD_BENCHMARKS=ON` configure hard-failure — duplicate
  `add_subdirectory(benchmarks)` removed; graceful degradation re-verified.
- **D-002**: 9 files with clang-format violations formatted; gate green.
- **D-003**: timing-fence flakiness under contention — statistical fences
  (median + ≤1% outliers, **5 ms ceilings unchanged**); proven idle×3 +
  contended×3, Release + Debug.
- **D-004**: Part III test-count report corrected (DiagnosticsTest 7).

## Architecture protection re-verified (§11)

C++20 scan OK; naming OK; comments OK (344 files); licenses OK;
layering OK (20 edges, allowed matrix); clang-format green; portable core
free of unguarded Windows headers (only `_WIN32`-guarded in
`persistence/atomic_file.cpp`); public C API unchanged (no `suborno.h`
edits); Bengali Unicode corpus regressions green (686-test suite).

## Honest limitations

- Soak passes prove the tested invariants over ~172k operations + sanitizer
  runs — **not** the absence of all memory-safety defects.
- Wall times are contended-shared-VM numbers; the Windows CI uses dedicated
  2-core runners.
- google/benchmark suites could not execute (dependency absent —
  `benchmarks.md`).
