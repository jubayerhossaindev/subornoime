#include "suborno/settings/settings.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <limits>
#include <sstream>
#include <string_view>
#include <system_error>
#include <unordered_set>
#include <utility>

#include "persistence/atomic_file.hpp"

namespace suborno::settings {
namespace {

constexpr std::size_t settings_version = 2;
constexpr std::size_t legacy_settings_version = 1;
constexpr std::size_t maximum_settings_line_length = 4096;

bool valid_value_string(const std::string& value) {
    return !value.empty() && value.find_first_of("=\r\n") == std::string::npos
           && value.find('\0') == std::string::npos;
}

bool valid_layout_id(const std::string& layout_id) {
    return valid_value_string(layout_id);
}

// Font family names may contain spaces and parentheses ("Nirmala UI",

// "Noto Sans Bengali"); an empty value selects the OS default font.

//

// Windows font faces are LF_FACESIZE (32) UTF-16 code units wide and

// legitimately carry non-ASCII names — a Bangla font's family name in a

// Bangla locale ("বাংলা ফন্ট") is a valid GDI face. The historical ASCII

// whitelist stays in force for ASCII input (so legacy files and hand-edited

// junk behave exactly as before), while non-ASCII code points are accepted

// when the whole name is valid UTF-8, printable, and fits the GDI face

// buffer: at most 32 code points. `settings` is a layering leaf, so the

// decoder is local rather than a suborno::unicode dependency.

constexpr std::size_t maximum_font_face_code_points = 32;  // LF_FACESIZE - 1

bool decode_utf8_code_point(const std::string& text, std::size_t& index, char32_t& code_point) {
    const auto byte_at = [&text, &index](std::size_t offset) {
        return static_cast<unsigned char>(text[index + offset]);
    };
    const unsigned char lead = byte_at(0);
    std::size_t continuation_count = 0;
    char32_t decoded = 0;
    if (lead < 0x80) {
        code_point = lead;
        index += 1;
        return true;
    } else if ((lead & 0xE0) == 0xC0) {
        continuation_count = 1;
        decoded = lead & 0x1Fu;
    } else if ((lead & 0xF0) == 0xE0) {
        continuation_count = 2;
        decoded = lead & 0x0Fu;
    } else if ((lead & 0xF8) == 0xF0) {
        continuation_count = 3;
        decoded = lead & 0x07u;
    } else {
        return false;  // stray continuation byte or invalid 0xF8..0xFF lead
    }
    // The sequence needs continuation_count + 1 bytes in total.

    if (index + continuation_count >= text.size()) {
        return false;  // truncated sequence
    }
    for (std::size_t offset = 1; offset <= continuation_count; ++offset) {
        const unsigned char continuation = byte_at(offset);
        if ((continuation & 0xC0) != 0x80) {
            return false;
        }
        decoded = (decoded << 6) | (continuation & 0x3Fu);
    }
    // Overlong encodings, UTF-16 surrogates and out-of-range scalars are

    // all invalid UTF-8.

    if ((continuation_count == 1 && decoded < 0x80) || (continuation_count == 2 && decoded < 0x800)
        || (continuation_count == 3 && decoded < 0x10000)
        || (decoded >= 0xD800 && decoded <= 0xDFFF) || decoded > 0x10FFFF) {
        return false;
    }
    code_point = decoded;
    index += continuation_count + 1;
    return true;
}

bool valid_font_family(const std::string& family) {
    // A 32-code-point name can never exceed 128 bytes of UTF-8; this bound

    // also keeps the settings line well under the parser limit.

    if (family.size() > 4 * maximum_font_face_code_points) {
        return false;
    }
    std::size_t index = 0;
    std::size_t code_points = 0;
    while (index < family.size()) {
        char32_t code_point = 0;
        if (!decode_utf8_code_point(family, index, code_point)) {
            return false;
        }
        ++code_points;
        if (code_points > maximum_font_face_code_points) {
            return false;
        }
        if (code_point < 0xA0) {
            // ASCII / C1 range: keep the legacy whitelist. '=' would break

            // the key=value line format, C0/C1 controls and DEL are unsafe

            // in configuration text.

            const auto character = static_cast<unsigned char>(code_point);
            const bool allowed =
                (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z')
                || (character >= '0' && character <= '9') || character == ' ' || character == '-'
                || character == '_' || character == '.' || character == '(' || character == ')';
            if (!allowed) {
                return false;
            }
        }
        // Non-ASCII printable code points (>= 0xA0) are accepted as-is.
    }
    return true;
}

bool valid_ui_mode(const std::string& mode) {
    return mode == "topbar" || mode == "tray" || mode == "window";
}

// UI language: "system" follows the Windows display language, or an

// explicit locale shipped under data/i18n. Unknown values fail validation

// so a hand-edited file cannot silently switch the shell to English.

bool valid_ui_language(const std::string& language) {
    return language == "system" || language == "en" || language == "bn";
}

bool valid_theme_id(const std::string& theme_id) {
    if (!valid_value_string(theme_id)) {
        return false;
    }
    for (const char raw_character : theme_id) {
        const auto character = static_cast<unsigned char>(raw_character);
        const bool allowed = (character >= 'a' && character <= 'z')
                             || (character >= '0' && character <= '9') || character == '-'
                             || character == '_';
        if (!allowed) {
            return false;
        }
    }
    return true;
}

bool parse_uint(std::string_view value, std::size_t& result) {
    if (value.empty()) {
        return false;
    }

    std::size_t parsed = 0;
    for (const char character : value) {
        if (character < '0' || character > '9') {
            return false;
        }
        const std::size_t digit = static_cast<std::size_t>(character - '0');
        if (parsed > (std::numeric_limits<std::size_t>::max() - digit) / 10) {
            return false;
        }
        parsed = parsed * 10 + digit;
    }

    result = parsed;
    return true;
}

bool parse_int(std::string_view value, std::int32_t& result) {
    const bool negative = !value.empty() && value.front() == '-';
    std::string_view digits = negative ? value.substr(1) : value;
    std::size_t parsed = 0;
    if (!parse_uint(digits, parsed)) {
        return false;
    }
    if (parsed > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())) {
        return false;
    }
    result = negative ? -static_cast<std::int32_t>(parsed) : static_cast<std::int32_t>(parsed);
    return true;
}

bool parse_bool(std::string_view value, bool& result) {
    if (value == "true") {
        result = true;
        return true;
    }
    if (value == "false") {
        result = false;
        return true;
    }
    return false;
}

std::string_view start_mode_name(StartMode value) {
    switch (value) {
    case StartMode::last_state:
        return "last-state";
    case StartMode::topbar:
        return "topbar";
    case StartMode::system_tray:
        return "system-tray";
    case StartMode::main_window:
        return "main-window";
    }
    return {};
}

bool parse_start_mode(std::string_view value, StartMode& result) {
    if (value == "last-state") {
        result = StartMode::last_state;
    } else if (value == "topbar") {
        result = StartMode::topbar;
    } else if (value == "system-tray") {
        result = StartMode::system_tray;
    } else if (value == "main-window") {
        result = StartMode::main_window;
    } else {
        return false;
    }
    return true;
}

std::string_view switching_mode_name(SwitchingMode value) {
    switch (value) {
    case SwitchingMode::per_application_window:
        return "per-application-window";
    case SwitchingMode::global:
        return "global";
    }
    return {};
}

bool parse_switching_mode(std::string_view value, SwitchingMode& result) {
    if (value == "per-application-window") {
        result = SwitchingMode::per_application_window;
    } else if (value == "global") {
        result = SwitchingMode::global;
    } else {
        return false;
    }
    return true;
}

std::string_view orientation_name(CandidateOrientation value) {
    switch (value) {
    case CandidateOrientation::vertical:
        return "vertical";
    case CandidateOrientation::horizontal:
        return "horizontal";
    }
    return {};
}

bool parse_orientation(std::string_view value, CandidateOrientation& result) {
    if (value == "vertical") {
        result = CandidateOrientation::vertical;
    } else if (value == "horizontal") {
        result = CandidateOrientation::horizontal;
    } else {
        return false;
    }
    return true;
}

std::string_view encoding_name(TextEncoding value) {
    switch (value) {
    case TextEncoding::unicode:
        return "unicode";
    case TextEncoding::ansi:
        return "ansi";
    }
    return {};
}

bool parse_encoding(std::string_view value, TextEncoding& result) {
    if (value == "unicode") {
        result = TextEncoding::unicode;
    } else if (value == "ansi") {
        result = TextEncoding::ansi;
    } else {
        return false;
    }
    return true;
}

bool valid_switching_shortcut(const std::string& shortcut) {
    constexpr std::array<std::string_view, 9> allowed_shortcuts{
        "F1",
        "F2",
        "F3",
        "F4",
        "F10",
        "F12",
        "Ctrl+Space",
        "Alt+Shift",
        "Win+Space",
    };
    for (const std::string_view allowed : allowed_shortcuts) {
        if (shortcut == allowed) {
            return true;
        }
    }
    return false;
}

bool validate(const EngineSettings& settings) {
    return valid_layout_id(settings.active_layout_id) && settings.candidate_limit > 0
           && settings.candidate_limit <= 100 && valid_theme_id(settings.ui.theme_id)
           && valid_font_family(settings.ui.font_family) && valid_ui_language(settings.ui.language)
           && valid_ui_mode(settings.general.last_ui_mode)
           && valid_layout_id(settings.general.last_bangla_layout_id)
           && settings.topbar.idle_opacity_percent <= 100 && settings.topbar.idle_time_seconds >= 1
           && settings.topbar.idle_time_seconds <= 3600 && settings.topbar.position_x >= -32768
           && settings.topbar.position_x <= 32767 && settings.topbar.position_y >= -32768
           && settings.topbar.position_y <= 32767
           && valid_switching_shortcut(settings.input.layout_switch_shortcut)
           && !start_mode_name(settings.general.start_mode).empty()
           && !switching_mode_name(settings.general.switching_mode).empty()
           && !orientation_name(settings.ui.candidate_orientation).empty()
           && !encoding_name(settings.input.preferred_encoding).empty()
           && (settings.typing_style == "normal" || settings.typing_style == "traditional");
}

bool parse_settings_value(EngineSettings& settings,
                          const std::string& key,
                          const std::string& value) {
    std::size_t numeric_value = 0;

    if (key == "active_layout") {
        if (!valid_layout_id(value)) {
            return false;
        }
        settings.active_layout_id = value;
    } else if (key == "candidate_limit") {
        if (!parse_uint(value, numeric_value) || numeric_value == 0 || numeric_value > 100) {
            return false;
        }
        settings.candidate_limit = numeric_value;
    } else if (key == "learning_enabled") {
        if (!parse_bool(value, settings.learning_enabled)) {
            return false;
        }
    } else if (key == "phonetic_correction") {
        if (!parse_bool(value, settings.phonetic_correction)) {
            return false;
        }
    } else if (key == "typing_style") {
        if (value != "normal" && value != "traditional") {
            return false;
        }
        settings.typing_style = value;
    } else if (key == "remember_last_input_layout") {
        if (!parse_bool(value, settings.general.remember_last_input_layout)) {
            return false;
        }
    } else if (key == "last_ui_mode") {
        if (!valid_ui_mode(value)) {
            return false;
        }
        settings.general.last_ui_mode = value;
    } else if (key == "last_bangla_layout_id") {
        if (!valid_layout_id(value)) {
            return false;
        }
        settings.general.last_bangla_layout_id = value;
    } else if (key == "start_with_windows") {
        if (!parse_bool(value, settings.general.start_with_windows)) {
            return false;
        }
    } else if (key == "start_mode") {
        if (!parse_start_mode(value, settings.general.start_mode)) {
            return false;
        }
    } else if (key == "switching_mode") {
        if (!parse_switching_mode(value, settings.general.switching_mode)) {
            return false;
        }
    } else if (key == "theme_id") {
        if (!valid_theme_id(value)) {
            return false;
        }
        settings.ui.theme_id = value;
    } else if (key == "font_family") {
        if (!valid_font_family(value)) {
            return false;
        }
        settings.ui.font_family = value;
    } else if (key == "language") {
        if (!valid_ui_language(value)) {
            return false;
        }
        settings.ui.language = value;
    } else if (key == "show_candidate_window") {
        if (!parse_bool(value, settings.ui.show_candidate_window)) {
            return false;
        }
    } else if (key == "next_word_prediction_enabled") {
        if (!parse_bool(value, settings.ui.next_word_prediction_enabled)) {
            return false;
        }
    } else if (key == "candidate_orientation") {
        if (!parse_orientation(value, settings.ui.candidate_orientation)) {
            return false;
        }
    } else if (key == "candidate_fixed_height") {
        if (!parse_bool(value, settings.ui.candidate_fixed_height)) {
            return false;
        }
    } else if (key == "use_os_default_bangla_font") {
        if (!parse_bool(value, settings.ui.use_os_default_bangla_font)) {
            return false;
        }
    } else if (key == "enable_sliding_effect") {
        if (!parse_bool(value, settings.ui.enable_sliding_effect)) {
            return false;
        }
    } else if (key == "follow_caret") {
        if (!parse_bool(value, settings.ui.follow_caret)) {
            return false;
        }
    } else if (key == "follow_mouse_pointer_if_caret_tracking_fails") {
        if (!parse_bool(value, settings.ui.follow_mouse_pointer_if_caret_tracking_fails)) {
            return false;
        }
    } else if (key == "idle_dimming_enabled") {
        if (!parse_bool(value, settings.topbar.idle_dimming_enabled)) {
            return false;
        }
    } else if (key == "idle_opacity_percent") {
        if (!parse_uint(value, numeric_value) || numeric_value > 100) {
            return false;
        }
        settings.topbar.idle_opacity_percent = static_cast<std::uint32_t>(numeric_value);
    } else if (key == "idle_time_seconds") {
        if (!parse_uint(value, numeric_value) || numeric_value < 1 || numeric_value > 3600) {
            return false;
        }
        settings.topbar.idle_time_seconds = static_cast<std::uint32_t>(numeric_value);
    } else if (key == "remember_topbar_position") {
        if (!parse_bool(value, settings.topbar.remember_position)) {
            return false;
        }
    } else if (key == "topbar_position_x") {
        std::int32_t parsed = 0;
        if (!parse_int(value, parsed) || parsed < -32768 || parsed > 32767) {
            return false;
        }
        settings.topbar.position_x = parsed;
    } else if (key == "topbar_position_y") {
        std::int32_t parsed = 0;
        if (!parse_int(value, parsed) || parsed < -32768 || parsed > 32767) {
            return false;
        }
        settings.topbar.position_y = parsed;
    } else if (key == "layout_switch_shortcut") {
        if (!valid_switching_shortcut(value)) {
            return false;
        }
        settings.input.layout_switch_shortcut = value;
    } else if (key == "disable_encoding_shortcut") {
        if (!parse_bool(value, settings.input.disable_encoding_shortcut)) {
            return false;
        }
    } else if (key == "map_numpad_to_bangla_numbers") {
        if (!parse_bool(value, settings.input.map_numpad_to_bangla_numbers)) {
            return false;
        }
    } else if (key == "change_locale_to_bn_bd_when_layout_not_english") {
        if (!parse_bool(value, settings.input.change_locale_to_bn_bd_when_layout_not_english)) {
            return false;
        }
    } else if (key == "ignore_caps_lock_state_in_bangla") {
        if (!parse_bool(value, settings.input.ignore_caps_lock_state_in_bangla)) {
            return false;
        }
    } else if (key == "select_candidate_using_number_keys") {
        if (!parse_bool(value, settings.input.select_candidate_using_number_keys)) {
            return false;
        }
    } else if (key == "preferred_encoding") {
        if (!parse_encoding(value, settings.input.preferred_encoding)) {
            return false;
        }
    } else {
        return false;
    }

    return true;
}

}  // namespace

SettingsStore::SettingsStore(std::filesystem::path path)
    : path_(std::move(path)) {}

SettingsLoadResult SettingsStore::load() const {
    SettingsLoadResult result;
    // Binary mode keeps '\r' bytes visible on Windows too: text-mode streams

    // translate CRLF to LF there, which silently ACCEPTED CRLF settings files

    // that the parser must reject (real MSVC run, 2026-11-25). POSIX already

    // saw the raw bytes, so this only aligns Windows with the documented

    // behaviour.

    std::ifstream input(path_, std::ios::binary);
    if (!input.is_open()) {
        // First run: no settings file yet — defaults are the correct outcome,

        // not an error. An existing file that cannot be opened (locked or

        // denied) stays a real io_error; file_existed distinguishes the two

        // so callers can report and recover precisely.

        result.file_existed = std::filesystem::exists(path_);
        result.status = result.file_existed ? SettingsStatus::io_error : SettingsStatus::ok;
        return result;
    }
    result.file_existed = true;

    EngineSettings settings;
    std::string line;
    std::size_t version = 0;
    bool has_version = false;
    bool has_layout = false;
    bool has_candidate_limit = false;
    bool has_learning = false;
    std::unordered_set<std::string> seen_keys;

    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        if (line.size() > maximum_settings_line_length) {
            result.status = SettingsStatus::invalid_format;
            result.error = "Settings line exceeds the maximum supported length";
            return result;
        }

        const std::size_t separator = line.find('=');
        if (separator == std::string::npos) {
            result.status = SettingsStatus::invalid_format;
            result.error = "Malformed settings line";
            return result;
        }
        const std::string key = line.substr(0, separator);
        const std::string value = line.substr(separator + 1);
        if (!seen_keys.insert(key).second) {
            result.status = SettingsStatus::invalid_format;
            result.error = "Duplicate settings key";
            return result;
        }
        if (key == "version") {
            std::size_t parsed = 0;
            if (!parse_uint(value, parsed)) {
                result.status = SettingsStatus::invalid_format;
                result.error = "Invalid settings version";
                return result;
            }
            version = parsed;
            has_version = true;
            continue;
        }

        if (!parse_settings_value(settings, key, value)) {
            result.status = SettingsStatus::invalid_format;
            result.error = "Invalid or unknown settings key: " + key;
            return result;
        }
        has_layout = has_layout || key == "active_layout";
        has_candidate_limit = has_candidate_limit || key == "candidate_limit";
        has_learning = has_learning || key == "learning_enabled";
    }

    if (input.bad()) {
        result.status = SettingsStatus::io_error;
        result.error = "Failed while reading settings";
        return result;
    }
    if (!has_version) {
        result.status = SettingsStatus::invalid_format;
        result.error = "Missing settings version";
        return result;
    }
    if (version != legacy_settings_version && version != settings_version) {
        result.status = SettingsStatus::unsupported_version;
        result.error = "Unsupported settings version";
        return result;
    }
    if (!has_layout || !has_candidate_limit || !has_learning) {
        result.status = SettingsStatus::invalid_format;
        result.error = "Missing required engine settings";
        return result;
    }
    if (!validate(settings)) {
        result.status = SettingsStatus::invalid_format;
        result.error = "Settings values are outside the supported range";
        return result;
    }

    result.settings = std::move(settings);
    return result;
}

SettingsStatus SettingsStore::save(const EngineSettings& settings) const {
    if (!validate(settings)) {
        return SettingsStatus::invalid_format;
    }

    std::error_code error;
    const std::filesystem::path parent = path_.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, error);
        if (error) {
            return SettingsStatus::io_error;
        }
    }

    std::ostringstream output;
    output << "version=" << settings_version << '\n'
           << "active_layout=" << settings.active_layout_id << '\n'
           << "candidate_limit=" << settings.candidate_limit << '\n'
           << "learning_enabled=" << (settings.learning_enabled ? "true" : "false") << '\n'
           << "phonetic_correction=" << (settings.phonetic_correction ? "true" : "false") << '\n'
           << "typing_style=" << settings.typing_style << '\n'
           << "remember_last_input_layout="
           << (settings.general.remember_last_input_layout ? "true" : "false") << '\n'
           << "start_with_windows=" << (settings.general.start_with_windows ? "true" : "false")
           << '\n'
           << "start_mode=" << start_mode_name(settings.general.start_mode) << '\n'
           << "switching_mode=" << switching_mode_name(settings.general.switching_mode) << '\n'
           << "last_ui_mode=" << settings.general.last_ui_mode << '\n'
           << "last_bangla_layout_id=" << settings.general.last_bangla_layout_id << '\n'
           << "theme_id=" << settings.ui.theme_id << '\n'
           << "font_family=" << settings.ui.font_family << '\n'
           << "language=" << settings.ui.language << '\n'
           << "show_candidate_window=" << (settings.ui.show_candidate_window ? "true" : "false")
           << '\n'
           << "next_word_prediction_enabled="
           << (settings.ui.next_word_prediction_enabled ? "true" : "false") << '\n'
           << "candidate_orientation=" << orientation_name(settings.ui.candidate_orientation)
           << '\n'
           << "candidate_fixed_height=" << (settings.ui.candidate_fixed_height ? "true" : "false")
           << '\n'
           << "use_os_default_bangla_font="
           << (settings.ui.use_os_default_bangla_font ? "true" : "false") << '\n'
           << "enable_sliding_effect=" << (settings.ui.enable_sliding_effect ? "true" : "false")
           << '\n'
           << "follow_caret=" << (settings.ui.follow_caret ? "true" : "false") << '\n'
           << "follow_mouse_pointer_if_caret_tracking_fails="
           << (settings.ui.follow_mouse_pointer_if_caret_tracking_fails ? "true" : "false") << '\n'
           << "idle_dimming_enabled=" << (settings.topbar.idle_dimming_enabled ? "true" : "false")
           << '\n'
           << "idle_opacity_percent=" << settings.topbar.idle_opacity_percent << '\n'
           << "idle_time_seconds=" << settings.topbar.idle_time_seconds << '\n'
           << "remember_topbar_position=" << (settings.topbar.remember_position ? "true" : "false")
           << '\n'
           << "topbar_position_x=" << settings.topbar.position_x << '\n'
           << "topbar_position_y=" << settings.topbar.position_y << '\n'
           << "layout_switch_shortcut=" << settings.input.layout_switch_shortcut << '\n'
           << "disable_encoding_shortcut="
           << (settings.input.disable_encoding_shortcut ? "true" : "false") << '\n'
           << "map_numpad_to_bangla_numbers="
           << (settings.input.map_numpad_to_bangla_numbers ? "true" : "false") << '\n'
           << "change_locale_to_bn_bd_when_layout_not_english="
           << (settings.input.change_locale_to_bn_bd_when_layout_not_english ? "true" : "false")
           << '\n'
           << "ignore_caps_lock_state_in_bangla="
           << (settings.input.ignore_caps_lock_state_in_bangla ? "true" : "false") << '\n'
           << "select_candidate_using_number_keys="
           << (settings.input.select_candidate_using_number_keys ? "true" : "false") << '\n'
           << "preferred_encoding=" << encoding_name(settings.input.preferred_encoding) << '\n';
    if (!output) {
        return SettingsStatus::io_error;
    }

    if (suborno::persistence::write_file_atomically(path_, output.str())) {
        return SettingsStatus::io_error;
    }
    return SettingsStatus::ok;
}

SettingsStore::FileStamp SettingsStore::file_stamp() const {
    FileStamp stamp;
    std::error_code error;
    const auto info = std::filesystem::status(path_, error);
    if (error || !std::filesystem::is_regular_file(info)) {
        return stamp;
    }
    const auto size = std::filesystem::file_size(path_, error);
    if (error) {
        stamp.exists = true;
        return stamp;
    }
    const auto mtime = std::filesystem::last_write_time(path_, error);
    if (error) {
        stamp.exists = true;
        stamp.size = size;
        return stamp;
    }
    stamp.exists = true;
    stamp.size = size;
    stamp.mtime_ticks =
        std::chrono::duration_cast<std::chrono::nanoseconds>(mtime.time_since_epoch()).count();
    return stamp;
}

}  // namespace suborno::settings
