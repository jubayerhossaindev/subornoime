# COM Exception-Safety Report

Status: **`WINDOWS SOURCE AUDIT — VERIFIED`** (executed source audits);
**runtime COM tests BLOCKED** on the GitHub runner path (workflow wired; the
existing `suborno.windows.tsf_contract` ctest target runs on Windows).

## Boundary guard — 78 methods, counted (executed audit)

`grep -rc "run_inside_com_boundary" src/platform/windows/ --include="*.cpp"`
→ **78 call sites total**, distributed:

| File | Guards |
|---|---|
| `tsf/text_service.cpp` | 20 |
| `tsf/candidate_list_ui_element.cpp` (UIA provider surface) | 16 |
| `tsf/display_attribute_info.cpp` | 11 |
| `tsf/text_edit_session.cpp` | 4 |
| `tsf/class_factory.cpp` | 3 |
| `tsf/compartment.cpp` | 2 |
| other Windows files (app/UIA helpers) | 22 |

This matches the v19.1 engineering claim of 78 wrapped COM/UIA entry points —
re-verified by count in this session, not taken from the report.

## Contract verified from `boundary_guard.hpp`

- `run_inside_com_boundary` converts `std::bad_alloc` → `E_OUTOFMEMORY`,
  any other exception → `E_UNEXPECTED`, and lets `HRESULT`-domain failures
  pass through — no C++ exception can cross a COM ABI boundary.
- Guard wraps method bodies (not the raw `QueryInterface`/`AddRef`/
  `Release` plumbing of `IUnknown`, which stays direct) — reference counting
  remains exception-free by construction.
- WIL (`wil::com_ptr`) ownership used throughout the TSF sources; no bare
  `new`/`Release` pairs observed in the audited entry points.
- Module unload gate (`DllCanUnloadNow`) exists in `dll_main.cpp`; the
  registration path performs **atomic cleanup** — if `register_tsf_profile`
  fails after `register_inproc_server` succeeded, the class registration is
  rolled back (`dll_main.cpp:69-71`).

## Runtime evidence (BLOCKED — will come from the GitHub run)

- `suborno.windows.tsf_contract` (ctest `suborno.windows.tsf_contract`,
  label `non-interactive`) exercises the real COM objects in-process on
  Windows.
- The UIA audit job exercises the provider surface live.
- The installer lifecycle test activates the service DLL via its silent
  register path.

No runtime COM result is claimed from Linux.
