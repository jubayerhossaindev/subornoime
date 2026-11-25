@echo off
setlocal EnableDelayedExpansion
:: ============================================================================
:: MSVC-WARNINGS-REPAIR-02 — source-sync pre-flight check
::
:: PURPOSE
::   The 2026-11-25 build attempt on this machine failed with 103 compiler
::   errors because the working tree here predates the MSVC repairs that are
::   already present in the verified reference tree (commits 59196f7, 6715513,
::   4f7b82b). This script checks, by literal content markers, whether the
::   repaired sources have actually landed in THIS directory.
::
:: HOW TO RUN
::   cd /d F:\my-windows-app\suborno\SUBORNOIME\SUBORNOIME
::   path\to\verify_source_sync.cmd
::
:: RESULT
::   Prints SYNC-OK or SYNC-STALE per marker and one overall verdict.
::   Exit code = number of stale markers (0 = fully synced).
:: ============================================================================

set "STALE=0"
set "OKN=0"

if not exist "CMakeLists.txt" (
    echo SYNC-STALE: run this script from the repository root ^(CMakeLists.txt not found here^).
    exit /b 99
)

call :check "src/platform/windows/app/application.cpp"  "#include <commctrl.h>"        "application.cpp: <commctrl.h> include"
call :check "src/platform/windows/app/application.cpp"  "#include <windowsx.h>"        "application.cpp: <windowsx.h> include"
call :check "src/platform/windows/app/application.cpp"  "ui_accessibility.hpp"         "application.cpp: ui_accessibility.hpp include"
call :check "src/platform/windows/app/topbar.cpp"       "#include <commctrl.h>"        "topbar.cpp: <commctrl.h> include"
call :check "src/platform/windows/app/topbar.cpp"       "InitCommonControlsEx"         "topbar.cpp: InitCommonControlsEx call"
call :check "src/platform/windows/app/toast_window.cpp" "struct ToastWindow::Impl"     "toast_window.cpp: nested Impl definition"
call :check "src/platform/windows/app/menu_popup.cpp"   "state->mouse_tracked"         "menu_popup.cpp: pointer member access"
call :check "src/platform/windows/app/osk_window.cpp"   "INPUT inputs[4]"              "osk_window.cpp: INPUT[4] event capacity"
call :check "src/platform/windows/tsf/candidate_list_ui_element.cpp" "namespace ui = suborno::platform::windows::app::ui;" "candidate_list: ui namespace alias"
call :check "src/platform/windows/tsf/candidate_list_ui_element.cpp" "#include <windowsx.h>" "candidate_list: <windowsx.h> include"
call :check "src/platform/windows/tsf/candidate_list_ui_element.cpp" "DefWindowProcW never dispatches timer callbacks" "candidate_list: WM_TIMER return fix + corrected comment"
call :check "cmake/modules/SubornoBuildOptions.cmake"   "/external:W4"                 "SubornoBuildOptions.cmake: /external:W4 policy"
call :check "cmake/modules/SubornoBuildOptions.cmake"   "/analyze:external-"           "SubornoBuildOptions.cmake: /analyze:external- policy"
call :check "tests/unit/tools_coverage_test.cpp"        "#ifndef _WIN32"               "tools_coverage_test.cpp: sys/wait.h guarded"
call :check "tests/unit/ui_functional_audit_test.cpp"   "C6287"                        "ui_functional_audit_test.cpp: C6287 single-find fix"
call :check "include/suborno/platform/windows/app/ui_accessibility.hpp" "ElementSource" "ui_accessibility.hpp present"
rem ---- V2 markers: real-Windows test-suite repairs (2026-11-25) ----
call :check "tests/CMakeLists.txt"                      "suborno_run_tests"            "tests/CMakeLists.txt: suborno_run_tests (MSB5004 fix)"
call :check "tests/unit/tools_coverage_test.cpp"        "_popen spawns"                "tools harness: cmd /c outer-quote wrap"
call :check "src/settings/settings.cpp"                 "std::ifstream input(path_, std::ios::binary)" "settings loader: binary mode (CRLF fix)"
call :check "tests/windows/windows_build_smoke_test.cpp" "EXPECT_EQ(default_pages[1], 5U)" "TSF contract: default fixed-height page starts"
call :check "tests/unit/coverage_api_settings_test.cpp"  "AtomicQuarantineReplacesAnExistingTargetOnWindowsRename" "quarantine tests: real rename semantics"
rem ---- V3 markers: Zero Bengali Language Pack Dependency standalone mode (2026-11-25) ----
call :check "src/platform/windows/standalone/typing_controller.hpp" "class TypingController" "standalone: portable typing policy core"
call :check "src/platform/windows/standalone/keyboard_hook.cpp"     "LLKHF_INJECTED" "standalone hook: injected-event feedback-loop guard"
call :check "src/platform/windows/standalone/unicode_injector.cpp"  "KEYEVENTF_UNICODE" "standalone injector: KEYEVENTF_UNICODE pipeline"
call :check "src/platform/windows/standalone/standalone_main.cpp"   "Suborno.Standalone.SingleInstance" "standalone host: single-instance mutex"
call :check "src/platform/windows/standalone/CMakeLists.txt"        "SubornoStandalone" "standalone target wired into the build"
call :check "src/platform/windows/CMakeLists.txt"                   "SUBORNO_BUILD_STANDALONE" "platform build: standalone option"
call :check "tests/unit/standalone_typing_controller_test.cpp"      "StandaloneTypingController" "controller policy unit tests registered"
call :check "install_standalone.cmd"                                "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run" "standalone installer: HKCU autostart"
rem ---- V4 markers: bundled Bangla fonts (2026-11-25) ----
call :check "src/platform/windows/standalone/preview_font_choice.hpp" "resolve_preview_font" "fonts: header-only preview font policy"
call :check "src/platform/windows/standalone/font_loader.cpp"       "AddFontResourceExW" "fonts: FR_PRIVATE per-process loader"
call :check "src/platform/windows/standalone/standalone_main.cpp"   "BundledFonts" "standalone host: bundled font bootstrap"
call :check "install_fonts.cmd"                                     "(TrueType)" "fonts: optional per-user install (HKCU, no admin)"
call :check "assets/fonts/FONT-CREDITS.md"                          "SIL Open Font License" "fonts: OFL credits + provenance"
call :check "assets/fonts/fonts_manifest.csv"                       "sha256" "fonts: byte-exact integrity manifest"

echo.
if "%STALE%"=="0" (
    echo SYNC-VERDICT: OK — all 35 markers present. This tree contains the repaired
    echo sources, the zero-dependency standalone mode, and the bundled fonts.
    echo Next: run install_suborno_ime.cmd ^(admin^) to register the TSF IME,
    echo       run_final_verification.cmd for the full evidence run, then
    echo       verify_zero_dep.cmd + install_standalone.cmd for the standalone mode,
    echo       and install_fonts.cmd once to make the bundled faces system-visible.
) else (
    echo SYNC-VERDICT: STALE — %STALE% marker^(s^) missing. This tree predates the repairs.
    echo Next: extract SUBORNOIME_SOURCE_ZERO_DEP_V4.zip OVER this directory
    echo       ^(keep the build\ folder^), then re-run verify_source_sync.cmd.
)
endlocal & exit /b %STALE%

:: --- helper -----------------------------------------------------------------
:check
set "FILE=%~1"
set "NEEDLE=%~2"
set "LABEL=%~3"
if not exist "%FILE%" (
    echo SYNC-STALE: %LABEL%  ^(file missing: %FILE%^)
    set /a STALE+=1
    goto :eof
)
findstr /C:"%NEEDLE%" "%FILE%" >nul 2>&1
if errorlevel 1 (
    echo SYNC-STALE: %LABEL%
    set /a STALE+=1
) else (
    echo SYNC-OK  : %LABEL%
    set /a OKN+=1
)
goto :eof
