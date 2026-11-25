# Workflow Definition — suborno-windows-verification.yml

File: `.github/workflows/suborno-windows-verification.yml`
YAML validation: parsed with PyYAML locally (structure assertions below all
passed). **BLOCKED**: not yet parsed by GitHub's service-side validator —
that requires the push that authentication currently prevents.

## Design (extends, does not duplicate, the existing CI)

The repository already runs a `windows-msvc` job in `.github/workflows/ci.yml`
(vcpkg pinned to `0df93a0`, presets, installer build + lifecycle test,
artifact upload). The verification workflow reuses the same conventions
(`actions/checkout@v7.0.1`, `actions/cache@v6.1.0`,
`actions/upload-artifact@v4.6.2`, the pinned vcpkg baseline) and adds the
mission-required evidence steps:

### Job 1 `windows-build-verify` (windows-2022, 120 min cap)

1. **Runner environment verification** — vswhere-located VS, toolset
   directory must be `14.3*` (v143) or the job fails, `cl` banner must contain
   `Optimizing Compiler`, cmake/msbuild/dumpbin presence, OS build, arch,
   memory → `verify-evidence/environment.json`.
2. **Preset contract assertions** — reads `CMakePresets.json` and fails unless
   `windows-msvc` pins generator `Visual Studio 17 2022`, architecture `x64`,
   toolset `v143` (mission §4 gate, executed not assumed).
3. Pinned vcpkg clone + bootstrap (same baseline as ci.yml).
4. `cmake --preset windows-msvc`; full Release build with captured log.
5. **`/analyze` execution proof** — verbose rebuild of `suborno_core`, grep
   compile lines for `/analyze`; job FAILS if the flag is absent (mission §5:
   successful compilation is not proof analysis ran) →
   `analyze_execution_proof.txt`.
6. **Analysis diagnostics collection** — counts `warning C6xxx` occurrences
   from the full build log (no suppression) → `analysis_diagnostics.txt`.
7. Release ctest (unit + integration + windows + app smoke + fuzz smoke).
8. **GoogleTest count reconciliation** — `--gtest_list_tests` count, executed
   summary lines, per-Part-III-suite presence assertions →
   `test_counts.txt` + step summary. Resolves 34-vs-35 on Windows too
   (expected: 34; DiagnosticsTest = 7).
9. **Part III regression filters** — explicit `--gtest_filter` runs for input
   reliability (matrix + layout typing + 3 layout suites), candidate suites,
   diagnostics/soak/budget suites, each failing the job on failure.
10. **Windows fuzz smoke** — `suborno_fuzz_smoke.exe tests/fuzz/corpus`
    (portable driver; no libFuzzer/MinGW involved).
11. **PE export evidence** — `dumpbin /exports` on `suborno_windows_tsf.dll`
    and `Suborno.exe`, `/dependents` on the DLL, then a real cross-check:
    every `src/platform/windows/tsf/suborno_windows_tsf.def` export name must
    appear in the dumpbin output or the job fails →
    `dumpbin_*.txt`, `def_vs_pe_check.txt`.
12. Installer: SHA-pinned Inno Setup 6.3.3 install, ISCC build,
    `scripts/test-windows-installer.ps1` (clean install + upgrade + uninstall
    + registration via the service `/s` path) → `installer_test.log`;
    Authenticode status recorded honestly (NotSigned accepted, never claimed
    signed).
13. Debug build + ctest → `debug_ctest.log`.
14. Artifacts: `windows-verification-evidence-*` (all evidence, `if: always()`
    so failures still upload diagnostics), `suborno-windows-x64-verify-*`
    (installer + DLL + app; `if: success()`).

### Job 2 `windows-uia-audit` (windows-2022, needs job 1)

Downloads the built `Suborno.exe` artifact and runs the repository's own
`scripts/windows-ui-audit/Invoke-SubornoUiAudit.ps1` — 84 happy-path + 14
failure-path UIA checks (element walk by AutomationId, 16 checkboxes twice,
10 dropdowns with option enumeration, 2 sliders min/mid/max, Save via UIA
Invoke AND real mouse click with persistence assertion, Cancel no-change,
rapid saves, About/OSK counts, topbar zones, toast, PNG screenshots). Exit
code = failed check count → the job fails on any failed check. Evidence
(`uia-results.json`, screenshots) uploaded `if: always()`.

### Triggers / permissions

- `push` on `ci/suborno-v19-windows-verification`; `workflow_dispatch`;
  `pull_request` to `main`.
- `permissions: contents: read` (minimum); concurrency-canceled per ref; no
  secrets introduced; no credential exposure to build steps.

## Validation performed locally

- PyYAML parse + structural assertions: job names, `needs`, `runs-on`, permissions, trigger keys — all pass.
- PS script snippets follow the repo's PS 5.1-compatible style (`Tee-Object`, no PS7-only operators); `Invoke-SubornoUiAudit.ps1` itself is already 5.1-clean per its header.
- Every invoked target/script verified to exist: presets (`windows-msvc`, `windows-msvc-release`, `windows-msvc-debug`), targets (`suborno_core`, `suborno_unit_tests`, `suborno_integration_tests`, `suborno_windows_tests`, `suborno_app`, `suborno_fuzz_smoke`), scripts (`test-windows-installer.ps1`, `Invoke-SubornoUiAudit.ps1`), `.def` path.

## What remains manual after activation

- Real interactive TSF typing inside a third-party host app (needs a desktop
  session with a focusable editor; the runner harness audits the app's own
  windows, not cross-application TSF typing).
- Signed-installer verification (no signing certificate exists; recorded as
  NotSigned).
- Baseline v18 binary ABI comparison (v18 source is not in the repository —
  strongest available comparison is `.def` ↔ dumpbin ↔ public header, which
  the workflow executes).
