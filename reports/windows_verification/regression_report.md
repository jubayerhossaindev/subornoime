# Part III Regression Report — Windows Wiring and Linux Execution

## Executed on Linux (this session, Release + Debug + sanitizers)

| Suite | Linux result | Evidence |
|---|---|---|
| Full unit suite (686 tests, 80 suites) | PASS | `part_iii/logs/release_ctest.log`, `test_count_reconciliation.log` |
| KeyEventMatrix (11) | PASS | `part_iii/logs/targeted_runs.log` |
| CandidateNavigation (6) + candidate suites (29) | PASS | same |
| DiagnosticsTest (7) | PASS (+ TSan run) | `tsan.log` |
| StabilitySoak (4) — 50k bursts, 100k clamp appends, 2k focus cycles, 20k churn | PASS ×2 repeats, peak RSS ~12.8 MB | `soak_and_budgets.log` |
| PerformanceBudget (6) | PASS idle; **2 timing fences failed under 3-way CPU contention** (documented defect D-003, fix in progress) | `soak_and_budgets.log`, `flaky_evidence.log` |
| Integration + fuzz smoke (CTest 3/3) | PASS | `release_ctest.log` |

Test-count reconciliation: **34 unique Part III test cases** (DiagnosticsTest
7, not 8) — resolved and corrected in the report; 0 disabled; the same
reconciliation executes on Windows in the verification workflow
(`test_counts.txt`).

## Windows execution (BLOCKED — the workflow runs it)

The verification workflow executes on the GitHub runner:

1. Release ctest (all suites incl. `suborno.windows.tsf_contract`,
   `suborno.windows.app_smoke`).
2. Explicit Part III filters with per-area logs
   (`part3_input.log`, `part3_candidates.log`,
   `part3_diagnostics_soak_budget.log`).
3. GoogleTest listing/execution count capture (`test_counts.txt`).
4. Debug ctest.

Constraint kept: no test disabled, skipped or weakened to reach green; the
2 timing-fence failures from Linux are being fixed by making the fences
statistically sound (median + bounded outliers), NOT by lowering the
thresholds — see `part_iii/performance_budgets.md`.

## Status

- Linux Part III regression: PASS (with the disclosed D-003 timing-fence
  robustness fix in flight).
- Windows Part III regression: BLOCKED (workflow wired; runs on activation).
