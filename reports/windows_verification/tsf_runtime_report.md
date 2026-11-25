# TSF Runtime Report (Edit Sessions, Composition, Interactive Typing)

Status: **source-level `WINDOWS SOURCE AUDIT — VERIFIED`**; **runtime
execution BLOCKED** (no GitHub-hosted runner reachable from this sandbox).

## What the source provides (executed audits)

| Mission area | Implementation | Audit result |
|---|---|---|
| Edit sessions | `tsf/text_edit_session.cpp` (ITfEditSession impls, read/write sessions, all guarded) | 4 boundary-guarded entry points |
| Composition lifecycle | `tsf/text_service.cpp` (ITfKeystrokeMgr sink, composition start/update/commit/terminate via ITfRange edits) | 20 guarded entry points |
| Candidate UI | `tsf/candidate_list_ui_element.cpp` (ITfCandidateListUIElement minimal provider) | 16 guarded entry points |
| Display attributes | `tsf/display_attribute_info.cpp` | 11 guarded entry points |
| Compartment state | `tsf/compartment.cpp` (keyboard-open/algswitch compartments) | 2 guarded entry points |
| Thread/apartment assumptions | `class_factory.cpp` + `dll_main.cpp` (STA expectations documented, `DllGetClassObject` guarded) | 3 guarded entry points |

Bengali output itself is produced by the portable core (686-test unit suite
green on Linux) — the TSF layer only routes text into `ITfRange` inserts.

## Runtime classification (mission §7 — honest split)

| Check | Status |
|---|---|
| COM activation (CoCreateInstance of the service, interface queries) | BLOCKED — `suborno.windows.tsf_contract` runs it on the runner |
| Edit-session request/read-write behavior | BLOCKED — same target |
| Composition lifecycle against a real context | BLOCKED — same target + app smoke |
| **Actual interactive Bengali typing through TSF** | **BLOCKED — needs an interactive desktop with a host editor; hosted `windows-2022` runs the UIA audit of the app's own windows but cross-application TSF typing requires the manual protocol** |
| Candidate window rendering on screen | BLOCKED — same interactive-desktop constraint |

The audit harness (84 checks) intentionally does **not** substitute engine
unit tests for TSF typing; no fake typing test was added.

## Required follow-up environment

An interactive Windows session (physical or self-hosted runner) running:
the installed profile enabled in `ITfInputProcessorProfiles`, a host editor
(Notepad), and the manual-protocol steps from
`scripts/windows-ui-audit/Manual-Test-Protocol.md`.
