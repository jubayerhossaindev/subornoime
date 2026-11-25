# UI_FUNCTIONAL_AUDIT_07 — Persistence & State Report (Phase 2, 2026-11-24)

All core persistence behavior is **executed** in this environment against real
files (atomic writes, quarantine, restart cycles). Registry/TSF-adjacent steps
are BLOCKED. Test code: `tests/unit/ui_functional_audit_test.cpp` + existing
settings/persistence suites.

## 1. Save (mission 3.2)

| Requirement | Evidence | Result |
|---|---|---|
| Modify one setting and save | `SavingOneControlLeavesEveryOtherValueIntact` | PASS |
| Modify multiple settings and save | `RestartPersistenceEverySavedSettingRestored` (10 keys mutated together) | PASS |
| Save without changes | `SaveIsIdempotentByteForByteWhenNothingChanged` (byte-identical rewrite) | PASS |
| Save repeatedly | `RapidRepeatedSaveLoadCyclesStayConsistent` (50 cycles) | PASS |
| Save with boundary values | opacity 0/100, idle time 1/3600, candidate_limit 1/100 | PASS |
| Save with invalid values | `SliderAndLimitValuesOutsideRangeAreRejected` (101, 0, 3601, 0-limit, 101-limit → `invalid_format`) | PASS |
| Save when persistence fails | `SaveFailureInReadOnlyDirectoryPreservesExistingSettings` (existing suite, still green) | PASS |
| Unrelated settings unchanged | every round-trip test asserts the full struct | PASS |
| Restart restores saved values | fresh `Engine` + fresh `SettingsStore` over the same file | PASS |
| Success feedback only after persistence | app policy audited: toast raised after `engine_->save_settings` success; failure → rollback + error dialog (static trace) | PASS (static) |

Write path: whole-file atomic replace via `persistence::write_file_atomically`
(unique temp file, fsync of file **and** parent directory, rename-replace);
temp file removed on failure (existing atomic-file suite, still green).

## 2. Cancel (mission 3.3)

`CancelLeavesPersistedStateUntouched`: staged copy mutated (theme, opacity,
startup flag) and dropped — the file on disk is byte-identical and the
reloaded state keeps pre-cancel values. Close-X shares the Cancel contract
(source trace). Live-window cancel-while-typing: BLOCKED.

## 3. Reset / restore defaults (mission 3.2/3.9)

There is **no Reset/Restore-defaults control** in the Settings window
(FIND-07, documented design gap). The model-level equivalent is verified:
a store over an absent file yields defaults; the quarantined-file path also
recovers to defaults.

## 4. Corruption & recovery

`CorruptSettingsQuarantinedAndDefaultsRestored`: an unknown key makes load
return `invalid_format`; the app-policy quarantine renames the file to
`.corrupt-<pid>-<n>` **preserving the original bytes** (asserted), and the
next start behaves like first launch (defaults, `file_existed=false`).
Duplicate keys, missing version, unsupported versions, oversized lines: all
rejected by the existing parser suite (still green).

## 5. Immediate-save paths (bypass Save/Cancel)

Six paths persist immediately and were audited + model-verified: save_font,
save_layout, persist_last_ui_mode, topbar-position persist, toggle_encoding,
startup-registry preference (registry itself is Windows-runtime BLOCKED).
`PredictionTogglePersistsThroughEngine` demonstrates the engine's
whole-file-save toggle path.

## 6. Multi-writer honesty note

Settings save is last-writer-wins (no lock/stamp re-check before write).
Correct for the single-process Windows shell; recorded as a known constraint
for any future multi-process adapter (engine's `FileStamp` change-detection
exists and is tested).
