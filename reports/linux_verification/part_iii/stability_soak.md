# Part III — Stability and Soak Verification

Mission §5. Scope: every `StabilitySoak` test, repeated Release runs with
resource metrics, plus ASan/UBSan instrumentation.

## The four workloads (executed, Release binary)

| Test | Workload | Assertions per step |
|---|---|---|
| `RapidTypingBurstsPreserveCompositionAcrossCommits` | 200 bursts × 250 keystrokes = **50,000 keystrokes**, alternating commit/reset at burst boundaries | buffer ≤ 256 cp; candidate index valid; commit success |
| `UnboundedRawInputNeverGrowsPastTheLimit` | **100,000 appends** without commit | clamp at exactly 256 cp; `input_limit` reported; final commit returns the buffered input |
| `FocusChangeChurnRecoversEveryCycle` | **2,000 app-switch cycles** (type → reset → verify-empty → type → commit `তুমি`) | snapshot empty after reset; exact commit text |
| `MixedChurnThroughTheKeyMatrixStaysConsistent` | **20,000 mixed ops** (append/repeat-key/backspace/candidate-tap/reset, fixed pattern) | bounded buffer + valid index every step; final `ami`→`আমি` commit |

## Resource metrics (Release, `getrusage` wrapper, log: `logs/soak_and_budgets.log`)

Two full repeats to test repeatability:

| Run | Wall | Exit | Peak child RSS |
|---|---|---|---|
| 1 | 68.34 s | 0 | 12,788 KiB |
| 2 | 67.41 s | 0 | 12,828 KiB |

- Duration delta between repeats: **1.4%** — no cumulative slowdown.
- Peak RSS delta: **+40 KiB** (0.3%) — bounded working set; the raw-buffer
  clamp is the mechanism that keeps memory flat (a runaway composition would
  grow unbounded and would be caught by workload 2).
- Note: these wall times were measured while a concurrent ASan/UBSan suite
  occupied the second core (2-core sandbox); idle-machine numbers are
  proportionally lower. The stability claims do not depend on wall time.
- Maximum input-buffer occupancy: structurally capped by
  `max_input_code_points = 256`, asserted every step; maximum candidate count
  bounded by `candidate_limit = 16` (asserted via index validity and the
  candidate suite).

## Sanitizer instrumentation

The same four tests ran under **ASan + UBSan** (`detect_leaks=1`,
`halt_on_error=1` — leak detection aborts the run): see `logs/asan_ubsan.log`
and `regression_matrix.md`. Under instrumentation the suite takes ~30 minutes
(vs ~68 s Release) — expected slowdown, no failures. TSan applies to the
concurrency-sensitive `DiagnosticsTest` producers, not to these
single-threaded sessions; see `diagnostic_privacy.md`.

## Honest scope statement

This soak proves the tested invariants held for ~172,000 executed operations
across five runs (Release ×2 + sanitizer runs) with bounded memory and zero
failed invariants, exceptions, crashes or sanitizer reports. It does **not**
prove the absence of all memory-safety defects — no soak can; the sanitizer
runs cover executed paths only, and TSan/PageHeap on Windows remain open
items for the native verification.
