# UI_FUNCTIONAL_AUDIT_01 — Complete UI Inventory (Phase 2, 2026-11-24)

Mission: exhaustive UI/UX functional audit of the SubornoIME Windows native
shell (C++20 / Win32 / TSF / MSVC). This is the Phase 2 re-verification of the
`UI_FUNCTIONAL_AUDIT_01..09` series, rebuilt from the current source tree and
extended with the National-layout positional rewrite that landed after the
Phase 1 audits.

**Environment honesty statement (applies to the whole series).** This audit was
produced in a Linux sandbox (Debian 13, g++-14, ICU 76.1). The Windows
application itself cannot be launched here — MSVC, Windows 10/11 x64, the tray,
Narrator and UI Automation clients are all unavailable. Everything that can be
truly executed was executed (the portable logic layer, real engine sessions,
real persistence files, the shipped i18n data, the project's own evidence
scripts); every Windows-runtime check is recorded **BLOCKED** with its reason.
No BLOCKED or NOT TESTED item was converted to PASS anywhere in this series.

The full element-level inventory lives in
`UI_FUNCTIONAL_AUDIT_INVENTORY_V2.csv` (113 elements, one row per element with
handler, settings key, UIA identity, persistence contract and the verification
that already executed). Phase 1's inventory findings were re-verified line by
line against the current tree; nothing in the Windows UI layer moved except the
National layout engine, which is outside the shell.

## 1. Surfaces (8 windows + 2 non-window surfaces)

| Surface | Window class | Source | Elements inventoried |
|---|---|---|---|
| Settings window | `SubornoDesktopWindow` (WS_POPUP, 736×620 design px) | application.cpp:1619 | 38 |
| Floating topbar pill | `SubornoTopbarWindow` (layered, topmost) | topbar.cpp:873 | 10 |
| System tray icon + native context menu | no HWND (owned) | application.cpp:3733 | 13 |
| Toast notification | `SubornoToastWindow` (WS_EX_NOACTIVATE) | toast_window.cpp:300 | 11 |
| About dialog | `SubornoAboutWindow` (frameless, single-instance) | application.cpp:1130 | 8 |
| Custom menu popup | `SubornoMenuPopupWindow` (blocking modal loop) | menu_popup.cpp:359 | 5 |
| On-screen keyboard | `SubornoOskWindow` (34-key model) | osk_window.cpp:755 | 8 |
| TSF candidate window | `SubornoCandidateListUIElement` | candidate_list_ui_element.cpp:407 | 9 |
| Application-global shortcuts | RegisterHotKey x2 | application.cpp:3257/3293 | 2 (in Application row) |
| Cross-surface state (theme/DPI/persistence) | — | application.cpp | 4+7 rows |

## 2. Settings window control map (4 pages, every staged control)

Every control is a `UiControl`; every mutation is **staged** and committed only
by Save. Action IDs, handlers and i18n keys are in the CSV.

- **General** (6 controls + status line): Remember Last Input Layout (cb),
  Start with Windows (cb → HKCU Run key on Save), Start-mode dropdown
  (4 options), Switching-mode dropdown (2), Default-layout dropdown
  (7 = registered engine layouts), Windows input status line (painted,
  read-only).
- **Interface** (11): Show Candidate Window, Next Word Prediction (cb),
  Orientation dd (2), Fixed Height, Sliding Effect, Follow the Caret,
  Follow Mouse Pointer fallback (cb), Theme dd (3), UI Language dd (3),
  Use OS Default Bangla Font (cb, **clears staged font**), Font dd (OS Default
  + up to 16 curated families, **rebuilds pages**).
- **Topbar** (4): Enable Idle Dimming (cb), Idle Opacity slider (0–100,
  default 50), Idle Time slider (1–60 on UI; model allows 1–3600),
  Remember Topbar Position (cb).
- **Input** (9): Map Numpad to Bangla Numbers, Ignore Caps Lock in Bangla (cb),
  Learning dd (2), **Clear Learnt Words… (immediate + confirm dialog — not
  staged, not undoable by Cancel)**, Select candidate using number keys (cb),
  Layout-switch shortcut dd (F1 F2 F3 F4 F10 F12 Ctrl+Space + display-only
  reserved values), Encoding dd (2), bn-BD locale switch (cb),
  Disable Ctrl+Alt+E (cb).
- **Footer**: Save (`save_settings_from_ui`), Cancel (`cancel_settings`),
  title-row Close X (Cancel-equivalent).

## 3. Non-settings surfaces

- **Topbar**: 4 hit-test zones (layout pill → 7-layout radio popup; A/ব mode
  icon → immediate English↔Bangla toggle; wrench → Settings/OSK/Change
  Font/About; X → System Tray/Exit), native tooltips ×4, idle dimming timer,
  drag-with-persist, right-click → full tray menu.
- **Tray menu** (12 items + 2 separators): Show Topbar (checkmark), Keyboard
  Layout popup (7, radio), Font popup (System Default + families, radio),
  On-Screen Keyboard…, Settings…, Enable/Disable Suborno in Windows (dynamic
  label), Check for Updates…, Documentation, Clear Learnt Words…, Uninstall
  Suborno, Exit. Dynamic tooltip `"Suborno - {enabled|disabled} - {layout}"`.
- **Toasts** (9 kinds + dismissal behavior): settings_saved, language_changed,
  font_changed, layout_switched, encoding_unicode, encoding_bijoy,
  profile_enabled, profile_disabled, learnt_cleared. 2600 ms timer, click and
  UIA Invoke dismiss, WS_EX_NOACTIVATE.
- **About dialog**: 5 hotspots (updates, documentation, license, notices,
  close) with Tab/Space/Enter/Esc keyboard model, drag row, single-instance
  focus handoff.
- **Menu popup**: hover highlight, radio/checkmark gutter, disabled items,
  ↑/↓/Enter/Esc keys, WM_KILLFOCUS dismissal, per-item UIA `menu.item.<n>`.
- **OSK**: English/বাংলা mode buttons, close, 30 letter keys (real
  `SendInput` injection through TSF), Shift latch, Space/Backspace/Enter,
  Esc hides, `host.bangla_preview` labels from `Engine::layout_preview`.
- **Candidate window**: show gate (`show_candidate_window` + ≥2 candidates),
  click-to-commit, digit/arrow/page/space/esc keys (TSF), caret→mouse→corner
  placement chain, slide animation, never activates, deliberately no UIA.
- **Global shortcuts**: configurable layout hotkey (7 registerable combos),
  hardcoded Ctrl+Alt+E encoding toggle.

## 4. Verification method per element

Each CSV row carries `portable_verification_test` (the executed unit test(s)
whose contract covers the element's model-level behavior) and
`runtime_verification_status`. Execution kinds:

1. **linux-unit-test** — real execution in this audit (new `UIAudit*` suite:
   23 tests; existing suites: 609 tests; integration: 2).
2. **static-source-trace** — the handler chain and settings key were traced in
   source during this audit (recorded per element; never counted as runtime
   PASS).
3. **windows-runtime** — requires the live Windows shell; **BLOCKED** here.

## 5. Change log since Phase 1 audit

- National layout: engine rewritten to the positional BDS 1738:2018 layout
  (outside the shell; see `BDS_NATIONAL_LAYOUT_DISCREPANCY_REPORT.md`). OSK
  preview labels for all 7 layouts verified in this audit
  (`UIAuditOsk.PreviewLabelsForEveryRegisteredLayout`).
- Settings model: `font_family` validator now accepts Unicode (Bangla-script)
  font family names up to 32 code points (fix FIND-06, Report 08).
- i18n tables unchanged (160 keys per locale; parity re-verified).
