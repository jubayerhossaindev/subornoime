# Type Safety Standards

Strong typing prevents whole classes of IME bugs — mixed encodings, swapped
indices, invalid enum states.

## Rules

1. **`enum class` only.** Unscoped enums are not accepted in new code.
2. **`std::optional`** for "may not exist" (`std::optional<POINT>
   saved_topbar_position`), never sentinel values like `-1` or empty strings.
3. **`std::variant`** for closed alternatives; `std::span`/`std::string_view`
   for non-owning views; `std::string_view` at read-only UTF-8 boundaries.
4. **Concepts** where template constraints improve diagnostics
   (`template <typename Parse> std::string parse_without_autocorrect(Parse&&
   parse, …)`).
5. **Const correctness**: every function that does not mutate state is `const`
   (`is_word_start() const`, `find_longest_match(...) const`); locals default
   to `const` (`const int parsed = std::stoi(...)`, `const std::string
   normalized`).
6. **No implicit narrowing conversions**: indexes cross `std::size_t` ↔
   integers only with explicit `static_cast` and a comment when the unit
   changes (see the `static_cast<std::ptrdiff_t>` cursor arithmetic in the
   `.mim` runtime).
7. **Unicode type discipline**:
   - `std::string` always holds UTF-8; `std::u32string` holds code points;
   - conversion happens through named helpers (`decode_utf8_to_utf32`,
     `encode_utf32_to_utf8`) — never raw reinterpretation;
   - counts and offsets carry their unit in the name
     (`utf8_byte_count`, `code_point_cursor`).
8. **Type aliases** for domain vocabulary (`using CandidateList =
   std::vector<CandidateResult>;`).
9. **Avoid implicit conversions**: no `bool`-returning classes, no single-
   argument converting constructors without `explicit`.
10. **`[[nodiscard]]`** on functions whose result must not be ignored
    (`[[nodiscard]] std::string transliterate(...)`).

## Enforcement

- Compilers: all three CI compilers build with warnings enabled; new implicit
  conversion warnings are fixed, not suppressed.
- No global warning suppressions; per-MSVC-analysis exemptions live in
  `cmake/modules/SubornoBuildOptions.cmake` with justifications.
