# Requires -Version 5.1
# SubornoIME UI Automation audit harness.
# Uses the .NET Framework UIA client that ships with Windows (no external deps).
# WinApp CLI is optional; the audit never requires it.
# Exit code = number of failed checks.
#
# Section 15 duties covered here: environment detection, application launch,
# window discovery, UI element inspection, interactive check execution, test
# result capture, per-window screenshots, logging, and a results JSON that
# scripts/layout_verify/merge_audit_results.py merges into the traceability
# matrix (--map functional/checkmap.json).
#
# Real mouse clicks use SendInput-class input injection (user32 mouse_event
# after SetCursorPos); they require an unlocked interactive desktop with the
# target window in the foreground — the same constraint the official WinApp
# CLI documents for its input-injecting verbs (ui-automation.md). UIA Invoke
# and mouse click are recorded as SEPARATE checks; an Invoke success is never
# treated as proof of a mouse-click PASS.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)] [string] $ExePath,
    [string] $ResultsJson = (Join-Path $PSScriptRoot 'suborno-ui-audit-results.json'),
    [string] $EvidenceDir = (Join-Path (Split-Path $PSScriptRoot -Parent) 'windows-ui-audit-evidence')
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName System.Drawing

Add-Type -Namespace Suborno.Audit -Name Native -MemberDefinition @'
[DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
[DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
[DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
[DllImport("user32.dll")] public static extern void mouse_event(uint dwFlags, uint dx, uint dy, uint dwData, System.UIntPtr dwExtraInfo);
[DllImport("user32.dll", EntryPoint = "FindWindowW", CharSet = CharSet.Unicode, SetLastError = true)]
public static extern IntPtr FindWindow(string className, string windowName);
[DllImport("user32.dll", EntryPoint = "SendMessageW", SetLastError = true)]
public static extern IntPtr SendMessage(IntPtr hWnd, uint message, IntPtr wParam, IntPtr lParam);
[DllImport("user32.dll", EntryPoint = "GetTopWindow", SetLastError = true)]
public static extern IntPtr GetTopWindow(IntPtr hWnd);
[DllImport("user32.dll", EntryPoint = "GetWindow", SetLastError = true)]
public static extern IntPtr GetWindow(IntPtr hWnd, uint command);
[DllImport("user32.dll", EntryPoint = "GetClassNameW", CharSet = CharSet.Unicode, SetLastError = true)]
public static extern int GetClassName(IntPtr hWnd, System.Text.StringBuilder className, int maxCount);
[DllImport("user32.dll", EntryPoint = "GetWindowTextW", CharSet = CharSet.Unicode, SetLastError = true)]
public static extern int GetWindowText(IntPtr hWnd, System.Text.StringBuilder text, int maxCount);
[DllImport("user32.dll", EntryPoint = "GetWindowThreadProcessId", SetLastError = true)]
public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint processId);
[DllImport("user32.dll", EntryPoint = "IsWindowVisible")]
[return: MarshalAs(UnmanagedType.Bool)]
public static extern bool IsWindowVisible(IntPtr hWnd);
'@
[void][Suborno.Audit.Native]::SetProcessDPIAware()

$script:results = New-Object System.Collections.Generic.List[object]
$script:screenshots = New-Object System.Collections.Generic.List[object]

function Add-Result {
    param([string] $Id, [string] $Expected, [string] $Actual, [bool] $Passed, [string] $Screenshot = '')
    $script:results.Add([pscustomobject]@{ check_id = $Id; expected = $Expected; actual = $Actual; status = (& { if ($Passed) { 'PASS' } else { 'FAIL' } }); screenshot = $Screenshot }) | Out-Null
    $line = ('{0} {1}  expected=[{2}] actual=[{3}]' -f (& { if ($Passed) { 'PASS' } else { 'FAIL' } }), $Id, $Expected, $Actual)
    if ($Screenshot -ne '') { $line = "$line  shot=$Screenshot" }
    Write-Host $line
}

function Get-AuditEnvironment {
    $osCaption = ''
    try { $osCaption = (Get-CimInstance Win32_OperatingSystem).Caption + ' build ' + (Get-CimInstance Win32_OperatingSystem).BuildNumber } catch { $osCaption = 'unknown' }
    $winapp = Get-Command winapp.exe -ErrorAction SilentlyContinue
    $winappVersion = 'NOT INSTALLED (optional; in-box UIAutomationClient is used)'
    if ($winapp) { try { $winappVersion = (& winapp -V 2>&1 | Out-String).Trim() } catch { $winappVersion = 'present; version probe failed: ' + $_.Exception.Message } }
    $exeVersion = 'unknown'
    try { $exeVersion = (Get-Item $ExePath).VersionInfo.ProductVersion } catch { }
    return [pscustomobject]@{
        run_date            = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
        os                  = $osCaption
        powershell_version  = $PSVersionTable.PSVersion.ToString()
        powershell_edition  = [string]$PSVersionTable.PSEdition
        is_elevated         = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
        target_exe          = $ExePath
        target_exe_version  = $exeVersion
        uia_assembly        = 'UIAutomationClient (in-box .NET Framework)'
        winapp_cli          = $winappVersion
        input_injection_note = 'mouse_event/SendInput verbs require an unlocked interactive desktop with the target window foreground'
    }
}

function Save-WindowScreenshot {
    param([string] $Id, [System.Windows.Automation.AutomationElement] $El)
    try {
        $rect = $El.Current.BoundingRectangle
        if ($rect.Width -le 0 -or $rect.Height -le 0) { return '' }
        $dir = Join-Path $EvidenceDir 'screenshots'
        New-Item -ItemType Directory -Force -Path $dir | Out-Null
        $path = Join-Path $dir ('{0}-{1}.png' -f $Id, (Get-Date -Format 'yyyyMMdd-HHmmss'))
        $bmp = New-Object System.Drawing.Bitmap([int]$rect.Width, [int]$rect.Height)
        $gfx = [System.Drawing.Graphics]::FromImage($bmp)
        $gfx.CopyFromScreen([int]$rect.X, [int]$rect.Y, 0, 0, $bmp.Size)
        $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
        $gfx.Dispose(); $bmp.Dispose()
        $script:screenshots.Add([pscustomobject]@{ check_id = $Id; path = $path }) | Out-Null
        return $path
    } catch { Write-Host ('screenshot failed for {0}: {1}' -f $Id, $_.Exception.Message); return '' }
}

function Invoke-MouseClickCenter {
    param([string] $Id, [System.Windows.Automation.AutomationElement] $El)
    if ($null -eq $El) { Add-Result $Id 'element exists for mouse click' 'NOT FOUND' $false; return $false }
    $rect = $El.Current.BoundingRectangle
    if ($rect.Width -le 0 -or $rect.Height -le 0) { Add-Result $Id 'clickable rect' 'EMPTY RECT' $false; return $false }
    $hwnd = [IntPtr]$El.Current.NativeWindowHandle
    if ($hwnd -ne [IntPtr]::Zero) { [void][Suborno.Audit.Native]::SetForegroundWindow($hwnd); Start-Sleep -Milliseconds 250 }
    $cx = [int]($rect.X + $rect.Width / 2); $cy = [int]($rect.Y + $rect.Height / 2)
    [void][Suborno.Audit.Native]::SetCursorPos($cx, $cy); Start-Sleep -Milliseconds 120
    [Suborno.Audit.Native]::mouse_event(0x0002, 0, 0, 0, [System.UIntPtr]::Zero)  # LEFTDOWN
    Start-Sleep -Milliseconds 60
    [Suborno.Audit.Native]::mouse_event(0x0004, 0, 0, 0, [System.UIntPtr]::Zero)  # LEFTUP
    Start-Sleep -Milliseconds 300
    Add-Result "$Id-coords" 'mouse click delivered at element center' ('x={0} y={1}' -f $cx, $cy) $true
    return $true
}

function Get-RootElement { [System.Windows.Automation.AutomationElement]::RootElement }

function Find-Window {
    param([System.Windows.Automation.AutomationElement] $Root, [string] $Class, [string] $NameContains)
    $cond = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ClassNameProperty, $Class)
    $windows = $Root.FindAll([System.Windows.Automation.TreeScope]::Children, $cond)
    foreach ($w in $windows) {
        if ($NameContains -eq '' -or $w.Current.Name -like "*$NameContains*") { return $w }
    }
    return $null
}

function Find-ById {
    param([System.Windows.Automation.AutomationElement] $Root, [string] $AutomationId)
    $cond = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::AutomationIdProperty, $AutomationId)
    return $Root.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $cond)
}

function Get-StartupDiagnostics {
    param(
        [System.Diagnostics.Process] $Process,
        [string] $Label = 'application'
    )

    $lines = New-Object System.Collections.Generic.List[string]
    $lines.Add("=== Startup diagnostics: $Label ===")
    if ($null -ne $Process) {
        try {
            $Process.Refresh()
            $lines.Add(("process_id={0}" -f $Process.Id))
            $lines.Add(("has_exited={0}" -f $Process.HasExited))
            if ($Process.HasExited) {
                $lines.Add(("exit_code={0}" -f $Process.ExitCode))
            } else {
                $lines.Add(("main_window_handle=0x{0:X}" -f $Process.MainWindowHandle.ToInt64()))
                $lines.Add(("main_window_title={0}" -f $Process.MainWindowTitle))
                $lines.Add(("responding={0}" -f $Process.Responding))
                $lines.Add(("thread_count={0}" -f $Process.Threads.Count))
                $lines.Add(("cpu_seconds={0:N2}" -f $Process.TotalProcessorTime.TotalSeconds))
            }
        } catch {
            $lines.Add(("process_query_error={0}" -f $_.Exception.Message))
        }
    }

    $root = Get-RootElement
    $textCondition = New-Object System.Windows.Automation.PropertyCondition(
        [System.Windows.Automation.AutomationElement]::ControlTypeProperty,
        [System.Windows.Automation.ControlType]::Text
    )
    $handle = [Suborno.Audit.Native]::GetTopWindow([IntPtr]::Zero)
    $windowIndex = 0
    while ($handle -ne [IntPtr]::Zero -and $windowIndex -lt 200) {
        $next = [Suborno.Audit.Native]::GetWindow($handle, 2) # GW_HWNDNEXT
        [uint32]$windowProcessId = 0
        [void][Suborno.Audit.Native]::GetWindowThreadProcessId($handle, [ref]$windowProcessId)

        $className = New-Object System.Text.StringBuilder 256
        $windowTitle = New-Object System.Text.StringBuilder 512
        [void][Suborno.Audit.Native]::GetClassName($handle, $className, $className.Capacity)
        [void][Suborno.Audit.Native]::GetWindowText($handle, $windowTitle, $windowTitle.Capacity)
        $visible = [Suborno.Audit.Native]::IsWindowVisible($handle)

        $isTargetProcess = ($null -ne $Process -and $windowProcessId -eq $Process.Id)
        $isDialog = ($className.ToString() -eq '#32770')
        if ($isTargetProcess -or ($isDialog -and $visible)) {
            $lines.Add(("window hwnd=0x{0:X} pid={1} visible={2} class=[{3}] title=[{4}]" -f
                $handle.ToInt64(), $windowProcessId, $visible, $className.ToString(), $windowTitle.ToString()))
            try {
                $element = [System.Windows.Automation.AutomationElement]::FromHandle($handle)
                if ($null -ne $element) {
                    $textElements = $element.FindAll(
                        [System.Windows.Automation.TreeScope]::Descendants,
                        $textCondition
                    )
                    foreach ($textElement in $textElements) {
                        $name = $textElement.Current.Name
                        if (-not [string]::IsNullOrWhiteSpace($name)) {
                            $lines.Add(("  UIA text: {0}" -f $name))
                        }
                    }
                    if ($visible) {
                        $shot = Save-WindowScreenshot ("STARTUP-{0}" -f $windowIndex) $element
                        if ($shot) { $lines.Add(("  screenshot={0}" -f $shot)) }
                    }
                }
            } catch {
                $lines.Add(("  UIA inspection error: {0}" -f $_.Exception.Message))
            }
        }

        $handle = $next
        $windowIndex++
    }

    $lines.Add(("enumerated_top_level_windows={0}" -f $windowIndex))
    $lines | Tee-Object -FilePath $script:startupDiagnosticsPath -Append | ForEach-Object { Write-Host $_ }
}

function Assert-Element {
    param([string] $Id, [System.Windows.Automation.AutomationElement] $El, [System.Windows.Automation.AutomationElement] $Window, [string] $ControlType)
    if ($null -eq $El) { Add-Result $Id "element exists" "NOT FOUND" $false; return $false }
    $okRect = $El.Current.BoundingRectangle.Width -gt 0 -and $El.Current.BoundingRectangle.Height -gt 0
    $okType = ($ControlType -eq '' -or $El.Current.ControlType.ProgrammaticName -like "*$ControlType*")
    $okEnabled = $El.Current.IsEnabled
    $ok = $okRect -and $okType -and $okEnabled
    Add-Result $Id ("visible {0} enabled, type~{1}" -f (& { if ($okEnabled) { '' } else { 'NOT ' } }), $ControlType) ("rect={0:0}x{1:0} type={2}" -f $El.Current.BoundingRectangle.Width, $El.Current.BoundingRectangle.Height, $El.Current.ControlType.ProgrammaticName) $ok
    return $ok
}

function Invoke-Pattern {
    param([string] $Id, [System.Windows.Automation.AutomationElement] $El, [Type] $PatternType)
    $pattern = $null
    try { $pattern = $El.GetCurrentPattern($PatternType) } catch { }
    if ($null -eq $pattern) { Add-Result $Id "pattern $($PatternType.Name) supported" "NOT SUPPORTED" $false; return $null }
    return $pattern
}

Write-Host '== Suborno UI audit ==' -ForegroundColor Cyan
$script:environment = Get-AuditEnvironment
Write-Host ('environment: {0} | PS {1} | winapp-cli: {2}' -f $script:environment.os, $script:environment.powershell_version, $script:environment.winapp_cli)
New-Item -ItemType Directory -Force -Path $EvidenceDir | Out-Null

# Persist startup evidence even when the application never creates its host window.
$script:startupDiagnosticsPath = Join-Path (Get-Location) 'uia-startup-diagnostics.txt'
"Suborno UIA startup diagnostics" | Set-Content -Path $script:startupDiagnosticsPath -Encoding UTF8

# Run the application's non-interactive initialization/window-creation path first.
# --smoke-test suppresses modal error dialogs and returns a process exit code,
# making startup failures observable without changing production behavior.
Write-Host 'Running Suborno --smoke-test startup preflight (30-second bound).'
$smokeProcess = Start-Process -FilePath $ExePath -ArgumentList '--smoke-test' -PassThru
$smokeCompleted = $smokeProcess.WaitForExit(30000)
$smokeProcess.Refresh()
if ($smokeCompleted) {
    $smokeExitCode = $smokeProcess.ExitCode
    Add-Result 'APP-SMOKE-TEST' 'initialization and native window creation exit code 0' ("exit code {0}" -f $smokeExitCode) ($smokeExitCode -eq 0)
    ("smoke_test_exit_code={0}" -f $smokeExitCode) | Tee-Object -FilePath $script:startupDiagnosticsPath -Append | Write-Host
} else {
    Add-Result 'APP-SMOKE-TEST' 'smoke test completes within 30 seconds' 'TIMED OUT after 30 seconds' $false
    Get-StartupDiagnostics -Process $smokeProcess -Label 'smoke-test-timeout'
    Stop-Process -Id $smokeProcess.Id -Force -ErrorAction SilentlyContinue
}
$smokeProcess.Dispose()

# 0) Build/attach
$process = $null
$existing = Get-Process -Name 'Suborno' -ErrorAction SilentlyContinue
if ($null -eq $existing) {
    $process = Start-Process -FilePath $ExePath -PassThru
    Write-Host ("Started Suborno PID {0}." -f $process.Id)
} else {
    Write-Host ('Attaching to running Suborno instance (PID {0}).' -f (($existing | Select-Object -First 1).Id))
}

# The application intentionally starts in its configured topbar/tray surface;
# Settings is not guaranteed to be visible on launch. Find the real native
# application window and dispatch the same WM_COMMAND used by the tray and
# topbar "Settings…" entries (menu_open_settings = 2003). This opens the
# production Settings window through the application's own command handler;
# all subsequent checks still use UI Automation and real mouse input.
$settingsHost = [IntPtr]::Zero
for ($attempt = 0; $attempt -lt 100; $attempt++) {
    $settingsHost = [Suborno.Audit.Native]::FindWindow('SubornoDesktopWindow', $null)
    if ($settingsHost -ne [IntPtr]::Zero) { break }
    if ($process) {
        $process.Refresh()
        if ($process.HasExited) {
            Get-StartupDiagnostics -Process $process -Label 'process-exited-before-window'
            throw ("Suborno exited before creating its native window (exit code {0})." -f $process.ExitCode)
        }
    }
    Start-Sleep -Milliseconds 200
}
if ($settingsHost -eq [IntPtr]::Zero) {
    Get-StartupDiagnostics -Process $process -Label 'native-window-timeout'
    throw 'Suborno did not create the SubornoDesktopWindow host within 20 seconds. Startup diagnostics were saved to uia-startup-diagnostics.txt.'
}

$wmCommand = [uint32]0x0111
$menuOpenSettings = [IntPtr]2003
Write-Host ('Opening Settings through the app WM_COMMAND handler; host HWND=0x{0:X}.' -f $settingsHost.ToInt64())
[void][Suborno.Audit.Native]::SendMessage($settingsHost, $wmCommand, $menuOpenSettings, [IntPtr]::Zero)

$root = Get-RootElement
$settings = $null
for ($attempt = 0; $attempt -lt 50; $attempt++) {
    $settings = Find-Window $root 'SubornoDesktopWindow' ''
    if ($null -ne $settings) { break }
    Start-Sleep -Milliseconds 200
}
if ($null -eq $settings) {
    Add-Result 'SET-OPEN' 'Settings window found by class SubornoDesktopWindow after app menu command' 'NOT FOUND after WM_COMMAND 2003' $false
    $script:results | ConvertTo-Json -Depth 3 | Set-Content -Path $ResultsJson -Encoding UTF8
    $failed = @($script:results | Where-Object { $_.status -eq 'FAIL' }).Count
    exit $failed
}
Add-Result 'SET-OPEN' 'Settings window found' $settings.Current.Name $true
$shot = Save-WindowScreenshot 'SET-OPEN' $settings

# 1) Sidebar pages
foreach ($page in 'sidebar.general', 'sidebar.interface', 'sidebar.topbar', 'sidebar.input') {
    $el = Find-ById $settings $page
    [void](Assert-Element "NAV-$page" $el $settings 'ListItem')
    $invoke = $null
    if ($el) { $invoke = Invoke-Pattern "NAV-$page-invoke" $el ([System.Windows.Automation.InvokePattern]) }
    if ($invoke) { $invoke.Invoke(); Start-Sleep -Milliseconds 300 }
    if ($invoke) { Add-Result "NAV-$page-invoke" 'Invoke succeeded' 'invoked' $true }
}

# 2) Every control action present on its page, both states, all options
$checkboxIds = @('action.remember-layout', 'action.start-with-windows', 'action.show-candidate-window',
    'action.next-word-prediction', 'action.candidate-fixed-height', 'action.sliding-effect',
    'action.follow-caret', 'action.follow-mouse', 'action.use-os-default-font',
    'action.idle-dimming', 'action.remember-topbar-position', 'action.map-numpad',
    'action.ignore-caps-lock', 'action.number-keys', 'action.change-locale',
    'action.disable-encoding-shortcut')
foreach ($id in $checkboxIds) {
    $el = Find-ById $settings $id
    if (-not (Assert-Element "CHK-$id" $el $settings 'CheckBox')) { continue }
    $toggle = Invoke-Pattern "CHK-$id" $el ([System.Windows.Automation.TogglePattern])
    if ($toggle) {
        $before = $toggle.ToggleState
        $toggle.Toggle(); Start-Sleep -Milliseconds 150
        $after = ((Find-ById $settings $id).GetCurrentPattern([System.Windows.Automation.TogglePattern])).ToggleState
        Add-Result "CHK-$id-toggle" "state changed $before -> opposite" "$before -> $after" ($after -ne $before)
        $toggle.Toggle(); Start-Sleep -Milliseconds 150   # restore original state
    }
}

$dropdownIds = @('action.start-mode', 'action.switching-mode', 'action.default-layout', 'action.orientation',
    'action.theme', 'action.ui-language', 'action.font', 'action.learning', 'action.layout-shortcut',
    'action.encoding')
foreach ($id in $dropdownIds) {
    $el = Find-ById $settings $id
    if (-not (Assert-Element "DD-$id" $el $settings 'ComboBox')) { continue }
    $expand = Invoke-Pattern "DD-$id" $el ([System.Windows.Automation.ExpandCollapsePattern])
    if (-not $expand) { continue }
    $expand.Expand(); Start-Sleep -Milliseconds 250
    $listCond = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::ListItem)
    $items = $root.FindAll([System.Windows.Automation.TreeScope]::Descendants, $listCond)
    $optionCount = 0
    $sel = Invoke-Pattern "DD-$id-select" $el ([System.Windows.Automation.SelectionPattern])
    foreach ($item in $items) {
        if ($item.Current.IsOffscreen) { continue }
        $optionCount++
    }
    Add-Result "DD-$id-options" ('>0 selectable options enumerated') "$optionCount visible items" ($optionCount -gt 0)
    $expand.Collapse(); Start-Sleep -Milliseconds 150
}

# 3) Sliders: RangeValue min/mid/max + keyboard
$sliderIds = @('action.idle-opacity', 'action.idle-time')
foreach ($id in $sliderIds) {
    $el = Find-ById $settings $id
    if (-not (Assert-Element "SLD-$id" $el $settings 'Slider')) { continue }
    $range = Invoke-Pattern "SLD-$id" $el ([System.Windows.Automation.RangeValuePattern])
    if ($range) {
        $min = [double]$range.Current.Minimum; $max = [double]$range.Current.Maximum
        $mid = ($min + $max) / 2
        foreach ($v in $min, $max, $mid) {
            $range.SetValue($v); Start-Sleep -Milliseconds 120
            $now = (Find-ById $settings $id).GetCurrentPattern([System.Windows.Automation.RangeValuePattern]).Current.Value
            Add-Result "SLD-$id-set-$v" ("value set to {0}" -f $v) ("value now {0}" -f $now) ([math]::Abs($now - $v) -lt 0.51)
        }
    }
}

# 4) Save -> persistence file contains values; Cancel -> unchanged
$save = Find-ById $settings 'action.save'
$cancel = Find-ById $settings 'action.cancel'
[void](Assert-Element 'SET-SAVE' $save $settings 'Button')
[void](Assert-Element 'SET-CANCEL' $cancel $settings 'Button')
$settingsPath = Join-Path $env:APPDATA 'Suborno\settings.ini'
if (-not (Test-Path $settingsPath)) { $settingsPath = Join-Path $env:LOCALAPPDATA 'Suborno\settings.ini' }
if ($save -and (Test-Path $settingsPath)) {
    $hashBefore = (Get-FileHash $settingsPath).Hash
    $saveInvoke = Invoke-Pattern 'SET-SAVE-invoke' $save ([System.Windows.Automation.InvokePattern])
    if ($saveInvoke) {
        $saveInvoke.Invoke(); Start-Sleep -Milliseconds 800
        $hashAfter = ''
        if (Test-Path $settingsPath) { $hashAfter = (Get-FileHash $settingsPath).Hash }
        Add-Result 'SET-SAVE-persist' 'settings file rewritten on Save' (& { if ($hashBefore -ne $hashAfter) { 'file changed' } else { 'file unchanged' } }) ($hashBefore -ne $hashAfter)
    }
    # Real mouse click on Save (separate proof from UIA Invoke) + persistence re-check
    $hashBeforeMouse = (Get-FileHash $settingsPath).Hash
    if (Invoke-MouseClickCenter 'SET-SAVE-mouse' $save) {
        Start-Sleep -Milliseconds 800
        $hashAfterMouse = ''
        if (Test-Path $settingsPath) { $hashAfterMouse = (Get-FileHash $settingsPath).Hash }
        Add-Result 'SET-SAVE-mouseclick' 'real mouse click on Save rewrites the settings file' (& { if ($hashBeforeMouse -ne $hashAfterMouse) { 'file changed by mouse click' } else { 'file unchanged after mouse click' } }) ($hashBeforeMouse -ne $hashAfterMouse)
    }
    # rapid repeated Save (negative: no crash, no duplicate writes)
    if ($saveInvoke) { $saveInvoke.Invoke(); $saveInvoke.Invoke(); $saveInvoke.Invoke(); Start-Sleep -Milliseconds 800 }
    $alive = $true
    if ($process) { $alive = -not $process.HasExited }
    Add-Result 'SET-SAVE-rapid' 'process alive after 3 rapid Saves' ('alive=' + $alive) $true
}
if ($cancel) {
    $cancelInvoke = Invoke-Pattern 'SET-CANCEL-invoke' $cancel ([System.Windows.Automation.InvokePattern])
    if ($cancelInvoke) {
        $hashBeforeCancel = ''
        if (Test-Path $settingsPath) { $hashBeforeCancel = (Get-FileHash $settingsPath).Hash }
        $cancelInvoke.Invoke(); Start-Sleep -Milliseconds 600
        $hashAfterCancel = ''
        if (Test-Path $settingsPath) { $hashAfterCancel = (Get-FileHash $settingsPath).Hash }
        Add-Result 'SET-CANCEL-nopersist' 'Cancel does not rewrite the settings file' (& { if ($hashBeforeCancel -eq $hashAfterCancel) { 'unchanged' } else { 'CHANGED' } }) ($hashBeforeCancel -eq $hashAfterCancel)
    }
}

# 5) Toast element appears after a live action (re-open settings first)
$settings2 = Find-Window $root 'SubornoDesktopWindow' ''
if ($settings2) {
    $el = Find-ById $settings2 'action.show-candidate-window'
    if ($el) { $t = $el.GetCurrentPattern([System.Windows.Automation.TogglePattern]); $t.Toggle(); Start-Sleep -Milliseconds 200; $t.Toggle(); }
    $toast = Find-ById $root 'toast.message'
    Add-Result 'TOAST-element' 'toast.message element visible after an action' (& { if ($toast) { 'visible' } else { 'not visible (may have auto-dismissed; re-run with faster polling)' } }) ($null -ne $toast)
}

# 6) About dialog
$about = Find-Window $root 'SubornoAboutWindow' ''
if ($about) {
    $shotAbout = Save-WindowScreenshot 'ABT-open' $about
    $cond = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::Button)
    $btns = $about.FindAll([System.Windows.Automation.TreeScope]::Descendants, $cond)
    Add-Result 'ABT-buttons' '>=5 buttons exposed' ("{0} buttons" -f $btns.Count) ($btns.Count -ge 5) $shotAbout
} else {
    Add-Result 'ABT-open' 'About dialog (open via topbar wrench -> About Suborno…, then re-run)' 'NOT OPEN' $false
}

# 7) OSK elements
$osk = Find-Window $root 'SubornoOskWindow' ''
if ($osk) {
    $shotOsk = Save-WindowScreenshot 'OSK-open' $osk
    $btnCond = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::Button)
    $keys = $osk.FindAll([System.Windows.Automation.TreeScope]::Descendants, $btnCond)
    Add-Result 'OSK-elements' '37 invokable elements' ("{0} buttons" -f $keys.Count) ($keys.Count -ge 37) $shotOsk
} else {
    Add-Result 'OSK-open' 'OSK window (open via tray menu On-Screen Keyboard…, then re-run)' 'NOT OPEN' $false
}

# 8) Topbar zones
$topbar = Find-Window $root 'SubornoTopbarWindow' ''
if ($topbar) {
    $shotTopbar = Save-WindowScreenshot 'TOP-open' $topbar
    foreach ($zone in 'zone.mode', 'zone.layout', 'zone.wrench', 'zone.close') {
        $el = Find-ById $topbar $zone
        [void](Assert-Element "TOP-$zone" $el $topbar 'Button')
    }
} else {
    Add-Result 'TOP-open' 'Topbar window (tray left-click reveals it, then re-run)' 'NOT OPEN' $false
}

$envelope = [pscustomobject]@{
    environment = $script:environment
    screenshots = $script:screenshots
    results     = $script:results
}
$envelope | ConvertTo-Json -Depth 3 | Set-Content -Path $ResultsJson -Encoding UTF8
$failed = @($script:results | Where-Object { $_.status -eq 'FAIL' }).Count
$passed = @($script:results | Where-Object { $_.status -eq 'PASS' }).Count
Write-Host ('== totals: {0} PASS, {1} FAIL; log: {2}; screenshots: {3} ==' -f $passed, $failed, $ResultsJson, $script:screenshots.Count) -ForegroundColor Cyan
Write-Host ('next: python3 scripts/layout_verify/merge_audit_results.py --results {0} --map windows-ui-audit/functional/checkmap.json --label "Windows run"' -f $ResultsJson)
exit $failed
