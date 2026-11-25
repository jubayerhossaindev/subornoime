# Folder-Structure Consolidation — Implementation Note

Date: 2026-11-21
Scope: repository structure only — no source-file relocation, no build or
behavior change. Implements the recommendations of the folder-structure
analysis report (radar rubric, seven-change proposal, P0–P3 roadmap).

## Applied changes

| # | Change | Before | After |
|---|--------|--------|-------|
| 1 | Distribution namespace consolidated | `installer/windows/` | `packaging/windows/` (`git mv`, history preserved) |
| 2 | Environment doc absorbed | `docs/environment/ubuntu.md` | `docs/development/ubuntu-environment.md`, cross-linked from `docs/development/ubuntu.md` |
| 3 | POC workflow archived | `.github/workflows/icu-minimal-poc.yml` | `.github/archive/icu-minimal-poc.yml` (no longer runs per push; still manually reproducible via the `windows-msvc-minimal-icu-poc` preset) |
| 4 | Status report relocated | root `PRODUCTION_READY.md` | `docs/reports/PRODUCTION_READY.md` |
| 5 | Root dotfile set completed | — | `.gitattributes` (LF normalization, CRLF for `*.iss`/`*.ps1`/`*.bat`/Windows project files, binary markers) |
| 6 | Issue intake formalized | PR template only | `.github/ISSUE_TEMPLATE/` with `bug_report.md` and `feature_request.md` |

Not applied now (per roadmap phasing): `packaging/macos/` (P3 — waits for a
buildable `src/platform/macos`), Doxygen CI publishing (P1 tooling, config
`docs/api/Doxyfile.in` already tracked), scheduled fuzz workflow and
`locales/` contract (P2).

## References updated with the move

- `.github/workflows/release.yml` — ISCC invocation, checksum, artifact
  upload and release-asset paths (5 occurrences)
- `.github/workflows/ci.yml` — ISCC invocation, checksum, artifact upload
  (4 occurrences)
- `scripts/test-windows-installer.ps1` — expected installer `Output` paths
- `CMakeLists.txt` — packaging-contract comment
- `packaging/README.md` — Windows section rewritten; the earlier
  "intentionally left in `installer/windows/`" policy is superseded by the
  report's consolidation decision
- `docs/architecture/target-architecture.md` — logical target tree
- `CHANGELOG.md` — `[Unreleased]` path mentions corrected and a
  consolidation bullet added

`ADR-0003-repository-governance.md` is left unmodified as an immutable
decision record; `packaging/README.md` (which the ADR designates as the
packaging map) now carries the current layout. The InnoSetup script's
internal `..\..\build\windows-msvc` relative paths are depth-invariant to
this move (`installer/windows` and `packaging/windows` are both two levels
below the root), so the packaging contract holds unchanged.

## Validation

- Reference sweep: no remaining `installer/`, `docs/environment/`,
  `.github/workflows/icu-minimal-poc.yml` references outside ADR-0003
  (immutable record) and `CHANGELOG.md` history.
- Configure + full build + CTest re-run green after the restructure.
- Format check (`scripts/check-format-ubuntu.sh`, clang-format 23.1.5,
  repo `.clang-format`): green — no C++ sources touched.

## Expected score movement

Quality-tool config 9.5 → 10 (dotfile set complete: `.clang-format`,
`.clang-tidy`, `.editorconfig`, `.gitattributes`, CI-enforced checks),
overall 8.3 → ≈8.4 after P0; P1 follow-ups (Doxygen CI publishing,
remaining P2/P3 items) target ≈8.5.
