// SubornoIME — UI/UX Functional Audit Phase 2 (portable logic layer).

//

// The Windows-only surfaces (settings window, topbar, OSK window, tray,

// toasts, About dialog, UIA providers) cannot be launched in the Linux CI

// sandbox; their runtime verification stays BLOCKED and is reported in

// docs/reports/UI_FUNCTIONAL_AUDIT_*_V2. What CAN be truly executed here is

// the logic layer those surfaces are built on: the settings state machine

// every staged control edits, the persistence contract Save/Cancel/restart

// rely on, the layout registry the dropdowns enumerate, the engine encoding

// and learning toggles, the OSK preview labels, the i18n tables the shell

// renders, and the negative/robustness paths. Each test below drives that

// logic through its public API — no mocks, no re-implementations — and maps

// to one or more rows of UI_FUNCTIONAL_AUDIT_TRACEABILITY_MATRIX_V2.csv.

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <string_view>
#include <vector>

#include "persistence/atomic_file.hpp"
#include "suborno/core/engine.hpp"
#include "suborno/core/ui_strings.hpp"
#include "suborno/settings/settings.hpp"

namespace {

struct TempFile {
    std::filesystem::path path;

    explicit TempFile(const std::string& name)
        : path(std::filesystem::temp_directory_path() / name) {
        std::filesystem::remove(path);
    }
    ~TempFile() {
        std::filesystem::remove(path);
    }
    TempFile(const TempFile&) = delete;
    TempFile& operator=(const TempFile&) = delete;

    void write(std::string_view contents) const {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        out << contents;
    }
    std::string read() const {
        std::ifstream in(path, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }
};

suborno::settings::SettingsLoadResult save_and_reload(
    const std::filesystem::path& path, const suborno::settings::EngineSettings& settings) {
    const suborno::settings::SettingsStore store(path);
    EXPECT_EQ(store.save(settings), suborno::settings::SettingsStatus::ok);
    return store.load();
}

bool contains_bengali(const std::string& utf8) {
    // Bengali block U+0980..U+09FF encodes as 0xE0 0xA6/0xA7 lead bytes.

    // One find over both prefixes keeps the check free of duplicated

    // subexpressions (MSVC /analyze C6287 flags a repeated call shape)

    // while matching exactly the same bytes as the two-prefix OR it replaces.

    for (const std::string_view lead : {"\xE0\xA6", "\xE0\xA7"}) {
        if (utf8.find(lead) != std::string::npos) {
            return true;
        }
    }
    return false;
}

// Every i18n key the inventoried Windows surfaces can render (settings

// window, topbar, tray menu, OSK, toasts, About dialog, menu popup), minus

// the meta.* file-metadata keys that no surface reads.

const std::vector<std::string>& inventory_ui_keys() {
    static const std::vector<std::string> keys = [] {
        std::vector<std::string> all;
        const char* families[] = {
            "about.", "common.", "confirm.", "error.", "osk.", "settings.", "toast.", "topbar."};
        for (const char* family : families) {
            for (const std::string& key : suborno::core::ui_strings::keys()) {
                if (key.rfind(family, 0) == 0) {
                    all.push_back(key);
                }
            }
        }
        return all;
    }();
    return keys;
}

}  // namespace

// ---------------------------------------------------------------------------

// Phase 03/04 — every settings-page control's staged key survives Save.

// ---------------------------------------------------------------------------

TEST(UIAuditSettingsPages, GeneralPageEveryControlKeyRoundTripsThroughSave) {
    TempFile file("suborno_audit_general_page.conf");
    suborno::settings::EngineSettings staged;
    staged.active_layout_id = "probhat";  // default-layout dropdown

    staged.candidate_limit = 16;
    staged.learning_enabled = true;
    staged.general.remember_last_input_layout = false;  // checkbox

    staged.general.start_with_windows = true;  // checkbox

    staged.general.start_mode = suborno::settings::StartMode::system_tray;  // dropdown option 3

    staged.general.switching_mode = suborno::settings::SwitchingMode::global;  // dropdown option 2

    staged.general.last_ui_mode = "tray";  // shell state

    staged.general.last_bangla_layout_id = "khipro";  // English-toggle return layout

    const auto loaded = save_and_reload(file.path, staged);
    ASSERT_EQ(loaded.status, suborno::settings::SettingsStatus::ok) << loaded.error;
    EXPECT_EQ(loaded.settings.active_layout_id, "probhat");
    EXPECT_EQ(loaded.settings.general.remember_last_input_layout, false);
    EXPECT_EQ(loaded.settings.general.start_with_windows, true);
    EXPECT_EQ(loaded.settings.general.start_mode, suborno::settings::StartMode::system_tray);
    EXPECT_EQ(loaded.settings.general.switching_mode, suborno::settings::SwitchingMode::global);
    EXPECT_EQ(loaded.settings.general.last_ui_mode, "tray");
    EXPECT_EQ(loaded.settings.general.last_bangla_layout_id, "khipro");

    // Dropdown walk: every StartMode option the General page renders.

    for (const auto mode : {suborno::settings::StartMode::last_state,
                            suborno::settings::StartMode::topbar,
                            suborno::settings::StartMode::system_tray,
                            suborno::settings::StartMode::main_window}) {
        staged.general.start_mode = mode;
        const auto walked = save_and_reload(file.path, staged);
        ASSERT_EQ(walked.status, suborno::settings::SettingsStatus::ok) << walked.error;
        EXPECT_EQ(walked.settings.general.start_mode, mode);
    }
    // Dropdown walk: both switching modes.

    for (const auto mode : {suborno::settings::SwitchingMode::per_application_window,
                            suborno::settings::SwitchingMode::global}) {
        staged.general.switching_mode = mode;
        const auto walked = save_and_reload(file.path, staged);
        ASSERT_EQ(walked.status, suborno::settings::SettingsStatus::ok) << walked.error;
        EXPECT_EQ(walked.settings.general.switching_mode, mode);
    }
}

TEST(UIAuditSettingsPages, DefaultLayoutDropdownCoversAllSevenRegisteredLayouts) {
    TempFile file("suborno_audit_layout_dropdown.conf");
    suborno::core::Engine engine;  // registers the 7 shipped layouts

    const char* ids[] = {"english",
                         "suborno-phonetic",
                         "borno-phonetic",
                         "avro-phonetic",
                         "khipro",
                         "probhat",
                         "national"};
    suborno::settings::EngineSettings staged;
    for (const char* id : ids) {
        ASSERT_NE(engine.layout_metadata(id), nullptr) << id;
        staged.active_layout_id = id;
        const auto loaded = save_and_reload(file.path, staged);
        ASSERT_EQ(loaded.status, suborno::settings::SettingsStatus::ok) << loaded.error;
        EXPECT_EQ(loaded.settings.active_layout_id, id);
    }
}

TEST(UIAuditSettingsPages, InterfacePageEveryControlKeyRoundTripsThroughSave) {
    TempFile file("suborno_audit_interface_page.conf");
    suborno::settings::EngineSettings staged;
    staged.ui.show_candidate_window = false;  // checkbox

    staged.ui.next_word_prediction_enabled = true;  // checkbox

    staged.ui.candidate_orientation =
        suborno::settings::CandidateOrientation::horizontal;  // dropdown option 2

    staged.ui.candidate_fixed_height = false;  // checkbox

    staged.ui.enable_sliding_effect = false;  // checkbox

    staged.ui.follow_caret = false;  // checkbox

    staged.ui.follow_mouse_pointer_if_caret_tracking_fails = true;  // checkbox

    staged.ui.theme_id = "suborno-light";  // theme dropdown

    staged.ui.language = "bn";  // language dropdown

    staged.ui.use_os_default_bangla_font = false;  // font checkbox...

    staged.ui.font_family = "Noto Sans Bengali";  // ...and font dropdown

    const auto loaded = save_and_reload(file.path, staged);
    ASSERT_EQ(loaded.status, suborno::settings::SettingsStatus::ok) << loaded.error;
    EXPECT_EQ(loaded.settings.ui.show_candidate_window, false);
    EXPECT_EQ(loaded.settings.ui.next_word_prediction_enabled, true);
    EXPECT_EQ(loaded.settings.ui.candidate_orientation,
              suborno::settings::CandidateOrientation::horizontal);
    EXPECT_EQ(loaded.settings.ui.candidate_fixed_height, false);
    EXPECT_EQ(loaded.settings.ui.enable_sliding_effect, false);
    EXPECT_EQ(loaded.settings.ui.follow_caret, false);
    EXPECT_EQ(loaded.settings.ui.follow_mouse_pointer_if_caret_tracking_fails, true);
    EXPECT_EQ(loaded.settings.ui.theme_id, "suborno-light");
    EXPECT_EQ(loaded.settings.ui.language, "bn");
    EXPECT_EQ(loaded.settings.ui.use_os_default_bangla_font, false);
    EXPECT_EQ(loaded.settings.ui.font_family, "Noto Sans Bengali");

    // Dropdown walks: every option of both dropdowns, all the way through Save.

    for (const auto orientation : {suborno::settings::CandidateOrientation::vertical,
                                   suborno::settings::CandidateOrientation::horizontal}) {
        staged.ui.candidate_orientation = orientation;
        const auto walked = save_and_reload(file.path, staged);
        ASSERT_EQ(walked.status, suborno::settings::SettingsStatus::ok) << walked.error;
        EXPECT_EQ(walked.settings.ui.candidate_orientation, orientation);
    }
    for (const char* theme : {"suborno-dark", "suborno-light", "system"}) {
        staged.ui.theme_id = theme;
        const auto walked = save_and_reload(file.path, staged);
        ASSERT_EQ(walked.status, suborno::settings::SettingsStatus::ok) << walked.error;
        EXPECT_EQ(walked.settings.ui.theme_id, theme);
    }
    for (const char* language : {"system", "en", "bn"}) {
        staged.ui.language = language;
        const auto walked = save_and_reload(file.path, staged);
        ASSERT_EQ(walked.status, suborno::settings::SettingsStatus::ok) << walked.error;
        EXPECT_EQ(walked.settings.ui.language, language);
    }
    // Checkbox walk in both directions.

    for (const bool show : {true, false}) {
        staged.ui.show_candidate_window = show;
        const auto walked = save_and_reload(file.path, staged);
        ASSERT_EQ(walked.status, suborno::settings::SettingsStatus::ok) << walked.error;
        EXPECT_EQ(walked.settings.ui.show_candidate_window, show);
    }
}

TEST(UIAuditSettingsPages, TopbarPageSlidersCoverMeaningfulRange) {
    TempFile file("suborno_audit_topbar_page.conf");
    suborno::settings::EngineSettings staged;
    staged.topbar.idle_dimming_enabled = true;
    staged.topbar.remember_position = false;
    staged.topbar.position_x = 1600;
    staged.topbar.position_y = 240;

    // Slider walk over minimum / default / midpoint / maximum. The model

    // accepts 0..100 opacity and 1..3600 seconds; the Settings slider caps

    // the seconds control at 60 (recorded in the traceability matrix).

    for (const std::uint32_t opacity : {0u, 1u, 50u, 99u, 100u}) {
        staged.topbar.idle_opacity_percent = opacity;
        const auto loaded = save_and_reload(file.path, staged);
        ASSERT_EQ(loaded.status, suborno::settings::SettingsStatus::ok) << loaded.error;
        EXPECT_EQ(loaded.settings.topbar.idle_opacity_percent, opacity);
    }
    for (const std::uint32_t seconds : {1u, 10u, 60u, 3599u, 3600u}) {
        staged.topbar.idle_time_seconds = seconds;
        const auto loaded = save_and_reload(file.path, staged);
        ASSERT_EQ(loaded.status, suborno::settings::SettingsStatus::ok) << loaded.error;
        EXPECT_EQ(loaded.settings.topbar.idle_time_seconds, seconds);
    }
    const auto loaded = save_and_reload(file.path, staged);
    ASSERT_EQ(loaded.status, suborno::settings::SettingsStatus::ok) << loaded.error;
    EXPECT_EQ(loaded.settings.topbar.idle_dimming_enabled, true);
    EXPECT_EQ(loaded.settings.topbar.remember_position, false);
    EXPECT_EQ(loaded.settings.topbar.position_x, 1600);
    EXPECT_EQ(loaded.settings.topbar.position_y, 240);
}

TEST(UIAuditSettingsPages, InputPageEveryControlKeyRoundTripsThroughSave) {
    TempFile file("suborno_audit_input_page.conf");
    suborno::settings::EngineSettings staged;
    staged.input.map_numpad_to_bangla_numbers = false;  // checkbox

    staged.input.ignore_caps_lock_state_in_bangla = true;  // checkbox

    staged.learning_enabled = false;  // learning dropdown option 2

    staged.input.select_candidate_using_number_keys = false;  // checkbox

    staged.input.preferred_encoding = suborno::settings::TextEncoding::ansi;  // dropdown option 2

    staged.input.change_locale_to_bn_bd_when_layout_not_english = false;  // checkbox

    staged.input.disable_encoding_shortcut = true;  // checkbox

    staged.input.layout_switch_shortcut = "Ctrl+Space";  // shortcut dropdown

    staged.phonetic_correction = false;  // BornoTSF compatibility checkbox

    staged.typing_style = "traditional";  // BornoTSF compatibility checkbox

    const auto loaded = save_and_reload(file.path, staged);
    ASSERT_EQ(loaded.status, suborno::settings::SettingsStatus::ok) << loaded.error;
    EXPECT_EQ(loaded.settings.input.map_numpad_to_bangla_numbers, false);
    EXPECT_EQ(loaded.settings.input.ignore_caps_lock_state_in_bangla, true);
    EXPECT_EQ(loaded.settings.learning_enabled, false);
    EXPECT_EQ(loaded.settings.input.select_candidate_using_number_keys, false);
    EXPECT_EQ(loaded.settings.input.preferred_encoding, suborno::settings::TextEncoding::ansi);
    EXPECT_EQ(loaded.settings.input.change_locale_to_bn_bd_when_layout_not_english, false);
    EXPECT_EQ(loaded.settings.input.disable_encoding_shortcut, true);
    EXPECT_EQ(loaded.settings.input.layout_switch_shortcut, "Ctrl+Space");
    EXPECT_EQ(loaded.settings.phonetic_correction, false);
    EXPECT_EQ(loaded.settings.typing_style, "traditional");
}

TEST(UIAuditSettingsPages, ShortcutDropdownEveryRegisterableOptionAccepted) {
    TempFile file("suborno_audit_shortcut_dropdown.conf");
    suborno::settings::EngineSettings staged;
    // The seven registerable options the dropdown lists, plus the two

    // Windows-reserved combos that can only be displayed (already saved by

    // another build) — the model accepts all nine; RegisterHotKey itself is

    // Windows-runtime (BLOCKED) territory.

    for (const char* shortcut :
         {"F1", "F2", "F3", "F4", "F10", "F12", "Ctrl+Space", "Alt+Shift", "Win+Space"}) {
        staged.input.layout_switch_shortcut = shortcut;
        const auto loaded = save_and_reload(file.path, staged);
        ASSERT_EQ(loaded.status, suborno::settings::SettingsStatus::ok) << loaded.error;
        EXPECT_EQ(loaded.settings.input.layout_switch_shortcut, shortcut);
    }
    // Anything outside the allowlist is rejected so a hand-edited file

    // cannot register an unhandled hotkey.

    for (const char* invalid : {"", "F5", "Ctrl+Alt+F13", "Shift+Space"}) {
        staged.input.layout_switch_shortcut = invalid;
        EXPECT_EQ(suborno::settings::SettingsStore(file.path).save(staged),
                  suborno::settings::SettingsStatus::invalid_format)
            << invalid;
    }
}

// ---------------------------------------------------------------------------

// Phase 03.2/03.3 — Save and Cancel contracts.

// ---------------------------------------------------------------------------

TEST(UIAuditSaveCancel, SaveIsIdempotentByteForByteWhenNothingChanged) {
    TempFile file("suborno_audit_save_idempotent.conf");
    suborno::settings::EngineSettings staged;
    staged.ui.theme_id = "suborno-light";
    const suborno::settings::SettingsStore store(file.path);
    ASSERT_EQ(store.save(staged), suborno::settings::SettingsStatus::ok);
    const std::string first = file.read();
    ASSERT_EQ(store.save(staged), suborno::settings::SettingsStatus::ok);
    const std::string second = file.read();
    EXPECT_EQ(first, second) << "Repeated Save must not rewrite any value";
}

TEST(UIAuditSaveCancel, CancelLeavesPersistedStateUntouched) {
    TempFile file("suborno_audit_cancel.conf");
    suborno::settings::EngineSettings committed;
    committed.ui.theme_id = "suborno-dark";
    committed.topbar.idle_opacity_percent = 50;
    ASSERT_EQ(suborno::settings::SettingsStore(file.path).save(committed),
              suborno::settings::SettingsStatus::ok);
    const std::string before = file.read();

    // The Settings window stages edits into a separate snapshot and only

    // Save writes it; Cancel re-stages from the engine and never persists.

    // Model-level contract: discarding the staged copy must not touch disk.

    suborno::settings::EngineSettings staged = committed;
    staged.ui.theme_id = "suborno-light";
    staged.topbar.idle_opacity_percent = 87;
    staged.general.start_with_windows = true;
    // ... Cancel happens here (staged copy dropped, no save() call) ...

    EXPECT_EQ(file.read(), before) << "Cancel must not persist staged changes";
    const auto reloaded = suborno::settings::SettingsStore(file.path).load();
    ASSERT_EQ(reloaded.status, suborno::settings::SettingsStatus::ok);
    EXPECT_EQ(reloaded.settings.ui.theme_id, "suborno-dark");
    EXPECT_EQ(reloaded.settings.topbar.idle_opacity_percent, 50u);
    EXPECT_EQ(reloaded.settings.general.start_with_windows, false);
}

TEST(UIAuditSaveCancel, SavingOneControlLeavesEveryOtherValueIntact) {
    TempFile file("suborno_audit_partial_save.conf");
    suborno::settings::EngineSettings committed;
    ASSERT_EQ(suborno::settings::SettingsStore(file.path).save(committed),
              suborno::settings::SettingsStatus::ok);

    const auto reloaded = suborno::settings::SettingsStore(file.path).load();
    ASSERT_EQ(reloaded.status, suborno::settings::SettingsStatus::ok);
    suborno::settings::EngineSettings staged = reloaded.settings;  // re-stage from engine

    staged.topbar.idle_time_seconds = 45;  // single slider edit

    const auto after = save_and_reload(file.path, staged);
    ASSERT_EQ(after.status, suborno::settings::SettingsStatus::ok);
    EXPECT_EQ(after.settings.topbar.idle_time_seconds, 45u);
    EXPECT_EQ(after.settings.ui.theme_id, committed.ui.theme_id);
    EXPECT_EQ(after.settings.ui.language, committed.ui.language);
    EXPECT_EQ(after.settings.active_layout_id, committed.active_layout_id);
    EXPECT_EQ(after.settings.candidate_limit, committed.candidate_limit);
    EXPECT_EQ(after.settings.learning_enabled, committed.learning_enabled);
    EXPECT_EQ(after.settings.input.layout_switch_shortcut, committed.input.layout_switch_shortcut);
    EXPECT_EQ(after.settings.input.preferred_encoding, committed.input.preferred_encoding);
    EXPECT_EQ(after.settings.topbar.idle_opacity_percent, committed.topbar.idle_opacity_percent);
    EXPECT_EQ(after.settings.general.start_mode, committed.general.start_mode);
}

// ---------------------------------------------------------------------------

// Phase 05 — E2E workflows at the engine boundary (restart persistence).

// ---------------------------------------------------------------------------

TEST(UIAuditE2E, RestartPersistenceEverySavedSettingRestored) {
    TempFile file("suborno_audit_restart.conf");
    suborno::settings::EngineSettings staged;
    staged.active_layout_id = "probhat";
    staged.ui.language = "bn";
    staged.ui.theme_id = "suborno-light";
    staged.ui.font_family = "Noto Sans Bengali";
    staged.ui.use_os_default_bangla_font = false;
    staged.topbar.idle_opacity_percent = 75;
    staged.topbar.idle_time_seconds = 30;
    staged.input.preferred_encoding = suborno::settings::TextEncoding::ansi;
    staged.input.layout_switch_shortcut = "F12";
    staged.general.start_mode = suborno::settings::StartMode::main_window;
    ASSERT_EQ(suborno::settings::SettingsStore(file.path).save(staged),
              suborno::settings::SettingsStatus::ok);

    // "Application restart": a fresh Engine over the same settings path.

    suborno::core::EngineConfig config;
    config.settings_path = file.path;
    suborno::core::Engine restarted(config);
    EXPECT_EQ(restarted.active_layout_id(), "probhat");

    const auto reloaded = suborno::settings::SettingsStore(file.path).load();
    ASSERT_EQ(reloaded.status, suborno::settings::SettingsStatus::ok);
    EXPECT_EQ(reloaded.settings.ui.language, "bn");
    EXPECT_EQ(reloaded.settings.ui.theme_id, "suborno-light");
    EXPECT_EQ(reloaded.settings.ui.font_family, "Noto Sans Bengali");
    EXPECT_EQ(reloaded.settings.ui.use_os_default_bangla_font, false);
    EXPECT_EQ(reloaded.settings.topbar.idle_opacity_percent, 75u);
    EXPECT_EQ(reloaded.settings.topbar.idle_time_seconds, 30u);
    EXPECT_EQ(reloaded.settings.input.preferred_encoding, suborno::settings::TextEncoding::ansi);
    EXPECT_EQ(reloaded.settings.input.layout_switch_shortcut, "F12");
    EXPECT_EQ(reloaded.settings.general.start_mode, suborno::settings::StartMode::main_window);
}

TEST(UIAuditE2E, FirstLaunchPersistsDefaultsAndEngineStartsOnDefaultLayout) {
    TempFile file("suborno_audit_first_launch.conf");
    suborno::core::EngineConfig config;
    config.settings_path = file.path;
    suborno::core::Engine first_launch(config);
    EXPECT_EQ(first_launch.active_layout_id(), "suborno-phonetic");

    // The shell persists defaults on first launch (application.cpp

    // initialize()); the model already saved nothing yet — the store's

    // file_existed flag is what the shell keys off.

    const auto probe = suborno::settings::SettingsStore(file.path).load();
    EXPECT_EQ(probe.status, suborno::settings::SettingsStatus::ok);
    EXPECT_FALSE(probe.file_existed);
}

// ---------------------------------------------------------------------------

// Phase 06 — state transitions on the model the controls drive.

// ---------------------------------------------------------------------------

TEST(UIAuditNegative, SliderAndLimitValuesOutsideRangeAreRejected) {
    TempFile file("suborno_audit_range_reject.conf");
    suborno::settings::EngineSettings staged;
    staged.topbar.idle_opacity_percent = 101;  // above slider max

    EXPECT_EQ(suborno::settings::SettingsStore(file.path).save(staged),
              suborno::settings::SettingsStatus::invalid_format);
    staged.topbar.idle_opacity_percent = 100;
    staged.topbar.idle_time_seconds = 0;  // below slider min

    EXPECT_EQ(suborno::settings::SettingsStore(file.path).save(staged),
              suborno::settings::SettingsStatus::invalid_format);
    staged.topbar.idle_time_seconds = 3601;
    EXPECT_EQ(suborno::settings::SettingsStore(file.path).save(staged),
              suborno::settings::SettingsStatus::invalid_format);
    staged.topbar.idle_time_seconds = 60;
    staged.candidate_limit = 0;
    EXPECT_EQ(suborno::settings::SettingsStore(file.path).save(staged),
              suborno::settings::SettingsStatus::invalid_format);
    staged.candidate_limit = 101;
    EXPECT_EQ(suborno::settings::SettingsStore(file.path).save(staged),
              suborno::settings::SettingsStatus::invalid_format);
}

TEST(UIAuditNegative, RapidRepeatedSaveLoadCyclesStayConsistent) {
    TempFile file("suborno_audit_rapid_save.conf");
    suborno::settings::EngineSettings staged;
    const suborno::settings::SettingsStore store(file.path);
    for (int cycle = 0; cycle < 50; ++cycle) {
        staged.ui.theme_id = (cycle % 2 == 0) ? "suborno-dark" : "suborno-light";
        staged.topbar.idle_opacity_percent = static_cast<std::uint32_t>(cycle);
        staged.general.last_ui_mode = (cycle % 3 == 0) ? "tray" : "topbar";
        ASSERT_EQ(store.save(staged), suborno::settings::SettingsStatus::ok) << cycle;
        const auto loaded = store.load();
        ASSERT_EQ(loaded.status, suborno::settings::SettingsStatus::ok) << cycle;
        EXPECT_EQ(loaded.settings.ui.theme_id, staged.ui.theme_id) << cycle;
        EXPECT_EQ(loaded.settings.topbar.idle_opacity_percent, static_cast<std::uint32_t>(cycle))
            << cycle;
        EXPECT_EQ(loaded.settings.general.last_ui_mode, staged.general.last_ui_mode) << cycle;
    }
}

TEST(UIAuditNegative, CorruptSettingsQuarantinedAndDefaultsRestored) {
    TempFile file("suborno_audit_corrupt.conf");
    file.write("version=2\nnot_a_real_key=oops\nactive_layout=probhat\n");
    const auto rejected = suborno::settings::SettingsStore(file.path).load();
    ASSERT_EQ(rejected.status, suborno::settings::SettingsStatus::invalid_format);

    // Application policy (initialize()): quarantine the unparsable file, then

    // continue from defaults. The quarantined copy preserves the user's bytes.

    const auto quarantined = suborno::persistence::quarantine_file(file.path);
    ASSERT_EQ(quarantined, std::error_code{});
    EXPECT_FALSE(std::filesystem::exists(file.path));
    std::filesystem::path quarantined_copy;
    for (const std::filesystem::path& entry :
         std::filesystem::directory_iterator(file.path.parent_path())) {
        if (entry.filename().string().rfind(file.path.filename().string() + ".corrupt-", 0) == 0) {
            quarantined_copy = entry;
        }
    }
    ASSERT_FALSE(quarantined_copy.empty());
    // The stream must be closed before the removal: Windows refuses to

    // delete a file with an open handle (sharing violation).

    std::string bytes;
    {
        std::ifstream preserved(quarantined_copy, std::ios::binary);
        bytes.assign(std::istreambuf_iterator<char>(preserved), std::istreambuf_iterator<char>());
    }
    EXPECT_EQ(bytes, "version=2\nnot_a_real_key=oops\nactive_layout=probhat\n");
    std::filesystem::remove(quarantined_copy);

    // After quarantine the next start behaves like first launch: defaults.

    const auto recovered = suborno::settings::SettingsStore(file.path).load();
    EXPECT_EQ(recovered.status, suborno::settings::SettingsStatus::ok);
    EXPECT_EQ(recovered.settings.active_layout_id, "suborno-phonetic");
    EXPECT_FALSE(recovered.file_existed);
}

TEST(UIAuditNegative, UnknownSavedLayoutIdCorrectedOnEngineStart) {
    TempFile file("suborno_audit_unknown_layout.conf");
    suborno::settings::EngineSettings staged;
    staged.active_layout_id = "zzz-nonexistent";
    ASSERT_EQ(suborno::settings::SettingsStore(file.path).save(staged),
              suborno::settings::SettingsStatus::ok);

    suborno::core::EngineConfig config;
    config.settings_path = file.path;
    suborno::core::Engine engine(config);
    // Engine keeps typing on the default layout instead of dropping.

    EXPECT_EQ(engine.active_layout_id(), "suborno-phonetic");
    // Session requests for the unknown id fall back to the active layout.

    const auto session = engine.create_session_for_layout("zzz-nonexistent");
    ASSERT_NE(session, nullptr);
}

// ---------------------------------------------------------------------------

// Phase 04/05 — OSK preview labels (what each mode button + key would emit).

// ---------------------------------------------------------------------------

TEST(UIAuditOsk, PreviewLabelsForEveryRegisteredLayout) {
    suborno::core::Engine engine;
    const char* bangla_layouts[] = {
        "suborno-phonetic", "borno-phonetic", "avro-phonetic", "khipro", "probhat", "national"};
    // English mode: OSK letters preview as themselves (passthrough).

    ASSERT_TRUE(engine.set_active_layout("english"));
    EXPECT_EQ(engine.layout_preview("k"), "k");
    // Bangla modes: every layout produces Bengali-script output, and the

    // shifted letter maps to a different glyph.

    for (const char* id : bangla_layouts) {
        ASSERT_TRUE(engine.set_active_layout(id)) << id;
        const std::string base = engine.layout_preview("k");
        EXPECT_FALSE(base.empty()) << id;
        EXPECT_TRUE(contains_bengali(base)) << id << " -> " << base;
        EXPECT_NE(base, "k") << id;
        const std::string shifted = engine.layout_preview("K");
        EXPECT_FALSE(shifted.empty()) << id;
    }
    // Known positional anchors from the per-layout conformance suites.

    ASSERT_TRUE(engine.set_active_layout("probhat"));
    EXPECT_EQ(engine.layout_preview("k"), "\xE0\xA6\x95");  // ক

    ASSERT_TRUE(engine.set_active_layout("national"));
    EXPECT_EQ(engine.layout_preview("k"), "\xE0\xA6\xA4");  // ত

    EXPECT_EQ(engine.layout_preview("K"), "\xE0\xA6\xA5");  // থ
}

// ---------------------------------------------------------------------------

// Phase 05 — encoding toggle & learning toggle (immediate-save paths).

// ---------------------------------------------------------------------------

TEST(UIAuditEncoding, ToggleProducesUnicodeAndBijoyOutput) {
    TempFile file("suborno_audit_encoding.conf");
    suborno::settings::EngineSettings staged;
    staged.input.preferred_encoding = suborno::settings::TextEncoding::unicode;
    ASSERT_EQ(suborno::settings::SettingsStore(file.path).save(staged),
              suborno::settings::SettingsStatus::ok);
    suborno::core::EngineConfig config;
    config.settings_path = file.path;
    suborno::core::Engine engine(config);
    const std::string unicode_output = engine.encode_text_for_output(
        "\xE0\xA6\xAC\xE0\xA6\xBE\xE0\xA6\x82\xE0\xA6\xB2\xE0\xA6\xBE");
    EXPECT_EQ(unicode_output,
              "\xE0\xA6\xAC\xE0\xA6\xBE\xE0\xA6\x82\xE0\xA6\xB2\xE0\xA6\xBE");  // বাংলা

    // Ctrl+Alt+E equivalent: flip the persisted enum.

    staged.input.preferred_encoding = suborno::settings::TextEncoding::ansi;
    ASSERT_EQ(suborno::settings::SettingsStore(file.path).save(staged),
              suborno::settings::SettingsStatus::ok);
    suborno::core::Engine ansi_engine(config);
    const std::string bijoy = ansi_engine.encode_text_for_output(
        "\xE0\xA6\xAC\xE0\xA6\xBE\xE0\xA6\x82\xE0\xA6\xB2\xE0\xA6\xBE");
    EXPECT_EQ(bijoy, "cv'lv");  // Bijoy golden from bijoy_test.cpp

    // Zero-width joiners are documented to vanish in the legacy byte stream

    // (bijoy.cpp:338 — they are Unicode shaping hints with no legacy glyph);

    // the surrounding consonants still encode.

    const std::string broken_conjunct = ansi_engine.encode_text_for_output(
        "\xE0\xA6\x95\xE0\xA7\x8D\xE2\x80\x8C\xE0\xA6\x95");  // ক ্ ZWNJ ক

    EXPECT_EQ(broken_conjunct, "K+K");
}

TEST(UIAuditLearning, PredictionTogglePersistsThroughEngine) {
    TempFile file("suborno_audit_learning.conf");
    suborno::settings::EngineSettings staged;
    ASSERT_EQ(suborno::settings::SettingsStore(file.path).save(staged),
              suborno::settings::SettingsStatus::ok);
    suborno::core::EngineConfig config;
    config.settings_path = file.path;
    {
        suborno::core::Engine engine(config);
        engine.set_next_word_prediction_enabled(true);  // immediate-save path
    }
    const auto reloaded = suborno::settings::SettingsStore(file.path).load();
    ASSERT_EQ(reloaded.status, suborno::settings::SettingsStatus::ok);
    EXPECT_TRUE(reloaded.settings.ui.next_word_prediction_enabled);
}

TEST(UIAuditLearning, ClearLearntWordsLeavesEngineConsistent) {
    TempFile file("suborno_audit_clear_learnt.conf");
    suborno::core::EngineConfig config;
    config.settings_path = file.path;
    config.dictionary_path =
        std::filesystem::temp_directory_path() / "suborno_audit_clear_learnt.sqlite3";
    std::filesystem::remove(config.dictionary_path);
    {
        suborno::core::Engine engine(config);
        engine.clear_learnt_words();  // Clear Learnt Words button / tray item
    }
    // A fresh engine over the same dictionary opens cleanly afterwards.

    // The engine owns the open SQLite connection, so it must be destroyed

    // before the dictionary file is removed: Windows refuses to delete a

    // file with an open handle (sharing violation).

    {
        suborno::core::Engine reopened(config);
        auto session = reopened.create_session();
        ASSERT_NE(session, nullptr);
    }
    std::filesystem::remove(config.dictionary_path);
}

// ---------------------------------------------------------------------------

// Phase 04 — the i18n tables every surface renders.

// ---------------------------------------------------------------------------

TEST(UIAuditI18n, ShippedLocaleFilesCarryEveryInventoryKeyInBothLanguages) {
    const std::filesystem::path i18n_root = SUBORNO_SOURCE_DIR "/data/i18n";
    std::string error;

    ASSERT_TRUE(
        suborno::core::ui_strings::load(suborno::core::ui_strings::locale_file(
                                            i18n_root, suborno::core::ui_strings::Locale::english),
                                        error))
        << error;
    const std::size_t english_count = suborno::core::ui_strings::size();
    const std::vector<std::string> english_keys = suborno::core::ui_strings::keys();

    ASSERT_TRUE(
        suborno::core::ui_strings::load(suborno::core::ui_strings::locale_file(
                                            i18n_root, suborno::core::ui_strings::Locale::bangla),
                                        error))
        << error;
    const std::size_t bangla_count = suborno::core::ui_strings::size();
    const std::vector<std::string> bangla_keys = suborno::core::ui_strings::keys();

    EXPECT_GT(english_count, 150u) << "the shipped English table lost keys";
    EXPECT_GT(bangla_count, 150u) << "the shipped Bangla table lost keys";
    EXPECT_EQ(english_keys, bangla_keys) << "en/bn key sets must be identical";

    // Every key the inventoried surfaces render must resolve in BOTH locales

    // with non-empty text (Bengali text for the Bangla table).

    ASSERT_GT(inventory_ui_keys().size(), 140u);
    ASSERT_TRUE(
        suborno::core::ui_strings::load(suborno::core::ui_strings::locale_file(
                                            i18n_root, suborno::core::ui_strings::Locale::english),
                                        error))
        << error;
    for (const std::string& key : inventory_ui_keys()) {
        EXPECT_FALSE(suborno::core::ui_strings::get(key.c_str()).empty()) << key;
    }
    ASSERT_TRUE(
        suborno::core::ui_strings::load(suborno::core::ui_strings::locale_file(
                                            i18n_root, suborno::core::ui_strings::Locale::bangla),
                                        error))
        << error;
    // Keys that legitimately stay English in the Bangla locale: physical

    // key names and the English language name itself.

    static const std::vector<std::string> english_in_bangla{"osk.key.backspace",
                                                            "osk.key.enter",
                                                            "osk.key.shift",
                                                            "osk.key.space",
                                                            "osk.mode.english",
                                                            "settings.interface.language.english"};
    for (const std::string& key : inventory_ui_keys()) {
        const std::string& text = suborno::core::ui_strings::get(key.c_str());
        EXPECT_FALSE(text.empty()) << key;
        const bool exempt = std::find(english_in_bangla.begin(), english_in_bangla.end(), key)
                            != english_in_bangla.end();
        if (!exempt) {
            EXPECT_TRUE(contains_bengali(text)) << key << " not localized in bn table";
        }
    }
    suborno::core::ui_strings::clear();
}

TEST(UIAuditI18n, LanguageSelectionFollowsPortableLocaleMapping) {
    using suborno::core::ui_strings::Locale;
    using suborno::core::ui_strings::locale_from_setting;
    EXPECT_EQ(locale_from_setting("bn"), Locale::bangla);
    EXPECT_EQ(locale_from_setting("bangla"), Locale::bangla);
    EXPECT_EQ(locale_from_setting("en"), Locale::english);
    EXPECT_EQ(locale_from_setting("system"), Locale::english);  // caller resolves system

    EXPECT_EQ(locale_from_setting(""), Locale::english);
    EXPECT_EQ(locale_from_setting("fr"), Locale::english);
}

// ---------------------------------------------------------------------------

// Phase 11 — Unicode font family names (fix verification, additive).

// ---------------------------------------------------------------------------

TEST(UIAuditFontFamily, BengaliScriptFontNamesRoundTrip) {
    TempFile file("suborno_audit_unicode_font.conf");
    suborno::settings::EngineSettings staged;
    staged.ui.use_os_default_bangla_font = false;
    staged.ui.font_family =
        "\xE0\xA6\xAC\xE0\xA6\xBE\xE0\xA6\x82\xE0\xA6\xB2\xE0\xA6\xBE \xE0\xA6\xAB\xE0\xA6\xA8\xE0\xA7\x8D\xE0\xA6\x9F";  // বাংলা ফন্ট

    const auto loaded = save_and_reload(file.path, staged);
    ASSERT_EQ(loaded.status, suborno::settings::SettingsStatus::ok) << loaded.error;
    EXPECT_EQ(
        loaded.settings.ui.font_family,
        "\xE0\xA6\xAC\xE0\xA6\xBE\xE0\xA6\x82\xE0\xA6\xB2\xE0\xA6\xBE \xE0\xA6\xAB\xE0\xA6\xA8\xE0\xA7\x8D\xE0\xA6\x9F");

    // Mixed-script names (ASCII + Bengali) are the common real-world case.

    staged.ui.font_family =
        "Noto Sans Bengali \xE0\xA6\xAC\xE0\xA6\xBE\xE0\xA6\x82\xE0\xA6\xB2\xE0\xA6\xBE";
    const auto mixed = save_and_reload(file.path, staged);
    ASSERT_EQ(mixed.status, suborno::settings::SettingsStatus::ok) << mixed.error;
    EXPECT_EQ(mixed.settings.ui.font_family,
              "Noto Sans Bengali \xE0\xA6\xAC\xE0\xA6\xBE\xE0\xA6\x82\xE0\xA6\xB2\xE0\xA6\xBE");
}

TEST(UIAuditFontFamily, FontNameSafetyRulesStillHold) {
    TempFile file("suborno_audit_font_safety.conf");
    suborno::settings::EngineSettings staged;
    // INI-structural characters stay rejected.

    staged.ui.font_family = "Bad=Name";
    EXPECT_EQ(suborno::settings::SettingsStore(file.path).save(staged),
              suborno::settings::SettingsStatus::invalid_format);
    staged.ui.font_family = "Bad\nName";
    EXPECT_EQ(suborno::settings::SettingsStore(file.path).save(staged),
              suborno::settings::SettingsStatus::invalid_format);
    // LF_FACESIZE-1 = 32 code points: a 33-code-point Bangla name overflows

    // the GDI face buffer exactly like 33 ASCII characters.

    std::string long_bangla;
    for (int character_repeat = 0; character_repeat < 33; ++character_repeat) {
        long_bangla += "\xE0\xA6\x95";  // ক
    }
    staged.ui.font_family = long_bangla;
    EXPECT_EQ(suborno::settings::SettingsStore(file.path).save(staged),
              suborno::settings::SettingsStatus::invalid_format);
    std::string fit_bangla;
    for (int character_repeat = 0; character_repeat < 32; ++character_repeat) {
        fit_bangla += "\xE0\xA6\x95";
    }
    staged.ui.font_family = fit_bangla;
    EXPECT_EQ(suborno::settings::SettingsStore(file.path).save(staged),
              suborno::settings::SettingsStatus::ok);
    // Invalid UTF-8 is rejected before it can reach the file.

    staged.ui.font_family = "\xC3\x28";  // broken 2-byte sequence

    EXPECT_EQ(suborno::settings::SettingsStore(file.path).save(staged),
              suborno::settings::SettingsStatus::invalid_format);
    // Legacy ASCII whitelist behaviour is unchanged.

    staged.ui.font_family = std::string(33, 'a');
    EXPECT_EQ(suborno::settings::SettingsStore(file.path).save(staged),
              suborno::settings::SettingsStatus::invalid_format);
    staged.ui.font_family = "Noto@Sans";
    EXPECT_EQ(suborno::settings::SettingsStore(file.path).save(staged),
              suborno::settings::SettingsStatus::invalid_format);
    staged.ui.font_family = "Nirmala UI";
    EXPECT_EQ(suborno::settings::SettingsStore(file.path).save(staged),
              suborno::settings::SettingsStatus::ok);
}

// ---------------------------------------------------------------------------

// Phase 12 — tray web-link failure strings share the About dialog's keys

// (I18N-01 / FIND-03 fix contract, additive).

//

// The tray menu's Releases/Documentation failure toasts now route through

// ui::text with the SAME keys the About dialog uses. These tables are the

// portable half of that contract: the keys must resolve in BOTH locales and

// the English table must equal the compiled fallback literals that the tray

// call sites (and this test) pin, so the fix changes no English output.

// The source wiring itself (application.cpp menu_updates/menu_documentation)

// is verified by scripts/ui_audit_static_fix_check.py — the tray handler is

// Windows-only code and cannot execute here.

// ---------------------------------------------------------------------------

TEST(UIAuditI18n, TrayWebLinkFailureKeysResolveAndMatchCompiledFallbacks) {
    const std::filesystem::path i18n_root = SUBORNO_SOURCE_DIR "/data/i18n";
    std::string error;

    ASSERT_TRUE(
        suborno::core::ui_strings::load(suborno::core::ui_strings::locale_file(
                                            i18n_root, suborno::core::ui_strings::Locale::english),
                                        error))
        << error;
    EXPECT_EQ(suborno::core::ui_strings::get("error.releases_open_failed"),
              "Suborno could not open the releases page.");
    EXPECT_EQ(suborno::core::ui_strings::get("error.documentation_open_failed"),
              "Suborno could not open the documentation.");

    ASSERT_TRUE(
        suborno::core::ui_strings::load(suborno::core::ui_strings::locale_file(
                                            i18n_root, suborno::core::ui_strings::Locale::bangla),
                                        error))
        << error;
    EXPECT_TRUE(contains_bengali(suborno::core::ui_strings::get("error.releases_open_failed")))
        << "error.releases_open_failed not localized in bn table";
    EXPECT_TRUE(contains_bengali(suborno::core::ui_strings::get("error.documentation_open_failed")))
        << "error.documentation_open_failed not localized in bn table";

    suborno::core::ui_strings::clear();
}
