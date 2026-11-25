# Final GUI Acceptance

Status: **BLOCKED pending the Windows run** — this file will be updated with
per-area results from `uia-results.json` + the traceability merge.

| Area | Required result | Status |
|---|---|---|
| UI inventory | source + runtime reconciled | **PARTIAL** — source complete (`ui_control_inventory.md`); runtime BLOCKED |
| Buttons | individual functional results | **BLOCKED** |
| Sliders | range/drag/keyboard/behavior | **BLOCKED** (drag+keyboard manual protocol) |
| Checkboxes | state + persistence | **BLOCKED** |
| Toggles | real feature activation | **BLOCKED** |
| Radio groups | selection + behavior | **NOT APPLICABLE (pending runtime discovery)** — settings use dropdowns in source |
| Dropdowns | every option | **BLOCKED** |
| Menus | all applicable actions | **BLOCKED** |
| Tabs | every tab + content state | **BLOCKED** |
| Text fields | validation + handling | **NOT APPLICABLE (pending runtime discovery)** — no text control kind in source |
| OSK | every visible key | **BLOCKED** (invokable-count half runs; output half manual) |
| Candidate window | selection + actual commit | **BLOCKED** (engine half executed on Linux) |
| Topbar/Toast | all controls + notifications | **BLOCKED** |
| Tray | all menu actions | **BLOCKED** |
| Shortcuts | registered supported shortcuts | **BLOCKED** |
| Keyboard/mouse events | applicable event types | **BLOCKED** |
| Settings | save/reload/reset/persist | **BLOCKED** (portable logic executed on Linux) |
| UI accessibility | UIA properties + patterns | **BLOCKED** |
| Full workflows | actual user journeys | **BLOCKED** |
| Visual behavior | visual evidence | **BLOCKED** (screenshot capture wired) |
| Recovery | repeated interactions + restart | **BLOCKED** |
| Coverage | reconciled measured metrics | **0% runtime — see `coverage_report.md`** |

## Gate to acceptance

The GUI mission completes when (a) the workflow run executes, (b) every
harness check has PASS/FAIL with evidence, (c) failures are repaired per
`failure_analysis.md`, (d) `merge_audit_results.py` reconciles the matrix
without unevidenced upgrades, (e) this table goes PASS/BLOCKED-with-reason —
never a manufactured 100%.
