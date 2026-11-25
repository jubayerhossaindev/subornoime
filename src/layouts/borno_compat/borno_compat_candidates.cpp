// SPDX-License-Identifier: MIT
// Candidate construction replicating BornoTSF
// composition/KeyHandler.cpp::_HandleCompositionInputWorker (GPL context;
// this glue logic is Suborno's own MIT implementation of the same sequence).
#include "borno_compat_layouts.hpp"

namespace borno_compat {

std::vector<suborno::layout::Candidate> build_phonetic_candidates(const std::string& main_output,
                                                                  const std::string& normal_parsed,
                                                                  const std::string& raw_keys,
                                                                  std::string_view profile_id) {
    std::vector<suborno::layout::Candidate> candidates;
    auto push = [&](const std::string& text, const char* rationale) {
        if (text.empty()) {
            return;
        }
        for (const suborno::layout::Candidate& existing : candidates) {
            if (existing.text == text) {
                return;
            }
        }
        candidates.push_back(suborno::layout::Candidate{
            .text = text,
            .normalized_text = text,
            .stable_id = std::string(profile_id) + ":" + text,
            .source = suborno::layout::CandidateSource::layout,
            .score = 1.0,
            .rationale = rationale,
        });
    };

    push(main_output, "borno phonetic output");
    if (!main_output.empty()) {
        const bool auto_correct_applied = (!normal_parsed.empty() && main_output != normal_parsed);
        if (auto_correct_applied) {
            push(normal_parsed, "parse without autocorrect");
            if (!raw_keys.empty() && raw_keys != normal_parsed && raw_keys != main_output) {
                push(raw_keys, "raw keys");
            }
        } else if (!raw_keys.empty() && raw_keys != main_output) {
            push(raw_keys, "raw keys");
        }
    }
    return candidates;
}

}  // namespace borno_compat
