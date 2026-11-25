# Layout Report — Khipro (`khipro`)

- **Execution**: 2026-11-24 fix-mission re-run (Asia/Dhaka) — baseline HEAD `86a543e` (branch `main`, clean tree) + the khipro/probhat trailing-virama fix commit
- **Environment**: Linux sandbox (Debian 13, GCC 14.2.0 / Clang 23, engine-level InputSession); Windows/MSVC unavailable
- **Pipeline**: InputSession `append_utf8`/`append_key` + `commit` — TSF text_service.cpp OnKeyDown-এর সমতুল্য engine path
- **Overall Verdict**: **ENGINE VERIFIED (fix applied) — WINDOWS RUNTIME BLOCKED**

## 0. Mission Fix (2026-11-24)

**Trailing/displaced virama (ক্ = U+0995 U+09CD) এখন টাইপ করা যায়** — explicit hasanta token **`qq`** (real Khipro Classic convention, khipro.khiproteam.com quickstart)। নিয়ম:

- ব্যঞ্জনের পরে `qq` → তাৎক্ষণিক `্` (U+09CD); পরের ব্যঞ্জন একই join সম্পন্ন করে (`kqqt`→ক্ত, দ্বিতীয় virama নেই)।
- Word-terminal অবস্থানে `kqq` → ক্ — exact `U+0995 U+09CD`, ZWNJ (U+200C) নেই (real Khipro Classic-এর `xx` trailing variant থেকে document-কৃত deviation, mission requirement অনুযায়ী)।
- Standalone বা vowel-এর পরে `qq` → silent break (National-এর bare-halant rule-এর মত)।
- Repeated `qq` → idempotent (`kqqqq`→ক্, কখনো দুটি virama পাশাপাশি নয়)।
- `/` slicer explicit virama-ও undo করে (`kqq/`→ক)।
- আগের সব key mapping অপরিবর্তিত — corpus 49/49, brief 14/14, inventory 67/67 পুনঃযাচাইকৃত।

Investigation trail: (1) `;`-এর terminal extension প্রত্যাখ্যাত — `;` হলো documented partition key (`k;t`→কত, `;;`→literal); (2) `/`-এর silent-ender case প্রত্যাখ্যাত — slicer/undo semantics documented (`ks/`→কস, code comment "k/ stays a silent ender"); (3) আপস্ট্রিম Khipro Classic-এর নিজস্ব explicit hasanta convention `qq` গৃহীত — collision-free (কোনো corpus/inventory/test input-এ `qq` নেই)।

## 1. Implementation

Compositional — KhiproLayout auto-virama consonant-run joiner; KhiproInputSession; src/layouts/khipro/khipro_layout.cpp

## 2. Engine-Verified Results

| Suite | Result | Evidence |
|---|---|---|
| Corpus (49 words × 6 layouts = 294) | PASS=49 | engine-results.csv ENG-CORP-*; fresh harness re-run 2026-11-24 |
| Paragraphs (3 × 6 = 18) | PASS=3 | engine-results.csv ENG-PARA-* |
| Extended inventory (now 74 items × 6 = 444) | PASS=74/74 khipro (INV-064 fixed + 7 new extension rows) | inventory-results.csv |
| Brief sequences | PASS=14 | brief-results.csv |
| Editing | PASS=3 | engine-results.csv ENG-EDIT-* |
| Stress | — | engine-results.csv ENG-STR-* |
| Chandrabindu/Nga probe (6 words × 6 layouts = 36) | PASS=36/36 + dict=ON 4/4 | chandrabindu_nga_probe.txt |
| Mission cases (26) | PASS=26 | reports/layout-fixes/KHIPRO_PROBHAT_REGRESSION_RESULTS.csv (khipro-trailing-virama + sentence-boundary) |

## 3. Key Mapping (engine-verified)

| Feature | Keys / Behavior |
|---|---|
| Auto virama | consonant-run এ automatic join (Ngk→ঙ্ক PASS) |
| **Explicit hasanta (নতুন)** | **`qq` — trailing ক্ (U+0995 U+09CD) এবং non-standard conjunct (`kqqt`→ক্ত)** |
| Vowel break | `;` (m;i→মই, t;i→তই PASS) |
| Nukta ় | `,` (k,,→ক়) |
| Chandrabindu ঁ | `/` (/→ঁ PASS) |
| Khanda-ta ৎ | `t/` |
| Danda । | single `.`; `..` = literal dot |
| ড় / ঢ় / য় | `rf` / `rff` / `y` |
| ঋ | `q` |
| Case sensitivity | uppercase T token নয় (literal) |

## 4. Regression Evidence (fresh unit run: 576/576 OK — 557 existing + 19 mission tests)

| Bug/Fix | gtest | Status |
|---|---|---|
| Mission | `KhiproLayoutTest.KhiproTrailingViramaCanBeTyped` | PASS |
| Mission | `KhiproLayoutTest.KhiproTrailingViramaProducesCorrectUnicode` | PASS |
| Mission | `KhiproLayoutTest.KhiproTrailingViramaCommitsCorrectly` | PASS |
| Mission | `KhiproLayoutTest.KhiproTrailingViramaBackspaceWorks` | PASS |
| Mission | `KhiproLayoutTest.KhiproTrailingViramaDoesNotBreakAutoJoin` | PASS |
| Mission | `KhiproLayoutTest.KhiproExplicitJoinRemainsUnchanged` | PASS |
| Mission | `KhiproLayoutTest.KhiproExistingCorpusRemainsUnchanged` | PASS |
| Mission | `KhiproLayoutTest.KhiproTrailingViramaEdgeCases` | PASS |
| Mission | `LayoutTypingRegressionTest.KhiproTrailingViramaIsTypeableWithCorrectCodePoints` | PASS |
| Mission | `LayoutTypingRegressionTest.KhiproTrailingViramaSessionCommitParity` | PASS |
| F-5 | `LayoutTypingRegressionTest.KhiproNgaIsTypeableAndJoinsConjuncts` | PASS |
| F-5 | `LayoutTypingRegressionTest.KhiproNuktaKeepsTheConsonantRunAlive` | PASS |
| F-5 | `LayoutTypingRegressionTest.KhiproDigitsAndHeadlines` | PASS |
| — | `KhiproLayoutTest.AutomaticallyJoinsAdjacentConsonants` | PASS |
| — | `KhiproLayoutTest.SupportsForcedVowelForms` | PASS |
| — | `KhiproLayoutTest.SupportsSlicerAndChandrabindu` | PASS |
| — | `KhiproLayoutTest.SupportsSpecialCharactersAndLiteralSeparator` | PASS |
| — | `KhiproLayoutTest.BasicConsonantAndVowelComposition` | PASS |
| — | `KhiproLayoutTest.CreatesCompositionSessionsAndCommitsTheSelectedOutput` | PASS |

## 5. Findings

- Brief-এর সব 14 sequence PASS; corpus 49/49; paragraph 3/3।
- **RESOLVED (2026-11-24)**: trailing/displaced virama (ক্) এখন explicit hasanta token `qq` দিয়ে টাইপ করা যায় — আগের KNOWN LIMITATION (INV-064 NOT_DERIVABLE, inventory 66/67) দূর হয়েছে। Inventory এখন 74/74 (7 নতুন extension row সহ)।
- বৈপরীত্য নোট: National explicit hasanta (`pX`→প্) এর মতই এখন Khipro-তেও trailing virama সম্ভব — layout-নির্দিষ্ট convention ভিন্ন হলেও capability parity আছে।
- Case-sensitive: uppercase T token নয় (literal passthrough) — ব্রিফের khipro তালিকা সঠিকভাবে lowercase ব্যবহার করে।
- দাঁড়ি = single `.`; `..` = escaped literal dot (khipro_layout.cpp)।
- `,` = nukta (k,,→ক়), `;` = vowel break (m;i→মই), `/` = chandrabindu, `t/` = ৎ।
- `qq`-এর সঙ্গে `,,`/`x`/`t/`/digits/punctuation-এর কোনো conflict নেই (26 mission case + full corpus re-run দ্বারা প্রমাণিত)।

## 6. Windows Runtime

সব Windows আইটেম **BLOCKED** — windows-results.csv (WIN-001..WIN-015), blocker: Windows 10 x64 + MSVC v143 environment অনুপস্থিত। Harness প্রস্তুত: `Run-BanglaTypingTests.ps1` (294-case SendInput), `Invoke-SubornoUiAudit.ps1`, `merge_audit_results.py`।

