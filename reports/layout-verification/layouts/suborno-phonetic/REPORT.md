# Layout Report — Suborno (`suborno-phonetic`)

- **Execution**: 2026-11-23 17:51Z run (Asia/Dhaka 2026-11-23 23:51) — fresh run from HEAD `a241477` (branch `main`, working tree clean)
- **Environment**: Linux sandbox (Debian 13, GCC 14.2.0, engine-level InputSession); Windows/MSVC unavailable
- **Pipeline**: InputSession `append_key` + `commit` — TSF text_service.cpp OnKeyDown-এর সমতুল্য engine path (probhat: scan-code events)
- **Overall Verdict**: **ENGINE VERIFIED — WINDOWS RUNTIME BLOCKED**

## 1. Implementation

Rule-based Phonetic (stateful) — PhoneticInputSession + phonetic_parser/phonetic_tokenizer; pending-control state; dictionary candidates; src/layouts/phonetic/suborno_phonetic_layout.cpp

## 2. Engine-Verified Results

| Suite | Result | Evidence |
|---|---|---|
| Corpus (49 words × 6 layouts = 294) | PASS=49 | engine-results.csv ENG-CORP-* |
| Paragraphs (3 × 6 = 18) | PASS=3 | engine-results.csv ENG-PARA-* |
| Extended inventory (67 items × 6 = 402) | PASS=67 | inventory-results.csv |
| Brief sequences | PASS=15 | brief-results.csv |
| Editing | PASS=4 | engine-results.csv ENG-EDIT-* |
| Composition | PASS=6 | engine-results.csv ENG-COMP-* |
| Stress | PASS=7 | engine-results.csv ENG-STR-* |
| Chandrabindu/Nga probe (6 words × 6 layouts = 36) | PASS=36/36 + dict=ON 4/4 | chandrabindu_nga_probe.txt |

## 3. Key Mapping (engine-verified)

| Feature | Keys / Behavior |
|---|---|
| Chandrabindu ঁ | `^` (pa^c→পাঁচ PASS) |
| Anusvara ং / ঙ | context-sensitive (M/ng; baMla→বাংলা, Bangali→বাঙালি PASS) |
| Hasanta ্ | `:halant:` control (k:halant:→ক্) |
| Khanda-ta ৎ | `:khanda:` control |
| Danda । | `:danda:` control |
| Nukta ় | `:nukta:` control (consonant-run context) |
| ঋ / ৃ | `rri` (rri→ঋ, krripa→কৃপা PASS) |
| য় | `y:nukta:` |

## 4. Regression Evidence (fresh unit run: 557/557 OK)

| Bug/Fix | gtest | Status |
|---|---|---|
| BUG-06 | `LayoutTypingRegressionTest.SubornoChandrabinduIsTyped` | PASS |
| F-3 | `LayoutTypingRegressionTest.SubornoRiNoLongerShadowsRaPlusIKar` | PASS |
| F-4 | `LayoutTypingRegressionTest.SubornoNgaBeforeVelarFinalAndStandalone` | PASS |
| — | `LayoutTypingRegressionTest.SubornoKhandaControl` | PASS |
| — | `EngineTest.HandlesCoreBanglaTransliterationExamples` | PASS |
| — | `EngineTest.LayoutDisplayNamesAreCorrectAndScriptClean` | PASS |

## 5. Findings

- সব mandated টাইপিং টেস্ট engine-level-এ PASS (brief 15/15, corpus 49/49, inventory 67/67, paragraph 3/3)।
- Stateful PhoneticInputSession: pending control token (যেমন :danda) পরে close হয়; composition suite এটি যাচাই করে (ENG-COMP-01)।
- :nukta: control শুধু consonant-run context-এ ় যোগ করে (phonetic_parser.cpp:355-360); standalone অবস্থায় literal থাকে — এটি ডিজাইন অনুযায়ী সঠিক (নুকতা একটি combining mark)।
- BUG-06/F-3/F-4 fix commit c1462f4; সংশ্লিষ্ট regression টেস্ট এই রানে OK।
- Windows runtime: BLOCKED (সব WIN-xxx আইটেম)।

## 6. Windows Runtime

সব Windows আইটেম **BLOCKED** — windows-results.csv (WIN-001..WIN-015), blocker: Windows 10 x64 + MSVC v143 environment অনুপস্থিত। Harness প্রস্তুত: `Run-BanglaTypingTests.ps1` (294-case SendInput), `Invoke-SubornoUiAudit.ps1`, `merge_audit_results.py`।

