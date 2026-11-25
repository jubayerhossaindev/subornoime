# UI_FUNCTIONAL_AUDIT_02 — Functional Testing Report (Phase 2, 2026-11-24)

Scope: every inventoried control's *actual behavior* tested at the layer that
can really execute in this environment, plus the exact Windows-runtime steps
that remain BLOCKED. Test code: `tests/unit/ui_functional_audit_test.cpp`
(23 tests, all executed and passing) on top of the existing 609-test suite.

**Result: 632 unit + 2 integration tests PASS (0 failures). 4 static findings
open (Report 08). All live-window interactions BLOCKED (no Windows runtime).**

## 1. Executed functional coverage by mission checklist

### Buttons (mission 3.1) — model-level
- Save: repeated saves are byte-for-byte identical
  (`SaveIsIdempotentByteForByteWhenNothingChanged`); one-control save leaves
  every other value intact (`SavingOneControlLeavesEveryOtherValueIntact`);
  save-failure path leaves the previous file intact (existing
  `SaveFailureInReadOnlyDirectoryPreservesExistingSettings` still green).
- Cancel: staged copy dropped without touching the file; reloaded state keeps
  the pre-cancel values (`CancelLeavesPersistedStateUntouched`).
- Clear Learnt Words: immediate engine operation leaves a consistent
  engine + dictionary (`ClearLearntWordsLeavesEngineConsistent`).
- Click/double-click/rapid-click/disabled-click behavior on the *live window*:
  BLOCKED (Windows runtime).

### Dropdowns (mission 3.4) — every option walked, not one
| Dropdown | Options | Evidence |
|---|---|---|
| Start mode | 4 | `GeneralPageEveryControlKeyRoundTripsThroughSave` walks all 4 |
| Switching mode | 2 | same test walks both |
| Default layout | 7 | `DefaultLayoutDropdownCoversAllSevenRegisteredLayouts` walks all 7 against the live registry |
| Orientation | 2 | walked |
| Theme | 3 | walked (suborno-dark / suborno-light / system) |
| UI language | 3 | walked (system / en / bn) |
| Learning | 2 | walked |
| Shortcut | 9 accepted values + 4 rejected | `ShortcutDropdownEveryRegisterableOptionAccepted` |
| Encoding | 2 | walked + engine output flip (`ToggleProducesUnicodeAndBijoyOutput`) |
| Font | OS-default + families + Unicode names | `UIAuditFontFamily.*` |
Popup open/close/Escape/outside-click on the live dropdowns: BLOCKED.

### Checkboxes & toggles (mission 3.5/3.6) — both states
Every bool settings key was saved and reloaded in both directions (the page
round-trip tests flip each flag through Save; `InterfacePage…` walks
`show_candidate_window` true→false→true). A toggle that changes appearance but
not the feature would fail the engine-level tests (prediction toggle really
changes the candidate pipeline; encoding toggle really changes output bytes).

### Sliders (mission 3.7)
Idle opacity 0/1/50/99/100 and idle time 1/10/60/3599/3600 all persist;
101, 0, 3601, candidate_limit 0/101 are rejected at the model boundary
(`SliderAndLimitValuesOutsideRangeAreRejected`). Value-label live update and
RangeValue Narrator reporting: BLOCKED (live window). Recorded mismatch: UI
slider caps idle time at 60 s while the model allows 3600 (FIND-08, benign).

### Menus (mission 3.9)
Every tray menu item, topbar zone, and popup item has a traced handler chain
(Inventory CSV `event_handler` column, static-source-trace). Live menu
activation: BLOCKED. No dead menu entries were found except the pre-existing
`kMenuOpen` handler for an item that is never created (FIND-09, harmless —
unreachable branch, documented).

### Text inputs
No free-text fields exist in the shell (dropdowns + checkboxes + sliders
only); the Bangla text path itself is covered by the layout regression suites
(1,000+ assertions across 7 layouts) and the C-API boundary tests.

## 2. i18n / localization (Phase 04 item "verify localization")

- `ShippedLocaleFilesCarryEveryInventoryKeyInBothLanguages`: all 157
  render-time keys resolve non-empty in **both** `en` and `bn`; bn values are
  Bengali except 6 documented exceptions (physical key names + the word
  "English" — correct design).
- Project evidence script `scripts/i18n_key_check.py`: parity OK, 156 keys
  code-referenced, **0 missing, 0 dead**.
- Language switch down-stream effects (re-render of every surface, tray
  tooltip, OSK labels, About dialog): portable tables verified; live re-render
  BLOCKED.

## 3. What remains BLOCKED (cannot be honestly tested here)

| Checklist item | Why |
|---|---|
| Mouse click → handler → visible state on every live control | Windows 10/11 x64 + MSVC binary unavailable |
| Keyboard interaction on live windows (Tab/Space/Enter/arrows per window) | same |
| Rapid-click, busy-click, disabled-click on live buttons | same |
| Popup positioning/clipping, long labels, system-dependent options | same |
| Tray icon right/left click on the real shell | same |
| Toast visual timing on the real shell | same |
| Settings window Save while TSF profile busy | same |

Each is a single BLOCKED row in the traceability matrix (TRC-040..045); the
static wiring each depends on is verified and recorded per element.
