#pragma once

#include <string>
#include <vector>

#include "suborno/layout-api/phonetic_layout.hpp"

namespace suborno::layouts {

class SubornoPhoneticLayout final : public layout::PhoneticLayout {
  public:
    [[nodiscard]] std::string transliterate(const std::string& input) const override;
    [[nodiscard]] std::vector<layout::Candidate> generate_candidates(
        const std::string& input) const override;
};

}  // namespace suborno::layouts
