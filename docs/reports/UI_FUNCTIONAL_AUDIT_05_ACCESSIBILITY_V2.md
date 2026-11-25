# UI_FUNCTIONAL_AUDIT_05 — Accessibility Report (Phase 2, 2026-11-24)

Covers mission items: keyboard navigation, UIA names/roles/states, Narrator.
**Narrator live sessions are BLOCKED** (no Windows runtime); everything below
is verified either by executed tests or by source audit.

## 1. Keyboard model per surface (source-audited; live check BLOCKED)

| Surface | Keyboard support |
|---|---|
| Settings window | Full: Ctrl+Tab / Ctrl+Shift+Tab cycle pages; Tab/Shift+Tab walk every control → Save → Cancel (section labels skipped); Space toggles checkboxes, opens dropdowns, fires buttons; sliders ←/→ ±1, PgUp/PgDn ±10, Home/End to bounds; Enter = focused footer button (else Save); Esc = Cancel. Focus ring drawn; focus events raised to UIA. |
| About dialog | Esc closes; Tab/Shift+Tab cycle the 5 hotspots; Space/Enter activate; focus starts on hotspot 0. |
| Menu popup | ↑/↓ (skips separators/disabled), Enter chooses, Esc dismisses; keys routed while modal. |
| OSK | Esc hides. Keys themselves are mouse/UIA only (by design — they inject real keys). |
| Topbar | Not Tab-focusable (`focused_element() → -1`); right-click context menu reachable via keyboard-invoked `WM_CONTEXTMENU` anchored under the pill. |
| Candidate window | Never takes focus (MA_NOACTIVATE); selection keys are the composition keys (digits/arrows/PgUp/PgDn/Space/Esc) delivered through TSF. |
| Global hotkeys | Configurable layout hotkey (7 registerable combos) + Ctrl+Alt+E encoding toggle; both `MOD_NOREPEAT`. |

## 2. UIA names/roles/states

- All six exposed surfaces provide Name + ControlType + patterns (Report 04
  tables). Checkbox state comes from TogglePattern.ToggleState; slider value
  from RangeValue; popup items expose Toggle when checked/radio.
- **Names are localized through the same i18n tables the visuals use** —
  verified by executed test `ShippedLocaleFilesCarryEveryInventoryKeyInBothLanguages`
  (157 keys × 2 locales, non-empty; bn localized with 6 documented exceptions).
- Gaps (static findings, Report 08): UIA-01 empty-name OSK slot; UIA-02
  positional AutomationIds on OSK/About; UIA-03 IsEnabled always TRUE;
  UIA-04 candidate window exposes no UIA element (screen-reader users get no
  candidate announcements — documented design decision, flagged).

## 3. Narrator (BLOCKED)

Real Narrator verification (Tab-order reading, toggle-state announcements,
slider value announcements, dropdown expand/collapse, OSK key activation)
requires the live Windows shell: **BLOCKED**, tracked as TRC-044. No
Narrator compatibility claim is made.

## 4. Executed accessibility-adjacent evidence

- i18n completeness for every user-visible string incl. tooltips, toasts,
  error dialogs and confirm dialogs (157 keys × 2 locales): **PASS**.
- WCAG contrast floors for both palettes (`scripts/contrast_check.py`): 22
  pairs, **0 required-pair failures** (text ≥ 4.5:1, boundaries ≥ 3:1).
- Keyboard-focusable state flags are served per element
  (`IsKeyboardFocusable`/`HasKeyboardFocus`) — source-audited.
