# Smart Pointer Policy

**Status:** Active. Complements `MEMORY_OWNERSHIP.md`.

## Decision table

| Situation | Choice |
| --- | --- |
| Object lives inside its parent | direct member (value semantics) |
| Sole heap owner | `std::unique_ptr` (make with `std::make_unique`) |
| Multiple independent owners | `std::shared_ptr` — only after review, never as default |
| Non-owning handle into shared state | `std::weak_ptr` |
| COM objects | `wil::com_ptr` (or COM refcount inside the implementation itself) |
| Windows module/BSTR handles | `wil::unique_hmodule` / `wil::unique_bstr` |
| Raw owning pointer | **forbidden** outside the C API boundary |

## Rules

1. `std::unique_ptr` is the default heap-ownership type.
2. `std::shared_ptr` exists at exactly one reviewed place: the C API session
   wrapper keeps the engine alive while a session exists
   (`SUBORNO_Session::engine_owner`). No other shared ownership is permitted
   without updating `MEMORY_OWNERSHIP.md`.
3. `shared_from_this` is not used in the codebase; if introduced, it must never
   run inside a constructor and must document the owning `shared_ptr` source.
4. Smart pointers are not nested gratuitously
   (`unique_ptr<shared_ptr<T>>` etc. is banned).
5. Ownership type is never changed in a way that breaks a lifetime contract
   silently; changing a member's ownership requires the ownership map update.
6. Null smart pointers are checked before dereference — same rule as raw
   pointers (`POINTER_AND_REFERENCE_SAFETY.md`).
7. WIL handles map each resource to its exact release function; a generic
   `CloseHandle`-style catch-all deleter for unrelated handle types is banned
   (`RESOURCE_MANAGEMENT.md`).

## Shared-pointer cycle prevention

No shared-ownership cycles exist today (the engine↔session edge is
owner→`unique_ptr`, with the shared edge pointing the other way:
session wrapper → engine). Any new `shared_ptr` member must document why a
cycle cannot form, or route through `std::weak_ptr`.
