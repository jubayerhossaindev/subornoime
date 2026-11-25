# Developer Toolchain

Every tool below is verified against the project constraints (C++20, Windows
10/11 x64 + MSVC v143 primary, Ubuntu CI secondary, low-RAM local builds).

## Selected tools

| Concern | Tool | Notes |
|---|---|---|
| Formatting | clang-format (LLVM 23-compatible options) | `.clang-format`; CI gate |
| Editor consistency | EditorConfig | `.editorconfig` |
| Linting | clang-tidy | `.clang-tidy`; CI via `check-tidy-ubuntu.sh` |
| Naming validation | `scripts/validate_naming.py` (custom) | CI gate — see STATIC_ANALYSIS.md |
| Static analysis | MSVC `/analyze`; clang-analyzer via clang-tidy | Windows job + Ubuntu job |
| Language server | clangd | `.clangd` config; compile_commands.json from presets |
| IDE | VS Code + clangd + CMake Tools | `.vscode/` fully configured |
| Include analysis | IWYU findings triaged locally | not a CI gate |
| Memory diagnostics | AddressSanitizer + UBSan | `SUBORNO_ENABLE_SANITIZERS=ON` |
| Fuzzing | libFuzzer | `SUBORNO_BUILD_FUZZING=ON`, `tests/fuzz/` + corpus |
| Testing | GoogleTest 1.15 via CTest | `BUILD_TESTING=ON` |
| Benchmarking | Google Benchmark | `SUBORNO_BUILD_BENCHMARKS=ON`, `benchmarks/` |
| Profiling | Visual Studio Profiler (Windows), perf (Linux) | on demand |
| Documentation | Doxygen | CMake `docs` target |
| Dependencies | vcpkg manifest mode | pinned baseline commit |
| CI/CD | GitHub Actions | `ci.yml`, `release.yml`, `bench.yml`, `fuzz.yml` |
| Dependency audit | Dependabot | `.github/dependabot.yml` |

## Duplicate tools are not added

One tool per responsibility: clang-format for formatting (not astyle),
clang-tidy for linting (not cpplint), GTest for tests (not Catch2), vcpkg for
dependencies (not Conan + vcpkg). New tools require a gap the existing set
cannot cover.

## Windows-first build policy

Primary compiler: MSVC `cl.exe`, toolset v143, Visual Studio 17 2022,
CMake + MSBuild, compatible Windows SDK, C++20. MinGW / MSYS2 / clang-cl /
GCC are **not** build pipelines for Windows targets; clang-format, clang-tidy
and clangd are auxiliary development tools and are fine on any host.
Build parallelism is controlled (`VCPKG_MAX_CONCURRENCY: 2`, `CMAKE_BUILD_PARALLEL_LEVEL`)
for low-RAM environments.

## Local quick start

```bash
cmake --preset ubuntu-dev
cmake --build --preset ubuntu-dev
ctest --preset ubuntu-dev --output-on-failure
cmake --build --preset ubuntu-dev --target docs   # Doxygen HTML
python3 scripts/validate_naming.py                 # naming gate
```
