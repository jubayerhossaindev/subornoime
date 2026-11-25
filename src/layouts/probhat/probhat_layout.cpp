#include "suborno/layouts/probhat/probhat_layout.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "suborno/unicode/bengali.hpp"

namespace suborno::layouts {
namespace {

constexpr unsigned int probhat_cap_lok = 0x01U;
constexpr unsigned int probhat_sg_caps = 0x04U;

struct ProbhatKeyMapping {
    std::uint32_t scan_code;
    std::string_view unshifted;
    std::string_view shifted;
    std::string_view altgr;
    std::string_view shift_altgr;
    std::uint8_t caps_flags;
};

// Base, Shift, CapsLock, and AltGr layers follow the MIT-licensed Probhat KLC

// table (upstream value columns 0, 1, 6 = Ctrl+Alt, 7 = Shift+Ctrl+Alt).

//

// SubornoIME Probhat Compatibility Extension (not upstream): the upstream

// layout.klc (princebillygk/probhat-layout-windows @ debc8310, verified

// 2026-11-24) contains no U+09BC standalone Bengali nukta anywhere, so

// AltGr (Ctrl+Alt) on scan 0x34 — the diacritic corner whose Shift layer is

// chandrabindu ঁ and whose neighbour 0x35 carries the virama ্ — emits the

// standalone nukta ়. The upstream KLC column for that slot is -1 (empty),

// so no existing key is overwritten. See docs/specifications/probhat-keymap.md.

constexpr ProbhatKeyMapping probhat_mappings[] = {
    {0x02, "১", "!", "¡", "¹", 0x00},
    {0x03, "২", "@", "²", "", 0x00},
    {0x04, "৩", "#", "³", "", 0x00},
    {0x05, "৪", "৳", "¤", "£", 0x00},
    {0x06, "৫", "%", "€", "", 0x00},
    {0x07, "৬", "^", "¼", "", 0x00},
    {0x08, "৭", "ঞ", "½", "", 0x00},
    {0x09, "৮", "ৎ", "¾", "", 0x00},
    {0x0A, "৯", "(", "‘", "", 0x00},
    {0x0B, "০", ")", "’", "", 0x00},
    {0x0C, "-", "_", "¥", "", 0x00},
    {0x0D, "=", "+", "×", "÷", 0x00},
    {0x10, "দ", "ধ", "ä", "Ä", 0x05},
    {0x11, "ূ", "ঊ", "å", "Å", 0x05},
    {0x12, "ী", "ঈ", "é", "É", 0x05},
    {0x13, "র", "ড়", "®", "", 0x01},
    {0x14, "ট", "ঠ", "þ", "Þ", 0x05},
    {0x15, "এ", "ঐ", "ü", "Ü", 0x05},
    {0x16, "ু", "উ", "ú", "Ú", 0x05},
    {0x17, "ি", "ই", "í", "Í", 0x05},
    {0x18, "ও", "ঔ", "ó", "Ó", 0x05},
    {0x19, "প", "ফ", "ö", "Ö", 0x05},
    {0x1A, "ে", "ৈ", "«", "", 0x00},
    {0x1B, "ো", "ৌ", "»", "", 0x00},
    {0x1E, "া", "অ", "á", "Á", 0x05},
    {0x1F, "স", "ষ", "ß", "§", 0x01},
    {0x20, "ড", "ঢ", "ð", "Ð", 0x05},
    {0x21, "ত", "থ", "", "", 0x01},
    {0x22, "গ", "ঘ", "", "", 0x01},
    {0x23, "হ", "ঃ", "", "", 0x01},
    {0x24, "জ", "ঝ", "", "", 0x01},
    {0x25, "ক", "খ", "", "", 0x01},
    {0x26, "ল", "ং", "ø", "Ø", 0x05},
    {0x27, ";", ":", "¶", "°", 0x00},
    {0x28, "'", "\"", "´", "¨", 0x00},
    {0x29, "\x60", "~", "", "", 0x00},
    {0x2B, "\\", "|", "¬", "¦", 0x00},
    {0x2C, "\u09af\u09bc", "য", "æ", "Æ", 0x05},
    {0x2D, "শ", "ঢ়", "", "", 0x01},
    {0x2E, "চ", "ছ", "©", "¢", 0x01},
    {0x2F, "আ", "ঋ", "", "", 0x01},
    {0x30, "ব", "ভ", "", "", 0x01},
    {0x31, "ন", "ণ", "ñ", "Ñ", 0x05},
    {0x32, "ম", "ঙ", "µ", "", 0x01},
    {0x33, ",", "ৃ", "ç", "Ç", probhat_sg_caps},
    {0x34, "।", "ঁ", "\u09BC", "", 0x00},
    {0x35, "্", "?", "¿", "", 0x00},
    {0x39, " ", " ", "", "", 0x00},
    {0x53, ".", ".", "", "", 0x00},
    {0x56, "\\", "|", "", "", 0x00},
};

const ProbhatKeyMapping* find_mapping(std::uint32_t scan_code) noexcept {
    for (const ProbhatKeyMapping& mapping : probhat_mappings) {
        if (mapping.scan_code == scan_code) {
            return &mapping;
        }
    }
    return nullptr;
}

bool ascii_key_to_event(char value, layout::KeyEvent& event) noexcept {
    event = {};
    event.key_down = true;

    constexpr std::array<std::uint8_t, 26> letter_scan_codes{
        0x1E, 0x30, 0x2E, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32,
        0x31, 0x18, 0x19, 0x10, 0x13, 0x1F, 0x14, 0x16, 0x2F, 0x11, 0x2D, 0x15, 0x2C};

    if (value >= 'a' && value <= 'z') {
        event.scan_code = letter_scan_codes[static_cast<std::size_t>(value - 'a')];
        return true;
    }
    if (value >= 'A' && value <= 'Z') {
        event.scan_code = letter_scan_codes[static_cast<std::size_t>(value - 'A')];
        event.modifiers.shift = true;
        return true;
    }
    if (value >= '1' && value <= '9') {
        event.scan_code = static_cast<std::uint8_t>(0x02 + (value - '1'));
        return true;
    }
    if (value == '0') {
        event.scan_code = 0x0B;
        return true;
    }

    struct PunctuationKey {
        char unshifted;
        char shifted;
        std::uint8_t scan_code;
    };
    constexpr PunctuationKey punctuation[]{
        {'1', '!', 0x02},
        {'2', '@', 0x03},
        {'3', '#', 0x04},
        {'4', static_cast<char>(36), 0x05},
        {'5', '%', 0x06},
        {'6', '^', 0x07},
        {'7', '&', 0x08},
        {'8', '*', 0x09},
        {'9', '(', 0x0A},
        {'0', ')', 0x0B},
        {'-', '_', 0x0C},
        {'=', '+', 0x0D},
        {'[', '{', 0x1A},
        {']', '}', 0x1B},
        {';', ':', 0x27},
        {'\'', '"', 0x28},
        {static_cast<char>(96), '~', 0x29},
        {'\\', '|', 0x2B},
        {',', '<', 0x33},
        {'.', '>', 0x34},
        {'/', '?', 0x35},
        {' ', ' ', 0x39},
    };
    for (const PunctuationKey& key : punctuation) {
        if (value == key.unshifted || value == key.shifted) {
            event.scan_code = key.scan_code;
            event.modifiers.shift = value == key.shifted;
            return true;
        }
    }
    return false;
}

std::string output_for_event(const layout::KeyEvent& event) {
    if (!event.key_down || event.modifiers.windows
        || (event.modifiers.control != event.modifiers.alt)) {
        // Windows key, Ctrl alone, or Alt alone: never inject text. AltGr

        // arrives as Ctrl+Alt together and is handled below.

        return {};
    }

    const ProbhatKeyMapping* mapping = find_mapping(event.scan_code);
    if (mapping == nullptr) {
        return {};
    }

    if (event.modifiers.control && event.modifiers.alt) {
        // AltGr layers follow the upstream KLC columns 7-8; CapsLock does not

        // affect them.

        return std::string(event.modifiers.shift ? mapping->shift_altgr : mapping->altgr);
    }

    const auto caps_flags = static_cast<unsigned int>(mapping->caps_flags);
    const bool caps_affects_key = (caps_flags & (probhat_cap_lok | probhat_sg_caps)) != 0;
    const bool shift_active =
        event.modifiers.shift != (caps_affects_key && event.modifiers.caps_lock);
    return std::string(shift_active ? mapping->shifted : mapping->unshifted);
}

std::string transliterate_probhat(std::string_view input) {
    if (!suborno::unicode::is_valid_utf8(input)) {
        return {};
    }

    std::string output;
    output.reserve(input.size() * 3U);
    for (const char value : input) {
        layout::KeyEvent event;
        if (ascii_key_to_event(value, event)) {
            output.append(output_for_event(event));
        } else {
            output.push_back(value);
        }
    }
    return suborno::unicode::normalize_nfc(output);
}

class ProbhatInputSession final : public layout::InputSession {
  public:
    layout::InputStatus append_utf8(std::string_view input) override {
        if (!suborno::unicode::is_valid_utf8(input)) {
            return layout::InputStatus::invalid_utf8;
        }
        if (input.empty()) {
            return layout::InputStatus::empty;
        }
        std::string updated = composition_;
        for (const char value : input) {
            layout::KeyEvent event;
            if (ascii_key_to_event(value, event)) {
                updated.append(output_for_event(event));
            } else {
                updated.push_back(value);
            }
        }
        return append_output(updated);
    }

    layout::InputStatus append_key(const layout::KeyEvent& event) override {
        // Ctrl alone, Alt alone, or the Windows key are shortcuts, never text.

        // AltGr (Ctrl+Alt together) is a text layer and must pass through.

        if (!event.key_down || event.modifiers.windows
            || (event.modifiers.control != event.modifiers.alt)) {
            return layout::InputStatus::empty;
        }
        const std::string output = output_for_event(event);
        if (output.empty()) {
            return layout::InputStatus::empty;
        }
        const std::size_t repeat_count = event.repeat_count == 0 ? 1 : event.repeat_count;
        if (repeat_count > layout::InputSession::max_input_code_points) {
            return layout::InputStatus::input_limit;
        }

        std::string updated = composition_;
        for (std::size_t index = 0; index < repeat_count; ++index) {
            updated.append(output);
        }
        return append_output(updated);
    }

    layout::InputStatus backspace() override {
        if (composition_.empty()) {
            return layout::InputStatus::empty;
        }
        const auto graphemes = suborno::unicode::split_graphemes(composition_);
        if (graphemes.empty()) {
            composition_.clear();
            return layout::InputStatus::empty;
        }
        composition_.resize(composition_.size() - graphemes.back().size());
        return composition_.empty() ? layout::InputStatus::empty : layout::InputStatus::updated;
    }

    layout::CommitResult commit() override {
        if (composition_.empty()) {
            return {.text_utf8 = {}, .status = layout::InputStatus::empty};
        }
        // Key 0x2C emits the decomposed য় (য + U+09BC); commit must produce

        // the same normalized text as ProbhatLayout::transliterate does.

        std::string committed = suborno::unicode::normalize_nfc(composition_);
        composition_.clear();
        return {.text_utf8 = std::move(committed), .status = layout::InputStatus::updated};
    }

    layout::CommitResult commit_candidate(std::size_t) override {
        return {.text_utf8 = {}, .status = layout::InputStatus::candidate_out_of_range};
    }
    bool select_candidate(std::size_t) override {
        return false;
    }
    bool select_next_candidate() override {
        return false;
    }
    bool select_previous_candidate() noexcept override {
        return false;
    }
    [[nodiscard]] std::size_t selected_candidate_index() const noexcept override {
        return 0;
    }
    void reset() noexcept override {
        composition_.clear();
    }
    [[nodiscard]] layout::CompositionSnapshot snapshot() const override {
        layout::CompositionSnapshot result;
        result.raw_input_utf8 = composition_;
        result.preedit_utf8 = composition_;
        result.status =
            composition_.empty() ? layout::InputStatus::empty : layout::InputStatus::updated;
        return result;
    }

  private:
    layout::InputStatus append_output(const std::string& updated) {
        if (suborno::unicode::count_utf8_code_points(updated)
            > layout::InputSession::max_input_code_points) {
            return layout::InputStatus::input_limit;
        }
        composition_ = updated;
        return layout::InputStatus::updated;
    }

    std::string composition_;
};

}  // namespace

std::string ProbhatLayout::transliterate(const std::string& input) const {
    return transliterate_probhat(input);
}

std::vector<layout::Candidate> ProbhatLayout::generate_candidates(const std::string& input) const {
    const std::string output = transliterate_probhat(input);
    if (output.empty()) {
        return {};
    }
    return {{
        .text = output,
        .normalized_text = output,
        .stable_id = "layout:probhat:" + output,
        .source = layout::CandidateSource::layout,
        .score = 1.0,
        .rationale = "Probhat physical-key output",
    }};
}

std::unique_ptr<layout::InputSession> ProbhatLayout::create_session(
    dictionary::Dictionary*, layout::InputSessionOptions) const {
    return std::make_unique<ProbhatInputSession>();
}

}  // namespace suborno::layouts
