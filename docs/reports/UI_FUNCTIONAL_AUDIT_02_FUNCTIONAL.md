# Report 02 — Functional Testing Report (Per-Control Behavior)

**Audit rule applied:** a UI element is functional only when its complete expected
behavior is demonstrated with evidence. In this sandbox there is no Windows desktop, so
**click/toggle/drag-level execution is BLOCKED**, not passed. What this audit *did*
execute for real: the portable logic layer (542 unit + 2 integration tests, all passing),
full-source handler tracing for every element, and scripted static verifications. Every
BLOCKED row ships with a runnable Windows procedure in
`scripts/windows-ui-audit/` so it can be converted to PASS on a Windows machine.

## 1. Buttons (Settings Save / Cancel / Close, Clear Learnt Words…)

| Element | Contract (traced, file:line) | Status |
|---|---|---|
| Save | Press-and-release (fires on mouse-up inside, `handle_settings_mouse_up`); commits registry startup pref first with rollback on settings-save failure (`save_settings_from_ui`, lines ~2975-3029 pre-format); applies theme/language/hotkeys/fonts/topbar live; success toast; staged reset; hides. No silent discard. | BLOCKED (runtime) — rollback order and persistence traced |
| Cancel | Discards staged (`staged_ = engine_->settings()`), rebuilds pages, hides; **cannot** persist | BLOCKED |
| Close (X) | Same cancel contract as Cancel (deliberate: closing ≠ save) | BLOCKED |
| Clear Learnt Words… | Immediate (non-staged): `MB_YESNO` confirmation (default No), `engine_->clear_learnt_words()`, status refresh, success toast; error MessageBox on failure. Correctly **outside** Save/Cancel flow. | BLOCKED |
| Rapid-repeated clicks | Press-and-release model makes double-fire impossible for Save/Cancel (press state machine); Clear-Learnt re-enters confirmation each time | BLOCKED (negative; harness includes rapid-click case) |

## 2. Dropdowns (10 total)

Each dropdown independently traced: options ↔ settings mapping, selection handler,
persistence, keyboard open (Space), popup keyboard (↑/↓/Enter/Esc), work-area clamping.
Key mappings verified in `open_dropdown()`: start_mode index 0–3 → `StartMode` enum;
layout index → `kLayouts[].id`; theme 0/1/2 → suborno-dark/light/system; language
0/1/2 → system/en/bn; orientation/switching-mode/encoding binary mappings; learning
0=on; shortcut free-text → staged + page rebuild; font index 0 = OS default, 1..N =
family list. **Never marked PASS after testing one option — runtime enumeration of all
options is BLOCKED** and included in the harness (it walks every option via
ExpandCollapsePattern + SelectionItemPattern and verifies the persisted value).

## 3. Checkboxes (16 total)

Every checkbox has its own `toggle_staged_checkbox` case → exact settings field (16/16
traced, no shared/aliased handlers). Space toggles; Tab reaches each; UIA
CheckBox+Toggle reports live staged state. Dependent-control coupling exists and is
traced: "Use OS Default Bangla Font" clears the staged font family when checked
(mutual exclusion), and the font dropdown re-syncs via `staged_font_index()` on page
rebuild. Runtime two-state testing for each of the 16: BLOCKED (harness covers each).

## 4. Sliders (2)

- **Idle Opacity 0–100**, **Idle Time 1–60**, both clamped (`apply_staged_slider`).
- Mouse drag math verified correct (the Phase-01 double-offset bug is fixed: the code
  clamps `x` itself, with a comment documenting the regression).
- Keyboard: ←/→ ±1, PgUp/PgDn ±10, Home/End bounds — all present
  (`handle_settings_key_down`). Displayed value = stored value (one source:
  `control.value` painted and persisted).
- Downstream effect: alpha = `percent*255/100` in the topbar timer; dimming delay from
  `idle_time_seconds`. Runtime drag/range sweep: BLOCKED (harness drives RangeValue).

## 5. Menus

- **Tray menu:** 11 top-level ids + 2 submenus; `handle_menu_command()` handles every
  id; dynamic Enable/Disable label from live `profile_enabled()`; Show Topbar carries a
  live checkmark. Dead-id check: only `kMenuOpen` (alias of Settings, documented Low).
- **Topbar menus:** layout (radio 7), wrench (4 items), close (2 items), font
  (radio 1+N) — every chosen index routed to a real host callback; no decorative item.
- **Menu popup component:** hover/leave highlighting, enabled-item clicks only,
  ↑/↓ skipping separators/disabled, Enter selects, Esc and kill-focus dismiss,
  keystrokes captured even when focus is elsewhere; radio vs check rendered distinctly
  and mapped to UIA Toggle state.
- Localized labels: full coverage verified by the key cross-check (see Report 05 §4).

## 6. Text/Unicode behavior

No editable text fields exist in the shell UI (settings are choice-based) — the
 Bengali-text/surrogate/undo requirements therefore apply to the engine layer, where
they are covered by executed tests: `UnicodeTest/UnicodeCoverageTest` (surrogates,
validation), `phonetic`/`layout input session` suites (Bengali composition), and the
fuzz corpus (invalid UTF-8, long repeats). The `ui_strings` JSON parser handles
`\uXXXX` surrogate pairs — executed tests.

## 7. Toasts

9 event producers wired (`show_toast` call sites: settings saved, layout switched,
font changed, encoding unicode/bijoy, profile enabled/disabled, learnt cleared, and —
the BUG-03 fix — interface language changed, shown instead of the generic saved toast
when `ui.language` actually changed), one
visible at a time with timer restart, 2.6 s auto-dismiss, click dismiss, never steals
focus (`WS_EX_NOACTIVATE`), status dot + text (never color alone). Runtime popup
verification: BLOCKED (harness asserts the toast element via UIA after a live toggle).

Error/confirmation dialogs (BUG-04 fix): every `show_error` message, the
Clear-Learnt confirmation text/title, the `Error: {code}` line and the dialog title are
routed through `ui::text` with compiled English fallbacks — 22 `error.*` messages +
the `error.code_line` template, 2 `confirm.*` keys and `common.app_name` exist in both
locales (executed cross-check:
160 = 160 keys, 0 code-referenced gaps). Dialogs raised by `initialize()` itself keep
the compiled English strings by documented load-order contract (the language setting is
read by the very engine `initialize()` creates). Live rendering: BLOCKED (Windows).

## 8. Start-mode & second-instance behavior

`apply_start_mode()` honors topbar / system-tray / main-window / last-state; the
sign-in launch (`--startup`) suppresses the intrusive settings window; second instance
posts `kSecondInstanceShowTopbar` (20×50 ms find window loop). Traced; runtime:
BLOCKED.

## Status totals for this report

All 84 interactive rows: **BLOCKED (runtime) with traced contracts**; 7 logic-layer
rows **PASS** with executed-test evidence; 13 static-text rows **NOT APPLICABLE**
(nothing to activate; announced via UIA text elements). Zero rows were converted from
BLOCKED to PASS.
