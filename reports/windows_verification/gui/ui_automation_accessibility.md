# UI Automation Accessibility Audit

Status: **BLOCKED (runtime)** — executes via `Invoke-SubornoUiAudit.ps1` in
the `windows-uia-audit` workflow job.

## Provider architecture (source-verified)

The application's settings/topbar/OSK/toast controls are **custom-drawn**
Win32 surfaces. They are not standard `BUTTON`/`COMBOBOX` windows; instead
`app/ui_accessibility.cpp` implements a real `IRawElementProviderSimple`
provider that exposes per-control `AutomationId`, `Name`, `ControlType`,
`IsEnabled` and `BoundingRectangle` — so the custom controls are reachable
from UIA clients (Narrator, the audit harness, FlaUI) without fake nodes.

## What the audit asserts per element (harness §2 walk)

- Non-empty `AutomationId` and `Name`; meaningful ControlType; `IsEnabled`
  and `BoundingRectangle` non-empty for every `sidebar.*`, `page.N`,
  `action.*`, `zone.*` element.
- Duplicate-AutomationId detection: ids are composed per page + index
  (`control_automation_id`); the runtime walk surfaces collisions as
  selector mismatches (none found by source inspection).

## Pattern expectations (mission §4 table → harness)

| Control | Expected pattern | Harness check |
|---|---|---|
| Button (save/cancel/close, OSK keys, About) | InvokePattern | `SET-SAVE-invoke`, OSK/About counts |
| Checkbox ×16 | TogglePattern (toggled twice) | `CHK-*` + `CHK-*-toggle` (32 ids) |
| Dropdown ×10 | ExpandCollapse + option enumeration | `DD-*` (30 ids) |
| Slider ×2 | RangeValue (min/mid/max) | `SLD-*` (8 ids) |
| List/menu item | selection/invoke per item | `NAV-*` (8 ids), `menu.item.N` |
| Topbar zones | visibility/enabled | `TOP-*` (5 ids) |

Keyboard accessibility and focus order: partially in the harness (menu
keyboard walks are manual-protocol per `checkmap.json` meta note).

## Evidence produced by the run

`uia-results.json` (per-check PASS/FAIL + screenshot paths), PNGs, merged
rows in the traceability matrix. Any failure follows
`failure_analysis.md` (fix → rebuild → re-audit).
