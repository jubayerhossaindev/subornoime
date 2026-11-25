#include "suborno/layouts/phonetic/avro_phonetic_layout.hpp"

#include <stdexcept>
#include <utility>

#include "borno_compat_layouts.hpp"
#include "suborno/unicode/bengali.hpp"

namespace suborno::layouts {
namespace {

constexpr std::string_view profile_id = "avro-phonetic";

}  // namespace

struct AvroPhoneticLayout::PhoneticEngineState {
    borno_compat::AvroCompatEngine engine;
};

AvroPhoneticLayout::AvroPhoneticLayout()
    : engine_(std::make_unique<PhoneticEngineState>()) {}

AvroPhoneticLayout::~AvroPhoneticLayout() = default;

std::string AvroPhoneticLayout::transliterate(const std::string& input) const {
    // Suborno layout contract (kept from the previous implementation): the

    // input must be valid UTF-8. The upstream Avro engine is byte-oriented

    // and never sees invalid input through TSF.

    if (!suborno::unicode::is_valid_utf8(input)) {
        throw std::invalid_argument("Input is not valid UTF-8");
    }
    return engine_->engine.transliterate(input);
}

std::vector<layout::Candidate> AvroPhoneticLayout::generate_candidates(
    const std::string& input) const {
    // Same candidate semantics as the Borno Phonetic layout: the autocorrect

    // output first, then the no-autocorrect parse and the raw keys when they

    // differ.

    const std::string main = engine_->engine.transliterate(input);
    const std::string normal = borno_compat::parse_without_autocorrect(
        [this](const std::string& text) {
            return engine_->engine.transliterate(text);
        },
        input);
    return borno_compat::build_phonetic_candidates(main, normal, input, profile_id);
}

}  // namespace suborno::layouts
