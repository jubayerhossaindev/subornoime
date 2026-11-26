# UI Functionality Fix Report — Full Pass

**Date:** 2026-11-22 · **Scope:** Desktop shell (topbar, tray, settings window),
TSF text service, typing engine, settings schema · **Trigger:** the complete
UI bug-fix & improvement requirement list (7 groups).

Every change below is wired to real backend behavior. No placeholder, dummy
button, or display-only option was introduced; where an option cannot be made
functional on Windows (reserved system chords), it is not offered in the UI.

---

## 1. Desktop icon & topbar menu (Requirement 1)

| Problem | Fix |
|---|---|
| Double-clicking the desktop icon opened the Settings window directly | Launch now honors the stored `start_mode`: topbar / system tray / settings window / last state (`apply_start_mode()` in `application.cpp`). Default (`last-state`) shows the floating topbar — the main UI. A second instance (double-click while running) posts `kSecondInstanceShowTopbar`, revealing the topbar instead of resurrecting the settings window. |
| Topbar menu options not reliably working | Topbar zones re-anchored to the right edge of an auto-sized pill; mode zone opens the layout list, wrench opens Settings / Change Font / About, close opens System Tray / Exit. Every path calls a real handler (`handle_click`, `topbar.cpp`). |
| Topbar label truncation ("Suborno Phon…") | The pill now measures its label with `GetTextExtentPoint32W` and resizes itself (`measure_width` / `update_window_width`), so long names like বাংলা (Suborno Classic) fit. Tray left-click reveals the topbar (`show_topbar()`). |

## 2. Settings dropdown & close options (Requirement 2)

- Wrench icon: dropdown with **Settings…**, **Change Font…**, **About
  Suborno…** — all functional.
- Close (×) icon: dropdown with **System Tray** (hides the topbar and keeps
  the app + tray icon running; persists `last_ui_mode=tray`) and **Exit**
  (removes the tray icon, unregisters the hotkey, quits the process). Both
  handlers were already reachable; labels now match the requirement and the
  tray-dim path persists state.

## 3. Complete settings functionality (Requirement 3)

- **Save** persists the full staged settings through `Engine::save_settings`
  (atomic write), rolls back the registry startup flag on failure, then
  applies live: hotkey re-registration, font refresh, topbar settings +
  label, status line. **Cancel / Esc / ×** discards staged changes and
  hides. Settings survive restart (`settings.ini`, change-detected reload).
- **General tab overlap bug fixed** — "Remember Last Input Layout" (y=118)
  and "Start with Windows" (y=148) had shared Y=120 and rendered on top of
  each other.
- **New functional controls** (all backed by real consumers):
  - "When Windows starts, show" dropdown → `start_mode` (implemented launch
    behavior).
  - Interface: "Use OS Default Bangla Font" + "App & Candidate Font"
    dropdown → font family applied app-wide and in the candidate window.
  - Topbar: "Remember Topbar Position" → `topbar_position_x/y` saved after
    each drag and restored on launch.
  - Input: "Layout Switching Shortcut" dropdown (F1–F4, F10, F12,
    Ctrl+Space) → registered as a real global hotkey that cycles layouts.
- Sliders/dropdowns/checkboxes operate on the staged snapshot exactly as
  before; slider display values are clamped into the track range.
- Keyboard navigation (Tab / Space / arrows / Enter / Esc) unchanged and
  covers the new controls (status labels are skipped).

## 4. System tray & font selection (Requirement 4)

- Tray menu gains a **Font** submenu (System Default + enumerated
  Bangla-capable families, radio-checked) next to the Keyboard Layout
  submenu; selections apply immediately and persist.
- The same choice is available in the main UI (topbar wrench → Change
  Font…) and in Settings → Interface, so font changing is no longer tied to
  the tray.
- Effective font logic: `use_os_default_bangla_font` → OS default;
  otherwise the saved family is used by the settings window, topbar, custom
  menus, and the TSF candidate window (`CandidateListUIElement` now takes
  the configured family; empty falls back to Nirmala UI).

## 5. Simple layout naming (Requirement 5)

- The word **"Phonetic" is gone from every user-visible layout name**:
  English, বাংলা (Suborno), বাংলা (Suborno Classic), Suborno Smart, Khipro, Probhat, National.
- Engine metadata (`engine.cpp`) and the shell's layout list
  (`kLayouts` in `application.cpp`) are kept in sync; the topbar label, tray
  submenu, settings dropdown, and hotkey cycling all render these names.
- A unit test now asserts no layout display name contains "Phonetic".

## 6. Bengali typing & layout switching (Requirement 6)

- **Root cause of "English letters instead of Bangla":** the TSF text
  service kept its own in-memory engine and only reloaded settings on focus
  changes. After switching layouts in the desktop shell, keystrokes kept
  using the stale layout until the target app lost/regained focus.
- Fix: `OnTestKeyDown` and `OnKeyDown` now call
  `Engine::reload_settings_if_changed()` (cheap file-stamp check) before any
  decision, so a layout switch takes effect on the **very next keystroke**,
  and the idle-session recreation already keyed on the layout id completes
  the synchronization. Restart keeps the selected layout (`settings.ini`).
- **English passthrough:** `LayoutKind::passthrough` + `EnglishLayout`;
  the text service returns keys uneaten, so English typing goes straight to
  the host application. Switching to English mid-composition commits the
  composed text first instead of stranding the composition.
- New global shortcut (requirement 3's dropdown) cycles layouts instantly
  without touching the mouse.

## 7. Testing (Requirement 7)

- Portable core rebuilt and the full suite run on Linux (CMake + GTest):
  **186 unit tests + integration tests, all passing** (`ctest`), including
  the new `EngineTest.RegistersEnglishPassthroughLayoutWithCleanNames` and
  `SettingsTest.SavesAndLoadsFontPositionAndUiModeKeys`.
- `scripts/check-layering.py` passes (19 cross-module edges, all allowed).
- Windows-only files (application.cpp, topbar.cpp, text_service.cpp,
  candidate_list_ui_element.cpp) were reviewed for API misuse; the Windows
  build smoke test was updated for the new candidate-window constructor
  signature. The MSVC build (`windows-msvc-release` preset) should be run
  on a Windows machine before shipping the installer.

---

## Changed files

| File | Change |
|---|---|
| `include/suborno/layout-api/input_layout.hpp` | added `LayoutKind::passthrough` |
| `include/suborno/layouts/english/english_layout.hpp` (new) | English layout class |
| `src/layouts/english/english_layout.cpp` (new) | identity transliteration, passthrough kind |
| `src/layouts/english/CMakeLists.txt` (new) | build target |
| `src/layouts/CMakeLists.txt`, `src/core/CMakeLists.txt` | wire the new target |
| `src/core/engine.cpp` | register `english`; clean display names |
| `include/suborno/settings/settings.hpp` | `font_family`, `last_ui_mode`, `topbar_position_x/y` |
| `src/settings/settings.cpp` | parse/validate/save for the new keys (+`parse_int`) |
| `include/suborno/platform/windows/app/ui_theme.hpp` | process-wide UI font family used by `create_message_font` |
| `include/suborno/platform/windows/app/topbar.hpp` | font/position host callbacks, `remember_position`, `refresh_font()` |
| `src/platform/windows/app/topbar.cpp` | auto-width pill, right-anchored zones, System Tray/Exit, font menu, position restore/save |
| `src/platform/windows/app/application.cpp` | start-mode launch, second-instance signal, hotkey, font plumbing, tray Font submenu, rebuilt settings pages, overlap fix |
| `include/suborno/platform/windows/tsf/candidate_list_ui_element.hpp` | font family member/param |
| `src/platform/windows/tsf/candidate_list_ui_element.cpp` | use configured font |
| `src/platform/windows/tsf/text_service.cpp` | per-keystroke settings reload, passthrough handling, candidate font wiring |
| `tests/unit/engine_test.cpp`, `tests/unit/settings_test.cpp` | new/updated tests |
| `tests/windows/windows_build_smoke_test.cpp` | updated constructor calls |
| `docs/specifications/desktop-settings-schema.md` | new keys, layout-name contract, hotkey note |
| `data/i18n/{en,bn}/ui.json` | System Tray / Exit / Change Font strings |
| `CHANGELOG.md`, `README.md` | release notes |
