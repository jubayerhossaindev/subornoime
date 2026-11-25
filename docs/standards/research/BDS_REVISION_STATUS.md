# BDS Revision Status Register

**Mission**: COMPLETE BDS COMPLIANCE… — **PHASE 2 UPDATED** (2026-11-24)
**Companion**: `BDS_OFFICIAL_SOURCE_REGISTER.md` (SRC-xx identifiers)
**Rule**: a standard's current status is determined by the official record —
never by publication date alone. Where the official record could not be
read on the access date, status is `UNKNOWN/UNVERIFIED`, never "active by
assumption".

## Status table

| Field | BDS 1738 | BDS 1520 | BDS 1935 | BDS 1834 |
|---|---|---|---|---|
| Standard number | BDS 1738:2018 | BDS 1520:2018 | BDS 1935:2018 | BDS 1834:2011 |
| Title (official) | Computer Bangla keyboard (First Revision) — **verified from Catalogue 2023** | Specification for Bangla Coded Character Set for Information Interchange **(Third Revision)** — **verified from Catalogue 2023** | Bangladesh Standard Code for Information Interchange (BSCII) — **verified from Catalogue 2023, p.123** | Mobile phone bangla keypad — **verified from Catalogue 2023 + obtained full text** |
| Revision | First Revision, 2018 | **Third Revision, 2018** (catalogue); full-text chain: 1995 origin (SRC-17) → Second Revision (obtained edition, adopted 15-02-2011) → Third Revision 2018 | 2018 issue, no revision label in catalogue | 2011 issue (foreword of obtained full text: approved by BSTI) |
| Adoption date | 2018 (exact day not verifiable without full document) | Third Revision 2018 (exact day not verifiable); Second Revision adopted 15-02-2011 (obtained edition foreword, verbatim) | 2018 (exact adoption day not verifiable) | 2011 (obtained full text) |
| Current status | **VERIFIED AS LATEST LISTED** — Catalogue 2023 (till June 2023) + BCC page (site updated 2026-11-20, fetched 2026-11-24) both list the 2018 First Revision; no amendment/corrigendum found in any official source; post-June-2023 store changes unverifiable without account | **VERIFIED AS LATEST LISTED** — same official records; **edition discrepancy documented**: BCC row 25 labels "BDS 1520:2018" but its PDF link serves the 2011 Second Revision text; Catalogue 2023 says Third Revision | **VERIFIED AS LATEST LISTED** — same official records (BCC row 27, no PDF) | **VERIFIED AS LATEST LISTED** — same official records; full text obtained (BCC row 26 PDF) |
| Amendment | None found in any retrievable official source (Catalogue 2023 lists amendments for many other standards — none for 1738) | None found | None found | None found |
| Corrigendum | None found | None found | None found | None found |
| Superseded version | BDS 1738:2004 | BDS 1520 Second Revision (2011) — the edition actually obtained via BCC | Unknown | Unknown |
| Issuing authority | BSTI | BSTI | BSTI | BSTI |
| Official source | BSTI BDS Store (account-gated); BCC list (SRC-22); Catalogue 2023 (SRC-19) | Same | Same | Same + BCC full-text PDF (SRC-21) |
| Full copy | **Not obtained** (store-only; purchase needs user approval) | **Obtained — 2011 Second Revision edition** via BCC (SRC-20, SHA256 recorded); **2018 Third Revision NOT obtained** (store-only) | **Not obtained** (store-only) | **Obtained** via BCC (SRC-21, SHA256 recorded) |
| Copyright | BSTI copyright; redistribution prohibited; obtained PDFs kept outside the repository | Same | Same | Same |
| Applicability | Applicable (software keyboard conformance target for the `National` layout) | Applicable (character repertoire / encoding model) | Applicability audited — legacy interchange code; no mandatory path into a Unicode IME (see applicability analysis) | N/A for Windows desktop product — **now grounded in the obtained full text** (scope clause 1.1 + clause 2: 3×4 mobile keypad) |

## Phase-2 findings (new)

1. **Three-way revision-label discrepancy for BDS 1520**: BCC row label
   "BDS 1520:2018" ≠ obtained document "(Second Revision)"/"BDS 1520:2011" ≠
   Catalogue 2023 "Third Revision". Recorded, never reconciled by assumption.
2. **Catalogue scope continuity**: even the 2018 Third Revision's catalogue
   scope sentence still says "included in the Unicode version 6.0.0" — so the
   obtained 2011 edition's Table 2 allocation (exactly the Unicode-6.0.0-era
   assigned set) has no identified conflict with the current revision's own
   scope statement. This is recorded as an observation, NOT as proof that the
   2018 text is identical.
3. **bstibds.com** (which hosted the dead phase-1 record links) is now a
   license-checking app (SRC-23); the BSTI BDS-sale link to it is stale.

## Verification requirements this register imposes downstream

1. Every clause-level matrix row for these standards is `BLOCKED` until an
   authorized full copy is purchased via SRC-03 and read.
2. The `Act` (SRC-08) is fully obtained and its sections are cited verbatim
   where used (EV-A-01..05).
3. "Current status UNVERIFIED" is itself the honest finding: BSTI catalogue
   presence (2018) plus absence of any notice of withdrawal/replacement in
   any official source is corroboration, not proof, of continued validity.
4. If BSTI publishes a newer revision or amendment of any of these standards,
   it would supersede this register; the BDS Store account search is the
   designated re-verification step (see BSTI_APPLICATION_CHECKLIST.md).
