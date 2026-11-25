# MSVC Build Report (Windows)

Status: **BLOCKED — requires the GitHub-hosted Windows run** (no GitHub
authentication/remote in this sandbox; see `environment.md`). The build is
fully wired and gated in `.github/workflows/suborno-windows-verification.yml`
(job `windows-build-verify`); this report records what was verified from
source and what the run will produce.

## Verified from source (executed audits, local)

| Requirement | Evidence | Status |
|---|---|---|
| `windows-msvc` preset exists | `CMakePresets.json` — generator `Visual Studio 17 2022`, `architecture.value=x64`, `toolset.value=v143`, `binaryDir=build/windows-msvc`, vcpkg toolchain | VERIFIED (source) |
| C++20 enforced | `CMAKE_CXX_STANDARD=20`, `STANDARD_REQUIRED=ON`, `EXTENSIONS=OFF` in the preset + `scripts/check_cpp20_only.py` green | VERIFIED (source + Linux scan) |
| Release/Debug build presets | `windows-msvc-release`, `windows-msvc-debug` (configuration mapped, `jobs: 2`) | VERIFIED (source) |
| Windows production targets exist | `suborno_windows_tsf` (DLL, `.def`), `suborno_app` (`Suborno.exe`), `suborno_windows_tests`, unit/integration tests, `suborno_fuzz_smoke` — all referenced by the existing CI `windows-msvc` job | VERIFIED (source) |
| Existing CI already builds on `windows-2022` | `.github/workflows/ci.yml` job `windows-msvc` (vcpkg pinned, Release+Debug, installer) | VERIFIED (source) |

## What the workflow run will produce (BLOCKED evidence)

- Full Release + Debug build logs of every production target.
- Compiler identity proof: toolset dir `14.3*` assertion, `cl` banner
  (`Microsoft (R) C/C++ Optimizing Compiler`), vswhere catalog version.
- Target counts: successfully built targets vs failures (from the build log).
- Debug-configuration test execution.

No MSVC build result is claimed here. Nothing was built with MSVC in this
sandbox (it is Linux/g++-only, and substituting any other toolchain is
forbidden by the mission).

## Historical note

The v20 commit message and `docs/engineering/REFACTORING_REPORT.md` record a
green `windows-msvc` CI job on GitHub for the v19/v20 waves (MSVC v143,
installer, installer lifecycle test). Those are past-run claims from the
project's own CI, not evidence produced by this session — they are not
re-verified here and remain subject to the next CI run.
