# Memory Management Standards

Ownership must be visible in the type system; lifetime errors must be
impossible by construction.

## Ownership rules

1. **RAII everywhere.** Every resource (memory, files, COM objects, GDI
   handles, mutexes) is owned by a scope-bound type.
2. **`std::unique_ptr`** for exclusive ownership
   (`std::unique_ptr<DictionaryStorage> storage_`,
   `std::unique_ptr<khipro::SpecEngine>`).
3. **`std::shared_ptr`** only for genuinely shared ownership
   (`SUBORNO_Session` holds `shared_ptr<Engine>` because the C API allows the
   session to outlive a user-dropped engine reference).
4. **`wil::com_ptr`** owns all COM objects in the Windows layer; raw COM
   pointers appear only as non-owning parameters.
5. **GDI/Win32 handles** are wrapped (window proc state types
   `OskWindowState`/`TopbarWindowState`/`ToastWindowState`, `wil::unique_hwnd`
   style patterns where applicable); no bare new/delete for window state
   outside the audited pimpl destruction paths.
6. **`new`/`delete` are banned** except for COM class factories that must
   return raw pointers with manual refcounts (`class_factory.cpp`,
   edit-session objects) — each such site is documented and covered by tests.
7. **No use-after-free / double-free / leaks / dangling pointers**: verified by
   ASan/UBSan runs (`SUBORNO_ENABLE_SANITIZERS=ON`), fuzzing
   (`tests/fuzz`), and the COM lifetime model (module-level `DllAddRef`/
   `DllRelease` lock in `module_lifetime.hpp`).
8. **Deterministic destruction**: locks, files and transactions release on
   scope exit (`std::lock_guard`, `std::unique_lock` with explicit
   unlock/relock windows documented in `DictionaryLoader`).
9. **Container selection** follows the data shape: `unordered_map` for keyed
   lookups (`variables_by_name_`, `map_rules_by_name_`), `vector` for ordered
   data, `unordered_set` for membership (`lexicon_`).

## Prohibited

- Raw owning pointers, `malloc`/`free` pairs outside the C API boundary
  (`suborno_string_free` pairs with the allocation made in `copy_commit_result`).
- `reinterpret_cast` lifetimes that outlive the target.
- Global mutable state without documented synchronization — the two allowed
  process-global slots (`DictionaryLoader`, the phonetic-correction atomic)
  document their threading model.
