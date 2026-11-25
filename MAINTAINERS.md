# Maintainers

Ownership routing map, modeled on the Linux kernel and FFmpeg `MAINTAINERS`
files: every module lists the responsible owner and the review scope.
Reviewing is a responsibility, not a status symbol — a PR touching a module
routes to its listed owner first.

Format:

```
M:  Module owner (review requests route here first)
L:  Fallback reviewer when the owner is unavailable
S:  Status (Maintained / Odd fixes / Orphan)
F:  Files and directories (glob patterns)
```

## Owners

M:  SubornoIME Maintainers <maintainers@subornoime.org>
L:  Project lead
S:  Maintained

> NOTE: the project currently has a single maintainer. The `M:` line above is
> a placeholder for the maintainer's real name/address — fill it in before
> the first public release so PR routing becomes actionable.

## Modules

| Module            | Owner | Status      | Scope                                        |
| ----------------- | ----- | ----------- | -------------------------------------------- |
| Portable core     | M     | Maintained  | `src/core/` `include/suborno/core/`          |
| C API             | M     | Maintained  | `src/core/c_api.cpp` `include/suborno/suborno.h` `examples/` |
| Unicode layer     | M     | Maintained  | `src/unicode/` `include/suborno/unicode/`    |
| Dictionary        | M     | Maintained  | `src/dictionary/` `include/suborno/dictionary/` `data/` |
| Spell correction  | M     | Maintained  | `src/spell/` `include/suborno/spell/`        |
| Candidates        | M     | Maintained  | `src/candidates/` `include/suborno/candidates/` |
| Layout API        | M     | Maintained  | `src/layout-api/` `include/suborno/layout-api/` |
| Phonetic layouts  | M     | Maintained  | `src/layouts/phonetic/` + matching headers   |
| Fixed layouts     | M     | Maintained  | `src/layouts/{probhat,national,khipro}/` + matching headers |
| Settings          | M     | Maintained  | `src/settings/` `include/suborno/settings/`  |
| Persistence       | M     | Maintained  | `src/persistence/`                           |
| Windows platform  | M     | Maintained  | `src/platform/windows/` `installer/` `packaging/windows/` |
| Linux/macOS       | M     | Maintained  | `src/platform/{linux,macos}/` `packaging/`   |
| Tests             | M     | Maintained  | `tests/`                                     |
| Benchmarks        | M     | Maintained  | `benchmarks/`                                |
| Build system      | M     | Maintained  | `CMakeLists.txt` `cmake/` `CMakePresets.json` `vcpkg.json` `.github/` |
| Documentation     | M     | Maintained  | `docs/` `README.md` `CHANGELOG.md`           |

## Rules

1. Structural decisions (new top-level directories, build-system changes,
   packaging layout) require an ADR in `docs/decisions/` before merge.
2. Any change that can alter candidate output or the settings file format
   must be reviewed by the owning module owner *and* the tests owner.
3. Release gating follows `docs/decisions/ADR-0003-repository-governance.md`:
   no tag without a `CHANGELOG.md` entry.
