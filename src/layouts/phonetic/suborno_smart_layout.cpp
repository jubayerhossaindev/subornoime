#include "suborno/layouts/phonetic/suborno_smart_layout.hpp"

#include <stdexcept>
#include <utility>

#include "compat_layouts.hpp"
#include "suborno/unicode/bengali.hpp"

namespace suborno::layouts {
namespace {

constexpr std::string_view profile_id = "suborno-smart";

}  // namespace

struct SubornoSmartLayout::PhoneticEngineState {
    compat::SmartCompatEngine engine;
};

SubornoSmartLayout::SubornoSmartLayout()
    : engine_(std::make_unique<PhoneticEngineState>()) {}

SubornoSmartLayout::~SubornoSmartLayout() = default;

std::string SubornoSmartLayout::transliterate(const std::string& input) const {
    // Suborno layout contract (kept from the previous implementation): the

    // input must be valid UTF-8. The upstream legacy engine is byte-oriented

    // and never sees invalid input through TSF.

    if (!suborno::unicode::is_valid_utf8(input)) {
        throw std::invalid_argument("Input is not valid UTF-8");
    }
    return engine_->engine.transliterate(input);
}

std::vector<layout::Candidate> SubornoSmartLayout::generate_candidates(
    const std::string& input) const {
    // Same candidate semantics as the Suborno Classic layout: the autocorrect

    // output first, then the no-autocorrect parse and the raw keys when they

    // differ.

    const std::string main = engine_->engine.transliterate(input);
    const std::string normal = compat::parse_without_autocorrect(
        [this](const std::string& text) {
            return engine_->engine.transliterate(text);
        },
        input);
    return compat::build_phonetic_candidates(main, normal, input, profile_id);
}

}  // namespace suborno::layouts
