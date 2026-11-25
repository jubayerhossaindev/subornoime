# BDS BSCII (BDS 1935:2018) Interoperability Assessment

**Mission**: COMPLETE BDS COMPLIANCE… — **PHASE 2 UPDATED** (2026-11-24)
**Standard**: BDS 1935:2018 — Bangladesh Standard Code for Information
Interchange (BSCII). **Official full text: STILL NOT OBTAINED** (store-only;
no approval). PHASE 2 upgrade: the scope sentence is now verified from the
official BSTI Catalogue 2023 (SRC-19, p.123): the standard "specifies Bangla
Character Codes intended for the interchange of information among data
processing systems including the recording of data in the form of codes on
media… applicable to alphabets of the Bangla script" (13 pages, Gr-C, ET).
This assessment therefore fixes the **applicability boundary**, not clause
conformance.

## 1. Applicability determination

| Question | Determination | Basis |
|---|---|---|
| Is SubornoIME required to emit BSCII codes? | **No** | SubornoIME is a Unicode IME; no official source read on the access date imposes a legacy interchange code on a Unicode IME. Introducing 8-bit BSCII into the Unicode pipeline would *break* Unicode correctness (BDS 1520 model) |
| Is a BSCII encoder/decoder required now? | **No — and none was built** | Building one without reading the official code table would fabricate requirements (mission rule) |
| Is a separate compatibility module the right architecture if ever needed? | **Yes — recorded as a design constraint** | Legacy conversion must be an explicit, isolated layer; never mixed into default Unicode output |
| Is legacy document interoperability required? | **Unresolved** | Depends on the standard's real scope; BLOCKED |

## 2. What WAS verified (Unicode pipeline integrity)

The mission's actual interchange obligations — that the IME's output remains
clean, lossless, correct Unicode — were verified with executable evidence:

| Check | Result | Evidence |
|---|---|---|
| Default output pure Unicode (UTF-8 internal, UTF-16 at TSF boundary) | PASS — no 8-bit legacy path exists anywhere in `src/` | architecture review + block sweep |
| Round-trip integrity through the IME | PASS — 579/579 harness rows committed byte-identical text | `bds_engine_rows_579.csv` |
| Invalid code handling | PASS — invalid UTF-8 flagged (`SUBORNO_STATUS_INVALID_UTF8`), no crash, no mojibake; fuzz corpus `0012_invalid_utf8` green | unit suite 586/586 (ASan/UBSan clean) |
| Reserved/undefined code safety | PASS — reserved Bengali positions never emitted; safe under normalization | `ReservedBengaliCodePointsDoNotCorruptEngine` |
| Error reporting | PASS — status-code contract (C API) exercised by coverage tests | `c_api_coverage_test.cpp` |

## 3. Explicitly NOT done (and why)

1. **No BSCII code table was implemented or inferred** — the table was never
   read (the full text is still store-only); inventing it is forbidden.
   PHASE 2 note: the 8-bit code table inside the OBTAINED BDS 1520 (Second
   Revision) document is a related-but-distinct artifact — BDS 1520's own
   coded representation, NOT the BSCII table; it was NOT used to infer
   anything about BDS 1935's code table.
2. **No legacy import/export feature was added** — no requirement source
   establishes it; adding one would be an invented requirement and an
   architecture change without mandate.
3. **No round-trip BSCII↔Unicode tests** — impossible without the official
   table; recorded as BLOCKED, not skipped.

## 4. If BSCII interoperability is ever mandated (future route)

1. Purchase the full BDS 1935:2018 via the recorded official route.
2. Transcribe the official code table into a clean-room data file (with
   clause references).
3. Design `src/unicode/bscii.{hpp,cpp}` as an isolated conversion module with
   explicit opt-in API (no pipeline coupling).
4. Property tests: valid conversion, reverse conversion, round-trip
   integrity, reserved values, invalid code handling, duplicate mapping,
   loss detection, error reporting — each traced to the clause that requires
   it.
5. Re-run the full 7-layout regression plus Windows runtime validation.

## 5. Verdict

**Applicability boundary: established (no legacy path in the Unicode IME;
separate-module constraint recorded). Interchange integrity of the Unicode
pipeline: VERIFIED. Clause-level BSCII conformance: BLOCKED** — official
document not obtained; no requirement fabricated.
