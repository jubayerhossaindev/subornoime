# Final Acceptance Matrix — Windows/GitHub Verification

Classifications (mission §17): `PASS` / `PARTIAL` / `FAIL` / `BLOCKED` /
`UNVERIFIED`. No Windows runtime PASS is awarded from Linux evidence.

| Area | Required evidence | Final status | Basis |
|---|---|---|---|
| Native MSVC v143 Release build | Actual Windows build | **BLOCKED** | no GitHub auth/remote (`environment.md`); workflow job ready |
| Native MSVC v143 Debug build | Actual Windows build | **BLOCKED** | same |
| Windows SDK integration | Actual build | **BLOCKED** | same |
| MSVC `/analyze` | Actual analyzer execution | **BLOCKED** | execution-proof gate wired (verbose compile-line check) |
| COM exception safety | Windows COM tests | **PARTIAL** | 78 guarded entry points counted in source (`com_safety_report.md`); runtime tests BLOCKED |
| COM activation | Actual activation tests | **BLOCKED** | `suborno.windows.tsf_contract` runs on the runner |
| TSF registration | Actual registration execution | **BLOCKED** | installer suite executes `DllRegisterServer` via `/s` on the runner |
| TSF edit sessions | Actual integration execution | **BLOCKED** | same target |
| Actual Bengali IME typing | Interactive Windows test | **UNVERIFIED** | needs interactive desktop + host editor (manual protocol) |
| Installer clean installation | Actual installer execution | **BLOCKED** | 300-line lifecycle suite audited; runs in workflow |
| Installer upgrade | Actual upgrade test | **BLOCKED** | legacy-AppId upgrade path in the suite |
| DLL exports | Actual PE export inspection | **PARTIAL** | `.def` audited (5 standard exports); `dumpbin` cross-check BLOCKED |
| Windows ABI comparison | Baseline vs v19 binaries | **UNVERIFIED** | v18 source not in the repository; strongest available comparison executed (header/def/GUID unchanged) |
| Fuzz smoke (Windows) | Actual execution | **BLOCKED** | portable driver wired; Linux fuzz smoke executed PASS |
| Windows UI automation | Actual UI runtime execution | **BLOCKED** | 84+14 check harness wired; source inventory complete |
| Part III regression | Actual test execution | **PARTIAL** | Linux: PASS (686 tests, soak, sanitizers; D-003 fix in flight); Windows: BLOCKED |
| Test-count reconciliation | Discovery + execution | **PASS (Linux)** / **BLOCKED (Windows)** | 34 unique cases; DiagnosticsTest = 7; report corrected |

## The single root blocker

**GitHub authentication + remote are absent in this sandbox** (exact
evidence: `logs/github_blocker_evidence.log` — `gh auth status` exit 1, empty
`git remote -v`, absent GH_TOKEN/GITHUB_TOKEN; gh-cli 2.105.0 itself
installed and working). Everything downstream of the push (branch → workflow
run → artifacts → this matrix going green) activates with:

```bash
git push -u origin ci/suborno-v19-windows-verification
gh workflow run suborno-windows-verification.yml --ref ci/suborno-v19-windows-verification
```
