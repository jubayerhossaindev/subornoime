# Unicode & Bengali Text Safety

**Status:** Active. Unicode handling is a critical safety area for an IME.

## Encoding discipline

Every text buffer's encoding is explicit in its type and its name:

- Internal pipeline: UTF-8 (`std::string`, names like `input_utf8`,
  `preedit_utf8`, `text_utf8`).
- Windows-facing strings: UTF-16 (`wchar_t`/`std::wstring`, converted at the
  platform boundary with WIL helpers).
- Byte vs character counts are never mixed: variables carry the unit
  (`utf8_byte_count`, `utf8_byte_offset`, `utf16_code_unit_count`,
  `unicode_code_point_count`). A generic `character_count` that could mean any
  unit is banned.

## Length distinctions (mandatory)

Byte count, UTF-16 code-unit count, code-point count and grapheme-cluster
count are four different numbers. The codebase names which one it holds and
converts explicitly (`UNICODE_SAFETY` rule enforced by review and by the
linguistic regression tests).

## Validation rules

1. Invalid UTF-8 sequences are detected and rejected
   (`SUBORNO_STATUS_INVALID_UTF8`), never silently reinterpreted — verified by
   `tests/unit/unicode_test.cpp` and `c_api_test.cpp`.
2. The UTF-8 walker (`for_each_utf8_codepoint`, `src/dictionary/dictionary.cpp`)
   checks lead-byte classes and continuation bits with explicit bounds; it
   never reads past `text.size()` and rejects truncated sequences.
3. Unpaired UTF-16 surrogates and out-of-range code points are rejected at the
   conversion boundary.
4. Conversion buffer bounds are enforced before every copy
   (`BUFFER_AND_BOUNDS_SAFETY.md`); truncation never splits a code point.
5. Bengali combining characters (vowel signs, virama, conjuncts) are treated
   as grapheme parts: backspace and grapheme counting never split a cluster
   (regression-tested in `bds_conformance_test.cpp`,
   `bds1520_official_corpus_test.cpp`).
6. Normalization: the pipeline keeps NFC-equivalent dictionary keys
   (normalized column, `dictionary.hpp`) and documents where normalization is
   applied.
7. Grapheme boundaries are never broken mid-composition; the composition
   buffer limit counts characters the way users perceive them.

## Assamese policy

Assamese-specific characters stay out of the Bengali character inventory;
language-specific behavior is layered per layout, not by mixing inventories
(requirement inherited from the product specification).

## Verification evidence

- `tests/unit/unicode_test.cpp` — invalid sequences, mixed scripts, byte/unit
  counting.
- `bds_conformance_test.cpp` + `bds1520_official_corpus_test.cpp` — official
  Bengali typing-corpus conformance.
- `linguistic_regression_test.cpp`, `layout_typing_regression_test.cpp` —
  end-to-end Bangla output equivalence with the reference corpus.
- Fuzz campaigns feed arbitrary bytes through the phonetic pipeline and C API
  (invalid UTF-8 seed corpus included: `tests/fuzz/corpus/0008_invalid_utf8_sequences`).
