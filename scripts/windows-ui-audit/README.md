# Windows UI Audit Harness (SubornoIME)

Converts the BLOCKED interactive checks from `docs/reports/UI_FUNCTIONAL_AUDIT_*`
into executed PASS/FAIL evidence on a real Windows 10/11 machine, then merges
that evidence back into the traceability matrix.

## Contents

| File | Purpose |
|---|---|
| `Invoke-SubornoUiAudit.ps1` | Automated UIA pass: environment detection, element discovery, patterns, all 10 dropdowns (options **enumerated**, not selected), both sliders via RangeValue, UIA Invoke **and** real mouse click on Save (separate checks), Save/Cancel persistence assertions, About/OSK button-count checks, topbar-zone visibility, toast check, per-window screenshots. Emits an envelope JSON (environment + screenshots + results) + console log. |
| `Manual-Test-Protocol.md` | The checks automation cannot do: Narrator sessions, High Contrast live toggles, 100–200% DPI, mixed-DPI multi-monitor drags, tray keyboard walks, drag persistence, screenshots. |
| `functional/checkmap.json` | Every harness check-id (84 happy-path + 14 failure-path) mapped onto the traceability-matrix test-ids consumed by the merge below. |
| `../../layout_verify/merge_audit_results.py` | Matrix evidence update (brief Section 15): merges this script's results JSON into `docs/reports/UI_FUNCTIONAL_AUDIT_TRACEABILITY_MATRIX.csv` using `windows-ui-audit/functional/checkmap.json` (orchestrator check-id → matrix test-id). Refuses unevidenced PASS upgrades and never touches NOT APPLICABLE rows. |

## Requirements

- Windows 10 1703+ (Per-Monitor v2) or Windows 11.
- The built app: the `windows-msvc-release` preset output (`Suborno.exe`) — first run
  the MSVC pipeline (`cmake --preset windows-msvc && cmake --build --preset
  windows-msvc-release -j1`) and `ctest --preset windows-msvc-release`; the build
  itself is acceptance item 1 (it proves the BUG-01 fix compiles).
- Windows PowerShell 5.1 (built into Windows) — uses only `System.Windows.Automation`,
  `System.Drawing` and .NET Framework types; **no FlaUI, no WinAppDriver, no extra
  installs** (no new dependency introduced). The script is 5.1-syntax-clean (no
  PS7-only operators).
- Real mouse clicks and OSK key injection need an **unlocked interactive desktop**
  with the target window foreground — the same constraint Microsoft documents for
  WinApp CLI's input-injecting verbs.

## Run

```powershell
powershell -ExecutionPolicy Bypass -File scripts\windows-ui-audit\Invoke-SubornoUiAudit.ps1 `
    -ExePath <path-to>\Suborno.exe [-ResultsJson results.json] [-EvidenceDir <dir>]
```

The script:

1. detects the environment first (OS/build, PowerShell version, elevation, target
   exe version, WinApp CLI presence via `winapp -V`), then launches (or attaches to)
   the app and reveals Settings via the topbar wrench menu or the tray menu,
2. walks every Settings element by AutomationId (`sidebar.*`, `action.*`,
   `action.save/cancel/close`) and asserts Name, ControlType, IsEnabled and
   BoundingRectangle are non-empty,
3. toggles all 16 checkboxes twice (both states), opens all 10 dropdowns via ExpandCollapse and **counts the visible options** (>0 asserted; per-option selection and value-change assertions are manual-protocol work), drives both sliders to min/mid/max via RangeValue SetValue (no keyboard step in the script),
4. exercises Save via UIA Invoke **and** via a real mouse click at the element center
   (each asserting the persisted settings file was rewritten), Cancel (file
   unchanged), plus 3 rapid Save invokes with a process-liveness assertion (Esc
   semantics live in the manual protocol),
5. counts the About dialog's exposed buttons (>= 5 asserted; no Esc check in the
   script) and the OSK's invokable elements (>= 37 asserted; no Notepad typing in
   the script), asserts the four topbar zones are visible/enabled (no Invoke), and
   looks for the toast element (`toast.message`) after a live toggle,
6. saves PNG screenshots of the Settings/About/OSK/Topbar windows next to the results,
7. writes one result line per check (`check_id`, expected, actual, PASS/FAIL,
   screenshot) into an envelope JSON (environment + screenshots + results) and prints
   the totals.

Exit code: number of failed checks (0 = all pass).

## Merge evidence into the matrix

```bash
python3 scripts/layout_verify/merge_audit_results.py \
    --results <ResultsJson> \
    --map scripts/windows-ui-audit/functional/checkmap.json \
    --label "Windows run" \
    --counts windows-ui-audit/functional/matrix_counts_after_windows_run.json
```

The merge appends `[Windows run <date>] <PASS/FAIL>: <actual>` to each affected
row's evidence column, keeps the previous status in the note for auditability, and
reports any results that matched no matrix row — evidence is never silently dropped.
Rows stay `NOT APPLICABLE` forever and a `PASS` without a `verification_level`
(`runtime-executed` / `engine-executed`) is rejected by design.
