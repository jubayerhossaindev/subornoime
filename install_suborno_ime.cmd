@echo off
:: ============================================================================
:: SUBORNO IME — install / diagnose (run from an ELEVATED "Terminal (Admin)")
::
:: WHY THIS IS NEEDED
::   Bangla typing works only when suborno_windows_tsf.dll is REGISTERED as a
::   Windows text service. Registration writes HKCR\CLSID and the TSF profile
::   (Bangla-Bangladesh) and needs ADMIN rights. The test suite skips the
::   registration check when it is not elevated, so a normal build+test run
::   never registers anything.
::
:: USAGE
::   1. Open "Terminal (Admin)" (Win+X -> Terminal (Admin))
::   2. cd /d F:\my-windows-app\suborno\SUBORNOIME\SUBORNOIME\build\windows-msvc\Release
::      (or pass the Release folder as the first argument)
::   3. path\to\install_suborno_ime.cmd
::   4. After SUCCESS: press Win+Space and pick "Suborno (Bengali, Bangladesh)"
::      in EVERY app (Notepad/Word/browser), then just type banglish.
::
::   Uninstall: install_suborno_ime.cmd /uninstall
:: ============================================================================
setlocal EnableDelayedExpansion

if /i "%~1"=="/uninstall" goto :uninstall

set "DLL=%~1"
if "!DLL!"=="" set "DLL=%CD%\suborno_windows_tsf.dll"
if not exist "!DLL!" (
    echo [FAIL] suborno_windows_tsf.dll not found at:
    echo        !DLL!
    echo Build first: cmake --build --preset windows-msvc-release --parallel 1
    echo Then run this script from the Release folder or pass its path.
    exit /b 2
)

echo ==================================================================
echo SUBORNO IME installation  (elevation check follows)
echo DLL: !DLL!
echo ==================================================================
net session >nul 2>&1
if errorlevel 1 (
    echo [FAIL] Not elevated. Open "Terminal (Admin)" and re-run.
    echo        Registration must write HKLM/HKCR keys.
    exit /b 2
)
echo [OK] Elevated.

echo.
echo [1/5] Registering the COM + TSF text service ...
regsvr32 /s "!DLL!"
if errorlevel 1 (
    echo [FAIL] regsvr32 returned an error. Try visibly to see the message:
    echo        regsvr32 "!DLL!"
    exit /b 2
)
echo [OK] regsvr32 completed.

echo.
echo [2/5] Verifying the COM class key ...
reg query "HKCR\CLSID" /s /f "Suborno Bengali IME Text Service" /d > "%TEMP%\suborno_clsid.txt" 2>nul
findstr /i "HKEY_CLASSES_ROOT" "%TEMP%\suborno_clsid.txt" >nul 2>&1
if errorlevel 1 (
    echo [FAIL] COM class key not found — registration did not persist.
    exit /b 2
)
findstr /i "HKEY_CLASSES_ROOT" "%TEMP%\suborno_clsid.txt"
echo [OK] COM class registered ^(default value = "Suborno Bengali IME Text Service"^).

echo.
echo [3/5] Verifying the TSF profile registry ^(CTF\TIP^) ...
reg query "HKLM\SOFTWARE\Microsoft\CTF\TIP" /s /f "Suborno" /d 2>nul | findstr /i "HKEY_LOCAL_MACHINE TIP {" | find /c "HKEY" > "%TEMP%\suborno_tip_count.txt"
set /p TIPN=<"%TEMP%\suborno_tip_count.txt"
if "!TIPN!"=="0" (
    echo [FAIL] No CTF\TIP profile found for Suborno — the text service exists
    echo        but no Bangla ^(BD^) language profile. Run:
    echo        regsvr32 /u /s "!DLL!"  then  regsvr32 /s "!DLL!"
    exit /b 2
) else (
    echo [OK] !TIPN! CTF\TIP profile entr^(y/ies^) present.
)

echo.
echo [4/5] Restarting the CTF loader stack so every app sees the new TIP ...
:: CUAS/TextServiceHost pick up registrations on next logon or Explorer restart.
taskkill /f /im ctfmon.exe >nul 2>&1
start "" /wait cmd /c "ctfmon.exe"
echo [OK] ctfmon restarted.

echo.
echo [5/5] What to do in Windows ^(human steps, cannot be automated^):
echo   a. Press Win+Space ^(or click the language indicator^).
echo   b. Pick:  Suborno ^(Bengali, Bangladesh^)   -- it appears under
echo      Settings ^> Time ^& Language ^> Language ^& region ^> Bangla ^> Options
echo      ^> Keyboards as "Suborno".
echo   c. Click inside Notepad / Word / the browser address bar and type
echo      banglish, e.g. "amar" -^> "আমার", "tomar" -^> "তোমার".
echo   d. The Suborno topbar/candidate window appears while composing.
echo.
echo If the keyboard is NOT listed under Bangla, run:
echo   reg query "HKLM\SOFTWARE\Microsoft\CTF\TIP" /s /f "Suborno" /d
echo and attach the output to the task report.
echo.
echo [DONE] Suborno IME registered. Switch with Win+Space and type Bangla.
endlocal & exit /b 0

:uninstall
net session >nul 2>&1
if errorlevel 1 (
    echo [FAIL] Not elevated. Open "Terminal (Admin)" and re-run.
    exit /b 2
)
set "DLL=%~2"
if "!DLL!"=="" set "DLL=%CD%\suborno_windows_tsf.dll"
echo Unregistering TSF profile + COM class from: !DLL!
regsvr32 /u /s "!DLL!"
echo [DONE] Unregistered. Sign out/in once so every app drops the profile.
endlocal & exit /b 0
