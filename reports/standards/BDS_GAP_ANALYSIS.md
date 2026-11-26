# BDS Gap Analysis & Solution Design

**Mission**: COMPLETE BDS COMPLIANCE… — **PHASE 2 UPDATED** (2026-11-24)
Every gap below carries: Finding ID, standard, requirement, current state,
root cause, severity, impact, solution, affected files, regression risk, test
plan, evidence. No gap was hidden; no fabricated requirement was created to
close one. **PHASE 2**: GAP-01 partially closed (2 official texts obtained,
1 of them an older edition), GAP-03 closed, new finding GAP-07 recorded.

## GAP-01 — Authorized BDS full texts not obtained (clause-level conformance unverifiable)

**PHASE 2 STATUS: PARTIALLY CLOSED.**

| Field | Value |
|---|---|
| Standard(s) | BDS 1738:2018, BDS 1520:2018, BDS 1935:2018 (BDS 1834:2011 obtained — N/A) |
| Requirement | Clause-by-clause conformance verification (mission PART 8) |
| Current implementation | **OBTAINED and READ**: BDS 1520 full text via BCC official file store (SRC-20) — but the obtained copy is the **2011 Second Revision** edition (cover/header verified); BDS 1834:2011 full text via the same route (SRC-21, current edition). **Still missing**: BDS 1738:2018 and BDS 1935:2018 full texts, and the BDS 1520:2018 **Third Revision** text — all store-only |
| Root cause | BSTI BDS Store requires account sign-in + purchase (official route recorded, SRC-03/24); BCC publishes full-text PDFs for only 2 of the 4 standards, and the BDS 1520 one is the older edition |
| Severity | **High** (for the still-missing texts); **Medium** for BDS 1520 (obtained-edition clause verification complete; current-edition binding pending) |
| Impact | Master matrix now 31 PASS / 8 BLOCKED / 1 NOT TESTED / 1 N/A (was 28/11/1/1); BDS 1520 obtained-edition clause table complete (16 clauses + tables); 2018 Third Revision clause binding stays BLOCKED |
| Proposed solution | User approves purchase via https://ec.eservice.bsti.gov.bd (How-To-Buy route: sign up → Buy BDS → Sonali Payment Gateway → one-time download; catalogue price groups suggest ≈BDT 500–700 per standard); then transcribe official tables clean-room and upgrade the matrices |
| Affected files | docs/standards/compliance/*.csv (status upgrades), BDS_1520_OBTAINED_EDITION_CLAUSE_VERIFICATION.md (new) |
| Regression risk | None until mapping changes are derived from the official table |
| Test plan | Key-per-cell national-layout corpus; repertoire crosscheck with BDS columns; 2018-edition re-comparison of the 1520 allocation corpus |
| Evidence | SRC-19/20/21/22/24; BDS_1520_OBTAINED_EDITION_CLAUSE_VERIFICATION.md |

## GAP-02 — Windows runtime verification unavailable

| Field | Value |
|---|---|
| Standard | Cross-cutting (BDS 1738 physical behaviour; TSF runtime) |
| Requirement | Real Windows desktop tests W-01..W-10 |
| Current | Engine-level verification complete; Windows rows BLOCKED |
| Root cause | Linux sandbox; no Windows 10 x64 + MSVC v143 + TSF runtime |
| Severity | **High** for release readiness; not a code defect |
| Impact | No "Windows Verified" claim anywhere; PRODUCTION readiness keeps its BLOCKED component |
| Solution | Run the documented procedure on a Windows 10 x64 machine (MSVC preset, ctest windows targets, UI-audit scripts) and fill W-01..W-10 with real results |
| Affected files | reports/standards/BDS_WINDOWS_RUNTIME_VERIFICATION.md |
| Regression risk | n/a |
| Test plan | W-01..W-10 checklist |
| Evidence | BDS_WINDOWS_RUNTIME_VERIFICATION.md |

## GAP-03 — BCC IT-standards list body not retrievable (other-standards applicability unresolved)

**PHASE 2 STATUS: CLOSED.**

| Field | Value |
|---|---|
| Standard | "Other applicable IT standards" (mission PART 3.5) |
| Requirement | Decide applicability of every BCC-listed IT standard |
| Current | **RESOLVED 2026-11-24**: the BCC page body now renders (SRC-22) — 27 rows retrievable: 23 ISO/IEC adoptions + the 4 BDS items. Applicability review performed: the 23 ISO/IEC adoptions (biometric ID cards, vocabulary parts, security 27001/27002, software evaluation 14598 series, maintenance 14764, CASE tools, categorization 12182, etc.) address domains outside a Bengali IME's normative scope (keyboard layout, character encoding, input methodology) — none is applicable to SubornoIME's conformance targets; the BCC advisory is explicitly advisory ("strongly advised"), not a mandate. Internal inconsistency in the BCC page recorded (advisory says 23+3 standards, table lists 23+4) |
| Root cause | (was) CMS content delivery issue on the official pages — no longer present |
| Severity | Closed |
| Impact | The "other IT standards" applicability question is answered with the official list; no mandatory claim made |
| Solution | Done — re-check the page on future missions for list changes |
| Affected files | BDS_APPLICABILITY_ANALYSIS.md §1 row 5; source register SRC-22 |
| Regression risk | none |
| Test plan | n/a |
| Evidence | SRC-22 (verbatim advisory text + full row list) |

## GAP-04 — National layout clause-level mapping equivalence unproven

| Field | Value |
|---|---|
| Standard | BDS 1738:2018 |
| Requirement | The layout claiming "national" identity must match the official mapping exactly |
| Current | National layout REWRITTEN as the positional Jatiyo layout (2026-11-24, PHASE 3): verified key-by-key against the official National Keyboard diagram republished in Unicode L2/19-079 Figure 4 + legacy reference IME factual cross-reference (121/121 independent engine rows PASS, legacy reference IME WIP status respected, Assamese ৰ/ৱ excluded, GPL facts-only clean-room). Clause-level equivalence to the purchased standard STILL BLOCKED (GAP-01) — diagram evidence is not the clause table |
| Root cause | GAP-01 (document unavailable) + Bijoy-linked IP question |
| Severity | High (for any conformance claim); none for current product honesty (no BDS claim is made anywhere) |
| Impact | UI/docs correctly avoid "BDS 1738 conformant" labels; no fake badge exists |
| Solution | Diagram-level step DONE (2026-11-24): positional rewrite + 3-way comparison in BDS_NATIONAL_LAYOUT_DISCREPANCY_REPORT.md. Remaining after GAP-01: transcribe the purchased clause table → key-per-cell corpus → re-run comparison, with Bijoy IP clearance first |
| Affected files | src/layouts/national/* (future), tests |
| Regression risk | Constrained to one layout; full 7-layout suite guards the rest |
| Test plan | New corpus tests + full regression |
| Evidence | BDS_NATIONAL_LAYOUT_DISCREPANCY_REPORT.md; national-keymap.md; bds_national_positional_engine_rows.csv (121 PASS); national_layout_test.cpp 20 tests; SRC-22 (row 24 has no link) |

## GAP-05 — BSTI eligibility for downloadable software unconfirmed

| Field | Value |
|---|---|
| Standard | Legal (BSTI Act 2018 + CM scheme) |
| Requirement | Determine real certification route (mission PART 6) |
| Current | Act sections captured verbatim; CM process pages render empty; no software scheme found; inquiry drafts (BN+EN) prepared, unsent |
| Root cause | Official procedure texts not retrievable; BSTI interpretation required |
| Severity | Medium |
| Impact | Verdict fixed at "INDEPENDENT BSTI ASSESSMENT PENDING"; no certificate claim |
| Solution | Send the prepared inquiries (requires user approval), then follow the reply |
| Affected files | reports/standards/BSTI_OFFICIAL_INQUIRY_BN.md / _EN.md, BSTI_APPLICATION_CHECKLIST.md |
| Regression risk | none |
| Test plan | n/a |
| Evidence | EV-A-01..05; SRC-01/09/10 |

## GAP-06 — Minor layout-scope notes carried forward (unchanged)

| Field | Value |
|---|---|
| Items | Classic & Smart lack a standalone-nukta token; Probhat dead-key composition stays literal |
| Status | Documented previously; outside this mission's protected scope; unchanged |
| Severity | Low |
| Evidence | docs/standards/BD_BENGALI_STANDARD_COMPLIANCE.md §4 (carried forward) |

## GAP-07 — BDS 1520 obtained-edition findings (PHASE 2, new)

| Field | Value |
|---|---|
| Standard | BDS 1520 (obtained 2011 Second Revision edition) |
| Finding | (a) Three-way revision-label discrepancy: BCC row label "BDS 1520:2018" vs document "(Second Revision)"/"BDS 1520:2011" vs Catalogue 2023 "Third Revision"; (b) internal count inconsistency: clause 5 states 90 graphic characters, Table 2 prints 94 allocation rows; (c) clause 2.2.2 direct-origination gap: no key path exists for a documented subset of the official repertoire (e.g. currency numerators U+09F4..U+09F9, ISSHAR, Ganda Mark) — device conformance under clause 2.2 still satisfied via the 2.2.3 receiving route |
| Severity | (a)/(b) Low-Medium (documentation-level; affects interpretation); (c) Medium (product-scope decision, not a defect) |
| Impact | All recorded in the matrices and clause-verification doc; nothing hidden |
| Solution | (a)/(b) re-check against the 2018 Third Revision after purchase; (c) deliberate product decision — key-level coverage extension would require a mapping-design mandate (out of this phase's authorized scope; National layout untouched) |
| Evidence | SRC-19/20/22; BDS_1520_OBTAINED_EDITION_CLAUSE_VERIFICATION.md §5 |

## Non-gaps verified clean this mission (PHASE 2 re-verified)

- Unicode scalar/sequence/UTF-8/UTF-16/NFC correctness: full evidence, 0 mismatches.
- Khipro/Probhat fixes: preserved, re-verified (`BdsConformanceTest` + 579-row harness re-run).
- **NEW — official BDS 1520 repertoire vs engine**: 94-row official allocation corpus verified against the engine (94/94 round-trip, NFC, UTF-16, offset rule) + harness subset property (66 emitted Bangla-script points all inside the official repertoire).
- Reserved-position safety, invalid-UTF8 handling, sanitizer cleanliness: re-verified (591/591 unit + 2/2 integration, ASan/UBSan clean).
- Performance: harness 579 rows in ~25 ms — no degradation.

**Release-blocking rule applied**: any change that damages another layout is a
release blocker — none occurred (full suite + harness green).
