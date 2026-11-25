# Windows COM

Windows-only COM support shared by the TSF text service and the UI
Accessibility providers:

- `module_lifetime.hpp` — `ModuleLifetime` tracks outstanding COM objects and
  server locks with atomics so `DllCanUnloadNow` can refuse to unload while
  objects or `LockServer` locks remain.
- `boundary_guard.hpp` — `run_inside_com_boundary` contains C++ exceptions at
  the COM binary interface (`std::bad_alloc` → `E_OUTOFMEMORY`, any other
  exception → `E_UNEXPECTED`). Every non-trivial COM/UIA entry point routes
  through it (see docs/engineering/WINDOWS_COM_TSF_SAFETY.md).

The COM implementations themselves live in `../tsf/` (text service, edit
sessions, class factory, candidate UI element, display attribute providers)
and `../app/ui_accessibility.cpp` (UI Automation providers).
