// SPDX-License-Identifier: MIT
// Suborno-facing facade over the GPL legacy reference IME compatibility engines. The
// candidate-list construction replicates legacy reference IME
// composition/KeyHandler.cpp::_HandleCompositionInputWorker semantics:
// [autocorrect output, no-autocorrect parse, raw keys], de-duplicated,
// extras only when the first output is non-empty.
#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "compat_settings.hpp"
#include "suborno/layout-api/candidate.hpp"

namespace compat {

class ClassicCompatEngine {
  public:
    ClassicCompatEngine();
    [[nodiscard]] std::string transliterate(const std::string& input);
};

class SmartCompatEngine {
  public:
    SmartCompatEngine();
    [[nodiscard]] std::string transliterate(const std::string& input);
};

class KhiproCompatEngine {
  public:
    KhiproCompatEngine();
    [[nodiscard]] std::string transliterate(const std::string& input);
};

// Applies the temporary "without autocorrect" parse used by legacy reference IME's
// ApplyRulesWithoutAutoCorrect: disables phonetic correction, runs `parse`,
// then restores the previous switch. Not thread-isolated (process-global
// switch), matching legacy reference IME's own global Settings behaviour.
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

// Shared legacy reference candidate construction for the phonetic layouts.
std::vector<suborno::layout::Candidate> build_phonetic_candidates(const std::string& main_output,
                                                                  const std::string& normal_parsed,
                                                                  const std::string& raw_keys,
                                                                  std::string_view profile_id);

}  // namespace compat
