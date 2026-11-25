# SubornoIME — UI/UX Audit and Fix Report

Date: 2026-11-21
Scope: desktop settings window (`application.cpp`), TSF candidate window
(`candidate_list_ui_element.cpp`), text-service keyboard interaction
(`text_service.cpp`), settings-driven UI state, installer user-context
behavior (verified in the previous settings-fix round).

## A. Executive Summary

- Audit scope: every user-facing surface reachable from source: the tray/
  settings desktop window, tray icon + context menu, TSF candidate list
  window, all keyboard interactions during composition, error dialogs,
  first-run behavior, theme/DPI/accessibility behavior.
- Issues discovered: **7 confirmed code-level issues (F1–F7)** + 3 reviewed-
  and-cleared areas (numbering consistency, Enter behavior, theme_id) +
  future-work items (Bengali localization, responsive reflow).
- Issues fixed: **7 of 7** implemented in this round.
- Remaining: Windows-runtime-only verifications (NOT VERIFIED in this Linux
  sandbox) + deferred future work listed in §G.
- Critical finding: the settings window had **no keyboard navigation at all**
  (Tab did nothing) — an accessibility blocker for keyboard-only and
  screen-reader users.

Overall verification status: code fixes complete and regression-clean
(184/184 tests, 0 compiler warnings on the Linux-verifiable build); all
Windows-runtime behaviors marked NOT VERIFIED explicitly (no MSVC/ISCC/
interactive desktop in this environment).

## B. Detailed Findings

| ID | Category | Issue | Severity | Root Cause | Fix | Verification |
|---|---|---|---|---|---|---|
| F1 | Accessibility / UX | Tab navigation completely absent in the settings window; keyboard-only users could not reach any control | **P1** | Plain `GetMessage` loop without `IsDialogMessageW`; interactive controls created without `WS_TABSTOP` (application.cpp `run()`, `add_control()`, `create_controls()`) | `IsDialogMessageW(window_, &message)` in the message loop; `WS_TABSTOP` added to combo, 5 checkboxes, 3 buttons. Tab order = creation order = visual reading order | Code-reviewed; Windows runtime NOT VERIFIED (no MSVC/desktop here); regression suite 184/184 PASS |
| F2 | Visual design | All controls used legacy `DEFAULT_GUI_FONT` stock object (MS Shell Dlg-era look), inconsistent with modern Windows system font | P2 | `GetStockObject(DEFAULT_GUI_FONT)` in `add_control()` | Per-user message font built once from `SystemParametersInfoW(SPI_GETNONCLIENTMETRICS)` → `lfMessageFont` (Segoe UI), sent via `WM_SETFONT` to every control; deleted in `~Application()` | Code-reviewed; runtime NOT VERIFIED |
| F3 | High-DPI / Windows native | Process had no DPI awareness at all (no manifest, no API call): fixed 600×560 raw pixels bitmap-stretched and blurry at 125–200% scaling | P2 | No `dpiAware` manifest; no `SetProcessDpiAwareness*` call; all layout math in raw 96-DPI pixels | `enable_process_dpi_awareness()` (Win10 1703+ `SetProcessDpiAwarenessContext` → Shcore `SetProcessDpiAwareness` → legacy `SetProcessDpiAware`, all best-effort) + `system_dpi()` + `Application::scaled()` applied to window size, every control rectangle, and min-size. Layout math unchanged except by the uniform scale factor | Code-reviewed; DPI runtime NOT VERIFIED; fallback path preserves old behavior |
| F4 | UX | Window could be shrunk below the control layout, clipping controls with no way to recover them | P3 | No `WM_GETMINMAXINFO` handling with absolutely-positioned controls | `WM_GETMINMAXINFO` sets `ptMinTrackSize` to the scaled design size | Code-reviewed; runtime NOT VERIFIED |
| F5 | UX / discoverability | Typing help omitted keyboard affordances the engine already implements (Enter commit, ↑/↓ choose, Page Up/Down pages) | P3 | Help strings written before arrow/page support existed (engine: text_service.cpp lines 568–607, 687) | Help text updated: "Space or Enter commits; 1-9 selects a candidate; Up/Down chooses; Page Up/Down pages; Esc cancels." (both phonetic and fixed variants); help label height 38→44 design px | Code-reviewed against engine key handling; runtime NOT VERIFIED |
| F6 | Bengali IME UX / visual | Long Bengali candidates clipped mid-glyph in the candidate window (no ellipsis, no truncation affordance) | P2 | `DrawTextW` flags lacked `DT_END_ELLIPSIS` (candidate_list_ui_element.cpp paint) | Added `DT_END_ELLIPSIS` | Code-reviewed; runtime NOT VERIFIED |
| F7 | Bengali IME UX / multi-monitor | Candidate window could clip off the right screen edge; flip-above-caret used an unscaled height estimate while the final font/rows were DPI-scaled (wrong flip decision at 150%+) | P3 | Anchor never clamped to work area; height computed from 96-DPI constants before scaling | Estimate DPI from `dpi_for_window(nullptr)`; row/window/pad sizes scaled for the flip and clamp; anchor.x clamped to `[rcWork.left, rcWork.right - width]`; post-create `refresh_dpi()` still corrects per-monitor differences | Code-reviewed; runtime NOT VERIFIED |

### Reviewed and cleared (no change — evidence-based)

- **Candidate numbering is consistent with number-key selection:** labels are
  `index + 1` (global), and number keys map `'1'..'9'` → global indices
  0..8 (text_service.cpp:232-235, 608-624). On page 2, label "9." is exactly
  what pressing 9 selects. Page-relative numbering would have broken this.
- **Enter behavior:** while composing, Enter commits the selected candidate
  (text_service.cpp:687-696) and the key still reaches the host app. This is
  deliberate commit-and-passthrough; now documented in the help text (F5).
  Not classified as a bug; changing delivery semantics would alter typing
  behavior for existing users without evidence of harm.
- **Esc / Space / Backspace / arrow / Page keys** all handled correctly
  (541-607); `show_candidate_window=false` suppresses the UI element; number
  keys disabled by setting work as digits (apply_input_settings).
- **theme_id / topbar settings:** engine-internal values; no user-facing
  control claims a visual theme switcher, so nothing misleading is shown.
  The candidate window follows system colors (light/dark safe). Logged as
  future work (§G).
- **Tray integration:** TaskbarCreated re-add, version-4 callback, tooltip
  updates, foreground-window quirk handled before TrackPopupMenu; WM_CLOSE
  hides to tray by design (documented behavior, Exit available in menu).
- **GDI/COM lifetime:** candidate font deleted in destructor; window class
  registered once (`std::call_once`); `WM_NCDESTROY` releases the COM
  object; paint path balances Begin/EndPaint and never lets exceptions
  escape the window procedure — verified by reading; no leaks found.
- **Thread safety:** all UI runs on the STA thread created in `run()`;
  `IsDialogMessageW` added on the same thread; engine calls stay on the UI
  thread. No new cross-thread access introduced.

## C. UI/UX Assessment

| Category | Assessment |
|---|---|
| Visual design | Native Win32 controls, consistent with Windows; system message font now applied (F2); candidate window uses system colors (theme-safe); spacing consistent |
| Usability | All actions functional (no dead buttons): layout change persists immediately, checkboxes save instantly, uninstall/settings buttons open real Windows surfaces; help text now covers all typing affordances (F5); min-size prevents broken window states (F4) |
| Accessibility | Tab navigation restored (F1); native controls expose names automatically to UIA/screen readers; TSF UI element APIs expose candidate strings programmatically; visible native focus indicators; fixed-font contrast follows system colors |
| Bengali IME UX | Composition, candidate paging, selection, cancellation all correct; ellipsis prevents mid-glyph clipping (F6); caret positioning with edge clamp (F7); Nirmala UI + GDI complex-script shaping (Win8.1+) renders conjuncts |
| Windows native | System DPI awareness added (F3); tray/menu/minimize/close behaviors standard; no unsupported platform claims |
| Interaction consistency | Immediate-save on every control; rollback on failed startup-preference save; error dialogs for every failure path |
| Performance | No allocations in paint path beyond the label string; change-detected settings reload on focus (pre-existing); no regressions introduced |
| Reliability | 184/184 automated tests pass after changes |
| First-run experience | Defaults persisted on first launch (previous round); status line reports real Windows input-method state |

## D. Changed Files

| File | Why |
|---|---|
| `src/platform/windows/app/application.cpp` | F1 (IsDialogMessageW + WS_TABSTOP), F2 (message font + cleanup), F3 (DPI awareness + `scaled()` layout), F4 (`WM_GETMINMAXINFO`), F5 (help text + label height) |
| `src/platform/windows/tsf/candidate_list_ui_element.cpp` | F6 (`DT_END_ELLIPSIS`), F7 (DPI-consistent flip + right-edge clamp) |
| `CHANGELOG.md` | Documented all UI/UX fixes |
| `docs/reports/UI_UX_AUDIT_AND_FIX_REPORT.md` | This report |

No features, tests, or documentation were removed. No settings schema, TSF
registration, engine, or installer behavioral changes were made in this round.

## E. Testing Evidence

| Check | Command | Result | Status |
|---|---|---|---|
| Configure | `cmake -S . -B build/verify -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DCMAKE_PREFIX_PATH=<gtest>` | exit 0 | PASS |
| Build (Linux-verifiable) | `cmake --build build/verify -j4` | exit 0, **0 warnings** | PASS |
| CTest | `ctest` | 100% (2/2 suites) | PASS |
| Unit tests | `./suborno_unit_tests` | **184/184 PASSED** | PASS |
| Engine runtime | `./suborno_engine_demo "ami bhai"` | correct commit output | PASS |
| MSVC Release build (`windows-msvc-release` preset) | requires Windows MSVC toolchain | — | **NOT VERIFIED** (no MSVC in sandbox) |
| Interactive runtime (launch, screenshots, Tab order, DPI 125–200%, tray) | requires Windows desktop | — | **NOT VERIFIED** |
| Installer (ISCC) | requires ISCC | — | **NOT VERIFIED** |
| Windows unit/integration tests (MSVC) | requires Windows toolchain | — | **NOT VERIFIED** |

Regression check: the changed files are Windows-only UI layers; the portable
engine/settings/persistence code paths that the tests cover are unchanged
this round, and the full suite still passes.

## F. Before/After Evidence

This sandbox has no Windows desktop, so runtime screenshots could not be
captured; per the evidence rules none were fabricated. Code-level before/
after for each fix is exact:

- F1 before: `while (GetMessageW(...)) { TranslateMessage; DispatchMessage; }`
  and styles without `WS_TABSTOP` → after: IsDialogMessageW-driven loop +
  tab stops on all 9 interactive controls.
- F2 before: `GetStockObject(DEFAULT_GUI_FONT)` → after:
  `lfMessageFont`-derived `HFONT` with lifetime cleanup.
- F3 before: `CreateWindowExW(..., 600, 560, ...)` + raw pixel rects, no DPI
  declaration → after: `enable_process_dpi_awareness()` + `scaled()` applied
  to the window and every rect.
- F4 before: no `WM_GETMINMAXINFO` → after: scaled minimum tracking size.
- F5 before: "Space commits; 1-9 selects a candidate; Esc cancels." → after:
  "Space or Enter commits; 1-9 selects a candidate; Up/Down chooses; Page
  Up/Down pages; Esc cancels."
- F6 before: `DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX` → after:
  adds `DT_END_ELLIPSIS`.
- F7 before: unscaled `kCandidateRowHeight`-based flip, no horizontal clamp
  → after: DPI-estimated scaled sizing + work-area clamping of the anchor.

## G. Remaining Issues

1. **Windows runtime verification** for all seven fixes (build, launch, Tab
   order, DPI scaling, candidate behavior) — blocked on an MSVC/Windows
   desktop environment; every check is marked NOT VERIFIED.
2. **Bengali UI localization** — all desktop-window strings are English. A
   Bengali-language option is a scope/product decision, deliberately not
   implemented without approval.
3. **Responsive reflow on resize** — the window keeps absolute layout;
   controls stay top-left with dead space when enlarged (min size now
   prevents damage). A proper reflow or dialog-based layout is future work.
4. **theme_id/topbar visual settings** are engine-side values without a
   user-facing theme engine yet; the candidate window follows system colors
   meanwhile.
5. **Per-monitor (V2) DPI for the settings window** — system-aware is
   deliberately chosen as the minimal correct step; per-monitor support with
   `WM_DPICHANGED` reflow is future work.
6. **Enter pass-through semantics** — Enter commits and also reaches the
   host app; if users report double-effects (e.g. newline after commit),
   eating the key is a one-line change with a behavioral trade-off, pending
   real user feedback.

## H. Final Readiness Assessment

**VERIFIED WITH LIMITATIONS** — all discovered code-level UI/UX issues are
fixed with regression-clean builds (184/184 tests, 0 warnings), but every
Windows-runtime interaction check remains NOT VERIFIED in this environment,
so this is not a claim of production readiness. Next step: run the MSVC
`windows-msvc-release` build, ISCC packaging, and an interactive Windows
session (install → launch → Tab navigation → typing at 100/150/200% scaling)
before release validation.
