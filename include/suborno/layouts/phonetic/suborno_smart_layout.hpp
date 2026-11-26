#pragma once

#include <memory>
#include <string>
#include <vector>

#include "suborno/layout-api/phonetic_layout.hpp"

namespace suborno::layouts {

/// Phonetic layout driven by the Suborno Smart compatibility engine
/// (src/layouts/compat — Suborno implementation of the GPL-3.0-or-later
/// legacy reference behaviour contract (attribution in LICENSE and
/// THIRD_PARTY_NOTICES.md); re-authored in V18, outputs parity-verified).
/// Typing behaviour is exactly the Suborno Smart algorithm: the ordered
/// per-lead-letter rule lists, CorrectCase, smallO/O/Dot/R handling and the
/// ac.json autocorrect pass-through layer.
class SubornoSmartLayout final : public layout::StatelessPhoneticLayout {
  public:
    SubornoSmartLayout();
    ~SubornoSmartLayout() override;

    [[nodiscard]] std::string transliterate(const std::string& input) const override;
    [[nodiscard]] std::vector<layout::Candidate> generate_candidates(
        const std::string& input) const override;

  private:
    struct PhoneticEngineState;
    std::unique_ptr<PhoneticEngineState> engine_;
};

}  // namespace suborno::layouts
