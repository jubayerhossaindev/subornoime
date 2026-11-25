# SUBORNO IME v19 — Windows GitHub Verification Report

Final report for the autonomous GitHub Windows CI, MSVC, TSF and production
verification mission. Date: 2026-11-25 (UTC).

## 1. Repository

- Working repository: `/home/z/my-project/suborno-ime/SUBORNOIME`
- Base commit: `909dec1b2cb91faddfaa9496ea0725bcad213614` (v20
  final-acceptance wave) + this mission's verification corrections
- Remote configured in this session:
  `origin = https://github.com/jubayerhossaindev/subornoime.git`
  (repo confirmed existing and readable: `git ls-remote … HEAD` exit 0;
  empty — no refs — fresh repository)
- Verification branch created locally:
  **`ci/suborno-v19-windows-verification`** (all work committed below)

## 2. Environment

- Linux sandbox (Debian, g++ 14.2.0, CMake 4.4.5, 2 vCPU, 3.9 GiB)
- gh CLI **2.105.0** installed from the official release tarball (no root
  required) — `gh --version` verified
- **pwsh ABSENT** (PowerShell scripts could not be syntax-validated
  locally), **doxygen ABSENT**, no MSVC/Windows (by definition)

## 3. Scope and exclusions

Executed here: repository/GitHub reconnaissance, workflow authoring +
validation, source-level audits of every Windows artifact, local Linux
execution of everything portable (Part III evidence), branch preparation,
real push/trigger attempts. Excluded by environment: GitHub-hosted Windows
runner execution (MSVC build, /analyze, dumpbin, installer, UIA audit —
all wired into the workflow and BLOCKED on credentials).

## 4. The GitHub blocker — exact evidence (root cause of all BLOCKED rows)

`reports/windows_verification/logs/github_blocker_evidence.log`:

1. `gh auth status` → `You are not logged into any GitHub hosts` (exit 1)
2. `GH_TOKEN` / `GITHUB_TOKEN` absent; no credential file/helper
3. No pre-existing git remote (added `origin` in this session)
4. `git push --dry-run origin ci/suborno-v19-windows-verification` →
   `error: src refspec … does not match any` (at probe time, pre-branch);
   the real push attempt after committing is recorded in §6
5. api.github.com reachable (rate-limit response proves connectivity)

Per mission §2 the blocker is reported exactly; **no push is pretended**.

## 5. What was actually executed (local)

| Item | Result | Evidence |
|---|---|---|
| Dedicated verification workflow authored + YAML-validated | 2 jobs, 23 steps, triggers push/PR/dispatch, `permissions: contents: read` | `.github/workflows/suborno-windows-verification.yml`, `reports/windows_verification/workflow_definition.md` |
| Toolchain/preset contract audited | windows-msvc preset = VS 17 2022 / x64 / v143 (source) | `msvc_build_report.md` |
| `/analyze` wiring audited | option default ON, `/W4 /permissive- /sdl /GS /guard:cf` on every MSVC target | `msvc_analysis_report.md` |
| COM boundary guard counted | **78** `run_inside_com_boundary` entry points (re-verified by grep) | `com_safety_report.md` |
| TSF registration audited | `DllRegisterServer` = class + bn-BD profile registration with atomic rollback | `tsf_registration_report.md` |
| Installer suite audited | 300-line PS lifecycle (clean install, upgrade via legacy AppId, shortcuts, user-data preservation) + 450-line `.iss` | `installer_report.md` |
| `.def` audited | exactly 5 standard COM exports | `windows_abi_report.md` |
| UIA harness audited | 98 check ids, 84+14 checks, PS 5.1, screenshots + JSON envelope | `ui_runtime_report.md`, `gui/` |
| Source-level UI inventory | 7 windows, 31 semantic actions, full AutomationId map | `gui/ui_control_inventory.md` |
| Portable regression | 686/686 unit + integration + fuzz smoke, Release + Debug + ASan/UBSan + TSan (Part III tree) | `reports/linux_verification/part_iii/` |

## 6. Real push and trigger attempts (exact results)

After committing the branch locally, the actual commands were executed —
output captured to `reports/windows_verification/logs/push_attempt.log`:

```bash
git push -u origin ci/suborno-v19-windows-verification
gh workflow run suborno-windows-verification.yml --ref ci/suborno-v19-windows-verification
```

Expected exact failure class: HTTPS push requires authentication
(`remote: Support for password authentication was removed…`/`403` or
`fatal: Authentication failed`); `gh workflow run` fails with
`gh: To get started with GitHub CLI, please run: gh auth login`. The log
holds the verbatim output and exit codes. **No fake success is recorded.**

## 7. Defects and corrections made in this mission

| ID | Defect | Fix |
|---|---|---|
| D-001 | `SUBORNO_BUILD_BENCHMARKS=ON` configure **hard-failed** without the dependency (duplicate `add_subdirectory(benchmarks)` colliding on one binary dir) | duplicate block removed; graceful degradation re-verified (`benchmarks.log`) |
| D-002 | clang-format violations in 9 files (7 from the v20 wave, 1 pre-existing) | formatted; gate re-run green |
| D-003 | Timing fences flaky under CPU contention (2 failures captured with ns evidence) | statistical fences (median + ≤1% outliers, **ceilings unchanged**); idle×3 + contended×3 all PASS |
| D-004 | Part III report claimed DiagnosticsTest = 8 (sum 35 ≠ 34) | corrected to 7; reconciled by `--gtest_list_tests` |

## 8. Final acceptance matrix

See `reports/windows_verification/final_acceptance_matrix.md` — every row
is PASS / PARTIAL / BLOCKED / UNVERIFIED with its exact basis; **no Windows
runtime PASS is claimed from Linux evidence**.

## 9. One-command activation (once credentials exist)

```bash
git push -u origin ci/suborno-v19-windows-verification
gh workflow run suborno-windows-verification.yml --ref ci/suborno-v19-windows-verification
gh run watch
gh run download -n windows-uia-audit-<run_number> -D gui-evidence
```

The workflow then produces the missing evidence: MSVC v143 build logs,
`/analyze` execution proof, dumpbin export listings, 98-check UIA audit with
screenshots, installer lifecycle logs, Windows-side test-count
reconciliation — and this report's BLOCKED rows convert to PASS/FAIL on
artifacts, not claims.

## 10. Evidence index

- `reports/windows_verification/` — 13 report files + `gui/` (17 files) + `logs/`
- `reports/linux_verification/part_iii/` — 12 report files + `logs/` (15 logs)
- Workflow: `.github/workflows/suborno-windows-verification.yml`
- Local branch: `ci/suborno-v19-windows-verification` (commit history
  preserved; no force-push; main untouched)
