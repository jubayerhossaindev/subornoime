# Text Input Tests

Status: **BLOCKED (runtime)** for GUI text fields; **engine-level text
handling fully executed on Linux** (686-test suite: grapheme-safe UTF-8
append, invalid-UTF8 rejection, 256-codepoint clamp, Bengali/ASCII/mixed
corpus regressions — `part_iii/input_reliability.md`).

## GUI text fields in scope

The settings pages expose dropdowns/checkboxes/sliders; free-text fields are
not part of the settings UI surface found in source (no `CtrlKind::text`
entry — `application.cpp:87-100` kinds are section_label/checkbox/dropdown/
slider/status/button). Runtime discovery by the audit walk confirms or
refutes this inventory — any runtime-discovered text field gets added to
`ui_control_inventory.md` and tested per mission §10 (empty/valid/invalid/
Unicode/Bengali/paste/editing/focus/validation/save-reload).

## Engine-level execution already done (Linux, evidence cited)

| Mission §10 item | Engine coverage |
|---|---|
| Unicode/Bengali/ASCII/mixed input | corpus regression tests (BDS-1520 official corpus, linguistic regressions) |
| Invalid input | invalid-UTF8 → `invalid_utf8` status (unit-tested) |
| Maximum length | 256-codepoint clamp + atomic reject above capacity |
| Backspace/Delete editing | backspace tests + `KeyMatrix` editing keys |
| Leading/trailing whitespace | wordlist validation suite |
| Save/reload | settings persistence tests (portable `settings_test`) |
| No user text to diagnostics | PII-safe logger audit (`part_iii/diagnostic_privacy.md`) |

Privacy rule kept: only deterministic test text (`ami`, `tumi`, corpus
seeds) — never real user text.
