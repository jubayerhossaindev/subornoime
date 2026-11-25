@echo off
setlocal EnableDelayedExpansion
:: ============================================================================
:: SUBORNO — Zero Bengali Language Pack Dependency
:: Standalone host installer (current user, HKCU — NO admin rights needed).
::
:: WHAT IT DOES
::   1. Locates SubornoStandalone.exe (argument, or the Release build output).
::   2. Copies it to %LOCALAPPDATA%\Suborno\bin\.
::   3. Copies the bundled Bangla fonts to %LOCALAPPDATA%\Suborno\fonts\
::      (the host loads them per-process with AddFontResourceExW FR_PRIVATE,
::      so the floating preview renders in Hind Siliguri on any machine).
::   4. Writes the HKCU autostart Run entry (survives reboots).
::   5. Writes the default %APPDATA%\Suborno\standalone.ini if absent.
::   6. Starts the host immediately.
::
:: It does NOT touch TSF registration, the Bengali language pack, or the
:: Windows input-language list. Install -> run -> type Bangla anywhere.
::
:: HOW TO RUN (repo root, after the Release build):
::   install_standalone.cmd
::   install_standalone.cmd D:\path\to\SubornoStandalone.exe
:: ============================================================================
set "DEFAULT_EXE=%CD%\build\windows-msvc\Release\SubornoStandalone.exe"
set "EXE=%~1"
if "%EXE%"=="" set "EXE=%DEFAULT_EXE%"

if not exist "%EXE%" (
    echo FAIL: SubornoStandalone.exe not found: %EXE%
    echo Build it first:
    echo   cmake --build --preset windows-msvc-release --target suborno_standalone
    endlocal & exit /b 1
)

taskkill /im SubornoStandalone.exe /f >nul 2>&1

set "DEST=%LOCALAPPDATA%\Suborno\bin"
if not exist "%DEST%" mkdir "%DEST%"
copy /y "%EXE%" "%DEST%\" >nul
if errorlevel 1 (
    echo FAIL: copy to %DEST% failed
    endlocal & exit /b 1
)
set "INSTALLED=%DEST%\SubornoStandalone.exe"

:: Bundled Bangla fonts (SIL OFL 1.1): the host probes bin\..\fonts and
:: bin\fonts, so copy to %LOCALAPPDATA%\Suborno\fonts. Missing fonts are not
:: fatal: the preview falls back to Nirmala UI (Windows 8+ built-in).
if exist "%~dp0assets\fonts" (
    if not exist "%LOCALAPPDATA%\Suborno\fonts" mkdir "%LOCALAPPDATA%\Suborno\fonts" >nul 2>&1
    copy /y "%~dp0assets\fonts\*.ttf" "%LOCALAPPDATA%\Suborno\fonts\" >nul
    if errorlevel 1 (
        echo WARN: font copy to %LOCALAPPDATA%\Suborno\fonts failed - preview falls back to Nirmala UI
    ) else (
        echo PASS: bundled Bangla fonts copied ^(Hind Siliguri, Baloo Da 2, Noto Sans Bengali^)
    )
) else (
    echo WARN: assets\fonts not found - preview falls back to Nirmala UI
)

reg add "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v SubornoStandalone ^
    /t REG_SZ /d "%INSTALLED%" /f >nul
if errorlevel 1 (
    echo FAIL: HKCU autostart registry write failed
    endlocal & exit /b 1
)

if not exist "%APPDATA%\Suborno\standalone.ini" (
    if not exist "%APPDATA%\Suborno" mkdir "%APPDATA%\Suborno" >nul 2>&1
    (
        echo [standalone]
        echo hotkey=ctrl+alt+b
        echo preview=1
        echo enabled_on_start=1
    ) > "%APPDATA%\Suborno\standalone.ini"
)

start "" "%INSTALLED%"
echo.
echo PASS: SubornoStandalone installed and started: %INSTALLED%
echo       Toggle Bangla typing : Ctrl+Alt+B   (tray icon = Suborno Standalone)
echo       Preview font         : Hind Siliguri Medium (bundled, per-process)
echo       Config file          : %%APPDATA%%\Suborno\standalone.ini
echo       (hotkey=ctrl+alt+b, preview=1, enabled_on_start=1)
echo.
echo The TSF IME pipeline is untouched. Bengali language pack / Win+Space are
echo NOT required for this mode. To make the bundled fonts available to ALL
echo apps (TSF candidate list, Word, browsers), also run install_fonts.cmd.
endlocal & exit /b 0
