# Visual Regression Evidence

Status: **BLOCKED (runtime)** — screenshots are captured by the audit
harness during the workflow run.

## What the harness captures (implemented)

PNG screenshots of: Settings (each audited page), About dialog, OSK, Topbar,
toast — plus any failed-interaction capture, written to the evidence
directory and referenced from every check row in `uia-results.json`.

## Rules honored

- Screenshots are **supplementary evidence only** — every behavioral claim
  has a programmatic assertion behind it (mission §21).
- Visual comparison (pixel-diff against the previous run's baseline) is not
  automated in this repository; the harness stores per-run PNGs so a human
  or a future visual-diff tool can compare runs. No fabricated baseline
  exists.
- Contrast/accessibility visual checks: `scripts/contrast_check.py` exists
  for palette-level contrast (portable, executable anywhere) — the runtime
  visual verification of themed windows remains manual-protocol.

## Expected artifact set from the run

`uia-evidence/*.png` (per window), envelope JSON mapping each check id →
screenshot path, uploaded as `windows-uia-audit-<run_number>` regardless of
pass/fail (`if: always()`).
