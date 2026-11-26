# Requires -Version 5.1
# Shared HRESULT and UIA exception helpers for the Windows UI Automation audit.

function ConvertTo-UiaHResult {
    [CmdletBinding()]
    param(
        [Parameter(Position = 0)]
        [AllowNull()]
        [object] $Value
    )

    if ($null -eq $Value) {
        return $null
    }

    $text = [Convert]::ToString($Value, [System.Globalization.CultureInfo]::InvariantCulture)
    if ([string]::IsNullOrWhiteSpace($text)) {
        return $null
    }
    $text = $text.Trim()

    if ($text -match '^0[xX]([0-9A-Fa-f]{1,8})$') {
        [uint64] $unsignedValue = 0
        $parsedHex = [uint64]::TryParse(
            $Matches[1],
            [System.Globalization.NumberStyles]::AllowHexSpecifier,
            [System.Globalization.CultureInfo]::InvariantCulture,
            [ref] $unsignedValue
        )
        if (-not $parsedHex -or $unsignedValue -gt 4294967295L) {
            return $null
        }
        return [long] $unsignedValue
    }

    [long] $decimalValue = 0
    $parsedDecimal = [long]::TryParse(
        $text,
        [System.Globalization.NumberStyles]::Integer,
        [System.Globalization.CultureInfo]::InvariantCulture,
        [ref] $decimalValue
    )
    if (-not $parsedDecimal -or $decimalValue -lt -2147483648L -or $decimalValue -gt 4294967295L) {
        return $null
    }

    # Normalize signed Int32 and unsigned decimal forms to a common Int64 value.
    return [long] ($decimalValue -band 4294967295L)
}

function Format-UiaHResult {
    [CmdletBinding()]
    param(
        [Parameter(Position = 0)]
        [AllowNull()]
        [object] $Value
    )

    $normalized = ConvertTo-UiaHResult -Value $Value
    if ($null -eq $normalized) {
        return 'unknown'
    }
    return ('0x{0:X8}' -f $normalized)
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

    $hrElemNotAvail = ConvertTo-UiaHResult -Value '0x80040201'
    $hrPatternNotSupported = ConvertTo-UiaHResult -Value '0x80040202'
    $hrInvalidOp = ConvertTo-UiaHResult -Value '0x80131509'

    # Never accept a conversion failure as evidence of a stale UIA element.
    foreach ($ex in $unwrapped.Chain) {
        if ($ex -is [System.Management.Automation.PSInvalidCastException] -or
            $ex -is [System.InvalidCastException] -or
            $ex -is [System.OverflowException] -or
            $ex -is [System.FormatException]) {
            return 'UnexpectedError'
        }
    }

    foreach ($ex in $unwrapped.Chain) {
        if ($ex -is [System.Windows.Automation.ElementNotAvailableException]) {
            return 'ElementUnavailable'
        }
        if ($ex -is [System.Runtime.InteropServices.COMException] -and
            (ConvertTo-UiaHResult -Value $ex.HResult) -eq $hrElemNotAvail) {
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
        $normalizedHResult = ConvertTo-UiaHResult -Value $ex.HResult
        if ($ex -is [System.Runtime.InteropServices.COMException] -and
            $normalizedHResult -eq $hrPatternNotSupported) {
            return 'PatternUnsupported'
        }
        if ($isPatternAcquireOp -and $ex -is [System.InvalidOperationException] -and
            ($normalizedHResult -eq $hrInvalidOp -or $ex.Message -like '*Pattern*' -or $ex.Message -like '*Unsupported*')) {
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
