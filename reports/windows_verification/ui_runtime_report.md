# Windows UI, DPI, Tray and Accessibility — Runtime Report

Status: **UI runtime BLOCKED** (no GitHub-hosted runner reachable); the
repository's own UIA audit harness executes in the verification workflow's
`windows-uia-audit` job. Source/build validation is complete.

## What exists and was audited (executed source audits)

- **UIA provider**: `app/ui_accessibility.cpp` implements
  `IRawElementProviderSimple` exposing `AutomationId`/`Name`/`ControlType`/
  `IsEnabled`/`BoundingRectangle` for the custom-drawn controls — the audit
  harness walks exactly these ids (`sidebar.*`, `action.*`, `page.N`,
  `zone.*`, `toast.message`, `menu.item.N`).
- **Audit harness**: `scripts/windows-ui-audit/Invoke-SubornoUiAudit.ps1`
  (PS 5.1, only .NET Framework UIA + System.Drawing — no new dependency):
  84 happy-path + 14 failure-path checks; UIA Invoke and real mouse clicks
  recorded as **separate** checks; Save/Cancel persistence asserted against
  the settings file; per-window PNG screenshots; envelope JSON merged into
  the traceability matrix by `scripts/layout_verify/merge_audit_results.py`.
- **Manual protocol**: `Manual-Test-Protocol.md` — the checks automation
  cannot do honestly (Narrator, High Contrast, 100–200% DPI, mixed-DPI
  multi-monitor drags, tray keyboard walks).

## Runtime mapping (mission §12 → harness)

| Area | Harness coverage | Status |
|---|---|---|
| Settings window (16 checkboxes, 10 dropdowns, 2 sliders, buttons) | element walk + toggles + ExpandCollapse + RangeValue | BLOCKED (job executes) |
| OSK | invokable key count ≥ 37 | BLOCKED |
| Topbar | 4 zone visibility/enabled | BLOCKED |
| Toast | `toast.message` presence after live toggle | BLOCKED |
| Tray | menu discovery + non-destructive actions | PARTIAL (menu items exposed as `menu.item.N`; tray-icon interaction is manual-protocol) |
| DPI / monitor change / focus | manual protocol (hosted-runner graphics constraints) | BLOCKED → manual |
| UIA pattern correctness | asserted per element in the envelope JSON | BLOCKED |

## Non-claims

No UI feature is claimed runtime-verified from compilation. Everything
runtime is BLOCKED until the workflow job runs; then per-check PASS/FAIL
lands in `uia-results.json` with screenshots, and failures are repaired per
the mission's failure-handling rules.

`gui/` contains the full GUI-mission evidence tree
(`gui/ui_control_inventory.md` = completed source-level inventory).
