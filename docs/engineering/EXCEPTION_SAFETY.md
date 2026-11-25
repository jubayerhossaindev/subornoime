# Exception Safety & Error Handling

**Status:** Active. Complements `MEMORY_SAFETY_POLICY.md`.

## Exception safety levels

Operations aim for the strongest practical guarantee:

1. **No-throw** — move operations, swap, refcounting, C API status returns.
2. **Strong** — transactional helpers roll back on failure (dictionary bulk
   writes roll back the transaction and rethrow).
3. **Basic** — invariants hold, no leaks, state remains consistent.

## Rules

1. No empty catch blocks; no swallowed errors. Every `catch` either handles
   the condition meaningfully or rethrows. The two best-effort catches in
   `candidate_engine.cpp` / `spell_checker.cpp` document why a degraded
   result is correct behavior.
2. `noexcept` is used only where throwing is genuinely impossible (C API
   boundary helpers, COM refcount methods, status converters). Declaring
   `noexcept` to hide potential termination is banned.
3. Failures produce explicit representations:
   - internal APIs: `std::optional`/status enums/`CommitResult`;
   - C API: `SUBORNO_Status`/`SUBORNO_ERROR_*` codes (never exceptions);
   - COM: `HRESULT` (never exceptions across the boundary).
   `std::expected` is forbidden (C++20-only policy).
4. Partial updates leave consistent state: dictionary writes are transactional
   (`begin_bulk_write` / `commit_bulk_write` / `rollback_bulk_write`), and
   settings files write through `AtomicFile` with rollback.
5. Error messages are meaningful — including the failing operation, never
   just a bare code.

## Boundary containment (reviewed and enforced)

| Boundary | Mechanism | Evidence |
| --- | --- | --- |
| C API (`src/core/c_api.cpp`) | every entry point wrapped in `protect_c_boundary` or an explicit try/catch; failures map to `SUBORNO_ERROR_INTERNAL`/`E_OUTOFMEMORY`-equivalents | 13/13 entry points |
| COM/TSF (`tsf/`) | every non-trivial `STDMETHODIMP` body wrapped in `run_inside_com_boundary` (`com/boundary_guard.hpp`): `bad_alloc` → `E_OUTOFMEMORY`, other escapes → `E_UNEXPECTED` | 56 methods wrapped (audit fix F-001) |
| UIA providers (`ui_accessibility.cpp`) | same guard for all non-trivial `IFACEMETHODIMP` bodies; pre-existing try/catch blocks retained | 22 methods wrapped (audit fix F-002) |
| `extern "C"` DLL exports (`dll_main.cpp`) | return plain BOOL/HRESULT, no project exceptions escape | code review |

## Error-handling style

- Internal code throws standard exceptions (`std::invalid_argument`,
  `std::runtime_error`, `std::bad_alloc` propagation) and lets callers decide.
- Validation happens *before* mutation (`INPUT_VALIDATION.md`).
- Failure paths set all output parameters to their documented "empty" values
  first (C API null/0 initialization), so a failure return never leaves
  uninitialized outputs behind.
