# UI_FUNCTIONAL_AUDIT_04 — GUI Automation Report (Phase 2, 2026-11-24)

Mission 07 requires driving the app with a real Windows UI Automation client
(FlaUI / Inspect.exe / Accessibility Insights / WinAppDriver). **No such client
or Windows runtime exists in this sandbox — every live-automation item is
BLOCKED.** What this audit did instead: a line-by-line static audit of the
in-process UI Automation provider that the app itself implements, plus a full
inventory of what any future automation client will see.

## 1. Provider architecture (verified by source audit, ui_accessibility.cpp)

- Every surface implements the plain-C++ `ElementSource` view
  (`window_name/element_count/element_info/focused_element` + actions
  `focus/invoke/toggle/expand/set_element_value`); on `WM_GETOBJECT`
  (OBJID_CLIENT) the window heap-allocates an `ElementProvider` root and calls
  dynamically-resolved `UiaReturnRawElementProvider` (`uiaautomationcore.dll`
  resolved once; graceful DefWindowProc fallback when UIA is absent).
- Patterns implemented per element kind: Invoke (buttons, menu items, OSK
  keys, toast), Toggle (checkboxes; checked/radio popup items),
  ExpandCollapse (dropdowns, reported Collapsed), RangeValue (sliders:
  min/max/SmallChange 1.0/LargeChange 10.0), SelectionItem (sidebar),
  ValuePattern via `set_element_value` (sliders).
- Properties: Name, AutomationId (root `"window"`, elements `action.*` /
  `sidebar.*` / `zone.*` / `toast.message` / `menu.item.<n>` /
  positional `element-N` fallback), HelpText (topbar tooltips), ControlType
  (Window/Button/CheckBox/ComboBox/Slider/ListItem/Text), IsControlElement,
  IsKeyboardFocusable/HasKeyboardFocus, BoundingRectangle (client→screen),
  FrameworkId "Suborno", RuntimeId, ProcessId.
- Lifetime safety: `std::weak_ptr` liveness token — destroyed windows answer
  `UIA_E_ELEMENTNOTAVAILABLE` instead of touching freed memory (mission 7.2
  requirement). Focus events raised via `notify_focus_changed` on mouse-down
  and Tab moves.
- Runtime hit-testing: `ElementProviderFromPoint` (last match wins) and
  `GetFocus` implemented.

## 2. What an automation client will see (per surface)

| Surface | Elements | AutomationIds | Patterns |
|---|---|---|---|
| Settings window | 4 sidebar + current-page controls + Save/Cancel/Close | `sidebar.*`, `action.<name>` (28 distinct), `action.save/cancel/close`, `text.<n>` | per control type |
| Topbar | 4 | `zone.layout/wrench/close/mode` | Invoke (+HelpText) |
| OSK | 37 | `element-0..36` (positional — FINDING UIA-02) | Invoke |
| Toast | 1 | `toast.message` | Invoke |
| Menu popup | one per enabled item | `menu.item.<n>` | Invoke/Toggle |
| About | 5 | `element-0..4` (positional — FINDING UIA-02) | Invoke |
| Candidate window | 0 | — | deliberately not exposed (documented decision; screen-reader gap FINDING UIA-04) |

## 3. Static findings relevant to automation

- **UIA-03**: `IsEnabled` is hardcoded TRUE for every element
  (ui_accessibility.cpp:271-274). Disabled popup items only signal through
  focusable=false / Invoke refusal. A FlaUI pass asserting
  `IsEnabled == false` on disabled items would FAIL. Fix proposed, BLOCKED.
- **UIA-02**: OSK and About use positional fallback ids; stable targeting
  needs the explicit ids used by the other surfaces. Fix proposed, BLOCKED.
- **UIA-01**: OSK exposes the empty 34th slot as a button with an empty Name.
  Automation selectors matching "button with empty name" would hit it. Fix
  proposed, BLOCKED.
- Menu-popup element ordinal vs item index differ (separators skipped via
  item map) — documented so test automation does not conflate the two spaces.

## 4. BLOCKED (requires Windows)

- FlaUI/Inspect/Accessibility Insights tree walk of every surface; pattern
  invocation against the live app; UIA event subscription (focus,
  property-changed) verification; Narrator real sessions; WinAppDriver runs;
  destroyed-window element-not-available behavior observed live.
- Test vector recorded for the future session: TRC-044 in the traceability
  matrix.
