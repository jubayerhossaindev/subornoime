# Object Lifetime Rules

**Status:** Active. Complements `MEMORY_SAFETY_POLICY.md`.

## Mandatory rules

1. Initialize every object before use; no uninitialized locals, pointers or
   members (`MEMORY_OWNERSHIP` style: `{}` initializers on members).
2. No reference or pointer may outlive its object.
3. No dangling returns: never return a reference or view to a local or
   temporary (`Return Reference Safety`).
4. Views (`std::string_view`, `std::span`) may never outlive their backing
   storage. Storing a view in a class member requires a documented owner
   relationship; new members should prefer owned `std::string`/containers
   (see the `RuleBasedPhoneticLayout` member comments for the accepted form).
5. Iterator invalidation rules (`vector` reallocation, `string` mutation,
   `unordered_map` rehash, `map` erase, swap, insert/erase) must be respected;
   never reuse a reference or iterator after a documented-invalidating
   modification.
6. Moved-from objects are valid but unspecified: do not read their old value
   without re-assigning.
7. Member initialization order must match declaration order; do not rely on
   virtual dispatch inside constructors.
8. Constructors must not publish `this` to other components or register async
   callbacks before construction completes.
9. Destructors are effectively `noexcept`; they must not throw, and
   destruction must not race other threads' access.
10. Classes deletable through a base pointer have a virtual destructor (the
    COM implementations satisfy this through `IUnknown`).

## Verified codebase properties

- No `[&]`/`[=]` default lambda captures exist in the codebase; captures are
  explicit. The one blanket-capture pattern allowed is the synchronous COM
  boundary lambda (`run_inside_com_boundary`), which never escapes the calling
  stack frame — documented in `src/platform/windows/com/boundary_guard.hpp`.
- The only stored view member is
  `RuleBasedPhoneticLayout::tokens_` (`std::span`), whose static-lifetime
  contract is documented on the member and in `MEMORY_OWNERSHIP.md`.
- Window state objects are allocated with `new (std::nothrow)` and
  null-checked; teardown paths delete exactly once (`toast_window.cpp`,
  `topbar.cpp`, `osk_window.cpp`).
- The fuzz suite exercises repeated init/shutdown, copy/move and shutdown
  ordering through the public C API lifecycle target (`tests/fuzz/c_api_fuzz.cpp`).

## Temporary-object discipline

Do not bind a long-lived view to a temporary:

```cpp
// Forbidden: view of a temporary.
std::string_view input_text = create_temporary_text();

// Required: own the storage or verify the returned lifetime.
std::string input_text = create_temporary_text();
```

Review any function returning a reference for its lifetime provenance before
storing the result in anything longer-lived than the current statement.
