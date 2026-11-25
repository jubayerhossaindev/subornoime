# Report 05 — Accessibility Report

## 1. Keyboard access (traced per surface; runtime BLOCKED)

| Surface | Keyboard model | Status |
|---|---|---|
| Settings | Tab/Shift+Tab cycles controls skipping static text, then Save, then Cancel (wrapping); Ctrl+Tab/Ctrl+Shift+Tab pages; Space toggles checkboxes / opens dropdowns / presses buttons; sliders ←/→/PgUp/PgDn/Home/End; Enter activates focused footer (else Save); Esc cancels; visible focus ring on every control | Traced; runtime BLOCKED (harness + protocol cover it) |
| About | 5 hotspots in tab order, Shift+Tab reverse, Space/Enter activate, Esc closes, focus ring | Traced; BLOCKED |
| Menu popup | ↑/↓ (skips separators/disabled, wraps), Enter selects, Esc dismisses, kill-focus dismisses; off-window keystroke capture | Traced; BLOCKED |
| Topbar / OSK | Mouse/UIA-Invoke driven **by documented design** (`focused_element() = -1`); Esc hides the OSK | Documented design; BLOCKED |
| Tray menu | Native HMENU keyboard (arrows/Enter/Esc) via `SetForegroundWindow` + `WM_NULL` quirk workaround | Traced; BLOCKED |
| Toast | Never takes focus — deliberate (`WS_EX_NOACTIVATE`) so typing/TSF composition is never interrupted; UIA Invoke dismisses without focus | Design contract; NOT APPLICABLE for Tab order |

## 2. UIA names / roles / states / patterns

Per-element names, roles, bounding rects, focus state, Toggle/RangeValue/
ExpandCollapse/Invoke patterns are produced from each window's own model (Report 04 §1).
Dropdowns are named by their section heading (`dropdown_name()` — dropdowns have no
separate label row); OSK special keys are named with words ("Shift/Space/Backspace/
Enter") instead of ⇧/⌫/↵ glyphs which announce poorly; radio and check menu items map
to UIA Toggle state so screen readers announce "which one is active"; the toast exposes
its message as HelpText+Name on a Button (Invoke = dismiss). Narrator runtime sessions:
**BLOCKED** (manual protocol §1 provides the exact Narrator pass).

## 3. Contrast (recomputed from token literals — executed this audit)

`scripts/contrast_check.py` recomputed WCAG ratios from `ui_theme.hpp` at HEAD:

| Pair | Dark | Light | Floor | Verdict |
|---|---|---|---|---|
| text on surface | 13.97:1 | 15.12:1 | 4.5 | PASS both |
| text on surface_raised | 11.73:1 | 13.58:1 | 4.5 | PASS both |
| text_inactive on surface | 4.72:1 | 5.35:1 | 4.5 | PASS both |
| text_inactive on surface_raised | 3.96:1 | 4.80:1 | 4.5 | PASS light; dark **NOT APPLICABLE by documented pairing rule** (source lines 77–83: token valid on `surface`/hover only; every consumer verified to draw on `surface`) |
| accent on surface | 4.16:1 | 4.25:1 | 3.0 | PASS both |
| on_accent on accent_fill | 4.61:1 | 4.63:1 | 4.5 | PASS both |
| control_border on surface | 4.15:1 | 3.52:1 | 3.0 | PASS both |

High Contrast derives from live system colors (`system_color_palette()`), where the OS
scheme itself guarantees the floors. Focus rings use the accent token (≥4.16:1) at 2 px
— visible on both palettes; ring rendering traced, runtime visibility BLOCKED.

## 4. Localization (executed verification)

- Locale loader + fallback contract: **PASS** — executed by the `UiStringsTest` suite
  inside the 542-test run (parser, `\uXXXX` incl. surrogate pairs, malformed JSON,
  missing file, single missing key → compiled English fallback).
- Locale parity: **PASS** — en/bn both 160 keys (135 at the tooltip-fix baseline, then
  +26 error/confirm/app-name keys and −1 dead key from the BUG-03/04 fix pass),
  identical sets (executed test + `scripts/i18n_key_check.py`).
- Key coverage of code: **PASS after fix** — the four `topbar.tooltip.*` keys referenced
  by `topbar.cpp` were missing from both locales (tooltips could never localize);
  fixed in commit `993968c`; re-run shows 0 missing.
- Former dead keys (BUG-03, now **fixed**): `toast.language_changed` is wired — a
  successful interface-language switch shows it instead of the generic saved toast —
  and `topbar.state.idle_dimmed` (which had no natural consumer: idle dimming is a
  pure alpha effect with no text label) was removed from both locales in the same
  change. Key cross-check executed after the fix: 0 missing, 0 dead.
- Error/confirmation dialogs (BUG-04, now **fixed at the data+code level**): all ≈20
  MessageBox strings (every `show_error` message, the Clear-Learnt confirmation text
  and title, the `Error: {code}` line, the dialog title) route through `ui::text`;
  26 new keys (`error.*`, `confirm.clear_learnt.*`, `common.app_name`) ship in both
  locales (en/bn 160 = 160, cross-check 0 missing). Documented load-order contract:
  dialogs raised by `initialize()` before the locale table loads keep the compiled
  English fallbacks. Live rendering of the localized dialogs and the MSVC compile of
  the modified Windows-only TU: BLOCKED (Windows checklist).

## 5. Status

Keyboard/UIA implementation: traced and consistent, runtime confirmation BLOCKED.
Contrast and localization logic: PASS with executed evidence. The two accessibility-
adjacent items (dead keys Low; English error dialogs Medium) were fixed in the
BUG-03/04 pass recorded in Report 08; their live rendering is on the Windows checklist.
