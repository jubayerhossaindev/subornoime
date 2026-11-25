# GUI Functional Verification — Evidence Tree

Mission: prove Suborno IME v19 works from an actual user's perspective —
every discoverable interactive control operated and its effect verified.

## Execution status — one honest headline

**Every runtime GUI status in this tree is BLOCKED**: the sandbox has no
Windows desktop and no GitHub-hosted runner access (root blocker:
`../environment.md`). What IS complete here:

1. **Source-level control inventory** (`ui_control_inventory.md`) — every
   window, control kind, semantic action (31 `CtrlAction`s), AutomationId
   surface, executed as source audits with file/line references.
2. **Reconciled harness mapping** — the repository's own UIA audit harness
   (`scripts/windows-ui-audit/Invoke-SubornoUiAudit.ps1`, 98 check ids in
   `functional/checkmap.json`) already implements the mission's mandatory
   interactions; it executes in the verification workflow's
   `windows-uia-audit` job and produces `uia-results.json` + PNG
   screenshots as artifacts.
3. **Coverage accounting** (`coverage_report.md`) — the explicit metrics
   formulae with the current honest (0% runtime, 100% source-mapped) values
   and what the runner run will produce.

## How to turn BLOCKED into evidence

```bash
git push -u origin ci/suborno-v19-windows-verification
gh workflow run suborno-windows-verification.yml --ref ci/suborno-v19-windows-verification
gh run download -n windows-uia-audit-<run_number>   # uia-results.json + screenshots
```

Then `scripts/layout_verify/merge_audit_results.py` merges the envelope JSON
into `docs/reports/UI_FUNCTIONAL_AUDIT_TRACEABILITY_MATRIX.csv` — refusing
unevidenced PASS upgrades by design.

## Files

| File | Mission section |
|---|---|
| `ui_control_inventory.md` | §2–§3 (completed source inventory) |
| `ui_automation_accessibility.md` | §4 |
| `button_functional_tests.md` | §5 |
| `slider_functional_tests.md` | §6 |
| `checkbox_toggle_radio_tests.md` | §7 |
| `menu_dropdown_tab_tests.md` | §8–§9 |
| `text_input_tests.md` | §10 |
| `mouse_keyboard_event_tests.md` | §11 |
| `osk_functional_tests.md` | §12 |
| `candidate_window_tests.md` | §9/§13 |
| `topbar_toast_tests.md` | §13-E |
| `tray_tests.md` | §13-F |
| `settings_persistence_tests.md` | §14 |
| `complete_user_journeys.md` | §13 |
| `visual_regression.md` | §21 |
| `coverage_report.md` | §20 |
| `failure_analysis.md` | §18 |
| `final_gui_acceptance.md` | §22 |

`logs/` and `screenshots/` are created by the runner job (envelope JSON and
PNGs land there via the artifact download).
