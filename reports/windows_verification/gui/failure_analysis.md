# GUI Failure Analysis (standing procedure)

Status: **no runtime failures recorded yet** (runtime is BLOCKED —
`coverage_report.md`). This file documents the standing diagnosis-and-repair
procedure that the workflow run plugs into, so failures are handled per the
mission's rules, never papered over.

## Per-failure procedure (mission §18)

1. Capture UI state + screenshot (harness does both automatically per check).
2. Record failing AutomationId, control type, parent window (in the check
   row).
3. Record expected vs actual (harness fields).
4. Identify the source event handler (inventory row →
   `application.cpp`/`topbar.cpp`/…).
5. Trace the implementation path; find the root cause.
6. Minimal justified fix; preserve appearance/behavior.
7. Add regression coverage (engine-level test in the portable suite, or a
   harness check id + checkmap row).
8. Rebuild; rerun the failed check; rerun affected workflows; rerun the full
   relevant regression suite (686-test suite + ctest).

## Forbidden shortcuts (mission §18 + §1)

- No disabling/weakening tests for green results.
- No changing performance limits to pass.
- No broad warning suppressions without justification.
- No fake Windows runtime tests.
- No skipped-test conversion without documented justification.

## Known open items to watch in the run

- Timing-fence robustness (D-003 from Part III) — the fix makes fences
  outlier-tolerant without lowering thresholds; watch the Windows
  PerformanceBudget rows.
- UIA audit on a sessionless/locked desktop would produce legitimate
  failures — those are recorded as environment limitations, not app
  defects (the harness's environment-detection preamble distinguishes this).
