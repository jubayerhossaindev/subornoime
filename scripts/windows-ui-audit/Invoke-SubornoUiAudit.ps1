# Requires -Version 5.1
# SubornoIME UI Automation audit harness.
# Uses the .NET Framework UIA client that ships with Windows (no external deps).
# WinApp CLI is optional; the audit never requires it.
# Exit code = number of failed/blocked checks.
#
# Section 15 duties covered here: environment detection, application launch,
# process-scoped window discovery, UI element inspection, interactive check
# execution, test result capture, per-window screenshots, structured failure
# classification, and a results JSON that
# scripts/layout_verify/merge_audit_results.py merges into the traceability
# matrix (--map functional/checkmap.json).
#
# Real mouse clicks use Win32 SendInput after desktop availability verification
# and controlled foreground activation with live BoundingRectangle re-query;
# UIA Invoke and mouse click remain strictly separate checks.

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

Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Text;

namespace Suborno.Audit
{
    public static class Native
    {
        public const int SW_RESTORE = 9;
        public const uint SWP_NOSIZE = 0x0001;
        public const uint SWP_NOMOVE = 0x0002;
        public const int SM_XVIRTUALSCREEN = 76;
        public const int SM_YVIRTUALSCREEN = 77;
        public const int SM_CXVIRTUALSCREEN = 78;
        public const int SM_CYVIRTUALSCREEN = 79;
        public const uint DESKTOP_READOBJECTS = 0x0001;
        public const uint DESKTOP_WRITEOBJECTS = 0x0080;

        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool SetProcessDPIAware();

        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool SetForegroundWindow(IntPtr hWnd);

        [DllImport("user32.dll")]
        public static extern IntPtr GetForegroundWindow();

        [DllImport("user32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool AllowSetForegroundWindow(int dwProcessId);

        [DllImport("user32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool BringWindowToTop(IntPtr hWnd);

        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);

        [DllImport("user32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool SetWindowPos(
            IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);

        [DllImport("user32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool AttachThreadInput(uint idAttach, uint idAttachTo, bool fAttach);

        [DllImport("kernel32.dll")]
        public static extern uint GetCurrentThreadId();

        [DllImport("user32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool SetCursorPos(int x, int y);

        [DllImport("user32.dll")]
        public static extern int GetSystemMetrics(int nIndex);

        [DllImport("user32.dll", EntryPoint = "FindWindowW", CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern IntPtr FindWindow(string className, string windowName);

        [DllImport("user32.dll", EntryPoint = "SendMessageW", SetLastError = true)]
        public static extern IntPtr SendMessage(IntPtr hWnd, uint message, IntPtr wParam, IntPtr lParam);

        [DllImport("user32.dll", EntryPoint = "GetClassNameW", CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int GetClassName(IntPtr hWnd, StringBuilder className, int maxCount);

        [DllImport("user32.dll", EntryPoint = "GetWindowTextW", CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int GetWindowText(IntPtr hWnd, StringBuilder text, int maxCount);

        [DllImport("user32.dll", EntryPoint = "GetWindowThreadProcessId", SetLastError = true)]
        public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint processId);

        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool IsWindow(IntPtr hWnd);

        [DllImport("user32.dll", EntryPoint = "IsWindowVisible")]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool IsWindowVisible(IntPtr hWnd);

        [DllImport("user32.dll", SetLastError = true)]
        public static extern IntPtr OpenInputDesktop(uint dwFlags, [MarshalAs(UnmanagedType.Bool)] bool fInherit, uint dwDesiredAccess);

        [DllImport("user32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool CloseDesktop(IntPtr hDesktop);
    }

    public static class WindowEnumerator
    {
        private delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

        [DllImport("user32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool EnumWindows(EnumWindowsProc callback, IntPtr extraData);

        public static IntPtr[] GetTopLevelWindows()
        {
            var handles = new List<IntPtr>();
            EnumWindowsProc callback = delegate(IntPtr hWnd, IntPtr lParam)
            {
                handles.Add(hWnd);
                return true;
            };

            if (!EnumWindows(callback, IntPtr.Zero))
            {
                throw new Win32Exception(Marshal.GetLastWin32Error(), "EnumWindows failed.");
            }

            return handles.ToArray();
        }
    }

    public static class InputSender
    {
        private const uint INPUT_MOUSE = 0;
        private const uint MOUSEEVENTF_LEFTDOWN = 0x0002;
        private const uint MOUSEEVENTF_LEFTUP = 0x0004;

        [StructLayout(LayoutKind.Sequential)]
        private struct MOUSEINPUT
        {
            public int dx;
            public int dy;
            public uint mouseData;
            public uint dwFlags;
            public uint time;
            public UIntPtr dwExtraInfo;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct KEYBDINPUT
        {
            public ushort wVk;
            public ushort wScan;
            public uint dwFlags;
            public uint time;
            public UIntPtr dwExtraInfo;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct HARDWAREINPUT
        {
            public uint uMsg;
            public ushort wParamL;
            public ushort wParamH;
        }

        [StructLayout(LayoutKind.Explicit)]
        private struct InputUnion
        {
            [FieldOffset(0)] public MOUSEINPUT mi;
            [FieldOffset(0)] public KEYBDINPUT ki;
            [FieldOffset(0)] public HARDWAREINPUT hi;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct INPUT
        {
            public uint type;
            public InputUnion U;
        }

        [DllImport("user32.dll", SetLastError = true)]
        private static extern uint SendInput(uint nInputs, [MarshalAs(UnmanagedType.LPArray), In] INPUT[] pInputs, int cbSize);

        private static bool SendMouseFlag(uint flag, out int win32Error)
        {
            var inputs = new INPUT[1];
            inputs[0].type = INPUT_MOUSE;
            inputs[0].U.mi.dwFlags = flag;
            uint sent = SendInput(1, inputs, Marshal.SizeOf(typeof(INPUT)));
            win32Error = sent == 1 ? 0 : Marshal.GetLastWin32Error();
            return sent == 1;
        }

        public static bool SendLeftDown(out int win32Error)
        {
            return SendMouseFlag(MOUSEEVENTF_LEFTDOWN, out win32Error);
        }

        public static bool SendLeftUp(out int win32Error)
        {
            return SendMouseFlag(MOUSEEVENTF_LEFTUP, out win32Error);
        }
    }
}
'@

[void][Suborno.Audit.Native]::SetProcessDPIAware()

$script:results = New-Object System.Collections.Generic.List[object]
$script:screenshots = New-Object System.Collections.Generic.List[object]
$script:lastUiaError = ''
$script:pageProbeControl = @{
    'sidebar.general'   = 'action.remember-layout'
    'sidebar.interface' = 'action.show-candidate-window'
    'sidebar.topbar'    = 'action.idle-dimming'
    'sidebar.input'     = 'action.map-numpad'
}

function Add-Result {
    param(
        [string] $Id,
        [string] $Expected,
        [string] $Actual,
        [bool] $Passed,
        [string] $Screenshot = '',
        [string] $StatusOverride = '',
        [string] $VerificationLevel = 'runtime-executed'
    )
    $status = if ($StatusOverride -ne '') {
        $StatusOverride
    } elseif ($Passed) {
        'PASS'
    } else {
        'FAIL'
    }
    $level = if ($status -eq 'BLOCKED') { 'blocked' } else { $VerificationLevel }
    $script:results.Add([pscustomobject]@{
        check_id           = $Id
        expected           = $Expected
        actual             = $Actual
        status             = $status
        verification_level = $level
        screenshot         = $Screenshot
    }) | Out-Null
    $line = ('{0} {1}  expected=[{2}] actual=[{3}]' -f $status, $Id, $Expected, $Actual)
    if ($Screenshot -ne '') { $line = "$line  shot=$Screenshot" }
    Write-Host $line
}

function Get-AuditEnvironment {
    $osCaption = ''
    try {
        $os = Get-CimInstance Win32_OperatingSystem
        $osCaption = $os.Caption + ' build ' + $os.BuildNumber
    } catch {
        $osCaption = 'unknown'
    }
    $winapp = Get-Command winapp.exe -ErrorAction SilentlyContinue
    $winappVersion = 'NOT INSTALLED (optional; in-box UIAutomationClient is used)'
    if ($winapp) {
        try {
            $winappVersion = (& winapp -V 2>&1 | Out-String).Trim()
        } catch {
            $winappVersion = 'present; version probe failed: ' + $_.Exception.Message
        }
    }
    $exeVersion = 'unknown'
    try { $exeVersion = (Get-Item $ExePath).VersionInfo.ProductVersion } catch { }
    return [pscustomobject]@{
        run_date             = (Get-Date -Format 'yyyy-MM-ddTHH:mm:ssK')
        os                   = $osCaption
        powershell_version   = $PSVersionTable.PSVersion.ToString()
        powershell_edition   = [string]$PSVersionTable.PSEdition
        is_elevated          = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
        target_exe           = $ExePath
        target_exe_version   = $exeVersion
        uia_assembly         = 'UIAutomationClient (in-box .NET Framework)'
        winapp_cli           = $winappVersion
        input_injection_note = 'Win32 SendInput verbs verify interactive desktop availability and foreground activation'
    }
}

function Get-RuntimeIdString {
    param([System.Windows.Automation.AutomationElement] $El)
    if ($null -eq $El) { return '' }
    try {
        $rid = $El.GetRuntimeId()
        if ($null -ne $rid -and $rid.Length -gt 0) {
            return ($rid -join '.')
        }
    } catch {
    }
    return ''
}

function Unwrap-UiaException {
    param([System.Exception] $Exception)
    $chain = New-Object 'System.Collections.Generic.List[System.Exception]'
    if ($null -eq $Exception) {
        return [pscustomobject]@{
            Outer = $null
            Inner = $null
            Chain = $chain
        }
    }
    $visited = New-Object 'System.Collections.Generic.HashSet[int]'
    $queue = New-Object 'System.Collections.Generic.Queue[System.Exception]'
    $queue.Enqueue($Exception)
    while ($queue.Count -gt 0) {
        $cur = $queue.Dequeue()
        if ($null -eq $cur) { continue }
        $identity = [System.Runtime.CompilerServices.RuntimeHelpers]::GetHashCode($cur)
        if (-not $visited.Add($identity)) { continue }
        $chain.Add($cur) | Out-Null
        if ($cur -is [System.AggregateException]) {
            foreach ($aggInner in $cur.InnerExceptions) {
                if ($null -ne $aggInner) { $queue.Enqueue($aggInner) }
            }
        } elseif ($null -ne $cur.InnerException) {
            $queue.Enqueue($cur.InnerException)
        }
    }

    $chosen = $chain[$chain.Count - 1]
    for ($i = $chain.Count - 1; $i -ge 0; $i--) {
        $cand = $chain[$i]
        if ($cand -is [System.Windows.Automation.ElementNotAvailableException] -or
            $cand -is [System.Runtime.InteropServices.COMException] -or
            $cand -is [System.TimeoutException] -or
            $cand -is [System.InvalidOperationException]) {
            $chosen = $cand
            break
        }
    }

    return [pscustomobject]@{
        Outer = $Exception
        Inner = $chosen
        Chain = $chain
    }
}

function Classify-UiaException {
    param(
        [System.Exception] $Exception,
        [string] $Operation = ''
    )
    if ($null -eq $Exception) { return 'UnexpectedError' }
    $unwrapped = Unwrap-UiaException -Exception $Exception
    $hrElemNotAvail = [int][uint32]::Parse('80040201', [System.Globalization.NumberStyles]::HexNumber)
    $hrPatternNotSupported = [int][uint32]::Parse('80040202', [System.Globalization.NumberStyles]::HexNumber)
    $hrInvalidOp = [int][uint32]::Parse('80131509', [System.Globalization.NumberStyles]::HexNumber)

    foreach ($ex in $unwrapped.Chain) {
        if ($ex -is [System.Windows.Automation.ElementNotAvailableException]) {
            return 'ElementUnavailable'
        }
        if ($ex -is [System.Runtime.InteropServices.COMException] -and $ex.HResult -eq $hrElemNotAvail) {
            return 'ElementUnavailable'
        }
    }
    foreach ($ex in $unwrapped.Chain) {
        if ($ex -is [System.TimeoutException]) {
            return 'Timeout'
        }
    }

    $isPatternAcquireOp = ($Operation -eq 'GetCurrentPattern' -or $Operation -like 'GetCurrentPattern*' -or $Operation -eq 'QueryPattern')
    foreach ($ex in $unwrapped.Chain) {
        if ($ex -is [System.Runtime.InteropServices.COMException] -and $ex.HResult -eq $hrPatternNotSupported) {
            return 'PatternUnsupported'
        }
        if ($isPatternAcquireOp -and $ex -is [System.InvalidOperationException] -and ($ex.HResult -eq $hrInvalidOp -or $ex.Message -like '*Pattern*' -or $ex.Message -like '*Unsupported*')) {
            return 'PatternUnsupported'
        }
    }
    foreach ($ex in $unwrapped.Chain) {
        if ($ex -is [System.InvalidOperationException]) {
            return 'InvalidOperation'
        }
    }
    foreach ($ex in $unwrapped.Chain) {
        if ($ex -is [System.Runtime.InteropServices.COMException]) {
            return 'ComFailure'
        }
    }
    return 'UnexpectedError'
}

function Format-UiaDiagnostic {
    param(
        [string] $Category,
        [string] $Operation,
        [string] $AutomationId = '',
        [string] $ControlType = '',
        [int] $ProcessId = 0,
        [IntPtr] $Hwnd = [IntPtr]::Zero,
        [string] $PatternName = '',
        [string] $RuntimeId = '',
        [string] $CurrentPage = '',
        [double] $ElapsedMs = 0,
        [System.Exception] $Exception = $null
    )
    $parts = New-Object 'System.Collections.Generic.List[string]'
    $parts.Add("category=$Category")
    $parts.Add("op=$Operation")
    if ($AutomationId -ne '') { $parts.Add("id=$AutomationId") }
    if ($ControlType -ne '') { $parts.Add("type=$ControlType") }
    if ($PatternName -ne '') { $parts.Add("pattern=$PatternName") }
    if ($CurrentPage -ne '') { $parts.Add("page=$CurrentPage") }
    if ($RuntimeId -ne '') { $parts.Add("rid=$RuntimeId") }
    if ($ProcessId -gt 0) { $parts.Add("pid=$ProcessId") }
    if ($Hwnd -ne [IntPtr]::Zero) { $parts.Add(('hwnd=0x{0:X}' -f $Hwnd.ToInt64())) }
    if ($ElapsedMs -gt 0) { $parts.Add(('elapsed={0:N0}ms' -f $ElapsedMs)) }
    if ($null -ne $Exception) {
        $unwrapped = Unwrap-UiaException -Exception $Exception
        $outer = $unwrapped.Outer
        $inner = $unwrapped.Inner
        $parts.Add(('outer={0}(0x{1:X8}): {2}' -f $outer.GetType().Name, $outer.HResult, $outer.Message))
        if ($null -ne $inner -and -not [object]::ReferenceEquals($outer, $inner)) {
            $parts.Add(('inner={0}(0x{1:X8}): {2}' -f $inner.GetType().FullName, $inner.HResult, $inner.Message))
        }
        if ($unwrapped.Chain.Count -gt 1) {
            $chainText = New-Object 'System.Collections.Generic.List[string]'
            for ($idx = 0; $idx -lt $unwrapped.Chain.Count; $idx++) {
                $item = $unwrapped.Chain[$idx]
                $chainText.Add(('{0}:{1}(0x{2:X8})' -f $idx, $item.GetType().Name, $item.HResult))
            }
            $parts.Add(('chain=[{0}]' -f ($chainText -join ' -> ')))
        }
    }
    return ($parts -join ' ')
}

function Save-WindowScreenshot {
    param([string] $Id, [System.Windows.Automation.AutomationElement] $El)
    if ($null -eq $El) { return '' }
    try {
        $rect = $El.Current.BoundingRectangle
        if ([double]::IsInfinity($rect.Width) -or [double]::IsNaN($rect.Width) -or $rect.Width -le 0 -or $rect.Height -le 0) {
            return ''
        }
        $dir = Join-Path $EvidenceDir 'screenshots'
        New-Item -ItemType Directory -Force -Path $dir | Out-Null
        $path = Join-Path $dir ('{0}-{1}.png' -f $Id, (Get-Date -Format 'yyyyMMdd-HHmmss'))
        $bmp = New-Object System.Drawing.Bitmap([int]$rect.Width, [int]$rect.Height)
        $gfx = [System.Drawing.Graphics]::FromImage($bmp)
        $gfx.CopyFromScreen([int]$rect.X, [int]$rect.Y, 0, 0, $bmp.Size)
        $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
        $gfx.Dispose()
        $bmp.Dispose()
        $script:screenshots.Add([pscustomobject]@{ check_id = $Id; path = $path }) | Out-Null
        return $path
    } catch {
        Write-Host ('screenshot failed for {0}: {1}' -f $Id, $_.Exception.Message)
        return ''
    }
}

function Get-RootElement {
    [System.Windows.Automation.AutomationElement]::RootElement
}

function Find-NativeWindowByClass {
    param(
        [Parameter(Mandatory = $true)] [string] $ClassName,
        [Parameter(Mandatory = $true)] [int] $ProcessId,
        [switch] $RequireVisible
    )

    foreach ($handle in [Suborno.Audit.WindowEnumerator]::GetTopLevelWindows()) {
        if (-not [Suborno.Audit.Native]::IsWindow($handle)) { continue }
        [uint32]$windowProcessId = 0
        $threadId = [Suborno.Audit.Native]::GetWindowThreadProcessId($handle, [ref]$windowProcessId)
        if ($threadId -eq 0 -or $windowProcessId -ne $ProcessId) {
            continue
        }
        if ($RequireVisible -and -not [Suborno.Audit.Native]::IsWindowVisible($handle)) {
            continue
        }
        $classNameBuffer = New-Object System.Text.StringBuilder 256
        [void][Suborno.Audit.Native]::GetClassName($handle, $classNameBuffer, $classNameBuffer.Capacity)
        if ($classNameBuffer.ToString() -ceq $ClassName) {
            return $handle
        }
    }

    return [IntPtr]::Zero
}

function Find-WindowByClassAndPid {
    param(
        [System.Windows.Automation.AutomationElement] $Root,
        [string] $Class,
        [int] $ProcessId = 0,
        [string] $NameContains = ''
    )
    if ($null -eq $Root) { return $null }
    try {
        $classCond = New-Object System.Windows.Automation.PropertyCondition(
            [System.Windows.Automation.AutomationElement]::ClassNameProperty,
            $Class
        )
        $cond = if ($ProcessId -gt 0) {
            New-Object System.Windows.Automation.AndCondition(
                $classCond,
                (New-Object System.Windows.Automation.PropertyCondition(
                    [System.Windows.Automation.AutomationElement]::ProcessIdProperty,
                    $ProcessId
                ))
            )
        } else {
            $classCond
        }
        $windows = $Root.FindAll([System.Windows.Automation.TreeScope]::Children, $cond)
        foreach ($w in $windows) {
            if ($NameContains -eq '' -or $w.Current.Name -like "*$NameContains*") {
                return $w
            }
        }
    } catch {
        $script:lastUiaError = Format-UiaDiagnostic -Category (Classify-UiaException $_.Exception) -Operation 'FindAll(RootChildren)' -ControlType $Class -ProcessId $ProcessId -Exception $_.Exception
    }
    return $null
}

function Wait-WindowByClass {
    param(
        [Parameter(Mandatory = $true)] [string] $ClassName,
        [Parameter(Mandatory = $true)] [int] $ProcessId,
        [int] $TimeoutMs = 5000,
        [int] $PollMs = 50,
        [System.Windows.Automation.AutomationElement] $FallbackRoot = $null,
        [switch] $IncludeHidden
    )

    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    while ($sw.ElapsedMilliseconds -lt $TimeoutMs) {
        $hwnd = if ($IncludeHidden) {
            Find-NativeWindowByClass -ClassName $ClassName -ProcessId $ProcessId
        } else {
            Find-NativeWindowByClass -ClassName $ClassName -ProcessId $ProcessId -RequireVisible
        }
        if ($hwnd -ne [IntPtr]::Zero) {
            try {
                $el = [System.Windows.Automation.AutomationElement]::FromHandle($hwnd)
                if ($null -ne $el -and $el.Current.ProcessId -eq $ProcessId) {
                    $sw.Stop()
                    return $el
                }
            } catch {
                $script:lastUiaError = Format-UiaDiagnostic -Category (Classify-UiaException $_.Exception) -Operation 'FromHandle' -ControlType $ClassName -ProcessId $ProcessId -Hwnd $hwnd -ElapsedMs $sw.ElapsedMilliseconds -Exception $_.Exception
            }
        }
        if ($null -ne $FallbackRoot -and -not $IncludeHidden) {
            $fallback = Find-WindowByClassAndPid -Root $FallbackRoot -Class $ClassName -ProcessId $ProcessId
            if ($null -ne $fallback) {
                $sw.Stop()
                return $fallback
            }
        }
        Start-Sleep -Milliseconds $PollMs
    }
    $sw.Stop()
    $script:lastUiaError = Format-UiaDiagnostic -Category 'Timeout' -Operation 'WaitWindowByClass' -ControlType $ClassName -ProcessId $ProcessId -ElapsedMs $sw.ElapsedMilliseconds
    return $null
}

function Wait-WindowClosed {
    param(
        [Parameter(Mandatory = $true)] [string] $ClassName,
        [Parameter(Mandatory = $true)] [int] $ProcessId,
        [int] $TimeoutMs = 2000,
        [int] $PollMs = 40
    )
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    while ($sw.ElapsedMilliseconds -lt $TimeoutMs) {
        $hwnd = Find-NativeWindowByClass -ClassName $ClassName -ProcessId $ProcessId -RequireVisible
        if ($hwnd -eq [IntPtr]::Zero) {
            return $true
        }
        Start-Sleep -Milliseconds $PollMs
    }
    return $false
}

function Try-FindById {
    param(
        [System.Windows.Automation.AutomationElement] $Root,
        [string] $AutomationId
    )
    $script:lastUiaError = ''
    if ($null -eq $Root) {
        $script:lastUiaError = Format-UiaDiagnostic -Category 'ElementNotFound' -Operation 'FindFirst(null root)' -AutomationId $AutomationId
        return $null
    }
    try {
        $cond = New-Object System.Windows.Automation.PropertyCondition(
            [System.Windows.Automation.AutomationElement]::AutomationIdProperty,
            $AutomationId
        )
        $el = $Root.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $cond)
        if ($null -eq $el) {
            $script:lastUiaError = Format-UiaDiagnostic -Category 'ElementNotFound' -Operation 'FindFirst' -AutomationId $AutomationId
        }
        return $el
    } catch {
        $script:lastUiaError = Format-UiaDiagnostic -Category (Classify-UiaException $_.Exception) -Operation 'FindFirst' -AutomationId $AutomationId -Exception $_.Exception
        return $null
    }
}

function Find-ById {
    param(
        [System.Windows.Automation.AutomationElement] $Root,
        [string] $AutomationId
    )
    return (Try-FindById -Root $Root -AutomationId $AutomationId)
}

function Try-GetElementSnapshot {
    param([System.Windows.Automation.AutomationElement] $El)
    if ($null -eq $El) {
        return [pscustomobject]@{
            Success = $false; Category = 'ElementNotFound'; Error = 'Element is null'
        }
    }
    try {
        $cur = $El.Current
        return [pscustomobject]@{
            Success            = $true
            Name               = [string]$cur.Name
            AutomationId       = [string]$cur.AutomationId
            ControlTypeName    = [string]$cur.ControlType.ProgrammaticName
            BoundingRectangle  = $cur.BoundingRectangle
            IsEnabled          = [bool]$cur.IsEnabled
            IsOffscreen        = [bool]$cur.IsOffscreen
            ProcessId          = [int]$cur.ProcessId
            NativeWindowHandle = [IntPtr]$cur.NativeWindowHandle
            Category           = ''
            Error              = ''
        }
    } catch {
        $cat = Classify-UiaException $_.Exception
        return [pscustomobject]@{
            Success  = $false
            Category = $cat
            Error    = Format-UiaDiagnostic -Category $cat -Operation 'CurrentSnapshot' -Exception $_.Exception
        }
    }
}

function Try-GetValueText {
    param([System.Windows.Automation.AutomationElement] $El)
    if ($null -eq $El) { return '' }
    try {
        $vp = $El.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern)
        if ($null -ne $vp) {
            return [string]$vp.Current.Value
        }
    } catch {
    }
    try {
        $raw = $El.GetCurrentPropertyValue([System.Windows.Automation.ValuePattern]::ValueProperty)
        if ($null -ne $raw -and $raw -ne [System.Windows.Automation.AutomationElement]::NotSupported) {
            return [string]$raw
        }
    } catch {
    }
    return ''
}

function Try-InvokePattern {
    param(
        [System.Windows.Automation.InvokePattern] $Pattern,
        [string] $AutomationId = '',
        [string] $RuntimeId = '',
        [string] $CurrentPage = ''
    )
    if ($null -eq $Pattern) {
        return [pscustomobject]@{ Success = $false; Category = 'PatternUnsupported'; HResult = 0; Error = 'InvokePattern is null' }
    }
    try {
        $Pattern.Invoke()
        return [pscustomobject]@{ Success = $true; Category = ''; HResult = 0; Error = '' }
    } catch {
        $unwrapped = Unwrap-UiaException -Exception $_.Exception
        $innerHr = if ($null -ne $unwrapped.Inner) { $unwrapped.Inner.HResult } else { $_.Exception.HResult }
        $cat = Classify-UiaException -Exception $_.Exception -Operation 'InvokePattern.Invoke'
        return [pscustomobject]@{
            Success  = $false
            Category = $cat
            HResult  = $innerHr
            Error    = Format-UiaDiagnostic -Category $cat -Operation 'InvokePattern.Invoke' -AutomationId $AutomationId -PatternName 'InvokePattern' -RuntimeId $RuntimeId -CurrentPage $CurrentPage -Exception $_.Exception
        }
    }
}

function Try-TogglePattern {
    param(
        [System.Windows.Automation.TogglePattern] $Pattern,
        [string] $AutomationId = ''
    )
    if ($null -eq $Pattern) {
        return [pscustomobject]@{ Success = $false; Category = 'PatternUnsupported'; HResult = 0; Error = 'TogglePattern is null' }
    }
    try {
        $Pattern.Toggle()
        return [pscustomobject]@{ Success = $true; Category = ''; HResult = 0; Error = '' }
    } catch {
        $unwrapped = Unwrap-UiaException -Exception $_.Exception
        $innerHr = if ($null -ne $unwrapped.Inner) { $unwrapped.Inner.HResult } else { $_.Exception.HResult }
        $cat = Classify-UiaException -Exception $_.Exception -Operation 'TogglePattern.Toggle'
        return [pscustomobject]@{
            Success  = $false
            Category = $cat
            HResult  = $innerHr
            Error    = Format-UiaDiagnostic -Category $cat -Operation 'TogglePattern.Toggle' -AutomationId $AutomationId -PatternName 'TogglePattern' -Exception $_.Exception
        }
    }
}

function Try-GetToggleState {
    param(
        [System.Windows.Automation.AutomationElement] $El,
        [string] $AutomationId = ''
    )
    if ($null -eq $El) {
        return [pscustomobject]@{ Success = $false; State = ''; Category = 'ElementNotFound'; Error = 'Element is null' }
    }
    try {
        $tp = $El.GetCurrentPattern([System.Windows.Automation.TogglePattern]::Pattern)
        if ($null -eq $tp) {
            return [pscustomobject]@{ Success = $false; State = ''; Category = 'PatternUnsupported'; Error = 'TogglePattern unavailable' }
        }
        return [pscustomobject]@{ Success = $true; State = [string]$tp.Current.ToggleState; Pattern = $tp; Category = ''; Error = '' }
    } catch {
        $cat = Classify-UiaException -Exception $_.Exception -Operation 'GetCurrentPattern'
        return [pscustomobject]@{
            Success  = $false
            State    = ''
            Category = $cat
            Error    = Format-UiaDiagnostic -Category $cat -Operation 'TogglePattern.Current.ToggleState' -AutomationId $AutomationId -PatternName 'TogglePattern' -RuntimeId (Get-RuntimeIdString $El) -Exception $_.Exception
        }
    }
}

function Try-ExpandCollapseOp {
    param(
        [System.Windows.Automation.ExpandCollapsePattern] $Pattern,
        [ValidateSet('Expand', 'Collapse')] [string] $Action,
        [string] $AutomationId = ''
    )
    if ($null -eq $Pattern) {
        return [pscustomobject]@{ Success = $false; Category = 'PatternUnsupported'; HResult = 0; Error = 'ExpandCollapsePattern is null' }
    }
    try {
        if ($Action -eq 'Expand') {
            $Pattern.Expand()
        } else {
            $Pattern.Collapse()
        }
        return [pscustomobject]@{ Success = $true; Category = ''; HResult = 0; Error = '' }
    } catch {
        $unwrapped = Unwrap-UiaException -Exception $_.Exception
        $innerHr = if ($null -ne $unwrapped.Inner) { $unwrapped.Inner.HResult } else { $_.Exception.HResult }
        $op = "ExpandCollapsePattern.$Action"
        $cat = Classify-UiaException -Exception $_.Exception -Operation $op
        return [pscustomobject]@{
            Success  = $false
            Category = $cat
            HResult  = $innerHr
            Error    = Format-UiaDiagnostic -Category $cat -Operation $op -AutomationId $AutomationId -PatternName 'ExpandCollapsePattern' -Exception $_.Exception
        }
    }
}

function Try-GetExpandCollapseState {
    param(
        [System.Windows.Automation.AutomationElement] $El,
        [string] $AutomationId = ''
    )
    if ($null -eq $El) {
        return [pscustomobject]@{ Success = $false; State = ''; Category = 'ElementNotFound'; Error = 'Element is null' }
    }
    try {
        $ep = $El.GetCurrentPattern([System.Windows.Automation.ExpandCollapsePattern]::Pattern)
        if ($null -eq $ep) {
            return [pscustomobject]@{ Success = $false; State = ''; Category = 'PatternUnsupported'; Error = 'ExpandCollapsePattern unavailable' }
        }
        return [pscustomobject]@{ Success = $true; State = [string]$ep.Current.ExpandCollapseState; Pattern = $ep; Category = ''; Error = '' }
    } catch {
        $cat = Classify-UiaException -Exception $_.Exception -Operation 'GetCurrentPattern'
        return [pscustomobject]@{
            Success  = $false
            State    = ''
            Category = $cat
            Error    = Format-UiaDiagnostic -Category $cat -Operation 'ExpandCollapseState' -AutomationId $AutomationId -PatternName 'ExpandCollapsePattern' -RuntimeId (Get-RuntimeIdString $El) -Exception $_.Exception
        }
    }
}

function Try-SetRangeValue {
    param(
        [System.Windows.Automation.RangeValuePattern] $Pattern,
        [double] $Value,
        [string] $AutomationId = ''
    )
    if ($null -eq $Pattern) {
        return [pscustomobject]@{ Success = $false; Category = 'PatternUnsupported'; Error = 'RangeValuePattern is null' }
    }
    try {
        $Pattern.SetValue($Value)
        return [pscustomobject]@{ Success = $true; Category = ''; Error = '' }
    } catch {
        $op = "RangeValuePattern.SetValue($Value)"
        $cat = Classify-UiaException -Exception $_.Exception -Operation $op
        return [pscustomobject]@{
            Success  = $false
            Category = $cat
            Error    = Format-UiaDiagnostic -Category $cat -Operation $op -AutomationId $AutomationId -PatternName 'RangeValuePattern' -Exception $_.Exception
        }
    }
}

function Try-GetRangeValue {
    param(
        [System.Windows.Automation.AutomationElement] $El,
        [string] $AutomationId = ''
    )
    if ($null -eq $El) {
        return [pscustomobject]@{ Success = $false; Value = [double]::NaN; Category = 'ElementNotFound'; Error = 'Element is null' }
    }
    try {
        $rp = $El.GetCurrentPattern([System.Windows.Automation.RangeValuePattern]::Pattern)
        if ($null -eq $rp) {
            return [pscustomobject]@{ Success = $false; Value = [double]::NaN; Category = 'PatternUnsupported'; Error = 'RangeValuePattern unavailable' }
        }
        return [pscustomobject]@{
            Success  = $true
            Value    = [double]$rp.Current.Value
            Minimum  = [double]$rp.Current.Minimum
            Maximum  = [double]$rp.Current.Maximum
            Pattern  = $rp
            Category = ''
            Error    = ''
        }
    } catch {
        $cat = Classify-UiaException -Exception $_.Exception -Operation 'GetCurrentPattern'
        return [pscustomobject]@{
            Success  = $false
            Value    = [double]::NaN
            Category = $cat
            Error    = Format-UiaDiagnostic -Category $cat -Operation 'RangeValuePattern.Current' -AutomationId $AutomationId -PatternName 'RangeValuePattern' -RuntimeId (Get-RuntimeIdString $El) -Exception $_.Exception
        }
    }
}

function Test-InteractiveDesktopAvailable {
    $access = [Suborno.Audit.Native]::DESKTOP_READOBJECTS -bor [Suborno.Audit.Native]::DESKTOP_WRITEOBJECTS
    $desktop = [Suborno.Audit.Native]::OpenInputDesktop(0, $false, $access)
    if ($desktop -eq [IntPtr]::Zero) {
        return $false
    }
    [void][Suborno.Audit.Native]::CloseDesktop($desktop)
    return $true
}

function Set-WindowForeground {
    param(
        [IntPtr] $TargetHwnd,
        [int] $TargetPid = 0
    )
    if ($TargetHwnd -eq [IntPtr]::Zero -or -not [Suborno.Audit.Native]::IsWindow($TargetHwnd)) {
        return $false
    }
    if ($TargetPid -gt 0) {
        [void][Suborno.Audit.Native]::AllowSetForegroundWindow($TargetPid)
    }
    $fgHwnd = [Suborno.Audit.Native]::GetForegroundWindow()
    [uint32]$unusedPid = 0
    $fgThread = if ($fgHwnd -ne [IntPtr]::Zero) { [Suborno.Audit.Native]::GetWindowThreadProcessId($fgHwnd, [ref]$unusedPid) } else { [uint32]0 }
    $curThread = [Suborno.Audit.Native]::GetCurrentThreadId()
    $attached = $false
    try {
        if ($fgThread -ne 0 -and $fgThread -ne $curThread) {
            $attached = [Suborno.Audit.Native]::AttachThreadInput($curThread, $fgThread, $true)
        }
        [void][Suborno.Audit.Native]::ShowWindow($TargetHwnd, [Suborno.Audit.Native]::SW_RESTORE)
        [void][Suborno.Audit.Native]::SetWindowPos($TargetHwnd, [IntPtr]::Zero, 0, 0, 0, 0, ([Suborno.Audit.Native]::SWP_NOMOVE -bor [Suborno.Audit.Native]::SWP_NOSIZE))
        [void][Suborno.Audit.Native]::BringWindowToTop($TargetHwnd)
        [void][Suborno.Audit.Native]::SetForegroundWindow($TargetHwnd)
    } finally {
        if ($attached) {
            [void][Suborno.Audit.Native]::AttachThreadInput($curThread, $fgThread, $false)
        }
    }

    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    while ($sw.ElapsedMilliseconds -lt 1000) {
        if ([Suborno.Audit.Native]::GetForegroundWindow() -eq $TargetHwnd) {
            return $true
        }
        Start-Sleep -Milliseconds 40
    }
    return ([Suborno.Audit.Native]::GetForegroundWindow() -eq $TargetHwnd)
}

function Test-RectInVirtualDesktop {
    param([System.Windows.Rect] $Rect)
    if ([double]::IsNaN($Rect.Width) -or [double]::IsNaN($Rect.Height) -or $Rect.Width -le 0 -or $Rect.Height -le 0) {
        return $false
    }
    $vx = [Suborno.Audit.Native]::GetSystemMetrics([Suborno.Audit.Native]::SM_XVIRTUALSCREEN)
    $vy = [Suborno.Audit.Native]::GetSystemMetrics([Suborno.Audit.Native]::SM_YVIRTUALSCREEN)
    $vw = [Suborno.Audit.Native]::GetSystemMetrics([Suborno.Audit.Native]::SM_CXVIRTUALSCREEN)
    $vh = [Suborno.Audit.Native]::GetSystemMetrics([Suborno.Audit.Native]::SM_CYVIRTUALSCREEN)
    if ($vw -le 0 -or $vh -le 0) { return $true }
    $cx = $Rect.X + ($Rect.Width / 2.0)
    $cy = $Rect.Y + ($Rect.Height / 2.0)
    return ($cx -ge $vx -and $cx -lt ($vx + $vw) -and $cy -ge $vy -and $cy -lt ($vy + $vh))
}

function Invoke-MouseClickCenter {
    param(
        [string] $Id,
        [System.Windows.Automation.AutomationElement] $ContainerWindow,
        [string] $AutomationId,
        [IntPtr] $FocusHwnd = [IntPtr]::Zero,
        [int] $ProcessId = 0
    )
    if (-not (Test-InteractiveDesktopAvailable)) {
        Add-Result "$Id-coords" 'interactive desktop available for SendInput' 'BLOCKED: input desktop locked or non-interactive' $false -StatusOverride 'BLOCKED'
        return 'BLOCKED'
    }

    $targetHwnd = $FocusHwnd
    if ($targetHwnd -eq [IntPtr]::Zero -and $null -ne $ContainerWindow) {
        $winSnap = Try-GetElementSnapshot $ContainerWindow
        if ($winSnap.Success) { $targetHwnd = $winSnap.NativeWindowHandle }
    }
    if ($targetHwnd -ne [IntPtr]::Zero) {
        $fgOk = Set-WindowForeground -TargetHwnd $targetHwnd -TargetPid $ProcessId
        if (-not $fgOk) {
            $actualFg = [Suborno.Audit.Native]::GetForegroundWindow()
            Add-Result "$Id-coords" ('target hwnd 0x{0:X} is foreground before SendInput' -f $targetHwnd.ToInt64()) ('FOREGROUND LOCKED: actual fg=0x{0:X}' -f $actualFg.ToInt64()) $false -StatusOverride 'BLOCKED'
            return 'BLOCKED'
        }
        Start-Sleep -Milliseconds 120
    }

    $freshEl = if ($null -ne $ContainerWindow -and $AutomationId -ne '') {
        Try-FindById $ContainerWindow $AutomationId
    } else {
        $null
    }
    if ($null -eq $freshEl) {
        Add-Result "$Id-coords" 'element exists for live mouse click' ("NOT FOUND: $script:lastUiaError") $false
        return 'FAIL'
    }
    $snap = Try-GetElementSnapshot $freshEl
    if (-not $snap.Success) {
        Add-Result "$Id-coords" 'live element snapshot available' $snap.Error $false
        return 'FAIL'
    }
    $rect = $snap.BoundingRectangle
    if (-not (Test-RectInVirtualDesktop $rect)) {
        Add-Result "$Id-coords" 'clickable non-empty rect within virtual desktop' ('INVALID RECT x={0:0} y={1:0} w={2:0} h={3:0}' -f $rect.X, $rect.Y, $rect.Width, $rect.Height) $false
        return 'FAIL'
    }

    $cx = [int][math]::Round($rect.X + $rect.Width / 2.0)
    $cy = [int][math]::Round($rect.Y + $rect.Height / 2.0)
    [void][Suborno.Audit.Native]::SetCursorPos($cx, $cy)
    Start-Sleep -Milliseconds 80
    $errDown = 0
    $errUp = 0
    if (-not [Suborno.Audit.InputSender]::SendLeftDown([ref]$errDown)) {
        Add-Result "$Id-coords" 'SendInput LEFTDOWN succeeds' ("SendInput LEFTDOWN failed win32=$errDown") $false
        return 'FAIL'
    }
    Start-Sleep -Milliseconds 50
    if (-not [Suborno.Audit.InputSender]::SendLeftUp([ref]$errUp)) {
        Add-Result "$Id-coords" 'SendInput LEFTUP succeeds' ("SendInput LEFTUP failed win32=$errUp") $false
        return 'FAIL'
    }
    Start-Sleep -Milliseconds 250
    $fgNow = [Suborno.Audit.Native]::GetForegroundWindow()
    Add-Result "$Id-coords" 'mouse click delivered via SendInput at live element center' ('x={0} y={1} fg=0x{2:X}' -f $cx, $cy, $fgNow.ToInt64()) $true
    return 'OK'
}

function Select-SettingsPage {
    param(
        [System.Windows.Automation.AutomationElement] $SettingsWindow,
        [string] $PageId
    )
    $script:lastUiaError = ''
    $pageEl = Try-FindById $SettingsWindow $PageId
    if ($null -eq $pageEl) {
        if ($script:lastUiaError -eq '') {
            $script:lastUiaError = Format-UiaDiagnostic -Category 'ElementNotFound' -Operation 'SelectSettingsPage.FindSidebar' -AutomationId $PageId -CurrentPage $PageId
        }
        return $false
    }
    $rid = Get-RuntimeIdString $pageEl
    $patRes = Get-UiaPattern -El $pageEl -PatternType ([System.Windows.Automation.InvokePattern]) -AutomationId $PageId -CurrentPage $PageId
    if (-not $patRes.Success) {
        $script:lastUiaError = $patRes.Error
        return $false
    }
    $res = Try-InvokePattern -Pattern $patRes.Pattern -AutomationId $PageId -RuntimeId $rid -CurrentPage $PageId
    if (-not $res.Success) {
        $script:lastUiaError = $res.Error
        return $false
    }

    $probeId = $script:pageProbeControl[$PageId]
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    while ($sw.ElapsedMilliseconds -lt 2500) {
        if ([string]::IsNullOrEmpty($probeId) -or $null -ne (Try-FindById $SettingsWindow $probeId)) {
            $sw.Stop()
            Start-Sleep -Milliseconds 50
            $script:lastUiaError = ''
            return $true
        }
        Start-Sleep -Milliseconds 40
    }
    $sw.Stop()
    $probeDetail = if ($script:lastUiaError -ne '') { "; lastLookup=[$script:lastUiaError]" } else { '' }
    $script:lastUiaError = ("category=Timeout op=SelectSettingsPage.WaitProbe targetPage=$PageId expectedProbe=$probeId rid=$rid elapsed={0:N0}ms{1}" -f $sw.ElapsedMilliseconds, $probeDetail)
    return $false
}

function Open-SettingsWindow {
    param(
        [System.Windows.Automation.AutomationElement] $Root,
        [IntPtr] $HostHwnd,
        [int] $ProcessId,
        [uint32] $WmCommand,
        [IntPtr] $MenuOpenSettings
    )
    [void][Suborno.Audit.Native]::SendMessage($HostHwnd, $WmCommand, $MenuOpenSettings, [IntPtr]::Zero)
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    while ($sw.ElapsedMilliseconds -lt 5000) {
        if ([Suborno.Audit.Native]::IsWindowVisible($HostHwnd)) {
            try {
                $byHandle = [System.Windows.Automation.AutomationElement]::FromHandle($HostHwnd)
                if ($null -ne $byHandle -and $byHandle.Current.ProcessId -eq $ProcessId -and $null -ne (Try-FindById $byHandle 'sidebar.general')) {
                    return $byHandle
                }
            } catch {
            }
        }
        $win = Find-WindowByClassAndPid -Root $Root -Class 'SubornoDesktopWindow' -ProcessId $ProcessId
        if ($null -ne $win -and $null -ne (Try-FindById $win 'sidebar.general')) {
            return $win
        }
        Start-Sleep -Milliseconds 80
    }
    return $null
}

function Wait-WindowHidden {
    param(
        [IntPtr] $Hwnd,
        [int] $TimeoutMs = 2000
    )
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    while ($sw.ElapsedMilliseconds -lt $TimeoutMs) {
        if (-not [Suborno.Audit.Native]::IsWindowVisible($Hwnd)) {
            return $true
        }
        Start-Sleep -Milliseconds 40
    }
    return (-not [Suborno.Audit.Native]::IsWindowVisible($Hwnd))
}

function Get-SettingsIniPath {
    $candidates = @(
        (Join-Path $env:APPDATA 'Suborno\settings.ini'),
        (Join-Path $env:LOCALAPPDATA 'Suborno\settings.ini')
    )
    foreach ($candidate in $candidates) {
        if (-not [string]::IsNullOrWhiteSpace($candidate) -and (Test-Path $candidate)) {
            return $candidate
        }
    }
    return $candidates[0]
}

function Get-IniKeyValue {
    param(
        [string] $Path,
        [string] $Key
    )
    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path $Path)) { return $null }
    $prefix = "$Key="
    foreach ($line in (Get-Content -Path $Path -Encoding UTF8)) {
        if ($line.StartsWith($prefix, [System.StringComparison]::Ordinal)) {
            return $line.Substring($prefix.Length).Trim()
        }
    }
    return $null
}

function Wait-SettingsIniKey {
    param(
        [string] $Path,
        [string] $Key,
        [string] $ExpectedValue,
        [int] $TimeoutMs = 1500
    )
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    while ($sw.ElapsedMilliseconds -lt $TimeoutMs) {
        $actual = Get-IniKeyValue -Path $Path -Key $Key
        if ($actual -eq $ExpectedValue) {
            return $actual
        }
        Start-Sleep -Milliseconds 50
    }
    return (Get-IniKeyValue -Path $Path -Key $Key)
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

    $textCondition = New-Object System.Windows.Automation.PropertyCondition(
        [System.Windows.Automation.AutomationElement]::ControlTypeProperty,
        [System.Windows.Automation.ControlType]::Text
    )
    $topLevelWindows = [Suborno.Audit.WindowEnumerator]::GetTopLevelWindows()
    $windowIndex = 0
    foreach ($handle in $topLevelWindows) {
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

        $windowIndex++
    }

    $lines.Add(("enumerated_top_level_windows={0}" -f $windowIndex))
    $lines | Tee-Object -FilePath $script:startupDiagnosticsPath -Append | ForEach-Object { Write-Host $_ }
}

function Write-UIATreeDiagnostics {
    param(
        [System.Windows.Automation.AutomationElement] $Window,
        [string] $Label
    )

    if ($null -eq $Window) {
        Write-Host ("UIA tree [{0}]: window element is null." -f $Label)
        return
    }

    try {
        $snap = Try-GetElementSnapshot $Window
        if ($snap.Success) {
            Write-Host ("UIA tree [{0}] root: Name=[{1}] AutomationId=[{2}] ControlType=[{3}] HWND=0x{4:X} PID={5}" -f
                $Label, $snap.Name, $snap.AutomationId, $snap.ControlTypeName, $snap.NativeWindowHandle.ToInt64(), $snap.ProcessId)
        }
        $elements = $Window.FindAll([System.Windows.Automation.TreeScope]::Descendants, [System.Windows.Automation.Condition]::TrueCondition)
        Write-Host ("UIA tree [{0}] descendant_count={1}" -f $Label, $elements.Count)
        $limit = [math]::Min($elements.Count, 120)
        for ($index = 0; $index -lt $limit; $index++) {
            $s = Try-GetElementSnapshot $elements.Item($index)
            if ($s.Success) {
                Write-Host ("  [{0}] Name=[{1}] AutomationId=[{2}] Type=[{3}] HWND=0x{4:X}" -f
                    $index, $s.Name, $s.AutomationId, $s.ControlTypeName, $s.NativeWindowHandle.ToInt64())
            }
        }
    } catch {
        Write-Host ("UIA tree [{0}] inspection failed: {1}" -f $Label, $_.Exception.Message)
    }
}

function Assert-Element {
    param(
        [string] $Id,
        [System.Windows.Automation.AutomationElement] $El,
        [System.Windows.Automation.AutomationElement] $Window,
        [string] $ControlType
    )
    if ($null -eq $El) {
        $msg = if ($script:lastUiaError -ne '') { "NOT FOUND ($script:lastUiaError)" } else { 'NOT FOUND' }
        Add-Result $Id 'element exists' $msg $false
        return $false
    }
    $snap = Try-GetElementSnapshot $El
    if (-not $snap.Success) {
        Add-Result $Id ("visible enabled, type~$ControlType") ("SNAPSHOT FAILED: $($snap.Error)") $false
        return $false
    }
    $okRect = ($snap.BoundingRectangle.Width -gt 0 -and $snap.BoundingRectangle.Height -gt 0)
    $okType = ($ControlType -eq '' -or $snap.ControlTypeName -like "*$ControlType*")
    $okEnabled = $snap.IsEnabled
    $ok = $okRect -and $okType -and $okEnabled
    Add-Result $Id ("visible enabled, type~$ControlType") ("enabled=$($snap.IsEnabled) rect=$($snap.BoundingRectangle.Width.ToString('0'))x$($snap.BoundingRectangle.Height.ToString('0')) type=$($snap.ControlTypeName)") $ok
    return $ok
}

function Get-UiaPattern {
    param(
        [System.Windows.Automation.AutomationElement] $El,
        [Type] $PatternType,
        [string] $AutomationId = '',
        [string] $CurrentPage = ''
    )
    if ($null -eq $El) {
        return [pscustomobject]@{
            Success      = $false
            Pattern      = $null
            Availability = 'element null'
            Category     = 'ElementNotFound'
            HResult      = 0
            Error        = Format-UiaDiagnostic -Category 'ElementNotFound' -Operation 'GetCurrentPattern' -AutomationId $AutomationId -PatternName $PatternType.Name -CurrentPage $CurrentPage
        }
    }
    $patternObject = switch ($PatternType.Name) {
        'InvokePattern'         { [System.Windows.Automation.InvokePattern]::Pattern }
        'TogglePattern'         { [System.Windows.Automation.TogglePattern]::Pattern }
        'ExpandCollapsePattern' { [System.Windows.Automation.ExpandCollapsePattern]::Pattern }
        'RangeValuePattern'     { [System.Windows.Automation.RangeValuePattern]::Pattern }
        default                 { $null }
    }
    $availabilityProperty = switch ($PatternType.Name) {
        'InvokePattern'         { [System.Windows.Automation.AutomationElement]::IsInvokePatternAvailableProperty }
        'TogglePattern'         { [System.Windows.Automation.AutomationElement]::IsTogglePatternAvailableProperty }
        'ExpandCollapsePattern' { [System.Windows.Automation.AutomationElement]::IsExpandCollapsePatternAvailableProperty }
        'RangeValuePattern'     { [System.Windows.Automation.AutomationElement]::IsRangeValuePatternAvailableProperty }
        default                 { $null }
    }
    if ($null -eq $patternObject) {
        return [pscustomobject]@{
            Success      = $false
            Pattern      = $null
            Availability = 'unmapped'
            Category     = 'PatternUnsupported'
            HResult      = 0
            Error        = ('UNMAPPED pattern type: ' + $PatternType.FullName)
        }
    }
    $rid = Get-RuntimeIdString $El
    $availability = 'not queried'
    if ($null -ne $availabilityProperty) {
        try {
            $availability = [string]$El.GetCurrentPropertyValue($availabilityProperty)
        } catch {
            $unwAvail = Unwrap-UiaException -Exception $_.Exception
            $availEx = if ($null -ne $unwAvail.Inner) { $unwAvail.Inner } else { $_.Exception }
            if ($null -ne $availEx) {
                $availability = ('property error {0}(0x{1:X8}): {2}' -f $availEx.GetType().Name, $availEx.HResult, $availEx.Message)
            } else {
                $availability = 'property error (unknown)'
            }
        }
    }
    $pattern = $null
    $patternError = ''
    $cat = ''
    $hr = 0
    try {
        $pattern = $El.GetCurrentPattern($patternObject)
    } catch {
        $unwrapped = Unwrap-UiaException -Exception $_.Exception
        $hr = if ($null -ne $unwrapped.Inner) { $unwrapped.Inner.HResult } else { $_.Exception.HResult }
        $cat = Classify-UiaException -Exception $_.Exception -Operation 'GetCurrentPattern'
        $patternError = Format-UiaDiagnostic -Category $cat -Operation 'GetCurrentPattern' -AutomationId $AutomationId -PatternName $PatternType.Name -RuntimeId $rid -CurrentPage $CurrentPage -Exception $_.Exception
    }
    if ($null -eq $pattern) {
        $actual = "NOT SUPPORTED; availability=$availability"
        if ($patternError -ne '') { $actual += "; $patternError" }
        return [pscustomobject]@{
            Success      = $false
            Pattern      = $null
            Availability = $availability
            Category     = $(if ($cat -ne '') { $cat } else { 'PatternUnsupported' })
            HResult      = $hr
            Error        = $actual
        }
    }
    Write-Host ("UIA pattern diagnostic [{0}]: pattern={1} availability={2} rid={3} GetCurrentPattern=OK" -f $AutomationId, $PatternType.Name, $availability, $rid)
    return [pscustomobject]@{
        Success      = $true
        Pattern      = $pattern
        Availability = $availability
        Category     = ''
        HResult      = 0
        Error        = ''
    }
}

function Invoke-Pattern {
    param(
        [string] $Id,
        [System.Windows.Automation.AutomationElement] $El,
        [Type] $PatternType,
        [string] $AutomationId = ''
    )
    $res = Get-UiaPattern -El $El -PatternType $PatternType -AutomationId $(if ($AutomationId -ne '') { $AutomationId } else { $Id })
    if (-not $res.Success) {
        $script:lastUiaError = $res.Error
    }
    return $res
}

Write-Host '== Suborno UI audit ==' -ForegroundColor Cyan
$script:environment = Get-AuditEnvironment
Write-Host ('environment: {0} | PS {1} | winapp-cli: {2}' -f $script:environment.os, $script:environment.powershell_version, $script:environment.winapp_cli)
New-Item -ItemType Directory -Force -Path $EvidenceDir | Out-Null

$script:startupDiagnosticsPath = Join-Path (Get-Location) 'uia-startup-diagnostics.txt'
'Suborno UIA startup diagnostics' | Set-Content -Path $script:startupDiagnosticsPath -Encoding UTF8

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

# 0) Launch or attach strictly scoped to target PID.
$existing = Get-Process -Name 'Suborno' -ErrorAction SilentlyContinue | Select-Object -First 1
if ($null -eq $existing) {
    $process = Start-Process -FilePath $ExePath -PassThru
    Write-Host ("Started Suborno PID {0}." -f $process.Id)
} else {
    $process = Get-Process -Id $existing.Id
    Write-Host ("Attaching to running Suborno PID {0}." -f $process.Id)
}

$settingsHost = [IntPtr]::Zero
$windowStartupTimeoutMs = 60000
$windowPollIntervalMs = 200
$startupStopwatch = [System.Diagnostics.Stopwatch]::StartNew()
while ($startupStopwatch.ElapsedMilliseconds -lt $windowStartupTimeoutMs) {
    $settingsHost = Find-NativeWindowByClass -ClassName 'SubornoDesktopWindow' -ProcessId $process.Id
    if ($settingsHost -ne [IntPtr]::Zero) { break }

    $process.Refresh()
    if ($process.HasExited) {
        $startupStopwatch.Stop()
        Get-StartupDiagnostics -Process $process -Label 'process-exited-before-window'
        throw ("Suborno exited before creating its native window (exit code {0})." -f $process.ExitCode)
    }

    Start-Sleep -Milliseconds $windowPollIntervalMs
}
$startupStopwatch.Stop()
Write-Host ("Native host discovery elapsed: {0:N0} ms; timeout: {1:N0} ms." -f
    $startupStopwatch.Elapsed.TotalMilliseconds, $windowStartupTimeoutMs)

if ($settingsHost -eq [IntPtr]::Zero) {
    Get-StartupDiagnostics -Process $process -Label 'native-window-timeout'
    throw ("Suborno did not create the SubornoDesktopWindow host within {0} seconds. Startup diagnostics were saved to uia-startup-diagnostics.txt." -f
        [math]::Ceiling($windowStartupTimeoutMs / 1000))
}

$wmCommand = [uint32]0x0111
$wmClose = [uint32]0x0010
$wmSecondInstanceShowTopbar = [uint32]0x8002
$menuOpenSettings = [IntPtr]2003
$menuOsk = [IntPtr]2007
$menuAbout = [IntPtr]2011

Write-Host ('Opening Settings through WM_COMMAND handler; host HWND=0x{0:X} PID={1}.' -f $settingsHost.ToInt64(), $process.Id)
$root = Get-RootElement
$settings = Open-SettingsWindow -Root $root -HostHwnd $settingsHost -ProcessId $process.Id -WmCommand $wmCommand -MenuOpenSettings $menuOpenSettings
if ($null -eq $settings) {
    Add-Result 'SET-OPEN' 'Settings window found by class SubornoDesktopWindow after app menu command' 'NOT FOUND after WM_COMMAND 2003' $false
    $envelope = [pscustomobject]@{ environment = $script:environment; screenshots = $script:screenshots; results = $script:results }
    $envelope | ConvertTo-Json -Depth 4 | Set-Content -Path $ResultsJson -Encoding UTF8
    exit (@($script:results | Where-Object { $_.status -ne 'PASS' }).Count)
}
$settingsSnap = Try-GetElementSnapshot $settings
Add-Result 'SET-OPEN' 'Settings window found' ($(if ($settingsSnap.Success) { $settingsSnap.Name } else { 'Suborno Settings' })) $true
Write-UIATreeDiagnostics -Window $settings -Label 'settings-root'
$shot = Save-WindowScreenshot 'SET-OPEN' $settings

# 1) Sidebar pages (also verify persistent shell RuntimeId stability + old-page stale invalidation)
$shellSidebarProbe = Try-FindById $settings 'sidebar.general'
$shellSaveProbe = Try-FindById $settings 'action.save'
$shellSidebarRidBefore = Get-RuntimeIdString $shellSidebarProbe
$shellSaveRidBefore = Get-RuntimeIdString $shellSaveProbe
$generalControlProbe = Try-FindById $settings 'action.remember-layout'

foreach ($page in 'sidebar.general', 'sidebar.interface', 'sidebar.topbar', 'sidebar.input') {
    $el = Try-FindById $settings $page
    $okEl = Assert-Element "NAV-$page" $el $settings 'ListItem'
    if (-not $okEl) {
        Add-Result "NAV-$page-invoke" 'Invoke succeeded and switched page' "skipped invoke because sidebar element failed: $script:lastUiaError" $false
        continue
    }
    $pageRid = Get-RuntimeIdString $el
    $patRes = Invoke-Pattern "NAV-$page-invoke" $el ([System.Windows.Automation.InvokePattern]) -AutomationId $page
    if (-not $patRes.Success) {
        Add-Result "NAV-$page-invoke" 'pattern InvokePattern supported and invoked' $patRes.Error $false
        continue
    }
    $invRes = Try-InvokePattern -Pattern $patRes.Pattern -AutomationId $page -RuntimeId $pageRid -CurrentPage $page
    $switched = $false
    if ($invRes.Success) {
        $probeId = $script:pageProbeControl[$page]
        $sw = [System.Diagnostics.Stopwatch]::StartNew()
        while ($sw.ElapsedMilliseconds -lt 2500) {
            if ([string]::IsNullOrEmpty($probeId) -or $null -ne (Try-FindById $settings $probeId)) {
                $switched = $true
                break
            }
            Start-Sleep -Milliseconds 40
        }
    }
    $lifecycleNote = ''
    $lifecycleOk = $true
    if ($invRes.Success -and $switched) {
        $sbSnap = Try-GetElementSnapshot $shellSidebarProbe
        $svSnap = Try-GetElementSnapshot $shellSaveProbe
        $sbRidNow = Get-RuntimeIdString $shellSidebarProbe
        $svRidNow = Get-RuntimeIdString $shellSaveProbe
        if (-not $sbSnap.Success -or $sbRidNow -ne $shellSidebarRidBefore) {
            $lifecycleOk = $false
            $lifecycleNote += "; sidebar.general proxy broke after switch (ok=$($sbSnap.Success) rid=$shellSidebarRidBefore->$sbRidNow)"
        }
        if (-not $svSnap.Success -or $svRidNow -ne $shellSaveRidBefore) {
            $lifecycleOk = $false
            $lifecycleNote += "; action.save proxy broke after switch (ok=$($svSnap.Success) rid=$shellSaveRidBefore->$svRidNow)"
        }
        if ($page -eq 'sidebar.interface' -and $null -ne $generalControlProbe) {
            $oldSnap = Try-GetElementSnapshot $generalControlProbe
            if ($oldSnap.Success -or $oldSnap.Category -ne 'ElementUnavailable') {
                $lifecycleOk = $false
                $lifecycleNote += "; old general control did not become ElementUnavailable (success=$($oldSnap.Success) cat=$($oldSnap.Category))"
            } else {
                $lifecycleNote += '; oldTab=ElementUnavailable; shellRidStable=True'
            }
        }
    }
    $actualMsg = if ($invRes.Success -and $switched) {
        "invoked and page controls verified (rid=$pageRid available=$($patRes.Availability)$lifecycleNote)"
    } elseif ($invRes.Success) {
        "invoked but expected page control $($script:pageProbeControl[$page]) missing ($script:lastUiaError)"
    } else {
        $invRes.Error
    }
    Add-Result "NAV-$page-invoke" 'Invoke succeeded and switched page' $actualMsg ($invRes.Success -and $switched -and $lifecycleOk)
}

# 2) Checkboxes across all 4 pages: Assert -> Toggle -> verify UIA state changed -> restore
$checkboxPages = @(
    @{ Page = 'sidebar.general'; Ids = @('action.remember-layout', 'action.start-with-windows') },
    @{ Page = 'sidebar.interface'; Ids = @('action.show-candidate-window', 'action.next-word-prediction', 'action.candidate-fixed-height', 'action.sliding-effect', 'action.follow-caret', 'action.follow-mouse', 'action.use-os-default-font') },
    @{ Page = 'sidebar.topbar'; Ids = @('action.idle-dimming', 'action.remember-topbar-position') },
    @{ Page = 'sidebar.input'; Ids = @('action.map-numpad', 'action.ignore-caps-lock', 'action.number-keys', 'action.change-locale', 'action.disable-encoding-shortcut') }
)
foreach ($group in $checkboxPages) {
    if (-not (Select-SettingsPage $settings $group.Page)) {
        $switchErr = $script:lastUiaError
        foreach ($id in $group.Ids) {
            Add-Result "CHK-$id" 'settings page active for checkbox' ("failed to switch to $($group.Page): $switchErr") $false
        }
        continue
    }
    foreach ($id in $group.Ids) {
        $el = Try-FindById $settings $id
        if (-not (Assert-Element "CHK-$id" $el $settings 'CheckBox')) { continue }
        $patRes = Invoke-Pattern "CHK-$id" $el ([System.Windows.Automation.TogglePattern]) -AutomationId $id
        if (-not $patRes.Success) {
            Add-Result "CHK-$id-toggle" 'TogglePattern supported and toggled' $patRes.Error $false
            continue
        }

        $beforeInfo = Try-GetToggleState -El $el -AutomationId $id
        if (-not $beforeInfo.Success) {
            Add-Result "CHK-$id-toggle" 'initial ToggleState readable' $beforeInfo.Error $false
            continue
        }
        $before = $beforeInfo.State
        $togRes = Try-TogglePattern -Pattern $patRes.Pattern -AutomationId $id
        if (-not $togRes.Success) {
            Add-Result "CHK-$id-toggle" "state changed $before -> opposite" $togRes.Error $false
            continue
        }
        Start-Sleep -Milliseconds 80

        $freshAfterEl = Try-FindById $settings $id
        $afterInfo = Try-GetToggleState -El $freshAfterEl -AutomationId $id
        $after = if ($afterInfo.Success) { $afterInfo.State } else { $afterInfo.Error }
        $changed = $afterInfo.Success -and ($after -ne $before)

        # Restore original checkbox state cleanly using fresh pattern reference.
        if ($changed -and $null -ne $afterInfo.Pattern) {
            [void](Try-TogglePattern -Pattern $afterInfo.Pattern -AutomationId $id)
            Start-Sleep -Milliseconds 60
        }
        Add-Result "CHK-$id-toggle" "state changed $before -> opposite" "$before -> $after" $changed
    }
}

# 3) Dropdowns across pages: Assert -> Expand -> Enumerate -> Select option & verify -> Restore -> Collapse
$dropdownPages = @(
    @{ Page = 'sidebar.general'; Ids = @('action.start-mode', 'action.switching-mode', 'action.default-layout') },
    @{ Page = 'sidebar.interface'; Ids = @('action.orientation', 'action.theme', 'action.ui-language', 'action.font') },
    @{ Page = 'sidebar.input'; Ids = @('action.learning', 'action.layout-shortcut', 'action.encoding') }
)
$listCond = New-Object System.Windows.Automation.PropertyCondition(
    [System.Windows.Automation.AutomationElement]::ControlTypeProperty,
    [System.Windows.Automation.ControlType]::ListItem
)
foreach ($group in $dropdownPages) {
    if (-not (Select-SettingsPage $settings $group.Page)) {
        $switchErr = $script:lastUiaError
        foreach ($id in $group.Ids) {
            Add-Result "DD-$id" 'settings page active for dropdown' ("failed to switch to $($group.Page): $switchErr") $false
        }
        continue
    }
    foreach ($id in $group.Ids) {
        $el = Try-FindById $settings $id
        if (-not (Assert-Element "DD-$id" $el $settings 'ComboBox')) { continue }
        $patRes = Invoke-Pattern "DD-$id" $el ([System.Windows.Automation.ExpandCollapsePattern]) -AutomationId $id
        if (-not $patRes.Success) {
            Add-Result "DD-$id-options" '>0 selectable options enumerated' $patRes.Error $false
            Add-Result "DD-$id-select" 'option selected and dropdown value updated' $patRes.Error $false
            continue
        }
        $expand = $patRes.Pattern

        $beforeValue = Try-GetValueText $el
        $expRes = Try-ExpandCollapseOp -Pattern $expand -Action 'Expand' -AutomationId $id
        if (-not $expRes.Success) {
            Add-Result "DD-$id-options" '>0 selectable options enumerated' $expRes.Error $false
            Add-Result "DD-$id-select" 'option selected and dropdown value updated' $expRes.Error $false
            continue
        }

        $popup = Wait-WindowByClass -ClassName 'SubornoMenuPopupWindow' -ProcessId $process.Id -TimeoutMs 3000 -FallbackRoot $root
        $visibleItems = New-Object System.Collections.Generic.List[object]
        if ($null -ne $popup) {
            try {
                $items = $popup.FindAll([System.Windows.Automation.TreeScope]::Descendants, $listCond)
                foreach ($item in $items) {
                    $itemSnap = Try-GetElementSnapshot $item
                    if ($itemSnap.Success -and -not $itemSnap.IsOffscreen -and $itemSnap.IsEnabled) {
                        $togInfo = Try-GetToggleState -El $item -AutomationId $itemSnap.AutomationId
                        $visibleItems.Add([pscustomobject]@{
                            Element     = $item
                            Name        = $itemSnap.Name
                            ToggleState = if ($togInfo.Success) { $togInfo.State } else { '' }
                        }) | Out-Null
                    }
                }
            } catch {
                $script:lastUiaError = Format-UiaDiagnostic -Category (Classify-UiaException $_.Exception) -Operation 'EnumeratePopupItems' -AutomationId $id -Exception $_.Exception
            }
        }
        $optionCount = $visibleItems.Count
        Add-Result "DD-$id-options" '>0 selectable options enumerated' ("$optionCount visible items" + $(if ($null -eq $popup) { " ($script:lastUiaError)" } else { '' })) ($optionCount -gt 0)

        # Select an option via real UIA InvokePattern, verify selection change, then restore & test Collapse.
        if ($optionCount -le 0) {
            Add-Result "DD-$id-select" 'selectable dropdown item invoked and verified' 'no visible popup options to select' $false
            [void](Try-ExpandCollapseOp -Pattern $expand -Action 'Collapse' -AutomationId $id)
            continue
        }

        $candidate = $null
        $originalItemName = $beforeValue
        foreach ($opt in $visibleItems) {
            if ($opt.ToggleState -eq 'On' -and $originalItemName -eq '') {
                $originalItemName = $opt.Name
            }
            if ($null -eq $candidate -and ($opt.ToggleState -eq 'Off' -or ($beforeValue -ne '' -and $opt.Name -ne $beforeValue))) {
                $candidate = $opt
            }
        }
        if ($null -eq $candidate) {
            $candidate = if ($visibleItems.Count -gt 1) { $visibleItems[1] } else { $visibleItems[0] }
        }

        $selectOk = $false
        $selectActual = ''
        try {
            $itemPatRes = Get-UiaPattern -El $candidate.Element -PatternType ([System.Windows.Automation.InvokePattern]) -AutomationId $id
            if (-not $itemPatRes.Success) {
                $selectActual = $itemPatRes.Error
            } else {
                $selRes = Try-InvokePattern -Pattern $itemPatRes.Pattern -AutomationId $id -RuntimeId (Get-RuntimeIdString $candidate.Element)
                if (-not $selRes.Success) {
                    $selectActual = $selRes.Error
                } else {
                $closedAfterPick = Wait-WindowClosed -ClassName 'SubornoMenuPopupWindow' -ProcessId $process.Id -TimeoutMs 2000
                Start-Sleep -Milliseconds 60
                $freshDropdown = Try-FindById $settings $id
                $afterValue = Try-GetValueText $freshDropdown
                if (-not $closedAfterPick) {
                    $selectActual = 'popup remained open after selecting item'
                } elseif ($optionCount -gt 1) {
                    $selectOk = ($afterValue -ne '' -and $afterValue -ne $beforeValue -and $afterValue -eq $candidate.Name)
                    $selectActual = "value [$beforeValue] -> [$afterValue] (picked [$($candidate.Name)])"
                } else {
                    $selectOk = ($afterValue -eq $candidate.Name)
                    $selectActual = "single-option selected [$afterValue]"
                }
                }
            }
        } catch {
            $cat = Classify-UiaException $_.Exception -Operation 'SelectDropdownItem'
            $selectActual = Format-UiaDiagnostic -Category $cat -Operation 'SelectDropdownItem' -AutomationId $id -Exception $_.Exception
        }

        # Restore original selection if changed, using fresh UIA elements.
        if ($optionCount -gt 1 -and $originalItemName -ne '') {
            $freshForRestore = Try-FindById $settings $id
            $restoreExp = Try-GetExpandCollapseState -El $freshForRestore -AutomationId $id
            if ($restoreExp.Success -and $null -ne $restoreExp.Pattern) {
                $rExpRes = Try-ExpandCollapseOp -Pattern $restoreExp.Pattern -Action 'Expand' -AutomationId $id
                if ($rExpRes.Success) {
                    $restorePopup = Wait-WindowByClass -ClassName 'SubornoMenuPopupWindow' -ProcessId $process.Id -TimeoutMs 2000 -FallbackRoot $root
                    if ($null -ne $restorePopup) {
                        try {
                            $restoreItems = $restorePopup.FindAll([System.Windows.Automation.TreeScope]::Descendants, $listCond)
                            foreach ($rItem in $restoreItems) {
                                $rSnap = Try-GetElementSnapshot $rItem
                                if ($rSnap.Success -and $rSnap.Name -eq $originalItemName) {
                                    $rInv = $rItem.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern)
                                    [void](Try-InvokePattern -Pattern $rInv -AutomationId $id)
                                    [void](Wait-WindowClosed -ClassName 'SubornoMenuPopupWindow' -ProcessId $process.Id -TimeoutMs 1500)
                                    break
                                }
                            }
                        } catch {
                        }
                    }
                }
            }
        }

        # Also explicitly verify Expand -> Collapse returns S_OK and closes popup cleanly.
        $freshForCollapse = Try-FindById $settings $id
        $colStateCheck = Try-GetExpandCollapseState -El $freshForCollapse -AutomationId $id
        $collapseOk = $true
        $collapseNote = ''
        if ($colStateCheck.Success -and $null -ne $colStateCheck.Pattern) {
            $reOpenRes = Try-ExpandCollapseOp -Pattern $colStateCheck.Pattern -Action 'Expand' -AutomationId $id
            if ($reOpenRes.Success) {
                [void](Wait-WindowByClass -ClassName 'SubornoMenuPopupWindow' -ProcessId $process.Id -TimeoutMs 2000 -FallbackRoot $root)
                $colRes = Try-ExpandCollapseOp -Pattern $colStateCheck.Pattern -Action 'Collapse' -AutomationId $id
                $closedAfterCol = Wait-WindowClosed -ClassName 'SubornoMenuPopupWindow' -ProcessId $process.Id -TimeoutMs 1500
                if (-not $colRes.Success) {
                    $collapseOk = $false
                    $collapseNote = "; collapse failed: $($colRes.Error)"
                } elseif (-not $closedAfterCol) {
                    $collapseOk = $false
                    $collapseNote = '; popup did not close on Collapse()'
                } else {
                    $collapseNote = '; Collapse=OK'
                }
            } else {
                $collapseOk = $false
                $collapseNote = "; re-expand failed: $($reOpenRes.Error)"
            }
        }

        Add-Result "DD-$id-select" 'non-active option selected, verified via UIA Value, restored, and Collapse=OK' ($selectActual + $collapseNote) ($selectOk -and $collapseOk)
    }
}

# 4) Sliders: RangeValue min / max / mid aligned with checkmap.json
if (Select-SettingsPage $settings 'sidebar.topbar') {
    $sliderIds = @('action.idle-opacity', 'action.idle-time')
    foreach ($id in $sliderIds) {
        $el = Try-FindById $settings $id
        if (-not (Assert-Element "SLD-$id" $el $settings 'Slider')) { continue }
        $patRes = Invoke-Pattern "SLD-$id" $el ([System.Windows.Automation.RangeValuePattern]) -AutomationId $id
        if (-not $patRes.Success) {
            foreach ($suffix in 'min', 'max', 'mid') {
                Add-Result "SLD-$id-set-$suffix" "slider value set to $suffix" $patRes.Error $false
            }
            continue
        }

        $rangeSnap = Try-GetRangeValue -El $el -AutomationId $id
        if (-not $rangeSnap.Success) {
            foreach ($suffix in 'min', 'max', 'mid') {
                Add-Result "SLD-$id-set-$suffix" "slider value set to $suffix" $rangeSnap.Error $false
            }
            continue
        }
        $initial = $rangeSnap.Value
        $min = $rangeSnap.Minimum
        $max = $rangeSnap.Maximum
        $mid = ($min + $max) / 2.0
        $targets = @(
            @{ Suffix = 'min'; Raw = $min; Expected = [int][math]::Round($min, [System.MidpointRounding]::AwayFromZero) },
            @{ Suffix = 'max'; Raw = $max; Expected = [int][math]::Round($max, [System.MidpointRounding]::AwayFromZero) },
            @{ Suffix = 'mid'; Raw = $mid; Expected = [int][math]::Round($mid, [System.MidpointRounding]::AwayFromZero) }
        )
        foreach ($t in $targets) {
            $freshEl = Try-FindById $settings $id
            $freshRangeInfo = Try-GetRangeValue -El $freshEl -AutomationId $id
            if (-not $freshRangeInfo.Success) {
                Add-Result "SLD-$id-set-$($t.Suffix)" "value set to $($t.Expected)" $freshRangeInfo.Error $false
                continue
            }
            $setRes = Try-SetRangeValue -Pattern $freshRangeInfo.Pattern -Value $t.Raw -AutomationId $id
            if (-not $setRes.Success) {
                Add-Result "SLD-$id-set-$($t.Suffix)" "value set to $($t.Expected)" $setRes.Error $false
                continue
            }
            Start-Sleep -Milliseconds 80
            $readEl = Try-FindById $settings $id
            $readBack = Try-GetRangeValue -El $readEl -AutomationId $id
            $passed = $readBack.Success -and ([math]::Abs($readBack.Value - $t.Expected) -lt 0.01)
            $actualText = if ($readBack.Success) { "target=$($t.Raw) expectedInt=$($t.Expected) actual=$($readBack.Value)" } else { $readBack.Error }
            Add-Result "SLD-$id-set-$($t.Suffix)" "value set to $($t.Expected)" $actualText $passed
        }

        $restoreEl = Try-FindById $settings $id
        $restoreRange = Try-GetRangeValue -El $restoreEl -AutomationId $id
        if ($restoreRange.Success -and $null -ne $restoreRange.Pattern) {
            [void](Try-SetRangeValue -Pattern $restoreRange.Pattern -Value $initial -AutomationId $id)
            Start-Sleep -Milliseconds 60
        }
    }
} else {
    $switchErr = $script:lastUiaError
    foreach ($id in 'action.idle-opacity', 'action.idle-time') {
        Add-Result "SLD-$id" 'sidebar.topbar active for slider checks' ("failed to switch to sidebar.topbar: $switchErr") $false
    }
}

# 5) Save -> persistence file + reopened UIA verification; Mouse-click Save -> SendInput; Rapid Save; Cancel -> unchanged
$toastVisible = $false
$toastShot = ''
$settingsPath = Get-SettingsIniPath
$null = Select-SettingsPage $settings 'sidebar.general'
$save = Try-FindById $settings 'action.save'
$cancel = Try-FindById $settings 'action.cancel'
$okSaveEl = Assert-Element 'SET-SAVE' $save $settings 'Button'
$okCancelEl = Assert-Element 'SET-CANCEL' $cancel $settings 'Button'

if ($okSaveEl -and (Test-Path $settingsPath)) {
    $rememberEl = Try-FindById $settings 'action.remember-layout'
    $initialToggle = Try-GetToggleState -El $rememberEl -AutomationId 'action.remember-layout'
    $expectedAfterSaveIni = if ($initialToggle.Success -and $initialToggle.State -eq 'On') { 'false' } else { 'true' }
    $expectedAfterSaveToggle = if ($expectedAfterSaveIni -eq 'true') { 'On' } else { 'Off' }
    $expectedRestoredIni = if ($expectedAfterSaveIni -eq 'true') { 'false' } else { 'true' }
    $expectedRestoredToggle = if ($expectedRestoredIni -eq 'true') { 'On' } else { 'Off' }

    if ($initialToggle.Success -and $null -ne $initialToggle.Pattern) {
        [void](Try-TogglePattern -Pattern $initialToggle.Pattern -AutomationId 'action.remember-layout')
        Start-Sleep -Milliseconds 80
    }
    $hashBeforeSave = (Get-FileHash $settingsPath).Hash
    $save = Try-FindById $settings 'action.save'
    $savePatRes = Invoke-Pattern 'SET-SAVE-invoke' $save ([System.Windows.Automation.InvokePattern]) -AutomationId 'action.save'
    if ($savePatRes.Success) {
        $saveInvRes = Try-InvokePattern -Pattern $savePatRes.Pattern -AutomationId 'action.save' -RuntimeId (Get-RuntimeIdString $save)
        Add-Result 'SET-SAVE-invoke' 'InvokePattern supported and Save invoked' ($(if ($saveInvRes.Success) { "available=$($savePatRes.Availability); Invoke=S_OK" } else { $saveInvRes.Error })) $saveInvRes.Success
        if ($saveInvRes.Success) {
            # Toast window appears immediately on save success; poll quickly before auto-dismiss.
            $toastWin = Wait-WindowByClass -ClassName 'SubornoToastWindow' -ProcessId $process.Id -TimeoutMs 2500 -PollMs 40 -FallbackRoot $root
            if ($null -ne $toastWin -and $null -ne (Try-FindById $toastWin 'toast.message')) {
                $toastVisible = $true
                $toastShot = Save-WindowScreenshot 'TOAST-element' $toastWin
            }
            [void](Wait-WindowHidden -Hwnd $settingsHost -TimeoutMs 2000)
            $actualIni = Wait-SettingsIniKey -Path $settingsPath -Key 'remember_last_input_layout' -ExpectedValue $expectedAfterSaveIni -TimeoutMs 1500
            $hashAfterSave = (Get-FileHash $settingsPath).Hash

            # Re-open Settings and verify persisted state matches via fresh UIA query.
            $settings = Open-SettingsWindow -Root $root -HostHwnd $settingsHost -ProcessId $process.Id -WmCommand $wmCommand -MenuOpenSettings $menuOpenSettings
            $reopenedState = 'UNAVAILABLE'
            if ($null -ne $settings) {
                [void](Select-SettingsPage $settings 'sidebar.general')
                $reopenedEl = Try-FindById $settings 'action.remember-layout'
                $reopenedTog = Try-GetToggleState -El $reopenedEl -AutomationId 'action.remember-layout'
                if ($reopenedTog.Success) { $reopenedState = $reopenedTog.State }
            }
            $persistOk = ($hashBeforeSave -ne $hashAfterSave) -and ($actualIni -eq $expectedAfterSaveIni) -and ($reopenedState -eq $expectedAfterSaveToggle)
            Add-Result 'SET-SAVE-persist' "remember_last_input_layout=$expectedAfterSaveIni persisted on disk & reopened UIA ($expectedAfterSaveToggle)" "hashChanged=$($hashBeforeSave -ne $hashAfterSave) ini=$actualIni reopenedUIA=$reopenedState" $persistOk
        } else {
            Add-Result 'SET-SAVE-persist' 'settings file rewritten on Save' $saveInvRes.Error $false
        }
    } else {
        Add-Result 'SET-SAVE-invoke' 'InvokePattern supported and Save invoked' $savePatRes.Error $false
        Add-Result 'SET-SAVE-persist' 'settings file rewritten on Save' $savePatRes.Error $false
    }

    # Restore original value via real SendInput mouse click on Save.
    if ($null -eq $settings) {
        $settings = Open-SettingsWindow -Root $root -HostHwnd $settingsHost -ProcessId $process.Id -WmCommand $wmCommand -MenuOpenSettings $menuOpenSettings
    }
    if ($null -ne $settings) {
        [void](Select-SettingsPage $settings 'sidebar.general')
        $rememberEl = Try-FindById $settings 'action.remember-layout'
        $togBeforeMouse = Try-GetToggleState -El $rememberEl -AutomationId 'action.remember-layout'
        if ($togBeforeMouse.Success -and $null -ne $togBeforeMouse.Pattern) {
            [void](Try-TogglePattern -Pattern $togBeforeMouse.Pattern -AutomationId 'action.remember-layout')
            Start-Sleep -Milliseconds 80
        }
        $hashBeforeMouse = (Get-FileHash $settingsPath).Hash
        $mouseStatus = Invoke-MouseClickCenter -Id 'SET-SAVE-mouse' -ContainerWindow $settings -AutomationId 'action.save' -FocusHwnd $settingsHost -ProcessId $process.Id
        if ($mouseStatus -eq 'BLOCKED') {
            Add-Result 'SET-SAVE-mouseclick' 'real mouse click on Save rewrites settings file' 'BLOCKED: interactive foreground unavailable for SendInput' $false -StatusOverride 'BLOCKED'
        } elseif ($mouseStatus -eq 'OK') {
            [void](Wait-WindowHidden -Hwnd $settingsHost -TimeoutMs 2000)
            $actualMouseIni = Wait-SettingsIniKey -Path $settingsPath -Key 'remember_last_input_layout' -ExpectedValue $expectedRestoredIni -TimeoutMs 1500
            $hashAfterMouse = (Get-FileHash $settingsPath).Hash
            $reopenedAfterMouse = Open-SettingsWindow -Root $root -HostHwnd $settingsHost -ProcessId $process.Id -WmCommand $wmCommand -MenuOpenSettings $menuOpenSettings
            $mouseUiaState = 'UNAVAILABLE'
            if ($null -ne $reopenedAfterMouse) {
                [void](Select-SettingsPage $reopenedAfterMouse 'sidebar.general')
                $mEl = Try-FindById $reopenedAfterMouse 'action.remember-layout'
                $mTog = Try-GetToggleState -El $mEl -AutomationId 'action.remember-layout'
                if ($mTog.Success) { $mouseUiaState = $mTog.State }
                $settings = $reopenedAfterMouse
            }
            $mouseOk = ($hashBeforeMouse -ne $hashAfterMouse) -and ($actualMouseIni -eq $expectedRestoredIni) -and ($mouseUiaState -eq $expectedRestoredToggle)
            Add-Result 'SET-SAVE-mouseclick' "real SendInput click restores remember_last_input_layout=$expectedRestoredIni" "hashChanged=$($hashBeforeMouse -ne $hashAfterMouse) ini=$actualMouseIni reopenedUIA=$mouseUiaState" $mouseOk
        } else {
            Add-Result 'SET-SAVE-mouseclick' 'real SendInput click on Save rewrites settings file' 'mouse click delivery failed' $false
        }
    } else {
        Add-Result 'SET-SAVE-mouse-coords' 'Settings window reopened for mouse click' 'NOT OPEN' $false
        Add-Result 'SET-SAVE-mouseclick' 'real mouse click on Save rewrites settings file' 'Settings window did not reopen' $false
    }

    # Strict SET-SAVE-rapid:
    #   Call #1: real Save commits, persists settings.ini, and hides Settings.
    #   Call #2 & #3 on the ORIGINAL stale proxy (without reopening): MUST return ElementUnavailable (0x80040201) with process alive.
    #   Fresh-Proxy Reopen check (within SET-SAVE-rapid): reopen Settings, acquire a fresh action.save proxy, restore & persist cleanly.
    $settingsRapid = Open-SettingsWindow -Root $root -HostHwnd $settingsHost -ProcessId $process.Id -WmCommand $wmCommand -MenuOpenSettings $menuOpenSettings
    $rapidUnexpectedError = ''
    $rapidSummary = ''
    $hrElemNotAvail = [int][uint32]::Parse('80040201', [System.Globalization.NumberStyles]::HexNumber)
    if ($null -ne $settingsRapid) {
        [void](Select-SettingsPage $settingsRapid 'sidebar.general')
        $rapidRememberEl = Try-FindById $settingsRapid 'action.remember-layout'
        $rapidInitialTog = Try-GetToggleState -El $rapidRememberEl -AutomationId 'action.remember-layout'
        $rapidOriginalIni = if ($rapidInitialTog.Success -and $rapidInitialTog.State -eq 'On') { 'true' } else { 'false' }
        $rapidTargetIni = if ($rapidOriginalIni -eq 'true') { 'false' } else { 'true' }
        if ($rapidInitialTog.Success -and $null -ne $rapidInitialTog.Pattern) {
            [void](Try-TogglePattern -Pattern $rapidInitialTog.Pattern -AutomationId 'action.remember-layout')
            Start-Sleep -Milliseconds 80
        }

        $saveRapidEl = Try-FindById $settingsRapid 'action.save'
        $saveRapidRid = Get-RuntimeIdString $saveRapidEl
        $saveRapidPat = Get-UiaPattern -El $saveRapidEl -PatternType ([System.Windows.Automation.InvokePattern]) -AutomationId 'action.save' -CurrentPage 'sidebar.general'
        if (-not $saveRapidPat.Success) {
            $rapidUnexpectedError = "rapid Save pattern setup failed: $($saveRapidPat.Error)"
        } else {
            $rapidProxy = $saveRapidPat.Pattern
            $hashBeforeRapid1 = (Get-FileHash $settingsPath).Hash
            # Call #1: genuine Save
            $r1 = Try-InvokePattern -Pattern $rapidProxy -AutomationId 'action.save' -RuntimeId $saveRapidRid -CurrentPage 'sidebar.general'
            $hidden1 = Wait-WindowHidden -Hwnd $settingsHost -TimeoutMs 2000
            $iniAfter1 = Wait-SettingsIniKey -Path $settingsPath -Key 'remember_last_input_layout' -ExpectedValue $rapidTargetIni -TimeoutMs 1500
            $hashAfterRapid1 = (Get-FileHash $settingsPath).Hash
            $process.Refresh()
            if (-not $r1.Success -or -not $hidden1 -or $iniAfter1 -ne $rapidTargetIni -or $hashBeforeRapid1 -eq $hashAfterRapid1 -or $process.HasExited) {
                $rapidUnexpectedError = "call #1 failed (ok=$($r1.Success) hidden=$hidden1 ini=$iniAfter1 expected=$rapidTargetIni hashChanged=$($hashBeforeRapid1 -ne $hashAfterRapid1) exited=$($process.HasExited) err=$($r1.Error))"
            } else {
                # Call #2: stale original proxy -> MUST be ElementUnavailable (0x80040201)
                $r2 = Try-InvokePattern -Pattern $rapidProxy -AutomationId 'action.save' -RuntimeId $saveRapidRid -CurrentPage 'sidebar.general(stale#2)'
                $process.Refresh()
                $r2IsNotAvail = (-not $r2.Success) -and ($r2.Category -eq 'ElementUnavailable') -and ($r2.HResult -eq $hrElemNotAvail -or $r2.Error -like '*ElementNotAvailableException*')
                if (-not $r2IsNotAvail -or $process.HasExited) {
                    $rapidUnexpectedError = ('call #2 expected ElementUnavailable(0x80040201); actual=[{0}] exited={1}' -f $r2.Error, $process.HasExited)
                } else {
                    # Call #3: same stale original proxy -> MUST also be ElementUnavailable (0x80040201)
                    $r3 = Try-InvokePattern -Pattern $rapidProxy -AutomationId 'action.save' -RuntimeId $saveRapidRid -CurrentPage 'sidebar.general(stale#3)'
                    $process.Refresh()
                    $r3IsNotAvail = (-not $r3.Success) -and ($r3.Category -eq 'ElementUnavailable') -and ($r3.HResult -eq $hrElemNotAvail -or $r3.Error -like '*ElementNotAvailableException*')
                    if (-not $r3IsNotAvail -or $process.HasExited) {
                        $rapidUnexpectedError = ('call #3 expected ElementUnavailable(0x80040201); actual=[{0}] exited={1}' -f $r3.Error, $process.HasExited)
                    } else {
                        # Separate Fresh-Proxy Reopen verification within SET-SAVE-rapid
                        $reopenedFresh = Open-SettingsWindow -Root $root -HostHwnd $settingsHost -ProcessId $process.Id -WmCommand $wmCommand -MenuOpenSettings $menuOpenSettings
                        if ($null -eq $reopenedFresh) {
                            $rapidUnexpectedError = 'fresh reopen failed after stale-proxy calls'
                        } else {
                            [void](Select-SettingsPage $reopenedFresh 'sidebar.general')
                            $freshRem = Try-FindById $reopenedFresh 'action.remember-layout'
                            $freshRemTog = Try-GetToggleState -El $freshRem -AutomationId 'action.remember-layout'
                            if ($freshRemTog.Success -and $null -ne $freshRemTog.Pattern) {
                                [void](Try-TogglePattern -Pattern $freshRemTog.Pattern -AutomationId 'action.remember-layout')
                                Start-Sleep -Milliseconds 80
                            }
                            $freshSaveEl = Try-FindById $reopenedFresh 'action.save'
                            $freshSavePat = Get-UiaPattern -El $freshSaveEl -PatternType ([System.Windows.Automation.InvokePattern]) -AutomationId 'action.save' -CurrentPage 'sidebar.general'
                            if (-not $freshSavePat.Success) {
                                $rapidUnexpectedError = "fresh reopen action.save pattern failed: $($freshSavePat.Error)"
                            } else {
                                $rFresh = Try-InvokePattern -Pattern $freshSavePat.Pattern -AutomationId 'action.save' -RuntimeId (Get-RuntimeIdString $freshSaveEl) -CurrentPage 'sidebar.general(fresh-reopen)'
                                $hiddenFresh = Wait-WindowHidden -Hwnd $settingsHost -TimeoutMs 2000
                                $iniFresh = Wait-SettingsIniKey -Path $settingsPath -Key 'remember_last_input_layout' -ExpectedValue $rapidOriginalIni -TimeoutMs 1500
                                if (-not $rFresh.Success -or -not $hiddenFresh -or $iniFresh -ne $rapidOriginalIni) {
                                    $rapidUnexpectedError = "fresh reopen Save failed (ok=$($rFresh.Success) hidden=$hiddenFresh ini=$iniFresh expected=$rapidOriginalIni err=$($rFresh.Error))"
                                } else {
                                    $rapidSummary = ('call1=S_OK(ini={0},hidden=True); call2={1}(0x{2:X8}); call3={3}(0x{4:X8}); freshReopen=S_OK(ini={5},hidden=True)' -f
                                        $iniAfter1, $r2.Category, $r2.HResult, $r3.Category, $r3.HResult, $iniFresh)
                                }
                            }
                        }
                    }
                }
            }
        }
        [void](Wait-WindowHidden -Hwnd $settingsHost -TimeoutMs 1500)
    } else {
        $rapidUnexpectedError = 'Settings window did not reopen for rapid Save check'
    }
    $process.Refresh()
    $alive = -not $process.HasExited
    Add-Result 'SET-SAVE-rapid' 'call #1 saves & hides; stale calls #2/#3 return ElementUnavailable(0x80040201); fresh-reopen Save succeeds; process alive' ("alive=$alive; $rapidSummary" + $(if ($rapidUnexpectedError -ne '') { "; $rapidUnexpectedError" } else { '' })) ($alive -and $rapidUnexpectedError -eq '')
} elseif ($okSaveEl) {
    Add-Result 'SET-SAVE-persist' 'settings.ini exists before Save persistence check' "NOT FOUND: $settingsPath" $false
}

# Cancel -> does not persist staged edits; discards staged UI state
$settingsForCancel = Open-SettingsWindow -Root $root -HostHwnd $settingsHost -ProcessId $process.Id -WmCommand $wmCommand -MenuOpenSettings $menuOpenSettings
if ($null -ne $settingsForCancel) {
    [void](Select-SettingsPage $settingsForCancel 'sidebar.general')
    $rememberBeforeCancelEl = Try-FindById $settingsForCancel 'action.remember-layout'
    $togBeforeCancel = Try-GetToggleState -El $rememberBeforeCancelEl -AutomationId 'action.remember-layout'
    $initialCancelState = if ($togBeforeCancel.Success) { $togBeforeCancel.State } else { '' }
    if ($togBeforeCancel.Success -and $null -ne $togBeforeCancel.Pattern) {
        [void](Try-TogglePattern -Pattern $togBeforeCancel.Pattern -AutomationId 'action.remember-layout')
        Start-Sleep -Milliseconds 80
    }
    $cancelEl = Try-FindById $settingsForCancel 'action.cancel'
    $cancelPatRes = Invoke-Pattern 'SET-CANCEL-invoke' $cancelEl ([System.Windows.Automation.InvokePattern]) -AutomationId 'action.cancel'
    if ($cancelPatRes.Success) {
        $hashBeforeCancel = if (Test-Path $settingsPath) { (Get-FileHash $settingsPath).Hash } else { '' }
        $iniBeforeCancel = Get-IniKeyValue -Path $settingsPath -Key 'remember_last_input_layout'
        $cancelRes = Try-InvokePattern -Pattern $cancelPatRes.Pattern -AutomationId 'action.cancel' -RuntimeId (Get-RuntimeIdString $cancelEl)
        Add-Result 'SET-CANCEL-invoke' 'InvokePattern supported and Cancel invoked' ($(if ($cancelRes.Success) { "available=$($cancelPatRes.Availability); Invoke=S_OK" } else { $cancelRes.Error })) $cancelRes.Success
        [void](Wait-WindowHidden -Hwnd $settingsHost -TimeoutMs 2000)
        $hashAfterCancel = if (Test-Path $settingsPath) { (Get-FileHash $settingsPath).Hash } else { '' }
        $iniAfterCancel = Get-IniKeyValue -Path $settingsPath -Key 'remember_last_input_layout'

        # Re-open Settings briefly to confirm staged modification was discarded, then dismiss cleanly.
        $verifyDiscardWin = Open-SettingsWindow -Root $root -HostHwnd $settingsHost -ProcessId $process.Id -WmCommand $wmCommand -MenuOpenSettings $menuOpenSettings
        $discardedState = ''
        if ($null -ne $verifyDiscardWin) {
            [void](Select-SettingsPage $verifyDiscardWin 'sidebar.general')
            $verifyEl = Try-FindById $verifyDiscardWin 'action.remember-layout'
            $verifyTog = Try-GetToggleState -El $verifyEl -AutomationId 'action.remember-layout'
            if ($verifyTog.Success) { $discardedState = $verifyTog.State }
            $dismissCancel = Try-FindById $verifyDiscardWin 'action.cancel'
            if ($null -ne $dismissCancel) {
                try {
                    $dismissInv = $dismissCancel.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern)
                    [void](Try-InvokePattern -Pattern $dismissInv -AutomationId 'action.cancel')
                    [void](Wait-WindowHidden -Hwnd $settingsHost -TimeoutMs 1500)
                } catch {
                }
            }
        }

        $cancelOk = $cancelRes.Success -and ($hashBeforeCancel -eq $hashAfterCancel) -and ($iniBeforeCancel -eq $iniAfterCancel) -and ($initialCancelState -eq '' -or $discardedState -eq $initialCancelState)
        Add-Result 'SET-CANCEL-nopersist' 'Cancel leaves settings.ini unchanged and discards staged edit' "fileUnchanged=$($hashBeforeCancel -eq $hashAfterCancel) ini=$iniBeforeCancel->$iniAfterCancel state=$initialCancelState->$discardedState" $cancelOk
    } else {
        Add-Result 'SET-CANCEL-invoke' 'InvokePattern supported and Cancel invoked' $cancelPatRes.Error $false
        Add-Result 'SET-CANCEL-nopersist' 'Cancel leaves settings.ini unchanged and discards staged edit' $cancelPatRes.Error $false
    }
} else {
    Add-Result 'SET-CANCEL-nopersist' 'Settings window reopened for Cancel check' 'NOT OPEN' $false
}

# 6) Toast element appears after Save
if (-not $toastVisible) {
    $toastWin = Wait-WindowByClass -ClassName 'SubornoToastWindow' -ProcessId $process.Id -TimeoutMs 800 -PollMs 40 -FallbackRoot $root
    if ($null -ne $toastWin -and $null -ne (Try-FindById $toastWin 'toast.message')) {
        $toastVisible = $true
        $toastShot = Save-WindowScreenshot 'TOAST-element' $toastWin
    }
}
Add-Result 'TOAST-element' 'toast.message element visible after Save' ($(if ($toastVisible) { 'visible' } else { "not visible ($script:lastUiaError)" })) $toastVisible $toastShot

# 7) About dialog (process-scoped visible HWND + Root fallback)
$about = Wait-WindowByClass -ClassName 'SubornoAboutWindow' -ProcessId $process.Id -TimeoutMs 300 -FallbackRoot $root
if ($null -eq $about) {
    [void][Suborno.Audit.Native]::SendMessage($settingsHost, $wmCommand, $menuAbout, [IntPtr]::Zero)
    $about = Wait-WindowByClass -ClassName 'SubornoAboutWindow' -ProcessId $process.Id -TimeoutMs 5000 -PollMs 50 -FallbackRoot $root
}
if ($null -ne $about) {
    $shotAbout = Save-WindowScreenshot 'ABT-open' $about
    $btnCond = New-Object System.Windows.Automation.PropertyCondition(
        [System.Windows.Automation.AutomationElement]::ControlTypeProperty,
        [System.Windows.Automation.ControlType]::Button
    )
    $btnCount = 0
    try {
        $btns = $about.FindAll([System.Windows.Automation.TreeScope]::Descendants, $btnCond)
        $btnCount = $btns.Count
    } catch {
        $script:lastUiaError = Format-UiaDiagnostic -Category (Classify-UiaException $_.Exception) -Operation 'AboutButtons' -Exception $_.Exception
    }
    Add-Result 'ABT-buttons' '>=5 buttons exposed' ("$btnCount buttons" + $(if ($btnCount -lt 5 -and $script:lastUiaError -ne '') { " ($script:lastUiaError)" } else { '' })) ($btnCount -ge 5) $shotAbout
    $aboutSnap = Try-GetElementSnapshot $about
    if ($aboutSnap.Success -and $aboutSnap.NativeWindowHandle -ne [IntPtr]::Zero) {
        [void][Suborno.Audit.Native]::SendMessage($aboutSnap.NativeWindowHandle, $wmClose, [IntPtr]::Zero, [IntPtr]::Zero)
        [void](Wait-WindowClosed -ClassName 'SubornoAboutWindow' -ProcessId $process.Id -TimeoutMs 1500)
    }
} else {
    Add-Result 'ABT-open' 'About dialog opened via WM_COMMAND menu_about' "NOT OPEN ($script:lastUiaError)" $false
}

# 8) OSK elements (process-scoped visible HWND + Root fallback)
$osk = Wait-WindowByClass -ClassName 'SubornoOskWindow' -ProcessId $process.Id -TimeoutMs 300 -FallbackRoot $root
if ($null -eq $osk) {
    [void][Suborno.Audit.Native]::SendMessage($settingsHost, $wmCommand, $menuOsk, [IntPtr]::Zero)
    $osk = Wait-WindowByClass -ClassName 'SubornoOskWindow' -ProcessId $process.Id -TimeoutMs 5000 -PollMs 50 -FallbackRoot $root
}
if ($null -ne $osk) {
    $shotOsk = Save-WindowScreenshot 'OSK-open' $osk
    $btnCond = New-Object System.Windows.Automation.PropertyCondition(
        [System.Windows.Automation.AutomationElement]::ControlTypeProperty,
        [System.Windows.Automation.ControlType]::Button
    )
    $keyCount = 0
    try {
        $keys = $osk.FindAll([System.Windows.Automation.TreeScope]::Descendants, $btnCond)
        $keyCount = $keys.Count
    } catch {
        $script:lastUiaError = Format-UiaDiagnostic -Category (Classify-UiaException $_.Exception) -Operation 'OskKeys' -Exception $_.Exception
    }
    Add-Result 'OSK-elements' '36 invokable elements' ("$keyCount buttons" + $(if ($keyCount -lt 36 -and $script:lastUiaError -ne '') { " ($script:lastUiaError)" } else { '' })) ($keyCount -ge 36) $shotOsk
} else {
    Add-Result 'OSK-open' 'OSK window opened via WM_COMMAND menu_osk' "NOT OPEN ($script:lastUiaError)" $false
}

# 9) Topbar zones (process-scoped visible HWND + Root fallback)
$topbar = Wait-WindowByClass -ClassName 'SubornoTopbarWindow' -ProcessId $process.Id -TimeoutMs 300 -FallbackRoot $root
if ($null -eq $topbar) {
    [void][Suborno.Audit.Native]::SendMessage($settingsHost, $wmSecondInstanceShowTopbar, [IntPtr]::Zero, [IntPtr]::Zero)
    $topbar = Wait-WindowByClass -ClassName 'SubornoTopbarWindow' -ProcessId $process.Id -TimeoutMs 5000 -PollMs 50 -FallbackRoot $root
}
if ($null -ne $topbar) {
    $shotTopbar = Save-WindowScreenshot 'TOP-open' $topbar
    foreach ($zone in 'zone.mode', 'zone.layout', 'zone.wrench', 'zone.close') {
        $el = Try-FindById $topbar $zone
        [void](Assert-Element "TOP-$zone" $el $topbar 'Button')
    }
} else {
    Add-Result 'TOP-open' 'Topbar window opened via second_instance_show_topbar' "NOT OPEN ($script:lastUiaError)" $false
}

$envelope = [pscustomobject]@{
    environment = $script:environment
    screenshots = $script:screenshots
    results     = $script:results
}
$envelope | ConvertTo-Json -Depth 4 | Set-Content -Path $ResultsJson -Encoding UTF8
$failed = @($script:results | Where-Object { $_.status -eq 'FAIL' }).Count
$blocked = @($script:results | Where-Object { $_.status -eq 'BLOCKED' }).Count
$passed = @($script:results | Where-Object { $_.status -eq 'PASS' }).Count
Write-Host ('== totals: {0} PASS, {1} FAIL, {2} BLOCKED; log: {3}; screenshots: {4} ==' -f $passed, $failed, $blocked, $ResultsJson, $script:screenshots.Count) -ForegroundColor Cyan
Write-Host ('next: python3 scripts/layout_verify/merge_audit_results.py --results {0} --map scripts/windows-ui-audit/functional/checkmap.json --label "Windows run"' -f $ResultsJson)
exit ($failed + $blocked)
