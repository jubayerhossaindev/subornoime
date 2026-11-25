# Resource Management (RAII)

**Status:** Active. Complements `MEMORY_OWNERSHIP.md` and
`MEMORY_SAFETY_POLICY.md`.

## Principle

Every acquired resource has exactly one owner and is released by RAII on every
exit path — normal return, early return and exception. Manual cleanup only
appears inside the two audited boundaries (C API handles, COM refcounting)
where the ABI demands it.

## Resource inventory and their owners

| Resource | Owner | Release mechanism |
| --- | --- | --- |
| Heap memory | containers / smart pointers | RAII (`MEMORY_OWNERSHIP.md`) |
| SQLite database handle | `DictionaryStorage` | RAII close in destructor; transactions roll back on exception (`dictionary.cpp`) |
| Committed C API strings | caller | `suborno_string_free` (`std::malloc`/`std::free` pair) |
| COM interfaces | `wil::com_ptr` | Release on destruction |
| DLL modules | `wil::unique_hmodule` | FreeLibrary on destruction |
| BSTRs | `wil::unique_bstr` | SysFreeString on destruction |
| Windows windows/classes | app window objects | explicit `destroy()` teardown paths, single-delete verified |
| Keyboard hooks | `KeyboardHook` (move-only RAII class) | unhook in destructor |
| Files | standard library streams / `AtomicFile` | scope exit, with rollback on write failure |

## Rules

1. Acquire → immediately name the owner. No naked acquire-and-forget.
2. Release happens on all exit paths; early returns must not leak (verified by
   ASan leak detection in CI).
3. Duplicate release is forbidden — COM refcounts and RAII owners make it
   structurally impossible in current code.
4. Partial initialization is safe: constructors acquire resources in RAII
   members, so a throw during construction releases what was already taken.
5. Shutdown ordering: the TSF thread manager deactivates and releases sinks
   before the module unlocks (`module_lifetime().can_unload()` refuses unload
   while objects or server locks remain).
6. Custom deleters must match the resource type (a `HANDLE` is not an
   `HKEY`); WIL's typed wrappers are preferred over hand-rolled deleters.
7. No unnecessary resource-management frameworks: the project uses standard
   containers, WIL and two small audited boundary patterns.

## Verification

- ASan leak detection on the full test suite: clean.
- COM object count balance is enforced by `com::module_lifetime()`
  (`can_unload()` gate in `dll_main.cpp`).
- C API ownership contract is documented on every `suborno.h` declaration that
  returns or consumes memory.
