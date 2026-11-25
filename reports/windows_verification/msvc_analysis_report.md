# MSVC Static Analysis Report

Status: **BLOCKED for execution** (GitHub run required); the execution-proof
gate is wired into the verification workflow. This report records the
source-level audit of the analysis configuration.

## Source-audited configuration (local, executed)

`cmake/modules/SubornoBuildOptions.cmake`:

- Line 20 — every MSVC target compiles with `/W4 /utf-8 /permissive- /sdl
  /GS /guard:cf` (plus the matching `/CETCOMPAT`-style linker guard wiring
  noted in the comment block).
- Line 31 + 114 — `SUBORNO_ENABLE_MSVC_ANALYSIS` gates the `/analyze` flag;
  the option defaults **ON** (`CMakeLists.txt:7`).
- `suborno_mark_dependency_headers_external(...)` — third-party headers
  (GTest, WIL) marked external so analysis warnings stay on project code,
  not dependencies (applied to `suborno_windows_tests` at
  `tests/CMakeLists.txt:128-131`).

So the configuration exists, is default-on, and propagates per-target.
However — per mission §5 — **configuration inspection is not execution
evidence**. No `/analyze` diagnostic was produced in this sandbox.

## What the workflow run will produce (BLOCKED evidence)

The verification workflow adds two gates the CI job lacks:

1. **Execution proof**: verbose rebuild of `suborno_core`; the job FAILS
   unless at least one real `cl.exe` command line contains `/analyze`
   (`analyze_execution_proof.txt` captures the matching lines).
2. **Honest diagnostics accounting**: the full Release build log is scanned
   for `warning C6xxx` analysis codes; every occurrence with its source
   location lands in `analysis_diagnostics.txt`. New findings are NOT
   suppressed — the report must classify them (the mission forbids automatic
   suppression).

## Acceptance status

`BLOCKED` — will become `PASS`/`FAIL` from the workflow artifacts.
