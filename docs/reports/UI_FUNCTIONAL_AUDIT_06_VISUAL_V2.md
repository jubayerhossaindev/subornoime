# UI_FUNCTIONAL_AUDIT_06 — Visual & Responsive Regression Report (Phase 2, 2026-11-24)

**Screenshots, DPI switching, monitor moves and live theme switching are all
BLOCKED** — they need the running Windows shell. Executed/static evidence
below.

## 1. Executed: WCAG contrast regression (both palettes)

`scripts/contrast_check.py` parses the literal `kSubornoDark` / `kSubornoLight`
palettes out of `ui_theme.hpp` and recomputes every documented text/boundary
pairing against WCAG AA floors (text ≥ 4.5:1, non-text boundaries ≥ 3:1):

- **22 pairs checked, 0 required-pair failures.**
- Pairings below the floor that have zero current consumers are printed under
  the documented NOT-APPLICABLE rule instead of being hidden.

## 2. Source-audited visual system

- Theme tokens: `ui::theme_surface/text/text_inactive/accent/accent_fill/
  on_accent/control_border/separator/success/warning/information` + spacing
  (4/8/12/16/24) and metrics constants; High Contrast wins over both palettes
  via `high_contrast_active()` → `system_color_palette()`.
- Every surface consumes the same process-global palette slot and repaints on
  `WM_THEMECHANGED`; `theme_id="system"` re-probes
  `HKCU\...\Themes\Personalize\AppsUseLightTheme`.
- DPI: Per-Monitor v2 (`enable_process_dpi_awareness`); all geometry goes
  through `ui::scaled`; the settings window refits into the new monitor's work
  area on `WM_DPICHANGED` (application.cpp:1561-1573); candidate window and
  OSK rescale; topbar clamps back into the work area after monitor moves.
- Focus indicators: `draw_focus_ring` on the settings window and About
  hotspots.

## 3. BLOCKED (needs live Windows)

- Screenshot regression at 100/125/150/175/200% DPI; single/multi-monitor and
  mixed-DPI moves for Settings/About/Topbar/OSK; live High-Contrast on/off
  restoration; popup positioning/clipping on screen edges; rounded corners and
  drop shadows on real compositor; text clipping with extreme strings.
- Rationale for not simulating: pixel-level rendering cannot be honestly
  verified without rendering. These remain explicit gaps (TRC-045) rather than
  optimistic PASS rows.
