# Function & Method Naming Rules

Functions are verbs; classes are nouns. A function name states what it does
and implies what it returns and how failure is reported.

## Canonical vocabulary

`create_ build_ make_ parse_ generate_ calculate_ compute_ validate_ normalize_
convert_ transform_ register_ unregister_ initialize_ shutdown_ load_ save_
read_ write_ find_ lookup_ search_ select_ remove_ clear_ reset_ update_
refresh_ commit_ cancel_ handle_`

Use one verb consistently for one kind of action. Dictionary retrieval is
always `lookup_*`/`find_*` — never a mix of `get`/`fetch`/`search` for the same
semantics.

## Return semantics

| Prefix | Contract |
|---|---|
| `find_` | Returns the object or the defined failure representation |
| `try_` | Failure is expected; the return type carries it |
| `create_` | Constructs and returns a new object |
| `load_` / `save_` | Persistence in / out (`load_corrections`, `save_user_settings`) |
| `parse_` | Builds a structured representation from input (`parse_json_corrections`) |
| `convert_` | Changes representation (`convert_utf8_to_utf16`, `convert_keystroke_text`) |
| `is_ / has_ / can_ / should_ / supports_ / contains_` | Boolean predicate (`is_guard_satisfied`, `has_remaining_content`) |

Return type, ownership and error handling must stay consistent with the verb.

## Examples from the codebase

- `generate_candidates()`, `rank_candidates()`, `select_candidate()`
- `update_composition()`, `commit_composition()`, `cancel_composition()`
- `load_dictionary()`, `save_dictionary()`, `load_user_settings()`
- `normalize_unicode_text()`, `validate_bengali_text()`
- `convert_utf8_to_utf16()`, `encode_utf32_to_utf8()`, `decode_utf8_to_utf32()`
- `initialize_text_service()`, `shutdown_text_service()`

## Constraints

- Methods are snake_case everywhere **except** COM/TSF interface overrides and
  Windows message handlers, which keep their external contract names
  (`QueryInterface`, `DoEditSession`, `OnKeyDown`, `WndProc`).
- `handle_*` / `on_*` are reserved for event handlers; plain logic must not
  use them.
- No `Impl`/`Manager`/`Helper`/`Processor`/`Utility` suffixes. Ownership and
  state go into domain-named types (`DictionaryStorage`, `OskWindowState`,
  `TopbarWindowState`, `ToastWindowState`, `PhoneticEngineState`).
- Boolean functions read as predicates: `is_composition_active()`,
  `has_dictionary_entry()`, `can_accept_input()`, `is_layout_registered()`.
