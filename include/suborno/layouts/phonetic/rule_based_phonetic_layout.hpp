#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>

#include "suborno/layout-api/phonetic_layout.hpp"

namespace suborno::layouts::phonetic {

enum class PhoneticTokenKind : std::uint8_t {
    consonant,
    vowel,
    sign,
    halant_prefix,
};

struct PhoneticTokenDefinition {
    std::string_view input;
    PhoneticTokenKind kind;
    std::string_view output;
    /// For vowels this is the dependent sign; for consonants it is the form
    /// used after an existing consonant run (for example, ya-phala).
    std::string_view dependent_output;
    bool nukta_eligible = true;
    bool force_independent_form = false;
    bool force_dependent_form = false;
};

/// A small immutable token table drives a complete, independent phonetic parser.
/// The table must have static lifetime and contain unique, non-empty input tokens.
class RuleBasedPhoneticLayout : public layout::StatelessPhoneticLayout {
  public:
    explicit RuleBasedPhoneticLayout(std::span<const PhoneticTokenDefinition> tokens,
                                     bool accent_key_enabled = false,
                                     std::string_view profile_id = "rule-phonetic");

    [[nodiscard]] std::string transliterate(const std::string& input) const override;
    [[nodiscard]] std::vector<layout::Candidate> generate_candidates(
        const std::string& input) const override;

  private:
    // Non-owning view of the caller-provided token table. The caller must
    // keep the table alive for the whole lifetime of this layout object;
    // the class contract above requires a static-lifetime table.
    std::span<const PhoneticTokenDefinition> tokens_;
    bool accent_key_enabled_ = false;
    // Owned copy of the profile identifier: profile identifiers may come from
    // short-lived caller strings, so the layout stores its own value.
    std::string profile_id_;
};

}  // namespace suborno::layouts::phonetic
