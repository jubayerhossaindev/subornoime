// SPDX-License-Identifier: MIT
// Suborno-facing facade over the GPL BornoTSF compatibility engines. The
// candidate-list construction replicates BornoTSF
// composition/KeyHandler.cpp::_HandleCompositionInputWorker semantics:
// [autocorrect output, no-autocorrect parse, raw keys], de-duplicated,
// extras only when the first output is non-empty.
#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "borno_compat_settings.hpp"
#include "suborno/layout-api/candidate.hpp"

namespace borno_compat {

class BornoPhoneticCompatEngine {
  public:
    BornoPhoneticCompatEngine();
    [[nodiscard]] std::string transliterate(const std::string& input);
};

class AvroCompatEngine {
  public:
    AvroCompatEngine();
    [[nodiscard]] std::string transliterate(const std::string& input);
};

class KhiproCompatEngine {
  public:
    KhiproCompatEngine();
    [[nodiscard]] std::string transliterate(const std::string& input);
};

// Applies the temporary "without autocorrect" parse used by BornoTSF's
// ApplyRulesWithoutAutoCorrect: disables phonetic correction, runs `parse`,
// then restores the previous switch. Not thread-isolated (process-global
// switch), matching BornoTSF's own global Settings behaviour.
template <typename Parse>
std::string parse_without_autocorrect(Parse&& parse, const std::string& input) {
    const bool previous = get_phonetic_correction();
    set_phonetic_correction(false);
    std::string output;
    try {
        output = parse(input);
    } catch (...) {
        set_phonetic_correction(previous);
        throw;
    }
    set_phonetic_correction(previous);
    return output;
}

// Shared BornoTSF candidate construction for the phonetic layouts.
std::vector<suborno::layout::Candidate> build_phonetic_candidates(const std::string& main_output,
                                                                  const std::string& normal_parsed,
                                                                  const std::string& raw_keys,
                                                                  std::string_view profile_id);

}  // namespace borno_compat
