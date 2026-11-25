# ADR 0003: Repository governance, hygiene and packaging layout

Status: Accepted

## Context

The structure audit (21 November 2026, "SubornoIME স্ট্রাকচার বিশ্লেষণ ও
উন্নয়ন রোডম্যাপ") scored repository hygiene and release governance at 4.0/10
and packaging at 5.0/10. Three concrete gaps drove the score:

1. An 876 KB `core` crash-dump artifact was tracked in the repository root,
   inflating every clone and potentially exposing process memory.
2. There was no `CHANGELOG.md`/`NEWS`/`VERSION` file, although seven of the
   eight benchmark repositories (systemd, FFmpeg, Git, Redis, …) keep one,
   and the project already had a release workflow.
3. The project builds and tests on Ubuntu in CI but shipped no deb, rpm or
   AppStream metadata, so Linux users had to build from source.

The audit's roadmap (phase 1 and 2) lists the remedies in risk-reverse order.

## Decision

1. **Hygiene**: the `core` artifact is removed from the tree;
   `.gitignore` gains `/core`, `/core.*` and `vgcore.*` so crash dumps can
   never be tracked again.
2. **Governance**: `CHANGELOG.md` (Keep-a-Changelog format) records every
   user-visible change; `MAINTAINERS.md` maps each module to an owner and
   review scope. A release may not be tagged without a CHANGELOG entry for
   it — the release workflow treats a missing entry as a blocker.
3. **Packaging**: a `packaging/` module is added with
   `packaging/debian/` (control, rules, changelog, copyright, install —
   the minimum dpkg-buildpackage set) and
   `packaging/appstream/org.subornoIME.metainfo.xml` (GNOME/Ubuntu Software
   listing prerequisite). The Windows InnoSetup script stays in
   `installer/windows/` as the source of truth; `packaging/README.md` maps
   all three.
4. **Build layout**: the single 414-line root `CMakeLists.txt` is split into
   per-directory build files (one per source domain, plus `tests/`,
   `benchmarks/` and `examples/`), leaving the root file with options,
   dependency discovery and `add_subdirectory` calls only.
5. **Examples**: `examples/c_api_example.c` and `examples/engine_demo.c`
   compile the C API into CI-gated executables so external integrators have
   a working entry point and broken examples are red CI jobs.

## Consequences

- Clones stop shipping binary artifacts; `git status` stays clean after
  crashes on Linux.
- Release notes become first-class: packagers read `CHANGELOG.md` instead of
  mining commit history.
- `dpkg-buildpackage` from a clean checkout produces a working deb; the
  AppStream file can be validated with `appstreamcli validate`.
- Build-file changes are localized: a layout change touches
  `src/layouts/<family>/CMakeLists.txt`, not the root file — merge-conflict
  surface shrinks accordingly.
- The remaining roadmap items (po/ i18n extraction, codecov configuration,
  sanitizer CI matrix, per-release benchmark gates) are deferred to the
  audit's phase 3 and tracked there, not silently dropped.
