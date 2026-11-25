# WINDOWS_E2E_VERIFICATION

Project: SubornoIME · Date: 2026-11-22 · Related: Reports 03 (`UI_FUNCTIONAL_AUDIT_03_E2E.md`), matrix rows `E2E-*`

The 9 defined workflows are re-audited here. Rule: a workflow whose every step did not really succeed is **not** called Verified E2E. GUI steps require a Windows desktop → BLOCKED; engine-level equivalents that really executed are recorded as partial evidence — they never substitute for the runtime workflow.

| # | Workflow | Initial state → steps → expected | Actual result | Status | Evidence |
|---|---|---|---|---|---|
| 1 | Launch → Settings open | Start `Suborno.exe`; tray/settings opens `SubornoDesktopWindow`; sidebar 4 pages | Engine launches + registry initializes (7 layouts); window open/inspect BLOCKED (no desktop) | **BLOCKED** | matrix SET-OPEN row; `Invoke-SubornoUiAudit.ps1` ready |
| 2 | Language change + Save | Settings→language bn/en; Save; UI locale flips, toast shows, persists restart | i18n parity 160/160 keys executed (both locales complete); toast/save dialogs BLOCKED | **BLOCKED** (parity PASS as logic evidence) | `logs/i18n` evidence; `I18N-01` row |
| 3 | Font change + Apply | Input page font picker; candidate/OSK fonts update | Settings model persistence contract executed in 543-suite; visual apply BLOCKED | **BLOCKED** | matrix Input-page rows |
| 4 | Settings persistence after restart | change → Save → restart → values restored | **Engine-level PASS executed**: real `SettingsStore` round-trip, `settings.ini` saved with `active_layout_id=probhat`, NEW `Engine` restores it; full GUI restart flow BLOCKED | **PARTIAL-PASS / runtime BLOCKED** | `TYPE-SW-01` row; evidence.json switching |
| 5 | Layout change + Bangla typing | select layout per surface; type corpus; correct Bangla committed | Engine-level: borno/avro/probhat/khipro corpus PASS; **suborno-phonetic FAIL (BUG-06 chandrabindu, ri-shadowing)**; national FAIL (no virama); SendInput runtime BLOCKED | **PASS** (engine, after fix campaign `c1462f4` 2026-11-24; was FAIL) + **BLOCKED** (runtime) | `BANGLA_LAYOUT_TYPING_AUDIT.md` §2.1; BUG-06 row. Update 2026-11-24: BUG-06 + ri-shadowing + nga fixed (`c1462f4`, 14 regression tests); national virama added; corpus 294/294 PASS on all 6 Bangla layouts — runtime sign-off still BLOCKED |
| 6 | Phonetic composition + candidate selection | type → composition updates → candidate window → arrow/Enter/Escape | Engine composition suite 7 PASS (start/update/commit/cancel, Enter/Escape semantics), 2 N/A (passthrough); candidate **window UI** BLOCKED | **PARTIAL-PASS / runtime BLOCKED** | `TYPE-COMP-01` row |
| 7 | Tray menu → app control | tray icon → menu → settings/exit works | BLOCKED (no tray/desktop) | **BLOCKED** | matrix System tray rows |
| 8 | Toast notification + focus preservation | language-changed toast appears; typing focus preserved | Toast element + localized message logic executed (UIA contract); runtime display/focus BLOCKED | **BLOCKED** | matrix Toast rows; commit `7cc39a4` |
| 9 | Error dialog + localization | trigger error; dialog localized (bn/en) | i18n parity executed incl. dialog keys; dialog runtime BLOCKED | **BLOCKED** | BUG-03 fix `5724528`; `I18N-01` |

**New typing E2E workflows added by this audit** (matrix `TYPE-*` rows): per-layout SendInput typing (6), host-app editing (1), tray/menu switching + restart (1) — all BLOCKED pending a Windows machine, with the 295-case manifest and orchestrator delivered.

**Count:** 9/9 workflows executed end-to-end on a Windows desktop: **0**. Engine-level partial evidence: 3 (4, 5, 6). Honest status is recorded in the matrix; BLOCKED was never converted to PASS.
