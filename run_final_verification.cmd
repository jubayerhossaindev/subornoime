@echo off
setlocal EnableDelayedExpansion
:: ============================================================================
:: MSVC-WARNINGS-REPAIR-02 — Final Windows verification runbook
::
:: HOW TO RUN
::   1. Place this file anywhere (it does not need to live in the repository).
::   2. A VS developer prompt is NOT required; CMake locates the Visual
::      Studio 17 2022 installation itself.
::   3. cd /d F:\my-windows-app\suborno\SUBORNOIME\SUBORNOIME
::   4. path\to\run_final_verification.cmd
::
:: The script executes the task commands verbatim and captures every log into
:: evidence\MSVC-WARNINGS-REPAIR-02\. Every gate prints PASS/FAIL and the
:: final verdict lands in evidence\MSVC-WARNINGS-REPAIR-02\VERDICT.txt.
:: Nothing is auto-suppressed; a gate failure leaves its evidence on disk.
:: ============================================================================

set "REPO=%CD%"
set "EV=%REPO%\evidence\MSVC-WARNINGS-REPAIR-02"
set "SUMMARY=%EV%\VERDICT.txt"
set "BUILD=build\windows-msvc"
set "FAILTOTAL=0"
:: Defaults so the verdict block is meaningful even when an early step jumps
:: straight to :write_verdict before STEP 4/6/8 ran.
set "ERRN=skipped"
set "C4715N=skipped"
set "C28182N=skipped"
set "D9007N=skipped"
set "WARNN=skipped"
set "NOTRUN=skipped"

if not exist "%EV%" mkdir "%EV%"
if exist "%SUMMARY%" del "%SUMMARY%" >nul 2>&1

echo ==================================================================
echo MSVC-WARNINGS-REPAIR-02 final Windows verification
echo Repository: %REPO%
echo Evidence  : %EV%
echo ==================================================================

:: ---------------------------------------------------------------------------
:: STEP 0 — environment + git working-tree capture
:: ---------------------------------------------------------------------------
echo.
echo [STEP 0] Environment and git working-tree capture
cmake --version > "%EV%\00_cmake_version.txt" 2>&1
git rev-parse HEAD > "%EV%\00_git_head.txt" 2>&1
git status --short > "%EV%\00_git_status_short.txt" 2>&1
git status > "%EV%\00_git_status_full.txt" 2>&1
git diff b94252e --stat > "%EV%\00_git_diff_stat_vs_b94252e.txt" 2>&1
git diff b94252e -- cmake/modules/SubornoBuildOptions.cmake src/platform/windows/tsf/candidate_list_ui_element.cpp > "%EV%\00_git_diff_vs_b94252e.txt" 2>&1
echo   git HEAD (00_git_head.txt):
type "%EV%\00_git_head.txt"
echo   git status --short (00_git_status_short.txt):
type "%EV%\00_git_status_short.txt"
echo   Working tree must show exactly the two intended modifications
echo   (candidate_list_ui_element.cpp, SubornoBuildOptions.cmake^).

:: ---------------------------------------------------------------------------
:: STEP 1 — configure preset (task command, verbatim)
:: ---------------------------------------------------------------------------
echo.
echo [STEP 1] cmake --preset windows-msvc
cmake --preset windows-msvc > "%EV%\01_configure.txt" 2>&1
if errorlevel 1 (
    echo   FAIL: configure failed — see 01_configure.txt
    set /a FAILTOTAL+=1
    goto :write_verdict
)
echo   PASS: configure completed (01_configure.txt)

:: ---------------------------------------------------------------------------
:: STEP 2 — clean-first Release build (task command, verbatim)
:: ---------------------------------------------------------------------------
echo.
echo [STEP 2] cmake --build --preset windows-msvc-release --clean-first --parallel 1
cmake --build --preset windows-msvc-release --clean-first --parallel 1 > "%EV%\02_build_release.txt" 2>&1
if errorlevel 1 (
    echo   FAIL: Release build failed — see 02_build_release.txt
    set /a FAILTOTAL+=1
    goto :write_verdict
)
findstr /C:"Build succeeded" "%EV%\02_build_release.txt" >nul 2>&1
if errorlevel 1 (
    echo   WARN: "Build succeeded" banner not found; inspect 02_build_release.txt
)
echo   PASS: Release build rc=0 (02_build_release.txt)

:: ---------------------------------------------------------------------------
:: STEP 3 — required build artifacts
:: ---------------------------------------------------------------------------
echo.
echo [STEP 3] Required Release artifacts
set ARTOK=1
for %%A in (
    "%BUILD%\Release\Suborno.exe"
    "%BUILD%\Release\suborno_windows_tsf.dll"
    "%BUILD%\Release\suborno_unit_tests.exe"
    "%BUILD%\Release\suborno_integration_tests.exe"
    "%BUILD%\Release\suborno_windows_tests.exe"
) do (
    if exist %%A (
        echo   PASS: %%~A
    ) else (
        echo   FAIL: missing %%~A
        set ARTOK=0
    )
)
if "!ARTOK!"=="0" set /a FAILTOTAL+=1

:: ---------------------------------------------------------------------------
:: STEP 4 — warning inventory over the complete build output
:: ---------------------------------------------------------------------------
echo.
echo [STEP 4] Compiler and Code Analysis warning inventory
findstr /S /C:"error C" "%EV%\02_build_release.txt" > "%EV%\04_errors.txt" 2>&1
for /f %%N in ('type "%EV%\04_errors.txt" ^| find /c /v ""') do set ERRN=%%N
echo   compiler errors          : !ERRN!  (must be 0)

findstr /C:"C4715" "%EV%\02_build_release.txt" > "%EV%\04_C4715.txt" 2>&1
for /f %%N in ('type "%EV%\04_C4715.txt" ^| find /c /v ""') do set C4715N=%%N
echo   C4715 occurrences        : !C4715N!  (must be 0)

findstr /C:"C28182" "%EV%\02_build_release.txt" > "%EV%\04_C28182.txt" 2>&1
for /f %%N in ('type "%EV%\04_C28182.txt" ^| find /c /v ""') do set C28182N=%%N
echo   C28182 occurrences       : !C28182N!  (must be 0 under the external-header policy)

:: D9007 would mean /external:W4 is missing and /external:I got ignored.
findstr /C:"D9007" "%EV%\02_build_release.txt" > "%EV%\04_D9007.txt" 2>&1
for /f %%N in ('type "%EV%\04_D9007.txt" ^| find /c /v ""') do set D9007N=%%N
echo   D9007 external-ignored   : !D9007N!  (must be 0 — /external:W4 must be active)

:: Complete inventory of every remaining warning line — do not suppress,
:: list them for review instead.
findstr /R /C:"warning C[0-9]" /C:"warning D[0-9]" "%EV%\02_build_release.txt" > "%EV%\04_all_warnings.txt" 2>&1
for /f %%N in ('type "%EV%\04_all_warnings.txt" ^| find /c /v ""') do set WARNN=%%N
echo   all remaining warnings   : !WARNN!  (review 04_all_warnings.txt; none may be new project-owned)
if not "!ERRN!"=="0" set /a FAILTOTAL+=1
if not "!C4715N!"=="0" set /a FAILTOTAL+=1
if not "!C28182N!"=="0" set /a FAILTOTAL+=1
if not "!D9007N!"=="0" set /a FAILTOTAL+=1

:: ---------------------------------------------------------------------------
:: STEP 5 — compiler configuration evidence (vcxproj + actual cl command lines)
:: ---------------------------------------------------------------------------
echo.
echo [STEP 5] Compiler / Code Analysis configuration evidence
:: 5a. Project files: every MSVC target must carry all four options.
for %%T in (suborno_app suborno_windows_tsf) do (
    findstr /C:"/analyze" "%BUILD%\src\platform\windows\%%T.vcxproj" > "%EV%\05_%%T.vcxproj.analyze.txt" 2>&1
    findstr /C:"/external:W4" "%BUILD%\src\platform\windows\%%T.vcxproj" >nul 2>&1
    if errorlevel 1 ( echo   FAIL: %%T.vcxproj missing /external:W4 & set /a FAILTOTAL+=1 ) else echo   PASS: %%T.vcxproj has /external:W4
    findstr /C:"/analyze:external-" "%BUILD%\src\platform\windows\%%T.vcxproj" >nul 2>&1
    if errorlevel 1 ( echo   FAIL: %%T.vcxproj missing /analyze:external- & set /a FAILTOTAL+=1 ) else echo   PASS: %%T.vcxproj has /analyze:external-
    findstr /C:"/external:I" "%BUILD%\src\platform\windows\%%T.vcxproj" >nul 2>&1
    if errorlevel 1 ( echo   FAIL: %%T.vcxproj missing /external:I & set /a FAILTOTAL+=1 ) else echo   PASS: %%T.vcxproj has /external:I
)
for %%T in (suborno_unit_tests suborno_integration_tests suborno_windows_tests) do (
    findstr /C:"/analyze" "%BUILD%\tests\%%T.vcxproj" > "%EV%\05_%%T.vcxproj.analyze.txt" 2>&1
    findstr /C:"/external:W4" "%BUILD%\tests\%%T.vcxproj" >nul 2>&1
    if errorlevel 1 ( echo   FAIL: %%T.vcxproj missing /external:W4 & set /a FAILTOTAL+=1 ) else echo   PASS: %%T.vcxproj has /external:W4
    findstr /C:"/analyze:external-" "%BUILD%\tests\%%T.vcxproj" >nul 2>&1
    if errorlevel 1 ( echo   FAIL: %%T.vcxproj missing /analyze:external- & set /a FAILTOTAL+=1 ) else echo   PASS: %%T.vcxproj has /analyze:external-
    findstr /C:"/external:I" "%BUILD%\tests\%%T.vcxproj" >nul 2>&1
    if errorlevel 1 ( echo   FAIL: %%T.vcxproj missing /external:I & set /a FAILTOTAL+=1 ) else echo   PASS: %%T.vcxproj has /external:I
)

:: 5b. Ground truth: the actual cl.exe command lines recorded by MSBuild
::     FileTracker (CL.command.1.tlog) after the clean-first rebuild.
dir /s /b "%BUILD%\CL.command.1.tlog" > "%EV%\05_tlog_files.txt" 2>&1
findstr /S /C:"/analyze" "%BUILD%\*.tlog" > "%EV%\05_tlog_analyze_hits.txt" 2>&1
:: NOTE: /analyze also matches /analyze:external- — the specific-option hits
:: are counted separately in 05_tlog_analyzeExternal_hits.txt below.
findstr /S /C:"/external:W4" "%BUILD%\*.tlog" > "%EV%\05_tlog_externalW4_hits.txt" 2>&1
findstr /S /C:"/analyze:external-" "%BUILD%\*.tlog" > "%EV%\05_tlog_analyzeExternal_hits.txt" 2>&1
findstr /S /C:"/external:I" "%BUILD%\*.tlog" > "%EV%\05_tlog_externalI_hits.txt" 2>&1
for /f %%N in ('type "%EV%\05_tlog_analyze_hits.txt" ^| find /c /v ""') do echo   tlog /analyze hits        : %%N
for /f %%N in ('type "%EV%\05_tlog_externalW4_hits.txt" ^| find /c /v ""') do echo   tlog /external:W4 hits    : %%N
for /f %%N in ('type "%EV%\05_tlog_analyzeExternal_hits.txt" ^| find /c /v ""') do echo   tlog /analyze:external- hits: %%N
for /f %%N in ('type "%EV%\05_tlog_externalI_hits.txt" ^| find /c /v ""') do echo   tlog /external:I hits     : %%N

:: 5c. The designated external root must be the vcpkg include tree and must
::     NOT contain any project-owned source or header directory.
powershell -NoProfile -Command "$files = Get-ChildItem -Path 'build\windows-msvc' -Recurse -Filter '*.tlog' | Select-Object -ExpandProperty FullName; $roots = @(); foreach ($f in $files) { $hits = Select-String -Path $f -Pattern '/external:I(\S+)' -AllMatches; foreach ($h in $hits) { foreach ($m in $h.Matches) { $roots += $m.Groups[1].Value } } }; $roots | Sort-Object -Unique | Set-Content 'evidence\MSVC-WARNINGS-REPAIR-02\05_external_roots.txt'; if (-not $roots) { Write-Output 'FAIL no /external:I found in any tlog'; exit 1 }; $bad = $roots | Where-Object { $_ -notmatch 'vcpkg_installed' -or $_ -match 'src[\\/]' -or $_ -match 'include[\\/]suborno' }; if ($bad) { Write-Output ('FAIL external roots outside vcpkg tree: ' + ($bad -join ', ')); exit 1 } else { Write-Output 'PASS every /external:I root is the vcpkg include tree; no project-owned path classified external' }" > "%EV%\05_external_root_check.txt" 2>&1
type "%EV%\05_external_root_check.txt"
findstr /C:"FAIL" "%EV%\05_external_root_check.txt" >nul 2>&1
if not errorlevel 1 set /a FAILTOTAL+=1

:: 5d. Collect MSVC Code Analysis artifacts as evidence.
dir /s /b "%BUILD%\*.nativecodeanalysis*.xml" > "%EV%\05_codeanalysis_files.txt" 2>&1
dir /s /b "%BUILD%\*.sarif" >> "%EV%\05_codeanalysis_files.txt" 2>&1

:: ---------------------------------------------------------------------------
:: STEP 6 — Release CTest (task commands, verbatim)
:: ---------------------------------------------------------------------------
echo.
echo [STEP 6] Release CTest
ctest --test-dir build\windows-msvc -N -C Release > "%EV%\06_ctest_release_N.txt" 2>&1
type "%EV%\06_ctest_release_N.txt"
ctest --test-dir build\windows-msvc -C Release --output-on-failure > "%EV%\06_ctest_release.txt" 2>&1
set CTRC=%errorlevel%
echo   ctest Release rc=%CTRC% (0 required)
if not "%CTRC%"=="0" set /a FAILTOTAL+=1
:: Every one of the four groups must be listed.
for %%G in (suborno.unit suborno.integration suborno.windows.tsf_contract suborno.windows.app_smoke) do (
    findstr /C:"%%G" "%EV%\06_ctest_release_N.txt" >nul 2>&1
    if errorlevel 1 ( echo   FAIL: %%G not registered in Release listing & set /a FAILTOTAL+=1 ) else echo   PASS: %%G registered
)
:: NOT RUN is not PASS: any "Not Run" line fails the gate.
findstr /C:"Not Run" "%EV%\06_ctest_release.txt" > "%EV%\06_ctest_release_notrun.txt" 2>&1
for /f %%N in ('type "%EV%\06_ctest_release_notrun.txt" ^| find /c /v ""') do set NOTRUN=%%N
echo   Release "Not Run" lines   : !NOTRUN!  (must be 0)
if not "!NOTRUN!"=="0" set /a FAILTOTAL+=1
findstr /C:"100%% tests passed" "%EV%\06_ctest_release.txt" >nul 2>&1
if errorlevel 1 ( echo   FAIL: Release did not report 100%% tests passed & set /a FAILTOTAL+=1 ) else echo   PASS: Release 100%% tests passed

:: ---------------------------------------------------------------------------
:: STEP 7 — Debug build (task command, verbatim)
:: ---------------------------------------------------------------------------
echo.
echo [STEP 7] cmake --build --preset windows-msvc-debug --parallel 1
cmake --build --preset windows-msvc-debug --parallel 1 > "%EV%\07_build_debug.txt" 2>&1
if errorlevel 1 (
    echo   FAIL: Debug build failed — see 07_build_debug.txt
    set /a FAILTOTAL+=1
    goto :write_verdict
)
echo   PASS: Debug build rc=0 (07_build_debug.txt)
for %%A in (
    "%BUILD%\Debug\Suborno.exe"
    "%BUILD%\Debug\suborno_windows_tsf.dll"
    "%BUILD%\Debug\suborno_unit_tests.exe"
    "%BUILD%\Debug\suborno_integration_tests.exe"
    "%BUILD%\Debug\suborno_windows_tests.exe"
) do (
    if exist %%A ( echo   PASS: %%~A ) else ( echo   FAIL: missing %%~A & set /a FAILTOTAL+=1 )
)

:: ---------------------------------------------------------------------------
:: STEP 8 — Debug CTest (task commands, verbatim)
:: ---------------------------------------------------------------------------
echo.
echo [STEP 8] Debug CTest
ctest --test-dir build\windows-msvc -N -C Debug > "%EV%\08_ctest_debug_N.txt" 2>&1
ctest --test-dir build\windows-msvc -C Debug --output-on-failure > "%EV%\08_ctest_debug.txt" 2>&1
set CTRC=%errorlevel%
echo   ctest Debug rc=%CTRC% (0 required)
if not "%CTRC%"=="0" set /a FAILTOTAL+=1
for %%G in (suborno.unit suborno.integration suborno.windows.tsf_contract suborno.windows.app_smoke) do (
    findstr /C:"%%G" "%EV%\08_ctest_debug_N.txt" >nul 2>&1
    if errorlevel 1 ( echo   FAIL: %%G not registered in Debug listing & set /a FAILTOTAL+=1 ) else echo   PASS: %%G registered
)
findstr /C:"Not Run" "%EV%\08_ctest_debug.txt" > "%EV%\08_ctest_debug_notrun.txt" 2>&1
for /f %%N in ('type "%EV%\08_ctest_debug_notrun.txt" ^| find /c /v ""') do set NOTRUN=%%N
echo   Debug "Not Run" lines     : !NOTRUN!  (must be 0)
if not "!NOTRUN!"=="0" set /a FAILTOTAL+=1
findstr /C:"100%% tests passed" "%EV%\08_ctest_debug.txt" >nul 2>&1
if errorlevel 1 ( echo   FAIL: Debug did not report 100%% tests passed & set /a FAILTOTAL+=1 ) else echo   PASS: Debug 100%% tests passed

:: ---------------------------------------------------------------------------
:: STEP 9 — UI regression preservation (task section 6)
:: ---------------------------------------------------------------------------
echo.
echo [STEP 9] UI regression preservation
powershell -NoProfile -Command "$c = (Import-Csv 'docs\reports\UI_FUNCTIONAL_AUDIT_INVENTORY_V2.csv').Count; Write-Output ('element rows: ' + $c); if ($c -eq 113) { exit 0 } else { exit 1 }" > "%EV%\09_ui_inventory_count.txt" 2>&1
type "%EV%\09_ui_inventory_count.txt"
if errorlevel 1 ( echo   FAIL: inventory element count is not 113 & set /a FAILTOTAL+=1 ) else echo   PASS: 113 UI elements present
:: Direct UIAudit regression evidence from the Release unit binary.
"%BUILD%\Release\suborno_unit_tests.exe" --gtest_filter=UIAudit* > "%EV%\09_uiaudit_release.txt" 2>&1
if errorlevel 1 (
    echo   FAIL: UIAudit regression subset failed on Release — see 09_uiaudit_release.txt
    set /a FAILTOTAL+=1
) else (
    echo   PASS: UIAudit regression subset passed — PASS count inside 09_uiaudit_release.txt
)
:: Narrator / UI Automation client / interactive shell verification is NOT
:: executable from a batch script: recorded as BLOCKED, never claimed PASS.
(
  echo BLOCKED registry — Windows runtime limitations ^(not executable here^):
  echo - Narrator announcement verification: requires interactive desktop session + Narrator ^(manual^).
  echo - UI Automation client walk of the 113 elements: requires a live interactive shell ^(manual^).
  echo - Candidate-window slide animation / click-to-commit live check: requires interactive IME session ^(manual^).
) > "%EV%\09_blocked_registry.txt" 2>&1
echo   BLOCKED items recorded in 09_blocked_registry.txt

:: ---------------------------------------------------------------------------
:: VERDICT
:: ---------------------------------------------------------------------------
:write_verdict
echo.
echo ==================================================================
if "%FAILTOTAL%"=="0" (
    set VERDICT=ALL GATES PASS — attach evidence\MSVC-WARNINGS-REPAIR-02\ to the task report
) else (
    set VERDICT=FAILTOTAL=!FAILTOTAL! gate(s) FAILED — inspect the evidence files listed above
)
echo VERDICT: !VERDICT!
echo ==================================================================
(
  echo MSVC-WARNINGS-REPAIR-02 final Windows verification verdict
  echo Date: %DATE% %TIME%
  echo Repository: %REPO%
  echo Result: !VERDICT!
  echo.
  echo Release: errors !ERRN! ^| C4715 !C4715N! ^| C28182 !C28182N! ^| D9007 !D9007N! ^| warnings !WARNN!
  echo Release Not Run lines: !NOTRUN!
) > "%SUMMARY%" 2>&1
echo Verdict written to %SUMMARY%
:: Task requirement: the script must return nonzero when any acceptance gate
:: failed. %FAILTOTAL% is expanded before endlocal executes on this line.
endlocal & exit /b %FAILTOTAL%
