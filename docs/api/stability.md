# Public API Stability Policy

Scope: everything under `include/suborno/` — the headers other code (and
other teams, in the future) compile against. This document defines the
stability tiers, the versioning scheme, and the deprecation rules, following
the audit recommendation (`docs/api/stability.md`, gap 5). mpv's
`include/mpv` contract and librime's `include/` layout are the reference
models: a small, explicitly tiered public surface inside a large codebase.

## Tiers

| Tier | Headers | Promise |
|------|---------|---------|
| **Tier 1 — Stable** | `suborno.h` (the C API umbrella) | Source- and ABI-compatible within a major version. Only additive changes (new functions, new enum values at the end) in minor releases. Removal or signature change requires a major version bump. |
| **Tier 2 — Contract** | `layout-api/*.hpp` (`input_layout.hpp`, `input_session.hpp`, `key_event.hpp`, `candidate.hpp`, `layout_registry.hpp`, `declarative_layout.hpp`, `phonetic_layout.hpp`) | Source-compatible within a minor version; may evolve additively. Breaking changes require a deprecation notice one minor release ahead. |
| **Tier 3 — Internal** | everything else (`core/`, `candidates/`, `dictionary/`, `spell/`, `settings/`, `unicode/`, `layouts/`, `platform/` headers) | No stability promise between releases. Installed only for building the engine and its tests. |

## Rules

1. **The C API is the product surface.** `suborno.h` is what third-party
   hosts (launchers, accessibility tools, future ibus/IMK bridges) consume.
   Anything it exposes must have an owner, a test (`tests/unit/c_api_test.cpp`)
   and a documentation entry (`docs/api/README.md`, Doxygen).
2. **Version visible at compile time.** Every Tier 1 header change that
   adds functionality must update the version macros in `suborno.h`
   (major/minor/patch) in the same commit, so `#if` guards stay possible.
3. **Deprecation path.** Tier 1–2 deprecations: mark with a comment
   (`// DEPRECATED since x.y.z — use …`), keep the symbol working for at
   least one minor release, list it in `CHANGELOG.md` under **Deprecated**,
   then remove at the next major boundary.
4. **No half-public headers.** A header is either installed under
   `include/suborno/` (documented tier) or private inside `src/`. Headers
   must not migrate in one direction only.
5. **ABI-sensitive items are frozen per major version.** Function
   signatures, enum numeric values, struct layout and callback contracts in
   Tier 1 must not change meaning — additive enum values only, appended at
   the end, so switch statements stay forward-compatible.

## Current assessment

- Tier 1 is small by design (`suborno.h` → `c_api.cpp`), exercised by
  `examples/c_api_example.c` and `examples/engine_demo.c`; new examples must
  accompany any Tier 1 growth.
- Tier 2 grew during the layout-session refactor; treat it as frozen for
  the next minor release and evolve it only additively (per rule 3).
- Tier 3 headers are build-time plumbing; consumers outside the repo must
  not be created against them.

## Enforcement today

- CI builds the Tier 1 examples (`suborno_c_api_example`,
  `suborno_engine_demo`) — a Tier 1 break breaks the build, not a user.
- `tests/unit/c_api_test.cpp` pins the C API contract.
- Planned (gap 6 roadmap): a header-include check in CI that fails when a
  Tier 3 header leaks into an installed umbrella.
