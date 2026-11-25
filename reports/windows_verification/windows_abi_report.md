# Windows DLL Export and ABI Verification Report

Status: **source-level audit executed**; **PE inspection BLOCKED** (needs the
GitHub MSVC build + `dumpbin`; wired into the workflow).

## `.def` inspection (executed)

`src/platform/windows/tsf/suborno_windows_tsf.def` — exactly the five
standard COM in-proc server exports, no extras:

```
DllCanUnloadNow, DllGetClassObject, DllRegisterServer,
DllUnregisterServer, DllInstall
```

These are the only exports the TSF DLL needs (class object + self-registration
+ lifetime gate). Decorated C++ symbols are **not** exported by design — the
public C API belongs to the portable core (`suborno.h`, unchanged identifier
set), and the TSF surface is pure COM/GUID (identifiers pinned in
`guids.cpp`, unchanged).

## What the workflow run will produce (BLOCKED evidence)

1. `dumpbin /exports build\windows-msvc\Release\suborno_windows_tsf.dll`
2. `dumpbin /exports build\windows-msvc\Release\Suborno.exe`
3. `dumpbin /dependents suborno_windows_tsf.dll` (import/dependency surface)
4. **Real cross-check**: every `.def` export name must appear in the dumpbin
   listing, else the job fails (`def_vs_pe_check.txt`).
5. Public-header cross-check: `SUBORNO_API` declarations in
   `include/suborno/suborno.h` vs the portable DLL/app exports.

## Baseline vs v19 ABI comparison — honest limitation

Mission §9 requires comparing v18 and v19 binaries where possible. The v18
source is **not in this repository** (it exists only as an archive outside
version control and cannot be fetched by the workflow). Building and
fabricating a baseline from memory is forbidden. Therefore:

- `BASELINE VS V19 ABI COMPARISON`: **UNVERIFIED (baseline source
  unavailable in the repository)**.
- The strongest executed comparison remains: public C API header unchanged
  since the v19 refactor (git history shows no `suborno.h` identifier edits —
  `git log --follow include/suborno/suborno.h`), `.def` content fixed at the
  five standard exports, GUID identifiers compile-time pinned and unchanged.

A `.def` inspection alone is not declared sufficient for binary ABI
compatibility — that is exactly why the dumpbin cross-check step exists in
the workflow for the Windows run.
