# Input Validation & External Input Handling

**Status:** Active. Every external input is untrusted until validated.

## Input sources and their validation points

| Source | Entry point | Validation |
| --- | --- | --- |
| Keyboard input (TSF) | `TextService::OnKeyDown`/`OnTestKeyDown`/`OnPreservedKey` | key filtering → engine append path |
| Keyboard input (C API / standalone) | `suborno_session_append_utf8` | null + UTF-8 validity + length limit |
| Windows TSF data | edit sessions, compartments | HRESULT checks, cookie range, compartment value validation |
| Dictionary files | `dictionary_import_main.cpp` | file size, entry count, line format, transaction rollback |
| User settings | settings loader | schema/enum/range validation before state mutation |
| Layout definitions | layout tables (code) + legacy import (`compat`) | token table validation (`tokens_.empty()` → `std::invalid_argument`) |
| Configuration | settings persistence | `AtomicFile` with rollback; partial writes never activate |
| External C API | every `suborno_*` function | pointer/nullability/bounds/status contract (`API_ABI_SAFETY.md`) |
| Windows API output | UI/platform helpers | return value + `GetLastError()` handling, buffer length checks |

## Mandatory validation checklist

For every input: length, encoding, nullability, bounds, integer conversion,
allocation size, file size, entry count, invalid sequences, invalid state,
invalid identifiers, unsupported format.

## Rules

1. Validate first, mutate after. An untrusted value never reaches internal
   state before passing its checks.
2. No unbounded allocation from untrusted data — every loop that builds
   containers from input has a documented maximum
   (`BUFFER_AND_BOUNDS_SAFETY.md` table).
3. Rejection is explicit and status-coded; silence is a bug.
4. Integer conversions from Windows/API types use explicit `static_cast` with
   range checks where the source can exceed the destination.
5. File parsing cannot exhaust memory: the import tool streams records and
   rolls back the transaction on any error.

## Verification evidence

- 652 unit tests include boundary cases per input source (`c_api_test.cpp`,
  `c_api_coverage_test.cpp`, `settings_test.cpp`, `settings_coverage_test.cpp`,
  `personal_dictionary_test.cpp`, `persistence_coverage_test.cpp`).
- Fuzz campaigns treat the phonetic pipeline and the C API as untrusted
  by construction (`FUZZ_TESTING.md`).
- The import tool's rollback path is exercised by the seed-dictionary CI step
  (`ci.yml` linux-system-deps job).
