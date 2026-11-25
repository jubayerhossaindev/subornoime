# MSVC Build Repair 01 — Windows x64 Release: Errors, Warnings & Code-Analysis Findings

Status: **ALL 9 diagnostic areas repaired and verified** (2026-11-24, repo commit
`afaefb5` was the build under repair; the fixes are the next commit).
Scope: exactly the diagnostics in the Windows MSVC build log of
`cmake --build --preset windows-msvc-release --parallel 1`
(MSVC 14.44.35207, MSBuild 17.14.60, Windows SDK 10.0.26100.0, `/W4 /utf-8`,
`/analyze` via `SUBORNO_ENABLE_MSVC_ANALYSIS=ON`, C++20, `x64-windows-static`).

---

## A. Executive summary

| # | Area | Diagnostics | Root cause (one line) | Fixed |
|---|------|-------------|----------------------|-------|
| A1 | `application.cpp` `control_automation_id` | C4430, C2143, C2065 ×3, C2440 ×6, C2046 ×6, C2660 | `UiControl` used as a **parameter type** at :1252 while the nested struct is declared at :1948 (parameter types need the declaration at the point of declaration; member-function bodies do not) | Moved the function (with its comment) after the `UiControl` struct |
| A2 | `show_toast` callers ×7 | C2664 ×7 | `show_toast`'s first parameter was declared `const wchar_t*` but the body passes it to `ui::text(const char* key, …)` and **all 7 call sites pass narrow i18n keys** — the declaration itself was the defect | Parameter retyped to `const char* key`; call convention unchanged |
| A3 | `ui::text` call at :3224 | C2664 | Same root cause (the call inside `show_toast` received the mis-typed `key`) | Fixed by A2 |
| B1 | `toast_window.cpp` PImpl | C2027 ×15, C4150 ×2, C2664 ×2 | Header declares nested `ToastWindow::Impl`; cpp defined a *different* anonymous-namespace `Impl`, so `new Impl` inside `ToastWindow::create` resolved to the **undefined** nested type | Defined the real nested `ToastWindow::Impl` (namespace scope); helpers/proc take `ToastWindow::Impl` |
| B2 | `CreateWindowExW` "11 args" | C2660 | **Cascade**, not an independent bug: the dropped 3rd argument is `impl_->window_name().c_str()`, which itself failed on the incomplete type (C2027 at :316,39). The source already passes the correct 12 arguments | No code change; resolved by B1 |
| C | `menu_popup.cpp:301` | C2228 (+ identical bugs at :304/:310 that MSVC suppressed) | `state` is `MenuPopupState*` in the window proc; three dot-accesses | `state->mouse_tracked` ×3 |
| D1 | `topbar.cpp` helpers | C3861 ×3, C2593 | `tooltip_text` (:163), `update_tooltip_layout` (:307), `handle_click` (:531) are defined *after* the `Impl` struct that calls them at :91/:140/:233 — free functions must be declared before their point of use; the existing forward declaration sat *after* the uses | Forward declarations for all three placed before `struct Topbar::Impl` |
| D2 | `topbar.cpp` tooltip declarations | C2065 ×21 (TOOLINFO, TTM_*, TOOLTIPS_CLASS, TTS_*, TTF_SUBCLASS) | `<commctrl.h>` never included. **Plus a latent typo**: `TTS_ALWAYSTOP` does not exist in the SDK — the style is `TTS_ALWAYSTIP` (masked by the missing include) | Added `<commctrl.h>`; fixed the typo; added the MSDN-documented `InitCommonControlsEx(ICC_BAR_CLASSES)` before tooltip creation and linked `Comctl32.lib` |
| E1 | `ui_accessibility.cpp` interface names | C3668 ×2 | `get_HostProvider` → must be `get_HostRawElementProvider`; `GetPropertyValues` → must be `GetPropertyValue` | Renamed to the exact `IRawElementProviderSimple` signatures |
| E2 | `ui_accessibility.cpp` abstract class | C2259 ×4 | Cascade of E1 (the two unoverridden pure methods) | Resolved by E1 |
| E3 | `SafeArrayPutElement` | C2664 ×3 | `const double values[4]` / `const LONG first/second` produce const element pointers; the API takes `void*` | Removed `const` on the local mutables; added `HRESULT` checks with `SafeArrayDestroy` cleanup on failure |
| E4 | `delete this` C5205 | C5205 | Cascade of E1 (deleting through pointer to abstract class) | Resolved by E1 — `Release()`'s `delete this` on the now-concrete `final` class follows the standard COM lifetime contract; no virtual destructor added to any interface |
| F | `osk_window.cpp` SendInput | C6386, C6385 | `INPUT inputs[3]` (120 B on x64) but the shifted path writes/reads **4** events (shift↓, key↓, key↑, shift↑ = 160 B) | `INPUT inputs[4]` with count comments; `SendInput(count, …, sizeof(INPUT))` unchanged |
| G | `candidate_list_ui_element.cpp` | C2653 ×6, C2660, C3861 ×8 | The `ui` theme namespace is `…::windows::app::ui`; this TU is in `…::windows` (sibling scope, not on the unqualified-lookup chain). `GET_X/Y_LPARAM` need `<windowsx.h>` | Namespace alias `namespace ui = suborno::platform::windows::app::ui;` + `#include <windowsx.h>`; `ui::scaled(UINT, int)` is the existing DPI API (no new overload) |
| H | `tools_coverage_test.cpp:22` | C1083 | `#include <sys/wait.h>` was the only un-guarded POSIX include — every other POSIX/Windows split in the file was already `#ifndef _WIN32`-guarded, and the Windows path (`_popen`/`_pclose`, `process.h`) already exists | Moved the include into the existing POSIX guard; all assertions unchanged |
| I | `ui_functional_audit_test.cpp:64` | C6287 | The two `find(…) != npos` subexpressions have an identical call shape (REDUNDANTTEST); the logic itself is correct (two distinct Bengali lead-byte prefixes) | Rewritten as one `find` over both prefixes (range-for + `string_view`) — byte-identical matching semantics, no duplicated sibling subexpression, assertion not weakened |

Note: `toast_window.hpp` also gained one change — the nested `struct Impl;`
forward declaration moved to the `public` section (with a comment), because the
TU-local helpers (`measure`, `place_above_tray`, `paint_toast`, the window
proc) now take `ToastWindow::Impl&`. This mirrors the established pattern in
`topbar.hpp` ("The name is public because TU-local helpers there take Impl
references").

---

## B. Files changed (11)

| File | Change |
|------|--------|
| `src/platform/windows/app/application.cpp` | A1 move, A2 parameter type (+ doc comment) |
| `src/platform/windows/app/toast_window.cpp` | B1: define nested `ToastWindow::Impl`; helpers/proc retyped |
| `include/suborno/platform/windows/app/toast_window.hpp` | `struct Impl;` forward declaration moved to public (topbar.hpp pattern) |
| `src/platform/windows/app/menu_popup.cpp` | C: 3 × `state->mouse_tracked` |
| `src/platform/windows/app/topbar.cpp` | D1 forward decls, D2 `<commctrl.h>` + `InitCommonControlsEx(ICC_BAR_CLASSES)` + `TTS_ALWAYSTIP` typo fix |
| `src/platform/windows/app/ui_accessibility.cpp` | E1 renames, E3 SafeArray fixes + HRESULT checks |
| `src/platform/windows/app/osk_window.cpp` | F: `INPUT inputs[4]` |
| `src/platform/windows/tsf/candidate_list_ui_element.cpp` | G: `<windowsx.h>` include + `ui` namespace alias |
| `src/platform/windows/CMakeLists.txt` | `Comctl32.lib` added to `suborno_app` (required import of `InitCommonControlsEx`) |
| `tests/unit/tools_coverage_test.cpp` | H: POSIX include moved under the existing guard |
| `tests/unit/ui_functional_audit_test.cpp` | I: `contains_bengali` rewritten to a single-find loop |

Not changed (on purpose): toolchain selection, SDK target, generator, vcpkg
triplet, MSVC runtime, C++ standard, dependency versions, build presets,
`candidate_list_ui_element.cpp`'s own `scaled(int)` member (no overload added),
any pre-existing test/coverage, and the clang-visible-but-MSVC-invisible nits
listed in §E.

---

## C. Verification evidence

Executed in the Linux CI sandbox (no MSVC available). Honesty statement:
**MSVC-specific behavior could not be executed here**; verification used
(1) a cross syntax-check of every Windows TU against **real mingw-w64 Windows
SDK headers** (clang 21 via the `ziglang` toolchain, `--target=x86_64-windows-gnu`,
`-DUNICODE -D_UNICODE -DNOMINMAX`, C++20, mirroring the project's definitions),
(2) the project's portable test suite, and (3) targeted static sweeps. The
final gate is your Windows rebuild (§D).

1. **clang-format** (project `.clang-format`): all 10 modified C++ files +
   `toast_window.hpp` pass `--dry-run --Werror` → `FORMAT_OK`.
2. **Windows-header syntax checks** (the strongest sandbox-available proof):
   - `application.cpp`, `menu_popup.cpp`, `toast_window.cpp`, `topbar.cpp`,
     `ui_accessibility.cpp`, `osk_window.cpp`, `app_paths.cpp`, `main.cpp`
     → compile **clean** (no errors, no warnings at `-Wall -Wextra` from
     project code).
   - `candidate_list_ui_element.cpp`: full-TU compile is blocked by a
     **mingw header gap** (bundled `msctf.h` has no `ITfCandidateListUIElement(Behavior)`
     — the real SDK has them, and your MSVC log proves the header compiled:
     the MSVC errors for this file start at :660). Verified instead with a
     synthetic TU in the exact namespace context that repeats the precise
     flagged call forms (`ui::scaled(dpi_, slide_offset_)`,
     `ui::theme_surface/separator/accent_fill/on_accent/text()`,
     `GET_X/Y_LPARAM`) against the real `ui_theme.hpp` + `windowsx.h`
     → compiles clean. The harness was itself sanity-checked: it correctly
     rejects a private `Impl` access.
   - The syntax check caught and I fixed **two real defects the MSVC log could
     not isolate**: the `ToastWindow::Impl` **access** violation (private
     nested name used by TU-local helpers — a hard error that MSVC would emit
     after B1) and the **`TTS_ALWAYSTOP` typo** (nonexistent style; MSVC
     reported it only as a cascade of the missing include).
   - Unmodified TUs blocked purely by mingw gaps (documented toolchain
     artifacts, proven fine on MSVC by your log): `text_service.cpp`,
     `class_factory.cpp`, `dll_main.cpp`, `module_smoke.cpp` (`msctf.h`
     interfaces + a zig-libc++/mingw `new.h` collision), `guids.cpp`
     (`DEFINE_GUID` linkage macro difference), `tsf_profiles.cpp` (mingw
     lacks `GUID_TFCAT_TIPCAP_UIELEMENTENABLED/IMMERSIVESUPPORT`),
     `text_edit_session.cpp` (clang-only nothrow-exception-spec note).
3. **Portable regression suite** (fresh `build/linux-audit`, Ninja/Release):
   - Build: **BUILD_OK** (all targets incl. both test executables).
   - `suborno_unit_tests`: **633 passed / 0 failed** (baseline 633).
   - `suborno_integration_tests`: **2 passed / 0 failed** (baseline 2).
   - `ctest`: `100% tests passed out of 2`.
   - FIND-06 regression guard `UIAuditFontFamily.*`: **2/2 PASSED**.
4. **Leftover-diagnostic sweep**: `TTS_ALWAYSTOP`, `GetPropertyValues`,
   `get_HostProvider`, `inputs[3]`, unguarded `state.mouse_tracked` — all gone.
   `tests/unit/tools_unix_injection_test.cpp` also includes `sys/wait.h` but is
   registered **only under `if(UNIX AND NOT APPLE)`** — never compiled on
   Windows; left untouched (documented POSIX-only scenario).
5. **No warning-disabling anywhere**: no `/W0`, no `#pragma warning(disable:)`,
   no `/analyze` suppression added; no `const_cast`; no `reinterpret_cast`
   added to any fix (the pre-existing window-proc casts are untouched).

---

## D. Expected result of your Windows rebuild

Re-run on your machine:

```
cmake --preset windows-msvc
cmake --build --preset windows-msvc-release --parallel 1
```

Expected: `suborno_app.vcxproj`, `suborno_windows_tsf.vcxproj` and
`suborno_unit_tests.vcxproj` compile with **0 errors** and no new `/W4` or
`/analyze` diagnostics (the analysis warnings C6385/C6386 and C6287 are fixed
at their root, not suppressed). `suborno_app` now links `Comctl32.lib` (needed
by `InitCommonControlsEx`). If MSVC still reports anything, send the log and
this report will be extended — the fixes are all evidence-based against the
documented API contracts, so residual risk is concentrated in toolchain-version
differences, not design.

---

## E. Pre-existing clang-only observations (out of scope, untouched)

Under `clang -Wall -Wextra` (not MSVC diagnostics; your log shows none of
these; lines not touched by this repair):

- `application.cpp:292` — `open_uri` unused in some configurations.
- `application.cpp:2889` — a lambda captures `this` without using it.
- `application.cpp:3412` — size_t/int sign comparison.

They predate this repair and fixing them would be unrelated refactoring
(prohibited by the scope rules). Recorded here for transparency.

---

## F. Root-cause method notes (why the fixes are minimal and safe)

- **A1** follows the C++ rule that *parameter types* need the declaration at
  the point of declaration while *member function bodies* are complete-class
  contexts — that is exactly why the compiler accepted `UiControl` inside
  `element_info` (body, :1359) but rejected it in the parameter list (:1252).
  The one-line diagnostic evidence: MSVC's C4430 pointed at the `const` of
  `const UiControl&` (col 40) and C2143 at the `&` (col 55).
- **A2** is not a workaround: `ui::text(const char* key, const wchar_t*
  fallback)` is the established localization API (ui_theme.hpp:613), every one
  of the 7 call sites passes a narrow key, and the keys
  (`toast.learnt_cleared`, `toast.settings_saved`, `toast.font_changed`,
  `toast.encoding_bijoy/unicode`, `toast.layout_switched`,
  `toast.profile_enabled/disabled`, `toast.language_changed`) all exist in
  `data/i18n/{en,bn}/ui.json`. No Bengali text flows through the narrow
  parameter — only ASCII keys.
- **B1** keeps one single PImpl type end-to-end (owner, `GWLP_USERDATA`,
  delete sites, helper signatures) as required by the C4150/C2027 contract;
  the destructor `~ToastWindow()` and `shutdown()` still delete in the TU
  where `Impl` is complete.
- **D2** follows the MSDN "Creating Common Controls" requirement ("To ensure
  that the common control DLL is loaded, include the InitCommonControlsEx
  function in your application"); `ICC_BAR_CLASSES` is the flag set that
  covers tooltips. The one-time lambda-init is idempotent and failure keeps
  the documented "tooltips are optional UI" behavior (nullptr check).
- **E1/E4** restore the exact `IRawElementProviderSimple` contract
  (`GetPropertyValue(PROPERTYID, VARIANT*)`, `get_HostRawElementProvider`
  propget, `STDMETHODCALLTYPE` via `IFACEMETHODIMP`) — `ElementProvider`
  becomes concrete again, so `delete this` in `Release()` deletes the correct
  concrete type and C5205 disappears without any virtual-destructor hack.
- **E3** keeps the SAFEARRAY VARTYPEs (`VT_R8`, `VT_I4`) and indexing intact;
  ownership transfers to the caller only on success; failure paths destroy
  the array.
- **F** verifies the event-count invariant: max 4 INPUTs (shifted key), 2
  otherwise; `SendInput(count, inputs, sizeof(INPUT))` reads only initialized
  elements.
- **G** uses a namespace alias instead of editing 6 call sites or inventing a
  namespace; `ui::scaled(UINT, int)` is the same DPI helper topbar/toast/menu
  already use.
