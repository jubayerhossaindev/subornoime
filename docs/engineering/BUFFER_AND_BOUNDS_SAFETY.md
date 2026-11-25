# Buffer & Bounds Safety

**Status:** Active. Complements `MEMORY_SAFETY_POLICY.md`.

## Container policy

Unbounded raw buffers are forbidden. Prefer, in order: `std::string`,
`std::vector`, `std::array`, `std::span` (view only). Raw C arrays appear only
where a platform ABI requires them and carry explicit bounds documentation.

## Bounds validation rules

1. Untrusted indices never touch `operator[]` unchecked. External candidate
   indices are validated against `.size()` before access in both the internal
   session (`InputSession::select_candidate`) and the C API
   (`suborno_session_candidate_snapshot` returns
   `SUBORNO_STATUS_CANDIDATE_OUT_OF_RANGE`).
2. Every buffer copy knows both sides' sizes:
   `copy_text_to_buffer` rejects text that does not fit
   (`SUBORNO_ERROR_BUFFER_TOO_SMALL`) and always NUL-terminates.
3. `size()` and `capacity()` are treated as different facts; reservation
   happens through container APIs only.
4. Byte offsets into UTF-8 text are named as byte quantities
   (`utf8_byte_count`, `utf8_byte_offset`) and are validated against
   `text.size()` before use (`UNICODE_SAFETY.md`).

## External input limits

| Input source | Limit point | Constant / mechanism |
| --- | --- | --- |
| Keyboard/composition input | append entry points | engine composition buffer limit → `SUBORNO_STATUS_INPUT_LIMIT` |
| Fuzz harness input | `tests/fuzz/fuzz_targets.hpp` | `maximum_fuzz_input_size = 4096` (truncates before the engine sees it) |
| Dictionary files | import tool + repository | entry count / file size guarded in `dictionary_import_main.cpp`; transaction rollback on failure |
| Settings files | settings loader | schema validation before state mutation (`INPUT_VALIDATION.md`) |
| Candidate generation | candidate engine | candidate count capped by engine policy |

Untrusted data never drives an allocation size directly.

## Arithmetic safety

- Allocation size computations check for overflow before allocating: the C API
  commit path rejects `size == std::numeric_limits<std::size_t>::max()` before
  `std::malloc(size + 1)`.
- Signed/unsigned comparisons and narrowing conversions go through explicit
  `static_cast` with a review reason; MSVC `/W4` + `/sdl` and clang-tidy
  `bugprone-*` watch the rest.
- Multiplications that feed allocation or index math must be range-checked
  first.

## Verification evidence

- 652 unit tests include empty input, oversized input, invalid UTF-8,
  maximum-buffer and out-of-range cases (`tests/unit/c_api_test.cpp`,
  `c_api_coverage_test.cpp`, `phonetic_input_session_test.cpp`).
- ASan+UBSan instrumented run: clean (`SANITIZER_CONFIGURATION.md`).
- Fuzz campaigns cover truncation, byte flips and repetition at the boundary
  (`FUZZ_TESTING.md`).
