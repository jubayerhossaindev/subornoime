# Part III — DiagnosticLog Privacy Verification

Mission §4. Scope: `suborno::core::DiagnosticLog` privacy contract, type-level
API audit, executed tests, and sanitizer runs.

## Type-level privacy contract (executed API audit — not comments)

The claim was audited against the **actual public API** in
`include/suborno/core/diagnostics.hpp` (post-format audit):

| API element | Findings |
|---|---|
| `DiagnosticLog::instance()` | Meyer's singleton; no text channel |
| `log(severity, category, message)` | accepts an **arbitrary caller-supplied string** — the API does not and cannot type-forbid user text inside `message`; the guard is procedural, not compiler-enforced |
| `scrub_user_text(std::string_view)` | verified implementation: emits a codepoint-count marker (e.g. `[3 codepoints]`), never the characters — pinned by `ScrubUserTextEmitsOnlyACodepointMarker` |
| `snapshot()` / file sink / severity threshold | entries carry severity/category/message/sequence only — **no session, composition, candidate or committed-text fields exist in `DiagnosticEntry`** |
| No API accepts session/composition types | verified — `composition.hpp` types appear nowhere in the diagnostics interface |

**Honest classification**: the guarantee is structural (no dedicated
user-text channel; scrub helper as the only text-volume mechanism; tests
enforce the documented workflow) — but a careless caller could still
concatenate user text into `message`. Not compiler-enforced. Stated as such.

## Production wiring (executed audit — significant honest finding)

`grep -rn "log_diagnostic\|DiagnosticLog::instance" src/ include/` outside
`core/diagnostics` → **ZERO call sites in production code**. Nothing in the
portable core logs anything yet, so no user text can flow today
(trivially). The facility is implemented and tested but not yet wired into
the TSF/settings lifecycle — that wiring is Windows-side work. Classify:
`DIAGNOSTIC FACILITY — VERIFIED`, `PRODUCTION WIRING — NOT PRESENT`.

## Executed tests (Release, all exit 0)

`DiagnosticsTest.*` — 7 unique tests (reconciliation: 7, not 8):

1. `SeverityThresholdFiltersLowerLevels`
2. `ScrubUserTextEmitsOnlyACodepointMarker`
3. `RingBufferStaysBoundedUnderHeavyLogging` (fixed 512-entry contract)
4. `ConcurrentProducersNeverLoseOrCorruptEntries` (multi-threaded producers)
5. `FileSinkWritesEntriesAndClosesCleanly`
6. `SinkFailureIsReportedAndLoggingContinues`
7. `NoUserTextEverReachesTheLog` (raw input, Bengali typed text, phonetic
   sequences, candidate contents, composition buffers, user fragments)

Command + result: `./suborno_unit_tests --gtest_filter="DiagnosticsTest.*"`
→ 7 passed, exit 0 (`logs/targeted_runs.log`).

## Sanitizers (executed)

- **TSan**: `build/tsan` (`-fsanitize=thread`), `DiagnosticsTest.*` →
  7/7 PASSED, `TSAN_EXIT=0` (`logs/tsan.log`) — the concurrency-sensitive
  producer suite is data-race clean under ThreadSanitizer.
- **ASan+UBSan**: the same suite runs inside the Part III sanitizer filter —
  see `regression_matrix.md` and `logs/asan_ubsan.log`.

## Accidental-logging search (executed grep sweeps)

Searched `src/` for log calls carrying session state: raw keyboard input,
Bengali typed text, phonetic sequences, candidate contents, dictionary
user-specific entries, full composition buffers, user text fragments —
**zero production logging exists at all** (finding above). The only logging
executions are the test's own synthetic entries.

## Verdict

`PASS` for the implemented facility's tested behavior + `PASS` under TSan
for concurrency; the honest caveats (procedural-not-compiler-enforced
privacy; zero production wiring yet) are recorded above and mirrored in the
final acceptance matrix as `PARTIAL` where the matrix row is about the whole
diagnostics story.
