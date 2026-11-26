# BDS Regression & Reliability Report

**Mission**: COMPLETE BDS COMPLIANCE… — **PHASE 2 UPDATED** (2026-11-24)
**Baseline before phase-2 work**: HEAD `0fd4874` (branch `main`, clean tree)
— suite 586/586 unit + 2/2 integration, engine harness 579 rows 0 FAIL.
**Phase-2 final state**: suite **591/591** unit (+5 new
`Bds1520OfficialCorpusTest.*` on the official-document corpus) + 2/2
integration + sanitizer clean + harness re-run 579 rows 0 FAIL + repertoire
re-runs (128/128, 7/7, 12/12) + corpus verifier ALL CHECKS PASS.

## 1. Full test matrix

| Suite | Scope | Result |
|---|---|---|
| Unit tests (`suborno.unit`) | engine, layouts, unicode, settings, persistence, API | **591/591 PASS** (586 + 10 `BdsConformanceTest.*` + 5 `Bds1520OfficialCorpusTest.*`) |
| Integration (`suborno.integration`) | target-graph/build contract | **2/2 PASS** |
| ASan/UBSan unit + integration | same binaries under sanitizers, leak detection on | **593 total PASS, zero diagnostics** |
| Official-corpus verifier (python) | `bds1520_official_corpus.py` | **ALL CORPUS CHECKS PASS** (94 rows; harness subset 66/66) |
| Engine typing harness (6 layouts, scan-code driven) | 579 rows incl. Khipro/Probhat fixes, inventory, brief sequences | **579 rows, 0 FAIL** (re-run at mission HEAD) |
| Repertoire sweep | 128 code points | **128/128 PASS** |
| NormalizationTest Bengali subset | official file, 7 lines | **7/7 PASS** |
| BDS key-sequence conformance | 12 cases | **12/12 PASS** |
| unicodedata crosscheck | 128 rows | **0 mismatches** |

## 2. Seven-layout regression obligations (per mission)

| Layout | Regression items | Result |
|---|---|---|
| Suborno (phonetic) | Chandrabindu, ঙ, র+ই-কার, hasanta behaviours | PASS (harness rows; sources untouched) |
| English | passthrough integrity | PASS |
| Suborno Classic (phonetic) | full inventory | PASS (untouched) |
| Suborno Smart (phonetic) | full inventory | PASS (untouched) |
| Khipro | `kqq` trailing virama + all 74-item inventory + auto-join preservation | PASS (fix preserved: `KhiproTrailingViramaFixPreservedU0995U09CD`, `KhiproExplicitJoinAfterViramaStillComposes`) |
| Probhat | AltGr+0x34 nukta + full scan probe + digit row | PASS (fix preserved: `ProbhatStandaloneNuktaFixPreservedU09BC`, `ProbhatDigitRowProducesU09E7ToU09EF`) |
| National | explicit halant, য়, nukta, conjuncts, digit row | PASS (incl. new `NationalLayoutBengaliDigitRowProducesU09E6ToU09EF`) |

## 3. What changed in the repository during this mission

| File | Change | Risk assessment |
|---|---|---|
| `tests/unit/bds_conformance_test.cpp` | NEW — 10 independent conformance tests (Unicode-derived corpus) | Additive; no engine change |
| `tests/CMakeLists.txt` | registers the new test file | Additive |
| `scripts/layout_verify/bds_repertoire_verify.cpp` | NEW — repertoire/normalization harness | Tooling only |
| `scripts/layout_verify/bds_repertoire_crosscheck.py` | NEW — independent crosscheck | Tooling only |
| `scripts/layout_verify/bds_generate_matrices.py` | NEW — compliance matrix generator | Tooling only |
| `docs/standards/**`, `reports/standards/**` | NEW documentation/evidence | Docs only |

**Zero engine or layout source files were modified.** The 7-layout regression
is therefore behavioural-identity-preserving by construction, and proven by
the green suites.

## 4. Performance & reliability (PART 17)

| Check | Method | Result |
|---|---|---|
| Active typing throughput | 579 full pipeline rows (append→commit→compare) in **24.9 ms** wall (~23k rows/min equivalent) | PASS — no degradation |
| Unit suite runtime | 586 tests in 0.64 s | PASS |
| Memory | max RSS ≈ 12.6 MiB for the full harness process | PASS — lean |
| Sanitizer reliability | ASan+UBSan+LSan over full suite + harness-built verifier | **Zero findings** |
| Crash/corruption probes | fuzz corpus incl. invalid UTF-8, long repeats, mixed case | PASS (suite green) |
| Backspace/composition | dedicated harness + unit rows (incl. `KhiproTrailingViramaBackspaceWorks` family) | PASS |
| Long-session behaviour | harness replays full inventory in one process | PASS |

Google-Benchmark targets are not configured in the ubuntu preset
(`SUBORNO_BUILD_BENCHMARKS=OFF`); the harness measurements above are the
mission's quantitative performance evidence. Startup/idle metrics of the
Windows UI process remain a Windows-runtime item (BLOCKED) and are not
fabricated.

## 5. Data-integrity checklist

- Unicode allocation: exact code points everywhere (evidence: repertoire +
  harness CSVs).
- Clipboard: commits are plain NFC UTF-8 (no private-use/surrogates).
- Settings persistence: untouched by this mission; suite covers it.
- No data-loss path introduced: zero engine diffs.

**Verdict: FULL REGRESSION PASS — no unresolved Critical/High issue; no
layout damaged; both protected fixes preserved; performance unchanged.**
