# BSTI Certification Readiness Dossier — SubornoIME

**Mission**: COMPLETE BDS COMPLIANCE… — **PHASE 2 UPDATED** (2026-11-24)
**Purpose**: assemble everything an independent assessment (BSTI or another
authority) would need, while stating honestly what is ready and what is
blocked. This dossier is NOT a certificate and claims none.

## 1. Product description

| Field | Value |
|---|---|
| Product | SubornoIME — native Windows Bengali Input Method Editor |
| Type | Downloadable desktop software (TSF/COM text service + settings UI) |
| Target OS | Windows 10 x64, Windows 11 x64 |
| Technology | C++20, Win32, TSF, COM, MSVC v143, CMake, Windows SDK |
| License | MIT (source) |
| Version / commit | repository `main`, evidence commit `533b3e2` |
| Layouts | 7: Suborno, English, Borno, Avro, Khipro, Probhat, National |
| Character pipeline | UTF-8 internal; UTF-16 at TSF boundary; NFC at commit; ICU-based normalization |

## 2. Intended use

Bengali text input on Windows desktop applications for all users, with
multiple keyboard conventions (phonetic, fixed physical, compositional,
national-style) and strict Unicode-correct output.

## 3. Standard applicability statement

| Standard | Applicability | Evidence anchor |
|---|---|---|
| BDS 1520:2018 | Applicable — character/encoding level; **obtained-edition (2011 Second Revision) clause verification COMPLETE**; 2018 Third Revision binding pending | BDS_1520_OBTAINED_EDITION_CLAUSE_VERIFICATION.md; BDS_1520_OBTAINED_EDITION_ALLOCATION.csv |
| BDS 1738:2018 | Applicable — National layout conformance target; revision identity verified (Catalogue 2023); clause verification blocked (full text store-only) | BDS_KEYBOARD_MAPPING_REPORT.md; SRC-19/22 |
| BDS 1935:2018 | Applicability boundary established (scope verified from Catalogue 2023); no legacy path; separate-module constraint | BDS_BSCII_INTEROPERABILITY.md |
| BDS 1834:2011 | N/A — grounded in the obtained full text (scope clauses 1.1/2: 3×4 mobile keypad); confirmation pending | docs/standards/compliance/BDS_1834_2011_APPLICABILITY.md; SRC-21 |

## 4. BDS revision register

See `docs/standards/research/BDS_REVISION_STATUS.md` — identities verified,
full texts NOT obtained, statuses recorded as UNVERIFIED (not assumed).

## 5. Compliance summary (master matrix)

41 requirement rows: **31 PASS** (evidence-backed; BDS 1520 rows now carry
obtained-edition clause verification) + **8 BLOCKED** (store-only documents /
Windows runtime) + **1 NOT TESTED** (Windows file save/reopen, CV-17) +
**1 N/A** (justified). Zero FAIL. PHASE 1 was 28 PASS / 11 BLOCKED. Full
detail: `docs/standards/compliance/BDS_MASTER_COMPLIANCE_MATRIX.csv`.

## 6. Test plan & results

- Independent Unicode-derived corpus (10 unit tests) — all PASS.
- **PHASE 2: official BDS 1520 allocation corpus (94 rows from the obtained
  document) — 5 unit tests PASS + script verifier ALL CHECKS PASS.**
- 579-row multi-layout engine harness (physical scan-code driven for Probhat)
  — 0 FAIL (re-run at phase-2 state).
- Full-block repertoire sweep (128/128) + official NormalizationTest Bengali
  subset (7/7) — PASS (re-run).
- Sanitizer (ASan/UBSan/LSan) — clean (591+2 at phase-2 state).
- Windows runtime — BLOCKED (10 rows) with re-verification procedure
  documented.

## 7. Risk analysis

| Risk | Likelihood | Mitigation status |
|---|---|---|
| Clause-level divergence discovered after reading official texts | Medium | Gated: no conformance claim until table compared; matrices carry BLOCKED |
| Bijoy/IP claim conflict for national mapping | Unknown | IP review + inquiry questions; no mapping copied; no claim made |
| Windows-only regressions invisible on Linux | Medium | Windows checklist + scripts ready; BLOCKED rows honest |
| Certification route mismatch for software | High until answered | Written inquiry prepared; no fabricated route |

## 8. Build & runtime information

- Engine verification: GCC 14.2 / Clang 23, ICU 76.1, Debian 13 (sandbox).
- Windows build pipeline (MSVC v143, VS 17 2022, Windows SDK, CMake, MSBuild)
  documented; not executable in this environment.
- Known limitations: Windows runtime rows BLOCKED; full BDS texts not held.

## 9. Third-party IP statement & license inventory

See `docs/standards/research/BDS_IP_AND_LICENSING_REVIEW.md`: MIT-compatible
dependency inventory; no Bijoy material; no BSTI copyrighted text committed;
Probhat upstream MIT attribution retained.

## 10. Known limitations (honest register)

1. Clause-level BDS conformance for the CURRENT editions of BDS 1738/1935
   and the BDS 1520 Third Revision: BLOCKED (store-only copies; purchase
   needs user approval). BDS 1520 obtained-edition (2011) verification is
   COMPLETE.
2. Windows runtime: BLOCKED (needs Windows environment).
3. BCC other-standards list: RESOLVED in PHASE 2 (27 rows reviewed; advisory
   only).
4. BSTI software-eligibility: pending official response.

## 11. Readiness verdict

**CERTIFICATION DOSSIER: ASSEMBLED (evidence-backed at engine level).
INDEPENDENT ASSESSMENT: PENDING — official route confirmation required.
The maximum honest technical verdict remains
`TECHNICALLY CONFORMANT — INDEPENDENT BSTI ASSESSMENT PENDING`, qualified by
BLOCKED rows (document access + Windows runtime). No certificate, mark, or
number is claimed.**
