#pragma once

#include <cstddef>
#include <memory>
#include <string_view>
#include <vector>

#include "suborno/layout-api/input_layout.hpp"

namespace suborno::dictionary {
class Dictionary;
}

namespace suborno::spell {
class SpellChecker;
}

namespace suborno::candidates {

// Produces the ranked candidate list shown while the user types.

//

// Stages (in insertion order, which is the on-screen order):

//   1. dictionary exact matches          (highest ranking)

//   2. next-word prediction              (only when the caller supplies the

//                                         previously committed word; score 0.85)

//   3. normalized-input prefix matches   (capped so layout variants cannot starve)

//   4. spelling suggestions              (only when the word is complete but unknown)

//   5. layout alternatives               (guaranteed at least minimum_layout_slots

//                                         slots whenever the layout produces any)

//

// Layout scores use a position-decay (1 / position) so the sequence stays

/// monotonically decreasing across sources.

class CandidateEngine {
  public:
    explicit CandidateEngine(const layout::InputLayout& layout,
                             const dictionary::Dictionary* dictionary = nullptr);
    CandidateEngine(const layout::InputLayout& layout, const dictionary::Dictionary& dictionary);
    ~CandidateEngine();

    CandidateEngine(const CandidateEngine&) = delete;
    CandidateEngine& operator=(const CandidateEngine&) = delete;

    [[nodiscard]] std::vector<layout::Candidate> make_candidates(
        std::string_view input,
        std::size_t candidate_limit = 16,
        const std::string& previous_word_normalized = {}) const;

  private:
    const layout::InputLayout& layout_;
    const dictionary::Dictionary* dictionary_ = nullptr;
    std::unique_ptr<spell::SpellChecker> spell_checker_;
};

}  // namespace suborno::candidates
