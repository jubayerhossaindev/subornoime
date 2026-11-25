# Layout Report — Probhat (`probhat`)

- **Execution**: 2026-11-24 fix-mission re-run (Asia/Dhaka) — baseline HEAD `86a543e` (branch `main`, clean tree) + the khipro/probhat standalone-nukta fix commit
- **Environment**: Linux sandbox (Debian 13, GCC 14.2.0 / Clang 23, engine-level InputSession); Windows/MSVC unavailable
- **Pipeline**: InputSession `append_key` (scan-code events) + `commit` — TSF text_service.cpp OnKeyDown-এর সমতুল্য engine path
- **Overall Verdict**: **ENGINE VERIFIED (fix applied) — WINDOWS RUNTIME BLOCKED**

## 0. Mission Fix (2026-11-24)

**Standalone Bengali nukta (় = U+09BC) এখন টাইপ করা যায়** — **AltGr (Ctrl+Alt) + scan 0x34** (`.`/OEM_PERIOD key)।

- এটি একটি document-কৃত **SubornoIME Probhat Compatibility Extension** — upstream MIT Probhat KLC (princebillygk/probhat-layout-windows @ debc8310, 2026-11-24 তারিখে byte-level পুনঃযাচাইকৃত)-এ U+09BC কোথাও নেই এবং 0x34-এর AltGr column `-1` (empty)।
- নির্বাচনের যুক্তি: (1) unassigned physical key মূল block-এ অবশিষ্ট নেই; (2) AltGr একটি supported modifier layer (KLC column 6); (3) 0x34 = diacritic corner (Shift স্তরে ঁ, প্রতিবেশী 0x35-এ ্) — marks cluster-এর সাথে সামঞ্জস্যপূর্ণ; (4) কোনো existing key overwrite হয়নি (upstream -1 slot)।
- Output হুবহু `U+09BC`; ব্যঞ্জনের পরে decomposed থাকে (ক় = U+0995 U+09BC; ড় = U+09A1 U+09BC — composition exclusion)।
- `ক + ়` এক grapheme cluster (Lo + Mn ccc=7) — এক backspace-এ পুরো cluster মুছে যায়।
- `ক + ্ + ়` commit-এ NFC canonical reorder হয় (`ক ় ্` = U+0995 U+09BC U+09CD; nukta ccc=7 < virama ccc=9) — Unicode-নির্ধারিত আচরণ।
- আগের সব scan-code mapping অপরিবর্তিত: base `.`→।, Shift→ঁ, Shift+0x13→ড়, Shift+0x2D→ঢ়, 0x2C→য় — পুনঃযাচাইকৃত।

## 1. Implementation

Fixed scan-code mapping (MIT Probhat KLC) — ProbhatLayout + ProbhatInputSession; AltGr/CapsLock layers; src/layouts/probhat/probhat_layout.cpp

## 2. Engine-Verified Results

| Suite | Result | Evidence |
|---|---|---|
| Corpus (49 words × 6 layouts = 294) | PASS=49 | engine-results.csv ENG-CORP-*; fresh harness re-run 2026-11-24 |
| Paragraphs (3 × 6 = 18) | PASS=3 | engine-results.csv ENG-PARA-* |
| Extended inventory (now 69 items × 6) | PASS=69/69 probhat (2 new standalone-nukta rows) | inventory-results.csv |
| Brief sequences | PASS=83 (BRIEF-060 fixed) | brief-results.csv |
| Editing | PASS=3 | engine-results.csv ENG-EDIT-* |
| Composition | NOT_APPLICABLE=1 | engine-results.csv ENG-COMP-* |
| Stress | PASS=2 | engine-results.csv ENG-STR-* |
| Chandrabindu/Nga probe (6 words × 6 layouts = 36) | PASS=36/36 + dict=ON 4/4 | chandrabindu_nga_probe.txt |
| Mission scan probes + nukta cases (75) | PASS | reports/layout-fixes/KHIPRO_PROBHAT_REGRESSION_RESULTS.csv (brief-scan + probhat-standalone-nukta) |

## 3. Key Mapping (engine-verified)

| Feature | Keys / Behavior |
|---|---|
| Mapping ধরন | fixed physical scan-code (MIT Probhat KLC; 0x02–0x53) |
| Hasanta ্ | scan 0x35 (ক+0x35+ক→ক্ক PASS) |
| Chandrabindu ঁ | Shift+0x34 |
| Danda । | scan 0x34 |
| **Standalone nukta ় (নতুন)** | **AltGr (Ctrl+Alt) + 0x34 — SubornoIME Compatibility Extension** |
| রেফ / র-ফলা | র ্ ক → র্ক; ক ্ র → ক্র (scan path PASS) |
| ড় / ঢ় / য় | Shift+0x13 / Shift+0x2D / 0x2C (য় = য+় decomposed) |
| র-ফলা ছাড়া | Shift 0x2C = য |

## 4. Regression Evidence (fresh unit run: 576/576 OK — 557 existing + 19 mission tests)

| Bug/Fix | gtest | Status |
|---|---|---|
| Mission | `ProbhatLayoutTest.ProbhatStandaloneNuktaCanBeTyped` | PASS |
| Mission | `ProbhatLayoutTest.ProbhatNuktaProducesU09BC` | PASS |
| Mission | `ProbhatLayoutTest.ProbhatNuktaAfterConsonantWorks` | PASS |
| Mission | `ProbhatLayoutTest.ProbhatNuktaUsesExpectedScanCode` | PASS |
| Mission | `ProbhatLayoutTest.ProbhatNuktaDoesNotBreakExistingMappings` | PASS |
| Mission | `ProbhatLayoutTest.ProbhatNuktaCommitMatchesEncoding` | PASS |
| Mission | `ProbhatLayoutTest.ProbhatNuktaBackspaceWorks` | PASS |
| Mission | `ProbhatLayoutTest.ProbhatNuktaCompositionAndEditingEdges` | PASS |
| Mission | `LayoutTypingRegressionTest.ProbhatStandaloneNuktaIsTypeableViaScanCode` | PASS |
| NFC parity | `LayoutTypingRegressionTest.ProbhatSessionCommitMatchesTransliterateEncoding` | PASS |
| — | `ProbhatLayoutTest.IsARealFixedPhysicalKeyboardLayout` | PASS |
| — | `ProbhatLayoutTest.UsesPhysicalScanCodeInsteadOfTheActiveLogicalCharacter` | PASS |
| — | `ProbhatLayoutTest.MapsPhysicalKeyPositionsAndShiftLayer` | PASS |
| — | `ProbhatLayoutTest.MapsDigitsSignsAndPunctuation` | PASS |
| — | `ProbhatLayoutTest.BackspaceRemovesOneBengaliGrapheme` | PASS |
| — | `ProbhatLayoutTest.AltGrLayerFollowsTheUpstreamKlcColumns` | PASS |
| — | `ProbhatLayoutTest.CapsLockAppliesToSgCapsKeysMarkedInTheKlc` | PASS |
| — | `ProbhatLayoutTest.AppliesCapsLockOnlyToKeysMarkedInTheKlc` | PASS |
| — | `ProbhatLayoutTest.AppliesShiftLayerFromPhysicalKeyEvents` | PASS |
| — | `ProbhatLayoutTest.ConcatenatesLiteralFixedKeyOutputsWithoutPhoneticRewriting` | PASS |
| — | `ProbhatLayoutTest.PassesThroughUnmappedNumpadDigits` | PASS |
| — | `ProbhatLayoutTest.CtrlAloneOrAltAloneInjectsNoPreedit` | PASS |

## 5. Findings

- Physical scan-code path-এ এখন সব প্রোব PASS — পূর্বের 82/83 (BRIEF-060 ছিল NOT_TYPEABLE) + নতুন নুকতা প্রোব; BRIEF-060 এখন PASS (scan=0x34 altgr=1)।
- **RESOLVED (2026-11-24)**: standalone ়-এর জন্য AltGr+0x34 key যোগ হয়েছে — আগের KNOWN LIMITATION (upstream MIT Probhat KLC-তে U+09BC absent) দূর হয়েছে। এটি একটি document-কৃত SubornoIME Probhat Compatibility Extension (docs/specifications/probhat-keymap.md); original mapping অপরিবর্তিত।
- NFC commit parity PASS: য় commit হয় U+09AF U+09BC (decomposed, composition exclusion) হিসেবে — নতুন নুকতা আউটপুটও NFC-stable।
- Canonical reordering নোট: ক+্+় টাইপ করলে commit NFC স্বয়ংক্রিয়ভাবে ক+়+্ (U+0995 U+09BC U+09CD) সাজায় (nukta ccc=7 < virama ccc=9) — Unicode Standard অনুযায়ী সঠিক।
- Backspace এক grapheme cluster মুছে (ক় = এক cluster — এক BS-এ পুরোটা; ProbhatLayoutTest.BackspaceRemovesOneBengaliGrapheme OK)।
- AltGr/CapsLock স্তর upstream KLC column অনুযায়ী — শুধুমাত্র 0x34 AltGr slot একটি documented extension (upstream -1 slot-এ নতুন মান; কোনো existing key overwrite হয়নি)।

## 6. Windows Runtime

সব Windows আইটেম **BLOCKED** — windows-results.csv (WIN-001..WIN-015), blocker: Windows 10 x64 + MSVC v143 environment অনুপস্থিত। Harness প্রস্তুত: `Run-BanglaTypingTests.ps1` (294-case SendInput), `Invoke-SubornoUiAudit.ps1`, `merge_audit_results.py`।

