@echo off
setlocal EnableDelayedExpansion
:: ============================================================================
:: SUBORNO — optional per-user install of the bundled Bangla fonts
:: (assets\fonts, SIL OFL 1.1 — see assets\fonts\FONT-CREDITS.md).
::
:: WHAT THIS DOES
::   Copies the bundled .ttf files to the per-user font folder and registers
::   them under HKCU\Software\Microsoft\Windows NT\CurrentVersion\Fonts.
::   Windows 10 1809+ supports per-user fonts, so NO admin rights are needed
::   and nothing touches HKLM.
::
:: WHEN TO RUN IT
::   Not required for the standalone host (SubornoStandalone.exe loads the
::   same files per-process with FR_PRIVATE). Run this once if you want the
::   bundled faces available EVERYWHERE for this user:
::     - the TSF IME candidate list (settings.ui.font_family =
::       "Hind Siliguri Medium" etc.),
::     - the Suborno settings font picker,
::     - every other application (Word, browsers, ...).
::
:: HOW TO RUN (repo root):
::   install_fonts.cmd
::   install_fonts.cmd /uninstall     (removes the per-user copies again)
:: ============================================================================

set "FONT_SRC=%~dp0assets\fonts"
set "FONT_DST=%LOCALAPPDATA%\Microsoft\Windows\Fonts"
set "REGKEY=HKCU\Software\Microsoft\Windows NT\CurrentVersion\Fonts"

if /i "%~1"=="/uninstall" goto :uninstall

if not exist "%FONT_SRC%" (
    echo FAIL: %FONT_SRC% not found - run this script from the repository root.
    endlocal & exit /b 1
)

:: Per-user font registration needs Windows 10 1809+ (per-user fonts in
:: Settings). On older builds the files copy fine but Windows ignores them;
:: the standalone host is unaffected in every case (FR_PRIVATE).
echo Per-user fonts work on Windows 10 1809+ (build 17134). On older builds the
echo files install but Windows may not list them; the standalone host is not
echo affected (it loads the fonts per-process).

if not exist "%FONT_DST%" mkdir "%FONT_DST%" >nul 2>&1

copy /y "%FONT_SRC%\*.ttf" "%FONT_DST%\" >nul
if errorlevel 1 (
    echo FAIL: copying fonts to %FONT_DST% failed
    endlocal & exit /b 1
)
echo PASS: 10 .ttf files copied to %FONT_DST%

:: Registry display names follow the faces' name tables exactly (verified with
:: fonttools; non-RIBBI styles carry their weight inside the family name).
set "FAILED=0"
call :regfont "Hind Siliguri Light (TrueType)"    "HindSiliguri-Light.ttf"    || set "FAILED=1"
call :regfont "Hind Siliguri (TrueType)"          "HindSiliguri-Regular.ttf"  || set "FAILED=1"
call :regfont "Hind Siliguri Medium (TrueType)"   "HindSiliguri-Medium.ttf"   || set "FAILED=1"
call :regfont "Hind Siliguri SemiBold (TrueType)" "HindSiliguri-SemiBold.ttf" || set "FAILED=1"
call :regfont "Hind Siliguri Bold (TrueType)"     "HindSiliguri-Bold.ttf"     || set "FAILED=1"
call :regfont "Baloo Da 2 (TrueType)"             "BalooDa2-Regular.ttf"      || set "FAILED=1"
call :regfont "Baloo Da 2 SemiBold (TrueType)"    "BalooDa2-SemiBold.ttf"     || set "FAILED=1"
call :regfont "Baloo Da 2 Bold (TrueType)"        "BalooDa2-Bold.ttf"         || set "FAILED=1"
call :regfont "Noto Sans Bengali (TrueType)"      "NotoSansBengali-Regular.ttf" || set "FAILED=1"
call :regfont "Noto Sans Bengali Med (TrueType)"  "NotoSansBengali-Medium.ttf"  || set "FAILED=1"

if not "%FAILED%"=="0" (
    echo FAIL: one or more registry entries could not be written.
    endlocal & exit /b 1
)

echo.
echo PASS: bundled Bangla fonts installed for the current user.
echo       Location : %FONT_DST%
echo       Registry : %REGKEY%
echo       Restart running applications (or sign out/in) to see the faces.
echo       Families : Hind Siliguri ^(5 weights^), Baloo Da 2 ^(3^),
echo                  Noto Sans Bengali ^(2^)  [SIL OFL 1.1]
echo       Next     : Suborno Settings font picker, or set settings.ui.font_family
echo                  (e.g. "Hind Siliguri Medium") for the TSF candidate list.
endlocal & exit /b 0

:: --- helpers ----------------------------------------------------------------
:regfont
reg add "%REGKEY%" /v "%~1" /t REG_SZ /d "%FONT_DST%\%~2" /f >nul 2>&1
if errorlevel 1 (
    echo FAIL: registry entry "%~1" -^> %~2
    exit /b 1
)
echo PASS: registered %~1
exit /b 0

:unregfont
reg delete "%REGKEY%" /v "%~1" /f >nul 2>&1
if exist "%FONT_DST%\%~2" del /q "%FONT_DST%\%~2" >nul 2>&1
echo PASS: removed %~1 (if it was installed)
exit /b 0

:uninstall
echo Removing per-user bundled font entries...
call :unregfont "Hind Siliguri Light (TrueType)"    "HindSiliguri-Light.ttf"
call :unregfont "Hind Siliguri (TrueType)"          "HindSiliguri-Regular.ttf"
call :unregfont "Hind Siliguri Medium (TrueType)"   "HindSiliguri-Medium.ttf"
call :unregfont "Hind Siliguri SemiBold (TrueType)" "HindSiliguri-SemiBold.ttf"
call :unregfont "Hind Siliguri Bold (TrueType)"     "HindSiliguri-Bold.ttf"
call :unregfont "Baloo Da 2 (TrueType)"             "BalooDa2-Regular.ttf"
call :unregfont "Baloo Da 2 SemiBold (TrueType)"    "BalooDa2-SemiBold.ttf"
call :unregfont "Baloo Da 2 Bold (TrueType)"        "BalooDa2-Bold.ttf"
call :unregfont "Noto Sans Bengali (TrueType)"      "NotoSansBengali-Regular.ttf"
call :unregfont "Noto Sans Bengali Med (TrueType)"  "NotoSansBengali-Medium.ttf"
echo.
echo PASS: per-user bundled font entries removed (if they existed).
echo The standalone host keeps working: it loads fonts per-process next to the exe.
endlocal & exit /b 0
