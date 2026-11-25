# Slider Functional Tests

Status: **BLOCKED (runtime)** — harness ids `SLD-*` (8), executed by the
workflow job.

## Sliders in scope (source inventory rows 6–7)

| Slider | Controls | Expected actual behavior (mission §6: value change alone is NOT proof) |
|---|---|---|
| `action.idle_opacity` | topbar idle opacity | topbar's rendered opacity changes accordingly |
| `action.idle_time` | idle-detection threshold | dimming engages after the configured idle interval |

## Harness behavior (implemented)

RangeValue `SetValue` at **minimum / midpoint / maximum**, then Save; the
persisted settings file is re-read to confirm the values stuck (Save
persistence is asserted by the shared Save checks). Visual verification of
opacity/threshold timing is manual-protocol work (screenshot comparison for
opacity; stop-watch for idle threshold) — recorded in
`Manual-Test-Protocol.md`, not assumed by the harness.

## Mission §6 step list → coverage

1–5 (min/max/current/step/initial): RangeValue reads — harness.
6–12 (set min/mid/max/intermediates): harness (`SLD-*`).
13–14 (keyboard arrows, Home/End): manual protocol.
15–17 (thumb drag, both extremes, beyond boundaries): manual protocol
(real input injection to a draggable thumb needs the interactive desktop
the runner does not guarantee).
18–20 (clamping, displayed value, controlled feature): harness for persisted
value; feature behavior manual.
21–25 (save, restart, persistence, reset to defaults): harness Save +
manual restart protocol.

Honest note: **keyboard stepping, dragging and beyond-boundary clamping are
not automatable on the hosted runner** — they are explicitly marked manual,
not skipped.
