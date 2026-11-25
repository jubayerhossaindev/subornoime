# Candidate Window Tests

Status: engine-level **executed (Linux)**; UI-level **BLOCKED (runtime)**.

## Engine level (executed, Linux evidence)

`part_iii/candidate_navigation.md`: next/previous walk both boundaries
without wraparound; out-of-range selection rejected atomically; commit
follows the selected candidate; reset restores a usable state; 500-cycle
rapid churn keeps the index in bounds; empty-state safe. 35 tests across
CandidateNavigation + candidate suites — all PASS, incl. under ASan+UBSan.

## UI level (BLOCKED)

The candidate window is a TSF UI element (`tsf/candidate_list_ui_element.cpp`
— ITfCandidateListUIElement provider, 16 boundary-guarded methods) plus the
app-side candidate window (`show_candidate_window` setting, orientation /
fixed-height / sliding-effect / follow-caret / follow-mouse options —
inventory rows 4, 18–22).

| Check | Where it runs | Status |
|---|---|---|
| Selection changes the highlighted item | runner (UIA) + interactive desktop | BLOCKED |
| Commit inserts the exact Bengali text into a host app | interactive desktop (TSF typing) | BLOCKED — manual protocol |
| Empty candidate state | engine tests (green) + UI walk | BLOCKED (UI half) |
| Out-of-range / reset | engine tests (green) | executed |
| Window position follows caret/mouse | manual protocol (mixed-DPI) | BLOCKED |
| Rendering with the configured font/theme | manual protocol + screenshots | BLOCKED |

No UI candidate interaction is claimed as verified.
