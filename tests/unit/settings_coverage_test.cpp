#include <cstddef>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "suborno/settings/settings.hpp"

#if !defined(_WIN32)
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

namespace {

using suborno::settings::CandidateOrientation;
using suborno::settings::EngineSettings;
using suborno::settings::SettingsLoadResult;
using suborno::settings::SettingsStatus;
using suborno::settings::SettingsStore;
using suborno::settings::StartMode;
using suborno::settings::SwitchingMode;
using suborno::settings::TextEncoding;

std::filesystem::path coverage_path(const std::string& file_name) {
    return std::filesystem::temp_directory_path() / file_name;
}

void write_file(const std::filesystem::path& path, const std::string& contents) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output << contents;
}

SettingsLoadResult load_from_contents(const std::string& contents, const std::string& file_name) {
    const std::filesystem::path path = coverage_path(file_name);
    static_cast<void>(std::filesystem::remove(path));
    write_file(path, contents);
    const SettingsLoadResult result = SettingsStore(path).load();
    static_cast<void>(std::filesystem::remove(path));
    return result;
}

// Builds a version=2 file that always contains the mandatory keys; the probed
// key either replaces its mandatory entry or is appended, so the probe never
// trips the duplicate-key or missing-key guards by accident.
SettingsLoadResult load_with_key(const std::string& key, const std::string& value) {
    std::vector<std::pair<std::string, std::string>> entries{
        {"version", "2"},
        {"active_layout", "suborno-phonetic"},
        {"candidate_limit", "16"},
        {"learning_enabled", "true"},
    };
    bool replaced = false;
    for (auto& entry : entries) {
        if (entry.first == key) {
            entry.second = value;
            replaced = true;
        }
    }
    if (!replaced) {
        entries.emplace_back(key, value);
    }
    std::string contents;
    for (const auto& entry : entries) {
        contents += entry.first + "=" + entry.second + "\n";
    }
    return load_from_contents(contents, "suborno_cov_settings_key_probe.ini");
}

SettingsLoadResult round_trip_through_file(const EngineSettings& settings,
                                           const std::string& file_name) {
    const std::filesystem::path path = coverage_path(file_name);
    static_cast<void>(std::filesystem::remove(path));
    const SettingsStore store(path);
    EXPECT_EQ(store.save(settings), SettingsStatus::ok);
    const SettingsLoadResult loaded = store.load();
    static_cast<void>(std::filesystem::remove(path));
    return loaded;
}

void expect_settings_equal(const EngineSettings& actual, const EngineSettings& expected) {
    EXPECT_EQ(actual.active_layout_id, expected.active_layout_id);
    EXPECT_EQ(actual.candidate_limit, expected.candidate_limit);
    EXPECT_EQ(actual.learning_enabled, expected.learning_enabled);
    EXPECT_EQ(actual.general.remember_last_input_layout,
              expected.general.remember_last_input_layout);
    EXPECT_EQ(actual.general.start_with_windows, expected.general.start_with_windows);
    EXPECT_EQ(actual.general.start_mode, expected.general.start_mode);
    EXPECT_EQ(actual.general.switching_mode, expected.general.switching_mode);
    EXPECT_EQ(actual.general.last_ui_mode, expected.general.last_ui_mode);
    EXPECT_EQ(actual.general.last_bangla_layout_id, expected.general.last_bangla_layout_id);
    EXPECT_EQ(actual.ui.theme_id, expected.ui.theme_id);
    EXPECT_EQ(actual.ui.font_family, expected.ui.font_family);
    EXPECT_EQ(actual.ui.show_candidate_window, expected.ui.show_candidate_window);
    EXPECT_EQ(actual.ui.next_word_prediction_enabled, expected.ui.next_word_prediction_enabled);
    EXPECT_EQ(actual.ui.candidate_orientation, expected.ui.candidate_orientation);
    EXPECT_EQ(actual.ui.candidate_fixed_height, expected.ui.candidate_fixed_height);
    EXPECT_EQ(actual.ui.use_os_default_bangla_font, expected.ui.use_os_default_bangla_font);
    EXPECT_EQ(actual.ui.enable_sliding_effect, expected.ui.enable_sliding_effect);
    EXPECT_EQ(actual.ui.follow_caret, expected.ui.follow_caret);
    EXPECT_EQ(actual.ui.follow_mouse_pointer_if_caret_tracking_fails,
              expected.ui.follow_mouse_pointer_if_caret_tracking_fails);
    EXPECT_EQ(actual.topbar.idle_dimming_enabled, expected.topbar.idle_dimming_enabled);
    EXPECT_EQ(actual.topbar.idle_opacity_percent, expected.topbar.idle_opacity_percent);
    EXPECT_EQ(actual.topbar.idle_time_seconds, expected.topbar.idle_time_seconds);
    EXPECT_EQ(actual.topbar.remember_position, expected.topbar.remember_position);
    EXPECT_EQ(actual.topbar.position_x, expected.topbar.position_x);
    EXPECT_EQ(actual.topbar.position_y, expected.topbar.position_y);
    EXPECT_EQ(actual.input.layout_switch_shortcut, expected.input.layout_switch_shortcut);
    EXPECT_EQ(actual.input.disable_encoding_shortcut, expected.input.disable_encoding_shortcut);
    EXPECT_EQ(actual.input.map_numpad_to_bangla_numbers,
              expected.input.map_numpad_to_bangla_numbers);
    EXPECT_EQ(actual.input.change_locale_to_bn_bd_when_layout_not_english,
              expected.input.change_locale_to_bn_bd_when_layout_not_english);
    EXPECT_EQ(actual.input.ignore_caps_lock_state_in_bangla,
              expected.input.ignore_caps_lock_state_in_bangla);
    EXPECT_EQ(actual.input.select_candidate_using_number_keys,
              expected.input.select_candidate_using_number_keys);
    EXPECT_EQ(actual.input.preferred_encoding, expected.input.preferred_encoding);
}

void expect_key_rejected(const std::string& key, const std::string& value) {
    const SettingsLoadResult loaded = load_with_key(key, value);
    EXPECT_EQ(loaded.status, SettingsStatus::invalid_format);
    EXPECT_EQ(loaded.error, "Invalid or unknown settings key: " + key);
}

std::size_t count_files_with_prefix(const std::filesystem::path& directory,
                                    const std::string& prefix) {
    std::size_t count = 0;
    std::error_code probe_error;
    for (const auto& entry : std::filesystem::directory_iterator(directory, probe_error)) {
        if (entry.path().filename().string().rfind(prefix, 0) == 0) {
            ++count;
        }
    }
    return count;
}

}  // namespace

// Covers every start_mode name/parse branch (118-144) plus the full save
// serialization block (517-567): each enum value round-trips byte-exact.
TEST(SettingsCoverage, RoundTripPreservesEveryStartMode) {
    for (const StartMode mode : {StartMode::last_state,
                                 StartMode::topbar,
                                 StartMode::system_tray,
                                 StartMode::main_window}) {
        EngineSettings settings;
        settings.general.start_mode = mode;
        settings.active_layout_id = "probhat";
        settings.candidate_limit = 48;
        const auto loaded =
            round_trip_through_file(settings, "suborno_cov_settings_roundtrip_start_mode.ini");
        ASSERT_EQ(loaded.status, SettingsStatus::ok);
        EXPECT_TRUE(loaded.file_existed);
        EXPECT_TRUE(loaded.error.empty());
        expect_settings_equal(loaded.settings, settings);
    }
}

// Covers switching_mode/orientation/encoding name+parse branches (147-207)
// across every combination of their two values via save→load round trips.
TEST(SettingsCoverage, RoundTripPreservesSwitchingOrientationAndEncodingCombos) {
    for (const SwitchingMode switching :
         {SwitchingMode::per_application_window, SwitchingMode::global}) {
        for (const CandidateOrientation orientation :
             {CandidateOrientation::vertical, CandidateOrientation::horizontal}) {
            for (const TextEncoding encoding : {TextEncoding::unicode, TextEncoding::ansi}) {
                EngineSettings settings;
                settings.general.switching_mode = switching;
                settings.ui.candidate_orientation = orientation;
                settings.input.preferred_encoding = encoding;
                const auto loaded =
                    round_trip_through_file(settings, "suborno_cov_settings_roundtrip_modes.ini");
                ASSERT_EQ(loaded.status, SettingsStatus::ok);
                expect_settings_equal(loaded.settings, settings);
            }
        }
    }
}

// Covers valid_switching_shortcut's match loop (210-228) for all 9 allowed
// shortcut values through real save→load round trips.
TEST(SettingsCoverage, RoundTripPreservesEveryAllowedLayoutSwitchShortcut) {
    for (const char* shortcut :
         {"F1", "F2", "F3", "F4", "F10", "F12", "Ctrl+Space", "Alt+Shift", "Win+Space"}) {
        EngineSettings settings;
        settings.input.layout_switch_shortcut = shortcut;
        const auto loaded =
            round_trip_through_file(settings, "suborno_cov_settings_roundtrip_shortcut.ini");
        ASSERT_EQ(loaded.status, SettingsStatus::ok);
        EXPECT_EQ(loaded.settings.input.layout_switch_shortcut, shortcut);
        expect_settings_equal(loaded.settings, settings);
    }
}

// Covers valid_theme_id character classes (59-67), valid_font_family classes
// (38-47), valid_ui_mode (51-53) and both "true"/"false" outcomes of every
// boolean serialization ternary in save() (521-566) with alternating values.
TEST(SettingsCoverage, RoundTripPreservesThemesFontsAndUiModes) {
    const std::vector<std::string> themes{
        "light", "dark", "high-contrast_black", "suborno_dark_v2"};
    const std::vector<std::string> fonts{
        "Nirmala UI", "Noto Sans Bengali (Local)", "Fixed.Sys_9.5", "Bengali-UI_One"};
    const std::vector<std::string> ui_modes{"topbar", "tray", "window"};
    const std::vector<std::string> layouts{
        "suborno-phonetic", "avro-phonetic", "probhat", "khipro"};
    std::size_t iteration = 0;
    for (const std::string& theme : themes) {
        for (const std::string& font : fonts) {
            for (const std::string& ui_mode : ui_modes) {
                EngineSettings settings;
                settings.ui.theme_id = theme;
                settings.ui.font_family = font;
                settings.general.last_ui_mode = ui_mode;
                settings.active_layout_id = layouts[iteration % layouts.size()];
                settings.general.last_bangla_layout_id = layouts[(iteration + 1) % layouts.size()];
                const bool flip = iteration % 2 == 0;
                settings.learning_enabled = flip;
                settings.general.remember_last_input_layout = !flip;
                settings.general.start_with_windows = flip;
                settings.ui.show_candidate_window = flip;
                settings.ui.next_word_prediction_enabled = !flip;
                settings.ui.candidate_fixed_height = flip;
                settings.ui.use_os_default_bangla_font = !flip;
                settings.ui.enable_sliding_effect = flip;
                settings.ui.follow_caret = !flip;
                settings.ui.follow_mouse_pointer_if_caret_tracking_fails = flip;
                settings.topbar.idle_dimming_enabled = !flip;
                settings.topbar.remember_position = flip;
                settings.input.disable_encoding_shortcut = !flip;
                settings.input.map_numpad_to_bangla_numbers = flip;
                settings.input.change_locale_to_bn_bd_when_layout_not_english = !flip;
                settings.input.ignore_caps_lock_state_in_bangla = flip;
                settings.input.select_candidate_using_number_keys = !flip;
                const auto loaded =
                    round_trip_through_file(settings, "suborno_cov_settings_roundtrip_theme.ini");
                ASSERT_EQ(loaded.status, SettingsStatus::ok);
                expect_settings_equal(loaded.settings, settings);
                ++iteration;
            }
        }
    }
}

// Covers the exact-integer serialization of save()/load() (lines 549-554:
// candidate_limit, idle_opacity_percent, idle_time_seconds, positions) and
// every boolean persisted as its non-default value.
TEST(SettingsCoverage, RoundTripPreservesFlippedBoolsAndExactIntegers) {
    EngineSettings settings;
    settings.candidate_limit = 77;
    settings.topbar.idle_opacity_percent = 13;
    settings.topbar.idle_time_seconds = 42;
    settings.topbar.position_x = -1234;
    settings.topbar.position_y = 4321;
    settings.general.remember_last_input_layout = false;
    settings.general.start_with_windows = true;
    settings.learning_enabled = false;
    settings.ui.show_candidate_window = false;
    settings.ui.next_word_prediction_enabled = true;
    settings.ui.candidate_fixed_height = false;
    settings.ui.use_os_default_bangla_font = false;
    settings.ui.enable_sliding_effect = false;
    settings.ui.follow_caret = false;
    settings.ui.follow_mouse_pointer_if_caret_tracking_fails = false;
    settings.topbar.idle_dimming_enabled = false;
    settings.topbar.remember_position = false;
    settings.input.disable_encoding_shortcut = true;
    settings.input.map_numpad_to_bangla_numbers = false;
    settings.input.change_locale_to_bn_bd_when_layout_not_english = false;
    settings.input.ignore_caps_lock_state_in_bangla = true;
    settings.input.select_candidate_using_number_keys = false;
    const auto loaded =
        round_trip_through_file(settings, "suborno_cov_settings_roundtrip_integers.ini");
    ASSERT_EQ(loaded.status, SettingsStatus::ok);
    expect_settings_equal(loaded.settings, settings);
}

// Covers the first-run outcome of load() (is_open-false branch 408-416, the
// ok ternary outcome): a missing file yields pure EngineSettings{} defaults
// with file_existed=false and no error text.
TEST(SettingsCoverage, LoadWithoutFileYieldsCompleteDefaults) {
    const auto path = coverage_path("suborno_cov_settings_first_run_probe.ini");
    static_cast<void>(std::filesystem::remove(path));
    const SettingsStore store(path);
    const auto loaded = store.load();
    ASSERT_EQ(loaded.status, SettingsStatus::ok);
    EXPECT_FALSE(loaded.file_existed);
    EXPECT_TRUE(loaded.error.empty());
    expect_settings_equal(loaded.settings, EngineSettings{});
    static_cast<void>(std::filesystem::remove(path));
}

// Covers the no-'=' separator guard (438-443) with its exact error text.
TEST(SettingsCoverage, LoadRejectsMalformedLineWithExactError) {
    const auto loaded =
        load_from_contents("justakey\n", "suborno_cov_settings_malformed_probe.ini");
    ASSERT_EQ(loaded.status, SettingsStatus::invalid_format);
    EXPECT_EQ(loaded.error, "Malformed settings line");
    EXPECT_TRUE(loaded.file_existed);
}

// Covers duplicate-key detection (446-450) for the version key and a normal
// settings key alike, asserting the exact error text each time.
TEST(SettingsCoverage, LoadRejectsDuplicateKeysWithExactError) {
    const auto duplicated_version = load_from_contents(
        "version=2\nversion=2\nactive_layout=probhat\ncandidate_limit=16\n"
        "learning_enabled=true\n",
        "suborno_cov_settings_dup_version_probe.ini");
    ASSERT_EQ(duplicated_version.status, SettingsStatus::invalid_format);
    EXPECT_EQ(duplicated_version.error, "Duplicate settings key");

    const auto duplicated_layout = load_from_contents(
        "version=2\nlast_ui_mode=tray\nlast_ui_mode=tray\nactive_layout=probhat\n"
        "candidate_limit=16\nlearning_enabled=true\n",
        "suborno_cov_settings_dup_layout_probe.ini");
    ASSERT_EQ(duplicated_layout.status, SettingsStatus::invalid_format);
    EXPECT_EQ(duplicated_layout.error, "Duplicate settings key");
}

// Covers the missing-version guard (478-482): parseable keys but no version
// line are rejected with the exact error text.
TEST(SettingsCoverage, LoadRejectsMissingVersionWithExactError) {
    const auto loaded =
        load_from_contents("active_layout=probhat\ncandidate_limit=16\nlearning_enabled=true\n",
                           "suborno_cov_settings_no_version_probe.ini");
    ASSERT_EQ(loaded.status, SettingsStatus::invalid_format);
    EXPECT_EQ(loaded.error, "Missing settings version");
}

// Covers the unsupported-version guard (483-487) on both rejected sides of
// the accepted {1, 2} set: version=0 below and version=3 above.
TEST(SettingsCoverage, LoadRejectsUnsupportedVersionsWithExactError) {
    const std::string body = "active_layout=probhat\ncandidate_limit=16\nlearning_enabled=true\n";
    const auto zero =
        load_from_contents("version=0\n" + body, "suborno_cov_settings_version0_probe.ini");
    ASSERT_EQ(zero.status, SettingsStatus::unsupported_version);
    EXPECT_EQ(zero.error, "Unsupported settings version");
    const auto three =
        load_from_contents("version=3\n" + body, "suborno_cov_settings_version3_probe.ini");
    ASSERT_EQ(three.status, SettingsStatus::unsupported_version);
    EXPECT_EQ(three.error, "Unsupported settings version");
}

// Covers the required-engine-settings guard (488-492) with one file per
// missing mandatory key, asserting the exact error text each time.
TEST(SettingsCoverage, LoadRejectsMissingRequiredEngineSettingsWithExactError) {
    const auto no_layout =
        load_from_contents("version=2\n", "suborno_cov_settings_missing_layout_probe.ini");
    ASSERT_EQ(no_layout.status, SettingsStatus::invalid_format);
    EXPECT_EQ(no_layout.error, "Missing required engine settings");

    const auto no_limit = load_from_contents("version=2\nactive_layout=probhat\n",
                                             "suborno_cov_settings_missing_limit_probe.ini");
    ASSERT_EQ(no_limit.status, SettingsStatus::invalid_format);
    EXPECT_EQ(no_limit.error, "Missing required engine settings");

    const auto no_learning =
        load_from_contents("version=2\nactive_layout=probhat\ncandidate_limit=16\n",
                           "suborno_cov_settings_missing_learning_probe.ini");
    ASSERT_EQ(no_learning.status, SettingsStatus::invalid_format);
    EXPECT_EQ(no_learning.error, "Missing required engine settings");
}

// Covers parse_uint's empty (72-74), non-digit (77-80) and overflow (81-84)
// rejection branches through the version key, plus the "Invalid settings
// version" error text (453-457).
TEST(SettingsCoverage, LoadRejectsInvalidAndOverflowingVersions) {
    const std::string body = "active_layout=probhat\ncandidate_limit=16\nlearning_enabled=true\n";
    const std::vector<std::string> invalid_versions{
        "", "abc", "2.5", "-2", "-", "99999999999999999999999999"};
    for (const std::string& version : invalid_versions) {
        SCOPED_TRACE(version);
        const auto loaded = load_from_contents("version=" + version + "\n" + body,
                                               "suborno_cov_settings_version_probe.ini");
        ASSERT_EQ(loaded.status, SettingsStatus::invalid_format);
        EXPECT_EQ(loaded.error, "Invalid settings version");
    }
}

// Covers the maximum-line-length guard (432-436) including its boundary:
// a 4096-character line is accepted, a longer one rejected with the exact
// error text.
TEST(SettingsCoverage, LoadRejectsOverlongLinesAndAcceptsMaxLengthBoundary) {
    const std::string body = "version=2\ncandidate_limit=16\nlearning_enabled=true\n";
    const auto too_long =
        load_from_contents("active_layout=" + std::string(5000, 'a') + "\n" + body,
                           "suborno_cov_settings_overlong_probe.ini");
    ASSERT_EQ(too_long.status, SettingsStatus::invalid_format);
    EXPECT_EQ(too_long.error, "Settings line exceeds the maximum supported length");

    // "active_layout=" is 14 characters, so 4082 'a' characters make the
    // boundary line exactly 4096 characters long.
    const auto boundary =
        load_from_contents("active_layout=" + std::string(4082, 'a') + "\n" + body,
                           "suborno_cov_settings_maxline_probe.ini");
    ASSERT_EQ(boundary.status, SettingsStatus::ok);
    EXPECT_EQ(boundary.settings.active_layout_id.size(), std::size_t{4082});
}

// Covers every per-key rejection branch in parse_settings_value (lines
// 253-394) plus the unknown-key fallthrough (393-395): each probe writes a
// version=2 file with one invalid value and asserts the exact error text
// naming that key.
TEST(SettingsCoverage, LoadRejectsInvalidValueForEachSettingsKey) {
    const std::vector<std::pair<std::string, std::string>> invalid_entries{
        {"active_layout", ""},
        {"active_layout", "with=equals"},
        {"active_layout", "carriage\rreturn"},
        {"active_layout", std::string("nul\0byte", 8)},
        {"candidate_limit", "0"},
        {"candidate_limit", "101"},
        {"candidate_limit", "abc"},
        {"candidate_limit", ""},
        {"candidate_limit", "999999999999999999999999999999"},
        {"learning_enabled", "yes"},
        {"remember_last_input_layout", "maybe"},
        {"last_ui_mode", "bogus"},
        {"last_bangla_layout_id", "with=equals"},
        {"start_with_windows", "1"},
        {"start_mode", "bogus"},
        {"switching_mode", "bogus"},
        {"theme_id", "Light"},
        {"theme_id", "with space"},
        {"theme_id", "bad=id"},
        {"font_family", std::string(33, 'a')},
        {"font_family", "Noto@Sans"},
        {"show_candidate_window", "nope"},
        {"next_word_prediction_enabled", "maybe"},
        {"candidate_orientation", "diagonal"},
        {"candidate_fixed_height", "1"},
        {"use_os_default_bangla_font", "y"},
        {"enable_sliding_effect", "off"},
        {"follow_caret", "1"},
        {"follow_mouse_pointer_if_caret_tracking_fails", "nah"},
        {"idle_dimming_enabled", "1"},
        {"idle_opacity_percent", "101"},
        {"idle_opacity_percent", ""},
        {"idle_time_seconds", "0"},
        {"idle_time_seconds", "3601"},
        {"remember_topbar_position", "1"},
        {"topbar_position_x", "-32769"},
        {"topbar_position_x", "32768"},
        {"topbar_position_x", "-abc"},
        {"topbar_position_x", "5000000000"},
        {"topbar_position_y", "-32769"},
        {"topbar_position_y", "32768"},
        {"layout_switch_shortcut", "F5"},
        {"disable_encoding_shortcut", "1"},
        {"map_numpad_to_bangla_numbers", "1"},
        {"change_locale_to_bn_bd_when_layout_not_english", "1"},
        {"ignore_caps_lock_state_in_bangla", "1"},
        {"select_candidate_using_number_keys", "1"},
        {"preferred_encoding", "utf8"},
        {"totally_unknown_key", "1"},
    };
    for (const auto& probe : invalid_entries) {
        SCOPED_TRACE(probe.first);
        expect_key_rejected(probe.first, probe.second);
    }
}

// Covers the accepted boundary values of every numeric key (candidate_limit
// 1..100, idle_opacity_percent 0..100, idle_time_seconds 1..3600, topbar
// positions -32768..32767) including parse_int's negative path (93-103) and
// the zero-opacity corner that candidate_limit forbids.
TEST(SettingsCoverage, LoadAcceptsBoundaryValuesForNumericKeys) {
    {
        const auto loaded = load_with_key("candidate_limit", "1");
        ASSERT_EQ(loaded.status, SettingsStatus::ok);
        EXPECT_EQ(loaded.settings.candidate_limit, std::size_t{1});
    }
    {
        const auto loaded = load_with_key("candidate_limit", "100");
        ASSERT_EQ(loaded.status, SettingsStatus::ok);
        EXPECT_EQ(loaded.settings.candidate_limit, std::size_t{100});
    }
    {
        const auto loaded = load_with_key("idle_opacity_percent", "0");
        ASSERT_EQ(loaded.status, SettingsStatus::ok);
        EXPECT_EQ(loaded.settings.topbar.idle_opacity_percent, 0U);
    }
    {
        const auto loaded = load_with_key("idle_opacity_percent", "100");
        ASSERT_EQ(loaded.status, SettingsStatus::ok);
        EXPECT_EQ(loaded.settings.topbar.idle_opacity_percent, 100U);
    }
    {
        const auto loaded = load_with_key("idle_time_seconds", "1");
        ASSERT_EQ(loaded.status, SettingsStatus::ok);
        EXPECT_EQ(loaded.settings.topbar.idle_time_seconds, 1U);
    }
    {
        const auto loaded = load_with_key("idle_time_seconds", "3600");
        ASSERT_EQ(loaded.status, SettingsStatus::ok);
        EXPECT_EQ(loaded.settings.topbar.idle_time_seconds, 3600U);
    }
    {
        const auto loaded = load_with_key("topbar_position_x", "-1");
        ASSERT_EQ(loaded.status, SettingsStatus::ok);
        EXPECT_EQ(loaded.settings.topbar.position_x, -1);
    }
    {
        const auto loaded = load_with_key("topbar_position_x", "-32768");
        ASSERT_EQ(loaded.status, SettingsStatus::ok);
        EXPECT_EQ(loaded.settings.topbar.position_x, -32768);
    }
    {
        const auto loaded = load_with_key("topbar_position_x", "32767");
        ASSERT_EQ(loaded.status, SettingsStatus::ok);
        EXPECT_EQ(loaded.settings.topbar.position_x, 32767);
    }
    {
        const auto loaded = load_with_key("topbar_position_y", "-1");
        ASSERT_EQ(loaded.status, SettingsStatus::ok);
        EXPECT_EQ(loaded.settings.topbar.position_y, -1);
    }
    {
        const auto loaded = load_with_key("topbar_position_y", "-32768");
        ASSERT_EQ(loaded.status, SettingsStatus::ok);
        EXPECT_EQ(loaded.settings.topbar.position_y, -32768);
    }
    {
        const auto loaded = load_with_key("topbar_position_y", "32767");
        ASSERT_EQ(loaded.status, SettingsStatus::ok);
        EXPECT_EQ(loaded.settings.topbar.position_y, 32767);
    }
}

// Documents CRLF handling: getline keeps the '\r' inside the value, so a
// CRLF file is rejected at the version line by parse_uint (77-80) with the
// exact "Invalid settings version" error.
TEST(SettingsCoverage, LoadRejectsCarriageReturnLineEndings) {
    const auto loaded = load_from_contents(
        "version=2\r\nactive_layout=probhat\r\ncandidate_limit=16\r\nlearning_enabled=true\r\n",
        "suborno_cov_settings_crlf_probe.ini");
    ASSERT_EQ(loaded.status, SettingsStatus::invalid_format);
    EXPECT_EQ(loaded.error, "Invalid settings version");
    EXPECT_TRUE(loaded.file_existed);
}

// Covers the blank-line skip (429-431): empty lines between keys are
// accepted and parsing continues normally.
TEST(SettingsCoverage, LoadAcceptsBlankLinesBetweenKeys) {
    const auto loaded = load_from_contents(
        "\nversion=2\n\nactive_layout=probhat\n\ncandidate_limit=16\n\nlearning_enabled=true\n\n",
        "suborno_cov_settings_blanklines_probe.ini");
    ASSERT_EQ(loaded.status, SettingsStatus::ok);
    EXPECT_EQ(loaded.settings.active_layout_id, "probhat");
    EXPECT_TRUE(loaded.file_existed);
}

// Covers the read-failure outcome of load() on POSIX: glibc fopen succeeds
// on a directory but the first read fails, which libstdc++ turns into
// badbit, so the input.bad() branch (473-477) reports io_error with its
// exact error text. On Windows the directory instead fails is_open and the
// 408-416 branch reports io_error without an error string.
TEST(SettingsCoverage, LoadOnDirectoryPathReportsIoErrorWithFileExistedTrue) {
    const auto directory = coverage_path("suborno_cov_settings_load_dir_probe");
    static_cast<void>(std::filesystem::remove_all(directory));
    std::filesystem::create_directories(directory);

    const auto loaded = SettingsStore(directory).load();
    EXPECT_EQ(loaded.status, SettingsStatus::io_error);
    EXPECT_TRUE(loaded.file_existed);
#if defined(_WIN32)
    EXPECT_TRUE(loaded.error.empty());
#else
    EXPECT_EQ(loaded.error, "Failed while reading settings");
#endif

    static_cast<void>(std::filesystem::remove_all(directory));
}

// Covers the existing-but-unopenable outcome of load() (408-416, io_error
// ternary outcome): a POSIX socket file stat()s fine — so file_existed is
// true — but cannot be opened by ifstream, giving io_error with no error
// text. Running as root rules out permission-based alternatives.
TEST(SettingsCoverage, LoadOnExistingButUnopenablePathReportsIoErrorWithoutErrorText) {
#if defined(_WIN32)
    GTEST_SKIP() << "POSIX socket files are required to make an existing path unopenable.";
#else
    const auto path = coverage_path("suborno_cov_settings_socket_probe.ini");
    static_cast<void>(std::filesystem::remove(path));

    const int listener = ::socket(AF_UNIX, SOCK_STREAM, 0);
    ASSERT_GE(listener, 0);
    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    const std::string path_text = path.string();
    ASSERT_LT(path_text.size(), sizeof(address.sun_path));
    std::strncpy(address.sun_path, path_text.c_str(), sizeof(address.sun_path) - 1);
    ASSERT_EQ(::bind(listener, reinterpret_cast<const sockaddr*>(&address), sizeof(address)), 0);
    ASSERT_EQ(::close(listener), 0);

    ASSERT_TRUE(std::filesystem::exists(path));
    const auto loaded = SettingsStore(path).load();
    EXPECT_EQ(loaded.status, SettingsStatus::io_error);
    EXPECT_TRUE(loaded.file_existed);
    EXPECT_TRUE(loaded.error.empty());

    static_cast<void>(std::filesystem::remove(path));
#endif
}

// Covers save()'s validate gate (503-506) with every struct-level violation
// the file parser itself can never produce, and proves nothing is written
// to disk (validate runs before any filesystem access).
TEST(SettingsCoverage, SaveRejectsInvalidSettingsWithoutTouchingDisk) {
    const auto path = coverage_path("suborno_cov_settings_save_invalid_probe.ini");
    static_cast<void>(std::filesystem::remove(path));

    std::vector<EngineSettings> invalid_settings;
    EngineSettings variant;
    variant.candidate_limit = 0;
    invalid_settings.push_back(variant);
    variant = {};
    variant.active_layout_id = "";
    invalid_settings.push_back(variant);
    variant = {};
    variant.active_layout_id = "with=equals";
    invalid_settings.push_back(variant);
    variant = {};
    variant.general.last_bangla_layout_id = "";
    invalid_settings.push_back(variant);
    variant = {};
    variant.general.last_ui_mode = "bogus";
    invalid_settings.push_back(variant);
    variant = {};
    variant.ui.theme_id = "Has Space";
    invalid_settings.push_back(variant);
    variant = {};
    variant.ui.font_family = std::string(33, 'a');
    invalid_settings.push_back(variant);
    variant = {};
    variant.input.layout_switch_shortcut = "F99";
    invalid_settings.push_back(variant);
    variant = {};
    variant.topbar.idle_opacity_percent = 101;
    invalid_settings.push_back(variant);
    variant = {};
    variant.topbar.idle_time_seconds = 0;
    invalid_settings.push_back(variant);
    variant = {};
    variant.topbar.idle_time_seconds = 3601;
    invalid_settings.push_back(variant);
    variant = {};
    variant.topbar.position_x = 32768;
    invalid_settings.push_back(variant);
    variant = {};
    variant.topbar.position_y = -32769;
    invalid_settings.push_back(variant);

    for (const EngineSettings& settings : invalid_settings) {
        EXPECT_EQ(SettingsStore(path).save(settings), SettingsStatus::invalid_format);
    }
    EXPECT_FALSE(std::filesystem::exists(path));
    static_cast<void>(std::filesystem::remove(path));
}

// Covers save()'s create-directories failure branch (508-515): the parent
// path is an existing regular file, so create_directories fails even as
// root (ENOTDIR) and save() must report io_error without writing anything.
TEST(SettingsCoverage, SaveReportsIoErrorWhenParentIsAnExistingRegularFile) {
    const auto blocker = coverage_path("suborno_cov_settings_parent_blocker.ini");
    static_cast<void>(std::filesystem::remove(blocker));
    write_file(blocker, "blocker");

    const SettingsStore store(blocker / "settings.ini");
    EXPECT_EQ(store.save(EngineSettings{}), SettingsStatus::io_error);
    EXPECT_TRUE(std::filesystem::is_regular_file(blocker));
    EXPECT_FALSE(std::filesystem::exists(blocker / "settings.ini"));

    static_cast<void>(std::filesystem::remove(blocker));
}

// Covers the atomic-write failure outcome of save() (572-574): the
// destination is an existing directory, so the temporary file is written
// and the rename fails, leaving no temporary litter beside the destination.
TEST(SettingsCoverage, SaveReportsIoErrorWhenDestinationIsAnExistingDirectory) {
    const auto directory = coverage_path("suborno_cov_settings_dest_dir_probe");
    static_cast<void>(std::filesystem::remove_all(directory));
    std::filesystem::create_directories(directory);

    const SettingsStore store(directory);
    EXPECT_EQ(store.save(EngineSettings{}), SettingsStatus::io_error);
    EXPECT_TRUE(std::filesystem::is_directory(directory));
    std::size_t entry_count = 0;
    std::error_code probe_error;
    for (const auto& entry : std::filesystem::directory_iterator(directory, probe_error)) {
        static_cast<void>(entry);
        ++entry_count;
    }
    EXPECT_EQ(entry_count, std::size_t{0});
    EXPECT_EQ(count_files_with_prefix(std::filesystem::temp_directory_path(),
                                      "suborno_cov_settings_dest_dir_probe.tmp."),
              std::size_t{0});

    static_cast<void>(std::filesystem::remove_all(directory));
}

// Covers save()'s empty-parent-path branch (509-515, skipped when parent is
// empty): a bare relative filename has no parent, create_directories is
// skipped, and the settings file lands in the current working directory.
TEST(SettingsCoverage, SaveWorksWithRelativeDestinationWithoutParentDirectory) {
    const std::filesystem::path relative("suborno_cov_settings_relative_save.ini");
    const auto absolute = std::filesystem::current_path() / relative;
    static_cast<void>(std::filesystem::remove(absolute));

    const SettingsStore store(relative);
    EXPECT_EQ(store.save(EngineSettings{}), SettingsStatus::ok);
    ASSERT_TRUE(std::filesystem::is_regular_file(absolute));
    const auto loaded = store.load();
    ASSERT_EQ(loaded.status, SettingsStatus::ok);
    expect_settings_equal(loaded.settings, EngineSettings{});

    static_cast<void>(std::filesystem::remove(absolute));
}

// Covers file_stamp()'s reachable outcomes (578-601): nonexistent paths and
// directories report exists=false; a regular file reports its exact size and
// the file_clock modification ticks (the tick epoch is stdlib-defined —
// negative under libstdc++, positive under MSVC — so only within-platform
// ordering and equality are portable); unchanged files compare equal via
// the defaulted operator==.
TEST(SettingsCoverage, FileStampTracksExistenceSizeAndModificationTime) {
    const auto path = coverage_path("suborno_cov_settings_stamp_probe.ini");
    static_cast<void>(std::filesystem::remove(path));

    const auto missing = SettingsStore(path).file_stamp();
    EXPECT_FALSE(missing.exists);
    EXPECT_EQ(missing.size, 0U);
    EXPECT_EQ(missing.mtime_ticks, 0);

    const auto directory = coverage_path("suborno_cov_settings_stamp_dir");
    static_cast<void>(std::filesystem::remove_all(directory));
    std::filesystem::create_directories(directory);
    const auto directory_stamp = SettingsStore(directory).file_stamp();
    EXPECT_FALSE(directory_stamp.exists);
    EXPECT_EQ(directory_stamp.size, 0U);
    EXPECT_EQ(directory_stamp.mtime_ticks, 0);

    write_file(path, std::string(123, 'x'));
    const auto stamp = SettingsStore(path).file_stamp();
    EXPECT_TRUE(stamp.exists);
    EXPECT_EQ(stamp.size, 123U);
    const auto repeat = SettingsStore(path).file_stamp();
    EXPECT_TRUE(stamp == repeat);

    // Deterministic ordering: an explicitly later modification time must
    // produce strictly larger ticks on the same platform.
    using FileClock = std::filesystem::file_time_type::clock;
    const FileClock::time_point early = FileClock::now() - std::chrono::hours(24);
    const FileClock::time_point late = early + std::chrono::hours(1);
    std::filesystem::last_write_time(path, early);
    const auto early_stamp = SettingsStore(path).file_stamp();
    std::filesystem::last_write_time(path, late);
    const auto late_stamp = SettingsStore(path).file_stamp();
    EXPECT_GT(late_stamp.mtime_ticks, early_stamp.mtime_ticks);

    write_file(path, "shorter");
    const auto rewritten = SettingsStore(path).file_stamp();
    EXPECT_TRUE(rewritten.exists);
    EXPECT_EQ(rewritten.size, 7U);

    static_cast<void>(std::filesystem::remove(path));
    static_cast<void>(std::filesystem::remove_all(directory));
}
