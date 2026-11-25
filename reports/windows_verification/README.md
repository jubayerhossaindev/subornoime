# Windows Verification Evidence

Status: **BLOCKED for remote execution** — the mission's GitHub phase could
not run because this sandbox has **no GitHub authentication and no git
remote** (exact evidence in `environment.md`). Everything that can be
executed locally has been executed; everything that requires a GitHub-hosted
Windows runner is prepared and encoded in a validated workflow so it runs
the moment a remote + token exist.

## What exists (executed locally)

| File | Content |
|---|---|
| `environment.md` | exact blocker evidence + gh-cli install proof |
| `workflow_definition.md` | the validated dedicated workflow walkthrough |
| `msvc_build_report.md` | preset/toolchain audit + BLOCKED runtime build |
| `msvc_analysis_report.md` | `/analyze` wiring audit + BLOCKED execution |
| `com_safety_report.md` | COM boundary-guard source verification (78 methods) |
| `tsf_registration_report.md` | registration source + installer-script audit |
| `tsf_runtime_report.md` | TSF runtime + interactive typing limitations |
| `installer_report.md` | installer/upgrade script audit (300-line PS suite) |
| `windows_abi_report.md` | `.def`/`SUBORNO_API` export audit + dumpbin plan |
| `fuzz_report.md` | portable fuzz smoke (Linux executed) + Windows wiring |
| `regression_report.md` | Part III suites (Linux executed) + Windows wiring |
| `ui_runtime_report.md` | UIA harness audit + BLOCKED runtime |
| `final_acceptance_matrix.md` | honest PASS/PARTIAL/BLOCKED/UNVERIFIED matrix |
| `gui/` | the GUI mission evidence tree (inventory executed; runtime BLOCKED) |
| `logs/` | local execution logs (gh-cli, YAML validation, blocker evidence) |

## One-command activation (once a remote + token exist)

```bash
git push -u origin ci/suborno-v19-windows-verification
gh workflow run suborno-windows-verification.yml --ref ci/suborno-v19-windows-verification
gh run watch   # monitor; artifacts: windows-verification-evidence-*, windows-uia-audit-*
```

The workflow then produces the actual Windows evidence this tree currently
cannot: MSVC v143 build logs, `/analyze` execution proof, `dumpbin` export
listings, the 84-check UIA audit results with screenshots, installer
lifecycle logs, and GoogleTest count reconciliation on Windows.
