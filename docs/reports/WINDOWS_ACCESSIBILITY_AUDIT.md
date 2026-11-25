# WINDOWS_ACCESSIBILITY_AUDIT

Project: SubornoIME · Date: 2026-11-22 · Companion: `WINDOWS_UI_FUNCTIONAL_AUDIT.md` (matrix), `WINDOWS_AUTOMATION_SETUP.md` (tools)

## 1. Scope and honest status

Accessibility verification covers the UI Automation properties/patterns, keyboard accessibility and Narrator behavior of every custom surface. In this Linux audit environment **no UIA runtime, no Accessibility Insights and no Narrator exist**, so:

- What was **executed**: the UIA-provider logic layer — the 543-test suite includes UIA surface/label contracts, locale-string validation (24-test validator with 98.9 % line / 99.5 % branch coverage, commit `f3e2489`), and the UIA name/pattern mapping tables used by `src/platform/windows/app/ui_accessibility.cpp`.
- What is **statically traced** in the matrix: per-element Accessible Name, AutomationId, ControlType and required patterns (13 static-traced rows + per-row `uia_surface` column).
- What is **BLOCKED**: FastPass/Tab-Stops/Live-Inspect runs, Narrator speech, focus-visibility/high-contrast/theme checks on a real desktop.

## 2. UIA properties verified statically (and how to confirm at runtime)

For each surface (Settings + 5 pages, About, Topbar, OSK, Toast, Popup/Tray menus, Font Picker, TSF candidate window):

| Property/Pattern | Where ensured | Runtime check (BLOCKED) |
|---|---|---|
| Window Name (localized) | localized window names for menu/toast (commit `7cc39a4`) | Accessibility Insights Live Inspect + Narrator window announcement |
| Accessible Name per control | label keys in `data/i18n/{bn,en}/ui.json` (160/160 parity executed) | Live Inspect element dump |
| AutomationId | `sidebar.*`, `action.*`, page/option ids in `application.cpp` | `winapp ui inspect` / orchestrator `Find-ById` |
| Control Type + Bounds | ListItems/Buttons/Checkboxes/Sliders per page | Live Inspect |
| Invoke/Toggle/Selection/Value/RangeValue patterns | popup menu items expose Invoke/Toggle + focus events; toast exposes a message element (commit `7cc39a4`) | pattern presence check per element |
| IsEnabled / IsOffscreen / IsKeyboardFocusable | state contracts in logic tests | FastPass |

A missing Accessible Name or missing required pattern must be filed as a bug when found at runtime — FastPass green alone does not close this audit.

## 3. Keyboard accessibility

Static contracts (matrix `keyboard_access` column): Ctrl+Tab page cycling, Tab/Shift+Tab order, Enter/Space activation, Escape dismissal, arrow navigation in dropdown/menus, Home/End, focus restoration after menu close. Engine-level keyboard handling (keystroke→composition→commit, Backspace, modifiers) is executed and PASS. Live Tab-Stops order, keyboard-trap detection and focus-ring visibility on the desktop: **BLOCKED**.

## 4. Narrator protocol (Windows machine)

1. `Win+Ctrl+Enter` → Narrator on.
2. Walk: Settings window open → each page → every control (announce name+state) → About dialog → Topbar → OSK → Popup/Tray menus (focus change announced) → Font Picker → Toast (message read; typing focus NOT stolen) → error/confirmation dialogs (localized Bangla announcement).
3. Record Speech Recap excerpts per check id into `windows-ui-audit/accessibility/`.
4. A control not announced, or announced with wrong state/localization, = documented FAIL in the matrix.

## 5. Visual accessibility checks (BLOCKED)

Contrast, text size, focus ring, high-contrast theme, theme change without restart, clipping, hit-target size, keyboard-only operation of every workflow — require a real display; all BLOCKED with the blocker recorded; palette/geometry logic is unit-traced only where the portable core exposes it.

## 6. Result

| Category | Executed here | Statically verified | BLOCKED |
|---|---|---|---|
| UIA names/patterns (logic) | PASS (543-test suite contracts) | 13 rows static-traced | runtime confirmation |
| Keyboard engine handling | PASS | — | host-app editing keys |
| Narrator | — | — | all (no screen reader) |
| Visual (contrast/HC/theme) | — | palette logic traced | all (no display) |

No BLOCKED item was converted to PASS. Re-run this audit on Windows with Tools 1/2/4 from `WINDOWS_AUTOMATION_SETUP.md` to complete sections 3–5.
