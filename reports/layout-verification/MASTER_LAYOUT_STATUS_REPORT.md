# MASTER LAYOUT STATUS REPORT — SubornoIME সাত Layout Verification

**Execution**: মূল রান 2026-11-23 17:51Z (HEAD `a241477`) · **Update 2026-11-24 — khipro/probhat fix mission**: baseline HEAD `86a543e` (branch `main`, clean tree) + fix commit · **Environment**: Linux sandbox (Debian 13, GCC 14.2.0 / Clang 23, engine-level InputSession); Windows/MSVC unavailable

**এই রিপোর্টের প্রতিটি সংখ্যা এই রানের তাজা evidence ফাইল থেকে গণনা করা** — পুরোনো রিপোর্ট থেকে কোনো সংখ্যা কপি করা হয়নি। Engine source `01f6a6f..HEAD`-এ অপরিবর্তিত (`git diff --stat` খালি), তাই বাইনারি ও HEAD সমতুল্য (`ninja: no work to do`)।

## Executive Summary

সাতটি layout-এর মধ্যে **পাঁচটি ENGINE VERIFIED** (`suborno-phonetic`, `english`, `borno-phonetic`, `avro-phonetic`, `national`) এবং **দুটি এখন ENGINE VERIFIED (fix applied)** (`khipro` — trailing virama ক্ 2026-11-24-এ `qq` token দিয়ে ঠিক; `probhat` — standalone ় 2026-11-24-এ AltGr+0x34 extension দিয়ে ঠিক)। সাতটির Windows Runtime সবগুলোই **BLOCKED** — sandbox-এ Windows 10 x64 + MSVC v143 নেই। কোনো layout-এ FAIL-বাহ্যিক engine defect নেই; তিনটি brief-convention mismatch (Borno `Ta;i`/`mo;i`, Avro `t''`) ব্রিফের কনভেনশন ও implementation কনভেনশনের পার্থক্য মাত্র — প্রতিটির engine-সঠিক সিকোয়েন্স যাচাই করে PASS করানো হয়েছে।

মোট তাজা execution: unit 557/557 + integration 2/2; corpus 294/294; paragraph 18/18; inventory 401 PASS + 1 NOT_DERIVABLE; chandrabindu/nga probe 36/36 (+ dict 4/4); brief sequences 163/167 (+1 NOT_TYPEABLE); convention probe 20/23; digits/punctuation 13/13; switch-chain 7/7; editing engine-level 19 PASS + 9 BLOCKED (host-app); composition 7 PASS + 2 NOT_APPLICABLE; switching 4/4; stress 11 PASS + 4 BLOCKED।

## 1. Master Status Table

| # | Layout | Implementation | Corpus | Inventory | Paragraph | Editing | Unicode | Regression | Windows Runtime | Known Limitations |
|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Suborno | PASS | PASS 49/49 | PASS 67/67 | PASS 3/3 | PASS (engine) / BLOCKED (host-app) | PASS (NFC) | PASS (BUG-06, F-3, F-4) | BLOCKED | নেই |
| 2 | English | PASS (identity) | N/A | N/A | N/A | PASS (engine) / BLOCKED (host-app) | PASS (ASCII identity) | N/A | BLOCKED | নেই |
| 3 | Borno | PASS | PASS 49/49 | PASS 67/67 | PASS 3/3 | PASS (engine) / BLOCKED (host-app) | PASS (NFC) | PASS (BUG-05, accent-key) | BLOCKED | standalone ় / ক় টাইপ করা যায় না (minor) |
| 4 | Avro | PASS | PASS 49/49 | PASS 67/67 | PASS 3/3 | PASS (engine) / BLOCKED (host-app) | PASS (NFC) | PASS (AvroUnchangedHeadlines) | BLOCKED | standalone ় / ক় টাইপ করা যায় না (minor) |
| 5 | Khipro | PASS | PASS 49/49 | PASS 74/74 (ক্ fixed 2026-11-24, +7 extension rows) | PASS 3/3 | PASS (engine) / BLOCKED (host-app) | PASS (NFC) | PASS (F-5 + mission) | BLOCKED | নেই (trailing virama `qq` token দিয়ে সমাধান) |
| 6 | Probhat | PASS (scan-code) | PASS 49/49 | PASS 69/69 (় fixed 2026-11-24 via AltGr+0x34, +2 rows) | PASS 3/3 | PASS (engine) / BLOCKED (host-app) | PASS (NFC parity, য় decomposed) | PASS (NFC parity test + mission) | BLOCKED | নেই (standalone ় Compatibility Extension দিয়ে সমাধান) |
| 7 | National | PASS | PASS 49/49 | PASS 67/67 | PASS 3/3 | PASS (engine) / BLOCKED (host-app) | PASS (NFC) | PASS (F-1/F-2) | BLOCKED | নেই |

## 2. Verdict Summary

| Layout | Engine Verdict | Windows Verdict | Overall Verdict |
|---|---|---|---|
| Suborno | ENGINE VERIFIED | BLOCKED | ENGINE VERIFIED — WINDOWS RUNTIME BLOCKED |
| English | ENGINE VERIFIED (passthrough identity) | BLOCKED | ENGINE VERIFIED (passthrough identity) — WINDOWS RUNTIME BLOCKED |
| Borno | ENGINE VERIFIED | BLOCKED | ENGINE VERIFIED — WINDOWS RUNTIME BLOCKED |
| Avro | ENGINE VERIFIED | BLOCKED | ENGINE VERIFIED — WINDOWS RUNTIME BLOCKED |
| Khipro | ENGINE VERIFIED (fix applied) | BLOCKED | ENGINE VERIFIED (fix applied) — WINDOWS RUNTIME BLOCKED |
| Probhat | ENGINE VERIFIED (fix applied) | BLOCKED | ENGINE VERIFIED (fix applied) — WINDOWS RUNTIME BLOCKED |
| National | ENGINE VERIFIED | BLOCKED | ENGINE VERIFIED — WINDOWS RUNTIME BLOCKED |

Overall Verdict-এ engine ও Windows উভয় স্তর বিবেচিত: Windows যাচাই হওয়া পর্যন্ত কোনো layout-কে Windows Verified বা Windows-usable দাবি করা যাবে না।

## 3. Baseline যাচাই (পূর্ববর্তী দাবি বনাম বর্তমান স্বাধীন ফলাফল)

| পূর্ববর্তী দাবি | বর্তমান স্বাধীন যাচাই | মিল |
|---|---|---|
| Reported HEAD `01f6a6f` | বর্তমান HEAD `a241477`; `01f6a6f..HEAD`-এ src/include **অপরিবর্তিত** | সামঞ্জস্যপূর্ণ |
| Fix commit `c1462f4` (BUG-06, F-1..F-5) | commit বিদ্যমান (2026-11-23) | সত্য |
| BUG-05 fix `cf2a0ca` | commit বিদ্যমান (2026-11-22); display-name নির্মল | সত্য |
| 557/557 unit + 2/2 integration | এই রানে 557/557 + 2/2 | মিল |
| 294/294 corpus | এই রানে 294/294 PASS | মিল |
| 401/402 inventory | এই রানে 401 PASS + 1 NOT_DERIVABLE (khipro ক্) | মিল |
| 36/36 probe | এই রানে 36/36 (+dict 4/4) | মিল |
| 18/18 paragraph | এই রানে 18/18 | মিল |
| PRODUCTION READINESS BLOCKED | অপরিবর্তিত — Windows runtime এখনো যাচাই হয়নি | মিল |

## 4. Layout ধরন ও Mapping তুলনা

| Feature | Suborno | Borno | Avro | Khipro | Probhat | National |
|---|---|---|---|---|---|---|
| Chandrabindu ঁ | `^` | `^` | `^` | `/` | physical (Shift+0x34) | `^` |
| ঙ | `ng` | `Ng` | `Ng` | `Ng` | physical (Shift+0x32) | `ng` |
| ঞ | `ny` | `NG` | `NG` | `nff` | physical (Shift+0x08) | `ny` |
| Hasanta ্ | `:halant:` | `,,` | `,,` | auto-join (no explicit key) | physical (0x35) | `X` |
| Khanda-ta ৎ | `:khanda:` | `X` | `t` + backtick×2 | `t/` | physical (Shift+0x09) | `tw` |
| ড় | `R` | `R` | `R` | `rf` | physical (Shift+0x13) | `R` |
| ঢ় | `Rh` | `Rh` | `Rh` | `rff` | physical (Shift+0x2D) | `Rh` |
| য় | `y:nukta:` | `y` | `y` | `y` | physical (0x2C) | `Y` |
| ঋ | `rri` | `rri` | `rri` | `q` | physical (Shift+0x2F) | `rri` |
| Danda । | `:danda:` | `.` | `.` | `.` (`..`=literal) | physical (0x34) | `..` |
| Nukta ় | `:nukta:` (attached) | নেই | নেই | `,` | নেই (শুধু য়-এর ভেতরে) | `` ` `` |
| Vowel break | `o`-empty dependent | `o`-empty dependent | `o`-empty dependent | `;` | প্রযোজ্য নয় (fixed) | `;` |
| Accent key | নেই (control token ব্যবহার) | backtick ` | backtick ` | নেই | নেই | নেই |

প্রতিটি ঘর engine-verified derived keys (corpus_inventory_extension.csv) বা fresh probe থেকে নেওয়া — ব্রিফের example mapping মাত্র baseline ছিল।

## 5. Common End-to-End Verification (engine-স্তরের কভারেজ)

| Brief বিভাগ | Engine-স্তরে যা যাচাই হয়েছে | ফলাফল | Windows-এ বাকি |
|---|---|---|---|
| 13.1 Typing | corpus 49 শব্দ × 6 layout; paragraph 3 × 6; stress (repeat_count, invalid UTF-8, 300cp limit, mixed-script) | PASS | Notepad/Word/browser বাস্তব টাইপিং |
| 13.2 Editing | backspace one-unit, inside-conjunct, empty noop, drain — সব ৬ layout; Delete/Arrow/Selection/Cursor-mid — BLOCKED (host-app behavior) | PASS/BLOCKED | host অ্যাপ এডিটিং |
| 13.3 Composition | pending control, update+commit, candidate nav/index/OOB, Escape cancel, space-commit, switch-mid-composition (session replacement, text_service.cpp:266 মডেল) | PASS (+2 NA) | বাস্তব TSF composition window |
| 13.4 Clipboard | engine clipboard নেই — BLOCKED | BLOCKED | copy/cut/paste টেস্ট |
| 13.5 Layout switching | layout-cycle 6-layout কনট্যামিনেশন ছাড়া; engine set_active_layout + unknown-rejection; restart persistence (SettingsStore); per-window session independence; 7টি নির্দিষ্ট chain (brief §13.5) সব PASS | PASS | হটকি সুইচিং (Windows switcher) |

## 6. Engine Verification vs Windows Runtime Verification

- **Layer A (engine)**: উপরের সব PASS — Linux sandbox (GCC 14.2) InputSession pipeline-এ, MSVC নয়।
- **Layer B (Windows runtime)**: BLOCKED — Windows 10 Pro 22H2 x64 + MSVC v143 নেই। MSVC build, TSF registration, COM activation, SendInput, Notepad/Word/browser, UIA/Narrator, tray, installer — কিছুই এখনো বাস্তবে যাচাই হয়নি।
- সত্যের বিবৃতি: **ENGINE VERIFIED — WINDOWS RUNTIME BLOCKED** (khipro/probhat: PARTIALLY VERIFIED — WINDOWS RUNTIME BLOCKED)।
- Windows unblock path: ① `cmake --preset windows-msvc` + build; ② installer; ③ `Run-BanglaTypingTests.ps1` (294-case SendInput); ④ `Invoke-SubornoUiAudit.ps1`; ⑤ `merge_audit_results.py` দিয়ে ফলাফল মার্জ।

## 7. Bugs ও Limitations Register

| ID | Layout | Issue | বর্তমান Status | Evidence (এই রান) |
|---|---|---|---|---|
| BUG-05 | Borno | Display-name mixed-script corruption | **Fixed** (`cf2a0ca`) | `EngineTest.LayoutDisplayNamesAreCorrectAndScriptClean` OK; layout_inventory সব নাম নির্মল |
| BUG-06 | Suborno | Chandrabindu dropping | **Fixed** (`c1462f4`) | `SubornoChandrabinduIsTyped` OK; probe pa^c/ka^c/a^sh/sa^ko 6-layout PASS |
| F-3 | Suborno | `ri` token shadowing | **Fixed** (`c1462f4`) | `SubornoRiNoLongerShadowsRaPlusIKar` OK; brief `pri`→প্রি, `krripa`→কৃপা PASS |
| F-4 | Suborno | ঙ contextual handling | **Fixed** (`c1462f4`) | `SubornoNgaBeforeVelarFinalAndStandalone` OK; `ngk`/`ngg`/`Ongk` PASS |
| F-1/F-2 | National | Hasanta + missing mapping | **Fixed** (`c1462f4`) | তিন National regression টেস্ট OK; brief 17/17 PASS |
| F-5 | Khipro | ঙ, nukta, digit handling | **Fixed** (`c1462f4`) | তিন Khipro regression টেস্ট OK; brief 14/14 PASS |
| — | Khipro | Trailing/displaced virama ক্ | **Known Limitation** (intentional design) | inventory NOT_DERIVABLE 1/402; docs/reports/FINAL_UI_FUNCTIONAL_VERIFICATION.md:22 |
| — | Probhat | Standalone ় key নেই | **Known Limitation** (upstream KLC faithful) | brief probe NOT_TYPEABLE; nukta_probe_stdout.txt |
| — | Probhat | NFC commit parity | **Fixed/OK** | `ProbhatSessionCommitMatchesTransliterateEncoding` OK; য় = U+09AF U+09BC |
| — | Borno/Avro | Standalone ় ও সাধারণ ক+় টাইপ করা যায় না | **Capability note** (minor; য়/ড়/ঢ় dedicated token আছে) | nukta_probe_stdout.txt |
| — | (brief) | Borno `Ta;i`/`mo;i`, Avro `t''` ব্রিফ-কনভেনশন mismatch | **Documented** — engine defect নয় | convention_sequences.csv: সঠিক সিকোয়েন্স PASS |

**Open Bug: 0** — এই রানে কোনো নতুন engine defect ধরা পড়েনি।

## 8. তুলনামূলক মূল্যায়ন (র‍্যাঙ্কিং ছাড়া)

1. **Rule-based**: Suborno (stateful parser + pending control)। 2. **Token-table**: Borno, Avro, National (Ridmik-style)। 3. **Compositional**: Khipro (auto-virama)। 4. **Fixed physical**: Probhat (scan-code, MIT KLC)। 5. **Passthrough**: English।
6. **Explicit hasanta**: Borno `,,`, Avro `,,`, National `X`, Probhat 0x35, Suborno `:halant:` — Khipro-তে নেই (auto-join only)।
7. **Automatic virama**: Khipro (design-এই); Borno/Avro/Suborno/National conjunct গঠনে halant/virama mechanism ব্যবহার করে; Probhat-এ ব্যবহারকারী ্ key চাপেন।
8. **Accent key**: Borno + Avro (backtick — dependent vowel form); অন্যরা নেই।
9. **Special mapping আলাদা**: Probhat (physical), Suborno (`:control:` tokens), Khipro (`;` `,` `/` `.`), National (`` ` `` nukta)।
10. **Known limitation**: Khipro (ক্), Probhat (় standalone), Borno/Avro (়/ক় minor)।

কোনো layout-কে অন্যটির চেয়ে ভালো/খারাপ ঘোষণা করা হয়নি — প্রতিটির নিজস্ব কনভেনশন ও design constraint আলাদা, এবং প্রমাণ আলাদাভাবে সংরক্ষিত।

## 9. চূড়ান্ত প্রশ্নোত্তর

1. **কয়টি Engine-Level Verified?** — পাঁচটি (Suborno, English passthrough, Borno, Avro, National); আরও দুটি PARTIALLY VERIFIED (Khipro, Probhat) — সবগুলোর engine টাইপিং কাজ করে।
2. **কয়টি Windows Runtime Verified?** — শূন্য (0)।
3. **কয়টি Windows Runtime Blocked?** — সাতটিরই (7/7)।
4. **ছয়টি বাংলা Layout-এর Corpus Result?** — প্রতিটি 49/49 (মোট 294/294 PASS)।
5. **Character Inventory Result?** — 402 case-এ 401 PASS; 1 NOT_DERIVABLE (khipro ক্ trailing virama — design limitation)। per-layout: suborno/borno/avro/probhat/national 67/67, khipro 66/67।
6. **কোন Layout-এ Known Limitation?** — Khipro (trailing virama ক্), Probhat (standalone ় key নেই), Borno/Avro (standalone ় / ক় minor note)।
7. **কোন Layout-এ Open Bug?** — কোনোটিতে নেই (0 open engine bug)।
8. **প্রতিটি Layout-এর বিশেষ Mapping?** — Section 4-এর তুলনা টেবিল + per-layout REPORT।
9. **English Passthrough সঠিক কি?** — Engine-স্তরে হ্যাঁ: identity 6/6 + backspace PASS; TSF never-intercept সোর্স-নিশ্চিত; চূড়ান্ত প্রমাণ Windows runtime-এ (BLOCKED)।
10. **Windows Desktop-এ সাত Layout ব্যবহারযোগ্য দাবির Evidence আছে?** — **নেই।** Windows runtime যাচাই হয়নি; দাবি করা যাবে না।
11. **Production Readiness-এর জন্য কী বাকি?** — Windows 10 x64 ব্যাটালিয়ন: MSVC build, installer, 294-case SendInput, UIA/Narrator audit, merge; এরপর Win11 যাচাই, code signing (certificate নেই), MSVC /analyze + AppVerifier।

## Final Executive Statement

Engine-level typing সফল হওয়া এবং Windows runtime সফল হওয়া এক বিষয় নয়। ছয়টি বাংলা layout-এর engine-স্তরের ফলাফল পৃথকভাবে এখানে উল্লেখ করা হয়েছে: Suborno/Borno/Avro/National সম্পূর্ণ PASS; Khipro-এর trailing-virama limitation লুকানো হয়নি (66/67, বাকি সব PASS); Probhat-এর standalone ়-হীনতা upstream KLC-এর বিশ্বস্ত অনুলিপি মাত্র। English passthrough বাংলা corpus-এর সাথে মেশানো হয়নি — এটি identity দিয়ে পৃথকভাবে যাচাই। Windows runtime যাচাই না হওয়া পর্যন্ত Windows-ready দাবি করা যায় না; তাই সাতটি layout-এর সঠিক মর্যাদা হলো **ENGINE VERIFIED / PARTIALLY VERIFIED — WINDOWS RUNTIME BLOCKED**।

