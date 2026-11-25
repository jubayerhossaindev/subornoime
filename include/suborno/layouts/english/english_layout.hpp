#pragma once

#include <memory>
#include <string>
#include <vector>

#include "suborno/layout-api/input_layout.hpp"

namespace suborno::layouts {

/// English passthrough "layout": transliteration is the identity mapping so
/// sessions created for it echo Latin text back, but the Windows text service
/// checks the layout kind first and never intercepts keys at all — the host
/// application receives the physical keyboard output untouched. It exists so
/// "english" is a first-class, selectable layout id that persists in settings
/// and shows as "English" in the topbar / tray / settings menus.
class EnglishLayout final : public layout::InputLayout {
  public:
    [[nodiscard]] layout::LayoutKind kind() const noexcept override;
    [[nodiscard]] std::string transliterate(const std::string& input) const override;
    [[nodiscard]] std::vector<layout::Candidate> generate_candidates(
        const std::string& input) const override;
    std::unique_ptr<layout::InputSession> create_session(
        dictionary::Dictionary* dictionary, layout::InputSessionOptions options) const override;
};

}  // namespace suborno::layouts
