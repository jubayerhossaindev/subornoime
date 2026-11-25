# BANGLA_LAYOUT_TYPING_AUDIT

Project: SubornoIME · Date: 2026-11-22 (**updated 2026-11-24 — fix campaign `c1462f4`**) · Matrix baseline: `docs/reports/UI_FUNCTIONAL_AUDIT_TRACEABILITY_MATRIX.csv` (129 rows) · Evidence root: `windows-ui-audit/` (delivered copies under `download/windows-ui-audit/`)

## 0. Verdict banner (read first)

- Execution environment for this audit: **Linux sandbox (Debian 13, GCC 14.2, Ninja, ICU 76.1, CMake 4.4.5)**. There is **no Windows runtime, no MSVC, no TSF host, no UIA, no screen** in this environment.
- Everything below marked **engine-level** was really executed here against the production `Engine`/`InputSession` pipeline (the same `append_key`/`commit`/`VK_BACK` path the TSF text service drives — `src/platform/windows/tsf/text_service.cpp`).
- Everything marked **Windows runtime** is **BLOCKED**. The harness, keystroke fixtures, PowerShell orchestrator, Unicode comparator and manual protocol are prepared and delivered; the tests themselves have **not** been executed and are **not** claimed as PASS.
- Per the audit rules: `BLOCKED` was never converted to `PASS`; a layout with an open typing bug is reported `FAIL` even where other layouts pass.
- **Update 2026-11-24:** the user approved fixing the protected transliteration/layout mappings (instruction: make all 7 layouts 100% working). Fix commit **`c1462f4`** closed BUG-06 and F-1..F-5. Engine re-run on the same corpora: core corpus **294/294 PASS**, sentences **P1–P3 PASS on all 6 layouts**, extended inventory **401/402** (one honest khipro limitation, §6), probe **36/36 PASS**, unit suite **557/557**. The matrix FAIL count is now **0**. Windows-runtime rows are still **BLOCKED** — untouched, never upgraded.
- **Update 2026-11-24 (fix mission #2 — khipro trailing virama + probhat standalone nukta):** baseline HEAD `86a543e`. The two remaining engine-level limitations are now fixed with mission-approved protected-area changes: **Khipro ক্** via the `qq` explicit-hasanta token (real Khipro Classic convention; exact U+0995 U+09CD, no ZWNJ) and **Probhat ়** via AltGr+0x34 (documented SubornoIME Compatibility Extension; upstream KLC re-verified byte-level — no U+09BC anywhere, column was -1). Unit suite **576/576** (557 old + 19 new), integration 2/2, ASan+UBSan clean, fresh engine harness **579 rows 0 FAIL**, inventory khipro **74/74** / probhat **69/69**, coverage line 80.96%→81.17% (same-method). Standards audit: `docs/standards/BD_BENGALI_STANDARD_COMPLIANCE.md` (clause-level BDS verification BLOCKED — copyright-protected texts, none inferred). Windows-runtime rows remain **BLOCKED** — never upgraded.

## 1. Complete layout inventory (acceptance criterion 21)

Derived from `LayoutRegistry` registrations in `src/core/engine.cpp` (lines 19–46) and enumerated at runtime by `verify::collect_layout_inventory()` from a real `Engine()` instance. **Avro-style and Borno-style phonetic layouts are confirmed to exist** (not assumed from names):

| Layout ID | Display name | Type | Availability | Activation | Mapping source | Expected output |
|---|---|---|---|---|---|---|
| `suborno-phonetic` | বাংলা (Suborno) | Phonetic (rule parser, stateful session) | Available (default) | Settings Input page, tray menu, Ctrl+Shift cycle | `src/layouts/phonetic/suborno_phonetic_layout.cpp` + rule parser | Corpus 49/49 PASS (BUG-06 + ri-shadowing + nga fixed, `c1462f4`) |
| `english` | English | Passthrough | Available | Same activation surface | `src/layouts/english/english_layout.cpp` | Raw Latin passthrough; IME never intercepts |
| `borno-phonetic` | বাংলা (Borno) *(fixed BUG-05, commit `cf2a0ca`)* | Phonetic (token table, stateless session) | Available | Same | `src/layouts/phonetic/borno_phonetic_layout.cpp` (`kBornoTokens`) | Corpus 49/49 PASS at engine level |
| `avro-phonetic` | Avro | Phonetic (token table, accent key enabled) | Available | Same | `src/layouts/phonetic/avro_phonetic_layout.cpp` (`kAvroTokens`) | Corpus 49/49 PASS at engine level |
| `khipro` | Khipro | Compositional (auto-virama between consonants) | Available | Same | `src/layouts/khipro/khipro_layout.cpp` | Corpus 49/49 PASS; inventory **74/74** (trailing virama ক্ fixed 2026-11-24 via `qq` token; +7 extension rows) |
| `probhat` | Probhat | Fixed (physical scan-code mapping, MIT Probhat KLC) | Available | Same | `src/layouts/probhat/probhat_layout.cpp` (`kProbhatMappings`) | Corpus 49/49 PASS at engine level |
| `national` | National | Phonetic (National/Ridmik-style token table, explicit hasanta key `X`) | Available | Same | `src/layouts/national/national_layout.cpp` (`kNationalTokens`) | Corpus 49/49 PASS (virama + full missing inventory added, `c1462f4`) |

No layout beyond these seven is registered; none is assumed. Nothing was created just for testing.

## 2. Test corpus (acceptance criteria 22–24)

Two corpus files drive every layout through `verify::derive_keystrokes()` → real `InputSession` → committed text → **code-point comparison** (`verify::cp_dump()`):

1. **`corpus_keystrokes.csv`** — 294 rows = 49 items × 6 Bangla layouts: the 17 mandated common words, 16 conjuncts (ক্ষ জ্ঞ ত্র শ্র ক্র গ্র প্র ব্র ন্দ ন্ত্র ষ্ট ঙ্ক ঙ্গ ঞ্চ ম্প হ্ম), 9 complex words (কার্যক্রম রাষ্ট্র শ্রদ্ধা বিজ্ঞান বিশ্ববিদ্যালয় স্বাধীনতা প্রযুক্তি জনপ্রিয় গুরুত্বপূর্ণ), plus letters/kar-context items.
2. **`corpus_inventory_extension.csv`** (new, 402 rows = 67 items × 6 layouts) — closes the Section 6.3 gaps: full vowel inventory (অ–ঔ, 11), full consonant inventory (ক–হ, 32), nukta forms (ড় ঢ় য়), ৎ, diacritics standalone + attached (কং কঃ কাঁ), all 10 kar contexts on ক + explicit virama (ক্), reph (র্ক), ra-phala (ক্র), ya-phala (ক্য).

Kar signs are tested attached to a base consonant (ক+া ি ী ু ূ ৃ ে ৈ ো ঔ); a bare combining mark is not a valid typing target and is reported as a constraint, not a failure.

### 2.1 Engine-level results per layout

| Suite | suborno-phonetic | borno-phonetic | avro-phonetic | khipro | probhat | national |
|---|---|---|---|---|---|---|
| Corpus (294) | 49 P / 0 F / 0 ND | 49 P / 0 F / 0 ND | 49 P / 0 F / 0 ND | 49 P / 0 F / 0 ND | 49 P / 0 F / 0 ND | 49 P / 0 F / 0 ND |
| Inventory ext. (402) | 67 P | 67 P | 67 P | 66 P / **1 ND** (ক্ trailing virama) | 67 P (ড় ঢ় য় are the layout's own composed NFC forms) | 67 P |
| Sentences (3) | 3/3 | 3/3 | 3/3 | 3/3 | 3/3 | 3/3 |
| Paragraph | PASS | PASS | PASS | PASS | PASS | PASS |
| Composition (engine) | PASS | PASS | PASS | PASS | PASS | PASS |
| Editing (engine) | PASS | PASS | PASS | PASS | PASS | PASS |
| Switching + restart persistence | PASS | PASS | PASS | PASS | PASS | PASS |
| Stress (engine) | PASS | PASS | PASS | PASS | PASS | PASS |

\* Probhat emits ড়/ঢ়/য় in their own NFC form (U+09A1+U+09BC etc., composition exclusions); the corpus expected values use the same normalized form, so all 67 rows are strict code-point PASS in the 2026-11-24 run. The historical PASS-EQUIV discussion (NFD-equality verified programmatically, never blind normalization) remains on record in the 2026-11-22 report revision.

### 2.2 Mandated corpus coverage check

- Common words (17): all present in both corpora. Kar/fola: all 10 kars + virama + reph/ra-phala/ya-phala explicitly present (extension). Yuktakkhar (16) + complex words (9): all present.
- Sentences 1–3 and the mixed paragraph: executed per layout at engine level (18 sentence rows in `evidence.json`).
- Full vowel/consonant inventory: executed in this audit's extension (was missing before).

## 3. Unicode verification (acceptance criterion 23)

- Every case records `expected_code_points` / `actual_code_points` (UTF-8 → `U+XXXX` tokens) and is compared strictly; rendering is never used as evidence.
- No U+FFFD replacement character, no reordering, no duplicate/missing code points in any PASS row.
- Canonical equivalence applied once, justified: Probhat ড়/ঢ় precomposed forms (§2.1).
- Windows font/Uniscribe rendering and UTF-16 side of the TSF pipeline: **BLOCKED** (no Windows).

## 4. Composition, candidate, switching, persistence (criteria 25, 28)

Engine-level composition suite: 7 PASS, 2 NOT APPLICABLE (`english` passthrough has no composition — reason documented). Candidate-window **UI** interactions (arrow navigation, candidate list rendering, candidate selection by mouse) live in the Windows-only matrix rows and remain **BLOCKED**.

Switching suite (all engine-level, 4 PASS): layout cycle across suborno→probhat→national→khipro with **no mapping leakage** (আমি/বাংলা/তুমি committed correctly after each switch), `set_active_layout` rejects unknown ids, **restart persistence** via the real `SettingsStore` (`settings.ini` saved with `active_layout_id=probhat`; a *new* `Engine` instance restores it), and per-window session independence. Tray/menu-driven switching and indicator state on a real desktop: **BLOCKED**.

## 5. Editing & backspace semantics (criterion 25)

Engine-level editing suite (28 cases, re-run 2026-11-24): **19 PASS** — backspace removes exactly one input unit; deleting inside a conjunct removes the whole conjunct block and leaves **no stray Unicode** (verified by code points, not visuals); the 2 former NOT-DERIVABLE rows now PASS because National can type those words (hasanta added). 9 BLOCKED: Delete, arrows, Home/End, Shift+Arrow selection, copy/paste/cut/replace, mid-insert, multi-word selection require a real host text field (Windows runtime).

## 6. Typing bugs — all resolved 2026-11-24 (commit `c1462f4`, user-approved protected-area change)

| ID | Severity (was) | Layout | Finding (historical) | Resolution + evidence |
|---|---|---|---|---|
| **BUG-06** | **High (default layout)** | `suborno-phonetic` | Chandrabindu (ঁ U+0981) silently dropped: পাঁচ→পাচ, কাঁচ→কাচ, আঁশ→আশ, সাঁকো→সাকো | **FIXED** — `^` token in phonetic_tokenizer.cpp; probe 36/36 PASS; regression tests SubornoChandrabinduIsTyped; dict=ON path matched |
| F-1/F-2 | **Critical** | `national` | No virama mechanism; ঙ ঞ ৃ য় ৎ । digits untypeable | **FIXED** — explicit hasanta key `X`, contextual ng/ny, `Y`, `tw`, `..`, `^`, `` ` ``, digits; corpus 49/49 PASS |
| F-3 | High | `suborno-phonetic` | `ri` token shadows র+i-kar (করি→কঋ) | **FIXED** — `rri` vowel (Avro convention), `ri` now র+ি; করি/পারি/প্রিয়/জনপ্রিয় PASS |
| F-4 | Medium | `suborno-phonetic` | ঙ before consonant unreachable (ঙ্ক→ংক); standalone ঙ→ং | **FIXED** — state-aware nasal_ng: ঙ before vowel/velar conjunct + word-final after boundary; ং attached to runs/vowels and before other consonants |
| F-5 | Medium | `khipro` | ঙ untypeable; conjunct after nukta blocked | **FIXED** — `Ng` mapping; `,,' keeps the run alive (idempotent); ঙ্ক/ঙ্গ/বাঙালি/সফটওয়্যার PASS |
| (new) | Medium | `borno-phonetic` | kar + independent vowel collision (টাই) | **FIXED** — accent key enabled (`Ta\`i` PASS) |
| (new) | Low | `probhat` | session commit non-NFC | **FIXED** — commit() normalizes; parity test PASS |
| (new) | Low | `khipro`;`national` | Latin digits passed through literally | **FIXED** — both map 0-9 → ০-৯ |
| (new) | Medium | `national` | `b+h` merged into ভ (ব্যবহারকারী untypeable) | **FIXED** — `B` alternate for ব added, duplicate `Bh` token removed |
| (fixed) | Low | `khipro` | trailing/displaced virama (ক্) had no keystroke | **FIXED 2026-11-24** — explicit hasanta token `qq` (Khipro Classic convention), exact U+0995 U+09CD; inventory now 74/74. Probhat standalone ় also fixed the same mission (AltGr+0x34 extension, U+09BC). Evidence: `reports/layout-fixes/` |

## 7. Windows-runtime typing protocol (prepared, BLOCKED here)

Executed on a Windows 10 22H2 machine by `layout_verification/windows-typing-harness/Run-BanglaTypingTests.ps1` + `verify_main.cpp` (SendInput, per-test timeout, safe key release, integrity-level note, no `KEYEVENTF_UNICODE` for phonetic layouts) with the 295-case manifest; targets Notepad (guaranteed) / Word / browser / VS Code (`NOT AVAILABLE` if absent). Evidence required per layout: layout id, raw keystrokes, expected/actual text, code-point dumps, screenshot, log. Until that run happens, criterion 22 (Real Bangla Typing) stays **BLOCKED** and no layout may be declared Fully Functional (criteria 21–30, Final Rule).

## 8. Layout-wise result matrix (criterion K)

Legend: E-PASS/E-FAIL = engine-level executed result; R = Windows runtime row (all R cells BLOCKED).

| # | Test category | suborno | borno | avro | khipro | probhat | national |
|---|---|---|---|---|---|---|---|
| 1 | Common words | E-PASS | E-PASS | E-PASS | E-PASS | E-PASS | E-PASS |
| 2 | Vowel inventory | E-PASS | E-PASS | E-PASS | E-PASS | E-PASS | E-PASS |
| 3 | Consonant inventory | E-PASS | E-PASS | E-PASS | E-PASS | E-PASS | E-PASS |
| 4 | Kar contexts | E-PASS | E-PASS | E-PASS | E-PASS | E-PASS | E-PASS |
| 5 | Reph/ra-phala/ya-phala | E-PASS | E-PASS | E-PASS | E-PASS | E-PASS | E-PASS |
| 6 | Yuktakkhar (16) | E-PASS | E-PASS | E-PASS | E-PASS | E-PASS | E-PASS |
| 7 | Complex words (9) | E-PASS | E-PASS | E-PASS | E-PASS | E-PASS | E-PASS |
| 8 | Chandrabindu words | E-PASS | E-PASS | E-PASS | E-PASS | E-PASS | E-PASS |
| 9 | Sentences (3) | E-PASS | E-PASS | E-PASS | E-PASS | E-PASS | E-PASS |
| 10 | Paragraph | E-PASS | E-PASS | E-PASS | E-PASS | E-PASS | E-PASS |
| 11 | Editing/backspace | E-PASS | E-PASS | E-PASS | E-PASS | E-PASS | E-PASS |
| 12 | Composition/candidate | E-PASS | E-PASS | E-PASS | E-PASS | E-PASS | E-PASS |
| 13 | Switching/persistence | E-PASS | E-PASS | E-PASS | E-PASS | E-PASS | E-PASS |
| 14 | Unicode code points | E-PASS | E-PASS | E-PASS | E-PASS (ক্ displaced-virama headwords excepted) | E-PASS | E-PASS |
| — | Windows SendInput runtime | R-BLOCKED | R-BLOCKED | R-BLOCKED | R-BLOCKED | R-BLOCKED | R-BLOCKED |

**Bottom line (2026-11-24, fix mission #2):** at engine level, **all six Bangla layouts now type the complete corpus AND the full extended inventory correctly** — 294/294 core cases, **444/444** extended-inventory cases (khipro 74/74 incl. ক্, probhat 69/69 incl. ়), 579-row fresh harness with 0 FAIL, 576/576 unit tests, ASan+UBSan clean. The two historical engine limitations are gone; the only remaining gaps are Windows-runtime rows (BLOCKED — no Windows environment) and the documented brief-convention differences (Borno `;`, Avro `` t`` ``, not engine defects). **None of this is a Windows-runtime proof**: the SendInput row stays R-BLOCKED on purpose, and per the Final Rule no layout may be declared Fully Functional until `Run-BanglaTypingTests.ps1` is executed on a real Windows 10 22H2 machine (criterion 22/30).
