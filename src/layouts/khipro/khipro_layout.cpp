#include "suborno/layouts/khipro/khipro_layout.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "borno_compat_layouts.hpp"
#include "suborno/candidates/candidate_engine.hpp"
#include "suborno/dictionary/dictionary.hpp"
#include "suborno/layouts/layout_input_session.hpp"
#include "suborno/unicode/bengali.hpp"

namespace suborno::layouts {
namespace {

// Khipro typing delegates to the Khipro compatibility engine
// (src/layouts/borno_compat: Suborno implementation of the GPL-3.0-or-later
// Khipro .mim S-expression interpreter over the canonical bn-khipro.mim
// spec shipped at assets/layouts/bn-khipro.mim). The engine lazily loads
// the spec using the Suborno path search (Suborno user/program-data paths
// first, the legacy BornoTSF paths second, the bundled asset last) and
// falls back to pass-through when no spec file can be found.
borno_compat::KhiproCompatEngine& khipro_compat_engine() {
    static borno_compat::KhiproCompatEngine engine;
    return engine;
}

class KhiproInputSession final : public layout::InputSession {
  public:
    KhiproInputSession(const layout::InputLayout& input_layout,
                       dictionary::Dictionary* dictionary,
                       layout::InputSessionOptions options) {
        if (dictionary != nullptr) {
            state_ = std::make_unique<LayoutInputSession>(input_layout, *dictionary, options);
        } else {
            state_ = std::make_unique<LayoutInputSession>(input_layout, options);
        }
    }

    layout::InputStatus append_utf8(std::string_view input) override {
        return state_->append_utf8(input);
    }
    layout::InputStatus append_key(const layout::KeyEvent& event) override {
        return state_->append_key(event);
    }
    layout::InputStatus backspace() override {
        return state_->backspace();
    }
    layout::CommitResult commit() override {
        return state_->commit();
    }
    layout::CommitResult commit_candidate(std::size_t index) override {
        return state_->commit_candidate(index);
    }
    bool select_candidate(std::size_t index) override {
        return state_->select_candidate(index);
    }
    bool select_next_candidate() override {
        return state_->select_next_candidate();
    }
    bool select_previous_candidate() noexcept override {
        return state_->select_previous_candidate();
    }
    [[nodiscard]] std::size_t selected_candidate_index() const noexcept override {
        return state_->selected_candidate_index();
    }
    void reset() noexcept override {
        state_->reset();
    }
    [[nodiscard]] layout::CompositionSnapshot snapshot() const override {
        return state_->snapshot();
    }

  private:
    std::unique_ptr<LayoutInputSession> state_;
};

}  // namespace

std::string KhiproLayout::transliterate(const std::string& input) const {
    // Suborno layout contract (kept from the previous implementation): the
    // input must be valid UTF-8.
    if (!suborno::unicode::is_valid_utf8(input)) {
        throw std::invalid_argument("Input is not valid UTF-8");
    }
    return khipro_compat_engine().transliterate(input);
}

std::vector<layout::Candidate> KhiproLayout::generate_candidates(const std::string& input) const {
    // BornoTSF keeps the candidate window off for the Khipro layout (fixed
    // compositional output); a single candidate keeps Suborno's session
    // contract while preserving that behaviour.
    const std::string output = transliterate(input);
    if (output.empty()) {
        return {};
    }
    return {{
        .text = output,
        .normalized_text = output,
        .stable_id = "layout:khipro:" + output,
        .source = layout::CandidateSource::layout,
        .score = 1.0,
        .rationale = "Khipro compositional output (BornoTSF mim engine)",
    }};
}

std::unique_ptr<layout::InputSession> KhiproLayout::create_session(
    dictionary::Dictionary* dictionary, layout::InputSessionOptions options) const {
    return std::make_unique<KhiproInputSession>(*this, dictionary, options);
}

}  // namespace suborno::layouts
