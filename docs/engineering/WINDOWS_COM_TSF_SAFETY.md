# Windows COM & TSF Lifetime Safety

**Status:** Active. Complements `MEMORY_SAFETY_POLICY.md`. Evidence source for
acceptance criteria F (Windows Resource Safety).

## COM reference counting

- Every COM implementation (`SubornoClassFactory`, `SubornoTextService`, edit
  sessions, candidate UI element, display attribute providers, UIA element
  providers) follows `IUnknown` exactly: `AddRef`/`Release` via
  `InterlockedIncrement`/`InterlockedDecrement`, `delete this` only when the
  count reaches zero.
- Underflow/overcount protection: refcounts start at 1 on creation (creator
  holds the initial reference) and call sites release symmetrically; `wil::com_ptr`
  removes manual counting at borrower sites.
- The module tracks outstanding objects and server locks
  (`com/module_lifetime.hpp`): `DllCanUnloadNow` returns S_OK only when both
  counters are zero, so the DLL cannot unload under live objects.

## COM smart pointers

`wil::com_ptr` (WIL 1.0.x, pinned in `vcpkg.json`) owns interface pointers at
call sites; manual `AddRef`/`Release` appears only inside refcounted
implementations. `QueryInterface` failures are handled (E_NOINTERFACE paths
null the out pointer first).

## Exception containment

C++ exceptions must never escape the COM binary interface. Every non-trivial
COM/UIA entry point routes through `run_inside_com_boundary`
(`src/platform/windows/com/boundary_guard.hpp`):

- `std::bad_alloc` → `E_OUTOFMEMORY`
- any other exception → `E_UNEXPECTED`

Applied to 78 methods across `tsf/` and `ui_accessibility.cpp`
(audit fixes F-001/F-002; see `REFACTORING_REPORT.md`). `AddRef`/`Release`
remain unguarded by design (they cannot throw).

## TSF-specific lifetime rules

| Concern | Rule in force |
| --- | --- |
| Edit sessions | Requested through `ITfEditSession` with the edit cookie used only inside `DoEditSession`; session objects release themselves when TSF drops the last reference |
| Text store / document manager / context | Pointers are held via `wil::com_ptr` and never stored beyond the callback frame unless the interface contract pins the object |
| Sink registration | Advise/unadvise pairs are symmetric; sinks are removed during `Deactivate` before the text service releases TSF resources |
| Reentrant calls | TSF callbacks never re-enter the engine while a project mutex is held; no locks are held across COM calls (`CONCURRENCY_SAFETY.md`) |
| Thread affinity | All TSF interaction stays on the apartment thread; no TSF pointer crosses threads |
| Shutdown ordering | Deactivate → unadvise sinks → release context/document manager → module lifetime gate; verified by `tests/windows/` contract tests |

## Verification evidence

- MSVC CI job builds and runs the TSF contract tests
  (`suborno.windows.tsf_contract`) on every push.
- `DllCanUnloadNow` gating is exercised by the module smoke test.
- The COM boundary audit (F-001/F-002) is recorded with file/line evidence in
  `REFACTORING_REPORT.md`.
