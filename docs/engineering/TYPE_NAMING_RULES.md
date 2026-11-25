# Type Naming Rules

Types are nouns that state their responsibility. The name must make the role of
the type obvious to a new developer.

## Rules

1. **PascalCase** for every class, struct, enum type, type alias, template type
   parameter and concept.
2. **Noun or noun phrase** describing responsibility:
   `PhoneticParser`, `CompositionEngine`, `CompositionSession`,
   `CandidateGenerator`, `CandidateRanker`, `DictionaryRepository`,
   `LayoutRegistry`, `LayoutDefinition`, `UnicodeConverter`,
   `SettingsRepository`, `WindowsTextServiceAdapter`, `CandidateResult`,
   `InputEvent`, `CompositionState`, `DictionaryEntry`, `LayoutMetadata`,
   `DictionaryStorage`, `OskWindowState`.
3. **Banned type names**: `Manager`, `Handler`, `Helper`, `Processor`,
   `Utility`, `Data`, `Info`, `Object`, `Controller`. If a generic name seems
   unavoidable, it must state its concrete responsibility instead.
4. **No `I` prefix for interfaces.** Use a responsibility name:
   `LayoutProvider`, `DictionaryReader`, `CandidateGeneratorInterface`.
   Existing external interfaces keep their published names.
5. **Enum values are snake_case** and read as facts: `TokenClass::word_start /
   consonant / vowel / mark / other`; `StepKind::insert_text / erase_text /
   seek_cursor / switch_state / choose_branch`; `GuardKind::always_true /
   equals_value / both_true / either_true`.
6. **Pimpl / internal state types** carry the owning concept:
   `Dictionary::DictionaryStorage`, `OskWindow::OskWindowState`,
   `Topbar::TopbarWindowState`, `ToastWindow::ToastWindowState`,
   `BornoPhoneticLayout::PhoneticEngineState` — never a bare `Impl`.
7. **Enum classes only** (`enum class`); no unscoped enums in new code.
8. **Templates** use descriptive parameters:
   `template <typename ElementType, typename AllocatorType>`,
   `template <std::size_t MaximumCandidateCount>` — never `T`, `U`, `N` in
   application code.
9. **Type aliases** express intent: `using CandidateList =
   std::vector<CandidateResult>;`, `using RuleList = std::vector<Rule>;`.
10. External type names (Windows SDK, COM, ICU, SQLite) are never renamed.
