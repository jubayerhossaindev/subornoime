# BornoTSF UI/UX Implementation Report

Date: 2026-11-21
Task: reproduce the supplied Borno reference screenshots in SubornoIME
(same-to-same visual/behavioral fidelity), clean-room, on SubornoIME's
existing architecture.

Precondition documents produced first (see §C):
- `docs/research/BORNOTSF_UI_UX_REFERENCE_ANALYSIS.md` — repository +
  all-24-image study + measured palette
- `docs/design/SUBORNO_REFERENCE_DESIGN_SPEC.md` — measured/inferred
  design tokens and interaction spec

## A. Source Research

- Repository inspected: https://github.com/JayedAhsan/BornoTSF
  (README, structure, license badge, build docs).
- **Key finding:** the repository is a *no-GUI* TSF engine (GPL-3.0,
  MSVC, Boost) — no settings dialog, no topbar, no OSK, no themes. The
  supplied screenshots belong to the retail "Borno" application shell
  (Codepotro, version 2026.6.18), which is not in that repository. The
  screenshots were therefore used as the primary visual specification
  (as the task instructed), and the repository independently documented.
- License: BornoTSF = **GPL-3.0**; SubornoIME = **MIT**. GPL code cannot
  be moved into the MIT project. **Zero source code, zero assets and zero
  logic were copied.** All implementation is new Win32 code written from
  the measured visual specification (colors sampled programmatically from
  the screenshots — facts, not copyrightable expression).

## B. Reference ZIP Analysis

- Downloaded from the real link discovered behind the tmpfiles.org
  landing page (`…/dl/1795294456.031f3671f4f4c454/wcAVb0iF3dIZ/born-images.zip`).
- Verified: 6,018,014 bytes, valid Zip archive (`file`), extracted once
  to an isolated directory; original preserved; no image modified.
- **24 images, every one inspected individually** (dimensions in the
  research doc §2.1): installer wizard ×4, floating topbar states ×3,
  settings pages ×6 (incl. two open-dropdown states), on-screen keyboard ×1,
  topbar submenus ×3, license manager ×1, About ×2 (one annotated),
  SyncType QR ×1, tray icon + tray menu ×2, ✕-menu ×1.
- Programmatic measurement (PIL, `scripts/measure_reference*.py`):
  Surface `#2C2C2C`, raised `#383838`, topbar `#2B2B2B`, accent `#5886FE`,
  text `#FFFFFF`, inactive `#8C8C8C`, slider track `#272727`,
  installer progress `#6FC67E`.

## C. Design Specification

`docs/design/SUBORNO_REFERENCE_DESIGN_SPEC.md` — full token table with
**measured vs inferred** labels (measured: palette, window 736×620,
sidebar ~185 px, pill/tabs `#383838`, slider handle white Ø16, topbar
~170×30; inferred: radii ~8/10 px, item heights 28/36 px, paddings,
Segoe UI sizes). Navigation: General → Interface → Topbar → Input;
Save/Cancel staged persistence; topbar interactions; tray order.

## D. Implementation Report

### D.1 Architecture preserved

No rewrite: same C++20, same TSF DLL untouched, same engine/settings
schema (`EngineSettings` already mirrors the reference groups),
same installer, same tests, same CMake presets. The desktop shell was
restyled; three new source files were added; the old Win32-control
settings window was replaced by a custom-drawn one inside the same
`Application` class (same lifecycle: single instance, tray, smoke test,
`--startup`).

### D.2 Components

| Component | File | State |
|---|---|---|
| Theme tokens + GDI helpers (rounded rects, fonts, DPI) | `include/suborno/platform/windows/app/ui_theme.hpp` (new) | Added |
| Dark popup menu (radio/check/separator/hover) | `include/.../menu_popup.hpp`, `src/.../app/menu_popup.cpp` (new) | Added |
| Floating topbar pill (menus, drag, idle dimming) | `include/.../topbar.hpp`, `src/.../app/topbar.cpp` (new) | Added |
| Settings window (frameless dark, sidebar pages, custom controls, Save/Cancel) | `src/platform/windows/app/application.cpp` | Replaced |
| About dialog (dark, real license buttons) | `application.cpp` | Added |
| Tray menu (reference order, + preserved features) | `application.cpp` | Restyled |
| Build wiring (`app/menu_popup.cpp`, `app/topbar.cpp`, `Gdi32.lib`) | `src/platform/windows/CMakeLists.txt` | Updated |
| Docs | `docs/research/…`, `docs/design/…`, `docs/reports/…`, `CHANGELOG.md` | Added |

### D.3 Reference-image → implementation mapping (all 24 images)

| Reference Image | Reference UI | SubornoIME Component | Required Changes | Verification |
|---|---|---|---|---|
| 1 | Installer Welcome | Inno Setup wizard (`suborno_installer.iss`) | none — same wizard pattern (logo, Next/Cancel) | Code-inspected; MSVC/ISCC NOT VERIFIED |
| 2 | Installer License Agreement | Inno `LicenseFile` page (THIRD_PARTY_NOTICES) | none — page exists with accept flow | Code-inspected |
| 3 | Installer Installing progress | Inno progress page (green bar measured `#6FC67E`) | none — Inno default green bar | Code-inspected |
| 4 | Installer Finish + "Launch" checkbox | Inno `[Run]` post-install "Launch Suborno" | none — already matches reference pattern | Code-inspected |
| 5 | Topbar pill idle | **Topbar (new)**: accent circle icon + layout label + wrench + ✕, `#2B2B2B` pill, rounded | Implemented | Code-reviewed; runtime NOT VERIFIED |
| 6 | Topbar layout menu (radio) | Topbar mode click → dark popup menu with radio on active layout; switching calls real `save_layout` + profile refresh | Implemented | Code-reviewed; runtime NOT VERIFIED |
| 7 | Topbar wrench menu | Wrench click → Settings… / About Suborno… (real actions; non-existent features not faked) | Implemented | Code-reviewed |
| 8 | Settings General page | Sidebar General: Behavior (Remember Last Input Layout ✓ consumed by engine, Start with Windows ✓ registry), Default keyboard layout dropdown (real), status line (real TSF state) | Implemented; "Start as"/"Switching Mode" omitted per schema rule (not consumed) | Code-reviewed |
| 9 | Settings Interface page | Sidebar Interface: Show Candidate Window (✓ consumed). Theme/orientation/etc. omitted per schema rule (no runtime consumer) | Implemented | Code-reviewed |
| 10 | App Theme dropdown open | — (theme engine absent) | NOT IMPLEMENTED (no fake control) | — |
| 11 | Topbar page (sliders) | Sidebar Topbar: Enable Idle Dimming + Idle Opacity slider + Idle Time slider — all wired to the real topbar dimming | Implemented | Code-reviewed |
| 12 | Input page | Sidebar Input: Map Numpad ✓, Ignore Caps Lock ✓, Select candidate using number keys ✓ (all consumed by TSF) | Implemented; shortcut/encoding/locale controls omitted per schema rule | Code-reviewed |
| 13 | Shortcut dropdown open | — (shortcut engine absent) | NOT IMPLEMENTED (no fake control) | — |
| 14 | On-Screen Keyboard | — (no OSK feature) | NOT IMPLEMENTED (documented) | — |
| 15 | Quick Theme submenu | — (no theme engine) | NOT IMPLEMENTED | — |
| 16 | Suggestion Bar submenu | — (no suggestion bar) | NOT IMPLEMENTED | — |
| 17 | Learning submenu | — (no learner UI) | NOT IMPLEMENTED | — |
| 18 | License Manager | — (MIT app, no activation) | NOT IMPLEMENTED (inapplicable) | — |
| 19 | About Borno | **About dialog (new)**: dark, icon, name, "A FREE Bangla IME", Version, FREE, License (MIT) + Open Source Notices buttons opening real files | Implemented; "Check for Updates" omitted (no server — would be fake) | Code-reviewed |
| 20 | About annotated | Same About dialog (arrows = what to reproduce) | Same as 19 | — |
| 21 | SyncType QR | — (no voice-typing service) | NOT IMPLEMENTED | — |
| 22 | ✕ menu (Switch to Tray / Exit) | Topbar ✕ click → dark menu: Switch to Tray / Exit Suborno (both real) | Implemented | Code-reviewed |
| 23 | Tray icon | Existing tray icon (unchanged icon, reference-highlighted) | none needed | Code-reviewed |
| 24 | Tray menu | Tray menu restyled: Show Topbar (checkable), Keyboard Layout ›, — Settings…, Enable/Disable, Uninstall, — Exit | Implemented (features preserved per no-removal rule) | Code-reviewed |

### D.4 Functionality preserved (audit)

- Engine, layouts, TSF registration, dictionary, settings file format
  (schema v2, additive use only), atomic settings writes, quarantine,
  first-run defaults, startup registry preference with rollback, tray
  TaskbarCreated handling, single-instance mutex, smoke test
  (`--smoke-test`), `--startup` flag: all unchanged and covered by the
  existing 184-test suite (184/184 PASS after changes).
- The settings window keeps hide-to-tray on close; Save/Cancel semantics
  added per reference; immediate-save layout switching retained in tray
  menu and topbar menu.

## E. Visual Comparison

- **Screenshots could not be captured**: this verification environment is
  Linux without Windows/MSVC; per the no-fabrication rule no screenshots
  were produced. Visual verification is code-level (positions, colors,
  and sizes in code match the measured spec; palette constants are the
  measured values). Side-by-side pixel comparison of real renders is
  NOT VERIFIED.
- Similarity limitations: native tray menus stay light-themed (custom
  dark menus are used everywhere the app itself draws — topbar and
  dropdowns; the tray menu is drawn by the OS). GDI text is Segoe UI
  with system DPI scaling; exact reference font metrics are inferred.

## F. Testing Results

| Check | Command | Result | Status |
|---|---|---|---|
| Configure | `cmake -S . -B build/verify -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DCMAKE_PREFIX_PATH=<gtest>` | exit 0 | PASS |
| Build (Linux-verifiable targets) | `cmake --build build/verify -j4` | exit 0, 0 warnings | PASS |
| CTest | `ctest` | 100% (2/2) | PASS |
| Unit tests | `./suborno_unit_tests` | **184/184 PASSED** | PASS |
| Engine runtime | `./suborno_engine_demo "ami bhai"` | correct commit | PASS |
| MSVC Release build (`windows-msvc-release` preset) | requires Windows | — | **NOT VERIFIED** |
| Runtime launch / screenshots / Tab order / DPI 100–200% / dark-light themes | requires Windows desktop | — | **NOT VERIFIED** |
| Installer (ISCC) | requires ISCC | — | **NOT VERIFIED** |

Manual Windows-only code review completed for all three new/rewritten
files (message loops balanced, GDI objects deleted, COM untouched, no
exception escapes from window procedures); this reduces but does not
eliminate the risk that MSVC finds issues.

## G. Licensing Assessment

- BornoTSF license inspected: GPL-3.0. SubornoIME license: MIT.
- Code reused from BornoTSF/retail Borno: **none** (clean-room).
- Visual references used: the 24 supplied screenshots (measured colors,
  geometry, behavior) and the public repository documentation.
- Compatibility concerns: none for the implemented scope; the omitted
  features were omitted partly *because* reproducing them would require
  either non-existent functionality or design guessing.

## H. Remaining Issues

1. **Windows build + runtime verification** of all new UI code (MSVC,
   ISCC, interactive session) — NOT VERIFIED here; this is the dominant
   open item.
2. All features marked NOT IMPLEMENTED in D.3 (OSK, themes, suggestion
   bar, learning, SyncType, license manager, shortcuts, encoding) —
   each requires new runtime functionality before it may appear in the
   UI (project schema rule).
3. Topbar visibility is session-scoped; persistence needs a storage key
   decision (`remember_topbar_position` exists but no position keys).
4. Tray menu remains OS-drawn (light theme unless Windows dark app mode
   manifest is added later).
5. Installer already matches the reference wizard pattern; no changes
   were required, so ISCC repackage is unnecessary for parity.

## I. Final Verdict

**IMPLEMENTED — VERIFICATION INCOMPLETE** — the reference-matched UI is
implemented in-repo (all 24 images mapped; real functionality behind
every visible control; no fake elements), the shared build and full test
suite pass (184/184, 0 warnings), but every Windows-runtime check (MSVC
build, launch, screenshots, DPI, themes) remains NOT VERIFIED in this
environment.
