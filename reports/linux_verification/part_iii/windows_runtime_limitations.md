# Part III — Windows Runtime Limitations (Honest Reclassification)

Mission §10. Every Part III acceptance claim is reclassified below using the
required labels. **No Windows runtime behavior was verified from Linux** — the
sandbox has no Windows, no MSVC, no TSF, and `pwsh` is ABSENT (PowerShell
scripts could not even be syntax-validated locally; recorded as a blocker).

## Installer test script coverage (source inspection only)

`scripts/test-windows-installer.ps1` (300 lines) automates, on Windows:

1. Signature status check of the built installer.
2. Silent install via `Invoke-SubornoSetup`.
3. `Assert-CleanInstallDirectory` — install directory contents.
4. `Assert-ServiceDeployment` — TSF DLL deployed to
   `ProgramData/components/tsf/x64` (+x86), and **only** that DLL present.
5. `Assert-SharedResourceDirectories` — ProgramData resource layout; common
   files must NOT be duplicated.
6. Desktop / Start-menu / quick-launch shortcut targets.
7. `Assert-UserDataPreserved` — user dictionary + learnt data survive an
   upgrade (legacy-AppId copy + silent register via `/s`).
8. `Assert-UserDataInitialized` — first-run state.

`packaging/windows/suborno_installer.iss` (450 lines): two AppIds (upgrade
path), x64 mode, uninstaller, version metadata. `pwsh` absent → none of this
was executed or syntax-validated here; inspection was static only.

## Reclassification table

| Part III area | Classification | Evidence |
|---|---|---|
| TSF text service, COM plumbing, key routing (`src/platform/windows/tsf/`) | `WINDOWS SOURCE AUDIT — VERIFIED` | 78 COM methods boundary-guarded; structure scans, format, review; Windows smoke test compiles via CI |
| Actual TSF registration / text-service activation / real typing through TSF | `WINDOWS RUNTIME — UNVERIFIED` | Requires Windows + MSVC build |
| Settings, switching, tray, DPI, candidate-window rendering (window-level) | `WINDOWS RUNTIME — UNVERIFIED` | Unit-level logic (`settings_test`, `ui_functional_audit_test`, `ui_strings_test`) is `PORTABLE CORE — VERIFIED`; window behavior is not |
| Installer install / uninstall / upgrade / registry / DLL runtime | `WINDOWS INSTALLER RUNTIME — UNVERIFIED` | PS script inspected (`WINDOWS TEST SCRIPT — INSPECTED`); cannot run or syntax-check on Linux |
| Portable engine core (sessions, candidates, unicode, dictionaries, diagnostics) | `PORTABLE CORE — VERIFIED` | 686/686 unit + integration + fuzz smoke, Release/Debug/ASan/UBSan in this mission |

## Explicit non-claims (mission list)

The following were **NOT** verified on Linux and are not claimed anywhere in
Part III evidence: TSF registration; native text-service activation; real
application typing through TSF; Windows tray interaction; native DPI
behavior; Windows candidate-window rendering; actual installer installation;
upgrade execution; Windows registry modification; Windows DLL runtime
behavior.

## Blockers recorded

1. No Windows OS / MSVC toolchain in sandbox.
2. `pwsh` ABSENT → `test-windows-installer.ps1` and
   `audit-windows-dependencies.ps1` could not even be syntax-validated.
3. Doxygen ABSENT → API-doc target not built (affects no runtime claim).

## Verdict

Any Part III matrix row that depends on Windows runtime stays
`UNVERIFIED`/`PARTIAL` until the CI `windows-msvc` job and the PowerShell
installer test suite execute on Windows. This is the maximum defensible
position from Linux evidence.
