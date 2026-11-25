# BDS Unicode Verification Report

> **PHASE 2 (2026-11-24)**: UTF-16/NFC/round-trip verification now also runs
> against the OFFICIAL BDS 1520 allocation (94 rows, obtained edition) —
> `Bds1520OfficialCorpusTest.*` 5/5 PASS; the 3 composition-exclusion rows
> (DC/DD/DF) are verified to normalize to their canonical decompositions
> (U+09A1 U+09BC / U+09A2 U+09BC / U+09AF U+09BC) and never recompose,
> matching `bds_normalizationtest_engine.csv` NT-09DC/09DD/09DF.

**Mission**: COMPLETE BDS COMPLIANCE… (2026-11-24)
**Authorities**: The Unicode Standard (Bengali chart, current revision,
byte-identical in 18.0.0 — SRC-11/12), UAX #15 normative clauses (SRC-14),
official `NormalizationTest.txt` (SRC-15). Independent reference:
Python `unicodedata`.

## 1. Scalar-level verification

Every BDS-relevant output was verified as exact Unicode scalar sequences —
never by visual appearance:

| Sequence | Expected scalars | Engine actual | Evidence |
|---|---|---|---|
| Khipro trailing virama | U+0995 U+09CD | identical (byte-verified) | UNI-003, BdsConformanceTest, 579-row harness |
| Probhat standalone nukta | U+09BC | identical | UNI-005, BdsConformanceTest |
| Probhat ড+nukta | U+09A1 U+09BC | identical | BdsConformanceTest.ProbhatNuktaAfterConsonantFormsU09A1U09BCSequence |
| ড় / ঢ় / য় | U+09A1 U+09BC / U+09A2 U+09BC / U+09AF U+09BC | identical | KEY-04..09, prior UNI-006..008 |
| Bengali digits | U+09E6..U+09EF | identical | digit unit tests, FP-122 |

## 2. Sequence-order verification

- No unordered or invented sequences: the only reordering observed anywhere
  is the **standard-mandated canonical reordering** (nukta ccc=7 sorts before
  virama ccc=9) — documented, expected, and Unicode-correct (prior mission
  FP-166; unchanged).
- Conjuncts commit as consonant+virama+consonant; no reordering.

## 3. UTF-8 / UTF-16 verification

- Full-block sweep: 128/128 UTF-8 encodings byte-identical to the Unicode
  reference (`bds_repertoire_crosscheck.csv`, utf8_match=TRUE).
- 128/128 correct UTF-16 units (single BMP unit; block contains no astral or
  surrogate content).
- Engine rejects invalid UTF-8 (`SUBORNO_STATUS_INVALID_UTF8`); fuzz corpus
  `0012_invalid_utf8` green; suite 586/586 under ASan/UBSan clean.

## 4. Normalization conformance (UAX #15)

UAX15-C3 requires processes that transform into a normalization form to
reproduce `NormalizationTest.txt` results. The engine purports **NFC only**;
therefore the NFC-relevant columns were executed for every Bengali-block
line of the official file:

| Test | Expected | Actual |
|---|---|---|
| Official Bengali lines (U+09CB, U+09CC, U+09DC, U+09DD, U+09DF, 2 tilde-overlay rows) | NFC columns match | 7/7 PASS |
| Composition exclusion (U+09A1 U+09BC stays decomposed) | no recomposition | PASS (KEY-04) |
| Singleton decomposition (U+09DC → U+09A1 U+09BC; U+09DD → U+09A2 U+09BC; U+09DF → U+09AF U+09BC) | decomposes | PASS (KEY-05/07/09) |
| NFC composition works where mandated (U+09C7 U+09BE → U+09CB; U+09C7 U+09D7 → U+09CC) | composes | PASS (KEY-11) |
| Virama sequences never swallowed by normalization (ক্ stable; ক্+ক stable) | byte-stable | PASS (KEY-01, KEY-10, unit tests) |

Evidence: `bds_normalizationtest_engine.csv`, `bds_key_sequences.csv`.

## 5. No-loss / no-duplication / no-corruption

| Requirement | Method | Result |
|---|---|---|
| No character dropping | committed == expected bytes in all 579 session rows | PASS |
| No duplicated marks | repeated-token probes (`kqqqq` idempotence; nukta repeats only on physical repeats) | PASS |
| No mojibake / invalid encoding | every commit `is_valid_utf8` TRUE | PASS |
| No private-use / surrogate leakage | block sweep + harness byte checks | PASS |
| Copy-paste compatibility | commits are plain NFC-stable UTF-8 text | PASS |

## 6. Blind-normalization prohibition (mission rule)

The engine applies NFC at commit — this is the documented pipeline contract.
The mission rule "no blind normalization without verifying that required
sequences survive" was applied as an explicit test obligation: ক্, ়, ক়,
ড়-decompositions are all asserted NFC-stable **before** trusting the
normalizer (Section 4). NFC/NFD equivalence and exact byte-level output are
reported separately (Section 3 vs Section 4).

## 7. Unicode version policy

Current Unicode is 18.0.0 (SRC-13). The Bengali chart is unchanged in 18.0.0
(SRC-11 == SRC-12 content). BDS 1738's foreword extract references Unicode
10.0; per the mission rule **no** version substitution is claimed: the engine
targets the current Bengali block, and no BDS clause is quoted to justify or
forbid it — the binding remains BLOCKED until the full text is read.

## 8. Verdict

**Unicode correctness: FULLY VERIFIED with executable, reproducible evidence**
(scalar, sequence order, UTF-8, UTF-16, NFC conformance, no-loss, no-dup,
no-corruption). This verdict does not extend to BDS clause conformance,
which remains BLOCKED for lack of authorized standard texts.
