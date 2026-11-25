#include "suborno/layouts/phonetic/borno_phonetic_layout.hpp"

#include <stdexcept>
#include <utility>

#include "borno_compat_layouts.hpp"
#include "suborno/unicode/bengali.hpp"

namespace suborno::layouts {
namespace {

constexpr std::string_view profile_id = "borno-phonetic";

}  // namespace

struct BornoPhoneticLayout::PhoneticEngineState {
    borno_compat::BornoPhoneticCompatEngine engine;
};

BornoPhoneticLayout::BornoPhoneticLayout()
    : engine_(std::make_unique<PhoneticEngineState>()) {}

BornoPhoneticLayout::~BornoPhoneticLayout() = default;

std::string BornoPhoneticLayout::transliterate(const std::string& input) const {
    // Suborno layout contract (kept from the previous implementation): the

    // input must be valid UTF-8. BornoTSF's parser itself is byte-oriented

    // and never sees invalid input through TSF.

    if (!suborno::unicode::is_valid_utf8(input)) {
        throw std::invalid_argument("Input is not valid UTF-8");
    }
    return engine_->engine.transliterate(input);
}

std::vector<layout::Candidate> BornoPhoneticLayout::generate_candidates(
    const std::string& input) const {
    // BornoTSF _HandleCompositionInputWorker semantics: the autocorrect

    // output is the primary composition; the no-autocorrect parse and the

    // raw keys join the candidate window only when they differ.

    const std::string main = engine_->engine.transliterate(input);
    const std::string normal = borno_compat::parse_without_autocorrect(
        [this](const std::string& text) {
            return engine_->engine.transliterate(text);
        },
        input);
    return borno_compat::build_phonetic_candidates(main, normal, input, profile_id);
}

}  // namespace suborno::layouts
