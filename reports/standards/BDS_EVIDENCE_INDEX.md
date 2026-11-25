# BDS Evidence Index

**Mission**: COMPLETE BDS COMPLIANCE… — **PHASE 2 UPDATED** (2026-11-24)
**Traceability rule**: every PASS in the compliance matrices and final report
points to at least one Evidence ID below. Every result is independently
reproducible with the recorded commands. No synthetic/mock data is presented
as real evidence; screenshot-type evidence that does not exist (Windows UI)
is explicitly listed as BLOCKED, never simulated.

## 1. Evidence registry

| Evidence ID | Type | Description | Location | Reproduce |
|---|---|---|---|---|
| EV-S-01..18 | Official source records | 18-entry source register with URLs, access dates, retrieval status | docs/standards/research/BDS_OFFICIAL_SOURCE_REGISTER.md | fetch URLs on the recorded date |
| EV-A-01 | Legal text (verbatim) | BSTI Act 2018 §6 — Functions | BDS_CERTIFICATION_ROUTE.md §1 | bdlaws PDF (SRC-08) |
| EV-A-02 | Legal text (verbatim) | §15 — Standard Mark prohibition | same | same |
| EV-A-03 | Legal text (verbatim) | §16 — Licence (3-year validity, inspection/testing condition) | same | same |
| EV-A-04 | Legal text (verbatim) | §18 — prohibited expressions | same | same |
| EV-A-05 | Legal text (verbatim) | §21 — prohibition of non-conforming articles | same | same |
| EV-T-01 | Test run | Full unit suite **586/586 PASS** (incl. 10 new BDS tests) | ctest/console | `./build/ubuntu-dev/suborno_unit_tests` |
| EV-T-02 | Test run | Integration **2/2 PASS** | ctest | `ctest` in build/ubuntu-dev |
| EV-T-03 | Test run | ASan/UBSan/LSan suite **588 PASS, zero diagnostics** | console | `cmake --build build/bds-asan --target suborno_unit_tests suborno_integration_tests && ASAN_OPTIONS=detect_leaks=1 ./build/bds-asan/suborno_unit_tests` |
| EV-T-04 | Test run | BDS conformance tests **10/10** (digits, fix preservation, NFC, sweep, reserved) | tests/unit/bds_conformance_test.cpp | `--gtest_filter='BdsConformanceTest.*'` |
| EV-R-01 | Engine evidence | 579-row multi-layout typing harness, **0 FAIL**, scan-code driven | reports/standards/evidence-raw/bds_engine_rows_579.csv | `./build/ubuntu-dev/khipro_probhat_engine_verify . <out> <uni>` |
| EV-R-02 | Engine evidence | Commit Unicode rows (code-point comparisons) | reports/standards/evidence-raw/bds_unicode_rows.csv | same |
| EV-U-01 | Data + run | Bengali block repertoire sweep **128/128 PASS** | evidence-raw/bds_repertoire_inventory.csv | `./build/ubuntu-dev/bds_repertoire_verify …` |
| EV-U-02 | Data + run | Independent unicodedata crosscheck **0 mismatches** (96 assigned, 32 reserved) | evidence-raw/bds_repertoire_crosscheck.csv | `python3 scripts/layout_verify/bds_repertoire_crosscheck.py …` |
| EV-U-03 | Data + run | Official NormalizationTest.txt Bengali subset through engine NFC **7/7 PASS** (UAX15-C3) | evidence-raw/bds_normalizationtest_engine.csv + NORMALIZATION_TEST_BN_EXTRACT.txt | part B of the harness |
| EV-U-04 | Data + run | BDS key-sequence conformance **12/12 PASS** (composition exclusions, singletons, virama stability) | evidence-raw/bds_key_sequences.csv | part C of the harness |
| EV-U-05 | Official data | Unicode Bengali chart (current + 18.0.0, identical) + UAX #15 clauses | evidence-raw/UNICODE_CHART_U0980_TEXT.txt; SRC-11..14 | unicode.org |
| EV-P-01 | Performance | Harness 579 rows in **24.9 ms**, max RSS ≈ 12.6 MiB | BDS_REGRESSION_REPORT.md §4 | time the harness |
| EV-P-02 | Performance | Unit suite 586 tests in 0.64 s; ASan suite 1.6 s | same | time the suites |
| EV-G-xx | Git | Source commit(s): baseline `cce3413`, mission commit `533b3e2`, phase-2 commit (this report's commit) | git log | `git log --oneline` |
| EV-S-19..24 | Official source records (PHASE 2) | 6 new register entries: Catalogue 2023, BDS 1520 obtained-edition full text, BDS 1834 full text, BCC page re-verification, bstibds.com status, e-store API/TLS re-check | docs/standards/research/BDS_OFFICIAL_SOURCE_REGISTER.md | fetch URLs on 2026-11-24 |
| EV-C-01 | Official-document corpus | BDS 1520 obtained-edition allocation corpus — 94 rows machine-verified (offset rule, danda exceptions, unicodedata names, uniqueness) | docs/standards/compliance/BDS_1520_OBTAINED_EDITION_ALLOCATION.csv | `python3 scripts/layout_verify/bds1520_official_corpus.py` |
| EV-C-02 | Corpus verifier run | "ALL CORPUS CHECKS PASS" — 94 rows, 92 block + 2 danda, modern-set reconciliation (4 post-6.0.0 additions absent), harness subset check PASS (66/66 Bangla-script points inside official repertoire) | script stdout | same command |
| EV-C-03 | Test run (PHASE 2) | Bds1520OfficialCorpusTest 5/5 PASS (round-trip incl. composition-exclusion rows DC/DD/DF, offset rule, UTF-16 BMP, block classification, row count) — full suite **591/591** | tests/unit/bds1520_official_corpus_test.cpp + bds1520_corpus_data.inc | `./build/ubuntu-dev/suborno_unit_tests --gtest_filter='Bds1520OfficialCorpusTest.*'` |
| EV-T-05 | Test run (PHASE 2) | ASan/UBSan suite **591 + 2 PASS, zero diagnostics** (rebuilt at phase-2 state) | console | `cmake --build build/bds-asan && ./build/bds-asan/suborno_unit_tests` |
| EV-W-01 | BLOCKED register | Windows runtime rows W-01..W-10 **BLOCKED — WINDOWS RUNTIME UNAVAILABLE** | BDS_WINDOWS_RUNTIME_VERIFICATION.md §2 | requires Windows 10 x64 |
| EV-T-06 | Test run (PHASE 3) | Full unit suite **609/609 PASS** (National positional rewrite: +18 net tests; 20 National tests) | ctest/console | `./build/ubuntu-ci/suborno_unit_tests` |
| EV-T-07 | Test run (PHASE 3) | Integration **2/2 PASS**; ASan/UBSan (GCC) **609 + 2 PASS, zero diagnostics** | console | sanitizer build with `-DSUBORNO_ENABLE_SANITIZERS=ON` |
| EV-R-05 | Engine evidence (PHASE 3) | National positional 4-layer engine rows **121/121 PASS** (independently transcribed expected table vs real engine: base/shift/Link/Link+Shift + sequences) | reports/standards/evidence-raw/bds_national_positional_engine_rows.csv | build+run `scripts/layout_verify/national_positional_verify.cpp` (build command in file header) |
| EV-D-05 | Reference document (PHASE 3) | Official National Keyboard of Bangladesh (BDS 1738:2018) diagram — Unicode Consortium public document L2/19-079, Figure 4 (page 4), fetched 2026-11-24 | unicode.org/L2/L2019/19079-bangla-letters.pdf | fetch URL on access date |
| EV-D-06 | Reference source (PHASE 3) | BornoTSF National.cpp/.h factual mapping (GPL-3.0) — facts only, no code copied; README status “Tested (WIP)” recorded | github.com/JayedAhsan/BornoTSF | fetch on access date |
| EV-R-06 | Report (PHASE 3) | Three-way key-by-key discrepancy report (official diagram vs BornoTSF vs SubornoIME before/after) | reports/standards/BDS_NATIONAL_LAYOUT_DISCREPANCY_REPORT.md | n/a (report) |
| EV-D-01..04 | Officially DEAD/blocked access | bstibds.com record links 404 (SRC-04/05); store API token-gated (SRC-18); BCC/BSTI pages body-empty (SRC-06/09/10) | source register | fetch on access date |

## 2. Evidence → matrix traceability

- BDS 1738 matrix rows KV-01..KV-15 → EV-S-*, EV-T-*, EV-R-01, EV-W-01, EV-A-*, EV-G-*; KV-02 → EV-S-19/22; KV-03/KV-04/KV-07/KV-08 → EV-R-05/EV-D-05/EV-D-06/EV-R-06/EV-T-06 (PHASE 3).
- BDS 1520 matrix rows CV-01..CV-17 → EV-U-01..05, EV-R-01/02, EV-T-01..05, EV-S-*; CV-14/CV-15/CV-16 → EV-C-01..03, EV-S-20.
- BDS 1935 matrix rows BV-01..BV-08 → EV-U-01, EV-R-01, EV-T-03, EV-S-16/19/22.
- BDS 1834 row MV-01 → SRC-21 (obtained full text) + BDS_1834_2011_APPLICABILITY.md.
- Certification claims: **none** (no certificate exists) — see
  BDS_FINAL_CONFORMANCE_REPORT.md verdicts.

## 3. Reproduction notes

- All harnesses and scripts are committed in `scripts/layout_verify/`.
- CSV evidence is engine-generated output, committed byte-for-byte.
- The unit/integration suites are CI-contract targets (`suborno.unit`,
  `suborno.integration`).
- Every BLOCKED row names the exact missing resource and the exact official
  path to obtain it.
