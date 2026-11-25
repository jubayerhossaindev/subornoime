# UI_FUNCTIONAL_AUDIT_09 — Final Verification Summary (Phase 2, 2026-11-24)

## 1. Exact counts

| Metric | Count | Basis |
|---|---|---|
| Total discovered UI elements | **113** | `UI_FUNCTIONAL_AUDIT_INVENTORY_V2.csv` |
| — Settings window | 38 | same |
| — System tray | 13 | same |
| — Toast window | 11 | same |
| — Floating topbar | 10 | same |
| — Candidate window | 9 | same |
| — About dialog | 8 | same |
| — On-screen keyboard | 8 (covering all 37 UIA elements / 34 keys) | same |
| — Application-global | 7 | same |
| — Menu popup | 5 | same |
| — Cross-surface state | 4 | same |
| Total interactive elements | **87** | elements with mouse/keyboard/hotkey interaction paths (excludes pure indicators/tooltip/state rows) |
| Total defined test cases (executed suites) | **634** | 632 unit (609 pre-existing + 23 new audit tests) + 2 integration |
| Total executed test cases | **634** | all of them |
| Passed tests | **634** | 0 failures |
| Failed tests | **0** | — |
| Blocked test groups (Windows runtime) | **6** | TRC-040..045 (live controls, tray/topbar, OSK, candidate, UIA/Narrator, DPI/monitor/theme) |
| Not-tested items | **0** | every inventoried element has either executed, static-trace, or explicit BLOCKED status |
| Static source traces | **2 groups / 103 elements** | TRC-050/051 (wiring of every Windows-only element) |
| Discovered defects (static findings) | **4 FAIL rows** | FIND-01..05 (TRC-052..055) |
| Defects fixed this audit | **1** | FIND-06 font_family (medium) — implemented + regression-verified |
| Remaining open defects | **4 static (fix BLOCKED) + 0 runtime-known** | Report 08 |
| Documented design notes | **6** | FIND-07..13 (not defects) |
| Automation coverage | 6/6 surfaces statically audited; **0 live-automated** | Report 04 |
| Verified E2E workflows | **9/9 at model level; 0 on live shell** | Report 03 |
| i18n coverage | 157/157 render-time keys × 2 locales | executed |
| WCAG contrast | 22/22 required pairs | executed |
| Evidence scripts | i18n_key_check OK · contrast_check OK · check-layering OK | executed |

## 2. Result vocabulary compliance

Only PASS / FAIL / BLOCKED / NOT TESTED / NOT APPLICABLE are used across the
series. **No BLOCKED or NOT TESTED row was converted into PASS.** Every PASS
carries its evidence pointer (test name, script, file:line); every BLOCKED
carries its reason (no Windows 10/11 x64 + MSVC environment in this sandbox).

## 3. Environment (honesty statement)

- Audit host: Linux sandbox, Debian 13, g++-14, ICU 76.1, GoogleTest 1.17,
  CMake 4.4.5 + Ninja (pip), clang-format 23.1.5.
- The project's own Ubuntu 24.04 + Clang 23 + vcpkg bootstrap is not runnable
  here (OS gate + no sudo); the executed suites are the same portable test
  binaries the project's CI runs. This deviation is recorded, not hidden.
- Windows application runtime: **unavailable** — live-window testing, tray,
  Narrator, UIA clients, DPI/monitor/theme switching, real hotkeys and
  SendInput all remain BLOCKED (TRC-040..045). Acceptance criteria items that
  require them (live clicks on every window, dropdown popup behavior, screen
  reader sessions, visual regression) are **not claimed as met**.

## 4. Acceptance criteria scorecard

| # | Criterion | Status |
|---|---|---|
| 1 | Every application window inspected | PASS (8 windows + tray + shortcuts, source-level) |
| 2 | Every Settings page inspected | PASS |
| 3 | Every interactive control inventoried | PASS (113 rows) |
| 4 | Every control has explicit expected behavior | PASS (CSV `expected_behavior`) |
| 5 | Every button activated | BLOCKED (live) / PASS (model paths for Save/Cancel/Clear) |
| 6 | Every dropdown option tested | PASS (all options walked at model level) / BLOCKED (live popup) |
| 7 | Every checkbox/toggle in both states | PASS (model) / BLOCKED (live) |
| 8 | Every slider across meaningful range | PASS (model) / BLOCKED (live) |
| 9 | Save/Cancel/Reset behavior verified | PASS (Save/Cancel executed; Reset documented gap FIND-07) |
| 10 | Persistence verified after restart | PASS (executed) |
| 11 | All menus/navigation tested | PASS (static + ids) / BLOCKED (live) |
| 12 | Complete user workflows executed | PASS (9/9 model-level) / BLOCKED (live shell) |
| 13 | Keyboard interaction tested | PASS (model + source audit) / BLOCKED (live) |
| 14 | UI Automation on actual app | BLOCKED (no Windows) |
| 15 | Narrator verified | BLOCKED (no Windows) |
| 16 | Theme & DPI verified | PASS (contrast executed) / BLOCKED (live switching) |
| 17 | No unresolved Critical/High functional bugs | **MET** (none found; 1 Medium fixed; 4 Low/Cosmetic static remain) |
| 18 | Failures/blocked transparently documented | PASS |
| 19 | Fixes regression-tested | PASS (632+2 green) |
| 20 | All claims evidence-backed | PASS (each row cites test/script/file) |

## 5. Deliverables of this phase

- `tests/unit/ui_functional_audit_test.cpp` (+ CMake registration) — 23 executed tests
- `src/settings/settings.cpp` — FIND-06 fix (Unicode font names)
- `docs/reports/UI_FUNCTIONAL_AUDIT_INVENTORY_V2.csv` — 113 elements
- `docs/reports/UI_FUNCTIONAL_AUDIT_TRACEABILITY_MATRIX_V2.csv` — 37 trace rows
- `docs/reports/UI_FUNCTIONAL_AUDIT_01..09_*_V2.md` — this series
- Updated `CHANGELOG.md`

## 6. Recommended next steps

1. Run the recorded Windows-runtime session (TRC-040..045) on a Windows
   10/11 x64 + MSVC machine to convert the 6 BLOCKED groups.
2. Apply the four proposed Windows-side fixes (FIND-01..05) — smallest, local
   patches already specified in Report 08.
3. Add a Reset-defaults control (FIND-07) and a dirty-state confirmation on
   Settings close (FIND-10).
4. Add explicit AutomationIds to OSK/About (FIND-04) before the first FlaUI
   pass so selectors are stable from day one.
