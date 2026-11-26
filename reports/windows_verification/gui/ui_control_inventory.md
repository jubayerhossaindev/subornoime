# UI Control Inventory — Source-Level (Windows Application)

Mission requirement: reconcile **source-inspection inventory** with the
**runtime UIA inventory**. Runtime discovery happens in the
`windows-uia-audit` workflow job (hosted Windows runner) via the repository's
own harness `scripts/windows-ui-audit/Invoke-SubornoUiAudit.ps1` (84
happy-path + 14 failure-path checks, PS 5.1 UIA, screenshots, envelope JSON).
Until that job executes on GitHub, **all runtime statuses below are
BLOCKED** — this file carries the source half of the reconciliation.

Source of truth: `src/platform/windows/app/application.cpp` (settings pages,
`CtrlKind`/`CtrlAction` enums at :87/:104), `topbar.cpp`, `menu_popup.cpp`,
`osk_window.cpp`, `toast_window.cpp`, `ui_accessibility.cpp`
(`IRawElementProviderSimple` custom provider exposing `AutomationId`,
`Name`, `ControlType`, `IsEnabled`, `BoundingRectangle`).

## 1. Windows (source-inspected)

| Window | Source | AutomationId surface | Runtime status |
|---|---|---|---|
| Main application (tray-backed) | `application.cpp` | tray menu items `menu.item.N` | BLOCKED |
| Topbar | `topbar.cpp` | 4 zones: `zone.layout`, `zone.mode`, `zone.wrench`, `zone.close` | BLOCKED |
| Settings (custom-drawn pages) | `application.cpp` | `sidebar.general/input/interface/topbar`, `page.N` controls, `action.save/cancel/close` | BLOCKED |
| OSK | `osk_window.cpp` | invokable keys (harness asserts ≥37) | BLOCKED |
| Toast | `toast_window.cpp` | `toast.message` | BLOCKED |
| Menu popup (tray/topbar) | `menu_popup.cpp` | `menu.item.<index>` | BLOCKED |
| Candidate window | `tsf/candidate_list_ui_element.cpp` | TSF `ITfCandidateListUIElement` provider | BLOCKED |

## 2. Settings controls (semantic inventory, `CtrlAction` 1–31)

| # | Action | Kind | Expected observable behavior | Runtime status |
|---|---|---|---|---|
| 1 | remember_layout | checkbox | persisted; layout remembered across restart | BLOCKED |
| 2 | start_with_windows | checkbox | registry Run key toggled | BLOCKED |
| 3 | layout_dropdown | dropdown (7 options: English, বাংলা Suborno/Classic, Smart, Khipro, Probhat, National) | subsequent typing uses selected layout | BLOCKED |
| 4 | show_candidate_window | checkbox | candidate window appears during composition | BLOCKED |
| 5 | idle_dimming | checkbox | topbar dims when idle | BLOCKED |
| 6 | idle_opacity | slider | topbar opacity actually changes | BLOCKED |
| 7 | idle_time | slider | idle threshold actually changes | BLOCKED |
| 8 | map_numpad | checkbox | numpad digits map on fixed layouts | BLOCKED |
| 9 | ignore_caps_lock | checkbox | caps-lock no longer toggles case mapping | BLOCKED |
| 10 | number_keys | checkbox | number-row output mode changes | BLOCKED |
| 11 | start_mode | dropdown | start-of-session mode | BLOCKED |
| 12 | font_dropdown | dropdown | topbar/OSK font family changes | BLOCKED |
| 13 | use_os_default_font | checkbox | overrides #12 | BLOCKED |
| 14 | remember_topbar_position | checkbox | topbar position restored | BLOCKED |
| 15 | layout_shortcut | dropdown | registered hotkey changes | BLOCKED |
| 16 | theme_dropdown | dropdown | theme applied to all windows | BLOCKED |
| 17 | next_word_prediction | checkbox | prediction candidates offered | BLOCKED |
| 18 | orientation | dropdown | candidate window layout orientation | BLOCKED |
| 19 | fixed_height | checkbox | candidate window height mode | BLOCKED |
| 20 | sliding_effect | checkbox | candidate window transition | BLOCKED |
| 21 | follow_caret | checkbox | candidate window tracks caret | BLOCKED |
| 22 | follow_mouse | checkbox | candidate window tracks mouse | BLOCKED |
| 23 | switching_mode | dropdown | profile switching semantics | BLOCKED |
| 24 | change_locale | dropdown | UI locale | BLOCKED |
| 25 | encoding | dropdown | output encoding (Unicode/legacy) | BLOCKED |
| 26 | disable_encoding_shortcut | checkbox | shortcut suppressed | BLOCKED |
| 27 | learning | checkbox | dictionary learning on commit | BLOCKED |
| 28 | clear_learnt | **button (destructive, confirm)** | learnt words cleared immediately | BLOCKED |
| 29 | ui_language | dropdown | UI language changes | BLOCKED |
| 30 | phonetic_correction | checkbox | phonetic auto-correction toggles | BLOCKED |
| 31 | typing_style | dropdown | typing style setting | BLOCKED |

Plus staged actions: `action.save` (persist + file rewrite), `action.cancel`
(no change), `action.close` (window closes).

## 3. Harness coverage mapping (executes on the GitHub runner)

The audit harness already implements the mission's mandatory interactions for
this inventory: every Settings element walked by AutomationId (Name,
ControlType, IsEnabled, non-empty BoundingRectangle), all 16 checkboxes
toggled twice, all 10 dropdowns opened via ExpandCollapse with option
enumeration, both sliders driven min/mid/max via RangeValue, Save via UIA
Invoke **and** via real mouse click (separate checks, both asserting the
persisted settings file was rewritten), Cancel asserting the file unchanged,
3 rapid Saves with process-liveness, About button count, OSK invokable count,
topbar-zone visibility, toast presence after a live toggle, per-window PNG
screenshots, envelope JSON merged into the traceability matrix by
`scripts/layout_verify/merge_audit_results.py` (`functional/checkmap.json` —
refuses unevidenced PASS upgrades).

What stays manual-protocol / runner-limited even after the workflow job:
Narrator sessions, High Contrast, 100–200% DPI, mixed-DPI drags, tray
keyboard walks, real TSF typing inside a third-party application (needs an
interactive desktop with a host editor; documented in
`tsf_runtime_report.md`).

## 4. Reconciliation status

- Source inventory: **COMPLETE** (this file).
- Runtime inventory: **BLOCKED — requires the GitHub Windows run**. The
  workflow job `windows-uia-audit` produces the runtime half
  (`uia-results.json` + screenshots); this file must then be updated with the
  runtime-discovered rows and statuses per mission §3.
- Duplicate-AutomationId investigation: `control_automation_id(control,
  index)` composes ids per page + index; no duplicates found by inspection;
  the runtime audit asserts non-empty ids and would surface duplicates as
  selector mismatches.
