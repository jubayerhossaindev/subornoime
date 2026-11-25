# BDS Character Repertoire Verification Report

> **PHASE 2 (2026-11-24)**: the official BDS 1520 document (obtained edition,
> 2011 Second Revision) has been read. Its Table 2 allocates 94 characters
> (92 Bengali-block + the Daari pair U+0964/U+0965 at positions E4/E5). The
> machine-verified comparison corpus lives at
> `docs/standards/compliance/BDS_1520_OBTAINED_EDITION_ALLOCATION.csv`
> (EV-C-01..03). Everything below (engine sweep 128/128 etc.) remains valid
> and was re-run at the phase-2 commit.

**Mission**: COMPLETE BDS COMPLIANCE… (2026-11-24)
**Standard under verification**: BDS 1520:2018 — Specification for Bangla
Coded Character Set for Information Interchange. **Official full text: NOT
OBTAINED**; the repertoire verification below therefore binds to the Unicode
Bengali block (U+0980–U+09FF) as the publicly documented technical model of
the standard, with clause binding BLOCKED.

## 1. Method

A machine-readable inventory of the complete Bengali block was produced by
the engine itself and independently cross-checked against Python
`unicodedata` (an authority independent of both the engine and this report's
author):

1. `scripts/layout_verify/bds_repertoire_verify.cpp` (part A) — sweeps all
   128 code points U+0980..U+09FF through the engine's unicode module:
   UTF-8 validity, code-point count, UTF-16 units, `is_bengali_codepoint`,
   NFC stability/idempotence.
2. `scripts/layout_verify/bds_repertoire_crosscheck.py` — for every row:
   assigned status, official Unicode name, general category, reference NFC/NFD,
   reference UTF-8 bytes vs engine bytes.

## 2. Results

| Check | Expected | Actual | Verdict |
|---|---|---|---|
| Code points swept | 128 (U+0980..U+09FF) | 128 | — |
| Assigned characters | Unicode-defined | 96 assigned + 32 reserved/unassigned | matches Unicode exactly |
| UTF-8 validity | 128/128 | 128/128 | PASS |
| Code-point count | 1 per character | 128/128 | PASS |
| `is_bengali_codepoint` | accepts block | 128/128 | PASS |
| UTF-8 bytes == reference | 96/96 assigned | 96/96 | PASS (0 mismatches) |
| UTF-16 units | BMP single unit | 128/128 | PASS |
| NFC idempotent | 128/128 | 128/128 | PASS |
| NFC stability flag vs reference | equal | equal for all 128 | PASS |
| Sanitizer build (ASan/UBSan) re-run | same results | 128/128 | PASS |

Evidence: `reports/standards/evidence-raw/bds_repertoire_inventory.csv`,
`bds_repertoire_crosscheck.csv`.

## 3. BDS-critical characters (direct engine evidence)

| Character | Code point | Produced by | Evidence |
|---|---|---|---|
| Virama / hasanta ্ | U+09CD | Khipro `kqq`; National `X`; Probhat scan 0x35 | KEY-01, harness rows, unit tests |
| Nukta ় | U+09BC | Probhat AltGr+0x34; National backtick; Khipro `,`; phonetic tokens | KEY-02, FP-159/160, unit tests |
| ক় | U+0995 U+09BC | Khipro consonant+nukta | KEY-03, UNI-004 |
| ড় | U+09A1 U+09BC (canonical decomposition of U+09DC) | Probhat ড+nukta; Khipro `rf` | KEY-04/05, FP-166, NFC tests |
| ঢ় | U+09A2 U+09BC | Probhat ঢ+nukta; Khipro `rff` | KEY-06/07 |
| য় | U+09AF U+09BC | Probhat 0x2C; national `Y`; Khipro | KEY-08/09 |
| Bengali digits ০-৯ | U+09E6..U+09EF | National `0-9`; Probhat number row | digit tests, FP-122/141 |
| Currency ৳ | U+09F3 | Khipro; Avro `$`; Probhat Shift+0x05 | source + harness |
| Dari । | U+0964 | Probhat 0x34; National `..`; Khipro `.` | source + harness |
| Candrabindu ঁ / anusvara ং / visarga ঃ | U+0981..U+0983 | all Bengali layouts | harness rows |
| Reserved positions (e.g. U+0984, U+0991, U+0992, U+09E4, U+09E5) | — | never emitted; safe handling | `ReservedBengaliCodePointsDoNotCorruptEngine` |

## 4. Per-character inventory fields (mission-required)

The inventory CSV carries, for every one of the 128 code points: character,
code point, UTF-8 bytes, UTF-16 units, engine availability, NFC reference,
Unicode name, category, assigned status, crosscheck verdict. The BDS "official
code position" column is intentionally **absent** — the official code table
was not obtained, and inventing BDS code positions is forbidden. The CSV
instead records `NOT READ (BLOCKED)` semantics by omission plus this note.

## 5. Findings on the mission's explicit BDS 1520 checks

1. No missing character among the BDS-relevant set (all producible).
2. No unexpected character allocation (engine emits only repertoire code points).
3. No duplicate code assignment (579/579 harness rows byte-exact).
4. Correct digit mapping (U+09E6..U+09EF verified on two layouts).
5. Correct sign mapping (U+0981..U+0983 + virama/nukta).
6. Correct combining-mark handling (canonical order only; ccc=7 nukta before ccc=9 virama where sequences occur).
7. Correct conjunct sequences (consonant+virama+consonant exact).
8. Correct encoding (UTF-8/UTF-16 byte-exact vs reference).
9. Correct interchange (plain NFC-stable UTF-8 commits; no private-use, no surrogates).
10. Reserved position handling safe and never emitted.
11. Assamese ৰ/ৱ: not added to any layout; encodings handled correctly; the
    BDS-mandate question stays open and will be cited when the standard is read.
12. Clause-level code-table comparison: BLOCKED (document not obtained).

## 6. Verdict

**Character repertoire: VERIFIED at Unicode-model level with executable
evidence (0 mismatches across 128 code points and 579 session rows).
Clause-level BDS 1520 conformance: BLOCKED** until an authorized full copy is
obtained via the recorded official purchase route.
