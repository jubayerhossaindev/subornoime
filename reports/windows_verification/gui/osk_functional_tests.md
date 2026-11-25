# OSK Functional Tests

Status: **BLOCKED (runtime)** — harness ids `OSK-*` (open + invokable
element count ≥ 37), executed by the workflow job.

## Source-level facts (executed audit)

- The OSK is a custom-drawn window (`osk_window.cpp`) whose keys are exposed
  through the UIA provider; the harness counts invokable keys per layout
  page and asserts ≥ 37.
- Key→output mapping is owned by the portable layout engine — every layout's
  full key mapping is already regression-tested on Linux:
  - Phonetic/Borno/Avro/Khipro: phonetic pipeline tests + BDS-1520 official
    corpus + linguistic regressions (686-test suite, green).
  - Probhat/National: dedicated layout suites (59 tests) + typing
    regressions (17).
- Shift/modifier behavior, backspace/space/enter, repeat: `KeyEventMatrix`
  (11 tests) + `KeyEventTest` (11) — green on Linux.

## What the OSK runtime test adds (and why it stays BLOCKED)

The mission requires pressing every visible key through the **intended
input-service path** in a real text application — i.e., key press → OSK
handler → session/engine → composition → committed text in a host editor.
The harness verifies the OSK's discoverability/invokability; the
output-verification half needs the interactive desktop (same constraint as
TSF typing — see `../tsf_runtime_report.md`). The per-key expected-character
table is the existing per-layout engine test data; the runner job asserts
the OSK exposes an invokable element for each.

## Per-layout independence

Never assumed across layouts: each layout's mapping is its own Linux test
set; the OSK runtime check enumerates the visible keys per selected layout
(`OSK-*` + manual layout-switch walk).
