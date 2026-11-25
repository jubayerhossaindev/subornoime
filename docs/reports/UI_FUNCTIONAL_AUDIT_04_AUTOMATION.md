# Report 04 — GUI Automation Report (Windows UI Automation)

## 1. What exists in the app (verified by full source read)

The shell implements the standard MS pattern for owner-drawn Win32 content
(`src/platform/windows/app/ui_accessibility.cpp`, 845 lines + header):

- `ElementSource` (plain C++ view) implemented by **five** surfaces: Settings window,
  About dialog, Topbar, OSK, Toast; the menu popup is also a source (6 total).
- `WM_GETOBJECT` → `handle_get_object()` returns a heap-only ref-counted provider
  implementing `IRawElementProviderSimple` + `IRawElementProviderFragment(+Root)`.
- Patterns exposed per element kind, verified against `ElementInfo` flags:
  Invoke (buttons, OSK keys, topbar zones, toast dismiss), Toggle (16 checkboxes,
  radio/check menu items), ExpandCollapse (10 dropdowns), RangeValue (2 sliders with
  min/max/value), SelectionItem/ListItem (sidebar, menu items), Text (sections, status).
- AutomationIds are semantic and stable: `sidebar.general|interface|topbar|input`,
  `action.<semantic-name>` for all 29 control actions, `action.save/cancel/close`,
  `zone.mode/layout/wrench/close`, `toast.message`, `menu.item.<n>`, `osk.mode.*`,
  `osk.key.*`. `control_automation_id()`/`action_automation_name()` exist exactly once
  each (duplicate definitions found this audit were removed — Report 08 BUG-01).
- Focus: `focus_element()` moves the window's real keyboard focus (UIA SetFocus works);
  `notify_focus_changed()` raises focus events on Tab/Ctrl+Tab/click.
- Robustness: `uiautomationcore.dll` resolved dynamically (no UIA → DefWindowProc
  fallback, window names still announced); every provider holds a liveness token reset
  in `WM_DESTROY`/`WM_NCDESTROY` so stale providers answer
  `UIA_E_ELEMENTNOTAVAILABLE` instead of touching freed memory; all callbacks are
  exception barriers; providers are heap-allocated because UIA `Release()`s them.
- The TSF candidate window deliberately keeps its default-HWND fallback name
  ("Suborno candidates") — protected surface, documented decision.

## 2. What could NOT be executed here

Locating/operating controls through a real UIA client, pattern invocations, bounding
rect verification on screen, UIA event subscription, and Narrator sessions — all
require a Windows desktop: **BLOCKED**. No UIA claim is made from source reading
beyond the implementation inventory above.

## 3. Delivered: the runtime harness (no new dependencies)

`scripts/windows-ui-audit/Invoke-SubornoUiAudit.ps1` automates the whole interactive
matrix on a Windows 10/11 machine using the .NET Framework `System.Windows.Automation`
UIA client that ships with Windows — **no FlaUI install, no WinAppDriver, no new
dependency** (constraint held). It:

1. Launches `Suborno.exe` (or attaches), walks the Settings window by AutomationId,
2. verifies names/roles/patterns/bounding rects/IsEnabled for all 36 settings elements
   + Save/Cancel/Close,
3. toggles every checkbox via TogglePattern, opens every dropdown via
   ExpandCollapsePattern and walks **every option** via SelectionItemPattern, drives
   both sliders via RangeValuePattern (min/mid/max/keyboard),
4. exercises Save (asserts the persisted settings file contents), Cancel (asserts file
   unchanged), rapid repeated clicks, Escape semantics,
5. verifies the About dialog (5 buttons + Esc), the OSK (37 elements, Invoke types a
   test string into Notepad), the topbar zones, and the toast element after a live
   toggle,
6. emits a per-check PASS/FAIL log (JSON + console) suitable for attaching as evidence.

`scripts/windows-ui-audit/Manual-Test-Protocol.md` covers what UIA cannot: Narrator
reading sessions, High Contrast live toggles, 100–200% DPI + multi-monitor drags,
tray-menu keyboard walks, drag persistence, and screenshot capture for visual
regression.

## 4. Coverage accounting

| Metric | Value |
|---|---|
| Surfaces exposing UIA providers | 6 of 6 custom-drawn (candidate window intentionally excluded, documented) |
| Elements declared to UIA (settings alone) | 4 sidebar + page controls (16+10+2+1 per page set) + Save/Cancel/Close |
| Patterns implemented | Invoke, Toggle, ExpandCollapse, RangeValue, SelectionItem/ListItem, Text |
| Pattern invocations executed in this sandbox | 0 (BLOCKED) — all invocations run via the delivered harness on Windows |
