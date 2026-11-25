# Report 06 — Visual & Responsive Regression Report

**Runtime screenshots: BLOCKED** — this sandbox has no Windows desktop, and per the
evidence rules none were fabricated. What follows is (a) the DPI/theme machinery
verified in full source read, (b) one executed visual check that needs no desktop
(contrast recomputation), and (c) the delivered capture protocol that turns every
BLOCKED item below into evidence on a Windows machine.

## 1. DPI (Per-Monitor v2)

- Process declares `PER_MONITOR_AWARE_V2` first, then Shcore system-aware, then legacy
  `SetProcessDpiAware` (pre-1703 fallback) — `enable_process_dpi_awareness()`.
- Settings window: 736×620 design px scaled by `ui_dpi_` fitted to the monitor work
  area (`dpi_fitting` cap keeps Save/Cancel reachable on short 125% laptops / 200%
  desktops); `WM_DPICHANGED` refits to the NEW monitor's work area, recreates all four
  cached fonts, re-cuts the rounded region, moves to the system-suggested rect.
- About dialog: same handler pattern (`about_window_proc WM_DPICHANGED`).
- Topbar: DPI-scaled geometry, fonts rebuilt, tooltip rects re-synced
  (`update_tooltip_layout`), rounded region re-applied.
- OSK: fonts + geometry rescale and — the Phase-01 finding — the rounded region is
  **re-applied** (`apply_window_treatment` in its `WM_DPICHANGED`), fixing stale corners.
- Toast: follows `WM_DPICHANGED`, re-measures per message from the current DPI.
- Hit-testing inverse-maps client px → design px (`to_design`), so interactions stay
  correct at any scale.

## 2. Theme system

- Three persisted themes (suborno-dark / suborno-light / system) resolved through
  `set_active_theme()`; `system` tracks the Windows app light/dark setting
  (`windows_app_light_theme()`); High Contrast overrides via `high_contrast_active()`
  + `system_color_palette()` from live syscolors.
- Every live surface handles `WM_THEMECHANGED` (settings, About, topbar, OSK, toast)
  and repaints from the process-global palette — a live HC toggle updates all windows.
- The candidate window renders a theme-aware 1 px frame (system `WS_BORDER` removed —
  earlier-pass fix present at `candidate_list_ui_element.cpp:478-483, 768`).
- Status semantics use color **and** words (success/warning tokens never alone).

## 3. Layout consistency (static-verified)

- Control heights unified: `kButtonHeight = kDropdownHeight = 34` (Phase-01 finding
  fixed), checkbox 24, menu item 28; radii from one token scale (4/6/8/10).
- `fill_rounded` is region-free `RoundRect` (Phase-01 perf fix present).
- Footer Save/Cancel geometry defined once (`save_button_rect`/`cancel_button_rect`)
  shared by paint, hit-test and press logic; About hotspots defined once and shared by
  paint, hit-test, tab order and UIA rects.
- General-page checkbox overlap (Phase-01 fix) still correct: y=118/148 distinct rows.
- Menu popup width 240 with `DT_END_ELLIPSIS` for long Bangla family names.
- Topbar auto-width clamped 150–360 design px so long labels (বাংলা (Bornো)) fit.

## 4. Delivered runtime protocol (converts BLOCKED → evidence)

`scripts/windows-ui-audit/Manual-Test-Protocol.md` §3–5 captures: screenshots of every
surface in dark/light/System/HC; DPI 100/125/150/175/200%; cross-monitor drags of the
settings window, About, topbar and OSK between mixed-DPI monitors; popup positioning
near screen edges; focus-ring visibility. The harness's UIA bounding-rect dump doubles
as an automated geometry regression baseline (JSON).

## 5. Status

| Check | Status |
|---|---|
| DPI machinery correctness | Traced (all five windows + process declaration) — runtime BLOCKED |
| Theme/HC machinery correctness | Traced — live toggles BLOCKED |
| Token contrast floors | PASS (executed recomputation; pairing rule honored) |
| Screenshot/alignment/pixel checks | BLOCKED — protocol delivered |
