# Suborno Reference Design Spec — Reference Dark UI

Date: 2026-11-21
Basis: measured values from the 24 reference screenshots (see
`docs/research/UI_UX_REFERENCE_ANALYSIS.md` §3). Items marked
**[M]** are measured; **[I]** are the smallest reasonable visual
inference where the screenshots do not allow exact extraction.

## 1. Color palette

| Token | Value | Usage | Source |
|---|---|---|---|
| `Surface` | `#2C2C2C` | window/page/dialog/menu background | [M] |
| `SurfaceTopbar` | `#2B2B2B` | topbar pill background | [M] |
| `SurfaceRaised` | `#383838` | selected sidebar pill, active tab, button face, dropdown field at rest | [M] |
| `SurfaceKey` | `#4F4F4F` | OSK keys (documented for parity; OSK not implemented) | [M] |
| `BorderKey` | `#3B3B3B` | OSK key border | [M] |
| `Track` | `#272727` | slider track | [M] |
| `Text` | `#FFFFFF` | titles, labels, checkbox labels, buttons | [M] |
| `TextInactive` | `#8C8C8C` | inactive tab labels, secondary hints | [M] |
| `Accent` | `#5886FE` | topbar mode icon, focus outline | [M] |
| `ProgressGreen` | `#6FC67E` | installer progress bar (Inno) | [M] |
| `Separator` | `#3F3F3F` | menu separators | [I] |
| `HoverPill` | `#333333` | hover state for sidebar items/buttons | [I] |

## 2. Typography

- Family: **Segoe UI** (Windows shell font; [I] identification from
  rendering). Bengali strings render via font fallback (Nirmala UI).
- Title (dialog caption, e.g. "Suborno Settings"): Segoe UI Semibold
  16 px, white, top-left. [I]
- Body/labels/checkbox text: Segoe UI 14 px, white. [I]
- Sidebar items: Segoe UI 14 px white; selected item same size on
  `SurfaceRaised` pill. [M for pill, I for size]
- Menu items: Segoe UI 14 px, white, left inset ~12 px, height ~28 px. [I]
- Buttons: Segoe UI 14 px white. [I]

## 3. Spacing and geometry

- Settings window: **736×620 design px** [M from S8/S9/S11/S12], rounded
  corners ~8 px [I], borderless custom frame (custom title row ~36 px).
- Sidebar width **~185 px** [M]; sidebar items height **~36 px** [I],
  left inset 12 px, item spacing 6 px [I], selected = full-width rounded
  pill `SurfaceRaised`.
- Content panel: inset ~24 px from sidebar and window edge, rounded
  outline border `#3F3F3F` [I], background = `Surface` (panel is not a
  different color) [M].
- Section labels ("Behavior", "Candidate Window", "Candidate Selection"):
  Segoe UI 14 px white, extra vertical breathing room (~28 px above,
  ~16 px below) [I].
- Checkbox rows: 24 px square hit area; box 18×18 px, 2 px radius [I],
  1.5 px white border, white checkmark when on; label 12 px right of box;
  rows may share one line with ~24 px gap (two-column like S8) [I].
- Dropdown: field height **34 px** [I], width up to ~300 px, face
  `SurfaceRaised`, radius 6 px [I], chevron right-inset 10 px; popup =
  `Surface` menu with 4 px radius, item hover `SurfaceRaised`, selected
  row shows white checkmark in left gutter (S10/S13/S15) [M concept].
- Slider: track 4 px `Track`, handle Ø16 px white circle (S11) [M],
  value label right of track.
- Buttons (Save/Cancel/About/EULA): height **34 px**, padding ~24 px
  horizontal, radius 6 px [I], face `SurfaceRaised`, white text; no
  gradient/strong border [M].
- Topbar pill: **~170×30 px**, radius ~10 px [I], contents: accent
  circular mode icon (Ø16 px, `Accent`) → mode label (white 14 px) →
  wrench glyph → ✕ glyph, 12 px internal padding [M/I].
- Menu geometry: width ~185–235 px [M from S6/S7], item height 28 px [I],
  radius 4 px [I], separators between logical groups (S7: after
  "Learning…", S24: two separators) [M concept].

## 4. Navigation structure (settings)

Sidebar pages, order fixed by reference: **General → Interface → Topbar →
Input** [M]. Suborno pages contain only real settings (§5). Page switch
replaces the content panel; selection pill moves.

## 5. Page contents (real Suborno settings mapped to reference layout)

- **General** — "Behavior": [x] Start with Windows (registry-backed,
  existing). "Default keyboard layout": dropdown of the six real layouts
  (deviation note: reference General has Start-as/Switching-mode controls
  which have no real counterpart in Suborno; layout selection is kept in
  the window so the existing workflow is preserved).
- **Interface** — "Candidate Window": [x] Show Candidate Window.
  (Reference's Orientation/Fixed-Height/Sliding/Prediction controls have
  no real counterpart — omitted, documented.)
- **Topbar** — "Topbar Visibility": [x] Show floating topbar;
  "Idle Opacity (%)" slider; "Idle Time (Seconds)" slider. All wired to
  the real topbar feature.
- **Input** — "Input preferences": [x] Map Numpad to Bangla Numbers,
  [x] Ignore Caps Lock state in Bangla layouts. "Candidate Selection":
  [x] Select candidate using number keys. (Reference's Layout Switching
  Shortcut + Disable Encoding Shortcut + locale-dummy controls have no
  real counterpart — omitted, documented.)

## 6. Interaction specification

- **Save / Cancel**: Save commits the staged settings (writes
  settings.ini via the engine, refreshes the Windows profile, updates
  tray/topbar); Cancel discards staged changes and closes. Window ✕ =
  Cancel. Esc = Cancel, Enter = Save. [Reference M: Save/Cancel exist]
- **Checkbox / dropdown / slider** keyboard support: Tab cycles all
  controls in visual order; Space toggles checkbox; dropdown opens with
  Space/Enter, Up/Down navigates, Enter selects, Esc closes; slider
  accepts Left/Right (1 unit) and PgUp/PgDn (10). [I — keyboard parity
  with the pre-existing accessible window; not captured in screenshots]
- **Topbar**: left-click on mode area → layout menu (radio on active);
  wrench → menu: Settings…, About Suborno…; ✕ → menu: Switch to Tray,
  Exit Suborno. Drag anywhere on pill moves it (within work area).
  Idle dimming: after `Idle Time` seconds without any typing input the
  pill fades to `Idle Opacity`; any mouse hover or typing restores 100%.
  [Reference M for menu contents mapped to real Suborno actions; idle
  behavior inferred from S11 control labels]
- **Tray**: left click opens the settings window; right-click menu in
  reference order: Show Topbar, Keyboard Layout ›, — Settings…, — Exit.
  [M order from S24 mapped to real actions]
- **About dialog**: custom dark dialog, logo, name, "A FREE Bangla IME",
  version, FREE, buttons "License (MIT)" and "Open Source Notices"
  (open the real LICENSE / THIRD_PARTY_NOTICES.md files). Reference's
  "Check for Updates" omitted (no server). [deviation documented]
- **Installer**: unchanged Inno Setup wizard (light theme per S1–S4);
  already has LicenseFile page and post-install "Launch Suborno"
  checkbox — matches the reference pattern.

## 7. Out of scope (no real functionality — see research doc §4)

Theme engine (App Theme/Quick Theme), On-Screen Keyboard, Suggestion Bar
& Next Word Prediction, Learning, SyncType, License Manager, Encoding
menu, Keyboard Layout Editor, Layout Switching Shortcut, Check for
Updates. None may appear as fake controls.
