# Requires -Version 5.1
# Focused regression tests for HRESULT normalization and UIA exception classification.
$ErrorActionPreference = 'Stop'
if ($PSVersionTable.PSVersion.Major -ne 5 -or $PSVersionTable.PSVersion.Minor -ne 1) {
    throw "This regression suite must run under Windows PowerShell 5.1; actual=$($PSVersionTable.PSVersion)."
}
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
$windowsAuditDirectory = Split-Path -Path $PSScriptRoot -Parent
$helperPath = Join-Path $windowsAuditDirectory 'UiaExceptionHelpers.ps1'
$auditPath = Join-Path $windowsAuditDirectory 'Invoke-SubornoUiAudit.ps1'
if (-not (Test-Path -LiteralPath $helperPath -PathType Leaf)) {
    throw "Required UIA exception helper not found: $helperPath"
}
if (-not (Test-Path -LiteralPath $auditPath -PathType Leaf)) {
    throw "Required UIA audit script not found: $auditPath"
}
. $helperPath

function Assert-Equal {
    param([string] $Name, [object] $Actual, [object] $Expected)
    if ($Actual -ne $Expected) { throw "$Name failed: expected=[$Expected], actual=[$Actual]" }
    Write-Host "PASS $Name expected=[$Expected] actual=[$Actual]"
}
function Assert-Null {
    param([string] $Name, [object] $Actual)
    if ($null -ne $Actual) { throw "$Name failed: expected=[null], actual=[$Actual]" }
    Write-Host "PASS $Name expected=[null] actual=[null]"
}

$expectedElementUnavailable = [long]2147746305
$equivalentInputs = @(
    [pscustomobject]@{ Name = 'unsigned decimal'; Value = '2147746305' },
    [pscustomobject]@{ Name = 'signed Int32 decimal'; Value = '-2147220991' },
    [pscustomobject]@{ Name = 'hexadecimal string'; Value = '0x80040201' }
)
foreach ($case in $equivalentInputs) {
    $normalized = ConvertTo-UiaHResult -Value $case.Value
    Assert-Equal -Name "normalize $($case.Name)" -Actual $normalized -Expected $expectedElementUnavailable
    Assert-Equal -Name "format $($case.Name)" -Actual (Format-UiaHResult -Value $case.Value) -Expected '0x80040201'
}
Assert-Equal -Name 'normalize unsigned UIA_E_NOTSUPPORTED' -Actual (ConvertTo-UiaHResult -Value '2147746306') -Expected ([long]2147746306)
Assert-Equal -Name 'normalize normal positive HRESULT' -Actual (ConvertTo-UiaHResult -Value '5') -Expected ([long]5)
Assert-Equal -Name 'format normal positive HRESULT' -Actual (Format-UiaHResult -Value '5') -Expected '0x00000005'
Assert-Null -Name 'missing HRESULT' -Actual (ConvertTo-UiaHResult -Value $null)
Assert-Null -Name 'malformed HRESULT' -Actual (ConvertTo-UiaHResult -Value 'not-an-hresult')
Assert-Null -Name 'out-of-range HRESULT' -Actual (ConvertTo-UiaHResult -Value '4294967296')
Assert-Equal -Name 'format missing HRESULT' -Actual (Format-UiaHResult -Value $null) -Expected 'unknown'

$elementUnavailable = [System.Runtime.InteropServices.COMException]::new('stale UIA element', [int]-2147220991)
Assert-Equal -Name 'classify ElementUnavailable' -Actual (Classify-UiaException -Exception $elementUnavailable -Operation 'GetCurrentPattern') -Expected 'ElementUnavailable'
$patternUnsupported = [System.Runtime.InteropServices.COMException]::new('pattern unsupported', [int]-2147220990)
Assert-Equal -Name 'classify PatternUnsupported' -Actual (Classify-UiaException -Exception $patternUnsupported -Operation 'GetCurrentPattern') -Expected 'PatternUnsupported'
$timeoutException = [System.TimeoutException]::new('UIA operation timed out')
Assert-Equal -Name 'classify Timeout' -Actual (Classify-UiaException -Exception $timeoutException -Operation 'GetCurrentPattern') -Expected 'Timeout'
$otherComFailure = [System.Runtime.InteropServices.COMException]::new('unclassified COM failure', [int]-2147467259)
Assert-Equal -Name 'classify other COM failure' -Actual (Classify-UiaException -Exception $otherComFailure -Operation 'InvokePattern.Invoke') -Expected 'ComFailure'
$unexpectedException = [System.Exception]::new('unexpected failure')
Assert-Equal -Name 'classify unexpected failure' -Actual (Classify-UiaException -Exception $unexpectedException) -Expected 'UnexpectedError'
$conversionException = [System.InvalidCastException]::new('PowerShell conversion failure', $elementUnavailable)
Assert-Equal -Name 'conversion failure cannot become ElementUnavailable' -Actual (Classify-UiaException -Exception $conversionException -Operation 'GetCurrentPattern') -Expected 'UnexpectedError'

$tokens = $null
$parseErrors = $null
[void][System.Management.Automation.Language.Parser]::ParseFile($helperPath, [ref] $tokens, [ref] $parseErrors)
Assert-Equal -Name 'HRESULT helper PowerShell syntax' -Actual $parseErrors.Count -Expected 0
$tokens = $null
$parseErrors = $null
[void][System.Management.Automation.Language.Parser]::ParseFile($auditPath, [ref] $tokens, [ref] $parseErrors)
Assert-Equal -Name 'complete UIA audit PowerShell syntax' -Actual $parseErrors.Count -Expected 0
Write-Host 'All HRESULT regression tests passed under Windows PowerShell 5.1.'
