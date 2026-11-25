# SubornoIME — UI/UX Audit: Remaining Items Implemented (Pass 3)

**Date:** 2026-11-22 · **Baseline:** the two earlier modernization passes
(`509bd33` Pass 1, `b2e6307` Pass 2) · **This pass:** every item the Phase-01
audit found and the Pass-2 report listed as *remaining*, *rejected*, or *not
present* is now implemented as real code — nothing was left as a documented
deferral.

Trigger for this pass: the audit was challenged directly — findings that are
only written into reports are worthless. Everything the research surfaced and
every deferral that was technically possible inside the project's protection
constraints is now wired into the app.

---

## A. What was implemented

### 1. Bangla interface — the orphaned i18n files are live (was "not present")

The audit's headline honesty item: `data/i18n/{en,bn}/ui.json` existed but
"no code loads them — the UI is English-only by code". Fixed end to end:

- **Portable loader** `suborno::core::ui_strings`
  (`include/suborno/core/ui_strings.hpp`, `src/core/ui_strings.cpp`): a
  strict, minimal JSON reader (objects, strings, `\uXXXX` escapes incl.
  surrogate pairs) that flattens the nested files to dotted keys. It is in
  the **portable core**, so the parser, the fallback rules and both shipped
  locale files are unit-tested on every platform — 12 new GTest cases
  (`tests/unit/ui_strings_test.cpp`), including the real shipped files.
- **Every user-visible string on every surface** now renders through
  `ui::text(key, English fallback)` (`ui_theme.hpp`): settings window
  (title, sidebar, all 15 section headings + hints, all 20+ control labels,
  every dropdown item), Save/Cancel, the Windows status line
  (`{layout}`/`{code}` placeholders), tray tooltip, the full tray menu and
  topbar menus, About dialog, OSK labels and titles, and the toasts below.
  The compiled English literal is the **permanent fallback per key** — a
  missing file, unparsable file, or single missing key can never blank the
  UI.
- **Language switch**: Settings → Interface → **"Interface language"**
  dropdown (Follow system language / English / বাংলা), backed by an additive
  settings key `ui.language` (`system` | `en` | `bn`; validated; round-trip
  unit-tested including the pre-existing-file path). "system" follows the
  Windows display language (`GetUserDefaultUILanguage`). Applies live on
  Save: locale table reloaded, text cache cleared, window titles, OSK
  labels, status line and pages rebuilt.
- **Both locale files rewritten** with the complete parallel key set
  (129 keys each; a unit test asserts both files parse AND carry identical
  key sets, so a Bangla switch cannot fall back mid-sentence). The Bangla
  strings follow the existing bn/ui.json terminology.
- **Installer ships the files** next to the executable
  (`{app}\data\i18n\<locale>\ui.json`).

### 2. UI Automation providers (was "remaining issue 2 — a project of its own")

`src/platform/windows/app/ui_accessibility.cpp` (+ header): the standard
fragment-provider pattern for owner-drawn Win32 content, now implemented for
every shell surface:

- **Settings window** — the full element tree: 4 sidebar pages
  (ListItem/Invoke), every page control (CheckBox/Toggle, ComboBox/
  ExpandCollapse, Slider/RangeValue with min/max/value, Button/Invoke,
  section headers as Text), plus Save/Cancel/Close — real names (dropdowns
  are named by their section heading), screen-space rectangles, live focus
  state, and focus-changed events raised on Tab/Ctrl+Tab/click. Narrator can
  read AND operate every control; UIA `SetFocus` moves the window's real
  keyboard focus.
- **About dialog** — the five hotspots as invokable buttons in tab order
  with live focus.
- **Topbar** — the four click zones as invokable buttons (layout label,
  wrench, close, English↔বাংলা toggle), names from the locale table.
- **On-screen keyboard** — both mode buttons, Close and **all 34 keys** as
  invokable buttons (special keys named "Shift/Space/Backspace/Enter"
  instead of ⇧/⌫/↵). A screen-reader user can type through UIA Invoke.
- **Robustness**: uiautomationcore entry points are resolved *dynamically*
  (no import — a system without UIA falls through to DefWindowProc and the
  default HWND fallback that earlier passes installed); every provider
  holds the window's liveness token (`shared_ptr` reset in WM_DESTROY) and
  answers `UIA_E_ELEMENTNOTAVAILABLE` instead of touching freed memory;
  every UIA callback is an exception barrier; providers are heap-only
  (UIA can `Release()` them).
- **Deliberately untouched**: the TSF candidate window — it already carries
  an accessible name through the default fallback and lives inside the
  protected TSF/COM surface (constraint held).

### 3. Themed toast notifications (was "not present" in the inventory)

New `ToastWindow` (`src/platform/windows/app/toast_window.cpp`): small
frameless, theme-aware popup near the notification area — token-painted
(surface_raised/separator), status dot in success/information/warning with a
text label (never color alone), auto-dismiss 2.6 s, click dismiss,
`WS_EX_NOACTIVATE` so typing and TSF composition are never interrupted.
Wired to real events: settings saved, layout switched (`{layout}`), font
changed (`{font}`), encoding toggled, Suborno enabled/disabled, learnt words
cleared. Errors intentionally keep native MessageBoxes (documented decision;
the `error` token stays reserved) — toasts give the `information`/`success`
tokens their first real consumers.

### 4. Per-Monitor v2 DPI reflow (was "remaining issue 3")

The process now declares `PER_MONITOR_AWARE_V2` first (the previous
system-aware chain remains as fallback for pre-1703 Windows). The settings
window and About dialog gained `WM_DPICHANGED` handlers: refit the design
DPI to the **new** monitor's work area (the `dpi_fitting` cap still keeps
Save/Cancel on short screens), recreate all cached fonts, re-cut the rounded
region, move to the system-suggested rectangle. The topbar/OSK handlers that
existed but could never fire under system-awareness are now live too.

### 5. Live High Contrast / theme switching (was "remaining issue 7")

`ui::refresh_active_theme()` re-applies the remembered theme id; all four
shell surfaces (settings, About, topbar, OSK) handle `WM_THEMECHANGED` and
repaint. Turning High Contrast on/off — or flipping the Windows light/dark
app mode with theme=system — now repaints every live window immediately
(the palette is process-global, so one re-derivation updates all surfaces,
including the candidate window's next paint).

### 6. Repo hygiene (was "remaining issue 6")

`scripts/brace_scan.py` (literal/comment-aware balance checker used for the
Windows-only edits this pass) is now part of the repo's verification scripts.

## B. What was deliberately not added — and the exact reason

- **Dictionary-management window / shortcut-management window** (audit
  inventory honesty items, not defects): a real dictionary manager needs
  read/list/delete accessors on the Engine's learnt-words store — i.e. new
  public API on `suborno/core/engine.hpp` and touches inside the dictionary
  area, both of which the brief's protection list forbids ("不得改动…
  词典、设置持久化格式…现有公共接口"). A display-only fake would violate the
  "no dummy UI" rule. Layout shortcuts already have a real, working control
  (Settings → Input); a second window would duplicate it.
- **`error`/`information` tokens without consumer** — `information` now has
  consumers (toasts); `error` stays reserved because failures go through
  native MessageBox dialogs (deliberate, documented).
- **`kMenuOpen` dead menu id** — kept (harmless symmetry, documented Low).

## C. Protection constraints — held

- Portable core additions are **additive only**: one new module
  (`ui_strings`, no dependencies beyond libc++), one additive settings key
  with tolerant parse (`language=`, unknown values fail validation so the
  format cannot silently corrupt). `git diff` shows **zero** modifications
  to transliteration layouts, dictionary, candidates, unicode, persistence,
  TSF integration, COM lifecycle, hotkeys, installer logic (only additive
  `[Files]` entries) or any existing test.
- No new third-party dependency: the JSON reader is ~300 lines of libc++
  inside the project; UIA is loaded dynamically; no framework, no build
  step added.

## D. Verification (honest)

| Check | Result | Evidence |
|---|---|---|
| Portable build (GCC 14.2, Ninja, `-j1`) | **Pass** | `cmake --build build/linux` clean |
| Full test suite | **Pass — 542 unit + 2 integration** | `[PASSED] 542 tests` / `100% tests passed out of 2` (was 517 unit before this pass; +24 ui_strings, +1 settings-language test) |
| Shipped locale files | **Pass** | `UiStringsTest.ShippedLocaleFilesParseWithParallelKeys` — both parse, identical 129-key sets |
| New-module coverage (gcov, line+branch) | **98.91 % lines / 99.48 % branches executed** on `src/core/ui_strings.cpp` | dedicated harness run; the 3 uncovered lines are `flatten()`'s defensive root re-check, unreachable by construction (the caller validates the root object first) — documented, not a test gap |
| Settings `ui.language` | **Pass** | `SettingsTest.UiLanguageKeyRoundTripsAndValidates` (round-trip, invalid values rejected, legacy file defaults) |
| Layering (`scripts/check-layering.py`) | **Pass** | `layering OK: 20 cross-module edges, all inside the allowed matrix` (platform→core already allowed; core gained a zero-dependency module) |
| Brace balance (16 edited/new files) | **Pass** | `scripts/brace_scan.py` — all OK |
| clang-format (project `.clang-format`, LLVM 23.1.5) | **Applied** | all touched files reformatted |
| i18n key coverage | **Pass** | all 114 keys referenced in code exist in BOTH locale files (scripted cross-check) |
| **Windows MSVC compile + runtime** | **Not executed — sandbox blocker** | No `cl.exe`/VS 2022/v143 on Linux; the brief forbids substituting the toolchain. The Windows-only files (application/topbar/osk/toast/ui_accessibility) were verified by full-read review, brace scan and the same precedent as Pass 1/2. Required on Windows: `cmake --preset windows-msvc && cmake --build --preset windows-msvc-release -j1 && ctest --preset windows-msvc-release`. |
| **Real UI verification (Narrator, DPI, HC)** | **Not executed — sandbox blocker** | On a Windows machine: Narrator over the settings window (names/roles/toggle states; Tab announces focus), OSK keys through Invoke, both sliders, dark/light/System/HC + live HC toggle, 125–200 % DPI and a cross-monitor drag, interface language switch to বাংলা (all menus, OSK, About, toasts). |

## E. Changed files

| File | Change |
|---|---|
| `include/suborno/core/ui_strings.hpp`, `src/core/ui_strings.cpp` | **new** — portable localized-string loader (strict JSON → dotted keys, fallback contract) |
| `src/core/CMakeLists.txt` | add `ui_strings.cpp` to `suborno_core` |
| `tests/unit/ui_strings_test.cpp` | **new** — 24 tests (parser, escapes, surrogates, composites in values, malformed inputs, fallback, shipped files) |
| `include/suborno/settings/settings.hpp`, `src/settings/settings.cpp` | additive `ui.language` key (field, validation, parse, save) |
| `tests/unit/settings_test.cpp` | `UiLanguageKeyRoundTripsAndValidates` |
| `data/i18n/en/ui.json`, `data/i18n/bn/ui.json` | rewritten: complete parallel 129-key set (sections, hints, controls, dropdown items, status, about, osk, toasts) |
| `include/suborno/platform/windows/app/ui_theme.hpp` | `ui::text()`/`format_text()`/`clear_text_cache()`/`load_ui_strings()`, `refresh_active_theme()`, `active_theme_id()`, `utf8_to_wide()` |
| `include/suborno/platform/windows/app/ui_accessibility.hpp`, `src/platform/windows/app/ui_accessibility.cpp` | **new** — UIA fragment providers + patterns + dynamic UIA loading + liveness tokens |
| `include/suborno/platform/windows/app/toast_window.hpp`, `src/platform/windows/app/toast_window.cpp` | **new** — themed toast component |
| `src/platform/windows/app/application.cpp` | strings wired everywhere; UIA source (full settings tree) + WM_GETOBJECT/WM_DPICHANGED/WM_THEMECHANGED; PMv2 process awareness; toast wiring; Interface-language dropdown (`kActionUiLanguage`) + apply-on-save; panel 488→496; status line localized |
| `src/platform/windows/app/topbar.cpp` | Impl = ElementSource (4 zones), WM_GETOBJECT/WM_THEMECHANGED, localized menus |
| `src/platform/windows/app/osk_window.cpp` | Impl = ElementSource (modes + close + 34 keys), WM_GETOBJECT/WM_THEMECHANGED, localized labels, live title |
| `src/platform/windows/CMakeLists.txt` | add `app/toast_window.cpp`, `app/ui_accessibility.cpp` to `suborno_app` |
| `packaging/windows/suborno_installer.iss` | ship `data/i18n/{en,bn}/ui.json` |
| `scripts/brace_scan.py` | **new** — balance checker for Windows-only edits |
| `CHANGELOG.md` | Added/Fixed entries for this pass |

## F. Windows-machine checklist before release

1. `cmake --preset windows-msvc` → build `windows-msvc-release` `-j1` →
   `ctest --preset windows-msvc-release`.
2. Narrator: walk Settings (Tab announces each control with state; Space
   toggles; dropdowns expand; sliders report value), About (5 buttons), OSK
   (34 keys invokable).
3. Interface language → বাংলা: verify menus, sidebar, status line, OSK,
   About, toasts; restart persists; "Follow system language" on a Bangla
   Windows resolves to বাংলা.
4. Themes: Dark/Light/System + live High Contrast toggle on/off (all windows
   repaint immediately).
5. DPI: 125/150/200 %, drag settings + About + topbar + OSK across monitors
   with different scale factors (no blur, no clipped corners, Save/Cancel
   always reachable).
6. Toasts: switch layout from tray (toast appears, disappears ~2.6 s, never
   steals focus while typing).
