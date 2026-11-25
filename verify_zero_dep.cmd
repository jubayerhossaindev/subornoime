@echo off
setlocal EnableDelayedExpansion
:: ============================================================================
:: SUBORNO — Zero Bengali Language Pack Dependency :: acceptance gates
::
:: Runs AFTER (or independently of) run_final_verification.cmd and proves the
:: standalone mode end to end:
::   GATE 1  source-sync pre-flight (verify_source_sync.cmd, incl. the V3
::           standalone markers) — exit code 0 required
::   GATE 2  configure preset windows-msvc (cached configure is cheap)
::   GATE 3  build target suborno_standalone (Release, MSVC v143)
::   GATE 4  artifact build\windows-msvc\Release\SubornoStandalone.exe exists
::   GATE 5  standalone typing-policy unit tests (StandaloneTypingController*)
::           run directly from the Release unit-test binary — 0 failures
::   GATE 6  aggregate Release unit suite still registers (suborno.unit)
::   GATE 7  install/uninstall helper scripts present + HKCU-safe (no admin)
::   GATE 8  bundled Bangla fonts: 10 TTFs + OFL texts + credits + manifest in
::           assets\fonts, and the POST_BUILD copy next to the Release exe
::
:: Evidence lands in evidence\ZERO-DEPENDENCY\; VERDICT.txt holds the summary.
:: Exit code = number of failed gates (0 = all pass).
:: ============================================================================
set "REPO=%CD%"
set "EV=%REPO%\evidence\ZERO-DEPENDENCY"
set "BUILD=build\windows-msvc"
set "FAILTOTAL=0"

if not exist "CMakeLists.txt" (
    echo SYNC-STALE: run this script from the repository root ^(CMakeLists.txt not found^).
    exit /b 99
)
if not exist "%EV%" mkdir "%EV%"
if exist "%EV%\VERDICT.txt" del "%EV%\VERDICT.txt" >nul 2>&1

echo ==================================================================
echo SUBORNO zero-dependency acceptance gates
echo Repository: %REPO%
echo Evidence  : %EV%
echo ==================================================================

:: ---------------------------------------------------------------------------
echo.
echo [GATE 1] Source-sync pre-flight (verify_source_sync.cmd)
call "%REPO%\verify_source_sync.cmd" > "%EV%\01_source_sync.txt" 2>&1
set SYNCRC=%errorlevel%
type "%EV%\01_source_sync.txt"
if not "%SYNCRC%"=="0" (
    echo   FAIL: verify_source_sync.cmd rc=%SYNCRC%
    set /a FAILTOTAL+=1
) else (
    echo   PASS: all source markers present
)

:: ---------------------------------------------------------------------------
echo.
echo [GATE 2] Configure preset windows-msvc
cmake --preset windows-msvc > "%EV%\02_configure.txt" 2>&1
if errorlevel 1 (
    echo   FAIL: configure failed — see 02_configure.txt
    set /a FAILTOTAL+=1
    goto :write_verdict
)
echo   PASS: configure completed (02_configure.txt)

:: ---------------------------------------------------------------------------
echo.
echo [GATE 3] Build target suborno_standalone (Release)
cmake --build --preset windows-msvc-release --target suborno_standalone --parallel 1 > "%EV%\03_build_standalone.txt" 2>&1
if errorlevel 1 (
    echo   FAIL: standalone build failed — see 03_build_standalone.txt
    set /a FAILTOTAL+=1
    goto :write_verdict
)
findstr /C:"warning C" "%EV%\03_build_standalone.txt" > "%EV%\03_warnings.txt" 2>&1
for /f %%N in ('type "%EV%\03_warnings.txt" ^| find /c /v ""') do set WARNN=%%N
echo   PASS: standalone built (03_build_standalone.txt); compiler warnings in project sources: !WARNN! (review 03_warnings.txt)

:: ---------------------------------------------------------------------------
echo.
echo [GATE 4] Artifact check
if exist "%BUILD%\Release\SubornoStandalone.exe" (
    echo   PASS: %BUILD%\Release\SubornoStandalone.exe
) else (
    echo   FAIL: missing %BUILD%\Release\SubornoStandalone.exe
    set /a FAILTOTAL+=1
)

:: ---------------------------------------------------------------------------
echo.
echo [GATE 5] Standalone typing-policy tests (StandaloneTypingController*)
"%BUILD%\Release\suborno_unit_tests.exe" --gtest_filter=StandaloneTypingController* > "%EV%\05_controller_tests.txt" 2>&1
if errorlevel 1 (
    echo   FAIL: StandaloneTypingController subset failed — see 05_controller_tests.txt
    set /a FAILTOTAL+=1
) else (
    echo   PASS: controller policy tests passed — see 05_controller_tests.txt
)
findstr /C:"PASSED" "%EV%\05_controller_tests.txt" >> "%EV%\05_controller_tests_summary.txt" 2>&1

:: ---------------------------------------------------------------------------
echo.
echo [GATE 6] Aggregate unit suite registers and passes (includes controller)
ctest --test-dir %BUILD% -C Release -R suborno.unit --output-on-failure > "%EV%\06_ctest_unit.txt" 2>&1
set CTRC=%errorlevel%
echo   ctest suborno.unit rc=%CTRC% (0 required)
if not "%CTRC%"=="0" set /a FAILTOTAL+=1

:: ---------------------------------------------------------------------------
echo.
echo [GATE 7] Helper scripts present
for %%S in (install_standalone.cmd uninstall_standalone.cmd) do (
    if exist "%REPO%\%%S" (
        echo   PASS: %%S present
    ) else (
        echo   FAIL: %%S missing
        set /a FAILTOTAL+=1
    )
)
findstr /C:"HKCU" "%REPO%\install_standalone.cmd" >nul 2>&1
if errorlevel 1 (
    echo   FAIL: install_standalone.cmd is not HKCU-scoped
    set /a FAILTOTAL+=1
) else (
    echo   PASS: install_standalone.cmd is HKCU-scoped (no admin required)
)

:: ---------------------------------------------------------------------------
echo.
echo [GATE 8] Bundled Bangla fonts (assets\fonts + build copy)
set "FONTN=0"
if exist "%REPO%\assets\fonts" (
    for /f %%N in ('dir /b "%REPO%\assets\fonts\*.ttf" 2^>nul ^| find /c /v ""') do set FONTN=%%N
)
if %FONTN% GEQ 10 (
    echo   PASS: %FONTN% bundled .ttf files in assets\fonts
) else (
    echo   FAIL: assets\fonts contains %FONTN% .ttf files — 10 required ^(5x Hind Siliguri, 3x Baloo Da 2, 2x Noto Sans Bengali^)
    set /a FAILTOTAL+=1
)
for %%F in ("assets\fonts\FONT-CREDITS.md" "assets\fonts\fonts_manifest.csv" "assets\fonts\OFL-HindSiliguri.txt" "assets\fonts\OFL-BalooDa2.txt" "assets\fonts\OFL-NotoSansBengali.txt") do (
    if exist "%REPO%\%%~F" (
        echo   PASS: %%~F present
    ) else (
        echo   FAIL: %%~F missing
        set /a FAILTOTAL+=1
    )
)
if exist "%BUILD%\Release\SubornoStandalone.exe" (
    set "BFONTN=0"
    for /f %%N in ('dir /b "%BUILD%\Release\fonts\*.ttf" 2^>nul ^| find /c /v ""') do set BFONTN=%%N
    if !BFONTN! GEQ 10 (
        echo   PASS: POST_BUILD font copy present next to the Release exe
    ) else (
        echo   FAIL: build\windows-msvc\Release\fonts has !BFONTN! .ttf files — rebuild suborno_standalone so POST_BUILD runs
        echo         ^(GATE 3 rebuild triggers the copy_directory POST_BUILD step^)
        set /a FAILTOTAL+=1
    )
) else (
    echo   NOTE: Release exe not built yet — build-output font copy not checked.
)
findstr /C:"SIL Open Font License" "%REPO%\assets\fonts\FONT-CREDITS.md" >nul 2>&1
if errorlevel 1 (
    echo   FAIL: FONT-CREDITS.md does not name the OFL license
    set /a FAILTOTAL+=1
) else (
    echo   PASS: OFL license obligations documented in FONT-CREDITS.md
)

:: ---------------------------------------------------------------------------
:write_verdict
echo.
echo ==================================================================
if "%FAILTOTAL%"=="0" (
    set VERDICT=ALL GATES PASS — zero-dependency standalone mode accepted
) else (
    set VERDICT=FAILTOTAL=!FAILTOTAL! gate(s) FAILED — inspect evidence\ZERO-DEPENDENCY\
)
echo VERDICT: !VERDICT!
echo ==================================================================
(
  echo SUBORNO zero-dependency acceptance verdict
  echo Date: %DATE% %TIME%
  echo Repository: %REPO%
  echo Result: !VERDICT!
) > "%EV%\VERDICT.txt" 2>&1
echo Verdict written to %EV%\VERDICT.txt
endlocal & exit /b %FAILTOTAL%
