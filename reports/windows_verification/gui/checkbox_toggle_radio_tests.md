# Checkbox, Toggle and Radio Tests

Status: **BLOCKED (runtime)** — harness ids `CHK-*` + `CHK-*-toggle` (32 =
16 checkboxes × 2 states), executed by the workflow job.

## Checkboxes in scope (source inventory rows 1,2,4,5,8,9,10,13,14,16,17,19,20,21,22,26,27,30 — 16 live checkboxes on the settings pages)

`CHK-action.remember-layout`, `CHK-action.map-numpad`,
`CHK-action.ignore-caps-lock`, `CHK-action.number-keys`,
`CHK-action.next-word-prediction`, `CHK-action.idle-dimming`,
`CHK-action.follow-caret`, `CHK-action.follow-mouse`,
`CHK-action.candidate-fixed-height`, `CHK-action.sliding-effect`,
`CHK-action.disable-encoding-shortcut`, `CHK-action.learning`,
`CHK-action.phonetic-correction`, `CHK-action.use-os-default-font`,
`CHK-action.remember-topbar-position`, `CHK-action.show-candidate-window`
(the exact live set is walked dynamically by AutomationId prefix
`CHK-action.*`).

## Harness behavior (implemented)

For every checkbox: initial state read → toggle ON → toggle OFF (twice, both
states) → Save → persisted settings file re-read (the shared Save/Cancel
persistence checks prove the state reached disk) → state-change
notifications surface through the UIA provider.

## Mission §7 remaining items

- Disabled-state toggling: manual protocol.
- Radio-group semantics: the settings pages use dropdowns for exclusive
  choices (no Win32 radio groups found in source — see
  `ui_control_inventory.md`); if runtime discovery surfaces any, the audit
  reconciles them into the inventory.
- Persistence across restart: manual restart protocol (runner job is
  single-session).

A visual toggle without a settings-file change **fails** the run — the
persistence assertions make that a hard rule, not a convention.
