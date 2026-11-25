# scripts/

One tool per file, self-contained, callable from CI and from a clean
machine. Structure audit #2 introduced the conventions below so the
directory stays navigable as it grows (the audit's industry set — mpv,
kitty, wezterm — all ship a documented tooling entry point).

## Naming conventions

| Kind | Convention | Examples |
|------|------------|----------|
| Shell scripts | `kebab-case.sh`, verb-first, OS suffix when platform-bound | `check-format-ubuntu.sh`, `bootstrap-ubuntu.sh`, `audit-windows-dependencies.ps1` |
| Python scripts | `snake_case.py` (PEP 8 module style) | `check_layering.py`, `clean_wordlist.py`, `validate_wordlist.py` |
| No duplicate dialects | one file per task; shared logic goes into the script itself or a CMake helper under `cmake/modules/`, not a new wrapper |

## Inventory

| Script | Purpose | Runs on |
|--------|---------|---------|
| `bootstrap-ubuntu.sh` | One-shot clean-machine bootstrap (deps + toolchain) | Ubuntu |
| `install-llvm-23-ubuntu.sh` | Pinned LLVM/Clang 23 toolchain install | Ubuntu |
| `install-linux-build-tools.sh` | apt packages for building | Ubuntu |
| `configure-ubuntu.sh` / `build-ubuntu.sh` / `debug-build.sh` | Preset-driven configure/build/debug wrappers | Ubuntu |
| `check-format-ubuntu.sh` | clang-format gate (CI step) | Ubuntu |
| `check-tidy-ubuntu.sh` | clang-tidy gate (CI step) | Ubuntu |
| `check-layering.py` | Module layering contract enforcement (`docs/architecture/layering.md`) | any (Python 3) |
| `test-ubuntu.sh` / `verify-ubuntu.sh` | ctest run / post-build verification | Ubuntu |
| `release-build.sh` | Release packaging build | Ubuntu |
| `security-scan-ubuntu.sh` | Dependency/vulnerability scan | Ubuntu |
| `clean_wordlist.py` | Normalize raw wordlist TSVs into `data/wordlist/` | any |
| `generate_clitics.py` | Generate clitic forms (wordlist part 15) | any |
| `generate_icon_assets.py` | Generate `packaging/linux/icons/` size set | any |
| `generate-guids.sh` | Regenerate TSF COM GUIDs (`src/platform/windows/tsf/guids.*`) | any |
| `validate_wordlist.py` | Wordlist schema/range validation gate | any |
| `audit-windows-dependencies.ps1` | Binary dependency audit (CI step) | Windows |
| `test-windows-installer.ps1` | Installer lifecycle test (CI step) | Windows |

## Adding a script

1. Pick the naming convention for its language (table above).
2. Add a row to this inventory in the same change.
3. If CI must run it, call it from the relevant workflow step — CI is the
   only sanctioned automation entry point (no cron-in-repo magic).
