# SubornoIME — Phase 01 Deep UI/UX Audit (modernization baseline)

**Date:** 2026-11-22 · **Baseline:** commit `96c5f8b` (working tree clean) ·
**Method:** every UI-relevant source file read in full (no folder-name inference),
contrast ratios recomputed from the token literals, keyboard/message-map traces
walked by hand. This audit drives the modernization pass that follows it.

## 1. UI inventory (what actually exists — nothing else is claimed)

| Surface | Implementation | Entry points |
|---|---|---|
| Settings window (4 pages: General / Interface / Topbar / Input) | custom-drawn `Application` window, staged settings + Save/Cancel | tray menu "Settings…", topbar wrench menu, start-mode `main_window` |
| About dialog | frameless owner-drawn popup (`about_window_proc`, anonymous namespace) | topbar wrench menu "About Suborno…" |
| Floating topbar pill | `topbar.cpp` (pimpl + `Host` callback interface), 4 click zones + drag + right-click | tray left-click, second-instance message, start modes |
| On-screen keyboard | `osk_window.cpp` (pimpl + `Host`), injects real VKs via `SendInput` | tray/topbar menus "On-Screen Keyboard…" |
| Popup menu component | `menu_popup.cpp` — owner-drawn, keyboard (↑↓/Enter/Esc), radio/check items | dropdowns, topbar menus, font pickers |
| Native tray menu | `Application::show_tray_menu_at` (Win32 `HMENU`) | tray right-click, topbar right-click |
| TSF candidate window | `candidate_list_ui_element.cpp` (`ITfCandidateListUIElement(Behavior)`) | composition while typing |
| Error/confirmation dialogs | native `MessageBoxW` (`show_error`, Clear-Learnt confirm) | failure paths |
| System tray icon | `Shell_NotifyIconW` v4, tooltip status, `TaskbarCreated` re-add | shell |

**Not present** (honestly): a dedicated dictionary-management window, a dedicated
shortcut-management window, notification toasts (status goes through the tray
tooltip + settings status line), an update-checker UI (opens the releases page).
The i18n string tables `data/i18n/{bn,en}/ui.json` exist but **no code loads
them** (grep over `src/` finds no reader) — the UI is English-only by code, with
Bangla appearing in layout names and OSK key labels.

## 2. Findings (severity-ordered, with evidence)

### Critical / High

- **[High · functional] Slider mouse-drag maps the whole track wrong.**
  `application.cpp:1908` — `const int clamped = std::max(track_left, std::min(track_left + x - 0, track_right));`
  `x` is a **client** coordinate, but `track_left` (already client-space,
  `scaled(kContentLeft + 164)` = 396 px at 96 DPI) is added again, so the value
  becomes `min + x·range/(track_right−track_left)`. Every press anywhere on the
  visible track (x ≥ 396) saturates the value at **max**; to reach 50% the user
  would have to click at window x = 140, which is outside the control. Both
  sliders ("Idle Opacity", "Idle Time") are unusable by mouse; keyboard
  Home/End/arrows still work. Fix: clamp `x` itself
  (`std::min(x, track_right)`), making `value = min + (x−track_left)·range/span`.
- **[High · a11y/claim-integrity] Dark-theme secondary text fails the AA floor
  the design system documents.** `ui_theme.hpp:72` `text_inactive = #8C8C8C` on
  surface `#2C2C2C` = **4.15:1** (recomputed; the header and the previous report
  claim ≥ 4.5:1). All 13 other token pairs recompute PASS. Fix: `#969696`
  (4.72:1) — the closest harmonious step above the floor.
- **[High · a11y] About dialog has no keyboard access beyond Esc.**
  `application.cpp:820-824` — `WM_KEYDOWN` handles only `VK_ESCAPE`. The five
  actions (close, Check for Updates, Documentation, License, Notices) are
  mouse-only; no tab order, no focus ring. The settings window already models
  focus correctly (lines 2088-2126) — the About dialog needs the same treatment.

### Medium

- **[Medium · a11y] Frameless custom windows have no accessible names.**
  The settings window (`application.cpp:996-1007`), About dialog
  (`application.cpp:873-884`), topbar (`topbar.cpp:578-589`) and OSK
  (`osk_window.cpp:572-583`) are created with a `nullptr` window name. The
  candidate window already carries `"Suborno candidates"` (line 482). Without a
  name, the default HWND accessibility fallback announces nothing. A full UIA
  provider remains out of scope (documented), but window names are a free,
  zero-layout-risk improvement.
- **[Medium · consistency] Control heights disagree inside one page.**
  `ui_theme.hpp:257-259`: `kButtonHeight = 28` vs `kDropdownHeight = 34`, and
  the footer Save/Cancel are 34 tall (`application.cpp:1327-1328`). The "Clear
  Learnt Words…" button renders 6 px shorter than the dropdown above it on the
  same page — the "control heights move together" rule is violated.
- **[Medium · typography] OSK ignores the configured Bangla UI font.**
  `osk_window.cpp:160-172` `ensure_fonts()` builds from raw
  `NONCLIENTMETRICSW` and never applies `ui::ui_font_family()` (which
  `ui::create_message_font()` honors). Bangla key glyphs depend on GDI font
  linking instead of the chosen family, and a font change while the OSK is open
  never reaches it (`refresh_labels()` only re-syncs the mode).
- **[Medium · visual] OSK rounded region goes stale after a DPI change.**
  `osk_window.cpp:493-508` handles `WM_DPICHANGED` by resizing but never
  re-applies the rounded region — `topbar.cpp:508-531` shows the correct
  pattern. On a DPI transition the OSK corners clip.
- **[Medium · UX] Topbar zones give no hover feedback.** `topbar.cpp:132-222`
  paints mode icon, label, wrench and close identically regardless of pointer
  position; the two right-edge zones are invisible until clicked. The surface
  already tracks the mouse for dragging; hover pills are a paint-only change.
  Also `update_idle_state` (lines 331-350) calls `SetLayeredWindowAttributes`
  every 500 ms even when the alpha did not change.
- **[Medium · visual] Candidate window border ignores the theme.**
  `candidate_list_ui_element.cpp:480-483` uses `WS_POPUP | WS_BORDER` — the
  system-drawn frame does not follow the dark/light/HC palettes; every other
  edge in the shell uses `theme_separator()`.

### Low

- **[Low · perf] `fill_rounded` allocates a region + brush per call**
  (`ui_theme.hpp:369-383`, `CreateRoundRectRgn`+`FillRgn`). Handles are always
  deleted (no leak) but regions are the most expensive GDI object here and
  10–30 fill_rounded calls run per paint across surfaces. `RoundRect` with a
  `NULL_PEN` produces the same geometry without the region.
- **[Low · UX] Popup menu clips long Bangla font names.**
  `menu_popup.cpp:21` fixes the popup at 200 design px; item text
  (`menu_popup.cpp:129-133`) has no `DT_END_ELLIPSIS`, so long families
  ("Noto Sans Bengali", "Siyam Rupali") can run against the edge unmarked.
- **[Low · code] `kMenuOpen` (application.cpp:53) is handled but no menu item
  carries it** — dead-but-harmless path kept for symmetry with the tray id
  block.
- **[Low · perf] Per-paint `CreateSolidBrush`/`CreatePen` churn** in
  `paint_settings_window` (application.cpp:2450-2454, 2475, 2640-2650),
  topbar paint (topbar.cpp:135-158, 183-221), OSK paint
  (osk_window.cpp:364-368, 393-402). All balanced (no leaks); the shared
  helper change above removes most of it.

### Verified as already sound (no action)

- Settings-window keyboard model: Tab/Shift+Tab cycle, Ctrl+Tab pages,
  Space/Enter/Esc, slider arrow/Home/End/PgUp/PgDn, focus rings, press-and-release
  footer buttons (application.cpp:2069-2183, 2545-2549).
- Theme machinery: dark/light/system resolution, HC override via live system
  colors, paired `on_accent` fills, `TaskbarCreated` re-add, DPI-aware process
  setup with triple fallback (application.cpp:516-545, ui_theme.hpp:118-217).
- Candidate window COM hygiene: exception barriers at every COM/window
  boundary, ref-paired `WM_NCCREATE`/`WM_NCDESTROY`, TF_CLUIE flag bookkeeping,
  page math, slide animation with timer-failure fallback.
- Menu popup keyboard routing (↑↓/Enter/Esc + off-window keystroke capture,
  menu_popup.cpp:206-295), WM_MOUSELEAVE highlight clear.
- Topbar WM_DPICHANGED handling and drag clamp-and-persist
  (topbar.cpp:463-531); position restore clamped to monitor work areas.
- Settings persistence untouched (schema v2, atomic write + quarantine path);
  tests untouched — 223/223 green on the portable suite at this baseline.

## 3. Implementation strategy for the modernization pass

1. Token corrections first (`ui_theme.hpp`): dark `text_inactive` → `#969696`;
   `kButtonHeight` 28 → 34 (heights move together); rework `fill_rounded` to
   region-free `RoundRect` (same geometry, less GDI churn).
2. Interaction fixes: slider arithmetic (`application.cpp:1908`), About-dialog
   keyboard model with focus rings, topbar hover + alpha caching, OSK DPI-region
   re-apply + font-family honoring + `refresh_font()`, menu ellipsis + width,
   candidate theme border.
3. Accessible names on the four frameless windows (paint-free, layout-free).
4. Verification: portable test suite (this sandbox, `-j1`), layering check,
   brace-balance scan of the Windows-only edits, recomputed contrast table,
   honest MSVC/GUI blockers. Windows-only files are verified by full-source
   reading and scanning because no MSVC toolchain exists here (constraint: the
   MSVC pipeline must not be substituted).
5. Explicitly rejected: extracting the About dialog into its own TU (single
   call site, no reuse, unverifiable compile step locally — abstraction risk
   without need); introducing an i18n loader (new runtime behavior, out of
   modernization scope); touching TSF/engine/persistence/hotkeys/tests.
