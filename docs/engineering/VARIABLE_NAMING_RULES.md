# Variable Naming Rules

Variables are the highest-priority naming surface. A variable name must state
what it holds, in which unit, and (when not obvious) where the data comes from.

## Rules

1. **Noun-based.** The name names the content: `input_text`, `committed_text`,
   `dictionary_file_path`, `candidate_results`, `dictionary_entries`.
2. **Context-aware, not redundant.** Inside `CandidateGenerator`, the member is
   `candidate_count_` — not `candidate_generator_current_candidate_count_`.
3. **Generic names are banned** when they carry no domain meaning:
   `data`, `info`, `value`, `result`, `item`, `object`, `temp`, `temporary`,
   `buffer`, `context`, `manager`, `processor`, `handler`, `helper`, `utility`,
   `state`, `status`, `type`, `number`, `count`, `name`, `text`.
   Give the variable the role it plays: `candidate_generation_result`,
   `layout_metadata`, `selected_candidate_value`, `composition_state`.
4. **Booleans** read as assertions: `is_composition_active`, `has_dictionary_entry`,
   `can_accept_keyboard_input`, `should_generate_candidates`, `needs_dictionary_reload`,
   `supports_phonetic_layout`, `contains_bengali_characters`. Banned:
   `flag`, `state`, `active`, `valid`, `check`, `enable`, `not_ready` (double
   negative — write `is_enabled`).
5. **Distinguish count / index / identifier / size / offset / length.**
   `candidate_count` (how many), `selected_candidate_index` (position),
   `layout_identifier` (identity), `input_buffer_size`, `utf8_byte_offset`,
   `utf8_byte_count`.
6. **Unicode naming carries the encoding and the unit** (the IME core processes
   UTF-8/UTF-16/UTF-32):
   `utf8_input_text`, `utf16_composition_text`, `utf32_unicode_text`,
   `utf8_byte_count`, `utf16_code_unit_count`, `unicode_code_point_count`,
   `grapheme_cluster_count`, `utf8_byte_offset`. Never collapse these into a
   generic `character_count` when the units differ.
7. **Strings** end with their role: `_text`, `_name`, `_path`, `_identifier`,
   `_key`, `_message`, `_content`, `_pattern`, `_query`, `_expression`,
   `_description`.
8. **Containers** are plural; element names singular; maps read `x_by_y`:
   `candidate_results`, `available_layouts`, `dictionary_entries`,
   `candidate_score_by_text`, `layout_by_identifier`,
   `correction_by_input_text_`.
9. **Loop variables** are never `i/j/k/n/x/y/z` in application code. Use
   `candidate_index`, `layout_index`, `character_index`, `grapheme_index`,
   `token_index`, `row_index`/`column_index` (matrix algorithms). The
   `scripts/validate_naming.py` CI check rejects one-letter loops.
10. **Iterators** are `x_iterator` (`candidate_iterator`,
    `matching_layout_iterator`, `dictionary_entry_iterator`), never `it`/`itr`.
11. **Pointers/references** name the pointee (`active_layout`,
    `selected_candidate`); ownership and nullability are expressed with
    `std::unique_ptr` / `std::shared_ptr` / `std::optional` / references, not
    with the name.
12. **Constants** are snake_case (`maximum_candidate_count`,
    `maximum_input_buffer_size`, `default_layout_name`) — never `kPascalCase`.
    Magic numbers are forbidden; name the purpose.
13. **Durations and sizes carry units**: `input_timeout_duration` (type
    `std::chrono::milliseconds`), `dictionary_file_size_in_bytes`,
    `input_buffer_capacity_in_bytes`.
14. **Parameters** are fully meaningful at the call site:
    `update_candidate_selection(selected_candidate_text, selected_candidate_index,
    should_commit_composition)`.
15. **Lambdas** get descriptive names when stored
    (`const auto find_matching_candidate = …`); capture lists and lambda
    parameters follow the same rules as ordinary variables.
16. **Structured bindings** name both elements:
    `const auto [layout_name, layout_definition] = registered_layout;`.
17. **Callbacks** state their purpose: `candidate_selection_callback`,
    `layout_change_callback`, `text_commit_callback`; handlers are
    `handle_candidate_selection()`, `on_candidate_selected()`.
18. **Banned abbreviations** (CI-enforced): `cfg ctx mgr proc gen impl util
    repo dict cand idx cnt num str buf ptr ref tmp val res req resp args
    params info obj misc stuff helper`.
