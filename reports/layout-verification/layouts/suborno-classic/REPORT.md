# Layout Report — Suborno Classic (`suborno-classic`)

- **Execution**: 2026-11-23 17:51Z run (Asia/Dhaka 2026-11-23 23:51) — fresh run from HEAD `a241477` (branch `main`, working tree clean)
- **Environment**: Linux sandbox (Debian 13, GCC 14.2.0, engine-level InputSession); Windows/MSVC unavailable
- **Pipeline**: InputSession `append_key` + `commit` — TSF text_service.cpp OnKeyDown-এর সমতুল্য engine path (probhat: scan-code events)
- **Overall Verdict**: **ENGINE VERIFIED — WINDOWS RUNTIME BLOCKED**

## 1. Implementation

Token-table Phonetic (stateless) — RuleBasedPhoneticLayout(kClassicTokens, accent=true); LayoutInputSession; src/layouts/phonetic/suborno_classic_layout.cpp

## 2. Engine-Verified Results

| Suite | Result | Evidence |
|---|---|---|
| Corpus (49 words × 6 layouts = 294) | PASS=49 | engine-results.csv ENG-CORP-* |
| Paragraphs (3 × 6 = 18) | PASS=3 | engine-results.csv ENG-PARA-* |
| Extended inventory (67 items × 6 = 402) | PASS=67 | inventory-results.csv |
| Brief sequences | FAIL=2, PASS=11 | brief-results.csv |
| Editing | PASS=3 | engine-results.csv ENG-EDIT-* |
| Stress | — | engine-results.csv ENG-STR-* |
| Chandrabindu/Nga probe (6 words × 6 layouts = 36) | PASS=36/36 + dict=ON 4/4 | chandrabindu_nga_probe.txt |

## 3. Key Mapping (engine-verified)

| Feature | Keys / Behavior |
|---|---|
| Chandrabindu ঁ | `^` |
| Hasanta ্ | `,,` (k,,→ক্ PASS) |
| Khanda-ta ৎ | `X` (X→ৎ PASS) |
| Accent key | backtick ` (a`→া, ka`→কা PASS) |
| ঙ / ঞ | `Ng` / `NG` |
| ড় / ঢ় | `R` / `Rh` |
| য় | `y` |
| ঋ | `rri` |
| vowel break | `moi`→মই (o-token-এর dependent form ফাঁকা) |

## 4. Regression Evidence (fresh unit run: 557/557 OK)

| Bug/Fix | gtest | Status |
|---|---|---|
| BUG-05 | `EngineTest.LayoutDisplayNamesAreCorrectAndScriptClean` | PASS |
| — | `LayoutTypingRegressionTest.ClassicAccentKeyBreaksVowelCollisions` | PASS |
| — | `SubornoClassicLayoutTest.ConvertsIndependentAndDependentVowels` | PASS |
| — | `SubornoClassicLayoutTest.PreservesCaseSensitiveConsonantTokens` | PASS |
| — | `SubornoClassicLayoutTest.ComposesConsonantRunsAndExplicitControls` | PASS |
| — | `SubornoClassicLayoutTest.EscapesLiteralGraphemesAndPreservesUnknownText` | PASS |
| — | `SubornoClassicLayoutTest.ExposesIndependentCandidateIdentity` | PASS |

## 5. Findings

- Brief-এর `Ta;i`/`mo;i` সিকোয়েন্স FAIL — Suborno Classic-তে `;` কোনো break key নয়, এটি literal হয়ে যায় (টা;ই/ম;ই)। Engine-এর নিজস্ব কনভেনশন: `Tai`→টাই, `moi`→মই — উভয়ই PASS (convention_sequences.csv)।
- Accent key হলো backtick (`), যা vowel-এর dependent form বাধ্য করে (`a`→া, `ka`→কা; rule_based_phonetic_layout.cpp:254-260)।
- Explicit hasanta = `,` ×2 (`k,,`→ক্ PASS); khanda-ta = `X`।
- Standalone ় এবং সাধারণ k+় টাইপ করা যায় না (য়/ড়/ঢ় শুধু dedicated token হিসেবে) — capability note, brief-mandated কোনো টেস্টে FAIL নেই।
- BUG-05 display-name fix commit cf2a0ca; EngineTest.LayoutDisplayNamesAreCorrectAndScriptClean এই রানে OK।

## 6. Windows Runtime

সব Windows আইটেম **BLOCKED** — windows-results.csv (WIN-001..WIN-015), blocker: Windows 10 x64 + MSVC v143 environment অনুপস্থিত। Harness প্রস্তুত: `Run-BanglaTypingTests.ps1` (294-case SendInput), `Invoke-SubornoUiAudit.ps1`, `merge_audit_results.py`।

