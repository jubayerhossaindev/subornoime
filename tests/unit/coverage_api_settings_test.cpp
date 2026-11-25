// Coverage tests for src/core/c_api.cpp, include/suborno/settings/settings.hpp,

// src/settings/settings.cpp, src/persistence/atomic_file.cpp,

// src/core/engine.cpp and src/candidates/candidate_engine.cpp

// (suite CoverageApiSettings).

//

// Each test names the previously uncovered gap lines it exercises. Every

// assertion targets documented behavior: documented status codes, exact error

// strings, boundary values and round-trip equality. Nothing here depends on

// timing, signals beyond the deterministic SIGXFSZ/SIG_IGN pairing below, or

// anything outside the public API.

#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <functional>
#include <gtest/gtest.h>
#include <iterator>
#include <memory>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#if !defined(_WIN32)
#include <csignal>
#include <sys/resource.h>
#include <unistd.h>
#else
#include <process.h>
#endif

#include "persistence/atomic_file.hpp"
#include "suborno/candidates/candidate_engine.hpp"
#include "suborno/core/engine.hpp"
#include "suborno/dictionary/dictionary.hpp"
#include "suborno/layout-api/input_layout.hpp"
#include "suborno/settings/settings.hpp"
#include "suborno/suborno.h"

namespace {

using suborno::settings::CandidateOrientation;
using suborno::settings::EngineSettings;
using suborno::settings::SettingsLoadResult;
using suborno::settings::SettingsStatus;
using suborno::settings::SettingsStore;
using suborno::settings::StartMode;
using suborno::settings::SwitchingMode;
using suborno::settings::TextEncoding;

using SubornoEngineHandle = std::unique_ptr<SUBORNO_Engine, decltype(&suborno_engine_destroy)>;
using SubornoSessionHandle = std::unique_ptr<SUBORNO_Session, decltype(&suborno_session_destroy)>;

SubornoEngineHandle make_engine() {
    return SubornoEngineHandle(suborno_engine_create(), &suborno_engine_destroy);
}

SubornoSessionHandle make_session(SUBORNO_Engine* engine) {
    return SubornoSessionHandle(suborno_session_create(engine), &suborno_session_destroy);
}

std::filesystem::path probe_path(const std::string& file_name) {
    return std::filesystem::temp_directory_path() / file_name;
}

void remove_quietly(const std::filesystem::path& path) {
    std::error_code ignored;
    std::filesystem::remove_all(path, ignored);
}

void write_text_file(const std::filesystem::path& path, const std::string& contents) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output << contents;
}

std::string read_text_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return std::string{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

std::vector<std::pair<std::string, std::string>> mandatory_settings_entries() {
    return {
        {"version", "2"},
        {"active_layout", "suborno-phonetic"},
        {"candidate_limit", "16"},
        {"learning_enabled", "true"},
    };
}

// Loads a synthetic settings file; `key` replaces its mandatory entry when it

// is one of the required keys, so the duplicate-key guard never fires first.

SettingsLoadResult load_with_entry(const std::string& key,
                                   const std::string& value,
                                   const std::string& file_name) {
    std::vector<std::pair<std::string, std::string>> entries = mandatory_settings_entries();
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
    const std::filesystem::path path = probe_path(file_name);
    remove_quietly(path);
    std::string contents;
    for (const auto& entry : entries) {
        contents += entry.first + "=" + entry.second + "\n";
    }
    write_text_file(path, contents);
    const SettingsLoadResult result = SettingsStore(path).load();
    remove_quietly(path);
    return result;
}

std::size_t count_temporary_files(const std::filesystem::path& directory) {
    std::size_t count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.path().filename().string().find(".tmp.") != std::string::npos) {
            ++count;
        }
    }
    return count;
}

// Reproduces the quarantine target name computed by atomic_file.cpp

// (stem + "-<pid>-<tid>-<attempt>"): the same std::hash expression over the

// same thread id inside the same process, and the same process id, make the

// collision targets fully predictable without touching production code.

std::string quarantine_collision_suffix(unsigned long long attempt) {
#if defined(_WIN32)
    const unsigned long long process_id = static_cast<unsigned long long>(::_getpid());
#else
    const unsigned long long process_id = static_cast<unsigned long long>(::getpid());
#endif
    const auto thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
    return "-" + std::to_string(process_id) + "-" + std::to_string(thread_id) + "-"
           + std::to_string(attempt);
}

std::filesystem::path quarantine_collision_target(const std::filesystem::path& path,
                                                  unsigned long long attempt) {
    std::filesystem::path target = path;
    target += ".corrupt";
    target += quarantine_collision_suffix(attempt);
    return target;
}

// Fixed layout whose transliteration differs from the raw input for "ab", so

// the candidate engine's dual exact/prefix queries (input vs normalized) both

// run and the per-stage cap logic becomes observable.

class ProbeLayout final : public suborno::layout::StatelessInputLayout {
  public:
    [[nodiscard]] suborno::layout::LayoutKind kind() const noexcept override {
        return suborno::layout::LayoutKind::fixed;
    }

    [[nodiscard]] std::string transliterate(const std::string& input) const override {
        return input == "ab" ? "বাংলা" : input;
    }

    [[nodiscard]] std::vector<suborno::layout::Candidate> generate_candidates(
        const std::string& input) const override {
        (void)input;
        return {
            {"বাংলা",
             "বাংলা",
             "probe:বাংলা",
             suborno::layout::CandidateSource::layout,
             1.0,
             "probe"},
            {"বই", "বই", "probe:বই", suborno::layout::CandidateSource::layout, 0.5, "probe"},
        };
    }
};

std::string temporary_database_path(const char* filename) {
    return (std::filesystem::temp_directory_path() / filename).string();
}

void remove_database(const std::string& database_path) {
    std::error_code ignored;
    std::filesystem::remove(std::filesystem::path(database_path), ignored);
    std::filesystem::remove(std::filesystem::path(database_path + "-wal"), ignored);
    std::filesystem::remove(std::filesystem::path(database_path + "-shm"), ignored);
}

}  // namespace

// ---------------------------------------------------------------------------

// src/core/c_api.cpp

// ---------------------------------------------------------------------------

TEST(CoverageApiSettings, CApiEngineAndSessionHandlesValidateNullArguments) {
    // Gap: c_api.cpp:95 (session_create with a null engine) plus the null

    // no-op contracts of both destroy functions.

    SubornoEngineHandle engine = make_engine();
    ASSERT_NE(engine, nullptr);
    EXPECT_EQ(suborno_session_create(nullptr), nullptr);
    suborno_engine_destroy(nullptr);
    suborno_session_destroy(nullptr);

    // A real engine handle round-trips create/destroy without leaking.

    SUBORNO_Engine* raw_engine = suborno_engine_create();
    ASSERT_NE(raw_engine, nullptr);
    SUBORNO_Session* raw_session = suborno_session_create(raw_engine);
    ASSERT_NE(raw_session, nullptr);
    suborno_session_destroy(raw_session);
    suborno_engine_destroy(raw_engine);
}

TEST(CoverageApiSettings, CApiAppendMapsEveryDocumentedInputStatus) {
    // Gap: c_api.cpp:26-37 — the pending, literal_passthrough, invalid_utf8,

    // input_limit, candidate_out_of_range and empty arms of to_c_status, each

    // reached through its documented public trigger.

    SubornoEngineHandle engine = make_engine();
    ASSERT_NE(engine, nullptr);

    {
        // An unterminated control token keeps the composition pending.

        SubornoSessionHandle session = make_session(engine.get());
        ASSERT_NE(session, nullptr);
        EXPECT_EQ(suborno_session_append_utf8(session.get(), ":"), SUBORNO_STATUS_PENDING);
    }
    {
        // Characters the phonetic parser does not model pass through.

        SubornoSessionHandle session = make_session(engine.get());
        ASSERT_NE(session, nullptr);
        EXPECT_EQ(suborno_session_append_utf8(session.get(), "@"),
                  SUBORNO_STATUS_LITERAL_PASSTHROUGH);
    }
    {
        // Invalid UTF-8 is rejected without entering the composition.

        SubornoSessionHandle session = make_session(engine.get());
        ASSERT_NE(session, nullptr);
        EXPECT_EQ(suborno_session_append_utf8(session.get(), "\xff\xfe"),
                  SUBORNO_STATUS_INVALID_UTF8);
    }
    {
        // 257 code points exceed the 256-code-point composition limit.

        SubornoSessionHandle session = make_session(engine.get());
        ASSERT_NE(session, nullptr);
        const std::string oversized(257, 'a');
        EXPECT_EQ(suborno_session_append_utf8(session.get(), oversized.c_str()),
                  SUBORNO_STATUS_INPUT_LIMIT);
    }
    {
        // Committing an out-of-range candidate index maps

        // InputStatus::candidate_out_of_range through the commit boundary.

        SubornoSessionHandle session = make_session(engine.get());
        ASSERT_NE(session, nullptr);
        ASSERT_EQ(suborno_session_append_utf8(session.get(), "ami"), SUBORNO_STATUS_UPDATED);
        char* committed = nullptr;
        std::size_t committed_size = 1;
        const int status =
            suborno_session_commit_candidate(session.get(), 55, &committed, &committed_size);
        std::unique_ptr<char, decltype(&suborno_string_free)> guard(committed,
                                                                    &suborno_string_free);
        EXPECT_EQ(status, SUBORNO_STATUS_CANDIDATE_OUT_OF_RANGE);
        // A positive status still allocates the (empty) output string.

        ASSERT_NE(guard, nullptr);
        EXPECT_EQ(committed_size, 0U);
        EXPECT_EQ(std::string(guard.get(), committed_size), "");
    }
    {
        // Empty input and an empty composition both map InputStatus::empty.

        SubornoSessionHandle session = make_session(engine.get());
        ASSERT_NE(session, nullptr);
        EXPECT_EQ(suborno_session_append_utf8(session.get(), ""), SUBORNO_STATUS_EMPTY);
        char* committed = nullptr;
        std::size_t committed_size = 0;
        std::unique_ptr<char, decltype(&suborno_string_free)> guard(nullptr, &suborno_string_free);
        EXPECT_EQ(suborno_session_commit(session.get(), &committed, &committed_size),
                  SUBORNO_STATUS_EMPTY);
        guard.reset(committed);
        ASSERT_NE(guard, nullptr);
        EXPECT_EQ(committed_size, 0U);
    }
}

TEST(CoverageApiSettings, CApiBackspaceLifecycleAndSnapshotBufferRules) {
    // Gap: c_api.cpp:126-128 (backspace through the C boundary) plus the

    // snapshot argument/buffer rules on both sides of every size check.

    SubornoEngineHandle engine = make_engine();
    ASSERT_NE(engine, nullptr);
    SubornoSessionHandle session = make_session(engine.get());
    ASSERT_NE(session, nullptr);

    // Backspacing an empty composition reports SUBORNO_STATUS_EMPTY.

    EXPECT_EQ(suborno_session_backspace(session.get()), SUBORNO_STATUS_EMPTY);

    ASSERT_EQ(suborno_session_append_utf8(session.get(), "ami"), SUBORNO_STATUS_UPDATED);
    EXPECT_EQ(suborno_session_backspace(session.get()), SUBORNO_STATUS_UPDATED);
    EXPECT_EQ(suborno_session_backspace(session.get()), SUBORNO_STATUS_UPDATED);
    std::array<char, 8> preedit{};
    ASSERT_EQ(suborno_session_snapshot(session.get(), preedit.data(), preedit.size()),
              SUBORNO_STATUS_UPDATED);
    EXPECT_STREQ(preedit.data(), "আ");
    // Dropping the last grapheme leaves the composition empty again.

    EXPECT_EQ(suborno_session_backspace(session.get()), SUBORNO_STATUS_EMPTY);

    // Snapshot argument guards: null session, null buffer and zero size.

    EXPECT_EQ(suborno_session_snapshot(nullptr, preedit.data(), preedit.size()),
              SUBORNO_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(suborno_session_snapshot(session.get(), nullptr, preedit.size()),
              SUBORNO_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(suborno_session_snapshot(session.get(), preedit.data(), 0),
              SUBORNO_ERROR_INVALID_ARGUMENT);

    ASSERT_EQ(suborno_session_append_utf8(session.get(), "ami"), SUBORNO_STATUS_UPDATED);
    // "আমি" needs 9 bytes plus the NUL: 9 is too small, 10 is exact.

    std::array<char, 9> too_small{};
    EXPECT_EQ(suborno_session_snapshot(session.get(), too_small.data(), too_small.size()),
              SUBORNO_ERROR_BUFFER_TOO_SMALL);
    std::array<char, 10> exact{};
    EXPECT_EQ(suborno_session_snapshot(session.get(), exact.data(), exact.size()),
              SUBORNO_STATUS_UPDATED);
    EXPECT_STREQ(exact.data(), "আমি");
}

TEST(CoverageApiSettings, CApiCandidateOutputGuardsCoverEveryRejectedArgument) {
    // Gap: c_api.cpp:152 (candidate_count invalid-argument return),

    // 166 (candidate_snapshot invalid-argument return), 180 (select_candidate

    // null-session guard) and 214 (commit_candidate invalid-argument return).

    SubornoEngineHandle engine = make_engine();
    ASSERT_NE(engine, nullptr);
    SubornoSessionHandle session = make_session(engine.get());
    ASSERT_NE(session, nullptr);
    ASSERT_EQ(suborno_session_append_utf8(session.get(), "ami"), SUBORNO_STATUS_UPDATED);

    std::size_t count = 7;
    EXPECT_EQ(suborno_session_candidate_count(session.get(), nullptr),
              SUBORNO_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(count, 7U);  // null output pointer leaves caller memory untouched

    EXPECT_EQ(suborno_session_candidate_count(nullptr, &count), SUBORNO_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(count, 0U);  // the count is zeroed even when the call is rejected

    std::array<char, 64> candidate{};
    EXPECT_EQ(suborno_session_candidate_snapshot(nullptr, 0, candidate.data(), candidate.size()),
              SUBORNO_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(suborno_session_candidate_snapshot(session.get(), 0, nullptr, candidate.size()),
              SUBORNO_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(suborno_session_candidate_snapshot(session.get(), 0, candidate.data(), 0),
              SUBORNO_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(suborno_session_candidate_snapshot(session.get(), 0, candidate.data(), 4),
              SUBORNO_ERROR_BUFFER_TOO_SMALL);
    EXPECT_EQ(candidate.data()[0], '\0');  // emptied before the size verdict

    EXPECT_EQ(suborno_session_candidate_snapshot(session.get(), 0, candidate.data(), 10),
              SUBORNO_STATUS_UPDATED);
    EXPECT_STREQ(candidate.data(), "আমি");

    EXPECT_EQ(suborno_session_select_candidate(nullptr, 0), SUBORNO_ERROR_INVALID_ARGUMENT);

    char* committed = nullptr;
    std::size_t committed_size = 0;
    EXPECT_EQ(suborno_session_commit_candidate(nullptr, 0, &committed, &committed_size),
              SUBORNO_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(suborno_session_commit_candidate(session.get(), 0, nullptr, &committed_size),
              SUBORNO_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(suborno_session_commit_candidate(session.get(), 0, &committed, nullptr),
              SUBORNO_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(committed, nullptr);
    EXPECT_EQ(committed_size, 0U);
}

// ---------------------------------------------------------------------------

// include/suborno/settings/settings.hpp + src/settings/settings.cpp

// ---------------------------------------------------------------------------

TEST(CoverageApiSettings, SettingsEveryKeyRejectsItsInvalidValuesWithTheExactError) {
    // Gap: settings.cpp:254,259,264,268,272,277,282,286,290,294,299,304,308,

    // 312,316,320,324,328,332,336,340,345,350,355,361,366,371,375,379,383,387,

    // 391,394 — the rejected-value return of every per-key branch — plus the

    // helper validators' failure arms: 65 (theme charset), 73 (empty numeric),

    // 79 (non-digit), 83 (size_t overflow), 97/100 (int parse/overflow),

    // 115 (bool literal), 142/163/184/205 (enum parse rejects).

    struct InvalidProbe {
        std::string key;
        std::string value;
    };
    const std::vector<InvalidProbe> probes{
        {"active_layout", ""},
        {"active_layout", "with=equals"},
        {"candidate_limit", ""},
        {"candidate_limit", "abc"},
        {"candidate_limit", "0"},
        {"candidate_limit", "101"},
        {"candidate_limit", "999999999999999999999999999999999"},
        {"learning_enabled", "yes"},
        {"remember_last_input_layout", "maybe"},
        {"last_ui_mode", "bogus"},
        {"last_bangla_layout_id", ""},
        {"start_with_windows", "1"},
        {"start_mode", "bogus"},
        {"switching_mode", "bogus"},
        {"theme_id", "Light"},
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
        {"topbar_position_x", "2147483648"},
        {"topbar_position_x", "-"},
        {"topbar_position_y", "32768"},
        {"layout_switch_shortcut", "F5"},
        {"disable_encoding_shortcut", "1"},
        {"map_numpad_to_bangla_numbers", "no"},
        {"change_locale_to_bn_bd_when_layout_not_english", "yeah"},
        {"ignore_caps_lock_state_in_bangla", "1"},
        {"select_candidate_using_number_keys", "true "},
        {"preferred_encoding", "utf8"},
        {"unknown_key", "anything"},
    };

    for (const InvalidProbe& probe : probes) {
        const SettingsLoadResult result =
            load_with_entry(probe.key, probe.value, "suborno_cov_api_settings_invalid_key.ini");
        EXPECT_EQ(result.status, SettingsStatus::invalid_format) << probe.key << "=" << probe.value;
        EXPECT_EQ(result.error, "Invalid or unknown settings key: " + probe.key)
            << probe.key << "=" << probe.value;
    }
}

TEST(CoverageApiSettings, SettingsEveryKeyAcceptsAValidNonDefaultValue) {
    // Gap: settings.cpp assignment arms of parse_settings_value (256, 261,

    // 274, 279, 296, 301, 342, 347, 357, 363, 368) plus the accepted arms of

    // every enum/bool/int parse helper — one file sets every key to a valid

    // value that differs from the struct defaults.

    std::vector<std::pair<std::string, std::string>> entries = mandatory_settings_entries();
    const std::vector<std::pair<std::string, std::string>> overrides{
        {"active_layout", "avro-phonetic"},
        {"candidate_limit", "100"},
        {"learning_enabled", "false"},
        {"remember_last_input_layout", "false"},
        {"start_with_windows", "true"},
        {"start_mode", "main-window"},
        {"switching_mode", "global"},
        {"last_ui_mode", "window"},
        {"last_bangla_layout_id", "probhat"},
        {"theme_id", "suborno-light"},
        {"font_family", "Noto Sans Bengali (Serif)"},
        {"show_candidate_window", "false"},
        {"next_word_prediction_enabled", "true"},
        {"candidate_orientation", "horizontal"},
        {"candidate_fixed_height", "false"},
        {"use_os_default_bangla_font", "false"},
        {"enable_sliding_effect", "false"},
        {"follow_caret", "false"},
        {"follow_mouse_pointer_if_caret_tracking_fails", "false"},
        {"idle_dimming_enabled", "false"},
        {"idle_opacity_percent", "100"},
        {"idle_time_seconds", "3600"},
        {"remember_topbar_position", "false"},
        {"topbar_position_x", "32767"},
        {"topbar_position_y", "-32768"},
        {"layout_switch_shortcut", "Win+Space"},
        {"disable_encoding_shortcut", "true"},
        {"map_numpad_to_bangla_numbers", "false"},
        {"change_locale_to_bn_bd_when_layout_not_english", "false"},
        {"ignore_caps_lock_state_in_bangla", "true"},
        {"select_candidate_using_number_keys", "false"},
        {"preferred_encoding", "ansi"},
    };
    for (const auto& override_entry : overrides) {
        bool replaced = false;
        for (auto& entry : entries) {
            if (entry.first == override_entry.first) {
                entry.second = override_entry.second;
                replaced = true;
            }
        }
        if (!replaced) {
            entries.push_back(override_entry);
        }
    }

    const std::filesystem::path path = probe_path("suborno_cov_api_settings_all_keys.ini");
    remove_quietly(path);
    std::string contents;
    for (const auto& entry : entries) {
        contents += entry.first + "=" + entry.second + "\n";
    }
    write_text_file(path, contents);
    const SettingsLoadResult result = SettingsStore(path).load();
    remove_quietly(path);

    ASSERT_EQ(result.status, SettingsStatus::ok) << result.error;
    EngineSettings expected;
    expected.active_layout_id = "avro-phonetic";
    expected.candidate_limit = 100;
    expected.learning_enabled = false;
    expected.general.remember_last_input_layout = false;
    expected.general.start_with_windows = true;
    expected.general.start_mode = StartMode::main_window;
    expected.general.switching_mode = SwitchingMode::global;
    expected.general.last_ui_mode = "window";
    expected.general.last_bangla_layout_id = "probhat";
    expected.ui.theme_id = "suborno-light";
    expected.ui.font_family = "Noto Sans Bengali (Serif)";
    expected.ui.show_candidate_window = false;
    expected.ui.next_word_prediction_enabled = true;
    expected.ui.candidate_orientation = CandidateOrientation::horizontal;
    expected.ui.candidate_fixed_height = false;
    expected.ui.use_os_default_bangla_font = false;
    expected.ui.enable_sliding_effect = false;
    expected.ui.follow_caret = false;
    expected.ui.follow_mouse_pointer_if_caret_tracking_fails = false;
    expected.topbar.idle_dimming_enabled = false;
    expected.topbar.idle_opacity_percent = 100;
    expected.topbar.idle_time_seconds = 3600;
    expected.topbar.remember_position = false;
    expected.topbar.position_x = 32767;
    expected.topbar.position_y = -32768;
    expected.input.layout_switch_shortcut = "Win+Space";
    expected.input.disable_encoding_shortcut = true;
    expected.input.map_numpad_to_bangla_numbers = false;
    expected.input.change_locale_to_bn_bd_when_layout_not_english = false;
    expected.input.ignore_caps_lock_state_in_bangla = true;
    expected.input.select_candidate_using_number_keys = false;
    expected.input.preferred_encoding = TextEncoding::ansi;

    EXPECT_EQ(result.settings.active_layout_id, "avro-phonetic");
    EXPECT_EQ(result.settings.candidate_limit, expected.candidate_limit);
    EXPECT_EQ(result.settings.learning_enabled, expected.learning_enabled);
    EXPECT_EQ(result.settings.general.remember_last_input_layout,
              expected.general.remember_last_input_layout);
    EXPECT_EQ(result.settings.general.start_with_windows, expected.general.start_with_windows);
    EXPECT_EQ(result.settings.general.start_mode, expected.general.start_mode);
    EXPECT_EQ(result.settings.general.switching_mode, expected.general.switching_mode);
    EXPECT_EQ(result.settings.general.last_ui_mode, expected.general.last_ui_mode);
    EXPECT_EQ(result.settings.general.last_bangla_layout_id,
              expected.general.last_bangla_layout_id);
    EXPECT_EQ(result.settings.ui.theme_id, expected.ui.theme_id);
    EXPECT_EQ(result.settings.ui.font_family, expected.ui.font_family);
    EXPECT_EQ(result.settings.ui.show_candidate_window, expected.ui.show_candidate_window);
    EXPECT_EQ(result.settings.ui.next_word_prediction_enabled,
              expected.ui.next_word_prediction_enabled);
    EXPECT_EQ(result.settings.ui.candidate_orientation, expected.ui.candidate_orientation);
    EXPECT_EQ(result.settings.ui.candidate_fixed_height, expected.ui.candidate_fixed_height);
    EXPECT_EQ(result.settings.ui.use_os_default_bangla_font,
              expected.ui.use_os_default_bangla_font);
    EXPECT_EQ(result.settings.ui.enable_sliding_effect, expected.ui.enable_sliding_effect);
    EXPECT_EQ(result.settings.ui.follow_caret, expected.ui.follow_caret);
    EXPECT_EQ(result.settings.ui.follow_mouse_pointer_if_caret_tracking_fails,
              expected.ui.follow_mouse_pointer_if_caret_tracking_fails);
    EXPECT_EQ(result.settings.topbar.idle_dimming_enabled, expected.topbar.idle_dimming_enabled);
    EXPECT_EQ(result.settings.topbar.idle_opacity_percent, expected.topbar.idle_opacity_percent);
    EXPECT_EQ(result.settings.topbar.idle_time_seconds, expected.topbar.idle_time_seconds);
    EXPECT_EQ(result.settings.topbar.remember_position, expected.topbar.remember_position);
    EXPECT_EQ(result.settings.topbar.position_x, expected.topbar.position_x);
    EXPECT_EQ(result.settings.topbar.position_y, expected.topbar.position_y);
    EXPECT_EQ(result.settings.input.layout_switch_shortcut, expected.input.layout_switch_shortcut);
    EXPECT_EQ(result.settings.input.disable_encoding_shortcut,
              expected.input.disable_encoding_shortcut);
    EXPECT_EQ(result.settings.input.map_numpad_to_bangla_numbers,
              expected.input.map_numpad_to_bangla_numbers);
    EXPECT_EQ(result.settings.input.change_locale_to_bn_bd_when_layout_not_english,
              expected.input.change_locale_to_bn_bd_when_layout_not_english);
    EXPECT_EQ(result.settings.input.ignore_caps_lock_state_in_bangla,
              expected.input.ignore_caps_lock_state_in_bangla);
    EXPECT_EQ(result.settings.input.select_candidate_using_number_keys,
              expected.input.select_candidate_using_number_keys);
    EXPECT_EQ(result.settings.input.preferred_encoding, expected.input.preferred_encoding);
}

TEST(CoverageApiSettings, SettingsEnumRoundTripCoversEverySerializerCase) {
    // Gap: settings.cpp:122-127 (start_mode_name last_state/topbar/main_window

    // arms), 136, 139-140 (parse_start_mode topbar/main-window), 158-161,

    // 170-173, 179-182, 200-203 — every named/parse arm of all four enums,

    // exercised through save()/load() round trips at the serialization

    // boundary (526, 527, 536, 567).

    const std::array<StartMode, 4> start_modes{
        StartMode::last_state, StartMode::topbar, StartMode::system_tray, StartMode::main_window};
    const std::array<SwitchingMode, 2> switching_modes{SwitchingMode::per_application_window,
                                                       SwitchingMode::global};
    const std::array<CandidateOrientation, 2> orientations{CandidateOrientation::vertical,
                                                           CandidateOrientation::horizontal};
    const std::array<TextEncoding, 2> encodings{TextEncoding::unicode, TextEncoding::ansi};

    const std::filesystem::path path = probe_path("suborno_cov_api_settings_enum_rt.ini");
    remove_quietly(path);
    const SettingsStore store(path);

    for (const StartMode start_mode : start_modes) {
        for (const SwitchingMode switching_mode : switching_modes) {
            for (const CandidateOrientation orientation : orientations) {
                for (const TextEncoding encoding : encodings) {
                    EngineSettings settings;
                    settings.general.start_mode = start_mode;
                    settings.general.switching_mode = switching_mode;
                    settings.ui.candidate_orientation = orientation;
                    settings.input.preferred_encoding = encoding;

                    ASSERT_EQ(store.save(settings), SettingsStatus::ok);
                    const SettingsLoadResult loaded = store.load();
                    ASSERT_EQ(loaded.status, SettingsStatus::ok) << loaded.error;
                    EXPECT_EQ(loaded.settings.general.start_mode, start_mode);
                    EXPECT_EQ(loaded.settings.general.switching_mode, switching_mode);
                    EXPECT_EQ(loaded.settings.ui.candidate_orientation, orientation);
                    EXPECT_EQ(loaded.settings.input.preferred_encoding, encoding);
                }
            }
        }
    }
    remove_quietly(path);
}

TEST(CoverageApiSettings, SettingsStructuralIoAndSaveErrorGuards) {
    // Gap: settings.cpp:433-434 (overlong line), 440-441 (malformed line),

    // 447-448 (duplicate key), 454-455 (invalid version), 464-465 (unknown

    // key), 479-480 (missing version), 489-490 (missing required keys),

    // 473-476 (read error), 408-416 (missing-file defaults) and 513

    // (save's create-directories failure when the parent is a regular file).

    const std::string file_name = "suborno_cov_api_settings_structural.ini";

    // A line above maximum_settings_line_length is rejected outright.

    {
        const SettingsLoadResult result =
            load_with_entry("active_layout", std::string(4200, 'a'), file_name);
        EXPECT_EQ(result.status, SettingsStatus::invalid_format);
        EXPECT_EQ(result.error, "Settings line exceeds the maximum supported length");
    }
    // A line without '=' is malformed.

    {
        const std::filesystem::path path = probe_path(file_name);
        remove_quietly(path);
        write_text_file(path, "version=2\nthis-line-has-no-separator\n");
        const SettingsLoadResult loaded = SettingsStore(path).load();
        remove_quietly(path);
        EXPECT_EQ(loaded.status, SettingsStatus::invalid_format);
        EXPECT_EQ(loaded.error, "Malformed settings line");
    }
    // Repeating a key is a duplicate.

    {
        const std::filesystem::path path = probe_path(file_name);
        remove_quietly(path);
        write_text_file(path, "version=2\nversion=2\n");
        const SettingsLoadResult loaded = SettingsStore(path).load();
        remove_quietly(path);
        EXPECT_EQ(loaded.status, SettingsStatus::invalid_format);
        EXPECT_EQ(loaded.error, "Duplicate settings key");
    }
    // A non-numeric version is invalid.

    {
        const SettingsLoadResult result = load_with_entry("version", "x2", file_name);
        EXPECT_EQ(result.status, SettingsStatus::invalid_format);
        EXPECT_EQ(result.error, "Invalid settings version");
    }
    // A version beyond 2 is unsupported.

    {
        const SettingsLoadResult result = load_with_entry("version", "3", file_name);
        EXPECT_EQ(result.status, SettingsStatus::unsupported_version);
        EXPECT_EQ(result.error, "Unsupported settings version");
    }
    // Missing version line.

    {
        const std::filesystem::path path = probe_path(file_name);
        remove_quietly(path);
        write_text_file(path,
                        "active_layout=suborno-phonetic\ncandidate_limit=16\n"
                        "learning_enabled=true\n");
        const SettingsLoadResult loaded = SettingsStore(path).load();
        remove_quietly(path);
        EXPECT_EQ(loaded.status, SettingsStatus::invalid_format);
        EXPECT_EQ(loaded.error, "Missing settings version");
    }
    // Missing one of the three mandatory engine keys.

    {
        const std::filesystem::path path = probe_path(file_name);
        remove_quietly(path);
        write_text_file(path, "version=2\nactive_layout=suborno-phonetic\n");
        const SettingsLoadResult loaded = SettingsStore(path).load();
        remove_quietly(path);
        EXPECT_EQ(loaded.status, SettingsStatus::invalid_format);
        EXPECT_EQ(loaded.error, "Missing required engine settings");
    }
    // A missing file is the documented first-run outcome: defaults, ok, and

    // file_existed=false; an existing directory reports io_error instead.

    {
        const std::filesystem::path path = probe_path(file_name);
        remove_quietly(path);
        const SettingsLoadResult missing = SettingsStore(path).load();
        EXPECT_EQ(missing.status, SettingsStatus::ok);
        EXPECT_FALSE(missing.file_existed);
        EXPECT_TRUE(missing.error.empty());
        EXPECT_EQ(missing.settings.candidate_limit, std::size_t{16});

        std::error_code ignored;
        std::filesystem::create_directories(path);
        const SettingsLoadResult directory = SettingsStore(path).load();
        std::filesystem::remove_all(path, ignored);
        EXPECT_EQ(directory.status, SettingsStatus::io_error);
        EXPECT_TRUE(directory.file_existed);
#if defined(_WIN32)
        EXPECT_TRUE(directory.error.empty());
#else
        EXPECT_EQ(directory.error, "Failed while reading settings");
#endif
    }
    // save() whose parent path is an existing regular file cannot create the

    // parent directory chain and reports io_error without touching anything.

    {
        const auto blocker = probe_path("suborno_cov_api_settings_parent_blocker.ini");
        remove_quietly(blocker);
        write_text_file(blocker, "blocker");
        const SettingsStatus status =
            SettingsStore(blocker / "settings.ini").save(EngineSettings{});
        EXPECT_EQ(status, SettingsStatus::io_error);
        EXPECT_TRUE(std::filesystem::is_regular_file(blocker));
        remove_quietly(blocker);
    }
}

TEST(CoverageApiSettings, SettingsFileStampEqualityAndMissingPathBehavior) {
    // Gap: settings.hpp:127-130 — the FileStamp fields and defaulted

    // operator== are only instantiated when callers actually compare stamps;

    // settings.cpp:583 — a missing path yields the default stamp.

    SettingsStore::FileStamp equal_first{true, 128, 1234567};
    SettingsStore::FileStamp equal_second{true, 128, 1234567};
    SettingsStore::FileStamp different_size{true, 256, 1234567};
    SettingsStore::FileStamp different_time{true, 128, 7654321};
    SettingsStore::FileStamp different_exists{false, 128, 1234567};

    EXPECT_TRUE(equal_first == equal_second);
    EXPECT_FALSE(equal_first != equal_second);
    EXPECT_FALSE(equal_first == different_size);
    EXPECT_FALSE(equal_first == different_time);
    EXPECT_FALSE(equal_first == different_exists);
    EXPECT_TRUE(equal_first != different_exists);

    // A missing file reports exists=false with default size/time (583).

    const auto missing_path = probe_path("suborno_cov_api_settings_stamp_missing.ini");
    remove_quietly(missing_path);
    const SettingsStore::FileStamp missing = SettingsStore(missing_path).file_stamp();
    EXPECT_EQ(missing, SettingsStore::FileStamp{});
    EXPECT_FALSE(missing.exists);
    EXPECT_EQ(missing.size, 0U);
    EXPECT_EQ(missing.mtime_ticks, 0);
}

// ---------------------------------------------------------------------------

// src/persistence/atomic_file.cpp

// ---------------------------------------------------------------------------

#if !defined(_WIN32)
TEST(CoverageApiSettings, AtomicWriteSurfacesWriteFailureAndRemovesTheTemporaryFile) {
    // Gap: atomic_file.cpp:128-129 (write() error break) and 144-145 (the

    // temporary file removal on a failed write). RLIMIT_FSIZE=0 makes the

    // first write() fail with EFBIG deterministically; SIGXFSZ is ignored for

    // the duration so the failure is an errno, not a process kill. The limit

    // and signal disposition are restored before any other assertion runs.

    const auto directory =
        std::filesystem::temp_directory_path() / "suborno_cov_api_atomic_write_fail_dir";
    remove_quietly(directory);
    ASSERT_TRUE(std::filesystem::create_directories(directory));
    const auto destination = directory / "settings.conf";

    void (*previous_handler)(int) = std::signal(SIGXFSZ, SIG_IGN);
    ASSERT_NE(previous_handler, SIG_ERR);
    rlimit previous_limit{};
    ASSERT_EQ(::getrlimit(RLIMIT_FSIZE, &previous_limit), 0);
    rlimit zero_limit = previous_limit;
    zero_limit.rlim_cur = 0;
    ASSERT_EQ(::setrlimit(RLIMIT_FSIZE, &zero_limit), 0);

    const std::error_code error =
        suborno::persistence::write_file_atomically(destination, "payload");

    ASSERT_EQ(::setrlimit(RLIMIT_FSIZE, &previous_limit), 0);
    std::signal(SIGXFSZ, previous_handler);

    EXPECT_EQ(error, std::make_error_code(std::errc::file_too_large));
    EXPECT_FALSE(std::filesystem::exists(destination));
    // The half-written temporary file was removed: the directory is empty.

    std::size_t entry_count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        static_cast<void>(entry);
        ++entry_count;
    }
    EXPECT_EQ(entry_count, 0U);
    EXPECT_EQ(count_temporary_files(directory), 0U);

    remove_quietly(directory);
}
#endif

#if defined(_WIN32)
TEST(CoverageApiSettings, AtomicQuarantineReplacesAnExistingTargetOnWindowsRename) {
    // std::filesystem::rename follows the standard replace semantics on every

    // supported platform: an existing regular-file target is REPLACED (the

    // MSVC implementation is MoveFileExW with MOVEFILE_REPLACE_EXISTING). The

    // earlier draft of this test assumed MoveFile refuses existing targets;

    // the real MSVC run of 2026-11-25 proved the replace semantics (the

    // quarantine reported success and the occupant was replaced), so this

    // test now asserts the real contract, mirroring the POSIX test below:

    // the first attempt wins and the stale target's bytes are replaced by

    // the quarantined file.

    const auto directory =
        std::filesystem::temp_directory_path() / "suborno_cov_api_quarantine_retry_dir";
    remove_quietly(directory);
    ASSERT_TRUE(std::filesystem::create_directories(directory));
    const auto path = directory / "settings.ini";
    write_text_file(path, "precious");

    const auto colliding_target = quarantine_collision_target(path, 0);
    write_text_file(colliding_target, "occupied");

    EXPECT_EQ(suborno::persistence::quarantine_file(path), std::error_code{});
    EXPECT_FALSE(std::filesystem::exists(path));
    EXPECT_EQ(read_text_file(colliding_target), "precious");
    // No retry happened: the attempt-1 target was never created.

    EXPECT_FALSE(std::filesystem::exists(quarantine_collision_target(path, 1)));

    remove_quietly(directory);
}

TEST(CoverageApiSettings, AtomicQuarantineSurfacesADirectoryTargetConflictOnWindows) {
    // The retry loop's file_exists branch is unreachable for regular-file

    // targets under the standard replace semantics. The one Windows-

    // observable collision is a DIRECTORY occupying the target name:

    // MoveFileExW with MOVEFILE_REPLACE_EXISTING refuses to replace a

    // directory with a file (ERROR_ACCESS_DENIED), and the implementation

    // reports that failure as-is instead of retrying, leaving the original

    // file untouched.

    const auto directory =
        std::filesystem::temp_directory_path() / "suborno_cov_api_quarantine_dir_conflict";
    remove_quietly(directory);
    ASSERT_TRUE(std::filesystem::create_directories(directory));
    const auto path = directory / "settings.ini";
    write_text_file(path, "precious");

    const auto blocked_target = quarantine_collision_target(path, 0);
    ASSERT_TRUE(std::filesystem::create_directories(blocked_target));

    const std::error_code error = suborno::persistence::quarantine_file(path);
    EXPECT_NE(error, std::error_code{});  // reported, not swallowed

    EXPECT_FALSE(error == std::errc::no_such_file_or_directory);
    // Nothing moved: the original survives with its bytes, and the blocking

    // directory is still in place.

    EXPECT_EQ(read_text_file(path), "precious");
    EXPECT_TRUE(std::filesystem::is_directory(blocked_target));

    remove_quietly(directory);
}
#else
TEST(CoverageApiSettings, AtomicQuarantineReplacesAnExistingTargetOnPosixRename) {
    // POSIX rename(2) atomically REPLACES an existing regular-file target, so

    // the collision-retry loop (atomic_file.cpp:192) and the exhaustion return

    // (:194) are unreachable here by design — they are Windows-only branches.

    // The POSIX contract is: the first attempt wins and the stale target's

    // bytes are replaced by the quarantined file.

    const auto directory =
        std::filesystem::temp_directory_path() / "suborno_cov_api_quarantine_retry_dir";
    remove_quietly(directory);
    ASSERT_TRUE(std::filesystem::create_directories(directory));
    const auto path = directory / "settings.ini";
    write_text_file(path, "precious");

    const auto colliding_target = quarantine_collision_target(path, 0);
    write_text_file(colliding_target, "occupied");

    EXPECT_EQ(suborno::persistence::quarantine_file(path), std::error_code{});
    EXPECT_FALSE(std::filesystem::exists(path));
    EXPECT_EQ(read_text_file(colliding_target), "precious");
    // No retry happened: the attempt-1 target was never created.

    EXPECT_FALSE(std::filesystem::exists(quarantine_collision_target(path, 1)));

    remove_quietly(directory);
}
#endif

#if !defined(_WIN32)
TEST(CoverageApiSettings, AtomicQuarantineTreatsAVirtualFileProbeAsNothingToQuarantine) {
    // Gap: atomic_file.cpp:182-185 — the no_such_file_or_directory branch of

    // the rename loop. A procfs entry stats fine (the existence probe passes)

    // but procfs cannot create the ".corrupt-*" target entry, so the rename

    // itself fails with ENOENT; the code must treat that exactly like a file

    // that vanished between probe and rename and report success. Verified

    // empirically: rename("/proc/self/stat", "/proc/self/stat.*") -> ENOENT.

    const std::filesystem::path proc_file("/proc/self/stat");
    ASSERT_TRUE(std::filesystem::exists(proc_file));

    EXPECT_EQ(suborno::persistence::quarantine_file(proc_file), std::error_code{});
    // Nothing was moved and no ".corrupt-*" name was created in procfs.

    EXPECT_TRUE(std::filesystem::exists(proc_file));
    EXPECT_FALSE(std::filesystem::exists(proc_file.string() + ".corrupt-0"));
}
#endif

#if !defined(_WIN32)
TEST(CoverageApiSettings, AtomicQuarantineSurfacesForeignRenameErrorsUnchanged) {
    // Gap: atomic_file.cpp:189-190 — a rename failure that is neither

    // file_exists nor no_such_file_or_directory is returned as-is. Stripping

    // write permission from the parent directory makes the rename fail with

    // EACCES for every non-root caller. As root, permission bits are

    // bypassed, so the documented success path is asserted instead; either

    // way the directory is restored and cleaned up.

    const auto directory =
        std::filesystem::temp_directory_path() / "suborno_cov_api_quarantine_foreign_dir";
    remove_quietly(directory);
    ASSERT_TRUE(std::filesystem::create_directories(directory));
    const auto path = directory / "settings.ini";
    write_text_file(path, "precious");

    const bool running_as_root = ::geteuid() == 0;
    ASSERT_EQ(::chmod(directory.c_str(), running_as_root ? 0755 : 0555), 0);

    const std::error_code error = suborno::persistence::quarantine_file(path);
    if (running_as_root) {
        EXPECT_EQ(error, std::error_code{});
        EXPECT_FALSE(std::filesystem::exists(path));
    } else {
        EXPECT_EQ(error, std::make_error_code(std::errc::permission_denied));
        // The file was left exactly where it was.

        EXPECT_TRUE(std::filesystem::exists(path));
        EXPECT_EQ(read_text_file(path), "precious");
        EXPECT_FALSE(std::filesystem::exists(path.string() + ".corrupt-0"));
    }

    ASSERT_EQ(::chmod(directory.c_str(), 0755), 0);
    remove_quietly(directory);
}
#endif

// ---------------------------------------------------------------------------

// src/core/engine.cpp

// ---------------------------------------------------------------------------

TEST(CoverageApiSettings, EngineStorelessSettingsAndDictionaryAccessors) {
    // Gap: engine.cpp:195-197 (save_settings without a store), 210-215

    // (load_settings without a store mirrors in-memory settings) and

    // 265-266 (the const dictionary() overload without a dictionary).

    suborno::core::Engine engine;
    engine.set_next_word_prediction_enabled(true);

    EXPECT_EQ(engine.save_settings(EngineSettings{}), suborno::settings::SettingsStatus::io_error);

    const SettingsLoadResult loaded = engine.load_settings();
    EXPECT_EQ(loaded.status, SettingsStatus::ok);
    EXPECT_FALSE(loaded.file_existed);
    EXPECT_EQ(loaded.settings.active_layout_id, "suborno-phonetic");
    EXPECT_EQ(loaded.settings.candidate_limit, std::size_t{16});
    EXPECT_TRUE(loaded.settings.ui.next_word_prediction_enabled);

    const suborno::core::Engine& const_view = engine;
    EXPECT_EQ(const_view.dictionary(), nullptr);
    EXPECT_EQ(engine.dictionary(), nullptr);
}

TEST(CoverageApiSettings, EngineRememberDisabledRestoresTheDefaultLayout) {
    // Gap: engine.cpp:58 — the remember_last_input_layout=false ternary arm

    // forces the default active layout regardless of the stored id.

    const std::filesystem::path settings_path =
        probe_path("suborno_cov_api_engine_remember_off.conf");
    remove_quietly(settings_path);
    EngineSettings stored;
    stored.active_layout_id = "probhat";
    stored.candidate_limit = 9;
    stored.general.remember_last_input_layout = false;
    ASSERT_EQ(SettingsStore(settings_path).save(stored), SettingsStatus::ok);

    suborno::core::EngineConfig config;
    config.settings_path = settings_path;
    suborno::core::Engine engine(config);

    EXPECT_EQ(engine.active_layout_id(), "suborno-phonetic");
    EXPECT_EQ(engine.settings().active_layout_id, "suborno-phonetic");
    EXPECT_EQ(engine.settings().candidate_limit, std::size_t{9});
    EXPECT_FALSE(engine.settings().general.remember_last_input_layout);

    // The restored default layout really drives new sessions.

    auto session = engine.create_session();
    ASSERT_NE(session, nullptr);
    ASSERT_EQ(session->append_utf8("ami"), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "আমি");

    remove_quietly(settings_path);
}

TEST(CoverageApiSettings, EngineReloadSettingsSurfacesFailuresWithoutStateChange) {
    // Gap: engine.cpp:227 — reload_settings returns a failed load result

    // unchanged, leaving the in-memory settings and active layout intact.

    const std::filesystem::path settings_path =
        probe_path("suborno_cov_api_engine_reload_fail.conf");
    remove_quietly(settings_path);
    EngineSettings stored;
    stored.candidate_limit = 9;
    ASSERT_EQ(SettingsStore(settings_path).save(stored), SettingsStatus::ok);

    suborno::core::EngineConfig config;
    config.settings_path = settings_path;
    suborno::core::Engine engine(config);
    ASSERT_EQ(engine.settings().candidate_limit, std::size_t{9});

    // Corrupt the file behind the engine's back.

    write_text_file(settings_path, "this-is-not-a-settings-file\n");

    const SettingsLoadResult reloaded = engine.reload_settings();
    EXPECT_EQ(reloaded.status, SettingsStatus::invalid_format);
    EXPECT_EQ(reloaded.error, "Malformed settings line");
    // The engine's own state is untouched by the failed reload.

    EXPECT_EQ(engine.settings().candidate_limit, std::size_t{9});
    EXPECT_EQ(engine.active_layout_id(), "suborno-phonetic");

    remove_quietly(settings_path);
}

TEST(CoverageApiSettings, EngineReloadIfChangedSkipsUntilTheFileActuallyChanges) {
    // Gap: engine.cpp:240-255 — the no-store branch (241-246), the

    // unchanged-stamp branch (248-252) and the changed-stamp reload (254).

    const std::filesystem::path settings_path =
        probe_path("suborno_cov_api_engine_reload_if_changed.conf");
    remove_quietly(settings_path);

    {
        // Without a store, the in-memory settings come straight back.

        suborno::core::Engine storeless;
        storeless.set_next_word_prediction_enabled(true);
        const SettingsLoadResult storeless_result = storeless.reload_settings_if_changed();
        EXPECT_EQ(storeless_result.status, SettingsStatus::ok);
        EXPECT_TRUE(storeless_result.settings.ui.next_word_prediction_enabled);
    }

    suborno::core::EngineConfig config;
    config.settings_path = settings_path;
    suborno::core::Engine engine(config);

    // Missing file: the stamp matches the never-set baseline, so no reload

    // happens and the current settings are returned unchanged.

    const SettingsLoadResult unchanged = engine.reload_settings_if_changed();
    EXPECT_EQ(unchanged.status, SettingsStatus::ok);
    EXPECT_EQ(unchanged.settings.candidate_limit, std::size_t{16});
    EXPECT_EQ(engine.settings().candidate_limit, std::size_t{16});

    // Persist new settings through the engine itself: the stamp now differs

    // from the baseline, so the next poll reloads from disk.

    EngineSettings updated;
    updated.candidate_limit = 20;
    ASSERT_EQ(engine.save_settings(updated), SettingsStatus::ok);
    EXPECT_EQ(engine.settings().candidate_limit, std::size_t{20});

    const SettingsLoadResult reloaded = engine.reload_settings_if_changed();
    EXPECT_EQ(reloaded.status, SettingsStatus::ok);
    EXPECT_EQ(reloaded.settings.candidate_limit, std::size_t{20});
    EXPECT_EQ(engine.settings().candidate_limit, std::size_t{20});

    // Nothing changed since the last reload: the stamp matches and the

    // in-memory settings are returned without touching the file again.

    const SettingsLoadResult steady = engine.reload_settings_if_changed();
    EXPECT_EQ(steady.status, SettingsStatus::ok);
    EXPECT_EQ(steady.settings.candidate_limit, std::size_t{20});

    // An external write with a different content length changes the stamp.

    EngineSettings external;
    external.candidate_limit = 7;
    ASSERT_EQ(SettingsStore(settings_path).save(external), SettingsStatus::ok);
    const SettingsLoadResult picked_up = engine.reload_settings_if_changed();
    EXPECT_EQ(picked_up.status, SettingsStatus::ok);
    EXPECT_EQ(picked_up.settings.candidate_limit, std::size_t{7});
    EXPECT_EQ(engine.settings().candidate_limit, std::size_t{7});

    remove_quietly(settings_path);
}

TEST(CoverageApiSettings, EngineConfiguredDictionaryIsExposedThroughBothAccessors) {
    // Gap: engine.cpp:265-266 with a live dictionary — the const overload

    // returns the same instance as the mutating one.

    const std::filesystem::path dictionary_path =
        probe_path("suborno_cov_api_engine_dictionary.sqlite");
    remove_quietly(dictionary_path);

    suborno::core::EngineConfig config;
    config.dictionary_path = dictionary_path;
    suborno::core::Engine engine(config);
    ASSERT_NE(engine.dictionary(), nullptr);

    const suborno::core::Engine& const_view = engine;
    EXPECT_EQ(const_view.dictionary(), engine.dictionary());

    // The dictionary actually serves lookups for new sessions.

    engine.dictionary()->add_entry("বাংলা", "ab", 10);
    auto session = engine.create_session();
    ASSERT_NE(session, nullptr);
    ASSERT_EQ(session->append_utf8("ab"), suborno::layout::InputStatus::updated);
    const auto snapshot = session->snapshot();
    ASSERT_FALSE(snapshot.candidates.empty());
    EXPECT_EQ(snapshot.candidates.front().text, "বাংলা");

    remove_quietly(dictionary_path);
}

// ---------------------------------------------------------------------------

// src/candidates/candidate_engine.cpp

// ---------------------------------------------------------------------------

TEST(CoverageApiSettings, CandidateExactStageBreaksBeforeTheRawInputQuery) {
    // Gap: candidate_engine.cpp:73 — the result-full break inside

    // append_dictionary_matches: the normalized query fills the list to the

    // limit, so the second (raw-input) exact query must stop on its first

    // match instead of overflowing the candidate limit.

    const std::string db_path = temporary_database_path("suborno_cov_api_cand_exact.sqlite");
    remove_database(db_path);
    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("বাংলা", "বাংলা", 10);
    dictionary.add_entry("ভাষা", "ab", 5);

    const ProbeLayout layout;
    const suborno::candidates::CandidateEngine engine(layout, dictionary);
    const auto candidates = engine.make_candidates("ab", 1);

    ASSERT_EQ(candidates.size(), 1U);
    EXPECT_EQ(candidates[0].text, "বাংলা");
    EXPECT_EQ(candidates[0].rationale, "exact dictionary match");
    EXPECT_DOUBLE_EQ(candidates[0].score, 1.0);
    // The raw-input match ("ভাষা" via normalized "ab") was cut off by the

    // break, not silently appended past the limit.

    remove_database(db_path);
}

TEST(CoverageApiSettings, CandidatePredictionStageBreaksWhenTheListIsAlreadyFull) {
    // Gap: candidate_engine.cpp:105 — the result-full break in the next-word

    // prediction stage: an exact match occupies the only slot, so the

    // transition follower must not be pushed.

    const std::string db_path = temporary_database_path("suborno_cov_api_cand_predict.sqlite");
    remove_database(db_path);
    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("বাংলা", "বাংলা", 10);
    dictionary.record_transition("ami", "বাংলাদেশ");

    const ProbeLayout layout;
    const suborno::candidates::CandidateEngine engine(layout, dictionary);
    const auto candidates = engine.make_candidates("বাংলা", 1, "ami");

    ASSERT_EQ(candidates.size(), 1U);
    EXPECT_EQ(candidates[0].text, "বাংলা");
    EXPECT_EQ(candidates[0].rationale, "exact dictionary match");
    for (const auto& candidate : candidates) {
        EXPECT_NE(candidate.rationale, "next-word prediction");
    }

    remove_database(db_path);
}

TEST(CoverageApiSettings, CandidatePrefixStageBreaksOnceTheCandidateLimitIsReached) {
    // Gap: candidate_engine.cpp:136 — the accepted/full break in the second

    // append_prefix_matches query. With limit 5 the raw-input prefix query

    // fills three slots, and the normalized query pushes the list to 5, so

    // the third normalized match must be cut off by the break.

    const std::string db_path = temporary_database_path("suborno_cov_api_cand_prefix.sqlite");
    remove_database(db_path);
    suborno::dictionary::Dictionary dictionary(db_path);
    // Prefix families "ab*" and "বাংলা*": no exact match for either query,

    // so stages 1/2 stay empty and both prefix queries run.

    dictionary.add_entry("কাল", "ab1", 9);
    dictionary.add_entry("কাম", "ab2", 7);
    dictionary.add_entry("কাজ", "ab3", 5);
    dictionary.add_entry("বাংলাদেশ", "বাংলাদেশ", 8);
    dictionary.add_entry("বাংলার", "বাংলার", 6);
    dictionary.add_entry("বাংলায়", "বাংলায়", 4);

    const ProbeLayout layout;
    const suborno::candidates::CandidateEngine engine(layout, dictionary);
    const auto candidates = engine.make_candidates("ab", 5);

    // 3 raw-input prefix matches + 2 normalized prefix matches; the third

    // normalized match was dropped by the break.

    ASSERT_EQ(candidates.size(), 5U);
    EXPECT_EQ(candidates[0].rationale, "normalized input prefix match");
    EXPECT_EQ(candidates[1].rationale, "normalized input prefix match");
    EXPECT_EQ(candidates[2].rationale, "normalized input prefix match");
    EXPECT_EQ(candidates[3].rationale, "transliterated prefix match");
    EXPECT_EQ(candidates[4].rationale, "transliterated prefix match");
    for (const auto& candidate : candidates) {
        EXPECT_EQ(candidate.source, suborno::layout::CandidateSource::system_dictionary);
    }
    EXPECT_DOUBLE_EQ(candidates[0].score, 0.75);
    EXPECT_DOUBLE_EQ(candidates[3].score, 0.7);

    remove_database(db_path);
}

TEST(CoverageApiSettings, CandidateSpellingStageAppendsCappedSuggestions) {
    // Gap: candidate_engine.cpp:168-170 (the two-term cap break) and

    // 172-177 (the suggestion adoption loop: source/rationale/score rewrite

    // and push). A complete-but-unknown word with five one-edit dictionary

    // neighbors and no prefix completions enters stage 4; the engine must

    // keep at most maximum_spell_suggestions of them.

    const std::string db_path = temporary_database_path("suborno_cov_api_cand_spell.sqlite");
    remove_database(db_path);
    suborno::dictionary::Dictionary dictionary(db_path);
    // Single-grapheme deletions and transpositions of "ami" (the first edit

    // candidates the spell checker looks up) — none of them starts with

    // "ami", so stage 3 finds no prefix completion and stage 4 fires.

    dictionary.add_entry("মা", "mi", 5);
    dictionary.add_entry("আই", "ai", 5);
    dictionary.add_entry("আম", "am", 5);
    dictionary.add_entry("মাই", "mai", 5);
    dictionary.add_entry("আইম", "aim", 5);

    const ProbeLayout layout;
    const suborno::candidates::CandidateEngine engine(layout, dictionary);
    const auto candidates = engine.make_candidates("ami", 4);

    ASSERT_EQ(candidates.size(), 4U);
    for (std::size_t index = 0; index < 3; ++index) {
        EXPECT_EQ(candidates[index].source, suborno::layout::CandidateSource::system_dictionary);
        EXPECT_EQ(candidates[index].rationale, "spelling suggestion");
    }
    EXPECT_DOUBLE_EQ(candidates[0].score, 0.95);
    EXPECT_DOUBLE_EQ(candidates[1].score, 0.95 / 2.0);
    EXPECT_DOUBLE_EQ(candidates[2].score, 0.95 / 3.0);
    // The layout alternative takes the last guaranteed slot.

    EXPECT_EQ(candidates[3].text, "বাংলা");
    EXPECT_EQ(candidates[3].source, suborno::layout::CandidateSource::layout);
    EXPECT_DOUBLE_EQ(candidates[3].score, 0.25);

    remove_database(db_path);
}
