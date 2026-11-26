#pragma once

#include <memory>
#include <string>
#include <vector>

#include "suborno/layout-api/phonetic_layout.hpp"

namespace suborno::layouts {

/// Bangla phonetic layout driven by the Suborno Classic compatibility engine
/// (src/layouts/compat — Suborno implementation of the GPL-3.0-or-later
/// legacy reference behaviour contract; re-authored in V18, outputs parity-verified).
/// Typing behaviour is exactly legacy reference IME "Suborno Classic": the eleven maps
/// (symbols, initials, after-vowel, vowel signs, consonants, 193 conjuncts,
/// diacritics, punctuation, phola, reph, digits), longest-match tokenization
/// with the r/T look-ahead cases, case-sensitive token handling, backslash
/// literal escaping, the trailing RemoveConsecutiveKarSigns pass and the
/// ac.json autocorrect/pass-through layer.
class SubornoClassicLayout final : public layout::StatelessPhoneticLayout {
  public:
    SubornoClassicLayout();
    ~SubornoClassicLayout() override;

    [[nodiscard]] std::string transliterate(const std::string& input) const override;
    [[nodiscard]] std::vector<layout::Candidate> generate_candidates(
        const std::string& input) const override;

  private:
    struct PhoneticEngineState;
    std::unique_ptr<PhoneticEngineState> engine_;
};

}  // namespace suborno::layouts
