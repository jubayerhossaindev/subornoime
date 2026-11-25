# Layout Report — National (`national`)

- **Execution**: 2026-11-23 17:51Z run (Asia/Dhaka 2026-11-23 23:51) — fresh run from HEAD `a241477` (branch `main`, working tree clean)
- **Environment**: Linux sandbox (Debian 13, GCC 14.2.0, engine-level InputSession); Windows/MSVC unavailable
- **Pipeline**: InputSession `append_key` + `commit` — TSF text_service.cpp OnKeyDown-এর সমতুল্য engine path (probhat: scan-code events)
- **Overall Verdict**: **ENGINE VERIFIED — WINDOWS RUNTIME BLOCKED**

## 1. Implementation

Token-table Phonetic (Ridmik-style) — NationalLayout explicit_halant state machine; src/layouts/national/national_layout.cpp

## 2. Engine-Verified Results

| Suite | Result | Evidence |
|---|---|---|
| Corpus (49 words × 6 layouts = 294) | PASS=49 | engine-results.csv ENG-CORP-* |
| Paragraphs (3 × 6 = 18) | PASS=3 | engine-results.csv ENG-PARA-* |
| Extended inventory (67 items × 6 = 402) | PASS=67 | inventory-results.csv |
| Brief sequences | PASS=17 | brief-results.csv |
| Editing | PASS=3 | engine-results.csv ENG-EDIT-* |
| Stress | — | engine-results.csv ENG-STR-* |
| Chandrabindu/Nga probe (6 words × 6 layouts = 36) | PASS=36/36 + dict=ON 4/4 | chandrabindu_nga_probe.txt |

## 3. Key Mapping (engine-verified)

| Feature | Keys / Behavior |
|---|---|
| Explicit hasanta | `X` (pXr→প্র, nXd→ন্দ, sXbadhInta→স্বাধীনতা PASS) |
| Chandrabindu ঁ | `^` (pa^c→পাঁচ PASS) |
| Vowel break | `;` (m;i→মই PASS) |
| Danda । | `..` (PASS) |
| Khanda-ta ৎ | `tw` (PASS) |
| ঙ / ঞ | `ng` / `ny` |
| জ্ঞ | `GY` (PASS) |
| ড় / ঢ় / য় | `R` / `Rh` / `Y` |
| Nukta ় | backtick ` (national_layout.cpp:87; k`→ক়) |
| ঋ | `rri` (krri→কৃ PASS) |

## 4. Regression Evidence (fresh unit run: 557/557 OK)

| Bug/Fix | gtest | Status |
|---|---|---|
| F-1/F-2 | `LayoutTypingRegressionTest.NationalExplicitHasantaFormsConjuncts` | PASS |
| F-1/F-2 | `LayoutTypingRegressionTest.NationalPreviouslyMissingCharacters` | PASS |
| F-1/F-2 | `LayoutTypingRegressionTest.NationalBreaksAndIndependentVowels` | PASS |
| — | `NationalLayoutTest.BasicConsonantTransliteration` | PASS |
| — | `NationalLayoutTest.VowelTransliteration` | PASS |
| — | `NationalLayoutTest.Signs` | PASS |
| — | `NationalLayoutTest.Clusters` | PASS |
| — | `NationalLayoutTest.Words` | PASS |
| — | `NationalLayoutTest.Candidates` | PASS |
| — | `NationalLayoutTest.EmptyInputYieldsNoCandidates` | PASS |

## 5. Findings

- Brief-এর সব 17 sequence PASS; explicit hasanta `X` (pXr→প্র, nXd→ন্দ, sXbadhInta→স্বাধীনতা)।
- Trailing virama সম্ভব: `pX`→প্ PASS (khipro-র বিপরীতে)।
- `;` = vowel break (m;i→মই, Ta;i→টাই), `..` = danda, `tw` = ৎ, `GY` = জ্ঞ, backtick = nukta token (national_layout.cpp:87; k`→ক়)।
- F-1/F-2 fix commit c1462f4; NationalExplicitHasantaFormsConjuncts / NationalPreviouslyMissingCharacters / NationalBreaksAndIndependentVowels এই রানে OK।

## 6. Windows Runtime

সব Windows আইটেম **BLOCKED** — windows-results.csv (WIN-001..WIN-015), blocker: Windows 10 x64 + MSVC v143 environment অনুপস্থিত। Harness প্রস্তুত: `Run-BanglaTypingTests.ps1` (294-case SendInput), `Invoke-SubornoUiAudit.ps1`, `merge_audit_results.py`।

