# Developer Workflow

The day-to-day loop for working on Suborno IME. All commands are verified
against the repository presets.

## Setup

```bash
git clone <repository> && cd subornoime
# Ubuntu: install toolchain (LLVM 23, ninja, ICU, SQLite)
./scripts/install-llvm-23-ubuntu.sh
./scripts/install-linux-build-tools.sh
# Windows: VS 17 2022 + v143 toolset + Windows SDK; vcpkg resolved via preset
```

## Every change

```bash
# 1. Configure + build
cmake --preset ubuntu-dev
cmake --build --preset ubuntu-dev

# 2. Fast gates (same ones CI runs first)
python3 scripts/validate_naming.py                 # naming standard
scripts/check-format-ubuntu.sh                     # clang-format
python3 scripts/check-layering.py                  # core stays platform-independent

# 3. Tests
cmake --build --preset ubuntu-dev --target suborno_unit_tests suborno_integration_tests
ctest --preset ubuntu-dev --output-on-failure

# 4. Static analysis (changed-file workflow)
scripts/check-tidy-ubuntu.sh build/ubuntu-dev

# 5. Documentation (public API changes)
cmake --build --preset ubuntu-dev --target docs
```

## Before pushing

- Formatting applied (`clang-format -i`) — CI fails otherwise.
- Naming validation green — CI fails otherwise.
- New behaviour carries a test; bug fixes carry the regression test.
- Public API changes update the Doxygen documentation in the same change.
- Commit messages follow Conventional Commits (`feat(unicode): …`,
  `fix(layouts): …`); commitlint runs via pre-commit (`.pre-commit-config.yaml`).

## Windows validation (owner of the Windows pipeline)

```powershell
cmake --preset windows-msvc
ctest --preset windows-msvc-release --output-on-failure
cmake --build --preset windows-msvc-release      # TSF DLL + Suborno.exe
./scripts/audit-windows-dependencies.ps1          # binary dependency audit
```

## VS Code

`.vscode/` is fully configured: clangd (compile commands from
`build/ubuntu-dev`), format-on-save via `.clang-format`, CMake presets, tasks
for build/test/format/naming/docs, launch profiles for the test binaries and
promptable GoogleTest filters.

## CI jobs (`.github/workflows/ci.yml`)

1. `naming-and-format` — naming validator + clang-format + layering.
2. `ubuntu-clang` — Clang 23 configure/tidy/test/build.
3. `ubuntu-gcc-13` — GCC 13 compatibility.
4. `windows-msvc` — MSVC v143 configure/test/build + installer audit.
