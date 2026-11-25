# Complete User Journeys

Status: **BLOCKED (runtime)** — every journey below runs in the workflow
jobs where the harness covers it, else in the manual protocol. Engine-level
prerequisites are already executed on Linux (green 686-test suite).

| Journey | Steps | Coverage |
|---|---|---|
| **A — Basic typing** | install/launch → activate profile → host app → type Bengali → verify inserted text → backspace → retype → commit | TSF typing half BLOCKED (interactive desktop); engine half executed (corpus regressions) |
| **B — Phonetic input** | phonetic layout → type sequence → candidates → navigate → select → commit → verify exact Bengali | engine half executed (CandidateNavigation + phonetic suites); UI half BLOCKED |
| **C — Layout switching** | settings → select each of 7 layouts → verify activation → type → switch back → no stale state | harness `DD-layout` + `NAV`; stale-state engine tests executed (`ResetClearsComposition…`); runtime BLOCKED |
| **D — Settings persistence** | change controls → save → restart → verify values → behavior matches | harness Save/persistence + manual restart — BLOCKED |
| **E — Window interaction** | OSK, candidate window, topbar, toast, menus; move/resize/focus/close; resource cleanup | harness `OSK-*`/`TOP-*`/`TOAST-*` + manual — BLOCKED |
| **F — Tray and lifecycle** | start → tray icon → menu → non-destructive actions → settings → exit → restart → clean init | manual protocol (see `tray_tests.md`) — BLOCKED |

## Isolation requirements (mission §17)

The runner provides a disposable machine per run; the harness uses a
controlled app instance, deterministic test text only (no real user text),
explicit timeouts and process-liveness assertions. Fresh-install vs
configured-state runs: the installer suite performs clean + upgrade
installs; the UIA audit runs on the freshly installed app.

## Verification standard

Every journey step needs an **independent observable result** (mission §15):
persisted file, committed Unicode text, real window handle/element, process
exit. Visual activity alone is never accepted.
