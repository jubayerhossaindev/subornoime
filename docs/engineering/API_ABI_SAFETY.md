# C API & ABI Boundary Safety

**Status:** Active. The public C API (`include/suborno/suborno.h`) is a stable
ABI: existing signatures, enum values and semantics are frozen.

## Boundary contract (per function)

| Requirement | Enforcement |
| --- | --- |
| Pointer validity | null-checked before any dereference; documented per `@param` |
| Buffer capacity | caller-declared size validated first (`text.size() >= buffer_size` → `SUBORNO_ERROR_BUFFER_TOO_SMALL`), output always NUL-terminated |
| Null pointer handling | argument checks precede everything; outputs pre-nulled (`*committed = nullptr`) so failure returns are safe |
| Explicit length | all buffers carry explicit `buffer_size`; no hidden NUL assumptions |
| Data ownership | engine-owned vs caller-owned stated on every API (`suborno_string_free` for returned text) |
| Allocation ownership | commit paths allocate with `std::malloc`; the matching `suborno_string_free` uses `std::free` — an exactly paired contract |
| Matching deallocation | single free function; internal `delete`/`new` pairs never cross the ABI |
| Calling convention | `extern "C"`, default convention; `SUBORNO_API` export/import decoration per platform |
| Struct layout | only opaque handles (`SUBORNO_Engine`, `SUBORNO_Session`) cross the ABI — no public struct layout to break |
| ABI stability | enum values documented as external contracts in the header; renames forbidden |
| Exception boundary | every entry point exception-contained (`EXCEPTION_SAFETY.md`) — 13/13 verified |

## Verified properties (audit evidence)

- `suborno_engine_create`/`suborno_session_create` wrap construction in
  try/catch and return `nullptr` on any failure (allocation failure never
  propagates).
- Integer-returning functions wrap their bodies in `protect_c_boundary`
  (`SUBORNO_ERROR_INTERNAL` on escape).
- `copy_commit_result` rejects a size overflow before `std::malloc(size + 1)`
  and handles allocation failure (`SUBORNO_ERROR_INTERNAL`).
- Candidate indices are validated against the live count before snapshot or
  selection (`SUBORNO_STATUS_CANDIDATE_OUT_OF_RANGE`).
- `suborno_engine_destroy` / `suborno_session_destroy` accept null and are
  noexcept by construction (RAII-only destructors).

## Change discipline

- Never break the existing ABI (no signature, enum-value or semantics change).
- New functions: follow the same contract table, document with Doxygen
  (`@brief`/`@param`/`@return`/`@note`), and add coverage to
  `tests/unit/c_api_test.cpp` / `c_api_coverage_test.cpp`.
- Fuzz target `tests/fuzz/c_api_fuzz.cpp` + portable smoke driver exercise the
  lifecycle, out-of-range and buffer-too-small paths continuously.
