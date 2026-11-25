# Code Readability Standards

Readability is the primary quality gate: a new developer must be able to read
any file top-to-bottom without hunting for hidden meaning.

## Function design

- Small and focused: one responsibility per function. The refactored
  compatibility engines are the reference (see `SpecEngine::run_rule_steps` —
  a flat step interpreter with no nesting deeper than three levels).
- Early returns over flag variables; guard clauses for invalid states.
- Meaningful conditions: extract named booleans instead of writing compound
  expressions inline (`is_token_matched`, `prev2_is_cluster_base`).
- Reduced nesting: loops over data, `switch` over steps, no nested `if` towers.

## Expressions

- No magic numbers — every literal that carries meaning is a named constant
  (`maximum_generated_edits`, `design_height`, `maximum_candidate_count`).
- Descriptive intermediate variables instead of chained one-liners
  (`const std::u32string decoded_text = decode_utf8_to_utf32(...)`).
- Complex boolean predicates become functions (`is_word_start`,
  `is_guard_satisfied`) so call sites read as sentences.

## Architecture hygiene

- Platform-independent core (`src/unicode`, `src/layout-api`, `src/dictionary`,
  `src/candidates`, `src/core`, `src/layouts/*`) never includes Windows/COM
  headers; the boundary lives in `src/platform/*` and is checked by
  `scripts/check-layering.py` in CI.
- No god classes: the Windows shell splits into `Application`, `OskWindow`,
  `TopbarWindow`, `ToastWindow`, `MenuPopup`, each with its own state type.
- No unnecessary inheritance; no premature abstractions; layout definitions and
  per-keystroke session state stay in separate types
  (`LayoutDefinition` vs `InputSession`).
- Business logic (transliteration, ranking, dictionary) and platform
  integration (TSF, hooks, injection) live in different layers.

## Verification

- `scripts/check-layering.py` — CI layering gate.
- `clang-tidy` (`readability-*`, `bugprone-*`) — noise-reduction checks.
- Code review: every new function should survive the "explain it out loud"
  test; if the explanation needs variable meanings looked up, rename first.
