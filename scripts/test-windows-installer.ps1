$ErrorActionPreference = 'Stop'

$installer = Join-Path $PWD 'packaging\windows\Output\Suborno-Setup-x64.exe'
$installerX86 = Join-Path $PWD 'packaging\windows\Output\Suborno-Setup-x86.exe'
if (-not (Test-Path -LiteralPath $installer -PathType Leaf)) {
    throw "Installer not found: $installer"
}

$setupVersion = (Get-Item -LiteralPath $installer).VersionInfo
$rawProductName = [string]$setupVersion.ProductName
$productName = $rawProductName.Trim()
if ($productName -cne 'Suborno') {
    throw "Unexpected installer ProductName: '$rawProductName' (trimmed: '$productName'; raw length: $($rawProductName.Length))."
}
Write-Host "Verified installer ProductName: '$productName' (raw length: $($rawProductName.Length))."
$signature = Get-AuthenticodeSignature -LiteralPath $installer
Write-Host "Installer signature status: $($signature.Status)"
if ($signature.Status -notin @('Valid', 'NotSigned')) {
    throw "Unexpected installer signature status: $($signature.Status)"
}

$installDirectory = Join-Path $env:ProgramFiles 'Suborno'
$installDirectoryX86 = Join-Path ${env:ProgramFiles(x86)} 'Suborno'
$programDataDirectory = Join-Path $env:ProgramData 'Suborno'
$serviceDirectory = Join-Path $programDataDirectory 'components\tsf\x64'
$serviceDirectoryX86 = Join-Path $programDataDirectory 'components\tsf\x86'
$serviceBuild = Join-Path $PWD 'build\windows-msvc\Release\suborno_windows_tsf.dll'
$service = Join-Path $serviceDirectory 'suborno_windows_tsf.dll'
$serviceX86 = Join-Path $serviceDirectoryX86 'suborno_windows_tsf.dll'
$userDataDirectory = Join-Path $env:APPDATA 'Suborno'
$userDictionary = Join-Path $userDataDirectory 'dictionaries\user_candidates.db'
$userLearnt = Join-Path $userDataDirectory 'dictionaries\suborno_learnt_bn'
$userLayouts = Join-Path $userDataDirectory 'layouts'
$desktopDirectory = [Environment]::GetFolderPath('CommonDesktopDirectory')
$startMenuDirectory = Join-Path ([Environment]::GetFolderPath('CommonPrograms')) 'Suborno'
$quickLaunchDirectory = Join-Path $env:APPDATA 'Microsoft\Internet Explorer\Quick Launch'
$commonFilesDirectory = [Environment]::GetFolderPath('CommonProgramFiles')
$regsvr32 = Join-Path $env:WINDIR 'System32\regsvr32.exe'
$shell = New-Object -ComObject WScript.Shell
$classRegistration = 'Registry::HKEY_CLASSES_ROOT\CLSID\{E1CC38E7-F986-4371-BA1D-A527CA600677}\InprocServer32'
$classKey = 'Registry::HKEY_CLASSES_ROOT\CLSID\{E1CC38E7-F986-4371-BA1D-A527CA600677}'

function Invoke-SubornoSetup {
    param([string]$Path)

    $process = Start-Process -FilePath $Path `
        -ArgumentList @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/SP-', '/TASKS=desktopicon,quicklaunchicon') `
        -Wait -PassThru
    if ($process.ExitCode -ne 0) {
        throw "Suborno installer exited with code $($process.ExitCode)."
    }
}

function Assert-CleanInstallDirectory {
    param([string]$Path)

    $expectedPayloadFiles = @(
        'LICENSE',
        'README.md',
        'Suborno.exe',
        'THIRD_PARTY_NOTICES.md'
    )
    $actualFiles = @(Get-ChildItem -LiteralPath $Path -Force -File |
        Sort-Object Name | Select-Object -ExpandProperty Name)
    $actualPayloadFiles = @($actualFiles | Where-Object {
        $_ -notmatch '^unins\d{3}\.(dat|exe)$'
    })
    if (($actualPayloadFiles -join '|') -ne (($expectedPayloadFiles | Sort-Object) -join '|')) {
        throw "Unexpected Program Files payload files: $($actualPayloadFiles -join ', ')"
    }

    $uninstallerExecutables = @($actualFiles | Where-Object { $_ -match '^unins\d{3}\.exe$' })
    $uninstallerDataFiles = @($actualFiles | Where-Object { $_ -match '^unins\d{3}\.dat$' })
    if ($uninstallerExecutables.Count -ne 1 -or $uninstallerDataFiles.Count -ne 1) {
        throw "Expected exactly one matching Inno Setup uninstaller executable/data pair; found $($uninstallerExecutables.Count) executables and $($uninstallerDataFiles.Count) data files."
    }
    $uninstallerExeBase = [IO.Path]::GetFileNameWithoutExtension($uninstallerExecutables[0])
    $uninstallerDataBase = [IO.Path]::GetFileNameWithoutExtension($uninstallerDataFiles[0])
    if ($uninstallerExeBase -cne $uninstallerDataBase) {
        throw "Inno Setup uninstaller files do not match: $($uninstallerExecutables[0]) and $($uninstallerDataFiles[0])."
    }
    $noticesPath = Join-Path $Path 'THIRD_PARTY_NOTICES.md'
    if (-not (Test-Path -LiteralPath $noticesPath -PathType Leaf) -or
        (Get-Item -LiteralPath $noticesPath).Length -eq 0) {
        throw 'Third-party license notices are missing from the installed application.'
    }
    # The installer intentionally ships the UI locale files under data/i18n.
    # Validate that exact directory tree instead of incorrectly requiring a flat install.
    $expectedDirectories = @('data')
    $actualDirectories = @(Get-ChildItem -LiteralPath $Path -Force -Directory |
        Sort-Object Name | Select-Object -ExpandProperty Name)
    if (($actualDirectories -join '|') -ne ($expectedDirectories -join '|')) {
        throw "Unexpected Program Files directories: $($actualDirectories -join ', ')"
    }

    $i18nDirectory = Join-Path $Path 'data\i18n'
    $expectedLocales = @('bn', 'en')
    $actualLocales = @(Get-ChildItem -LiteralPath $i18nDirectory -Force -Directory |
        Sort-Object Name | Select-Object -ExpandProperty Name)
    if (($actualLocales -join '|') -ne ($expectedLocales -join '|')) {
        throw "Unexpected installed UI locale directories: $($actualLocales -join ', ')"
    }

    foreach ($locale in $expectedLocales) {
        $localeDirectory = Join-Path $i18nDirectory $locale
        $localeFiles = @(Get-ChildItem -LiteralPath $localeDirectory -Force -File |
            Sort-Object Name | Select-Object -ExpandProperty Name)
        if (($localeFiles -join '|') -ne 'ui.json') {
            throw "Unexpected files in the '$locale' UI locale directory: $($localeFiles -join ', ')"
        }
        if (@(Get-ChildItem -LiteralPath $localeDirectory -Force -Directory).Count -ne 0) {
            throw "Unexpected nested directory in the '$locale' UI locale directory."
        }
    }
}

function Get-SubornoUninstallerPath {
    param([string]$Path)

    $uninstallers = @(Get-ChildItem -LiteralPath $Path -Force -File |
        Where-Object { $_.Name -match '^unins\d{3}\.exe$' })
    if ($uninstallers.Count -ne 1) {
        throw "Expected exactly one Suborno uninstaller in '$Path'; found $($uninstallers.Count)."
    }

    $uninstallerData = [IO.Path]::ChangeExtension($uninstallers[0].FullName, '.dat')
    if (-not (Test-Path -LiteralPath $uninstallerData -PathType Leaf)) {
        throw "Matching Inno Setup uninstaller data file is missing: $uninstallerData"
    }

    return $uninstallers[0].FullName
}

function Assert-ServiceDeployment {
    param([string]$Path, [string]$Architecture)

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "The TSF service is missing from ProgramData: $Path"
    }
    $serviceFiles = @(Get-ChildItem -LiteralPath (Split-Path -Parent $Path) -Force -File)
    if ($serviceFiles.Count -ne 1 -or $serviceFiles[0].Name -ne 'suborno_windows_tsf.dll') {
        throw "Unexpected ProgramData service files: $($serviceFiles.Name -join ', ')"
    }
    $registeredPath = (Get-Item -LiteralPath $classRegistration).GetValue('')
    if ([IO.Path]::GetFullPath($registeredPath) -ne [IO.Path]::GetFullPath($Path)) {
        throw "The TSF COM server points to the wrong file: $registeredPath"
    }
}

function Assert-SharedResourceDirectories {
    foreach ($resourceDirectory in @(
        'assets\fonts', 'assets\themes', 'assets\layout-icons\borno', 'dictionaries', 'layouts'
    )) {
        if (-not (Test-Path -LiteralPath (Join-Path $programDataDirectory $resourceDirectory) -PathType Container)) {
            throw "Machine-wide resource directory is missing: $resourceDirectory"
        }
    }
    if (Test-Path -LiteralPath (Join-Path $commonFilesDirectory 'Suborno')) {
        throw 'Suborno unexpectedly installed files under Common Files.'
    }
}

function Assert-Shortcut {
    param([string]$Path, [string]$ExpectedTarget)

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Shortcut not found: $Path"
    }
    $shortcut = $shell.CreateShortcut($Path)
    if ([IO.Path]::GetFullPath($shortcut.TargetPath) -ne [IO.Path]::GetFullPath($ExpectedTarget)) {
        throw "Shortcut target is incorrect: $Path -> $($shortcut.TargetPath)"
    }
}

function Assert-UserDataPreserved {
    if (-not (Test-Path -LiteralPath $userDictionary -PathType Leaf)) {
        throw 'The user dictionary was removed.'
    }
    if (-not (Test-Path -LiteralPath $userLearnt -PathType Container)) {
        throw 'The user learnt-word directory was removed.'
    }
    if (-not (Test-Path -LiteralPath $userLayouts -PathType Container)) {
        throw 'The user layouts directory was removed.'
    }
}

function Assert-UserDataInitialized {
    if (-not (Test-Path -LiteralPath $userDictionary -PathType Leaf) -or
        -not (Test-Path -LiteralPath $userLearnt -PathType Container) -or
        -not (Test-Path -LiteralPath $userLayouts -PathType Container)) {
        throw 'The per-user Roaming data structure was not initialized.'
    }
}

# Seed the prior deployment's layout and registration to exercise the real upgrade path.
if (Test-Path -LiteralPath $installDirectory) {
    throw "Expected a clean CI runner; installation directory already exists: $installDirectory"
}
if (Test-Path -LiteralPath $programDataDirectory) {
    throw "Expected a clean CI runner; ProgramData directory already exists: $programDataDirectory"
}
New-Item -ItemType Directory -Force -Path $installDirectory | Out-Null
$legacyService = Join-Path $installDirectory 'suborno_windows_tsf.dll'
Copy-Item -LiteralPath $serviceBuild -Destination $legacyService
Copy-Item -LiteralPath (Join-Path $PWD 'THIRD_PARTY_NOTICES.md') `
    -Destination (Join-Path $installDirectory 'THIRD_PARTY_NOTICES.md')
foreach ($legacyFile in @('sqlite3.dll', 'icuuc.dll', 'vcruntime140.dll', 'msvcp140.dll')) {
    Set-Content -LiteralPath (Join-Path $installDirectory $legacyFile) -Value 'legacy deployment residue'
}
$legacyRegistration = Start-Process -FilePath $regsvr32 `
    -ArgumentList "/s `"$legacyService`"" -Wait -PassThru
if ($legacyRegistration.ExitCode -ne 0) {
    throw "Legacy TSF registration simulation failed with exit code $($legacyRegistration.ExitCode)."
}

# Phase 1: x64 install, upgrade, smoke, and uninstall.
Invoke-SubornoSetup -Path $installer
$application = Join-Path $installDirectory 'Suborno.exe'
Assert-CleanInstallDirectory -Path $installDirectory
Assert-ServiceDeployment -Path $service -Architecture x64
Assert-SharedResourceDirectories

$desktopShortcut = Join-Path $desktopDirectory 'Suborno.lnk'
$startMenuShortcut = Join-Path $startMenuDirectory 'Suborno.lnk'
$quickLaunchShortcut = Join-Path $quickLaunchDirectory 'Suborno.lnk'
Assert-Shortcut -Path $desktopShortcut -ExpectedTarget $application
Assert-Shortcut -Path $startMenuShortcut -ExpectedTarget $application
Assert-Shortcut -Path $quickLaunchShortcut -ExpectedTarget $application

$smoke = Start-Process -FilePath $application -ArgumentList '--smoke-test' -Wait -PassThru
if ($smoke.ExitCode -ne 0) {
    throw "Installed GUI smoke test failed with exit code $($smoke.ExitCode)."
}
Assert-UserDataInitialized

Invoke-SubornoSetup -Path $installer
Assert-CleanInstallDirectory -Path $installDirectory
Assert-ServiceDeployment -Path $service -Architecture x64

$startupKey = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run'
New-Item -Path $startupKey -Force | Out-Null
Set-ItemProperty -Path $startupKey -Name 'Suborno' -Value "`"$application`" --startup"
$uninstaller = Get-SubornoUninstallerPath -Path $installDirectory
$uninstallLog = Join-Path $PWD 'verify-evidence\uninstall-x64-first.log'
$uninstall = Start-Process -FilePath $uninstaller `
    -ArgumentList @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', "/LOG=$uninstallLog") -Wait -PassThru
if ($uninstall.ExitCode -ne 0) {
    throw "Suborno uninstaller exited with code $($uninstall.ExitCode). See $uninstallLog."
}

# This test creates the legacy Program Files\Suborno directory itself before
# installing the new package. Inno Setup intentionally leaves directories
# that existed before installation. Only remove that fixture-owned root after
# proving the uninstaller removed every installed file and subdirectory.
if (Test-Path -LiteralPath $installDirectory -PathType Container) {
    $remainingEntries = @(Get-ChildItem -LiteralPath $installDirectory -Force -Recurse)
    if ($remainingEntries.Count -gt 0) {
        $remainingDescription = $remainingEntries | ForEach-Object {
            if ($_.PSIsContainer) {
                "[DIR] $($_.FullName)"
            } else {
                "[FILE] $($_.FullName)"
            }
        }
        throw "Installer left entries under the pre-existing Program Files directory: $($remainingDescription -join '; '). See $uninstallLog."
    }

    Remove-Item -LiteralPath $installDirectory -Force
}
if (Test-Path -LiteralPath $installDirectory) {
    throw 'The pre-existing installer test directory could not be removed after its contents were verified empty.'
}
if (Test-Path -LiteralPath $programDataDirectory) {
    throw 'Suborno service files remain in ProgramData after uninstall.'
}
Assert-UserDataPreserved
if (Test-Path -LiteralPath $desktopShortcut) {
    throw 'Desktop shortcut remains after uninstall.'
}
if (Test-Path -LiteralPath $startMenuShortcut) {
    throw 'Start Menu shortcut remains after uninstall.'
}
if (Test-Path -LiteralPath $quickLaunchShortcut) {
    throw 'Quick Launch shortcut remains after uninstall.'
}
if (Test-Path -LiteralPath $classKey) {
    throw 'The TSF COM server registration remains after uninstall.'
}
if (Get-ItemProperty -Path $startupKey -Name 'Suborno' -ErrorAction SilentlyContinue) {
    throw 'Suborno sign-in startup entry remains after uninstall.'
}

Write-Host 'x64 install, upgrade, exact folder contents, shortcuts, TSF registration, GUI smoke, reinstall, and uninstall checks passed.'

# Phase 2: x86 and x64 must share ProgramData resources and the same AppData user data.
if (-not (Test-Path -LiteralPath $installerX86 -PathType Leaf)) {
    Write-Host 'x86 installer not present; skipping cross-architecture co-existence checks.'
    Write-Host 'All installer checks passed.'
    exit 0
}

if (Test-Path -LiteralPath $installDirectoryX86) {
    throw "Expected a clean CI runner; x86 installation directory already exists: $installDirectoryX86"
}

# Install x86 first.
Invoke-SubornoSetup -Path $installerX86
$applicationX86 = Join-Path $installDirectoryX86 'Suborno.exe'
Assert-CleanInstallDirectory -Path $installDirectoryX86
Assert-ServiceDeployment -Path $serviceX86 -Architecture x86
Assert-SharedResourceDirectories
Assert-UserDataPreserved

# Install x64 on top; both architectures must coexist with shared ProgramData resources.
Invoke-SubornoSetup -Path $installer
Assert-CleanInstallDirectory -Path $installDirectory
Assert-ServiceDeployment -Path $service -Architecture x64
if (-not (Test-Path -LiteralPath $serviceX86 -PathType Leaf)) {
    throw 'The x86 TSF service was removed by the x64 installation.'
}
Assert-SharedResourceDirectories
Assert-UserDataPreserved

$smokeX64 = Start-Process -FilePath $application -ArgumentList '--smoke-test' -Wait -PassThru
if ($smokeX64.ExitCode -ne 0) {
    throw "Installed x64 GUI smoke test failed with exit code $($smokeX64.ExitCode)."
}

# Uninstall x86: its application directory goes, shared ProgramData resources and user data stay.
$uninstallerX86 = Get-SubornoUninstallerPath -Path $installDirectoryX86
$uninstallX86Log = Join-Path $PWD 'verify-evidence\uninstall-x86.log'
$uninstallX86 = Start-Process -FilePath $uninstallerX86 `
    -ArgumentList @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', "/LOG=$uninstallX86Log") -Wait -PassThru
if ($uninstallX86.ExitCode -ne 0) {
    throw "x86 uninstaller exited with code $($uninstallX86.ExitCode)."
}
if (Test-Path -LiteralPath $installDirectoryX86) {
    throw 'x86 installation directory remains after uninstall.'
}
if (-not (Test-Path -LiteralPath $installDirectory)) {
    throw 'x64 installation directory was removed by the x86 uninstall.'
}
if (-not (Test-Path -LiteralPath $service -PathType Leaf)) {
    throw 'The x64 TSF service was removed by the x86 uninstall.'
}
Assert-SharedResourceDirectories
Assert-UserDataPreserved

# Uninstall x64 last: shared ProgramData resources are removed only when no architecture remains.
$uninstallFinalLog = Join-Path $PWD 'verify-evidence\uninstall-x64-final.log'
$uninstall = Start-Process -FilePath $uninstaller `
    -ArgumentList @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', "/LOG=$uninstallFinalLog") -Wait -PassThru
if ($uninstall.ExitCode -ne 0) {
    throw "Suborno uninstaller exited with code $($uninstall.ExitCode). See $uninstallFinalLog."
}
if (Test-Path -LiteralPath $installDirectory) {
    throw 'Suborno installation directory remains after uninstall.'
}
if (Test-Path -LiteralPath $programDataDirectory) {
    throw 'Suborno service files remain in ProgramData after uninstall.'
}
Assert-UserDataPreserved
if (Test-Path -LiteralPath $desktopShortcut) {
    throw 'Desktop shortcut remains after uninstall.'
}
if (Test-Path -LiteralPath $startMenuShortcut) {
    throw 'Start Menu shortcut remains after uninstall.'
}
if (Test-Path -LiteralPath $quickLaunchShortcut) {
    throw 'Quick Launch shortcut remains after uninstall.'
}
if (Test-Path -LiteralPath $classKey) {
    throw 'The TSF COM server registration remains after uninstall.'
}
if (Get-ItemProperty -Path $startupKey -Name 'Suborno' -ErrorAction SilentlyContinue) {
    throw 'Suborno sign-in startup entry remains after uninstall.'
}

Write-Host 'x86 co-existence, shared ProgramData preservation, per-architecture uninstall, and last-architecture cleanup checks passed.'
Write-Host 'All installer checks passed.'
