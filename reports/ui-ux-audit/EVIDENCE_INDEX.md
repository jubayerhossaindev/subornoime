# EVIDENCE INDEX — Complete Existing UI/UX Audit

- **Audit date:** 2026-11-24
- **Repository HEAD:** `f8f33d2fb2e13854b9fb4442ce1dfe3f9340e02a` (short `f8f33d2`), branch `main`, working tree clean at audit start
- **Environment:** Linux sandbox (GCC build), **no Windows desktop / no MSVC** — every runtime UI evidence class is BLOCKED and marked as such; nothing was fabricated.

## 1. Executed evidence (fresh, at HEAD `f8f33d2`)

| Evidence ID | What was executed | Result | Artifact |
|---|---|---|---|
| EV-001 | Full portable test suite re-run (`build/ubuntu-dev`) | **557/557 unit + 2/2 integration PASS** | ctest console output; `suborno_unit_tests`, `suborno_integration_tests` |
| EV-002 | Locale parity + key-coverage + placeholder audit (re-implementation of the missing cited script) | en/bn **160 = 160**, 0 missing, 0 placeholder mismatch, 0 code-referenced gap, 2 dynamic call sites verified alive | `evidence/i18n_key_check_f8f33d2.txt` |
| EV-003 | WCAG contrast recompute from `ui_theme.hpp` literals (re-implementation of the missing cited script) | **22 pairs: 20 PASS; 2 sub-floor pairs have zero current consumers** (documented pairing rule `ui_theme.hpp:77-86`) | `evidence/contrast_check_f8f33d2.txt` |
| EV-004 | `scripts/brace_scan.py` | exit 0 | console |
| EV-005 | `scripts/check-layering.py` | layering OK: 20 cross-module edges | console |
| EV-006 | BUG-05 regression re-check: `kLayouts` display name | `L"বাংলা (Suborno Classic)"` clean at `application.cpp:139` — the **only** UI-source change since audit baseline `5724528` | `git diff 5724528..HEAD -- src/platform/windows/app/` |
| EV-007 | Previous-reports reconciliation (16 documents + 129-row CSV read in full) | 129 rows = 92 BLOCKED + 24 PASS + 13 N/A; 84 harness check IDs enumerated 1:1; "85 checks" claim not supported anywhere | reports 01–09, FINAL, WINDOWS_*, UI_UX_*; subagent summaries (Task 2-a / 2-b) |
| EV-008 | Source audit of all 6 custom-drawn surfaces + TSF candidate + installer (~10.5k lines) | inventory + control mapping verified | this report + `UI_INVENTORY.csv` |

## 2. Source evidence map (HEAD `f8f33d2`)

| UI Surface | Primary source | Lines |
|---|---|---|
| Settings window (Main) | `src/platform/windows/app/application.cpp` | 4,243 |
| About dialog | `application.cpp` L588–1205 | — |
| Topbar pill | `src/platform/windows/app/topbar.cpp` | 1,002 |
| On-Screen Keyboard | `src/platform/windows/app/osk_window.cpp` | 854 |
| Toast | `src/platform/windows/app/toast_window.cpp` | 364 |
| Menu popup | `src/platform/windows/app/menu_popup.cpp` | 425 |
| UIA providers | `src/platform/windows/app/ui_accessibility.cpp` | 843 |
| TSF candidate window | `src/platform/windows/tsf/candidate_list_ui_element.cpp` | 845 |
| Theme/token system | `include/suborno/platform/windows/app/ui_theme.hpp` | 687 |
| Locale tables | `data/i18n/{en,bn}/ui.json` | 160 keys each |
| Installer | `packaging/windows/suborno_installer.iss` | 132 |
| Automation harness | `scripts/windows-ui-audit/Invoke-SubornoUiAudit.ps1` | 333 |

## 3. Harness check-ID enumeration (84 happy-path IDs)

SET-OPEN(1) · NAV-sidebar.{general,interface,topbar,input}(4) · NAV-sidebar.<page>-invoke(4) · CHK-<16 ids>(16) · CHK-<id>-toggle(16) · DD-<10 ids>(10) · DD-<id>-options(10) · SLD-<2 ids>(2) · SLD-<id>-set-{min,mid,max}(6) · SET-SAVE(1) · SET-CANCEL(1) · SET-SAVE-persist(1) · SET-SAVE-mouse-coords(1) · SET-SAVE-mouseclick(1) · SET-SAVE-rapid(1) · SET-CANCEL-nopersist(1) · TOAST-element(1) · ABT-buttons(1) · OSK-elements(1) · TOP-open(1) · TOP-zone.{mode,layout,wrench,close}(4) = **84**.
Failure-path-only IDs: ABT-open, OSK-open, DD-<id>-select ×10, SET-SAVE-invoke, SET-CANCEL-invoke.
**Execution status in this sandbox: 0 executed / 84 BLOCKED** (Windows PowerShell 5.1 + built `Suborno.exe` required).

## 4. What is BLOCKED and why (never converted to PASS)

All 95 BLOCKED matrix rows + 20 BLOCKED screenshot slots share one root cause: **the audit sandbox has no Windows desktop**, so window launch, mouse/keyboard interaction, live UIA trees, Narrator, tray behavior, DPI transitions, and installer execution cannot be observed. The only sanctioned Windows-execution paths are:
1. `ctest -R suborno.windows.app_smoke` (`suborno_app --smoke-test`) on the `windows-2022` CI runner (`.github/workflows/ci.yml` L150–158),
2. `scripts/windows-ui-audit/Invoke-SubornoUiAudit.ps1` + `scripts/windows-ui-audit/Manual-Test-Protocol.md` (35 manual steps) on a real desktop.

## 5. Findings cross-reference

- All finding IDs (BUG-01…06, F-1…F-5, F1–F7(old), PH-01-*, N-01…N-07): `UI_UX_FINDINGS.csv`
- New in this audit: **N-01, N-02, N-03, N-04, N-05, N-06, N-07**
- Reconciliation verdicts: 27 previously-fixed claims re-verified → **VERIFIED FIXED** at source level (engine-level rows re-executed); 0 STILL PRESENT; 0 REGRESSION; 92 runtime rows carried BLOCKED.
