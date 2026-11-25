# Part III — Input Reliability (Auto-Repeat) Verification

Mission §1. Scope: `repeat_count` handling across every keyboard-event path,
executed on Linux (Release binary, commit `909dec1` + verification
corrections).

## Source inspection (all keyboard-event paths)

Every path that can receive a `KeyEvent` was inspected:

1. **Base default `layout::InputSession::append_key`**
   (`include/suborno/layout-api/input_session.hpp`) — inherited by phonetic
   sessions (`PhoneticInputSession`), `LayoutInputSession` (used by the khipro
   wrapper) and the English layout session. Since the v20 wave this loops
   `repeat_count` times: `repeat_count == 0` → 1, `repeat_count >
   max_input_code_points (256)` → atomic `input_limit` reject (nothing
   appended), early exit on `input_limit`/`invalid_utf8`, and key-up plus
   Ctrl/Alt/Win events are suppressed before any text is produced.
2. **National layout** (`src/layouts/national/national_layout.cpp`, append_key
   at :497) — same contract: caps repeats at 256 and rejects atomically, loops
   over its composed output otherwise.
3. **Probhat layout** (`src/layouts/probhat/probhat_layout.cpp`, append_key at
   :246) — same contract (loop at :259–:265).
4. **Khipro layout** (`src/layouts/khipro/khipro_layout.cpp`, append_key at
   :45) — delegates to `LayoutInputSession`, which does NOT override
   `append_key`, so it inherits the base default (path 1).

Consistency verdict: all four paths enforce one append per repeat, an atomic
reject above 256 repeats, and identical suppression rules (key-up, Ctrl, Alt,
Win). National/Probhat/Borno-compat difference: national and probhat override
`append_key` because they compose fixed-layout output per key; the semantics
match the base default's. `grep repeat_count` across
`src/layouts/borno_compat/` finds no key-level repeat logic — the
BornoTSF-compatibility engines are fed text (`append_utf8`), not keys, so
repeat handling belongs to the TSF layer there; no gap.

Historical defect (found in the v20 wave, fixed there, re-verified here): the
base default previously appended once per event regardless of `repeat_count` —
held keys lost input on the phonetic path. `AutoRepeatTypesOneCharacterPerRepeat`
now pins the fix.

## Executed tests (all Release, exit code 0)

Commands and results (log: `logs/targeted_runs.log`,
`logs/release_ctest.log`):

| Command (filter) | Result | Exit |
|---|---|---|
| `--gtest_filter=KeyEventMatrix.*` | 11 passed | 0 |
| `--gtest_filter=KeyEventTest.*` | 11 passed | 0 |
| `--gtest_filter=*LayoutTypingRegression*` | 17 passed | 0 |
| `--gtest_filter=NationalLayoutTest.*:ProbhatLayoutTest.*:KhiproLayoutTest.*` | 59 passed | 0 |
| full `suborno.unit` (686 tests) | 686 passed | 0 |

## Required scenario coverage (mission list 1–10)

1. Single key press — `KeyEventMatrix.EveryPhysicalKeyPassesThroughWithoutCorruption`
   (exhaustive sweep over `PhysicalKey` 0–200), `KeyEventTest.*`.
2. Multiple repeated presses — `AutoRepeatTypesOneCharacterPerRepeat`
   (repeat_count=5 → raw `aaaaa`, committed text equals the text-driven path's
   preedit for `aaaaa`).
3. Maximum accepted repeat count — the loop cap: 256 repeats append exactly
   256 characters (`StabilitySoak.MixedChurnThroughTheKeyMatrixStaysConsistent`
   sends repeat_count=3 keys; `PerformanceBudget.AutoRepeatLoopStaysWithinCeiling`
   sends repeat_count=256).
4. Repeat exceeding capacity — `AutoRepeatAboveSessionLimitIsRejectedAtomically`
   (repeat 257 → `input_limit`, previous composition `am` survives).
5. Empty/invalid event — `PhysicalKey::unknown` (0) is inside the exhaustive
   sweep; key-up and modifier-suppressed paths return `empty`.
6. Repeated phonetic input — `AutoRepeatTypesOneCharacterPerRepeat` (phonetic
   session, 5 repeats).
7. Repeated Bengali character input — covered by the phonetic pipeline
   equivalence (raw `aaaaa` → committed multi-`আ` text equal to
   `append_utf8("aaaaa")`); plus the full corpus tests
   (`bds1520_official_corpus_test`, `linguistic_regression_test`) in the unit
   suite.
8. Buffer boundary — `AutoRepeatAboveSessionLimitIsRejectedAtomically` and
   `StabilitySoak.UnboundedRawInputNeverGrowsPastTheLimit` (clamp at exactly
   256 codepoints).
9. Input after capacity exhaustion — the soak continues appending after the
   clamp; every later append reports `input_limit` and the composition still
   commits the buffered 256 exactly.
10. Regression against existing layout behavior — National/Probhat/Khipro
    layout suites (59 tests) plus `*LayoutTypingRegression*` (17) all green on
    the repeat-fixed code.

## ASan/UBSan

The complete Part III suites including `KeyEventMatrix.*` were re-run under
ASan+UBSan (`detect_leaks=1`, `halt_on_error=1`); results in
`logs/asan_ubsan.log` and `regression_matrix.md`.

## Verdict

`PORTABLE CORE — VERIFIED`. Auto-repeat is respected on all four paths,
repeats are bounded, nothing is silently dropped or duplicated, and existing
Unicode/composition behavior is unchanged (686/686 unit tests green).
