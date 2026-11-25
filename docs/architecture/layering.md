# Module Layering Contract

This document is the normative dependency contract for the product code
(`src/` and `include/`). It is enforced mechanically by
`scripts/check-layering.py`; the checker's allowed-edge matrix must stay a
superset of this document, and every edge in the matrix must be justified
here. Tests, examples and benchmarks are out of scope: they may include any
product module.

## Layers (bottom → top)

| Layer | Modules | Carries |
|-------|---------|---------|
| Leaves | `unicode`, `persistence`, `dictionary`, `settings` | ICU link requirements, SQLite persistence, word data, versioned settings |
| Contract | `layout-api` | The host-neutral input-layout / session / candidate contract |
| Services | `spell`, `candidates`, `layouts`, `tools` | Spell-checking, candidate ranking, layout implementations, CLI utilities |
| Composition | `core` | The engine and the public C API (`suborno.h`) |
| Host | `platform` | Windows COM/TSF shell, Linux/macOS adapters — thin by design |

## Measured dependency graph (2026-11 audit)

Extracted from every `#include "suborno/..."` in `src/` and `include/`
(19 cross-module edges; reproduce with
`python3 scripts/check-layering.py --report`):

```
candidates -> dictionary, layout-api, spell, unicode
core       -> dictionary, layout-api, layouts, settings
layout-api -> unicode
layouts    -> candidates, dictionary, layout-api, unicode
platform   -> core, layout-api
spell      -> dictionary, layout-api, unicode
tools      -> dictionary
```

The graph is acyclic. `unicode`, `persistence`, `dictionary` and `settings`
include no other product module — they are the leaves everything else may
stand on.

## Rules

1. **Downward only.** A module may include modules from the same or a lower
   layer, never a higher one. The full matrix lives in
   `scripts/check-layering.py` (`ALLOWED`).
2. **Host stays thin.** Only `platform` may include `core`; nothing may
   include `platform` from outside it. Platform adapters translate host
   events (TSF, ibus, IMK) into the `layout-api` contract and delegate —
   they must not re-implement engine logic. This mirrors the renderer
   backend split in OBS (`libobs-d3d11` / `libobs-metal` / `libobs-opengl`)
   and the OS abstraction in mpv (`osdep/`).
3. **Contract is the wall.** `layouts` and `platform` talk to layouts only
   through `layout-api` (plus leaves); they may not reach into `spell` or
   engine internals. This is the same discipline as VS Code's
   `src/vs/base → platform → editor → workbench` layering.
4. **Leaves stay pure.** `dictionary` and `settings` must not grow upward
   dependencies; they are shared data carriers. New leaves must keep the
   same property.
5. **Public umbrella is free.** `suborno.h` (and any root-level header
   directly under `include/suborno/`) is fair game for every module — it is
   the published surface, not a layer.
6. **Same-module includes are free.** Intra-module helpers
   (e.g. `layouts/phonetic/phonetic_parser.hpp`) never leave the module.

## Enforcement

- `python3 scripts/check-layering.py` — exits non-zero on any edge outside
  the matrix; wire into CI next to `scripts/check-format-ubuntu.sh`.
- `python3 scripts/check-layering.py --report` — prints the current edge
  summary; paste into review notes when adding a module.
- Adding a legitimate dependency: extend `ALLOWED` **and** update the table
  above plus the measured-graph list in the same pull request, so the
  document and the checker never drift apart.

## Rationale

Unenforced layering erodes exactly where the code is busiest: a host shim
that grabs one "convenient" engine internal becomes a second engine. The
industry references in the repository audit (VS Code's documented layering,
Windows Terminal's `src/` module walls, OBS's backend directories, mpv's
`osdep/` isolation) all pair a declared contract with a mechanical check —
this document plus `check-layering.py` is that pair for SubornoIME.
