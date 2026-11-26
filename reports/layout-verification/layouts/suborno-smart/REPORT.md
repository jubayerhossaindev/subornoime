# Layout Report — Suborno Smart (`suborno-smart`)

- **Execution**: 2026-11-23 17:51Z run (Asia/Dhaka 2026-11-23 23:51) — fresh run from HEAD `a241477` (branch `main`, working tree clean)
- **Environment**: Linux sandbox (Debian 13, GCC 14.2.0, engine-level InputSession); Windows/MSVC unavailable
- **Pipeline**: InputSession `append_key` + `commit` — TSF text_service.cpp OnKeyDown-এর সমতুল্য engine path (probhat: scan-code events)
- **Overall Verdict**: **ENGINE VERIFIED — WINDOWS RUNTIME BLOCKED**

## 1. Implementation

Token-table Phonetic (stateless) — RuleBasedPhoneticLayout(kSmartTokens, accent=true); LayoutInputSession; src/layouts/phonetic/suborno_smart_layout.cpp

## 2. Engine-Verified Results

| Suite | Result | Evidence |
|---|---|---|
| Corpus (49 words × 6 layouts = 294) | PASS=49 | engine-results.csv ENG-CORP-* |
| Paragraphs (3 × 6 = 18) | PASS=3 | engine-results.csv ENG-PARA-* |
| Extended inventory (67 items × 6 = 402) | PASS=67 | inventory-results.csv |
| Brief sequences | FAIL=1, PASS=10 | brief-results.csv |
| Editing | PASS=3 | engine-results.csv ENG-EDIT-* |
| Composition | PASS=1 | engine-results.csv ENG-COMP-* |
| Stress | PASS=1 | engine-results.csv ENG-STR-* |
| Chandrabindu/Nga probe (6 words × 6 layouts = 36) | PASS=36/36 + dict=ON 4/4 | chandrabindu_nga_probe.txt |

## 3. Key Mapping (engine-verified)

| Feature | Keys / Behavior |
|---|---|
| Chandrabindu ঁ | `^` |
| Hasanta ্ | `,,` (k,,→ক্ PASS) |
| Khanda-ta ৎ | ``t```` (t``→ৎ PASS; ব্রিফের t'' কনভেনশন ভিন্ন) |
| Accent key | backtick ` (SubornoSmartLayoutTest.HandlesAccentKeyOverrides OK) |
| ঙ / ঞ | `Ng` / `NG` |
| ড় / ঢ় / য় | `R` / `Rh` / `y` |
| ঋ | `rri` |

## 4. Regression Evidence (fresh unit run: 557/557 OK)

| Bug/Fix | gtest | Status |
|---|---|---|
| — | `LayoutTypingRegressionTest.SmartUnchangedHeadlines` | PASS |
| — | `SubornoSmartLayoutTest.HandlesAccentKeyOverrides` | PASS |
| — | `SubornoSmartLayoutTest.ConvertsIndependentAndDependentVowels` | PASS |
| — | `SubornoSmartLayoutTest.SupportsRephPhalaAndSpecialSigns` | PASS |
| — | `SubornoSmartLayoutTest.PreservesCaseSensitiveConsonantTokens` | PASS |
| — | `SubornoSmartLayoutTest.PreservesLiteralAndUnknownUnicodeText` | PASS |

## 5. Findings

- Brief-এর `t''` FAIL — Suborno Smart-র khanda-ta token হলো t + backtick×2 (``t````→ৎ PASS; suborno_smart_layout.cpp:77-83); apostrophe নয়।
- `y`→য় PASS (nukta handling); `R`→ড়, `Rh`→ঢ় PASS।
- SmartUnchangedHeadlines এই রানে OK — Suborno/Suborno Classic fix-এ Suborno Smart mapping অপরিবর্তিত।
- Accent key = backtick (SubornoSmartLayoutTest.HandlesAccentKeyOverrides OK)।
- Standalone ় টাইপ করা যায় না — capability note।

## 6. Windows Runtime

সব Windows আইটেম **BLOCKED** — windows-results.csv (WIN-001..WIN-015), blocker: Windows 10 x64 + MSVC v143 environment অনুপস্থিত। Harness প্রস্তুত: `Run-BanglaTypingTests.ps1` (294-case SendInput), `Invoke-SubornoUiAudit.ps1`, `merge_audit_results.py`।

