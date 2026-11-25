# TSF Registration Report

Status: **`WINDOWS SOURCE AUDIT — VERIFIED`**; execution
**BLOCKED** locally; **real registration executes on the GitHub runner** via
the installer lifecycle test and will be captured there.

## Registration mechanism (source-verified, `dll_main.cpp:39-88`)

`DllRegisterServer`:

1. Resolves its own module path (`GetModuleFileNameW`, 32 KiB buffer,
   `HRESULT_FROM_WIN32` error mapping).
2. `registration::register_inproc_server(CLSID_SubornoTextService, ...)` —
   COM class registration.
3. `registration::register_tsf_profile(CLSID_SubornoTextService,
   GUID_LANGUAGE_PROFILE_SUBORNO_BENGALI, MAKELANGID(0x45, 0x02) /* bn-BD */,
   L"Suborno (Bengali, Bangladesh)", module_path)` — text-service +
   language-profile registration.
4. **Atomic failure handling**: if profile registration fails, the class
   registration is unregistered before returning the error (no half-registered
   state).
5. `DllUnregisterServer` performs the symmetric teardown (profile first, then
   class). `DllInstall(TRUE)` delegates to `DllRegisterServer` (regsvr32
   `/i` compatibility).

Identifiers are compile-time constants in `guids.cpp`/`guids.hpp`
(`CLSID_SubornoTextService`, `GUID_LANGUAGE_PROFILE_SUBORNO_BENGALI`) —
unchanged from the v19 baseline (no identifier changes permitted or made).

## How the runner actually executes this (BLOCKED evidence)

1. `scripts/test-windows-installer.ps1` deploys the DLL to
   `ProgramData/components/tsf/{x64,x86}` and invokes the service with `/s`
   (silent register) — real `DllRegisterServer` execution on a real Windows
   machine, with post-install assertions on the deployed files.
2. The in-app path (`application.cpp`) calls
   `ITfInputProcessorProfiles::IsEnabledLanguageProfile` /
   `EnableLanguageProfile` — real TSF profile API usage compiled into the app
   and exercised by `suborno.windows.app_smoke`.

## Non-claims

- Registration was **not executed** in this sandbox (Linux; no Windows).
- The hosted-runner `windows-2022` environment supports `regsvr32`-style
  registration under the runner account; profile visibility for interactive
  logon sessions is part of the manual-protocol checklist (session-scoped
  profile activation) and is documented, not asserted.
