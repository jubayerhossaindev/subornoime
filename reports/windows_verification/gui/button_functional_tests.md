# Button Functional Tests

Status: **BLOCKED (runtime)** — harness check ids `SET-*` (10),
`ABT-buttons`/`ABT-open` (2), OSK invokables (`OSK-*`), executed by the
`windows-uia-audit` workflow job.

## Buttons in scope (from the source inventory)

| Button | Expected observable behavior | Harness check class |
|---|---|---|
| `action.save` | settings file rewritten with the staged values | UIA Invoke AND real mouse click — **separate checks**; persistence asserted |
| `action.cancel` | settings file unchanged, staged edits discarded | file-mtime/content comparison |
| `action.close` | settings window closes | element disappearance |
| `zone.close` | topbar hides | topbar element state |
| `zone.wrench` | settings opens | settings element appears |
| Clear Learnt Words (destructive) | confirmation then immediate dictionary operation | inventory row 28 (button kind) |
| About dialog buttons (≥5) | dialog opens; button count asserted | `ABT-buttons` |
| OSK keys (≥37 invokable) | each key is invokable/discoverable | `OSK-*` |

## The harness's anti-false-positive rules (already implemented)

- UIA `Invoke` success is **never** treated as proof of a mouse-click PASS —
  both are recorded separately (`SET-SAVE-invoke` covers the semantic path;
  a `SendInput`-class click at the element center covers the physical path).
- Save persistence is asserted against the actual settings file, not visual
  state (mission §15's "independent observable result").
- Rapid 3× Save invokes with process-liveness assertion.
- Exit code = failed check count → the workflow job fails on any failure.

## Required from the run (per mission §5)

Initial state read, enabled/disabled verification, disabled-state behavior,
repetition behavior, unintended-side-effect check — the harness covers
state/persistence; disabled-state and boundary variants for individual
buttons remain in the manual protocol where the harness has no check id
(documented in `checkmap.json` meta note).
