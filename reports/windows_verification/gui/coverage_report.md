# GUI Coverage Report

Formulas (mission §20):

- `Control Coverage = Tested Controls / Discovered Controls × 100`
- `Interaction Coverage = Tested Applicable Interactions / Discovered Applicable Interactions × 100`
- `Workflow Coverage = Executed Workflows / Identified Workflows × 100`

Skipped/blocked interactions never count as successful.

## Current measured values (honest)

| Metric | Discovered | Tested (runtime) | Coverage |
|---|---|---|---|
| Windows | 7 (main, topbar, settings, OSK, toast, menu popup, candidate) | 0 runtime / 7 source-audited | 0% |
| Controls | 31 semantic actions + topbar zones + menu items + OSK keys | 0 runtime / all source-mapped | 0% |
| Applicable interactions | 98 harness check ids + manual-protocol rows | 0 | 0% |
| Workflows | 6 (A–F) | 0 | 0% |
| UIA accessibility coverage | all elements provider-exposed | 0 runtime | 0% |
| Source-handler coverage | every `CtrlAction` mapped to a handler in source | executed as audit | 100% (source-level only) |

**Runtime coverage is currently 0%** — no GUI interaction has executed.
This is the honest baseline; the workflow's `windows-uia-audit` job fills
the runtime columns from `uia-results.json`:

- Control Coverage = checks with PASS / 98 (happy+failure paths).
- Interaction Coverage counts each applicable event type actually driven.
- Workflow Coverage = journeys whose every step has executed evidence.

## Code coverage note

UI interaction coverage ≠ internal code coverage. Portable-core code
coverage evidence lives in the Linux suite (686 tests + sanitizers + soak);
Windows-side code coverage (TSF/COM/app) is exercised by
`tsf_contract`/`app_smoke` on the runner. No 100%-tested claim is made or
can be made from this evidence set.
