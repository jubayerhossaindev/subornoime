# BDS Windows Runtime Verification Report

**Mission**: COMPLETE BDS COMPLIANCE… — **PHASE 2 UPDATED** (2026-11-24)

**PHASE 2 re-check (2026-11-24)**: still no Windows environment in this
sandbox — W-01..W-10 remain **BLOCKED — WINDOWS RUNTIME UNAVAILABLE**. The
checklist below stays the executable campaign procedure for a Windows 10 x64
machine; nothing was simulated. Engine-level numbers refreshed (591/591 unit
+ 2/2 integration + ASan/UBSan clean + 94-row official corpus tests).
**Environment reality**: this verification ran in a Linux sandbox (Debian 13).
There is **no Windows 10 x64 desktop, no MSVC v143 toolchain, no TSF runtime**
available. Per the mission's own rules, Linux engine tests are NEVER
presented as Windows verification, and no "Windows Verified" claim exists
anywhere in this repository.

## 1. What WAS executed (engine-level, Linux)

| Verification | Command | Result |
|---|---|---|
| Full unit suite | `build/ubuntu-dev/suborno_unit_tests` | **591/591 PASS** (586 + 5 `Bds1520OfficialCorpusTest.*`) |
| Integration suite | ctest `suborno.integration` | 2/2 PASS |
| Sanitizer suite (ASan + UBSan, leak check) | `build/bds-asan/suborno_unit_tests` + integration | **591+2 PASS, zero sanitizer diagnostics** |
| Engine typing harness (real InputSession, scan-code driven) | `khipro_probhat_engine_verify` | 579 rows, 0 FAIL, 24.9 ms, max RSS ≈ 12.6 MiB |
| Repertoire sweep + NormalizationTest conformance | `bds_repertoire_verify` (+ ASan build re-run) | 128/128, 7/7, 12/12 PASS |
| Performance probes | harness timing + suite timing | see BDS_REGRESSION_REPORT.md §4 |

## 2. Windows runtime checklist — BLOCKED rows

| # | Windows test | Required environment | Status |
|---|---|---|---|
| W-01 | TSF registration (regsvr32 / profile install) | Windows 10/11 x64 + MSVC build | **BLOCKED — WINDOWS RUNTIME UNAVAILABLE** |
| W-02 | Input profile activation | Same | **BLOCKED** |
| W-03 | Physical keyboard typing (main keys) | Real Windows desktop + keyboard | **BLOCKED** |
| W-04 | National keyboard layout runtime check | Same | **BLOCKED** |
| W-05 | Seven-layout switching | Same | **BLOCKED** |
| W-06 | Khipro trailing virama on Windows (`kqq` → ক্) | Same | **BLOCKED** |
| W-07 | Probhat standalone nukta on Windows (AltGr+0x34 → ়) | Same | **BLOCKED** |
| W-08 | Notepad commit/composition/cancel | Same | **BLOCKED** |
| W-09 | Microsoft Word / browser fields | Same | **BLOCKED** |
| W-10 | Copy/paste, save/open, focus changes, restart, installer/uninstaller | Same | **BLOCKED** |

Build-pipeline requirement (MSVC v143 / VS 17 2022 / Windows SDK / CMake /
MSBuild; no clang-cl/MinGW/MSYS2) is documented in
`docs/development/windows.md` and was **not** executed here — the sandbox
builds use GCC/Clang for engine verification only.

## 3. Why engine evidence still matters

The engine (layouts, sessions, Unicode module, commit pipeline) is shared
code compiled for Windows by MSVC. Every engine-level guarantee verified here
(scalars, sequences, normalization, no-loss) is the same code path the TSF
layer commits on Windows. What Linux cannot prove: TSF plumbing, physical
keyboard events, OS integration, installer behaviour — hence W-01..W-10 stay
BLOCKED and the final verdict explicitly carries a Windows-runtime
qualification.

## 4. Re-verification procedure for the Windows owner

1. Configure with the MSVC preset (VS 17 2022, v143, Windows SDK).
2. Build and run `suborno.unit`, `suborno.integration`, `suborno.windows.*`
   ctest targets.
3. Run `scripts/windows-ui-audit/Invoke-SubornoUiAudit.ps1` and the manual
   protocol for physical typing checks (W-03..W-10).
4. Record results into this report's table (replace BLOCKED with actuals —
   never without a real run).

**Verdict: WINDOWS RUNTIME BLOCKED. Engine-level verification: COMPLETE.**
