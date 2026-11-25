# Report 01 — Complete UI Inventory (Functional Audit, 2026-11-22)

**Baseline:** commit `6a124ea` (tree clean, after fixes `993968c` + `6a124ea`).
**Method:** every UI-relevant source file read in full this audit (`application.cpp` 4,141
lines, `topbar.cpp` 1,003, `osk_window.cpp` 855, `menu_popup.cpp` 425, `toast_window.cpp`
364, `ui_accessibility.cpp` 845 + header, `ui_theme.hpp` 688, `settings.hpp/cpp`,
`ui_strings.cpp`, both locale JSONs, `candidate_list_ui_element.cpp` + `text_service.cpp`
interaction paths). Nothing was inferred from names, screenshots or prior reports.

The machine-readable companion is
`UI_FUNCTIONAL_AUDIT_TRACEABILITY_MATRIX.csv` (104 rows, one per element/behavior).

## 1. Application surfaces (complete)

| # | Surface | Window class | Implementation | Opened by |
|---|---|---|---|---|
| 1 | Settings window (4 pages) | `SubornoDesktopWindow` | custom-drawn frameless 736×620 design window, staged-settings model, Save/Cancel footer | tray menu "Settings…", topbar wrench → Settings…, start-mode `main-window`, `show_window()` |
| 2 | About dialog | `SubornoAboutWindow` | frameless owner-drawn popup 560×430, 5 hotspots | topbar wrench → About Suborno… |
| 3 | Topbar pill | `SubornoTopbarWindow` | layered auto-sizing pill, 4 click zones + drag + right-click | tray left-click, second-instance message, start modes |
| 4 | On-screen keyboard | `SubornoOskWindow` | 37 invokable elements (2 mode buttons, close, 34 keys), injects real VKs via `SendInput` | tray/topbar menus "On-Screen Keyboard…" |
| 5 | Toast notification | `SubornoToastWindow` | frameless `WS_EX_NOACTIVATE` popup above tray, 2.6 s auto-dismiss | 8 shell events (see Report 02 §7) |
| 6 | Menu popup (component) | `SubornoMenuPopupWindow` | owner-drawn radio/check/disabled/separators, modal message loop | settings dropdowns, topbar menus, font pickers |
| 7 | Native tray menu | Win32 `HMENU` | `show_tray_menu_at()` builder, `handle_menu_command()` dispatcher | tray right-click, topbar right-click |
| 8 | System tray icon | `Shell_NotifyIconW` v4 | callback `WM_APP+1`, tooltip, `TaskbarCreated` re-add | shell |
| 9 | TSF candidate window | (protected surface) | `ITfCandidateListUIElement(Behavior)`, accessible name "Suborno candidates" | composition while typing |
| 10 | Error/confirmation dialogs | `MessageBoxW` | `show_error()` + Clear-Learnt `MB_YESNO`, every string routed through `ui::text` (`error.*`/`confirm.*`/`common.app_name` keys; BUG-04 fix) | failure paths, destructive action |
| 11 | Native tooltips | `TOOLTIPS_CLASS` | 4 rect-tracked zones on the topbar | hover |

**Explicitly not present** (unchanged from the Phase-01 audit, still honest): a dictionary
management window, a shortcut management window, an update-check UI (the menu entry opens
the releases page instead), notification toasts for error paths (documented contract:
errors use native MessageBoxes).

## 2. Settings window element census (from `build_pages()`)

| Page | Section labels | Checkboxes | Dropdowns (options) | Sliders | Buttons |
|---|---|---|---|---|---|
| General | 3 | 2 | 3 (4/2/7) | 0 | 0 |
| Interface | 3 | 7 | 4 (2/3/3/1+N≤17) | 0 | 0 |
| Topbar | 2 | 2 | 0 | 2 | 0 |
| Input | 4 | 5 | 3 (2/7+1/2) | 0 | 1 (Clear Learnt Words…) |
| **Total** | **12** | **16** | **10** | **2** | **1** |

Footer: **Save**, **Cancel**, **Close (X)** — plus title-bar drag and the status line
(General page, live Windows state). Sidebar: **4 page items**. Total interactive elements
in the Settings window: **16 + 10 + 2 + 1 + 3 + 4 = 36**.

## 3. Other surfaces

- **About dialog:** 5 buttons (Check for Updates / Documentation / License (MIT) /
  Open Source Notices / Close) + Tab order + Esc + drag.
- **Topbar:** 4 zones (mode toggle A↔ব, layout label menu, wrench menu, close menu) +
  drag-with-position-memory + right-click context menu + 4 tooltips + idle dimming.
- **OSK:** 2 mode buttons + close + 30 letter/digit keys + 4 special keys (Shift latch,
  Space, Backspace, Enter) = **37 elements**.
- **Tray menu:** 11 top-level items + 2 submenus (Keyboard Layout ×7 radio, Font ×(1+N))
  — every menu id (2001–2010, 2100+, 2200+) has a handler in `handle_menu_command()`.
- **Toast:** one element per event, 9 localized event keys (`toast.language_changed` wired by the BUG-03 fix).
- **Menu popup:** hover highlight, click select, ↑/↓/Enter/Esc, kill-focus dismiss,
  work-area clamping, off-window keystroke capture.
- **Candidate window (protected):** inventoried only — number/arrow/page/Esc/Enter/
  Backspace handling at `text_service.cpp:711–790`, settings consumption at
  `text_service.cpp:1000–1016`, per the protection constraint it was not modified.

## 4. Element records

Every element carries, in the traceability matrix: unique test id, surface, kind, label
i18n key, expected behavior, handler chain with `file:line` evidence, settings key,
persistence behavior, keyboard accessibility, UIA name/role/pattern/automation-id, and
its audit status. Visually similar controls are **not** assumed to behave identically —
each row is verified independently (e.g. the two sliders have different ranges, the two
footer buttons have different semantics, radio vs check items in menus map differently
to UIA Toggle state).
