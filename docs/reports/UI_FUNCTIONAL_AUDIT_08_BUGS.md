# Report 08 — Bug & Regression Report

All bugs below were found by this audit against HEAD `db42251`, with reproduction,
root cause, the smallest safe correction, and re-test results. No protected area
(TSF/COM lifecycle, transliteration, dictionary, persistence format, hotkeys, installer
behavior, public interfaces) was modified.

## BUG-01 — Windows app build broken by duplicated member definitions — **Critical, FIXED**

- **Symptom / reproduction:** `src/platform/windows/app/application.cpp` defined
  `control_automation_id()`, `action_automation_name()` and `kSidebarAutomationIds`
  **twice each** inside `class Application` (lines 1227–1277 and a verbatim copy at
  1290–1340 at the audit baseline). Redefinition of a member is ill-formed C++ —
  MSVC emits C2084/C2535 and the desktop app (`Suborno.exe`) cannot compile at all.
  Reproduced statically by a duplicate-definition scan (exact signature match ×2).
- **Why it survived:** Windows-only TUs are not compiled by the Linux sandbox pipeline
  (54-TU `compile_commands.json`), so no local build ever saw the file; the previous
  pass's "verified by full-read review" missed the copy-paste block.
- **Root cause:** an earlier modernization edit duplicated the UIA accessor block
  instead of moving it.
- **Fix (commit `993968c`):** delete the second copy; keep the first. No behavioral
  change — the copies were byte-identical.
- **Re-test:** one definition each (grep ×1), `scripts/brace_scan.py` OK, project
  clang-format clean after the follow-up format commit, portable suite still
  542 unit + 2 integration PASS. **Residual honest gap:** a real MSVC compile of the
  Windows app is still pending (BLOCKED, sandbox) — first item on the Windows checklist.

## BUG-02 — Topbar tooltips could never localize — **High (UX/i18n), FIXED**

- **Reproduction:** `scripts/i18n_key_check.py` at the audit baseline: 4 keys
  referenced by code missing from BOTH locale files — `topbar.tooltip.layout`,
  `topbar.tooltip.settings`, `topbar.tooltip.tray`, `topbar.tooltip.toggle`.
  The compiled English fallback always rendered, so choosing বাংলা left tooltips
  English — precisely the "research findings not applied everywhere" failure class.
- **Root cause:** the tooltip feature was added after the locale-table pass; the
  locale JSONs were never back-filled (the earlier report's "all 114 keys covered"
  claim predated the tooltips).
- **Fix (commit `993968c`):** added the 4 keys to `data/i18n/en/ui.json` and
  `data/i18n/bn/ui.json` (Bangla translations consistent with existing terminology).
- **Re-test:** key cross-check 0 missing; en/bn parity 135 = 135; the shipped-files
  parity test (reads the real JSONs) passed inside the 542-test rerun.

## BUG-03 — Dead locale keys — **Low, FIXED**

`toast.language_changed` and `topbar.state.idle_dimmed` existed in both locales with no
raising/rendering code path. Fixed in the same pass as BUG-04:

- `toast.language_changed` is now wired: `save_settings_from_ui()` captures the
  pre-save `ui.language`, and when the value actually changed, the success path shows
  the language toast instead of the generic "Settings saved" toast (it fires only
  after save + locale apply succeeded).
- `topbar.state.idle_dimmed` ("Idle" / "অলস") had no natural consumer — idle dimming
  is a pure layered-window alpha effect with no text label — so the key was removed
  from both locales rather than inventing UI for it.
- Re-test: `scripts/i18n_key_check.py` 0 missing / 0 dead; en/bn parity 160 = 160;
  `UiStringsTest.ShippedLocaleFilesParseWithParallelKeys` green in the full rerun.

## BUG-04 — Error/confirmation dialogs are English-only — **Medium, FIXED (code + data verified; MSVC compile + rendering BLOCKED)**

≈20 `MessageBoxW` strings (`show_error` call sites, settings-failure report, the
Clear-Learnt confirmation) bypassed the locale table, so the bn UI was incomplete on
error paths. Fixed exactly as planned in the original finding:

- **Locale data:** 26 new keys added to BOTH `data/i18n/{en,bn}/ui.json` —
  22 `error.*` messages + the `error.code_line` template ("Error: {code}" /
  "ত্রুটি: {code}"), `confirm.clear_learnt.title/.text`, and `common.app_name`
  ("Suborno" / "সুবর্ণ"). en/bn now 160 = 160, sets identical.
- **Code (`application.cpp`):** every `show_error` message call site, the
  `show_error`/`report_settings_failure` dialog title and code-line formatting, the
  Clear-Learnt `MessageBoxW` text/title, and the post-language-load startup dialogs
  (icon load, window class registration, window creation) now route through
  `ui::text` with the previous English string as the compiled fallback. HRESULT and
  Win32 error formatting is preserved (`swprintf_s` into the same buffers; the code
  line renders through `ui::format_text`).
- **Documented load-order contract:** dialogs raised by `initialize()`
  (`report_settings_failure` operations, `kSettingsFailureMessage`) keep their
  compiled English strings — the language setting is read by the very engine
  `initialize()` creates, so the locale table cannot be loaded before them; the
  fallback contract renders them exactly as before, and a code comment documents
  this. Every dialog that can appear after the shell is up localizes.
- **Executed verification in this sandbox:** `scripts/i18n_key_check.py` — all 26
  keys referenced by code and present in both locales, 0 code-referenced gaps;
  `UiStringsTest` shipped-files parity green inside the full 542 unit + 2 integration
  rerun; clang-format (LLVM 23.1.5) clean; brace scan OK.
- **Honest residual:** `application.cpp` is a Windows-only TU — the MSVC compile of
  the fix and the on-screen rendering of the localized dialogs are BLOCKED here;
  they stay on the Windows checklist (Report 09 §5).

## Verified-clean areas (evidence-based, no action)

- Slider drag arithmetic (Phase-01 regression fixed earlier, re-verified this audit:
  client-x clamp + documented comment).
- Contrast floors (recomputed; Report 05 §3) — an initial read flagged
  dark `on_accent`/`accent_fill` and light `accent` as failures, but that was a bug in
  my checker's COLORREF channel order; corrected results PASS everywhere. Recorded
  here because a false alarm is also a fact worth keeping.
- Duplicate-definition scan across all six Windows app TUs: only BUG-01.
- Menu id dispatch: every tray/topbar id handled; only documented `kMenuOpen` alias.
- Settings validation, atomic write, quarantine: executed tests green.
- Layering check green (20 edges allowed); brace scan green on all touched files.

## Regression testing after fixes

| Check | Result |
|---|---|
| Portable build (Ninja, GCC 14.2, `-j1`) | clean (JSON is data; binaries unchanged) |
| Unit suite | **542/542 PASSED** (re-run after the BUG-03/04 pass) |
| Integration suite | **2/2 PASSED** |
| clang-format (LLVM 23.1.5, project config) | clean on `application.cpp` (`--dry-run --Werror`) |
| brace_scan on `application.cpp` | OK |
| Layering | OK (strings only — no new module edges) |
| i18n cross-check | 0 missing / 0 dead; parity 160 = 160; 26 new keys in both locales |

## Severity accounting

Fixed: Critical ×1 (BUG-01), High ×1 (BUG-02), Medium ×1 (BUG-04), Low ×1 (BUG-03).
No Critical, High, Medium or Low functional bug remains open in anything this sandbox
can verify. The single remaining open item is environmental: the Windows MSVC compile
of the fix commits (BUG-01/02 in `993968c`; BUG-03/04 in the follow-up commit) and the
on-Windows runtime confirmation — first entries of the delivered Windows checklist.
