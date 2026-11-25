# WINDOWS_UI_FUNCTIONAL_AUDIT

Project: SubornoIME · Date: 2026-11-22 (updated 2026-11-23) · Matrix: `docs/reports/UI_FUNCTIONAL_AUDIT_TRACEABILITY_MATRIX.csv` (**129 rows**, script-generated counts) · Executor: `scripts/windows-ui-audit/Invoke-SubornoUiAudit.ps1`

## 1. Surfaces (11) — inventory verified

| # | Surface | Matrix rows | Status |
|---|---|---|---|
| 1 | Settings Window (+ General/Interface/Topbar/Input/Footer pages) | 11+7+15+6+14+4 | Runtime interaction BLOCKED; logic contracts executed |
| 2 | About Dialog | 8 | BLOCKED (runtime), static-traced |
| 3 | Topbar | 9 | BLOCKED (runtime) |
| 4 | OSK | 11 | BLOCKED (runtime) |
| 5 | Toast | 2 | BLOCKED (runtime) |
| 6 | Popup Menu | 3 + component 3 | BLOCKED (runtime) |
| 7 | Tray Menu | 5 | BLOCKED (runtime) |
| 8 | Tray Icon | (within System tray rows) | BLOCKED (runtime) |
| 9 | TSF Candidate Window | 3 | BLOCKED (runtime, protected area) |
| 10 | MessageBox Dialogs | covered by state/notification rows | BLOCKED (runtime) |
| 11 | Tooltips | 1 | BLOCKED (runtime) |

No surface missing; each row carries Window name/class, AutomationId, control type, expected behaviour, UIA surface and the concrete blocker.

## 2. The 84 interactive checks

Every check from the original audit is retained — none deleted, none weakened, none re-counted. Current execution status of the full matrix (**script-generated from the CSV**):

| Status | Rows | Meaning |
|---|---|---|
| BLOCKED | 92 | Windows desktop/runtime required — blocker documented per row |
| PASS | 19 at this report's writing; **24 after 2026-11-24** | Really executed and matched (7 logic-layer contracts + inventory/i18n/editing/composition/switching/stress engine-level rows + BUG-05 regression + fuzz corpus regression executed 2026-11-23; +5 typing rows closed with engine evidence after fix campaign `c1462f4`) |
| FAIL | **0** (was 5 at this report's writing) | Executed and mismatched — **all five closed 2026-11-24** by fix commit `c1462f4` (BUG-06 chandrabindu, suborno corpus/ri-shadowing, khipro ঙ, national virama, paragraph, BUG-06 row); see `FINAL_UI_FUNCTIONAL_VERIFICATION.md` for the post-fix accounting (PASS 24 / FAIL 0) |
| NOT TESTED | 0 | (was 1: the fuzz-corpus row is now really executed under ASan+UBSan — `logs/fuzz_corpus_gcc_asan_ubsan.log`; coverage-guided libFuzzer mutation remains a Clang-only documented limitation) |
| NOT APPLICABLE | 13 | Static text; no behavior to activate (reason documented) |

The 84 GUI-interactive checks (buttons 21, nav 16, checkboxes 16, dropdown 10, menu 7, state 5, IME UI 3, slider 2, notification 2, tooltip 1, status 1) are all in the BLOCKED set with the .NET-UIA orchestrator mapped to their AutomationIds: the script enumerates `SubornoDesktopWindow`, invokes sidebar items (`sidebar.*`), exercises every checkbox id (`action.*`), dropdown options, slider bounds/clamping, menu items, and captures JSON results per check. UIA Invoke and real mouse click are exercised separately; an Invoke success is never treated as proof of a mouse-click PASS.

## 3. Save / Cancel / Reset / persistence

Settings-model contracts (SettingsTest + PersistenceCoverageTest + AtomicFileTest) executed inside the 543-test suite — PASS. Save/Cancel/Reset **button flows**, apply-without-restart, restart persistence of the visible UI and user-data backup/restore on a real machine: **BLOCKED** (runtime), with the persistence *engine* (settings.ini round-trip incl. `active_layout_id`) executed and PASS at engine level.

## 4. What a Windows operator must run

1. `scripts/windows-ui-audit/Invoke-SubornoUiAudit.ps1 -ExePath <Suborno.exe>` → fills `suborno-ui-audit-results.json`, exit code = FAIL count.
2. `scripts/windows-ui-audit/Manual-Test-Protocol.md` → human checks (tooltips, tray, toast focus, DPI, themes).
3. `layout_verification/windows-typing-harness/Run-BanglaTypingTests.ps1` → 295-case SendInput typing suite per layout.
4. Attach results into this matrix's `evidence_or_blocker` column and flip the row status per the 5-state rules (never BLOCKED→PASS).

## 5. DPI / Theme / Multi-monitor

All runtime geometry checks (100–200 % scaling, light/dark/system/high-contrast, multi-monitor) are BLOCKED — no display hardware exists in this environment. Static evidence (geometry math, DPI awareness manifests, palette switching code paths) is traced in the matrix rows; no runtime PASS is claimed.
