# Installer and Upgrade Verification Report

Status: **`WINDOWS TEST SCRIPT — INSPECTED`** (300-line lifecycle suite +
450-line Inno Setup config audited line-by-line); **execution BLOCKED**
locally — `pwsh` is absent in this sandbox so even PowerShell syntax
validation could not run here; the suite executes for real inside the
verification workflow.

## Audited artifacts

| Artifact | Lines | Content |
|---|---|---|
| `packaging/windows/suborno_installer.iss` | 450 | Two AppIds (`9FF345F9-…` current, `9960C85A-…` legacy) — upgrade path; x64 mode; ProgramData TSF service deployment (x64+x86); shared resources; shortcuts; uninstaller |
| `scripts/test-windows-installer.ps1` | 300 | Automated lifecycle assertions (below) |

## The lifecycle suite's actual assertions (source-audited)

1. `Invoke-SubornoSetup` — silent install.
2. `Assert-CleanInstallDirectory` — install dir contents exact.
3. `Assert-ServiceDeployment` — `ProgramData/components/tsf/x64` contains
   **only** `suborno_windows_tsf.dll` (no stray files); x86 variant too.
4. Legacy service copy to the install dir + **silent register via `/s`**
   (real `DllRegisterServer` execution).
5. `Assert-SharedResourceDirectories` — ProgramData layout, no duplication.
6. Desktop / Start-menu / quick-launch shortcut target assertions.
7. `Assert-UserDataPreserved` — user dictionary + learnt data survive the
   **upgrade** (legacy-AppId install over existing data).
8. `Assert-UserDataInitialized` — first-run state.

## Mission scenario mapping

| Scenario | Suite coverage | Status |
|---|---|---|
| Clean installation | 1–2 | BLOCKED (runner executes) |
| Install dir + app files + DLL availability | 2–3 | BLOCKED |
| Registration checks | 4 (+ `tsf_registration_report.md`) | BLOCKED |
| Upgrade install | 7 (legacy AppId path) | BLOCKED |
| User configuration preservation | 7 | BLOCKED |
| Uninstallation + cleanup | uninstaller in `.iss`; teardown steps in the suite | BLOCKED |
| Signature verification | `Get-AuthenticodeSignature` gate accepts `Valid`/`NotSigned`; **no signing certificate exists** — the installer is honestly recorded as NotSigned; never published as signed | N/A until a cert exists |

## Non-claims

No install, upgrade, registry modification or DLL runtime behavior is
claimed from this session. The existing CI `windows-msvc` job already runs
this suite on GitHub (historical evidence, not re-verified here); the
verification workflow reproduces it with captured logs
(`installer_test.log`).
