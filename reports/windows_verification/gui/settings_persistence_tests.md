# Settings Persistence Tests

Status: **harness persistence checks BLOCKED (runtime)**; portable settings
logic **executed (Linux)**.

## What the harness asserts (implemented, executes in the workflow job)

- **Save** (`action.save`, UIA Invoke and real mouse click as separate
  checks): the settings file on disk is re-read and must reflect the staged
  changes (file rewrite asserted).
- **Cancel** (`action.cancel`): the settings file must be **unchanged**.
- **Rapid 3× Save**: process stays alive; file remains consistent.

## Portable logic (executed on Linux)

`settings_test` in the 686-test suite: load/save round-trip, defaults,
validation, atomic-write behavior of the persistence layer
(`src/persistence/atomic_file.cpp` — `_WIN32`-guarded Windows branch +
POSIX branch both compile in CI), invalid/missing/corrupt-file recovery.

## Mission §14 remaining items (runtime, BLOCKED)

| Item | Where |
|---|---|
| Change → save → **restart** → verify persisted value | manual restart protocol |
| Reset-to-defaults behavior | manual protocol (Reset control) |
| Permission-failure / failed-write recovery in an isolated dir | portable tests cover the code path; Windows filesystem ACL variants manual |
| Corrupted settings recovery | portable tests + manual corrupt-file scenario |
| Repeated saves | harness rapid-Save check |

No persistence claim beyond the Linux-executed portable logic.
