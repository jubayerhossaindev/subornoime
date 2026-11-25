# BDS Keyboard Mapping Verification Report

> **PHASE 3 (2026-11-24)**: National layout REWRITTEN from the phonetic prototype to the positional Jatiyo layout (official diagram L2/19-079 Figure 4 + BornoTSF factual reference; 121/121 engine evidence rows; Assamese ৰ/ৱ excluded; GPL facts-only clean-room). Full details: BDS_NATIONAL_LAYOUT_DISCREPANCY_REPORT.md; spec: docs/specifications/national-keymap.md. Clause-level BDS binding stays BLOCKED (GAP-01).
>
> **PHASE 2 (2026-11-24)**: no change to any layout mapping. The BDS 1738
> official keymap remains unavailable (BCC row 24 has no PDF; store-only),
> so the clause-level comparison stays BLOCKED (GAP-04). Revision identity
> of BDS 1738:2018 is now verified via Catalogue 2023 + BCC re-check
> (KV-02 PASS). Harness subset property: every Bangla-script code point the
> layouts emit (66 distinct) lies inside the official BDS 1520 repertoire.

**Mission**: COMPLETE BDS COMPLIANCE… (2026-11-24)
**Baseline**: branch `main`, HEAD `533b3e2`
**Standard under verification**: BDS 1738:2018 — Specification for Computer
Bangla Keyboard (First Revision). **Official full text: NOT OBTAINED** (see
source register). No clause number is cited or invented anywhere in this
report.

## 1. What could and could not be verified

| Verification target | Method | Result |
|---|---|---|
| Official mapping table (clause-level) | Requires the full standard text | **BLOCKED** — document not obtained; purchase route recorded |
| National-layout engine behaviour | Real InputSession pipeline + unit tests | **PASS** (mission-program level) |
| Bengali digit row | Executable test vs Unicode reference | **PASS** — U+09E6..U+09EF exact |
| Currency / punctuation / signs | Source + harness rows | **PASS** — ৳ U+09F3, । U+0964, ঁ, ং, ঃ emitted on documented keys |
| Shift/AltGr layer integrity | Unit tests incl. upstream-KLC parity | **PASS** — no overwrite of existing keys; upstream-empty slots stay empty |
| Non-interference across 7 layouts | Full 579-row harness + 586/586 suite | **PASS** — each layout keeps its documented identity |
| Physical scan-code driving (Probhat) | Scan-code KeyEvents through real session | **PASS** — all Probhat rows driven with physical scan codes |
| Windows physical keyboard behaviour | Requires Windows 10 x64 | **BLOCKED — WINDOWS RUNTIME UNAVAILABLE** |

## 2. Layout ownership model (the central BDS 1738 rule the mission imposed)

- Only the `National` layout is the candidate conformance target for the
  national mapping.
- Khipro, Probhat, Suborno, Borno, Avro, English are alternative layouts and
  were **not** forced into the national mapping.
- The National layout currently implements this project's own documented
  national-style conventions (explicit halant `X`, `Y` → য়, backtick → ়,
  `..` → ।, Latin digits → Bengali digits). **Whether this equals the official
  BDS 1738 table cannot be claimed until the purchased standard is read and a
  clean-room transcription of the official table is compared key-by-key.**
  No "BDS 1738 conformant" label is present anywhere in the product.

## 3. Key mapping matrix (engine-verified facts)

Character groups verified with exact code points (evidence:
`reports/standards/evidence-raw/bds_engine_rows_579.csv` and the new unit
tests):

| Group | Characters | Code points | Layouts verified | Result |
|---|---|---|---|---|
| Vowels | অ আ ই ঈ উ ঊ ঋ এ ঐ ও ঔ | U+0985..U+0994 | all phonetic + national | PASS |
| Consonants | ক..হ (full set) | U+0995..U+09B9 | all Bengali layouts | PASS |
| Vowel signs | া ি ী ু ূ ৃ ে ৈ ো ৌ | U+09BE..U+09CC | all Bengali layouts | PASS |
| Virama (hasanta) | ্ | U+09CD | Khipro (`kqq`), National (`X`), Probhat (0x35) | PASS |
| Nukta | ় | U+09BC | Probhat (AltGr+0x34), National (backtick), Khipro (`,`), phonetic (`+q` families) | PASS |
| RRA/RHA/YYA decomposition | ড় ঢ় য় | U+09A1 U+09BC / U+09A2 U+09BC / U+09AF U+09BC | Probhat/Khipro/national/phonetic | PASS — canonical decomposed forms |
| Bengali digits | ০-৯ | U+09E6..U+09EF | National (0-9), Probhat (number row) | PASS |
| Signs | ঁ ং ঃ | U+0981..U+0983 | all Bengali layouts | PASS |
| Punctuation | । | U+0964 | Probhat, National, Khipro (`.`) | PASS |
| Currency | ৳ | U+09F3 | Khipro, Avro (`$`), Probhat (Shift+0x05) | PASS |
| Conjunct composition | ক্ + ক | U+0995 U+09CD U+0995 | Khipro, National | PASS |

## 4. The two protected mission fixes (preserved and re-verified)

| Fix | Input | Expected Unicode | Verification at HEAD `533b3e2` |
|---|---|---|---|
| Khipro trailing virama | `kqq` | `ক্` = U+0995 U+09CD | `BdsConformanceTest.KhiproTrailingViramaFixPreservedU0995U09CD` PASS + harness rows |
| Probhat standalone nukta | AltGr + scan 0x34 | `়` = U+09BC | `BdsConformanceTest.ProbhatStandaloneNuktaFixPreservedU09BC` PASS + harness rows |

Both remain documented as their own layouts' mechanisms — neither is claimed
to be a BDS 1738 national-mapping key.

## 5. Regression obligations discharged

- Existing National layout: unchanged; full suite re-run green.
- Existing Suborno layout: unchanged; green.
- Existing phonetic layouts: unchanged; green.
- Khipro `kqq`: preserved; green.
- Probhat standalone nukta: preserved; green.
- Punctuation & modifier behaviour: green (see BDS_REGRESSION_REPORT.md).

## 6. Remaining gaps

1. Clause-level comparison against the official BDS 1738 mapping table —
   BLOCKED until an authorized copy is purchased and transcribed clean-room
   (planned corpus: key → expected character, one row per official cell).
2. Windows physical-keyboard validation (Caps Lock, repeat, switching) —
   BLOCKED — WINDOWS RUNTIME UNAVAILABLE.
3. IP clearance for any future exact-mapping conformance claim (Bijoy-linked
   foreword) — see BDS_IP_AND_LICENSING_REVIEW.md.

**Citation format used throughout**: BSTI | BDS 1738:2018 | clause NOT READ |
official document URL recorded in BDS_OFFICIAL_SOURCE_REGISTER.md |
Accessed 2026-11-24.
