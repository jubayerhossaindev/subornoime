# Changelog

## v19.1 - Memory Safety & Engineering Standards wave

- COM/UIA exception containment: all 78 non-trivial COM entry points route
  through `run_inside_com_boundary` (new `src/platform/windows/com/boundary_guard.hpp`).
- Window-state allocations use `new (std::nothrow)` with handled failure
  (toast/topbar), matching the OSK pattern.
- `RuleBasedPhoneticLayout` owns its profile identifier (view member removed,
  lifetime contracts documented).
- MSVC hardening: `/permissive- /sdl /GS /guard:cf` + `/GUARD:CF` on every target.
- Fuzz testing: portable fuzz-smoke driver + C API fuzz campaign + adversarial
  corpus seeds; CTest `suborno.fuzz_smoke`.
- CI: configuration validation, C++20 compliance scan, comment audit, cppcheck,
  Doxygen build, license/dependency review, CodeQL, portable fuzz smoke.
- 16 new engineering documents (memory safety, ownership, lifetime, buffers,
  COM/TSF, Unicode, input validation, ABI, sanitizers, fuzzing, Borno migration record).


All notable changes to SubornoIME are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- **Bundled Bangla fonts for the zero-dependency standalone host
  (2026-11-25)** — the floating preview now renders in a bundled brand face
  instead of whatever the machine happens to have: Hind Siliguri
  (5 weights, Indian Type Foundry), Baloo Da 2 (3 weights, Ek Type) and
  Noto Sans Bengali (2 weights, Noto project), all SIL OFL 1.1, all vendored
  unmodified into `assets/fonts` with per-family license texts, a byte-exact
  SHA-256 manifest and a QA record (Bengali cmap coverage, U+09E7 glyph fix
  verified). The new Windows-only `font_loader` registers the TTFs
  per-process with `AddFontResourceExW(FR_PRIVATE)` — no font install, no
  admin, nothing visible to other apps — and the header-only
  `preview_font_choice` policy (user pick > Hind Siliguri Medium > Hind
  Siliguri > Nirmala UI) is pinned by 7 portable unit tests
  (`StandalonePreviewFont.*`). `install_standalone.cmd` ships the fonts to
  `%LOCALAPPDATA%\Suborno\fonts`; the new optional `install_fonts.cmd`
  registers them as per-user fonts (HKCU, Win10 1809+) so the TSF candidate
  list and the settings picker can select them; `verify_source_sync.cmd`
  grew to 35 markers and `verify_zero_dep.cmd` gained GATE 8 (font bundle +
  POST_BUILD copy).

### Fixed

- **Real-Windows test-suite repairs (2026-11-25, MSVC machine evidence)** —
  the first full Release CTest run on the actual Windows machine exposed six
  defect classes, all repaired without touching application functionality:
  (1) MSB5004 "solution file has two projects named run_tests" — the VS
  generator's built-in `RUN_TESTS` utility project collides
  case-insensitively with the custom target, which is now
  `suborno_run_tests`; (2) 23 Tools*Test subprocess scenarios failed with
  "The system cannot find the path specified." — `_popen` reaches
  `cmd.exe /c`, whose legacy parsing strips the first AND last quote of a
  multi-quoted command line; the harness now wraps the whole command in one
  extra outer quote pair on Windows; (3) five tests removed files still held
  open (an unclosed `std::ifstream` or a live `Engine` owning the SQLite
  connection) — legal on POSIX, a sharing violation on Windows; the handles
  are now closed before removal; (4) the settings loader opened its stream
  in text mode, so Windows translated CRLF to LF and silently ACCEPTED
  CRLF files that must be rejected as invalid_format — the loader now reads
  binary on every platform (behaviour change on Windows only, aligning it
  with the documented POSIX behaviour); (5) the two Windows-branch quarantine
  tests asserted that MoveFile refuses existing targets — the real run
  proved `std::filesystem::rename` REPLACES on MSVC too (standard semantics),
  so the tests now assert the real contract (replace + the directory-target
  conflict being surfaced as-is); (6) the TSF contract test expected the
  auto-height 8-per-page starts under default options, contradicting the
  designed `ui.candidate_fixed_height=on` default (5-per-page) — the test
  now asserts both modes explicitly. Portable suite re-run: 633 unit + 2
  integration PASS, clang-format clean.
- **UI audit Phase 12 — the four remaining static defects implemented**- **UI audit Phase 12 — the four remaining static defects implemented**
  (2026-11-24, defect-fix mission): FIND-01 the OSK's empty dead grid slot no
  longer paints a button box, never hit-tests, and is not exposed through UI
  Automation (provider `Navigate` now skips indices a source declines);
  FIND-02 the OSK mode-button hover face follows the hovered button instead of
  the active mode (per-button `hover_mode` tracking); FIND-03 the tray menu's
  Releases/Documentation failure messages are localized via the same
  `error.releases_open_failed` / `error.documentation_open_failed` keys the
  About dialog uses; FIND-05 UIA `IsEnabled` now reflects the element model
  (additive `ElementInfo::enabled` field, default true; disabled menu items
  report unavailable). Regression: new `scripts/ui_audit_static_fix_check.py`
  (15 source invariants — 15/15 FAIL on the pre-fix tree, 15/15 PASS after) +
  portable `UIAuditI18n.TrayWebLinkFailureKeysResolveAndMatchCompiledFallbacks`;
  suite 633 unit + 2 integration all PASS; layering/i18n/contrast/format
  checks green; Windows-runtime behavioral confirmation stays BLOCKED
  (docs/reports/UI_FUNCTIONAL_AUDIT_10_STATIC_DEFECT_FIXES.md).

- **UI/UX functional audit Phase 2** (2026-11-24, comprehensive audit
  mission): full re-verification of every Windows shell surface with executed
  evidence. New portable audit suite `tests/unit/ui_functional_audit_test.cpp`
  (23 tests) covering settings save/cancel idempotency, every dropdown's
  complete option set (start mode ×4, switching ×2, layouts ×7, orientation
  ×2, theme ×3, language ×3, learning ×2, encoding ×2, shortcuts 9 accepted +
  4 rejected), slider/limit boundaries, restart persistence, corrupt-file
  quarantine, unknown-layout fallback, OSK preview labels for all 7 layouts,
  encoding flip to Bijoy, learning toggles, and 157-key × 2-locale i18n
  integrity. **Fixed: `font_family` validation rejected Bangla-script font
  names and capped LF_FACESIZE at 32 bytes instead of 32 code points** —
  additive UTF-8 validator in settings.cpp (ASCII whitelist unchanged for
  ASCII input); suite 632 unit + 2 integration all PASS; layering/i18n/
  contrast evidence scripts all green. Windows-runtime checks honestly
  BLOCKED (no Windows environment in the audit sandbox); 4 static a11y/i18n
  findings recorded with proposed minimal fixes
  (docs/reports/UI_FUNCTIONAL_AUDIT_01..09_*_V2).

- **National layout rewritten as the real positional Jatiyo layout**
  (2026-11-24, deep-research mission): the previous "National" layout was a
  phonetic prototype (README: "National prototype"); it is now the positional
  National Keyboard of Bangladesh layout with four Bangla layers per key
  (base / Shift / Link-AltGr / Link+Shift) following the official diagram
  published in Unicode document L2/19-079 (Figure 4) and cross-checked
  key-by-key against the BornoTSF factual mapping (GPL source used for facts
  only — clean-room MIT, no code copied; upstream "Tested (WIP)" status
  respected). National hallmarks now typeable: Shift+4 → ৳, Shift+7 → ঁ,
  `\` → ৎ, `|` → ঃ, Link digit row ৴ ৵ ৶ ৷ ৸ ৹, Link independent vowels
  আ ই ঈ উ ঊ ঋ এ ঐ ো ঔ, ×/÷ on the minus key, ॥ on Link+Shift+G. Regular-mode
  sequences: reph via Shift+A (before the trailing cluster), broken conjunct
  via double hasanta (্‌), ঁ+া → াঁ reorder, র/য-phala reorder, র‍্য (ZWJ),
  অ+া → আ, o-kar/au-kar NFC composition. Assamese ৰ (U+09F0) and ৱ (U+09F1)
  deliberately excluded from the Link layer (inventory decision). Breaking
  change: the old phonetic tokens (`ami`, `X`, `rri`, `..`, `^`) no longer
  exist; layout kind is now `fixed`. Tests: 609/609 unit (20 National tests)
  + 2/2 integration + ASan/UBSan clean; independent engine evidence 121/121
  rows (`bds_national_positional_engine_rows.csv`). Spec:
  `docs/specifications/national-keymap.md`; report:
  `reports/standards/BDS_NATIONAL_LAYOUT_DISCREPANCY_REPORT.md`.

- **Khipro trailing/displaced virama (ক্ = U+0995 U+09CD) now typeable**
  (2026-11-24, khipro/probhat fix mission): the new explicit hasanta token
  `qq` follows the real Khipro Classic convention (khipro.khiproteam.com
  quickstart). After a consonant it emits the virama immediately, making the
  trailing virama typeable at end of input (`kqq` → ক্); the next consonant
  completes the same join without a second virama (`kqqt` → ক্ত); standalone
  or vowel-preceded it is a silent break; repeated `qq` is idempotent; the `/`
  slicer undoes it like an automatic virama. Output is exactly U+0995 U+09CD
  with no ZWNJ (documented deviation from Khipro Classic's `xx`+ZWNJ trailing
  variant). Inventory 66/67 → 74/74 (INV-064 fixed, 7 extension rows added).
  Spec: `docs/specifications/khipro-keymap.md`.
- **Probhat standalone Bengali nukta (় = U+09BC) now typeable** (2026-11-24,
  same mission): documented **SubornoIME Probhat Compatibility Extension** —
  AltGr (Ctrl+Alt) + scan 0x34 emits U+09BC. The upstream MIT Probhat KLC
  (princebillygk/probhat-layout-windows @ debc8310, byte-level re-verified
  2026-11-24) contains no U+09BC anywhere and leaves that AltGr column at -1,
  so no existing key is overwritten. ক+় stays decomposed (composition
  exclusion); ক+় is one grapheme cluster (one backspace removes it); virama-
  then-nukta is canonical-reordered at commit (nukta ccc=7 < virama ccc=9).
  Spec: `docs/specifications/probhat-keymap.md`.
- Mission regression protection: 19 new unit tests (10 khipro, 9 probhat — all
  probhat cases driven through physical scan-code events), suite 576/576 + 2/2
  integration PASS; ASan+UBSan clean; fresh engine harness
  `scripts/layout_verify/khipro_probhat_engine_verify.cpp` re-runs the
  committed 67×6 inventory + brief sequences + 62 Probhat scan probes + 41
  mission cases = 579 rows, 0 FAIL. Both limitations removed from
  `reports/layout-verification/summary/limitations.json` (resolved:true).
  Evidence: `reports/layout-fixes/KHIPRO_PROBHAT_BDS_FIX_REPORT.md`,
  `KHIPRO_PROBHAT_REGRESSION_RESULTS.csv`, `UNICODE_VERIFICATION.csv`;
  standards audit `docs/standards/BD_BENGALI_STANDARD_COMPLIANCE.md`
  (BDS 1520/1738/1935 applicability; clause-level verification honestly
  BLOCKED — copyright-protected BSTI texts; no Windows-runtime claim).
  Protected areas: only the two mission-approved layout sources changed.

### Added

- **Complete existing UI/UX audit package** (2026-11-24, HEAD `f8f33d2`):
  `reports/ui-ux-audit/` — the full Technical Truth Report of the currently
  implemented UI (12 surfaces inventoried, 133-row functional/visual matrix
  carrying the previous 129-row matrix forward with fresh verdicts, 35
  findings with reconciliation verdicts, accessibility audit CSV, screenshot
  index, execution summary). All 31 previously-fixed claims re-verified at
  source level (VERIFIED FIXED; 0 STILL PRESENT, 0 REGRESSION); all
  Windows-runtime evidence honestly BLOCKED (sandbox has no Windows desktop;
  95 matrix rows, 0 screenshots captured/fabricated). New findings N-01..N-07
  documented (missing cited evidence scripts, broken automation merge
  pipeline, README overstatements, unlabeled Interface dropdowns, missing
  embedded manifest, "84 not 85" canonical harness check count).
- **Evidence reproducibility tooling** (2026-11-24, fix pass for findings
  N-01/N-02): `scripts/i18n_key_check.py` (locale parity + placeholder +
  code-coverage check — executed: en/bn 160=160, 0 missing, 0 dead) and
  `scripts/contrast_check.py` (WCAG recompute from the theme literals —
  executed: 22 pairs, 0 required-pair failures), the two evidence scripts
  cited by earlier reports but absent from the tree; and the Windows
  evidence-merge pipeline `scripts/layout_verify/merge_audit_results.py` +
  `scripts/windows-ui-audit/functional/checkmap.json` (98 harness check-ids
  mapped 1:many onto matrix rows; merge refuses unevidenced PASS upgrades,
  never touches NOT APPLICABLE rows, reports unmatched results; contract
  tested end-to-end on synthetic results).
- **Error and confirmation dialogs localized** (2026-11-22, BUG-04): every
  user-visible `MessageBoxW` string — all 22 `show_error` messages, the
  Clear-Learnt confirmation text and title, the `Error: {code}` line and the
  dialog title — now routes through the locale table with the previous English
  strings kept as compiled fallbacks; 26 new keys
  (`error.*`, `confirm.clear_learnt.*`, `common.app_name`) shipped in
  `data/i18n/{en,bn}/ui.json` (parity now 160 keys, cross-check executed with
  0 code-referenced gaps). Dialogs raised by `initialize()` before the locale
  table loads keep English by a documented load-order contract (comment in
  code). Live rendering + MSVC compile of the Windows-only TU are on the
  Windows checklist.
- **Interface-language-change toast wired** (2026-11-22, BUG-03): a successful
  interface-language switch now shows the localized
  `toast.language_changed` message instead of the generic saved toast; the
  dead key `topbar.state.idle_dimmed` (no consumer — idle dimming is a pure
  alpha effect) was removed from both locales.
- **UI/UX functional, interaction, GUI-automation and E2E testing audit**
  (2026-11-22): a 105-row traceability matrix
  (`docs/reports/UI_FUNCTIONAL_AUDIT_TRACEABILITY_MATRIX.csv`) covering all 11
  surfaces and 84 interactive elements with handler/settings-key/UIA evidence;
  nine audit reports (`docs/reports/UI_FUNCTIONAL_AUDIT_01..09_*.md`); and a
  Windows runtime harness (`scripts/windows-ui-audit/`: a dependency-free
  PowerShell UIA script + a manual Narrator/DPI/HC/tray protocol) that converts
  the 84 Windows-desktop-blocked checks into executed evidence on a real machine.
- Topbar zone tooltips localized: the four `topbar.tooltip.*` keys referenced by
  `topbar.cpp` were missing from both locale files (compiled English fallback
  always rendered); added to `data/i18n/{en,bn}/ui.json` (parity now 135 keys).

### Fixed

- **Windows app build broken by duplicated UIA member definitions**: `application.cpp`
  defined `control_automation_id()`, `action_automation_name()` and
  `kSidebarAutomationIds` twice each inside `Application` (ill-formed C++, MSVC
  C2084) — the second verbatim copy is removed. Found by the functional audit's
  duplicate-definition scan; the Linux sandbox pipeline never compiles the
  Windows-only TU, which is why it had gone unnoticed.

- **UI Automation coverage completed for the two remaining surfaces**: the
  popup-menu component (every dropdown, font picker and context menu) now
  exposes each enabled item as a list item with name, checked/radio state,
  screen rectangle and the Invoke pattern — highlight changes (arrows and
  hover alike) raise focus-changed events so Narrator follows the menu — and
  carries the localized window name "Suborno menu" instead of a `nullptr`
  title. Toast notifications expose their message as an invokable element
  (Invoke dismisses, the same action as clicking) and carry the localized
  name "Suborno notification" instead of the hardcoded English "Suborno".
  Two new locale keys (`common.menu_popup`, `common.notification`) added to
  both `data/i18n/{en,bn}/ui.json` in parallel. With this pass every
  custom-drawn surface in the shell — settings, About, topbar, OSK, popup
  menu, toast — is UIA-visible; the TSF candidate window remains protected
  (name fallback only, by constraint).

### Fixed

- The popup-menu window was created with a `nullptr` title and the toast
  window with a hardcoded English title — both are now localized through the
  `ui::text` fallback contract.

### Changed

- **Bangla interface (i18n loader)**: `data/i18n/{en,bn}/ui.json` existed in
  the repository but no code loaded them (the audit's top "not present"
  item — the UI was English-only by code). A portable loader
  (`suborno::core::ui_strings`, unit-tested on every platform) now parses
  the locale files and every user-visible string on every surface (settings
  window, topbar and tray menus, About dialog, OSK, status line, toasts)
  renders through it, with the compiled English text as the permanent
  per-key fallback (a missing file or key can never blank the UI). A new
  Settings → Interface → "Interface language" dropdown (`ui.language`:
  system / English / বাংলা, additive settings key) switches the language at
  runtime; "system" follows the Windows display language. The installer
  ships `data/i18n` next to the executable.
- **UI Automation providers for the owner-drawn shell** (the audit's largest
  open accessibility item): the settings window, About dialog, topbar and
  on-screen keyboard now expose their content through
  `IRawElementProviderSimple` + `IRawElementProviderFragment(Root)` with
  real names, roles, states, screen rectangles and the Invoke / Toggle /
  ExpandCollapse / RangeValue patterns — Narrator can read and operate
  every control, including all 34 OSK keys, instead of seeing one opaque
  window. Entry points are resolved dynamically from uiautomationcore.dll,
  so systems without UIA keep the default HWND fallback. Providers hold the
  window's liveness token and answer UIA_E_ELEMENTNOTAVAILABLE after
  destruction. The TSF candidate window keeps its protected COM surface
  untouched (its window name remains the fallback).
- **Themed toast notifications** (`ToastWindow`): layout switches, font
  changes, encoding toggles, enable/disable, settings saved and learnt-words
  cleared are now announced with a small frameless, theme-aware popup near
  the notification area (success/information/warning tokens + words, never
  color alone). It never takes keyboard focus (WS_EX_NOACTIVATE) so typing
  and TSF composition are never interrupted; errors keep native MessageBoxes
  (documented decision). The audit's inventory listed toasts as missing.
- **Per-Monitor v2 DPI reflow**: the process now declares
  PER_MONITOR_AWARE_V2 (with the previous system-aware fallbacks retained),
  and the settings window + About dialog handle `WM_DPICHANGED` — refit the
  design DPI to the new monitor's work area, recreate fonts, re-cut the
  rounded region and move to the suggested rectangle. Previously the shell
  was system-DPI aware, so cross-monitor moves bitmap-stretched and the
  topbar/OSK handlers never even fired.

### Fixed

- **Live High Contrast / theme switching**: all four shell surfaces handle
  `WM_THEMECHANGED` and re-derive the shared palette through
  `ui::refresh_active_theme()` — turning High Contrast on or off now
  repaints every live window immediately (previously the palette refreshed
  only on the next theme save/startup).

### Fixed

- **Fixed-layout windows overflowed short screens at high DPI** ("responsive"
  behavior of the native shell). The settings window (736x620 design px)
  scaled purely by system DPI: 920x775 device px on a 1366x768 laptop at
  125% and 1472x1240 on 1080p at 200% — in both cases taller than the work
  area, pushing Save/Cancel off-screen. `ui::dpi_fitting()` now caps the
  design DPI (92% work-area margin, floor 96 DPI) for the settings window
  and the About dialog, so every control stays reachable at 100/125/150/200%.
- **Slider mouse-drag mapped the whole track wrong** (settings → Topbar:
  "Idle Opacity", "Idle Time"). The drag handler clamped `track_left + x`
  where `x` is a whole-window client coordinate, double-counting the track
  origin — every press anywhere on the visible track saturated the value at
  the maximum. The handler now clamps `x` itself, so the value follows the
  pointer position across the track (keyboard behavior was already correct).
- **Dark-theme secondary text met the AA floor only on paper**: `text_inactive`
  `#8C8C8C` on the measured `#2C2C2C` surface recomputes to 4.15:1, below the
  documented 4.5:1. It is now `#969696` (4.72:1), keeping the same visual
  weight class. All other token pairs re-verified (14/14 pass).
- **About dialog keyboard access**: Tab/Shift+Tab cycle the five actions
  (Check for Updates, Documentation, License, Notices, Close) with a visible
  focus ring; Enter/Space activate the focused action and Esc still closes —
  previously the dialog answered only Esc and was mouse-only.
- **OSK now honors the configured Bangla UI font** and refreshes when the
  settings font picker changes (`OskWindow::refresh_font()`); its rounded
  region is re-cut after a DPI change (previously the corners clipped on a
  DPI transition).

### Changed

- **Topbar hover feedback**: the four click zones (mode icon, layout label,
  wrench, close) now show a hover pill — the right-edge zones were invisible
  until clicked — and the 500 ms idle timer skips redundant
  `SetLayeredWindowAttributes` calls when the alpha is unchanged.
- **Candidate window border follows the theme**: the system-drawn `WS_BORDER`
  was replaced by a 1 px frame from `theme_separator()`; dark/light/High
  Contrast palettes now control the whole window edge.
- **Popup menu readability**: width 200 → 240 design px and item labels gain
  `DT_END_ELLIPSIS`, so long Bangla-capable font family names no longer run
  unmarked into the edge.
- **Control heights unified**: page buttons (`ui::metrics::kButtonHeight`)
  28 → 34 design px, matching dropdowns and the Save/Cancel footer ("control
  heights move together").
- **Accessible window names**: the settings window, About dialog, topbar and
  on-screen keyboard now carry real window names so the default UIA fallback
  announces them instead of an empty frame (the candidate window already had
  one).
- **Region-free `fill_rounded`**: rounded fills use a NULL-pen `RoundRect`
  instead of allocating a GDI region per call (10–30 fills per paint on the
  busier surfaces), with a corner-radius clamp for tiny rectangles.
- **Design-token system expanded** (`ui_theme.hpp`): the two-palette token
  set now covers interaction states (hover/pressed), text-bearing accent
  fills (`accent_fill` + `on_accent`, chosen per theme to keep >= 4.5:1
  contrast with their text), control borders, keyboard focus rings, status
  colors (success/warning) and the topbar brand glyph (`accent_warm`). All
  custom surfaces (settings window, topbar, OSK, menus, candidate window)
  read tokens only — the last hardcoded RGB values in OSK/candidate/topbar
  are gone. WCAG-AA contrast floors are computed for both themes.
- **Windows High Contrast support**: when an HC scheme is active the shell
  renders every custom surface from the live system colors (window/text/
  highlight), instead of ignoring accessibility schemes. The saved theme is
  untouched and resumes when HC is turned off.
- **System theme option**: Settings → Interface → Appearance offers
  "System (Windows)" (`theme_id=system`), which follows the Windows
  personalization app-mode preference at startup and on save.
- **Settings window interaction & visual polish**: Save is now a real
  primary button (accent fill, hover/pressed/focus faces) while Cancel stays
  neutral; press-and-release semantics for both footer buttons (dragging off
  cancels); Save/Cancel are reachable with Tab/Shift+Tab (with visible focus
  rings) and Enter/Esc activate the focused button; Ctrl+Tab /
  Ctrl+Shift+Tab cycle pages; sliders gain PageUp/PageDown/Home/End and an
  accent-filled track; checkboxes and dropdowns gain hover states and
  border/checked tokens; every section header is set on the title scale
  with an inline description caption; the status line is color-coded
  success/warning next to its words; the window title uses the new heading
  scale (20px semibold).
- **Menu popup hover fix**: the highlight pill no longer sticks to the last
  row after the pointer leaves the menu (WM_MOUSELEAVE).

### Added

- **Full Borno-parity feature set** (every remaining recommendation from the
  13-point feature analysis, all with real backend behavior — no display-only
  options):
  - **English↔বাংলা quick mode toggle**: the topbar mode icon now shows the
    live mode ("A" / ব) and clicking it swaps between the English passthrough
    layout and the last Bangla layout (new persisted `last_bangla_layout_id`
    setting); the same toggle lives on the tray, in the on-screen keyboard
    mode buttons, and survives restarts. The TSF open/close compartment
    (GUID_COMPARTMENT_KEYBOARD_OPENCLOSE) mirrors the mode into the Windows
    input indicator.
  - **On-Screen Keyboard** (new window, tray/topbar/wrench-menu entry):
    clickable keys inject the real virtual keys via SendInput so typing runs
    through the full engine path (transliteration, candidates, learning);
    Bangla mode renders each key with the glyph the ACTIVE layout produces
    (Engine::layout_preview), Shift works, and the [English]/[বাংলা] buttons
    switch layouts.
  - **Topbar right-click context menu** (Borno F-06): the full menu — Show
    Topbar, Keyboard Layout ›, Font ›, On-Screen Keyboard…, Settings…,
    Enable/Disable, Check for Updates…, Documentation, Clear Learnt Words…,
    Uninstall Suborno, Exit — shared with the tray menu.
  - **Candidate window presentation options** (Interface tab): orientation
    (vertical list ↔ horizontal row), fixed height (five-row paging) vs
    auto-height, sliding effect (timer-driven slide-in), follow-caret /
    follow-mouse fallback (off = pinned to the work-area corner) — all
    implemented in CandidateListUIElement, which now also paints with the
    active theme.
  - **Next Word Prediction**: a real bigram transition model — committed
    word pairs are stored in the user dictionary (additive, schema-v4
    compatible `word_transitions` table), and while typing the next word,
    words that historically followed the previous one are boosted into the
    candidate list ("next-word prediction"). Off by default; the Interface
    tab exposes it.
  - **Per-application-window layout switching** (General tab): the text
    service keeps a per-context layout memory — switching layouts while
    typing in one window no longer changes the layout of other windows;
    Global mode restores the previous behavior.
  - **Bijoy ANSI output encoding** (Input tab): committed Bangla is
    re-encoded into the legacy Bijoy ASCII byte set (clean-room
    `unicode::encode_bijoy`, full vowel-sign/reph/conjunct handling, Unicode
    fallback for unencodable graphemes) for legacy DTP workflows;
    Ctrl+Alt+E toggles it live (disable via the Input tab checkbox).
  - **bn-BD locale option** (Input tab): when enabled, the TSF text service
    re-activates the Suborno bn-BD language profile on the typing thread
    whenever a Bangla layout becomes active (e.g. after Alt+Shift left it).
  - **Theme system** (Interface tab): Suborno Dark (the measured reference
    palette) and Suborno Light; every custom surface (settings, topbar,
    menus, About, OSK, candidate window) reads the live palette, so the
    switch applies everywhere immediately and survives restarts.
  - **Learning controls**: Settings → Input gains the learning toggle and a
    destructive "Clear Learnt Words…" action (confirmation-gated) that wipes
    user words, user frequencies and word-pair history; also in the tray and
    context menus.
  - **About dialog**: new "Check for Updates" and "Documentation" buttons
    (real project URLs, error-reported failures); the installer now ships
    LICENSE and README.md so the license buttons work offline.
- `Engine` additions: `create_session_for_layout`, `layout_kind_of`,
  `layout_preview`, `encode_text_for_output` — all unit-tested.

### Added

- **English passthrough layout** (`english`): a first-class, persistable
  layout id whose keys the TSF text service never intercepts — switching to
  it from the topbar, tray, settings, or the global hotkey types plain Latin
  text immediately. Displayed as "English", first in every layout menu.
- **Font selection everywhere** (previously tray-only in spirit): the
  effective font is now configurable from Settings → Interface ("App &
  Candidate Font" dropdown), the topbar wrench menu ("Change Font…"), and a
  new tray "Font" submenu. The chosen family applies to the app UI, the
  floating topbar, the menus, and the TSF candidate window immediately, and
  survives restarts. Bangla-capable installed families are enumerated via
  GDI; "OS Default" is always available.
- **Start-mode launch behavior**: the stored `start_mode` is now honored —
  topbar, system tray, settings window, or last state (`last_ui_mode` is
  persisted whenever topbar visibility changes). Relaunching the app
  (desktop icon double-click / second instance) now reveals the floating
  topbar — the main UI — instead of reopening the settings window.
- **Topbar position memory**: "Remember Topbar Position" is functional —
  the pill's dragged position is saved (`topbar_position_x/y`) and restored
  on launch.
- **Layout switching shortcut**: Settings → Input offers the shortcuts the
  shell can actually register (F1–F4, F10, F12, Ctrl+Space) as a global
  hotkey that cycles layouts instantly.
- Settings → Interface "Use OS Default Bangla Font" toggle wired to the
  effective-font logic; the candidate window now renders with the
  configured font family.

### Changed

- Layout names shown to users no longer contain the word "Phonetic":
  English, বাংলা (Suborno), বাংলা (Bornো), Avro, Khipro, Probhat, National.
- The floating topbar auto-sizes to its label (long Bangla layout names no
  longer truncate) and its wrench/close menus are right-anchored.
- The topbar close (×) dropdown now reads "System Tray" and "Exit" and both
  actions are fully wired (minimize to tray without quitting / full quit).
- Settings persistence extended (still version 2, backward compatible):
  `font_family`, `last_ui_mode`, `topbar_position_x`, `topbar_position_y`.

### Fixed

- **Instant layout switching while typing**: the TSF text service now
  change-detects the settings file on every keystroke test, so a layout
  switch made in the desktop shell takes effect on the very next keystroke —
  no more English letters after switching layouts, and no need to refocus
  the target application.
- **General tab overlap bug**: "Remember Last Input Layout" and "Start with
  Windows" no longer render on top of each other.
- Idle Time slider clamps its display into the track range.
- Candidate window constructor keeps the configured font family across DPI
  changes; the Windows build smoke test was updated accordingly.

### Added

- `run_tests` convenience target: `cmake --build --preset
  windows-msvc-release --target run_tests` builds every test executable
  (plus the app the smoke test needs) and runs ctest with the same
  configuration (`-C $<CONFIG>` on multi-config generators). Previously a
  Release build tree tested with `ctest -C Debug` reported the windows
  tests "Not Run" — and silently picked up stale Debug exes for the
  unit/integration tests. Verified end-to-end on single-config and
  multi-config generators (Ninja, Ninja Multi-Config).
- Folder-structure audit #2 follow-through (evidence-based, 12-project
  industry set):
  - `tests/fuzz/corpus/` (14 versioned seeds: valid romanization, Bengali
    UTF-8, invalid UTF-8, punctuation/case/conjunct stressors) and
    `tests/fuzz/dictionaries/phonetic.dict` (47-token libFuzzer
    dictionary); the CI fuzz smoke and the new nightly session now run over
    them instead of inline `printf` seeds.
  - `data/i18n/{en,bn}/ui.json` localized UI-string resources with a
    documented key contract (`data/i18n/README.md`).
  - `docs/architecture/layering.md` — normative module layering contract
    for the measured dependency graph, enforced by the new
    `scripts/check-layering.py` (19 edges, currently green).
  - `docs/api/stability.md` — three-tier public API stability policy for
    `include/suborno/`.
  - `.github/workflows/fuzz.yml` (nightly capped libFuzzer session with
    crash-artifact upload) and `.github/workflows/bench.yml` (weekly
    Google Benchmark run with JSON archive), plus the `ubuntu-bench`
    CMake preset (`SUBORNO_BUILD_BENCHMARKS=ON` over the ubuntu-ci base).
  - `scripts/README.md` — tooling inventory and naming conventions.
- Borno-reference dark shell (clean-room implementation from the supplied
  reference screenshots; see `docs/research/BORNOTSF_UI_UX_REFERENCE_ANALYSIS.md`
  and `docs/design/SUBORNO_REFERENCE_DESIGN_SPEC.md`):
  - The desktop settings window is now a frameless, custom-drawn dark
    dialog (#2C2C2C measured palette, rounded corners, custom title row)
    with a sidebar (General / Interface / Topbar / Input), styled
    checkboxes, dropdowns and sliders, and Save/Cancel (staged edits;
    Esc/Cancel discards, Enter/Save commits with rollback on failure).
    Pages expose only settings with implemented runtime consumers, per the
    desktop-settings-schema contract ("a persisted option must not be
    represented in the UI as operational until its runtime consumer is
    implemented").
  - New floating topbar pill (mode icon + active layout label, wrench and
    close zones): mode click opens a keyboard-layout menu, wrench opens
    Settings…/About Suborno…, close opens Switch to Tray/Exit Suborno;
    draggable, idle dimming driven by the real topbar settings
    (idle_dimming_enabled / idle_opacity_percent / idle_time_seconds).
  - New About dialog (identity block, version, License (MIT) and Open
    Source Notices buttons that open the real LICENSE and
    THIRD_PARTY_NOTICES.md files).
  - New shared dark popup menu component (radio dots, checkmarks,
    separators, hover pills) used by the topbar and dropdowns.
- System tray menu restructured to the reference order: Show Topbar
  (checkable), Keyboard Layout ›, then Settings…, Enable/Disable,
  Uninstall Suborno, Exit (Enable/Disable and Uninstall preserved from
  the previous menu).

### Fixed

- MSVC Release build of `suborno_app` failed with C2064 in
  `topbar.cpp` (`WM_PAINT`): the local `PAINTSTRUCT paint` shadowed the
  file-local `paint()` helper, so the repaint call stopped compiling and the
  installer packed a stale `Suborno.exe`. Renamed the locals in
  `topbar.cpp` and the identical landmine in `menu_popup.cpp`; the exe now
  builds and carries the current icon resource.
- Idle-dimming timing in `topbar.cpp` now uses `GetTickCount64` (MSVC
  /analyze C28159): the 32-bit `GetTickCount` wraps after 49 days of uptime,
  which would corrupt the idle calculation on long-running systems.
- Keyboard-only navigation in the desktop settings window now works: the
  message loop drives dialog-style keyboard UI (`IsDialogMessageW`) and every
  interactive control carries `WS_TABSTOP`. Previously Tab did nothing — a
  blocker for keyboard-only and screen-reader users.
- The settings window uses the per-user Windows message font (Segoe UI via
  `NONCLIENTMETRICS`) instead of the legacy `DEFAULT_GUI_FONT` stock object,
  which rendered controls in the dated MS Shell Dlg look.
- The desktop application declares system DPI awareness and lays out the
  settings window from 96-DPI design pixels scaled to the system DPI, so
  125–200% display scaling shows a crisp, proportionally sized window instead
  of a bitmap-stretched small one. Best effort: systems where the API is
  unavailable keep the previous behavior.
- The settings window now enforces its design size as the minimum tracking
  size; shrinking below it used to clip the absolutely-positioned controls.
- The typing-help line documents the keyboard affordances the engine already
  provides: Space or Enter commits, 1–9 selects, Up/Down chooses, Page
  Up/Down pages, Esc cancels.
- Long Bengali candidates in the TSF candidate window end with an ellipsis
  (`DT_END_ELLIPSIS`) instead of being clipped mid-glyph.
- The candidate window is clamped to the monitor work area near the right
  screen edge (it used to clip off the display), and its flip-above-caret
  calculation now uses the same DPI scaling as the font it creates.

### Changed

- The settings window grants slightly more room to the two-line typing-help
  text (44 design px instead of 38) with the buttons shifted accordingly;
  no control was removed or reordered.

- Settings initialization on Windows no longer dies with an opaque
  "could not load its settings. Error: 0x80070003" dialog on first launch:
  - First launch now persists the default settings, so `settings.ini` exists
    under `%APPDATA%\Suborno` from the first run on and user changes persist
    across restarts. Previously the file was only written when a setting was
    changed in the UI.
  - Directory creation under the user's RoamingAppData self-heals when a
    path component is occupied by a regular file (the classic producer of
    `ERROR_PATH_NOT_FOUND` during `create_directories`): the impostor file is
    moved beside itself under a `.SubornoConflicted-<pid>` name — contents
    preserved, nothing deleted — and creation is retried.
  - An unreadable or unsupported-version settings file is quarantined
    (`suborno::persistence::quarantine_file`, `.corrupt-<pid>` name, contents
    preserved) and the application recovers with validated defaults instead
    of failing to start.
  - Settings-initialization failures now name the exact operation, the path
    involved, the HRESULT and the native Win32 error code (dialog and
    `OutputDebugStringW`); the old dialog printed only an HRESULT, which made
    production reports like 0x80070003 undiagnosable.
  - `SHGetKnownFolderPath` reporting success with an empty string now fails
    explicitly instead of silently turning "Suborno" into a CWD-relative
    path.
  - The installer's post-install "Launch Suborno" step uses
    `runasoriginaluser`, so the first run executes in the end user's own
    (non-elevated) context and creates the settings tree under the real
    user's profile — not the elevated installer context.
- Windows TSF build now compiles: the `EngineSettings::interface` member was
  renamed to `ui` — Windows COM headers `#define interface struct`, which broke
  every translation unit that included both `<objbase.h>` and
  `suborno/settings/settings.hpp` (errors C2236/C2332/C2027 in
  `settings.hpp`, `application.cpp`, `text_service.cpp`).
- `app_paths.cpp`: the anonymous-namespace helper `create_directory` is now
  `ensure_directory`, removing the C2668 ambiguity with
  `std::filesystem::create_directory` found via argument-dependent lookup.
- `candidate_list_ui_element.hpp`: the candidate window implements only
  `ITfCandidateListUIElementBehavior` (which already derives from
  `ITfCandidateListUIElement`). The duplicate base class triggered C4584 and
  made the `QueryInterface` casts ambiguous (C2594).
- `text_service.cpp`: `repeat_count` now uses an explicit
  `std::uint16_t{1}` literal, fixing the narrowing conversion error C2397.
- `candidate_list_ui_element.cpp`: SDK cursor resource macros expand to ANSI
  types unless `UNICODE` is defined; `UNICODE`/`_UNICODE` are now defined for
  every MSVC target, fixing the C2664 `LoadCursorW` error.
- `personal_dictionary_test.cpp`: replaced a multi-character literal
  (`'\xe0\xa6\x86'`) with an explicit single-byte filler, removing the
  C4305/C4309 truncation warnings.
- MSVC code analysis stays enabled on test targets (as on production code).
  The single exception is rule C6326, scoped off per test target through a
  documented forced-include header (`cmake/msvc-test-c6326-suppress.h`):
  googletest 1.18's own assertion helper performs a bool == bool comparison
  inside `AssertionResultExpectation::operator bool()`, which the MSVC
  analyzer reports as constant-vs-constant C6326 on every EXPECT_*/ASSERT_*
  expansion line (~1000 false positives per build; googletest#471,
  Microsoft Developer Community #10978073). Every other analysis rule and
  every compiler warning remains active on test code.

### Changed

- Build restructured to per-directory `CMakeLists.txt` files
  (`src/unicode`, `src/layout-api`, `src/dictionary`, `src/spell`,
  `src/candidates`, `src/persistence`, `src/settings`, `src/layouts/*`,
  `src/core`, `src/platform/*`, `src/tools`, `tests`, `benchmarks`,
  `examples`). The root file now only carries options, dependency discovery
  and `add_subdirectory` calls (LLVM/systemd per-component layout).
- Build outputs are pinned to the top-level build directory
  (`CMAKE_RUNTIME_OUTPUT_DIRECTORY` and friends) so the packaging contract
  keeps working with the per-directory build files:
  `packaging/windows/suborno_installer.iss` reads
  `{#MyBuildDir}\Release\Suborno.exe`, `{#MyBuildDir}\Release\
  suborno_windows_tsf.dll`, `{#MyBuildDir}\suborno.ico` and
  `{#MyBuildDir}\suborno_version.iss.inc`; `.github/workflows/release.yml`
  reads the version file from the same root.
- Repository hygiene: removed the tracked `core` crash-dump artifact from the
  repository root and added `/core`, `/core.*` and `vgcore.*` to `.gitignore`.
- Structure consolidation per the folder-structure analysis and its
  implementation note (`docs/reports/FOLDER_STRUCTURE_IMPLEMENTATION.md`):
  the Windows InnoSetup script moved from `installer/windows/` to
  `packaging/windows/`, `docs/environment/ubuntu.md` merged into
  `docs/development/ubuntu-environment.md`, the minimal-ICU POC workflow
  archived to `.github/archive/` (still manually runnable via the
  `windows-msvc-minimal-icu-poc` preset), `PRODUCTION_READY.md` relocated
  to `docs/reports/`, and a root `.gitattributes` plus
  `.github/ISSUE_TEMPLATE/` (bug + feature) added.

### Added

- `CHANGELOG.md` (this file) and `MAINTAINERS.md` for release governance and
  module ownership routing.
- `packaging/debian/` — minimal deb packaging set (control, rules, changelog,
  copyright, install) so Linux users can install instead of source-building.
- `packaging/appstream/org.subornoIME.metainfo.xml` — AppStream metadata
  required for GNOME/Ubuntu Software listing.
- `packaging/README.md` — packaging map; the Windows InnoSetup script now
  lives in `packaging/windows/` (consolidated from `installer/windows/`) and
  is referenced from here.
- `examples/c_api_example.c` and `examples/engine_demo.c` — compilable C-API
  integration examples; both targets build in CI.
- `docs/api/Doxyfile.in` — Doxygen configuration for the 36 public headers.
- `benchmarks/dictionary_benchmark.cpp` and `benchmarks/engine_benchmark.cpp`
  — named benchmark suites for the dictionary and engine hot paths.
- `docs/decisions/ADR-0003-repository-governance.md` — records the
  governance, hygiene and packaging decisions.
- Project icon artwork in every place it renders: `suborno.ico` regenerated
  from the logo at 16/24/32/48/64/128/256 px (Windows exe and dll resource,
  tray, About and settings windows, InnoSetup installer via the existing
  build-dir contract), hicolor PNGs 16–512 px installed by the Debian
  package, an AppStream `<icon>` entry for software-center listings, a
  README logo, and `scripts/generate_icon_assets.py` to regenerate
  everything from one square source image.

### Fixed

- MSVC release build of the Windows shell target (`suborno_app`) — the
  dark-shell UI sources had never been compiled under MSVC and produced 22
  errors, all resolved: pointer/member typos (`AboutState` icon access,
  `font_message_` → `message_font_`), a `release()` call on a `const`
  `unique_ptr` in `show_about_dialog()`, `std::max` type ambiguity between
  `LONG` and `int` in the settings-window centering (`0` → `0L`),
  `min`/`max` macro collisions with `windows.h` in `menu_popup.cpp` and
  `topbar.cpp` (NOMINMAX is defined centrally for every MSVC target in
  `suborno_apply_build_options`, so application.cpp, menu_popup.cpp,
  topbar.cpp and candidate_list_ui_element.cpp all keep bare
  `std::min`/`std::max` calls), and nine C2248
  private-access errors on `Topbar::Impl` from TU-local helpers (the
  incomplete nested struct is now public in `topbar.hpp`).
- SAL annotation inconsistency warnings (C28252/C28253) for
  `DllCanUnloadNow` in `tsf/dll_main.cpp` and
  `tests/windows/windows_build_smoke_test.cpp`: both now carry the SDK's
  `__control_entrypoint(DllExport)` annotation form.
- LNK4104 warnings for the five `Dll*` def-file exports: kept non-PRIVATE
  because `suborno_windows_tests` imports them directly, and silenced with
  `/IGNORE:4104` on the `suborno_windows_tsf` target instead.

## [0.1.0]

### Added

- Portable Bengali input core: phonetic (Suborno/Borno/Avro), Probhat,
  Khipro and national keyboard layouts, candidate ranking, spell correction,
  SQLite-backed personal dictionary and settings persistence.
- C API (`include/suborno/suborno.h`) for external embedding.
- Windows production targets: TSF text-service DLL and settings application
  with InnoSetup installer.
- Linux/macOS platform adapters, CI workflows, word-list data modules and
  seed dictionary.

[Unreleased]: https://github.com/subornoime/subornoime/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/subornoime/subornoime/releases/tag/v0.1.0
