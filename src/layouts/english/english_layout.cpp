#include "suborno/layouts/english/english_layout.hpp"

#include <utility>

#include "suborno/layouts/layout_input_session.hpp"

namespace suborno::layouts {

layout::LayoutKind EnglishLayout::kind() const noexcept {
    return layout::LayoutKind::passthrough;
}

std::string EnglishLayout::transliterate(const std::string& input) const {
    return input;
}

std::vector<layout::Candidate> EnglishLayout::generate_candidates(const std::string& input) const {
    if (input.empty()) {
        return {};
    }
    return {{.text = input,
             .normalized_text = input,
             .stable_id = "english:" + input,
             .source = layout::CandidateSource::layout,
             .score = 1.0,
             .rationale = "passthrough output"}};
}

std::unique_ptr<layout::InputSession> EnglishLayout::create_session(
    dictionary::Dictionary* dictionary, layout::InputSessionOptions options) const {
    // Sessions are only created as a defensive fallback: the text service
    // treats the passthrough kind as "never eat keys", so real English typing
    // goes through the host application's own keyboard handling.
    if (dictionary != nullptr) {
        return std::make_unique<LayoutInputSession>(*this, *dictionary, std::move(options));
    }
    return std::make_unique<LayoutInputSession>(*this, std::move(options));
}

}  // namespace suborno::layouts
