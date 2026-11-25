# Part III — Test Count Reconciliation

Mission §9. Resolves the 34-vs-35 discrepancy in the Part III acceptance wave.

## The discrepancy

The Part III wave report (REFACTORING_REPORT.md §35, pre-correction) claimed
"34 new tests" but listed category counts summing to 35:

| Category | Report claim | Actual |
|---|---|---|
| KeyEventMatrix | 11 | 11 |
| CandidateNavigation | 6 | 6 |
| DiagnosticsTest | **8** | **7** |
| StabilitySoak | 4 | 4 |
| PerformanceBudget | 6 | 6 |
| **Sum** | **35** | **34** |

The arithmetic sum of the listed categories was 35 because DiagnosticsTest was
listed as 8. The correct count is 7; the total (34) was correct.

## Resolution method

Executed on the Release build (`build/final`, commit `909dec1` + verification
corrections):

```bash
cd build/final
./suborno_unit_tests --gtest_list_tests            # discovery, no execution
./suborno_unit_tests --gtest_filter="<part III>"   # executed run
./suborno_unit_tests                               # full unit suite
```

Log: `logs/test_count_reconciliation.log`.

## Actual results (GoogleTest, Release binary)

### Part III suites — discovered test names

```
DiagnosticsTest.
  SeverityThresholdFiltersLowerLevels
  ScrubUserTextEmitsOnlyACodepointMarker
  RingBufferStaysBoundedUnderHeavyLogging
  ConcurrentProducersNeverLoseOrCorruptEntries
  FileSinkWritesEntriesAndClosesCleanly
  SinkFailureIsReportedAndLoggingContinues
  NoUserTextEverReachesTheLog
```

Per-suite counts from `--gtest_list_tests` (unique TEST cases, no
parameterized/repeated counting — the project uses no value-parameterized or
typed suites in these files):

- `KeyEventMatrix` = 11
- `CandidateNavigation` = 6
- `DiagnosticsTest` = 7
- `StabilitySoak` = 4
- `PerformanceBudget` = 6
- **Unique Part III test cases = 34**

### Executed runs

- Part III filter run: `[==========] 34 tests from 5 test suites ran.`,
  `[  PASSED  ] 34 tests.`, `EXIT_CODE=0`.
- Full unit suite: `[==========] 686 tests from 80 test suites ran.`,
  `[  PASSED  ] 686 tests.`, `EXIT_CODE=0`.
- CTest suite count in Release: 3 (`suborno.unit`, `suborno.integration`,
  `suborno.fuzz_smoke`) — 100% passed.

### Disabled / skipped / failed

- `--gtest_list_tests | grep -c DISABLED` → `0`
- Skipped: none (no GTEST_SKIP in the Part III files)
- Failed: 0 in the Release full run

## Report correction applied

`docs/engineering/REFACTORING_REPORT.md` §35 was corrected: DiagnosticsTest 8 →
7, with a note pointing to this reconciliation file. The correction is a
documentation fix only — no production source changed.

## Verdict

- GoogleTest test-case count (Part III, unique): **34** — report claim of 34
  was right, its category breakdown was wrong (DiagnosticsTest).
- Discrepancy: **resolved and corrected in the report**.
