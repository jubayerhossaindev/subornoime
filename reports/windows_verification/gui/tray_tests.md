# Tray Tests

Status: **BLOCKED (runtime)** — tray interaction is explicitly
manual-protocol territory (`checkmap.json` meta note: `TRY-ICON-*`,
`MNU-*` rows are covered by `Manual-Test-Protocol.md`, never auto-evidenced).

## Source-level facts (executed audit)

- Tray icon + menu live in the application lifecycle (`application.cpp`;
  the installer suite asserts the app + service deployment).
- Tray menu items are exposed to UIA as `menu.item.<index>`
  (`menu_popup.cpp`), so the menu content is discoverable once the menu is
  opened; the audit harness reveals Settings via the tray menu when the
  topbar wrench path is unavailable (harness §1 of its run sequence).

## Mission §13-F journey (F — tray and application lifecycle)

| Step | Status |
|---|---|
| Start application | runner: app smoke (`suborno_app --smoke-test`) + audit harness launch — BLOCKED |
| Locate tray icon | manual protocol (notification-area UIA walk is shell-dependent) |
| Open tray menu | manual protocol |
| Execute every non-destructive option | manual protocol (destructive = exit, last) |
| Open settings, restore windows | harness (wrench/tray reveal) — BLOCKED |
| Exit | manual protocol |
| Restart + clean initialization | manual restart protocol |

## Non-claims

No tray interaction is runtime-verified. The tray menu's semantic actions
mirror the settings actions whose engine behavior IS executed on Linux
(layout switching, profile enable/disable via `ITfInputProcessorProfiles`
compiled into `application.cpp`).
