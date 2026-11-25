# Error Handling Standards

Errors are part of the contract of every function. Each layer chooses the
mechanism that fits its caller; the choice is consistent inside the layer.

## Mechanisms by layer

| Layer | Mechanism | Notes |
|---|---|---|
| Public C API | Status codes (`SUBORNO_Status` enum) | `SUBORNO_ERROR_*` negatives for API misuse; exceptions never cross the boundary — `protect_c_boundary` converts them |
| Core C++ | Exceptions for exceptional states, status enums for expected outcomes (`InputStatus::updated / pending / literal_passthrough / invalid_utf8 / input_limit`) | Types: `std::runtime_error` with context ("keymap: unterminated list") |
| File/IO | `std::error_code` overloads + explicit checks (`is_usable_dictionary_file`) | No unchecked `filesystem::exists` |
| Windows/COM | `HRESULT` propagation; every call checked (`if (result != S_OK) return result;`) | No silent swallowing; `wil::com_ptr` for object lifetime |
| Parsers | Fail-soft with documented fallback (loose integer parse falls back; malformed UTF-8 lead bytes skipped) | The fallback IS the documented behaviour, verified by tests |

## Rules

1. Every error path must leave the object in a valid state (strong or basic
   guarantee; document which when it matters — `parse_without_autocorrect`
   restores the correction switch even when `parse` throws).
2. Expected, recoverable conditions are **not** exceptions: invalid user input,
   out-of-range candidate index, buffer-too-small — these are status values.
3. Programming errors are bugs, not errors to "handle": assert invariants in
   debug builds, and let tests catch them.
4. No `catch (...) { /* ignore */ }`. Every catch either rethrows, converts to
   the layer's error representation, or documents why ignoring is safe
   (`parse_integer_with_fallback` documents its catch as the loose-parse
   contract).
5. Error messages name the failed operation and the offending value where
   cheap: `"keymap: unterminated string"`.
6. The C API never leaks: commit outputs are cleared **before** validation
   returns a failure, so a caller cannot observe a stale pointer.
7. Failure representations are defined per function family and documented in
   Doxygen (`@return … SUBORNO_ERROR_INVALID_ARGUMENT when …`).
