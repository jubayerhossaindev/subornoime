#include "suborno/layouts/national/national_layout.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "suborno/unicode/bengali.hpp"

namespace suborno::layouts {
namespace {

constexpr unsigned int national_no_caps = 0x00U;

struct NationalKeyMapping {
    std::uint32_t scan_code;
    std::string_view base;  // bottom-right: স্বাভাবিক বাংলা

    std::string_view shifted;  // top-right: শিফট বাংলা

    std::string_view altgr;  // bottom-left: লিংক বাংলা

    std::string_view shift_altgr;  // top-left: লিংক শিফট বাংলা

    std::uint8_t caps_flags;
};

// Four-layer table transcribed from the National Keyboard of Bangladesh

// diagram (Figure 4 of Unicode document L2/19-079, captioned "National

// Keyboard of Bangladesh (BDS 1738:2018)") and cross-checked against the

// legacy reference IME National mapping facts. Clean-room transcription: key->character

// facts only, no GPL code.

//

// Layer notes:

//  - Shift+4 emits ৳ (U+09F3) and Shift+7 emits ঁ (U+0981): the hallmark

//    positions of the National layout.

//  - The Link (AltGr) layer carries the fraction/currency signs on the digit

//    row, the independent vowels on the vowel-sign keys, ×/÷ on the minus

//    key, and ॥ on Shift+Link+G.

//  - Link+comma and Link+period would carry the Assamese letters ৰ (U+09F0)

//    and ৱ (U+09F1) per the reference; they are deliberately left empty in

//    SubornoIME per the project's Bengali inventory decision (see

//    docs/specifications/national-keymap.md).

//  - ড়/ঢ়/য় are emitted decomposed (consonant + U+09BC) because they are

//    Unicode composition exclusions under NFC; the codebase convention keeps

//    them decomposed end to end.

constexpr NationalKeyMapping national_mappings[] = {
    {0x02, "১", "!", "৴", "\u0980", national_no_caps},
    {0x03, "২", "@", "৵", "", national_no_caps},
    {0x04, "৩", "#", "৶", "ঽ", national_no_caps},
    {0x05, "৪", "৳", "৷", "৲", national_no_caps},
    {0x06, "৫", "%", "৸", "ৄ", national_no_caps},
    {0x07, "৬", "^", "ৠ", "ৠ", national_no_caps},
    {0x08, "৭", "ঁ", "৺", "\u09FC", national_no_caps},
    {0x09, "৮", "*", "ৡ", "ৣ", national_no_caps},
    {0x0A, "৯", "(", "ঌ", "ৢ", national_no_caps},
    {0x0B, "০", ")", "৹", "৽", national_no_caps},
    {0x0C, "-", "_", "×", "÷", national_no_caps},
    {0x0D, "=", "+", "", "", national_no_caps},
    {0x10, "ঙ", "ং", "", "", national_no_caps},
    {0x11, "য", "\u09AF\u09BC", "", "", national_no_caps},
    {0x12, "ড", "ঢ", "", "", national_no_caps},
    {0x13, "প", "ফ", "", "", national_no_caps},
    {0x14, "ট", "ঠ", "", "", national_no_caps},
    {0x15, "চ", "ছ", "", "", national_no_caps},
    {0x16, "জ", "ঝ", "", "", national_no_caps},
    {0x17, "হ", "ঞ", "", "", national_no_caps},
    {0x18, "গ", "ঘ", "", "", national_no_caps},
    {0x19, "\u09A1\u09BC", "\u09A2\u09BC", "", "", national_no_caps},
    {0x1A, "[", "{", "", "", national_no_caps},
    {0x1B, "]", "}", "", "", national_no_caps},
    {0x1E, "ৃ", "র্", "ঋ", "", national_no_caps},
    {0x1F, "ু", "ূ", "উ", "ঊ", national_no_caps},
    {0x20, "ি", "ী", "ই", "ঈ", national_no_caps},
    {0x21, "া", "অ", "আ", "", national_no_caps},
    {0x22, "্", "।", "", "॥", national_no_caps},
    {0x23, "ব", "ভ", "", "", national_no_caps},
    {0x24, "ক", "খ", "", "", national_no_caps},
    {0x25, "ত", "থ", "", "", national_no_caps},
    {0x26, "দ", "ধ", "", "", national_no_caps},
    {0x27, ";", ":", "", "", national_no_caps},
    {0x28, "'", "`", "", "", national_no_caps},
    {0x29, "`", "“", "", "", national_no_caps},
    {0x2B, "ৎ", "ঃ", "", "", national_no_caps},
    {0x2C, "্র", "্য", "", "", national_no_caps},
    {0x2D, "ও", "ৗ", "ো", "ঔ", national_no_caps},
    {0x2E, "ে", "ৈ", "এ", "ঐ", national_no_caps},
    {0x2F, "র", "ল", "", "", national_no_caps},
    {0x30, "ন", "ণ", "", "", national_no_caps},
    {0x31, "স", "ষ", "", "", national_no_caps},
    {0x32, "ম", "শ", "", "", national_no_caps},
    {0x33, ",", "<", "", "", national_no_caps},
    {0x34, ".", ">", "", "", national_no_caps},
    {0x35, "/", "?", "", "", national_no_caps},
    {0x39, " ", " ", "", "", national_no_caps},
};

constexpr std::string_view virama = "\u09CD";
constexpr std::string_view zwnj = "\u200C";
constexpr std::string_view zwj = "\u200D";
constexpr std::string_view reph = "\u09B0\u09CD";  // র্

bool is_bengali_consonant(char32_t cp) noexcept {
    return (cp >= 0x0995 && cp <= 0x09B9) || cp == 0x09DC || cp == 0x09DD || cp == 0x09DF;
}

// Decodes the UTF-8 code point ending exactly at byte `end`; returns the code

// point and sets `start` to its first byte. Returns false on malformed input.

bool decode_last_code_point(std::string_view text,
                            std::size_t end,
                            char32_t& cp,
                            std::size_t& start) noexcept {
    if (end == 0 || end > text.size()) {
        return false;
    }
    std::size_t position = end - 1;
    // Walk back over continuation bytes (10xxxxxx).

    while (position > 0 && (static_cast<unsigned char>(text[position]) & 0xC0U) == 0x80U) {
        --position;
    }
    const auto byte0 = static_cast<unsigned char>(text[position]);
    std::size_t length = end - position;
    if (length == 1 && byte0 < 0x80U) {
        cp = byte0;
        start = position;
        return true;
    }
    if (length == 2 && byte0 >= 0xC0U && byte0 < 0xE0U) {
        cp = static_cast<char32_t>((static_cast<std::uint32_t>(byte0 & 0x1FU) << 6U)
                                   | (static_cast<unsigned char>(text[position + 1]) & 0x3FU));
        start = position;
        return true;
    }
    if (length == 3 && byte0 >= 0xE0U && byte0 < 0xF0U) {
        cp = static_cast<char32_t>((static_cast<std::uint32_t>(byte0 & 0x0FU) << 12U)
                                   | (static_cast<unsigned char>(text[position + 1]) & 0x3FU) << 6U
                                   | (static_cast<unsigned char>(text[position + 2]) & 0x3FU));
        start = position;
        return true;
    }
    return false;
}

bool is_reorder_sign(char32_t cp) noexcept {
    switch (cp) {
    case 0x09BE:  // া

    case 0x09BF:  // ি

    case 0x09C0:  // ী

    case 0x09C1:  // ু

    case 0x09C2:  // ূ

    case 0x09C3:  // ৃ

    case 0x09C7:  // ে

    case 0x09C8:  // ৈ

    case 0x09CB:  // ো

    case 0x09CC:  // ৌ

        return true;
    default:
        return false;
    }
}

// Length of the trailing consonant cluster in `buffer`:

//   (consonant [+nukta]) (virama consonant [+nukta])*

// The nukta allowance covers the decomposed ড়/ঢ়/য় forms (NFC keeps them as

// consonant + U+09BC). Returns 0 when the buffer does not end with a

// consonant cluster.

std::size_t trailing_consonant_cluster_length(std::string_view buffer) noexcept {
    // Consumes one consonant unit (consonant + optional nukta) ending at

    // `end`; returns the unit byte-width and sets `new_end` to the unit's

    // first byte. Returns 0 when no consonant unit is present.

    const auto consume_consonant_unit = [&buffer](std::size_t end,
                                                  std::size_t& new_end) -> std::size_t {
        char32_t cp = 0;
        std::size_t start = 0;
        if (end == 0 || !decode_last_code_point(buffer, end, cp, start)) {
            return 0;
        }
        const std::size_t width = end - start;
        if (cp == 0x09BC) {
            const std::size_t nukta_end = start;
            char32_t base_cp = 0;
            std::size_t base_start = 0;
            if (nukta_end == 0 || !decode_last_code_point(buffer, nukta_end, base_cp, base_start)
                || !is_bengali_consonant(base_cp)) {
                return 0;
            }
            new_end = base_start;
            return width + (nukta_end - base_start);
        }
        if (!is_bengali_consonant(cp)) {
            return 0;
        }
        new_end = start;
        return width;
    };

    std::size_t end = buffer.size();
    std::size_t new_end = 0;
    const std::size_t first = consume_consonant_unit(end, new_end);
    if (first == 0) {
        return 0;
    }
    std::size_t total = first;
    end = new_end;
    while (end > 0) {
        char32_t cp = 0;
        std::size_t start = 0;
        if (!decode_last_code_point(buffer, end, cp, start) || cp != 0x09CD || end - start != 3) {
            break;
        }
        const std::size_t unit = consume_consonant_unit(start, new_end);
        if (unit == 0) {
            break;
        }
        total += 3 + unit;
        end = new_end;
    }
    return total;
}

bool ends_with(std::string_view text, std::string_view suffix) noexcept {
    return text.size() >= suffix.size()
           && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// Applies one National key output to the composition buffer, including the

// modern-standard sequence behaviours:

//  - Shift+A (reph): র্ is placed before the trailing consonant cluster.

//  - Second hasanta press: ্ becomes ্ (virama + ZWNJ), the broken conjunct.

//  - া after ঁ: canonical order াঁ.

//  - া after অ: অ+া-kar is invalid; the pair becomes the letter আ.

//  - ্র/্য after a dependent vowel sign: ra-kar / ya-phala reorder.

//  - ্য after র: the র‍্য ya-phala sequence (ZWJ keeps the conjunct explicit).

void apply_national_output(std::string& buffer, std::string_view output, bool reph_key) {
    if (output.empty()) {
        return;
    }

    if (reph_key) {
        const std::size_t cluster = trailing_consonant_cluster_length(buffer);
        if (cluster > 0) {
            buffer.insert(buffer.size() - cluster, reph);
        } else {
            buffer.append(reph);
        }
        return;
    }

    if (output == virama && ends_with(buffer, virama)) {
        // Double hasanta: ্ + ZWNJ keeps the following consonant unjoined.

        buffer.append(zwnj);
        return;
    }

    if (output == "া") {
        if (ends_with(buffer, "ঁ")) {
            buffer.resize(buffer.size() - 3);  // ঁ is a 3-byte code point

            buffer.append("াঁ");
            return;
        }
        if (ends_with(buffer, "অ")) {
            buffer.resize(buffer.size() - 3);
            buffer.append("আ");
            return;
        }
    }

    if (output == "্য" || output == "্র") {
        if (output == "্য" && ends_with(buffer, "র")) {
            buffer.append(zwj);
            buffer.append(output);
            return;
        }
        char32_t last_cp = 0;
        std::size_t last_start = 0;
        if (!buffer.empty() && decode_last_code_point(buffer, buffer.size(), last_cp, last_start)
            && is_reorder_sign(last_cp)) {
            const std::size_t sign_width = buffer.size() - last_start;
            const std::string sign = buffer.substr(last_start, sign_width);
            buffer.resize(last_start);
            buffer.append(output);
            buffer.append(sign);
            return;
        }
    }

    buffer.append(output);
}

const NationalKeyMapping* find_mapping(std::uint32_t scan_code) noexcept {
    for (const NationalKeyMapping& mapping : national_mappings) {
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

std::string_view output_for_event(const layout::KeyEvent& event) {
    if (!event.key_down || event.modifiers.windows
        || (event.modifiers.control != event.modifiers.alt)) {
        // Windows key, Ctrl alone, or Alt alone: never inject text. AltGr

        // arrives as Ctrl+Alt together and is handled below.

        return {};
    }

    const NationalKeyMapping* mapping = find_mapping(event.scan_code);
    if (mapping == nullptr) {
        return {};
    }

    if (event.modifiers.control && event.modifiers.alt) {
        // Link (AltGr) layers per the diagram legend; CapsLock does not

        // affect them.

        return event.modifiers.shift ? mapping->shift_altgr : mapping->altgr;
    }

    // CapsLock does not modify the National Bangla layers (the layout has no

    // caps-sensitive keys); Shift alone selects the second layer.

    return event.modifiers.shift ? mapping->shifted : mapping->base;
}

std::string transliterate_national(std::string_view input) {
    if (!suborno::unicode::is_valid_utf8(input)) {
        return {};
    }

    std::string buffer;
    buffer.reserve(input.size() * 3U);
    for (const char value : input) {
        layout::KeyEvent event;
        if (!ascii_key_to_event(value, event)) {
            buffer.push_back(value);
            continue;
        }
        const std::string_view output = output_for_event(event);
        // The reph key is exactly the Shift layer of scan 0x1E, whose table

        // value is র্; deriving the flag from the output keeps the session

        // and the string API consistent.

        const bool reph_key = output == reph;
        apply_national_output(buffer, output, reph_key);
    }
    return suborno::unicode::normalize_nfc(buffer);
}

class NationalInputSession final : public layout::InputSession {
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
                const std::string_view output = output_for_event(event);
                const bool reph_key = output == reph;
                apply_national_output(updated, output, reph_key);
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
        const std::string_view output = output_for_event(event);
        if (output.empty()) {
            return layout::InputStatus::empty;
        }
        const std::size_t repeat_count = event.repeat_count == 0 ? 1 : event.repeat_count;
        if (repeat_count > layout::InputSession::max_input_code_points) {
            return layout::InputStatus::input_limit;
        }

        const bool reph_key = output == reph;
        std::string updated = composition_;
        for (std::size_t index = 0; index < repeat_count; ++index) {
            apply_national_output(updated, output, reph_key);
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
        // Key 0x11/0x19 emit decomposed য়/ড়/ঢ় forms; commit must produce the

        // same normalized text as NationalLayout::transliterate does.

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

std::string NationalLayout::transliterate(const std::string& input) const {
    return transliterate_national(input);
}

std::vector<layout::Candidate> NationalLayout::generate_candidates(const std::string& input) const {
    const std::string output = transliterate_national(input);
    if (output.empty()) {
        return {};
    }
    return {{
        .text = output,
        .normalized_text = output,
        .stable_id = "layout:national:" + output,
        .source = layout::CandidateSource::layout,
        .score = 1.0,
        .rationale = "National positional-key output",
    }};
}

std::unique_ptr<layout::InputSession> NationalLayout::create_session(
    dictionary::Dictionary*, layout::InputSessionOptions) const {
    return std::make_unique<NationalInputSession>();
}

}  // namespace suborno::layouts
