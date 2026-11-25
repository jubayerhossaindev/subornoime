# Part III Linux Verification Evidence

Collected 2026-11-25 (UTC) in the z.ai Linux sandbox against commit
`909dec1` + verification corrections (see `regression_matrix.md` and the
final report for the exact diff list).

Environment: Debian, g++ 14.2.0, CMake 4.4.5, 2 cores, 3.9 GiB RAM
(`logs/environment.txt`).

## Files

| File | Mission section |
|---|---|
| `input_reliability.md` | §1 auto-repeat |
| `navigation_editing.md` | §2 nav/editing keys |
| `candidate_navigation.md` | §3 candidate navigation |
| `diagnostic_privacy.md` | §4 DiagnosticLog privacy |
| `stability_soak.md` | §5 soak |
| `performance_budgets.md` | §6 latency fences |
| `benchmarks.md` | §7 benchmark wiring |
| `regression_matrix.md` | §8 build/test matrix |
| `test_count_reconciliation.md` | §9 34-vs-35 |
| `windows_runtime_limitations.md` | §10 reclassification |

## Logs (raw, from actual executions)

| Log | Content |
|---|---|
| `environment.txt` | toolchain + capability snapshot |
| `release_ctest.log` | Release build + full ctest, pre- and post-format |
| `debug_build_and_ctest.log` | Debug configure/build/ctest |
| `asan_ubsan.log` | ASan+UBSan run of the Part III suites |
| `tsan.log` | TSan run of the concurrency suite (or its blocker) |
| `targeted_runs.log` | §1–§3 targeted filters with exit codes |
| `test_count_reconciliation.log` | gtest_list_tests + executed counts |
| `soak_and_budgets.log` | soak repeats with RSS + budget timing runs |
| `benchmarks.log` | benchmark configure attempts (pre/post defect fix) |
| `protection_battery.log` | §11 audit battery |
| `flaky_evidence.log` | the captured timing-fence failure + analysis |
| `idle_rerun.log` | idle-machine full-suite rerun after the fix |

## Conventions

- Every claim is backed by a command line and exit code in the logs.
- Failures are recorded, not hidden: two timing-fence failures under CPU
  contention and the flakiness fix are documented in `performance_budgets.md`.
- Windows-only behavior is never claimed from Linux (see
  `windows_runtime_limitations.md`).
