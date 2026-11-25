@echo off
setlocal EnableDelayedExpansion
:: ============================================================================
:: SUBORNO — Zero Bengali Language Pack Dependency
:: Standalone host uninstaller (current user, HKCU — no admin needed).
:: Removes the autostart entry, stops the host, and deletes the copied exe.
:: The user data (%APPDATA%\Suborno — settings, dictionary) is NOT deleted.
:: ============================================================================
taskkill /im SubornoStandalone.exe /f >nul 2>&1

reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v SubornoStandalone /f >nul 2>&1
if errorlevel 1 (
    echo NOTE: no SubornoStandalone autostart entry found ^(already uninstalled?^)
) else (
    echo PASS: HKCU autostart entry removed
)

if exist "%LOCALAPPDATA%\Suborno\bin\SubornoStandalone.exe" (
    del /q "%LOCALAPPDATA%\Suborno\bin\SubornoStandalone.exe" >nul 2>&1
    echo PASS: %LOCALAPPDATA%\Suborno\bin\SubornoStandalone.exe deleted
)

if exist "%LOCALAPPDATA%\Suborno\fonts" (
    rmdir /s /q "%LOCALAPPDATA%\Suborno\fonts" >nul 2>&1
    echo PASS: %LOCALAPPDATA%\Suborno\fonts deleted (per-process font copies)
)

echo.
echo PASS: standalone host uninstalled.
echo Kept : %%APPDATA%%\Suborno\standalone.ini and the shared settings/dictionary.
echo To also remove the TSF IME, use install_suborno_ime.cmd's uninstall mode.
endlocal & exit /b 0
