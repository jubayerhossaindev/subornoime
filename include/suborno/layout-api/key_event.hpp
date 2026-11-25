#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace suborno::layout {

enum class PhysicalKey : std::uint8_t {
    unknown = 0,
    letter_a = 'a',
    letter_b = 'b',
    letter_c = 'c',
    letter_d = 'd',
    letter_e = 'e',
    letter_f = 'f',
    letter_g = 'g',
    letter_h = 'h',
    letter_i = 'i',
    letter_j = 'j',
    letter_k = 'k',
    letter_l = 'l',
    letter_m = 'm',
    letter_n = 'n',
    letter_o = 'o',
    letter_p = 'p',
    letter_q = 'q',
    letter_r = 'r',
    letter_s = 's',
    letter_t = 't',
    letter_u = 'u',
    letter_v = 'v',
    letter_w = 'w',
    letter_x = 'x',
    letter_y = 'y',
    letter_z = 'z',
    digit_0 = '0',
    digit_1 = '1',
    digit_2 = '2',
    digit_3 = '3',
    digit_4 = '4',
    digit_5 = '5',
    digit_6 = '6',
    digit_7 = '7',
    digit_8 = '8',
    digit_9 = '9',
    numpad_0 = 128,
    numpad_1 = 129,
    numpad_2 = 130,
    numpad_3 = 131,
    numpad_4 = 132,
    numpad_5 = 133,
    numpad_6 = 134,
    numpad_7 = 135,
    numpad_8 = 136,
    numpad_9 = 137,
    semicolon = 160,
    equal = 161,
    comma = 162,
    minus = 163,
    period = 164,
    slash = 165,
    grave = 166,
    left_bracket = 167,
    backslash = 168,
    right_bracket = 169,
    apostrophe = 170,
    space = 171,
    // Editing and navigation keys never map to text. They travel through
    // KeyEvent so the TSF/standalone layers can route them (commit, cancel,
    // candidate navigation) while every InputSession answers them with
    // InputStatus::empty and leaves the composition untouched.
    tab = 180,
    return_key = 181,
    escape = 182,
    backspace_key = 183,
    delete_key = 184,
    insert_key = 185,
    home = 186,
    end = 187,
    page_up = 188,
    page_down = 189,
    arrow_left = 190,
    arrow_up = 191,
    arrow_right = 192,
    arrow_down = 193,
};

// Backspace and Delete edit the composition itself. The session exposes
// backspace() explicitly; Delete is a caret-side edit handled by the host
// application, so sessions only report that the key was not text.
[[nodiscard]] constexpr bool is_editing_key(PhysicalKey key) noexcept {
    return key == PhysicalKey::backspace_key || key == PhysicalKey::delete_key;
}

// Arrows, Home/End and PageUp/PageDown move the caret or navigate the
// candidate list. Composing text stays valid while they are pressed.
[[nodiscard]] constexpr bool is_navigation_key(PhysicalKey key) noexcept {
    switch (key) {
    case PhysicalKey::arrow_left:
    case PhysicalKey::arrow_up:
    case PhysicalKey::arrow_right:
    case PhysicalKey::arrow_down:
    case PhysicalKey::home:
    case PhysicalKey::end:
    case PhysicalKey::page_up:
    case PhysicalKey::page_down:
        return true;
    default:
        return false;
    }
}

// Enter commits, Escape cancels, Tab may cycle candidates: all three carry
// session-level meaning but never insert text. Returning to the default
// append_key with these keys must neither grow nor corrupt the composition.
[[nodiscard]] constexpr bool is_action_key(PhysicalKey key) noexcept {
    return key == PhysicalKey::return_key || key == PhysicalKey::escape || key == PhysicalKey::tab;
}

struct KeyModifiers {
    bool shift = false;
    bool caps_lock = false;
    bool num_lock = false;
    bool control = false;
    bool alt = false;
    bool windows = false;
};

struct KeyMessageData {
    std::uint16_t repeat_count = 1;
    std::uint8_t scan_code = 0;
    bool extended_key = false;
    bool previous_key_state = false;
    bool key_down = true;
};

[[nodiscard]] constexpr KeyMessageData decode_key_message_lparam(std::uint32_t lparam) noexcept {
    return {
        .repeat_count = static_cast<std::uint16_t>(lparam & 0xFFFFU),
        .scan_code = static_cast<std::uint8_t>((lparam >> 16U) & 0xFFU),
        .extended_key = ((lparam >> 24U) & 0x01U) != 0,
        .previous_key_state = ((lparam >> 30U) & 0x01U) != 0,
        .key_down = ((lparam >> 31U) & 0x01U) == 0,
    };
}

[[nodiscard]] constexpr bool is_numpad_key(PhysicalKey key) noexcept {
    const auto value = static_cast<std::uint8_t>(key);
    const auto first = static_cast<std::uint8_t>(PhysicalKey::numpad_0);
    const auto last = static_cast<std::uint8_t>(PhysicalKey::numpad_9);
    return value >= first && value <= last;
}

struct KeyEvent {
    PhysicalKey key = PhysicalKey::unknown;
    KeyModifiers modifiers = {};
    std::uint16_t scan_code = 0;
    bool extended_key = false;
    std::uint16_t repeat_count = 1;
    bool previous_key_state = false;
    bool key_down = true;
    bool logical_text_available = false;
    std::string logical_text_utf8 = {};
};

[[nodiscard]] constexpr bool remap_numpad_to_top_row(KeyEvent& event) noexcept {
    if (!is_numpad_key(event.key) || !event.modifiers.num_lock) {
        return false;
    }

    const auto numpad_value =
        static_cast<std::uint8_t>(event.key) - static_cast<std::uint8_t>(PhysicalKey::numpad_0);
    event.key = static_cast<PhysicalKey>(static_cast<std::uint8_t>('0') + numpad_value);
    event.scan_code =
        numpad_value == 0U
            ? static_cast<std::uint16_t>(0x0BU)
            : static_cast<std::uint16_t>(0x01U + static_cast<unsigned int>(numpad_value));
    event.modifiers.shift = false;
    event.modifiers.caps_lock = false;
    event.logical_text_available = false;
    event.logical_text_utf8.clear();
    return true;
}

[[nodiscard]] inline std::optional<char> key_event_to_ascii(const KeyEvent& event) noexcept {
    if (event.modifiers.control || event.modifiers.alt || event.modifiers.windows) {
        return std::nullopt;
    }

    const auto value = static_cast<std::uint8_t>(event.key);
    const auto first_letter = static_cast<std::uint8_t>(PhysicalKey::letter_a);
    const auto last_letter = static_cast<std::uint8_t>(PhysicalKey::letter_z);
    if (value >= first_letter && value <= last_letter) {
        const char lower = static_cast<char>(value);
        const bool uppercase = event.modifiers.shift != event.modifiers.caps_lock;
        return uppercase ? static_cast<char>(lower - ('a' - 'A')) : lower;
    }

    const auto first_digit = static_cast<std::uint8_t>(PhysicalKey::digit_0);
    const auto last_digit = static_cast<std::uint8_t>(PhysicalKey::digit_9);
    if (value >= first_digit && value <= last_digit) {
        constexpr char unshifted[] = "0123456789";
        constexpr char shifted[] = ")!@#$%^&*(";
        const std::size_t index = static_cast<std::size_t>(value - first_digit);
        return event.modifiers.shift ? shifted[index] : unshifted[index];
    }

    const auto first_numpad = static_cast<std::uint8_t>(PhysicalKey::numpad_0);
    const auto last_numpad = static_cast<std::uint8_t>(PhysicalKey::numpad_9);
    if (value >= first_numpad && value <= last_numpad) {
        if (!event.modifiers.num_lock) {
            return std::nullopt;
        }
        return static_cast<char>('0' + (value - first_numpad));
    }

    const bool shift = event.modifiers.shift;
    switch (event.key) {
    case PhysicalKey::semicolon:
        return shift ? ':' : ';';
    case PhysicalKey::equal:
        return shift ? '+' : '=';
    case PhysicalKey::comma:
        return shift ? '<' : ',';
    case PhysicalKey::minus:
        return shift ? '_' : '-';
    case PhysicalKey::period:
        return shift ? '>' : '.';
    case PhysicalKey::slash:
        return shift ? '?' : '/';
    case PhysicalKey::grave:
        return shift ? '~' : '`';
    case PhysicalKey::left_bracket:
        return shift ? '{' : '[';
    case PhysicalKey::backslash:
        return shift ? '|' : '\\';
    case PhysicalKey::right_bracket:
        return shift ? '}' : ']';
    case PhysicalKey::apostrophe:
        return shift ? '"' : '\'';
    case PhysicalKey::space:
        return ' ';
    default:
        return std::nullopt;
    }
}

}  // namespace suborno::layout
