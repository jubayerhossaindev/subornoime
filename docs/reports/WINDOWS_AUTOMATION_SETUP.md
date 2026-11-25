# WINDOWS_AUTOMATION_SETUP

Project: SubornoIME · Date: 2026-11-22 · Companion: `WINDOWS_UI_FUNCTIONAL_AUDIT.md`, `BANGLA_LAYOUT_TYPING_AUDIT.md`

## 1. Where this audit ran — honest environment statement

The full audit (brief Sections 1–20) was prepared and executed in a **Linux sandbox (Debian 13 trixie, x64)**. There is **no Windows desktop, no MSVC, no TSF host, no UIA runtime, no screen, no audio**. Everything that physically requires Windows is reported **BLOCKED** with the exact blocker recorded in `UI_FUNCTIONAL_AUDIT_TRACEABILITY_MATRIX.csv`; nothing was simulated into a PASS. Tool availability below is therefore split into *verified here* vs *install-on-Windows protocol*.

## 2. Tools verified in this environment

| Tool | Version | Location | Verified use |
|---|---|---|---|
| GCC | 14.2.0 | `/usr/bin/g++` | Build (parallelism 1), engine-level typing harness |
| CMake | 4.4.5 | system | Configure/build/test with project presets' cache variables |
| Ninja | 1.13.2 | system | Generator |
| clang-format | 23.1.5 | `/home/z/.venv/bin/clang-format` | Matches `.clang-format` (LLVM 23) — format gate |
| ICU | 76.1 | system | Unicode normalization evidence (canonical equivalence) |
| Python | 3.12.16 | system | Matrix counting, i18n parity, corpus analysis |
| GoogleTest | local provision | `/tmp/gtest-install` | 543 unit + 2 integration tests executed |

Not installable here (Windows-only): WinApp CLI, Accessibility Insights for Windows, Narrator, WinAppDriver, Visual Studio 2022/MSVC v143, Windows SDK, Notepad/Word/VS Code hosts.

## 3. Tool 1 — Microsoft WinApp CLI (Windows machine; install protocol)

```powershell
winget install Microsoft.winappcli --source winget
winapp -V
winapp --help
winapp ui --help
```

- WinApp CLI is **Public Preview**: never assume commands — read `--help` on the
  installed version first (brief rule). Verified against the official docs
  (`winappCli/docs/usage.md`, `winappCli/docs/ui-automation.md`, fetched
  2026-11-23): the UI-automation family exists as `winapp ui <verb>` with verbs
  `inspect`, `search`, `get-property`, `get-value`, `wait-for`, `set-value`,
  `invoke`, `focus`, `scroll`, `scroll-into-view`, `click`, `hover`, `drag`,
  `send-keys`, `scroll --wheel`, `touch`, `pen`, `screenshot`, `record`.
  **There is no `winapp ui status` verb** (the brief's example) — use
  `winapp ui inspect -a SubornoIME` for the equivalent app connection check. An
  unknown verb exits 1 and suggests the closest commands; every verb's `--help`
  shows examples; pass `--json` for structured output.
- Two verb classes (official doc): UIA-pattern verbs (`inspect`, `search`,
  `get-property`, `get-value`, `wait-for`, `set-value`, `invoke`,
  `scroll --direction`) are headless/locked-session friendly; input-injecting
  verbs (`click`, `hover`, `drag`, `touch`, `pen`, `send-keys --via send-input`,
  `scroll --wheel`) **need an unlocked interactive desktop with the target
  window in the foreground** and fail fast with `no_interactive_desktop`
  otherwise. Target apps with `-a <name>` or `-w <HWND>`; element selectors take
  optional `--root/--type/--class-name` filters.
- Compatibility gate: Windows 10 22H2 + WinApp CLI current build; if a needed
  verb is missing on the installed version, fall back to the .NET UIA client
  (already used by our orchestrator) and document the limitation.
- Planned usage (element tree FIRST, then real selectors — no guessed
  AutomationIds):

```powershell
winapp ui inspect    -a SubornoIME --interactive
winapp ui search     Button -a SubornoIME
winapp ui invoke     <selector> -a SubornoIME
winapp ui screenshot -a SubornoIME
winapp ui click      <selector> -a SubornoIME   # real mouse input; desktop must be unlocked
winapp ui send-keys  <selector> "text" -a SubornoIME --via send-input
```

Covers: window/UIA tree inspection, element discovery, Invoke, real mouse click,
hover, drag, dropdown (`set-value`/`invoke`), slider (`set-value`/RangeValue),
checkbox/toggle, menus, keyboard interaction (`send-keys`), focus navigation,
window state, screenshots, UI recording (`record`).

**Fallback already in place:** `scripts/windows-ui-audit/Invoke-SubornoUiAudit.ps1`
uses the .NET UIAutomationClient that ships with Windows — zero external
dependencies, exit code = failed-check count, PS 5.1-syntax-clean, with built-in
environment detection, per-window screenshots and a real-mouse-click check that
is recorded separately from UIA Invoke.

## 4. Tool 2 — Accessibility Insights for Windows

Install from <https://accessibilityinsights.io/downloads/>. Run **FastPass** (automated checks + Tab Stops) and **Live Inspect** per surface: Settings, About, Topbar, OSK, Popup Menu, Dropdown, Font Picker, Toast, TSF Candidate Window, Tray UI. Verify the 17 properties listed in brief Section 2 (Accessible Name, AutomationId, Control Type, Bounds, focus/enabled/offscreen/keyboard-focusable, Invoke/Toggle/Selection/Value/RangeValue/ExpandCollapse/Text patterns, window info, parent/child). Missing accessible name or required pattern = documented bug. FastPass green alone is not "accessibility complete".

## 5. Tool 3 — SendInput C++20 harness (delivered, engine-verified)

`layout_verification/windows-typing-harness/` (`Run-BanglaTypingTests.ps1` + `test-manifest.csv`, 295 cases) and `layout_verification/verify_main.cpp`/`verify_lib.cpp`:

- Sends real key down/up, modifiers, arrows, Backspace, Delete, Enter, Space, Tab via `SendInput()`; checks the injected-event count; releases pressed keys safely on failure; per-test timeout.
- **Never uses `KEYEVENTF_UNICODE` for phonetic layouts** — every layout is typed with its own mapping (fixtures generated per layout from the layout's real tables).
- UIPI note: keep sender/target integrity levels compatible.
- The engine-side of every fixture was executed and verified at code-point level in this audit (see `BANGLA_LAYOUT_TYPING_AUDIT.md`); the Windows-side execution remains BLOCKED here.

## 6. Tool 4 — Windows Narrator

Start with `Win+Ctrl+Enter` on the Windows machine; verify announcement of window names, button/menu item names, checked/selected/toggle states, focus changes, toast messages; confirm toasts do not steal typing focus. A tester listens to the real speech output; Speech Recap evidence preferred. Source-code UIA support alone is never counted as a Narrator PASS.

## 7. Tool 5 — CTest

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-msvc-release -j1
ctest --preset windows-msvc-release
```

On Linux the same suite ran as `ctest` + direct gtest binaries: **543 unit + 2 integration, all PASSED** (`windows-ui-audit/logs/`). Counts are verified from real output, not exit codes alone.

## 8. References (must-follow, checked for current-version compatibility on the Windows machine)

- WinApp CLI: learn.microsoft.com/en-us/windows/apps/dev-tools/winapp-cli/ · github.com/microsoft/winappCli
- Accessibility Insights: accessibilityinsights.io/docs/windows/overview/
- SendInput: learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput
- Windows accessibility testing: learn.microsoft.com/en-us/windows/apps/design/accessibility/accessibility-testing
- Narrator: support.microsoft.com complete guide
- CTest: cmake.org/cmake/help/latest/manual/ctest.1.html

## 9. Installation rules compliance

No tool was re-installed if present; no unnecessary dependency added; no production dependency list change; no UI-automation tool packaged into the product binary; no admin privilege used (none available, none needed); no system-wide configuration changed; every missing tool is explicitly reported as such (Sections 2–3).
