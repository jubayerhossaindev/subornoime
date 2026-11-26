# FINAL_UI_FUNCTIONAL_VERIFICATION

Project: SubornoIME · Date: 2026-11-22 (**updated 2026-11-24 — engine fix campaign `c1462f4`**) · HEAD: `c1462f4` · This file is the final roll-up: counts are **script-generated** from `docs/reports/UI_FUNCTIONAL_AUDIT_TRACEABILITY_MATRIX.csv` (129 rows) and the evidence artifacts under `windows-ui-audit/`.

## 1. Final counts (generated from the matrix CSV)

| Execution status | Rows | Share |
|---|---|---|
| BLOCKED | 92 | 71.3 % |
| PASS | 24 | 18.6 % |
| NOT APPLICABLE | 13 | 10.1 % |
| FAIL | **0** | — (was 5; all five typing rows closed with engine evidence on 2026-11-24) |
| NOT TESTED | 0 | — |
| **Total** | **129** | (was 105 before this audit; +24 typing/bug/i18n rows, none double-counted) |

PASS rows are only ever (a) the 7 logic-layer contracts, (b) engine-level suites really executed here (inventory, corpus, editing, composition, switching, stress, unicode), (c) BUG-05 regression, (d) i18n parity, (e) the **fuzz corpus regression really executed 2026-11-23** (`LOG-FUZZ-01`: 14/14 corpus cases through the real `LLVMFuzzerTestOneInput` under ASan+UBSan, 0 crash / 0 UB / 0 leak — `logs/fuzz_corpus_gcc_asan_ubsan.log`; coverage-guided libFuzzer mutation stays a documented Clang-only limitation, not claimed as executed), and (f) the **5 typing rows closed 2026-11-24** (`TYPE-ENG-suborno-phonetic`, `TYPE-ENG-national`, `TYPE-ENG-khipro`, `TYPE-PARAG-01`, `TYPE-BUG-06`): the protected transliteration/layout fixes were approved by the user, landed in commit `c1462f4`, and the same corpora were re-run end-to-end — core corpus 294/294, sentences P1–P3 on all 6 layouts, extended inventory 401/402, probe 36/36, unit suite 557/557 (`tests/unit/layout_typing_regression_test.cpp` + regenerated `layout_verification/` evidence).

## 2. Remaining limitations (honest)

1. **No Windows runtime** is available here: 92 rows stay BLOCKED (UI interaction, UIA/Narrator/DPI/theme/monitor, SendInput typing, cross-app insertion, tray/toast/dialog display). Harness + protocol are delivered; execution on Windows is the next action.
2. **Typing bugs: all closed 2026-11-24** (commit `c1462f4`, user-approved protected-area change): BUG-06 chandrabindu, ri-shadowing, contextual ঙ, national hasanta + missing inventory, khipro ঙ + nukta-run, classic accent, probhat NFC commit, digits. The default layout is engine-level typing-correct again; every fix carries regression tests.
3. **One documented structural limitation remains**: Khipro has no explicit hasanta key, so a trailing/displaced virama (ক্ dictionary-headword form) is the single NOT_DERIVABLE item in the 402-case inventory (1/402); all other layouts type it. Normal Khipro words are unaffected.
4. Windows-only TUs (TSF/COM/WIL, 54 files) are outside the Linux compile DB — MSVC `/analyze` + runtime only.

## 3. Acceptance criteria — full accounting (20 original + 10 new)

| # | Criterion | Status | Basis |
|---|---|---|---|
| 1–11 | 11 surfaces inventoried · 84 interactive checks w/ blockers · real control interaction · Save/Cancel/Reset · settings persist · UIA tree/names · Narrator · dropdown/font picker · toast focus · DPI/theme · layout inventory | BLOCKED (runtime) / logic PASS where engine-level | matrix rows; §2 above |
| 12 | Per-layout Bangla typing | **PASS (engine level, all 6 layouts) / BLOCKED (runtime)** | corpus 294/294 on every layout after `c1462f4`; runtime row stays BLOCKED |
| 13–19 | words · kar/fola · yuktakkhar · sentences+paragraph · editing · candidate/composition · switching | engine-executed: **all 6 layouts PASS** per category matrix | `BANGLA_LAYOUT_TYPING_AUDIT.md` §2, §8 |
| 20 | All results in matrix with evidence | PASS | 129-row matrix, script-generated; every status change flows through `merge_audit_results.py`, which refuses unevidenced PASS upgrades |
| 21 | Complete layout inventory | **PASS** | 7 layouts from real registry; Smart/Classic confirmed to exist |
| 22 | Real Bangla typing on Windows | **BLOCKED** | harness ready; not executed |
| 23 | Unicode verification | **PASS (engine level)** | 696 cases code-point compared; canonical equivalence justified (Probhat ড়/ঢ়) |
| 24 | Complex characters correct | **PASS (engine level)** | kar/fola/yuktakkhar PASS on all 6 layouts after `c1462f4`; khipro displaced-virama headword form documented as the single limitation |
| 25 | Editing behaviour | **PASS (engine) / BLOCKED (host app)** | 19/28 PASS (re-run 2026-11-24; the 2 former NOT-DERIVABLE rows now PASS), 9 host-app rows BLOCKED |
| 26 | Sentence & paragraph per layout | **PASS (engine level)** | P1–P3 + paragraph PASS on all 6 layouts after `c1462f4` |
| 27 | Cross-application verification | **BLOCKED** | Notepad/Word/browser/VS Code unavailable |
| 28 | Layout switching | **PASS (engine) / BLOCKED (GUI)** | cycle/persistence executed |
| 29 | Restart persistence + stress | **PASS (engine) / BLOCKED (TSF-host items)** | settings.ini restart restore; 11/15 stress PASS |
| 30 | Typing evidence per layout | **PASS (engine artifacts) / BLOCKED (screenshots)** | keystroke fixtures + code-point CSVs + logs delivered; screenshots impossible here |

**Overall:** still **not a Windows-runtime audit — by design** — but the engine-level verdict is now green: 30-criteria accounting = engine-level PASS across the typing criteria (12, 13–19, 24, 26, 30) with runtime rows BLOCKED, **0 FAIL**, **0 NOT TESTED**, remainder BLOCKED with prepared tooling. The 92 BLOCKED rows were never converted to PASS; the 5 typing PASS upgrades each carry observed actual state + fix commit `c1462f4` + regression tests.

## 4. Immediate next actions

1. On Windows 10 22H2: `winget install Microsoft.winappcli`; run `Invoke-SubornoUiAudit.ps1`, `Run-BanglaTypingTests.ps1`, Accessibility Insights FastPass + Narrator protocol; per target app `NOT AVAILABLE` if absent.
2. ~~Approve minimal protected-area fixes for BUG-06 and F-1..F-5.~~ **Done 2026-11-24:** approved by the user, landed in commit `c1462f4` with 14 regression tests; full suite re-run (557/557).
3. ~~Execute the 1 NOT TESTED fuzz row or establish its blocker.~~ **Done 2026-11-23** — corpus executed under ASan+UBSan (`LOG-FUZZ-01` → PASS / runtime-executed); coverage-guided mutation documented as Clang-only.
4. Update the matrix statuses from the Windows run outputs (`Invoke-SubornoUiAudit.ps1` → `merge_audit_results.py --map windows-ui-audit/functional/checkmap.json`; 5-state rules; evidence over screenshots).
