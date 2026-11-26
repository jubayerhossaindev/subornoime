# Menu, Dropdown, Tab and Selection Tests

Status: **BLOCKED (runtime)** — harness ids `DD-*` (30: 10 dropdowns ×
open/option-count/select variants), `NAV-*` (8 sidebar navigation = the
settings "tabs"), `menu.item.N` (tray/topbar menus); executed by the
workflow job.

## Dropdowns (10, source inventory rows 3, 11, 12, 15, 16, 18, 23, 24, 25, 29)

- layout (7 options: English, বাংলা Suborno, বাংলা Classic, Smart, Khipro,
  Probhat, National), start_mode, font, layout_shortcut, theme,
  orientation, switching_mode, locale, encoding, ui_language.
- Harness: open via ExpandCollapse → **enumerate and count visible options
  (each > 0 asserted)** → selection/value-change assertions live partly in
  the harness (`DD-*-select` failure-path ids exist so a partial run still
  reconciles) and partly in the manual protocol (per-option behavioral
  verification).
- Cancel-without-change and keyboard navigation: manual protocol.

## Tabs (settings sidebar)

- `NAV-*` checks: activate every `sidebar.general/input/interface/topbar`
  page, assert the corresponding `page.N` control set becomes exposed
  (active-tab state + content), switch repeatedly.

## Menus (tray + topbar popups)

- Elements exposed as `menu.item.<index>` (source:
  `menu_popup.cpp`); open via the wrench zone / tray.
- Activate non-destructive items (Settings open, OSK toggle, layout switch);
  Escape/outside-click dismissal + disabled items: manual protocol (tray
  keyboard walks are explicitly manual per `checkmap.json` meta).

## Candidate list (selection control, mission §9)

Covered end-to-end at the engine level on Linux (686-test suite incl.
CandidateNavigation boundary/reject/reset tests, `part_iii/
candidate_navigation.md`) and at the UI level by the candidate-window
provider (`candidate_window_tests.md`) — runtime BLOCKED.
