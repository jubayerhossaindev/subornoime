# Static Analysis Standards

Static analysis runs on every pull request. Findings are triaged — real bugs
are fixed, intentional patterns are justified at the suppression site, never
suppressed globally.

## Tool set

| Tool | Scope | Where it runs |
|---|---|---|
| MSVC Code Analysis (`/analyze`) | Windows targets (TSF, shell) | CI `windows-msvc` job; `SUBORNO_ENABLE_MSVC_ANALYSIS=ON` by default |
| clang-tidy | Full codebase (C++20) | CI `ubuntu-clang` job via `scripts/check-tidy-ubuntu.sh` |
| clang-analyzer / bugprone / cert / concurrency checks | `.clang-tidy` check set | CI |
| Cppcheck (optional) | local audits | developer workstation |
| CodeQL | repository security queries | GitHub Advanced Security (optional) |

## Configuration

- `.clang-tidy` starts from an explicit check list (`-*`, then
  `clang-analyzer-*`, `bugprone-*`, `cert-*`, `concurrency-*`, …) so the tool
  version cannot silently change the check set.
- Warnings are not errors yet; the goal is a clean, reviewed baseline.
- Suppressions: per-line `// NOLINT(...)` with a reason, or a narrowly scoped
  CMake definition — global disables are forbidden.

## Naming validation

`scripts/validate_naming.py` implements the regex-expressible part of the
naming standard (file names, kPascalCase constants, one-letter and abbreviated
locals) with an audited whitelist for COM/TSF external contracts. It runs as a
CI gate in the `naming-and-format` job together with `clang-format
--dry-run -Werror` and `check-layering.py`.

## Manual review rules

Regex cannot prove context; these stay review obligations:
- meaningfulness of names (domain context, Unicode units),
- thread-safety annotations and ownership comments,
- comment truthfulness.
