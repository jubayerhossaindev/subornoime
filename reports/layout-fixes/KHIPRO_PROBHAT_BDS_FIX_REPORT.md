# KHIPRO TRAILING VIRAMA & PROBHAT STANDALONE NUKTA — BDS COMPLIANCE FIX REPORT

**Mission**: KHIPRO TRAILING VIRAMA & PROBHAT STANDALONE NUKTA FIX — Bangladesh BDS
Standards Compliance, Unicode Correctness & Full Regression Verification
**Date**: 2026-11-24 (Asia/Dhaka)
**Baseline**: HEAD `86a543e`, branch `main`, working tree clean (baseline audit
re-verified fresh; the previous layout report's `a241477` was superseded by 5
commits of UI-audit packaging work — engine layout sources verified git-identical
between the two)
**Environment**: Linux sandbox (Debian 13; GCC 14.2.0 for the coverage build;
Clang 23 for the ubuntu-dev build; ICU 76.1). Windows 10 x64 + MSVC v143 not
available → Windows Runtime rows are **BLOCKED**, never upgraded.

---

## 1. Final Verdicts (Section 14 answers)

| # | Question | Answer |
|---|---|---|
| 1 | Khipro trailing virama ঠিক হয়েছে কি না | **হ্যাঁ — ENGINE VERIFIED (fix applied)**। `kqq` দিয়ে ক্ টাইপ হয়, committed output হুবহু U+0995 U+09CD; 26 mission case + 74/74 inventory PASS। Windows Runtime পরীক্ষা হয়নি — Windows Verified দাবি করা হয় না। |
| 2 | Probhat standalone nukta ঠিক হয়েছে কি না | **হ্যাঁ — ENGINE VERIFIED (fix applied)**। AltGr (Ctrl+Alt) + scan 0x34 দিয়ে ় আসে, output হুবহু U+09BC; physical scan-code path-এ 13 mission case + 62 probe PASS; document-কৃত Compatibility Extension। Windows Runtime BLOCKED। |
| 3 | প্রযোজ্য BDS ও Unicode Requirements অনুসরণ হয়েছে কি না | **Unicode: সম্পূর্ণ যাচাইকৃত** (scalar, sequence, UTF-8, NFC, no-drop, no-dup, reorder-শুধুমাত্র-standard)। **BDS: repertoire-level যাচাইকৃত + non-interference নীতি মেনে**; clause-level যাচাই **BLOCKED** (BSTI মূল document copyright-সুরক্ষিত, পূর্ণ টেক্সট পাওয়া যায়নি — কোনো clause নম্বর ভান করা হয়নি)। বিস্তারিত: docs/standards/BD_BENGALI_STANDARD_COMPLIANCE.md |
| 4 | অন্য Layout-এর কোনো আচরণ নষ্ট হয়েছে কি না | **না।** সাত layout-এর সম্পূর্ণ suite পুনঃচালিত: unit 576/576 (557 পুরোনো সব PASS + 19 নতুন), integration 2/2, 402-row inventory re-run + 62 scan probe + brief + english passthrough = 579 rows, 0 FAIL। অন্য পাঁচ layout-এর কোনো source পরিবর্তনই হয়নি। |

Verdict matrix:

| Layout | Engine Verdict | Windows Runtime |
|---|---|---|
| Khipro | **ENGINE VERIFIED (fix applied)** | BLOCKED |
| Probhat | **ENGINE VERIFIED (fix applied)** | BLOCKED |
| অন্য পাঁচ (suborno/english/borno/avro/national) | ENGINE VERIFIED (অপরিবর্তিত — পুনঃযাচাইকৃত) | BLOCKED |

---

## 2. Root Cause Analysis (Section 3)

### Khipro — trailing virama (ক্)

`transliterate_khipro()` (src/layouts/khipro/khipro_layout.cpp) auto-virama
model: virama শুধুমাত্র **দুই ব্যঞ্জনের মাঝে** insert হয় (second consonant
process হওয়ার মুহূর্তে)। Input buffer-এর শেষে ব্যঞ্জন থাকলে কোনো virama আসে
না — এবং `;` (partition, silent) ও `/` (slicer/undo/silent-ender) কোনোটিই
virama emit করে না। ফলে `ক্` (trailing/displaced) NOT_DERIVABLE — INV-064,
inventory 66/67।

Token audit (collision অনুসন্ধান):
- `;` — documented partition key (`k;t`→কত, `;;`→literal `;`)। Terminal
  extension প্রত্যাখ্যাত: `k;`-এর বর্তমান silent-ender behavior documented
  contract এবং break-key অর্থ context-dependent হতো।
- `/` — documented slicer (`ks/`→কস) + silent-ender (`k/`→ক, code comment
  "stays a silent ender, exactly as before") + chandrabindu (`ka/`→কাঁ)।
  Extension প্রত্যাখ্যাত: ব্যাপক blast radius (সব consonant+`/` sequence)।
- **`qq` — গৃহীত**: real Khipro Classic-এর নিজস্ব documented explicit hasanta
  convention (khipro.khiproteam.com quickstart: "use `qq` to create hasanta")।
  এই repo-র Khipro-তে `q` = ঋ, কিন্তু `qq` দুই-অক্ষর টোকেন হিসেবে
  collision-free — corpus/inventory/brief/test-এর কোনো ইনপুটেই `qq` নেই।

### Probhat — standalone nukta (়)

`kProbhatMappings` (src/layouts/probhat/probhat_layout.cpp) upstream MIT
Probhat KLC-র বিশ্বস্ত অনুলিপি (princebillygk/probhat-layout-windows @
debc8310c60200143c6c0420433cca8506771e09)। 2026-11-24 তারিখে upstream KLC
byte-level পুনঃডাউনলোড + পুনঃযাচাই: **U+09BC কোথাও নেই** (`rg -c 09bc` → 0),
এবং 0x34-এর AltGr column `-1`। অর্থাৎ সীমাবদ্ধতাটি upstream layout-এরই — ়
শুধু য় (0x2C), ড় (Shift+0x13), ঢ় (Shift+0x2D)-এর ভেতরে আসে।

Key audit: মূল block-এর প্রতিটি typeable key (0x02–0x35, 0x39, 0x53, 0x56)
mapped — unassigned physical key নেই। AltGr layer implementation-supported
(KLC column 6)। **AltGr+0x34** নির্বাচিত: upstream-empty slot (কোনো existing
key overwrite নয়), diacritic corner (base ।, Shift ঁ, প্রতিবেশী 0x35-এ ্),
দুই-modifier combo। বিকল্প প্রত্যাখ্যাত: 0x25 AltGr (গুরুত্বপূর্ণ খালি-slot
test assertion `AltGrLayerFollowsTheUpstreamKlcColumns` ভাঙত), Shift+AltGr+0x35
(তিন-modifier combo — খারাপ UX)।

---

## 3. Fixes Implemented (Sections 4–5)

### Fix A — Khipro `qq` explicit hasanta

- নতুন `qq` branch (before `find_mapping`, দুই-অক্ষর longest-match):
  - ব্যঞ্জনের পরে → তাৎক্ষণিক `্` append + `last_joiner_offset` set (slicer
    undo-able) + `explicit_virama=true` → পরের ব্যঞ্জন দ্বিতীয় virama ছাড়া
    join সম্পন্ন করে।
  - Standalone/vowel-পরে → silent state break (National bare-halant rule অনুরূপ)।
  - Idempotent: output tail ইতিমধ্যে ্ হলে দ্বিতীয়টা append হয় না।
- `explicit_virama` state: প্রতিটি অন্য token branch reset করে; consonant
  branch explicit flag থাকলে auto-insert বাদ দেয়।
- Output হুবহু U+09CD — real Khipro Classic-এর trailing `xx`+ZWNJ variant থেকে
  document-কৃত deviation (mission requirement: ZWNJ নিষিদ্ধ)।
- Slash-position নোট: branch loop-এর ভেতরে, `;;`-এর আগে।

### Fix B — Probhat AltGr+0x34 → U+09BC

- `{0x34, "।", "ঁ", "\u09BC", "", 0x00}` — শুধু altgr column পরিবর্তন;
  unshifted/shifted/shift_altgr/caps সব অপরিবর্তিত।
- Source comment + docs/specifications/probhat-keymap.md-তে
  "SubornoIME Probhat Compatibility Extension" হিসেবে document-কৃত।
- Session/NFC pipeline অপরিবর্তিত (commit NFC parity আগের contract)।

### Protected-area compliance

পরিবর্তিত source: শুধু `src/layouts/khipro/khipro_layout.cpp` ও
`src/layouts/probhat/probhat_layout.cpp` (mission-অনুমোদিত দুই file) + নতুন
test/doc/report file। অন্য কোনো Protected Component স্পর্ষ হয়নি।

---

## 4. Test Results (Sections 7–9)

### In-repo suite

| Suite | Before | After |
|---|---|---|
| Unit (gtest) | 557/557 PASS | **576/576 PASS** (557 পুরোনো অপরিবর্তিত + 19 নতুন) |
| Integration | 2/2 PASS | **2/2 PASS** |
| ASan+UBSan | বিদ্যমান config পুনঃচালিত | **576/576 PASS, zero finding** (leak/UB/memory নেই) |
| -Wall -Wextra -Wpedantic -Wshadow (দুই changed file) | — | **0 warning** |
| scripts/brace_scan.py, check-layering.py | PASS | PASS |
| clang-tidy | — | sandbox-এ binary অনুপস্থিত → **NOT RUN (BLOCKED)**, লুকানো হয়নি |

নতুন unit tests (naming brief-এর উদাহরণ অনুসরণ):

- Khipro (10): `KhiproTrailingViramaCanBeTyped`,
  `KhiproTrailingViramaProducesCorrectUnicode`, `KhiproTrailingViramaCommitsCorrectly`,
  `KhiproTrailingViramaBackspaceWorks`, `KhiproTrailingViramaDoesNotBreakAutoJoin`,
  `KhiproExplicitJoinRemainsUnchanged`, `KhiproExistingCorpusRemainsUnchanged`,
  `KhiproTrailingViramaEdgeCases`,
  `LayoutTypingRegressionTest.KhiproTrailingViramaIsTypeableWithCorrectCodePoints`,
  `LayoutTypingRegressionTest.KhiproTrailingViramaSessionCommitParity`
- Probhat (9): `ProbhatStandaloneNuktaCanBeTyped`, `ProbhatNuktaProducesU09BC`,
  `ProbhatNuktaAfterConsonantWorks`, `ProbhatNuktaUsesExpectedScanCode`,
  `ProbhatNuktaDoesNotBreakExistingMappings`, `ProbhatNuktaCommitMatchesEncoding`,
  `ProbhatNuktaBackspaceWorks`, `ProbhatNuktaCompositionAndEditingEdges`,
  `LayoutTypingRegressionTest.ProbhatStandaloneNuktaIsTypeableViaScanCode`

Probhat test-গুলো **সবই scan-code event path** (append_key + modifiers) —
শুধু transliterate() দিয়ে PASS করানো হয়নি (mission rule)।

### Fresh engine harness (scripts/layout_verify/khipro_probhat_engine_verify.cpp)

579 rows: **570 PASS + 6 PASS-EQUIV + 3 CONVENTION-DIFF + 0 FAIL**:

| Group | Rows | Result |
|---|---|---|
| inventory (67×6 committed CSV re-run) | 402 | INV-064 khipro ক্ এখন PASS (kqq); suborno I/U derivation-column নোট; বাকি সব PASS/PASS-EQUIV |
| brief (compositional sequences re-run) | 70 | PASS/PASS-EQUIV; 3 documented CONVENTION-DIFF (borno Ta;i/mo;i, avro t'') — LIM-BRIEF-CONVENTION-MISMATCH, engine defect নয় |
| brief-scan (Probhat physical probes) | 62 | PASS সব; BRIEF-060 equivalent (় via scan=0x34 altgr=1) এখন PASS |
| khipro-trailing-virama | 26 | PASS সব (basic/terminal/parser/editing/edge) |
| probhat-standalone-nukta | 13 | PASS সব (scan events, key-up, ctrl-alone, repeat, backspace, NFC, space boundary) |
| english-passthrough | 4 | PASS (identity অপরিবর্তিত) |
| sentence-boundary | 2 | PASS (word-commit boundary virama ধরে রাখে) |

PASS-EQUIV নিয়ম: ড়/ঢ়/য় composition exclusions — corpus CSV-র precomposed
form (U+09DC/U+09DD/U+09DF) বনাম engine-এর NFC form (decomposed); 2026-11-23
রানের একই documented equivalence, কখনো blind normalization নয়।

### Coverage (একই method, আগে ও পরে)

| Metric | Before (86a543e, 557 tests) | After (fix, 576 tests) |
|---|---|---|
| Line | 4053/5006 = **80.96%** | 4091/5040 = **81.17%** (+0.21pp) |
| Branch | 6734/8525 = **78.99%** | 6756/8545 = **79.06%** (+0.07pp) |
| khipro_layout.cpp | — | 99.59% lines (241), branches taken 72.48% |
| probhat_layout.cpp | — | 96.48% lines (142), branches taken 71.15% |

নোট: পুরোনো রিপোর্টের 96.17%/58.49% ভিন্ন টুল/ফিল্টারের সংখ্যা; এই mission-এ
আগে-পরে দুই মাপই gcc gcov একই পদ্ধতিতে তাই comparison internally consistent।

---

## 5. Unicode Verification (Section 6)

`reports/layout-fixes/UNICODE_VERIFICATION.csv` (12 checks, সব PASS):

- ্ = U+09CD, UTF-8 E0 A7 8D; ় = U+09BC, UTF-8 E0 A6 BC — exact bytes।
- ক্ = U+0995 U+09CD (NFC-stable, commit/copy/paste/serialization-এ অপরিবর্তিত)।
- ক় = U+0995 U+09BC; ড় = U+09A1 U+09BC; ঢ় = U+09A2 U+09BC; য় = U+09AF
  U+09BC — decomposed, precomposed look-alike (U+09DC/U+09DD/U+09DF) থেকে
  byte-distinct।
- Character dropping নেই (579 rows strict compare), duplicate mark নেই
  (idempotent qq; repeated-nukta শুধু physical repeat-এ), accidental reorder
  নেই — একমাত্র reorder হলো standard-নির্ধারিত canonical reorder (nukta ccc=7
  < virama ccc=9): ক+্+় → ক+়+্।
- Invalid encoding নেই (সব committed স্ট্রিং valid UTF-8, ICU/normalize
  roundtrip ক্লিন)।

---

## 6. Documentation Updates (Section 10)

| Document | Change |
|---|---|
| docs/specifications/khipro-keymap.md | **নতুন** — সম্পূর্ণ Khipro keymap spec, নতুন `qq` token সহ |
| docs/specifications/probhat-keymap.md | Compatibility Extension section + test coverage আপডেট |
| docs/standards/BD_BENGALI_STANDARD_COMPLIANCE.md | **নতুন** — BDS applicability matrix + Unicode verification + honest gaps |
| reports/layout-verification/layouts/khipro/{REPORT.md,findings.md,inventory-results.csv} | INV-064 → PASS + 7 নতুন extension row (INV-068..074); findings RESOLVED |
| reports/layout-verification/layouts/probhat/{REPORT.md,findings.md,inventory-results.csv,brief-results.csv} | BRIEF-060 → PASS; INV-068/069 নতুন nukta row; findings RESOLVED |
| reports/layout-verification/MASTER_LAYOUT_STATUS_MATRIX.csv + REPORT.md | khipro/probhat verdict → ENGINE VERIFIED (fix applied); limitations → none |
| reports/layout-verification/summary/{limitations.json,verification-status.json} | LIM-KHIPRO-TRAILING-VIRAMA, LIM-PROBHAT-STANDALONE-NUKTA → resolved:true |

BDS 1738-এর national mapping কোনো layout-এ মেশানো হয়নি — Khipro `qq` হলো
Khipro Classic convention, Probhat AltGr+0x34 হলো Probhat extension; দুটোই
আলাদা document করা।

---

## 7. Acceptance Criteria Checklists (Sections 4.6 / 5.6 / 13)

### Khipro

- [x] Explicit Terminal Virama টাইপ করা যায় (`kqq`)
- [x] ক্ সঠিক Unicode sequence (U+0995 U+09CD)
- [x] Existing Automatic Virama অপরিবর্তিত (`kt`, `ksf`, `Ngk` re-verified)
- [x] Existing Explicit Join/partition অপরিবর্তিত (`k;t`, `;;`, `ks/`, `bakr/uddh`)
- [x] Existing Corpus PASS (49/49 re-run + headlines unit-pinned)
- [x] Existing Character Inventory PASS (74/74)
- [x] New Trailing Virama Tests PASS (26 harness + 10 unit)
- [x] Backspace PASS (raw-buffer semantics, কৃ boundary সহ)
- [x] Commit PASS (NFC parity)
- [x] Existing Khipro Regression Tests PASS (F-5 suite)
- [x] অন্য পাঁচ layout অপরিবর্তিত (579-row re-run)
- Inventory সংশোধন: 66/67 → **74/74 PASS** (প্রত্যাশিত 67/67-এর পরিবর্তে
  সত্য সংখ্যা — নতুন extension row সহ; প্রকৃত execution ছাড়া দাবি করা হয়নি)

### Probhat

- [x] Standalone Nukta Physical Input সমর্থিত (AltGr+0x34, scan-code event)
- [x] Output U+09BC (byte-exact)
- [x] ক + Nukta code points সঠিক (U+0995 U+09BC)
- [x] ড + Nukta সঠিক (U+09A1 U+09BC)
- [x] ঢ + Nukta সঠিক (U+09A2 U+09BC)
- [x] ড়/ঢ় Existing Mapping অক্ষত (Shift+0x13/Shift+0x2D re-verified)
- [x] Physical Scan-code Test PASS (13 case + 62 probe, সব scan-event driven)
- [x] NFC Commit PASS (parity + stability)
- [x] Corpus 49/49 PASS
- [x] Updated Inventory PASS (69/69)
- [x] Regression Tests PASS (9 নতুন unit)
- [x] অন্য পাঁচ layout অপরিবর্তিত
- Known Limitations থেকে standalone nukta সরানো হয়েছে; extension document করা হয়েছে

### Global Regression

- [x] All 7 layout tests (576 unit + 2 integration)
- [x] Existing 557 unit tests সব PASS (কোনো assertion পরিবর্তন ছাড়া)
- [x] Existing 2 integration tests PASS
- [x] All Existing Regression Tests PASS (BUG-06, F-1..F-5 suite)
- [x] No New Unrelated Failure (0 FAIL in 579-row harness)
- [x] No Layout Mapping Regression (corpus+brief+inventory re-run)
- [x] No Unicode Regression (UNI checks)
- [x] Standards Mapping Document Updated (docs/standards/)
- [x] Evidence Complete (CSV × 2, harness source, unit tests, updated reports)

---

## 8. Windows Runtime Validation (Section 11) — BLOCKED

Linux sandbox-এ Windows 10 x64 + MSVC v143 অনুপস্থিত। **BLOCKED — WINDOWS
RUNTIME UNAVAILABLE**। Engine-level সব যাচাই সম্পন্ন; Windows-নির্ভর আইটেম
(MSVC build, TSF registration, SendInput, Notepad typing, clipboard,
layout-switching UI) পরবর্তী Windows run-এর জন্য harness প্রস্তুত:
`scripts/windows-ui-audit/Run-BanglaTypingTests.ps1`, `suborno.windows.*`
ctest targets, `merge_audit_results.py`। কোনো Windows Verified দাবি করা
হয়নি।

## 9. Changed Files

| File | Kind |
|---|---|
| src/layouts/khipro/khipro_layout.cpp | fix (qq token + explicit_virama state) |
| src/layouts/probhat/probhat_layout.cpp | fix (AltGr+0x34 → U+09BC + comment) |
| tests/unit/khipro_layout_test.cpp | +8 mission tests |
| tests/unit/probhat_layout_test.cpp | +8 mission tests |
| tests/unit/layout_typing_regression_test.cpp | +3 campaign guards |
| scripts/layout_verify/khipro_probhat_engine_verify.cpp | নতুন verification harness |
| docs/specifications/khipro-keymap.md | নতুন |
| docs/specifications/probhat-keymap.md | extension docs |
| docs/standards/BD_BENGALI_STANDARD_COMPLIANCE.md | নতুন |
| reports/layout-fixes/KHIPRO_PROBHAT_BDS_FIX_REPORT.md | এই report |
| reports/layout-fixes/KHIPRO_PROBHAT_REGRESSION_RESULTS.csv | 579-row fresh evidence |
| reports/layout-fixes/UNICODE_VERIFICATION.csv | 12-check character evidence |
| reports/layout-verification/** (khipro+probhat reports/inventories, master matrix/report, summary jsons) | status আপডেট |
| CHANGELOG.md | fix entry |
