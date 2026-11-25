# Doxygen Standards

Doxygen is the project's documentation system (the C++ equivalent of
JSDoc/TSDoc). The generated public-API documentation is built with the CMake
target:

```bash
cmake --build --preset ubuntu-dev --target docs
# Output: build/<preset>/docs/api/html/index.html
```

`docs/api/Doxyfile.in` is configured by CMake (`configure_file … @ONLY`) from
the root `CMakeLists.txt` (`find_package(Doxygen)` → custom `docs` target).

## Syntax

Primary style — `/** … */` blocks for API documentation:

```cpp
/**
 * @brief Manages the lifecycle of an active text composition.
 *
 * Maintains the current composition text, selected candidate and
 * composition state for one input session.
 */
class CompositionEngine { … };
```

`///` line style is accepted for declarations whose documentation is a short
single statement (the majority of struct member comments in
`include/suborno/**`).

## Required commands

`@brief @details @param @tparam @return @retval @throws @pre @post @note
@warning @see @since @deprecated @code @endcode @file @class @struct @enum
@namespace`

Use a command only when it carries real information.

## Coverage requirements

1. **Public API first.** The stable C surface `include/suborno/suborno.h` is
   fully documented — every exported function has `@brief`, per-parameter
   `@param`, `@return` semantics, and `@note` for ownership (`suborno_string_free`).
2. **Classes** document responsibility, ownership, lifetime, thread safety and
   restrictions — not just the class name.
3. **Functions** document real behaviour. Never invent return or exception
   behaviour that the implementation does not have.
4. **Parameters** document purpose and direction; prefer return values over
   out-parameters, and document both when an out-parameter is kept.
5. **Return values** explain semantics, not just "the result".
6. **Exceptions** are documented only where actually thrown (`@throws`).
7. **Thread safety** is documented for anything reachable from multiple
   threads (e.g. `DictionaryLoader`: mutex-guarded lazy load, safe from any
   thread).
8. **Private implementation** needs no Doxygen on every function — use
   explanatory comments where algorithms, invariants, Unicode rules or
   platform interactions are non-obvious.

## Warning policy

`WARN_AS_ERROR = NO`; `WARN_IF_UNDOCUMENTED = NO` — member-level documentation
is added progressively, but files, classes, functions and parameters must stay
documented. The remaining member-level gaps are tracked as technical debt in
`REFACTORING_REPORT.md`.
