# Memory Ownership Map & Rules

**Status:** Active. This document records the real ownership model of the
codebase after the memory-safety audit; update it with every ownership change.

## Ownership style rules

1. **Value semantics first.** Objects live as direct members when their
   lifetime matches the parent (`CompositionSession`-style plain members).
2. **`std::unique_ptr`** is the default ownership choice for heap objects.
3. **`std::shared_ptr`** only for genuinely shared, independently-owned state.
   The engine API (`Engine::create_session()`) returns `std::unique_ptr`; the
   C API wrapper struct `SUBORNO_Session` additionally holds a
   `shared_ptr` engine handle so the engine cannot die before its session —
   the one reviewed shared-ownership point, documented below.
4. **`std::weak_ptr`** is reserved for non-owning shared references; the
   codebase currently has no shared-ownership cycles (verified by audit
   F-006 review).
5. **Owning raw pointers are forbidden.** `new`/`delete` appear only inside
   the C API boundary (`src/core/c_api.cpp`) and COM reference-counted
   implementations where the ABI contract requires them.

## Module ownership map

| Module | Key objects | Owner | Borrowers | Lifetime contract |
| --- | --- | --- | --- | --- |
| `core` | `Engine` | `shared_ptr` held by `SUBORNO_Engine` / app shell | sessions | Engine outlives every session; enforced by session handle holding the engine handle |
| `layout` | `InputSession` | `unique_ptr` inside `SUBORNO_Session` | C API callers via opaque handle | Destroyed with the session |
| `layouts` | `RuleBasedPhoneticLayout` | registry / engine | candidates engine | Token table (`std::span` member) must have static lifetime — documented on the member; `profile_id_` is an owned copy (audit fix F-004) |
| `candidates` | `CandidateEngine` | `Engine` | sessions | Parent-owned, dies with parent |
| `dictionary` | `DictionaryRepository`, `DictionaryStorage` | `Engine` | spell checker | SQLite handle RAII-wrapped; rollback on exception (`dictionary.cpp` transaction guards) |
| `persistence` | `AtomicFile` | call-site local | settings writer | Scope-bound; exception-safe rollback |
| `settings` | `Settings` | app shell | engine, UI | Value member |
| `platform/windows` | window state structs | owning `unique_ptr`-equivalent via nothrow `new` + explicit delete in `destroy()` path | WndProc | Allocated `new (std::nothrow)`, null-checked (audit fix F-003); freed on window teardown |
| `platform/windows/tsf` | COM objects (`TextService`, edit sessions, UI element) | COM refcount (`AddRef`/`Release`/`delete this`) | TSF manager | Freed when refcount reaches 0; entry points exception-contained (audit fix F-001) |
| `platform/windows/app/ui_accessibility` | UIA providers | COM refcount | UI Automation | Same refcount model, all entry points guarded (audit fix F-002) |

## The two reviewed raw-allocation boundaries

1. **C API (`src/core/c_api.cpp`)** — allocates the opaque handle structs with
   `new`, frees with `delete`, and returns committed text buffers allocated
   with `std::malloc` that the caller releases through `suborno_string_free`
   (`std::free`) — an exactly matching pair. Every entry point is
   exception-contained by `protect_c_boundary`; overflow and null checks
   precede every allocation (`copy_commit_result`).
2. **COM implementations (`tsf/`, `ui_accessibility.cpp`)** — `delete this`
   inside `Release()` is the standard COM ownership contract; objects register
   with `com::module_lifetime()` so the DLL cannot unload with live objects.
   Manual `AddRef`/`Release` at call sites is minimized by `wil::com_ptr`.

## Change discipline

- New ownership must answer the ten questions in `MEMORY_SAFETY_POLICY.md`.
- New raw allocation outside the two boundaries above requires an audit
  finding entry and explicit approval in review.
- Update the map above in the same change that moves ownership.
