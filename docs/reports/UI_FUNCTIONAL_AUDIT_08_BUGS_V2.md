# UI_FUNCTIONAL_AUDIT_08 — Bug & Regression Report (Phase 2, 2026-11-24)

Severity scale per mission Phase 11. Protection rules honored: no layout,
dictionary, TSF or persistence-format changes; no feature removals; existing
tests untouched and all passing.

## Fixed in this audit

### FIND-06 (Medium) — Bangla-script font family names could never persist
- **Symptom**: `valid_font_family` (settings.cpp) whitelisted ASCII only and
  measured the GDI `LF_FACESIZE` cap (32 UTF-16 units) as **32 bytes**. A
  font whose family name contains Bangla script (perfectly valid on Windows)
  was rejected at save; names ≥ 33 bytes were rejected regardless of content.
- **Reproduction**: executed — `UIAuditFontFamily.BengaliScriptFontNamesRoundTrip`
  failed on the pre-fix build ("বাংলা ফন্ট" → save `invalid_format`).
- **Root cause**: ASCII whitelist + byte-count cap in the settings validator.
- **Fix (smallest safe, additive)**: local UTF-8 decoder in
  `src/settings/settings.cpp` (settings is a layering leaf; no new module
  edge). ASCII input keeps the exact legacy whitelist (including rejecting
  `@`, `=`, controls); non-ASCII printable code points (≥ U+00A0) are
  accepted; cap = **32 code points** (LF_FACESIZE − 1) with a 128-byte hard
  bound; invalid UTF-8 (truncated sequences, overlongs, surrogates, > U+10FFFF)
  rejected. Persistence format unchanged.
- **Verification**: both new font tests pass; **full regression 632 unit +
  2 integration PASS**; every pre-existing rejection assertion
  (33×'a', "Noto@Sans", "Bad=Name", 38-char name) still passes unchanged;
  `check-layering.py` OK (no new module edge); clang-format clean.
- **Residual**: GDI rendering of a Unicode-named font on the live shell is
  Windows-runtime BLOCKED (the model now accepts what GDI can consume).

## Open findings (fix requires the Windows toolchain — BLOCKED here)

### FIND-01 / UIA-01 (Cosmetic, a11y) — OSK paints an empty dead key
The second letter row reserves one empty slot (`{0, L'\0'}`,
osk_window.cpp:72). The paint loop draws all 34 keys, so an **empty button
box** is visible, and the UIA provider exposes it with an **empty Name**
(element_info default branch, ui_accessibility.cpp:457-459).
Proposed minimal fix: skip painting and skip UIA exposure for keys with a
zero VK. BLOCKED (Windows-only file, cannot compile/test here).

### FIND-02 / UI-01 (Low) — OSK mode-button hover cross-wired
`hover_mode_english` is true when hovering *either* mode button, but the
hover face is applied as `hover && is_english()` for the English button and
`hover && !is_english()` for the Bangla one (osk_window.cpp:578-587) —
hovering the inactive button highlights the *active* one. Proposed fix: track
hover per button. BLOCKED.

### FIND-03 / I18N-01 (Low) — Tray-menu error strings hardcoded English
Web-link failure paths in `handle_menu_command` use raw literals
("Suborno could not open the releases page." / "…documentation.",
application.cpp:3921-3924) while the About dialog localizes the same
messages (application.cpp:766-776). `error.releases_open_failed` /
`error.documentation_open_failed` keys exist in both locales. Proposed fix:
route through `ui::text`. BLOCKED.

### FIND-04 / UIA-02 (Low) — Missing explicit AutomationIds
OSK (37 elements) and About (5 buttons) expose positional `element-N` ids
while every other surface has stable ids. Proposed fix: add `osk.key.<name>`
and `about.<action>` ids. BLOCKED.

### FIND-05 / UIA-03 (Low, a11y) — UIA IsEnabled always TRUE
`GetPropertyValues` returns IsEnabled=TRUE unconditionally
(ui_accessibility.cpp:271-274); disabled popup items only signal through
focusable=false and Invoke refusal. Proposed fix: reflect
`ElementSource::element_info.enabled`. BLOCKED.

### FIND-11 / UIA-04 (Low, a11y) — Candidate window has no UIA exposure
Documented design decision (ui_accessibility.hpp:35-37): screen-reader users
get no candidate announcements. Flagged for a future design change; no silent
change made.

## Documented design notes (not defects)

- **FIND-07**: no Reset/Restore-defaults control in Settings (model supports
  defaults recovery; a control is a feature request).
- **FIND-08**: idle-time UI slider caps at 60 s while the model allows 1–3600
  (UI narrower than model — benign; recorded so RangeValue max is not
  mistaken for the model bound).
- **FIND-09**: dead handler `kMenuOpen` (2001) for an item never created;
  `CtrlKind::kStatus` has no producer (the status line paints directly).
  Harmless dead code.
- **FIND-10**: `WM_CLOSE` on the Settings window = Cancel + hide with no
  dirty-state warning — matches the documented Cancel contract, worth a
  future confirmation prompt.
- **FIND-12**: hotkey-registration failures only reach
  `OutputDebugStringW` (application.cpp:3274/3296) — no user-visible signal
  when another app owns the combo. Future UX item.
- **FIND-13**: the shortcut dropdown appends a saved Windows-reserved combo
  (Alt+Shift / Win+Space) for display; choosing it cannot register
  (`RegisterHotKey` fails silently) — display-only by design, FIND-12 is the
  missing feedback half.

## Regression status after fixes

| Suite | Result |
|---|---|
| Unit tests (609 pre-existing + 23 new audit tests) | **632/632 PASS** |
| Integration tests | **2/2 PASS** |
| `scripts/i18n_key_check.py` | parity OK, 0 missing, 0 dead |
| `scripts/contrast_check.py` | 22 pairs, 0 required failures |
| `scripts/check-layering.py` | 20 edges, all allowed |
| clang-format (LLVM 23.1.5, project style) | clean on changed files |
| Engine evidence CSV (`bds_national_positional_engine_rows.csv`) | 121/121 PASS (unchanged) |
