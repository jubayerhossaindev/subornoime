# Naming Conventions

This document is the authoritative, project-wide naming standard. No developer
may choose a different style. Every internal identifier in the repository
follows this table; external contracts (Windows SDK, COM/TSF interfaces, the
public C API in `include/suborno/suborno.h`, third-party libraries) keep their
own naming.

## Required styles

| Identifier | Convention | Example |
|---|---|---|
| Class / Struct | PascalCase | `CompositionEngine`, `CandidateResult` |
| Enum type | PascalCase | `InputEventType`, `TokenClass` |
| Enum value | snake_case | `key_pressed`, `word_start` |
| Function / Method | snake_case, verb-based | `generate_candidates()` |
| Variable / Parameter | snake_case | `input_text`, `candidate_index` |
| Private data member | snake_case + trailing `_` | `composition_text_` |
| Namespace | snake_case | `suborno::core` |
| Type alias | PascalCase | `CandidateList` |
| Template type parameter | PascalCase | `ElementType` |
| Concept | PascalCase | `CharacterRange` |
| Constant | snake_case | `maximum_candidate_count` |
| Macro | UPPER_SNAKE_CASE | `SUBORNO_BUILD_VERSION` |
| File / Test file / Benchmark | snake_case | `candidate_generator.cpp`, `candidate_generator_test.cpp` |

## Machine enforcement

- `scripts/validate_naming.py` — repository-wide audit (file names, constants,
  one-letter and abbreviated locals). Runs in the CI `naming-and-format` job.
- `clang-tidy` with `.clang-tidy` — static analysis; naming-sensitive checks
  are configured for warnings, not errors.
- Manual review rule: naming context and domain meaning (e.g. Unicode units)
  cannot be proven by regex and stays a review responsibility.

## Exceptions (by design, documented)

1. **External contracts** — COM/TSF overrides keep `PascalCase`
   (`QueryInterface`, `DoEditSession`, `OnSetFocus`, …) and Windows message
   handlers keep Win32 names. The validator whitelist in
   `scripts/validate_naming.py` documents the accepted set.
2. **The public C API** (`include/suborno/suborno.h`) uses its published
   `SUBORNO_*` / `suborno_*` ABI names — they are external contracts.
3. **Feature vocabulary** — Bangla keyboard layout names
   (`suborno-classic`, `suborno-smart`, `probhat`, `khipro`, `national`) and the
   `src/layouts/compat` compatibility module are domain concepts and part
   of the shipped feature surface; they are not "old project" remnants.

## Meaningful names

A name must let a reader guess its purpose without reading the declaration:

- No abbreviations: `configuration`, not `cfg`; `dictionary`, not `dict`;
  `index`, not `idx`.
- No one-letter locals (`i`, `j`, `x`) outside genuine mathematical algorithms
  — and even there prefer `row_index`/`column_index` (see the Levenshtein DP in
  `spell_checker.cpp`).
- Booleans use `is_ / has_ / can_ / should_ / needs_ / supports_ / contains_ / matches_`.
- Unicode data names carry the unit: `utf8_byte_count`, `utf16_code_unit_offset`,
  `unicode_code_point_count`, `grapheme_cluster_count`.
- Containers are plural nouns; maps read as `x_by_y` (`layout_by_identifier`,
  `correction_by_input_text_`).
