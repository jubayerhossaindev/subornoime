# Topbar and Toast Tests

Status: **BLOCKED (runtime)** — harness ids `TOP-*` (5) + `TOAST-*` (1),
executed by the workflow job.

## Topbar (4 zones, source-verified AutomationIds)

`zone.layout`, `zone.mode`, `zone.wrench`, `zone.close` (`topbar.cpp`).

Harness assertions: all four zones visible and enabled with non-empty
bounding rectangles, on every audit pass. Additional behaviors and their
placement:

| Behavior | Check | Status |
|---|---|---|
| Zone invoke (layout switch / mode / settings / hide) | UIA Invoke per zone + state assertions | BLOCKED |
| Zone visibility rules (configurable topbar settings) | manual protocol | BLOCKED |
| Idle dimming + opacity (slider-driven) | manual protocol (visual) | BLOCKED |
| Position remember/restore | manual restart protocol | BLOCKED |
| Drag/move | manual protocol | BLOCKED |

## Toast (`toast.message`)

Harness: trigger a live toggle that emits a toast and assert the
`toast.message` element appears (content comes from the toggle result).
Toast auto-dismiss timing and toast-on-error variants: manual protocol.

## Non-claims

Zone presence in source + UIA provider registration is verified; runtime
invoke results come only from the workflow job's envelope JSON.
