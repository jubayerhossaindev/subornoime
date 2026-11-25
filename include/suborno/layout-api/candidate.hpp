#pragma once

#include <cstdint>
#include <string>

namespace suborno::layout {

enum class CandidateSource : std::uint8_t {
    layout,
    system_dictionary,
    user_dictionary,
};

struct Candidate {
    std::string text{};
    std::string normalized_text{};
    std::string stable_id{};
    CandidateSource source = CandidateSource::layout;
    double score = 0.0;
    std::string rationale{};
};

}  // namespace suborno::layout
