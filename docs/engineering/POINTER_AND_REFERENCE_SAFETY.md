# Pointer & Reference Safety

**Status:** Active. Complements `MEMORY_SAFETY_POLICY.md`.

## Raw pointer policy

Raw pointers are not forbidden, but their use is restricted to:

- Non-owning references with an obvious lifetime.
- Windows API boundaries (`HWND`, `WPARAM`/`LPARAM` plumbing).
- COM interface interop (`IUnknown*`-style plumbing).
- Legacy C ABI boundaries (sqlite3, the public C API).

Every raw pointer must answer: who owns it, can it be null, how long is it
valid, who destroys the object, and is concurrent access allowed. Owning raw
pointers, unclear-lifetime pointers, uninitialized pointers and dangling
pointers are forbidden.

## Codebase rules in force

1. **Nullability is explicit.** C API entry points validate `session`,
   `buffer`, `buffer_size` and `count` before touching anything
   (`src/core/c_api.cpp`); output pointers are set to null/0 *before*
   validation so failures leave no uninitialized output.
2. **References for required access, pointers for optional access.**
   Non-optional callers take references; optional objects use pointers with a
   documented null contract.
3. **Pointer arithmetic is forbidden in application code.** UTF-8 iteration
   uses an index-based walk over `std::string::data()` with explicit bounds
   (`for_each_utf8_codepoint` in `src/dictionary/dictionary.cpp`), not manual
   pointer traversal.
4. **`reinterpret_cast` is limited and reviewed.** Current inventory:
   - sqlite3 text/byte conversions in `src/dictionary/dictionary.cpp`
     (C API boundary; the pointer is never written through and never read
     past the reported length).
   - `for_each_utf8_codepoint` (char → unsigned char byte inspection;
     reviewed boundary cast with an explanatory comment).
   Adding a new one requires a comment at the site plus a review note.
5. **C-style casts are banned** and are not present in project code; MSVC
   `/permissive-` and clang-tidy `cppcoreguidelines` checks watch for them.
6. **COM reference counting** follows `IUnknown`: `AddRef`/`Release` via
   `InterlockedIncrement`/`InterlockedDecrement`, `delete this` only at count
   zero, `QueryInterface` never throws through the boundary
   (`WINDOWS_COM_TSF_SAFETY.md`).
7. **COM smart pointers:** `wil::com_ptr` (plus `wil::unique_hmodule`,
   `wil::unique_bstr`) manages COM object ownership at call sites; manual
   `AddRef`/`Release` appears only inside the refcounted implementations
   themselves.

## Review checklist for new pointer code

- [ ] Owner named; lifetime bounded by a named scope.
- [ ] Null path either impossible by construction or handled explicitly.
- [ ] No arithmetic traversal; containers/views instead.
- [ ] No cast to a differently-typed buffer without a reviewed-boundary comment.
- [ ] Callback or stored reference cannot outlive the object (see
      `OBJECT_LIFETIME_RULES.md`).
