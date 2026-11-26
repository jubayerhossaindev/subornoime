# Header Standards

Headers are the API contract. Each one must be includable on its own and
expose as little as possible.

## Rules

1. **Self-contained headers.** Every header compiles alone — verified by
   including it directly in its own translation unit pattern (include order in
   the .cpp lists its own header first).
2. **Include-what-you-use.** Only direct dependencies are included; IWYU tooling
   findings are triaged, not ignored.
3. **Forward declarations** in headers where the full type is only used by
   reference/pointer (e.g. `struct DictionaryStorage;` inside `Dictionary`,
   `struct OskWindowState;` inside the window classes) — keeps include trees
   shallow and breaks cycles.
4. **No circular includes.** Module dependencies flow one way:
   `platform → core → layouts/candidates/dictionary → layout-api → unicode`.
   `scripts/check-layering.py` fails CI on layer violations.
5. **Public/private separation**: public headers live in `include/suborno/`,
   module-private ones next to their sources (`phonetic_parser.hpp` inside
   `src/layouts/phonetic/`). Public headers must not leak private detail
   (pimpl forward declarations keep internals out).
6. **Include ordering** (enforced by clang-format `IncludeBlocks: Regroup`):
   own header → C system → C++ standard → angle-bracket third parties →
   quoted project headers.
7. **`#pragma once`** is the include-guard policy for all new headers; classic
   guards (`SUBORNO_SMART_PHONETIC_ENGINE_HPP`) are kept where already present.
8. **No `using namespace` in headers.** Even in source files, aliases are used
   sparingly and only for long nested platform namespaces.
9. **No global namespace pollution**: every internal declaration lives in
   `suborno::*` (or a module namespace like `khipro`, `compat`);
   file-local helpers sit in an anonymous namespace.
10. **Header guard policy** includes license/provenance comments at the top of
    each file (see COMMENT_STANDARDS.md).
