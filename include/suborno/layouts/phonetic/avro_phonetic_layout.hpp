#pragma once

#include <memory>
#include <string>
#include <vector>

#include "suborno/layout-api/phonetic_layout.hpp"

namespace suborno::layouts {

/// Avro phonetic layout driven by the Avro Phonetic compatibility engine
/// (src/layouts/borno_compat — Suborno implementation of the GPL-3.0-or-later
/// cppAvroPhonetic behaviour contract, (c) 2014-2015 Muhammad Mominul Huque
/// as shipped inside BornoTSF; re-authored in V18, outputs parity-verified).
/// Typing behaviour is exactly the Avro Phonetic algorithm: the ordered
/// per-lead-letter rule lists, CorrectCase, smallO/O/Dot/R handling and the
/// ac.json autocorrect pass-through layer.
class AvroPhoneticLayout final : public layout::StatelessPhoneticLayout {
  public:
    AvroPhoneticLayout();
    ~AvroPhoneticLayout() override;

    [[nodiscard]] std::string transliterate(const std::string& input) const override;
    [[nodiscard]] std::vector<layout::Candidate> generate_candidates(
        const std::string& input) const override;

  private:
    struct PhoneticEngineState;
    std::unique_ptr<PhoneticEngineState> engine_;
};

}  // namespace suborno::layouts
