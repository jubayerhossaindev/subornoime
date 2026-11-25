#pragma once

#include <memory>
#include <string>
#include <vector>

#include "suborno/layout-api/input_layout.hpp"

namespace suborno::layouts {

/// National (Jatiyo / UniBijoy) fixed positional layout.
///
/// Key->character facts follow the National Keyboard of Bangladesh diagram
/// published as Figure 4 of public Unicode Consortium document L2/19-079
/// (captioned "National Keyboard of Bangladesh (BDS 1738:2018)"), with the
/// BornoTSF project's National mapping used as an independent factual
/// cross-reference. This is a clean-room MIT implementation: only the factual
/// key assignments were taken from the GPL-3.0 reference; no GPL code was
/// copied. See docs/specifications/national-keymap.md and
/// reports/standards/BDS_NATIONAL_LAYOUT_DISCREPANCY_REPORT.md.
///
/// Four layers per key, matching the diagram legend positions:
///   base        (bottom-right, "স্বাভাবিক বাংলা")
///   shift       (top-right,     "শিফট বাংলা")
///   AltGr/Link  (bottom-left,   "লিংক বাংলা")
///   Shift+AltGr (top-left,      "লিংক শিফট বাংলা")
///
/// Sequence behaviours implemented on top of the fixed layers (modern standard
/// "Regular" typing style): reph via Shift+A, broken conjunct via a second
/// hasanta press (্‌), candrabindu/aa-kar canonical order (াঁ), ra-kar and
/// ya-phala reordering, and the র‍্য ya-phala sequence.
class NationalLayout final : public layout::InputLayout {
  public:
    [[nodiscard]] layout::LayoutKind kind() const noexcept override {
        return layout::LayoutKind::fixed;
    }

    [[nodiscard]] std::string transliterate(const std::string& input) const override;
    [[nodiscard]] std::vector<layout::Candidate> generate_candidates(
        const std::string& input) const override;
    std::unique_ptr<layout::InputSession> create_session(
        dictionary::Dictionary* dictionary, layout::InputSessionOptions options) const override;
};

}  // namespace suborno::layouts
