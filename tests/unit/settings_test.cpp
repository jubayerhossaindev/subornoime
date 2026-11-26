#include "suborno/settings/settings.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <iterator>
#include <system_error>

#include "persistence/atomic_file.hpp"

#if !defined(_WIN32)
#include <unistd.h>
#endif

TEST(SettingsTest, SavesAndLoadsVersionedSettings) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "suborno_settings_test.conf";
    std::filesystem::remove(path);

    suborno::settings::SettingsStore store(path);
    suborno::settings::EngineSettings settings;
    settings.active_layout_id = "suborno-phonetic";
    settings.candidate_limit = 24;
    settings.learning_enabled = false;
    settings.general.remember_last_input_layout = false;
    settings.general.start_with_windows = true;
    settings.general.start_mode = suborno::settings::StartMode::system_tray;
    settings.general.switching_mode = suborno::settings::SwitchingMode::global;
    settings.ui.theme_id = "suborno-glass";
    settings.ui.show_candidate_window = false;
    settings.ui.next_word_prediction_enabled = true;
    settings.ui.candidate_orientation = suborno::settings::CandidateOrientation::horizontal;
    settings.ui.candidate_fixed_height = false;
    settings.ui.use_os_default_bangla_font = false;
    settings.ui.enable_sliding_effect = false;
    settings.ui.follow_caret = false;
    settings.ui.follow_mouse_pointer_if_caret_tracking_fails = false;
    settings.topbar.idle_dimming_enabled = false;
    settings.topbar.idle_opacity_percent = 35;
    settings.topbar.idle_time_seconds = 45;
    settings.topbar.remember_position = false;
    settings.input.layout_switch_shortcut = "Alt+Shift";
    settings.input.disable_encoding_shortcut = true;
    settings.input.map_numpad_to_bangla_numbers = false;
    settings.input.change_locale_to_bn_bd_when_layout_not_english = false;
    settings.input.ignore_caps_lock_state_in_bangla = true;
    settings.input.select_candidate_using_number_keys = false;
    settings.input.preferred_encoding = suborno::settings::TextEncoding::ansi;

    EXPECT_EQ(store.save(settings), suborno::settings::SettingsStatus::ok);
    const auto loaded = store.load();
    EXPECT_EQ(loaded.status, suborno::settings::SettingsStatus::ok);
    EXPECT_EQ(loaded.settings.active_layout_id, settings.active_layout_id);
    EXPECT_EQ(loaded.settings.candidate_limit, settings.candidate_limit);
    EXPECT_FALSE(loaded.settings.learning_enabled);
    EXPECT_FALSE(loaded.settings.general.remember_last_input_layout);
    EXPECT_TRUE(loaded.settings.general.start_with_windows);
    EXPECT_EQ(loaded.settings.general.start_mode, suborno::settings::StartMode::system_tray);
    EXPECT_EQ(loaded.settings.general.switching_mode, suborno::settings::SwitchingMode::global);
    EXPECT_EQ(loaded.settings.ui.theme_id, "suborno-glass");
    EXPECT_FALSE(loaded.settings.ui.show_candidate_window);
    EXPECT_TRUE(loaded.settings.ui.next_word_prediction_enabled);
    EXPECT_EQ(loaded.settings.ui.candidate_orientation,
              suborno::settings::CandidateOrientation::horizontal);
    EXPECT_FALSE(loaded.settings.ui.candidate_fixed_height);
    EXPECT_FALSE(loaded.settings.ui.use_os_default_bangla_font);
    EXPECT_FALSE(loaded.settings.ui.enable_sliding_effect);
    EXPECT_FALSE(loaded.settings.ui.follow_caret);
    EXPECT_FALSE(loaded.settings.ui.follow_mouse_pointer_if_caret_tracking_fails);
    EXPECT_FALSE(loaded.settings.topbar.idle_dimming_enabled);
    EXPECT_EQ(loaded.settings.topbar.idle_opacity_percent, 35U);
    EXPECT_EQ(loaded.settings.topbar.idle_time_seconds, 45U);
    EXPECT_FALSE(loaded.settings.topbar.remember_position);
    EXPECT_EQ(loaded.settings.input.layout_switch_shortcut, "Alt+Shift");
    EXPECT_TRUE(loaded.settings.input.disable_encoding_shortcut);
    EXPECT_FALSE(loaded.settings.input.map_numpad_to_bangla_numbers);
    EXPECT_FALSE(loaded.settings.input.change_locale_to_bn_bd_when_layout_not_english);
    EXPECT_TRUE(loaded.settings.input.ignore_caps_lock_state_in_bangla);
    EXPECT_FALSE(loaded.settings.input.select_candidate_using_number_keys);
    EXPECT_EQ(loaded.settings.input.preferred_encoding, suborno::settings::TextEncoding::ansi);

    std::filesystem::remove(path);
}

TEST(SettingsTest, SavesAndLoadsFontPositionAndUiModeKeys) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "suborno_settings_font_test.conf";
    std::filesystem::remove(path);

    suborno::settings::SettingsStore store(path);
    suborno::settings::EngineSettings settings;
    settings.ui.use_os_default_bangla_font = false;
    settings.ui.font_family = "Nirmala UI";
    settings.topbar.remember_position = true;
    settings.topbar.position_x = 320;
    settings.topbar.position_y = 40;
    settings.general.last_ui_mode = "tray";
    settings.general.last_bangla_layout_id = "suborno-smart";
    settings.general.start_mode = suborno::settings::StartMode::last_state;

    ASSERT_EQ(store.save(settings), suborno::settings::SettingsStatus::ok);
    const auto loaded = store.load();
    ASSERT_EQ(loaded.status, suborno::settings::SettingsStatus::ok);
    EXPECT_FALSE(loaded.settings.ui.use_os_default_bangla_font);
    EXPECT_EQ(loaded.settings.ui.font_family, "Nirmala UI");
    EXPECT_TRUE(loaded.settings.topbar.remember_position);
    EXPECT_EQ(loaded.settings.topbar.position_x, 320);
    EXPECT_EQ(loaded.settings.topbar.position_y, 40);
    EXPECT_EQ(loaded.settings.general.last_ui_mode, "tray");
    EXPECT_EQ(loaded.settings.general.last_bangla_layout_id, "suborno-smart");

    // A file written before the key existed loads with the default (the
    // English↔বাংলা toggle falls back to the Suborno layout).
    {
        suborno::settings::EngineSettings legacy;
        ASSERT_EQ(store.save(legacy), suborno::settings::SettingsStatus::ok);
    }
    const auto legacy_loaded = store.load();
    ASSERT_EQ(legacy_loaded.status, suborno::settings::SettingsStatus::ok);
    EXPECT_EQ(legacy_loaded.settings.general.last_bangla_layout_id, "suborno-phonetic");
    // A value the file format cannot store must be rejected, not stored
    // (the layout id itself is validated as a generic value string here;
    // unknown ids are healed by the engine at load time).
    {
        suborno::settings::EngineSettings broken;
        broken.general.last_bangla_layout_id = "bad=id";
        EXPECT_NE(store.save(broken), suborno::settings::SettingsStatus::ok);
    }

    // An empty font family means "OS default" and must stay loadable.
    settings.ui.font_family.clear();
    ASSERT_EQ(store.save(settings), suborno::settings::SettingsStatus::ok);
    const auto defaulted = store.load();
    ASSERT_EQ(defaulted.status, suborno::settings::SettingsStatus::ok);
    EXPECT_EQ(defaulted.settings.ui.font_family, "");

    // Out-of-range positions and invalid font names are rejected.
    settings.topbar.position_x = 40000;
    EXPECT_EQ(store.save(settings), suborno::settings::SettingsStatus::invalid_format);
    settings = {};
    settings.ui.font_family = "Bad=Name";
    EXPECT_EQ(store.save(settings), suborno::settings::SettingsStatus::invalid_format);
    settings = {};
    settings.ui.font_family = "ThisFontNameIsFarTooLongForGdiFaces";
    EXPECT_EQ(store.save(settings), suborno::settings::SettingsStatus::invalid_format);

    std::filesystem::remove(path);
}

TEST(SettingsTest, UiLanguageKeyRoundTripsAndValidates) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "suborno_settings_language_test.conf";
    std::filesystem::remove(path);

    suborno::settings::SettingsStore store(path);

    // Default: follow the system display language.
    {
        suborno::settings::EngineSettings defaults;
        EXPECT_EQ(defaults.ui.language, "system");
    }

    // Each accepted value round-trips.
    for (const char* value : {"en", "bn", "system"}) {
        suborno::settings::EngineSettings settings;
        settings.ui.language = value;
        ASSERT_EQ(store.save(settings), suborno::settings::SettingsStatus::ok) << value;
        const auto loaded = store.load();
        ASSERT_EQ(loaded.status, suborno::settings::SettingsStatus::ok) << value;
        EXPECT_EQ(loaded.settings.ui.language, value) << value;
    }

    // Unknown locales are rejected at save time, so a hand-edited value
    // that skipped load() cannot enter the format either.
    {
        suborno::settings::EngineSettings invalid;
        invalid.ui.language = "fr";
        EXPECT_EQ(store.save(invalid), suborno::settings::SettingsStatus::invalid_format);
        invalid.ui.language = "";
        EXPECT_EQ(store.save(invalid), suborno::settings::SettingsStatus::invalid_format);
    }

    // A settings file written before the key existed loads with the default.
    {
        suborno::settings::EngineSettings legacy;
        legacy.ui.language = "bn";
        ASSERT_EQ(store.save(legacy), suborno::settings::SettingsStatus::ok);
        std::ifstream stream(path, std::ios::binary);
        std::string line;
        std::string rewritten;
        while (std::getline(stream, line)) {
            if (line.rfind("language=", 0) != 0) {
                rewritten += line + "\n";
            }
        }
        stream.close();
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        out << rewritten;
    }
    {
        const auto legacy_loaded = store.load();
        ASSERT_EQ(legacy_loaded.status, suborno::settings::SettingsStatus::ok);
        EXPECT_EQ(legacy_loaded.settings.ui.language, "system");
    }

    std::filesystem::remove(path);
}

TEST(SettingsTest, MissingFileUsesDefaults) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "suborno_settings_missing_test.conf";
    std::filesystem::remove(path);

    const auto loaded = suborno::settings::SettingsStore(path).load();
    EXPECT_EQ(loaded.status, suborno::settings::SettingsStatus::ok);
    EXPECT_EQ(loaded.settings.active_layout_id, "suborno-phonetic");
    EXPECT_EQ(loaded.settings.candidate_limit, 16U);
    EXPECT_TRUE(loaded.settings.learning_enabled);
}

TEST(SettingsTest, MigratesLegacyVersionOneSettingsToCurrentDefaults) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "suborno_settings_v1_migration_test.conf";
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output << "version=1\n"
               << "active_layout=probhat\n"
               << "candidate_limit=12\n"
               << "learning_enabled=false\n";
    }

    suborno::settings::SettingsStore store(path);
    const auto loaded = store.load();
    ASSERT_EQ(loaded.status, suborno::settings::SettingsStatus::ok);
    EXPECT_EQ(loaded.settings.active_layout_id, "probhat");
    EXPECT_EQ(loaded.settings.candidate_limit, 12U);
    EXPECT_FALSE(loaded.settings.learning_enabled);
    EXPECT_EQ(loaded.settings.ui.theme_id, "suborno-dark");
    EXPECT_EQ(loaded.settings.topbar.idle_opacity_percent, 50U);
    EXPECT_EQ(loaded.settings.input.layout_switch_shortcut, "F10");
    EXPECT_EQ(loaded.settings.input.preferred_encoding, suborno::settings::TextEncoding::unicode);

    EXPECT_EQ(store.save(loaded.settings), suborno::settings::SettingsStatus::ok);
    {
        // The stream must be closed before the removal: Windows refuses to
        // delete a file with an open handle (sharing violation), while POSIX
        // unlinking tolerates it (observed on the real MSVC machine).
        std::ifstream migrated(path);
        std::string first_line;
        ASSERT_TRUE(std::getline(migrated, first_line));
        EXPECT_EQ(first_line, "version=2");
    }
    std::filesystem::remove(path);
}

TEST(SettingsTest, RejectsInvalidDesktopRangesShortcutsAndDuplicateKeys) {
    suborno::settings::EngineSettings invalid;
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "suborno_settings_invalid_desktop_test.conf";

    invalid.topbar.idle_opacity_percent = 101;
    EXPECT_EQ(suborno::settings::SettingsStore(path).save(invalid),
              suborno::settings::SettingsStatus::invalid_format);
    invalid = {};
    invalid.topbar.idle_time_seconds = 0;
    EXPECT_EQ(suborno::settings::SettingsStore(path).save(invalid),
              suborno::settings::SettingsStatus::invalid_format);
    invalid = {};
    invalid.input.layout_switch_shortcut = "F99";
    EXPECT_EQ(suborno::settings::SettingsStore(path).save(invalid),
              suborno::settings::SettingsStatus::invalid_format);
    invalid = {};
    invalid.ui.theme_id = "theme=invalid";
    EXPECT_EQ(suborno::settings::SettingsStore(path).save(invalid),
              suborno::settings::SettingsStatus::invalid_format);

    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output << "version=2\n"
               << "active_layout=suborno-phonetic\n"
               << "active_layout=probhat\n"
               << "candidate_limit=16\n"
               << "learning_enabled=true\n";
    }
    EXPECT_EQ(suborno::settings::SettingsStore(path).load().status,
              suborno::settings::SettingsStatus::invalid_format);
    std::filesystem::remove(path);
}

TEST(SettingsTest, RejectsMalformedOrUnsupportedFiles) {
    const std::filesystem::path malformed =
        std::filesystem::temp_directory_path() / "suborno_settings_malformed_test.conf";
    std::ofstream malformed_output(malformed, std::ios::binary | std::ios::trunc);
    malformed_output << "not-a-settings-line\n";
    malformed_output.close();

    const auto malformed_result = suborno::settings::SettingsStore(malformed).load();
    EXPECT_EQ(malformed_result.status, suborno::settings::SettingsStatus::invalid_format);

    const std::filesystem::path unsupported =
        std::filesystem::temp_directory_path() / "suborno_settings_unsupported_test.conf";
    std::ofstream unsupported_output(unsupported, std::ios::binary | std::ios::trunc);
    unsupported_output << "version=99\n"
                       << "active_layout=suborno-phonetic\n"
                       << "candidate_limit=16\n"
                       << "learning_enabled=true\n";
    unsupported_output.close();

    const auto unsupported_result = suborno::settings::SettingsStore(unsupported).load();
    EXPECT_EQ(unsupported_result.status, suborno::settings::SettingsStatus::unsupported_version);

    std::filesystem::remove(malformed);
    std::filesystem::remove(unsupported);
}

TEST(SettingsTest, RejectsOverflowingVersionAndInvalidLayoutIds) {
    const std::filesystem::path version_path =
        std::filesystem::temp_directory_path() / "suborno_settings_overflow_version_test.conf";
    {
        std::ofstream output(version_path, std::ios::binary | std::ios::trunc);
        output << "version=4294967297\n"
               << "active_layout=suborno-phonetic\n"
               << "candidate_limit=16\n"
               << "learning_enabled=true\n";
    }

    const auto version_result = suborno::settings::SettingsStore(version_path).load();
    EXPECT_NE(version_result.status, suborno::settings::SettingsStatus::ok);

    suborno::settings::EngineSettings invalid_settings;
    invalid_settings.active_layout_id = "layout\ncandidate_limit=100";
    const std::filesystem::path invalid_layout_path =
        std::filesystem::temp_directory_path() / "suborno_settings_invalid_layout_test.conf";
    EXPECT_EQ(suborno::settings::SettingsStore(invalid_layout_path).save(invalid_settings),
              suborno::settings::SettingsStatus::invalid_format);
    EXPECT_FALSE(std::filesystem::exists(invalid_layout_path));

    std::filesystem::remove(version_path);
}

TEST(SettingsTest, ReplacesExistingSettingsFile) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "suborno_settings_replace_test.conf";
    std::filesystem::remove(path);
    suborno::settings::SettingsStore store(path);

    suborno::settings::EngineSettings settings;
    EXPECT_EQ(store.save(settings), suborno::settings::SettingsStatus::ok);
    settings.candidate_limit = 32;
    EXPECT_EQ(store.save(settings), suborno::settings::SettingsStatus::ok);

    const auto loaded = store.load();
    EXPECT_EQ(loaded.status, suborno::settings::SettingsStatus::ok);
    EXPECT_EQ(loaded.settings.candidate_limit, 32U);
    std::filesystem::remove(path);
}

TEST(SettingsTest, AcceptsTheKhiproCompositionalLayoutId) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "suborno_settings_khipro_layout_test.conf";
    std::filesystem::remove(path);

    suborno::settings::EngineSettings settings;
    settings.active_layout_id = "khipro";
    const suborno::settings::SettingsStore store(path);

    EXPECT_EQ(store.save(settings), suborno::settings::SettingsStatus::ok);
    EXPECT_EQ(store.load().settings.active_layout_id, "khipro");

    std::filesystem::remove(path);
}

// ---------------------------------------------------------------------------
// Settings-loading robustness: the behaviors the Windows first-launch path
// depends on. Every test below performs real filesystem operations inside
// isolated temporary directories.
// ---------------------------------------------------------------------------

TEST(SettingsTest, MissingFileReportsFileExistedFalse) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "suborno_settings_firstrun_test.conf";
    std::filesystem::remove(path);

    const suborno::settings::SettingsStore store(path);
    const auto loaded = store.load();
    EXPECT_EQ(loaded.status, suborno::settings::SettingsStatus::ok);
    EXPECT_FALSE(loaded.file_existed);
}

TEST(SettingsTest, ExistingFileReportsFileExistedTrue) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "suborno_settings_firstrun2_test.conf";
    std::filesystem::remove(path);

    const suborno::settings::SettingsStore store(path);
    EXPECT_EQ(store.save(suborno::settings::EngineSettings{}),
              suborno::settings::SettingsStatus::ok);
    const auto loaded = store.load();
    EXPECT_EQ(loaded.status, suborno::settings::SettingsStatus::ok);
    EXPECT_TRUE(loaded.file_existed);
    std::filesystem::remove(path);
}

TEST(SettingsTest, SaveCreatesMissingParentDirectories) {
    const std::filesystem::path directory = std::filesystem::temp_directory_path()
                                            / "suborno_settings_nested_test" / "deep" / "hierarchy";
    std::filesystem::remove_all(directory.parent_path());
    const std::filesystem::path path = directory / "settings.ini";
    // No parent exists yet — first launch on a machine where nothing has been
    // created. save() must create the whole hierarchy, not fail.
    ASSERT_FALSE(std::filesystem::exists(directory));

    const suborno::settings::SettingsStore store(path);
    suborno::settings::EngineSettings settings;
    settings.active_layout_id = "probhat";
    EXPECT_EQ(store.save(settings), suborno::settings::SettingsStatus::ok);
    EXPECT_TRUE(std::filesystem::is_directory(directory));

    const auto loaded = store.load();
    EXPECT_EQ(loaded.status, suborno::settings::SettingsStatus::ok);
    EXPECT_EQ(loaded.settings.active_layout_id, "probhat");
    std::filesystem::remove_all(directory.parent_path());
}

TEST(SettingsTest, SettingsPersistAcrossStoreRestarts) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "suborno_settings_restart_test.conf";
    std::filesystem::remove(path);

    suborno::settings::EngineSettings written;
    written.active_layout_id = "suborno-smart";
    written.candidate_limit = 33;
    written.learning_enabled = false;
    written.ui.show_candidate_window = false;
    written.input.layout_switch_shortcut = "F12";

    {
        const suborno::settings::SettingsStore store(path);
        ASSERT_EQ(store.save(written), suborno::settings::SettingsStatus::ok);
    }
    {
        // A brand-new store object, as the application builds one per start:
        // everything written above must come back unchanged.
        const suborno::settings::SettingsStore store(path);
        const auto loaded = store.load();
        ASSERT_EQ(loaded.status, suborno::settings::SettingsStatus::ok);
        EXPECT_EQ(loaded.settings.active_layout_id, "suborno-smart");
        EXPECT_EQ(loaded.settings.candidate_limit, 33U);
        EXPECT_FALSE(loaded.settings.learning_enabled);
        EXPECT_FALSE(loaded.settings.ui.show_candidate_window);
        EXPECT_EQ(loaded.settings.input.layout_switch_shortcut, "F12");
    }
    std::filesystem::remove(path);
}

TEST(SettingsTest, HandlesUnicodeAndSpacesInDirectoryNames) {
    const std::filesystem::path directory =
        std::filesystem::temp_directory_path() / "suborno টেস্ট dir with spaces";
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);
    const std::filesystem::path path = directory / "settings.ini";

    const suborno::settings::SettingsStore store(path);
    suborno::settings::EngineSettings settings;
    settings.active_layout_id = "khipro";
    ASSERT_EQ(store.save(settings), suborno::settings::SettingsStatus::ok);
    const auto loaded = store.load();
    ASSERT_EQ(loaded.status, suborno::settings::SettingsStatus::ok);
    EXPECT_EQ(loaded.settings.active_layout_id, "khipro");
    std::filesystem::remove_all(directory);
}

TEST(SettingsTest, CorruptFileIsRejectedWithoutModification) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "suborno_settings_corrupt_test.conf";
    const std::string corrupt_contents = "this line has no equals sign\n";
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output << corrupt_contents;
    }

    const suborno::settings::SettingsStore store(path);
    const auto loaded = store.load();
    EXPECT_EQ(loaded.status, suborno::settings::SettingsStatus::invalid_format);
    EXPECT_TRUE(loaded.file_existed);

    // Loading must not touch the file — the user's data stays exactly as it
    // was for later inspection or quarantine.
    std::ifstream input(path, std::ios::binary);
    const std::string after{std::istreambuf_iterator<char>(input), {}};
    input.close();
    EXPECT_EQ(after, corrupt_contents);
    std::filesystem::remove(path);
}

TEST(SettingsTest, QuarantinedCorruptFileRecoversWithDefaults) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "suborno_settings_recover_test.conf";
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output << "version=99\nactive_layout=nonsense\n";
    }

    const suborno::settings::SettingsStore store(path);
    const auto first = store.load();
    ASSERT_EQ(first.status, suborno::settings::SettingsStatus::unsupported_version);

    // The application's recovery sequence: quarantine (contents preserved
    // beside the original), then load again from a fresh store.
    ASSERT_EQ(suborno::persistence::quarantine_file(path), std::error_code{});
    EXPECT_FALSE(std::filesystem::exists(path));

    const suborno::settings::SettingsStore fresh(path);
    const auto second = fresh.load();
    EXPECT_EQ(second.status, suborno::settings::SettingsStatus::ok);
    EXPECT_FALSE(second.file_existed);
    EXPECT_EQ(second.settings.active_layout_id, "suborno-phonetic");

    bool quarantined_found = false;
    for (const auto& entry :
         std::filesystem::directory_iterator(std::filesystem::temp_directory_path())) {
        if (entry.path().filename().string().rfind("suborno_settings_recover_test.conf.corrupt-", 0)
            == 0) {
            quarantined_found = true;
            // The stream must be closed before the removal: Windows refuses
            // to delete a file with an open handle (sharing violation).
            std::string contents;
            {
                std::ifstream input(entry.path(), std::ios::binary);
                contents.assign(std::istreambuf_iterator<char>(input), {});
            }
            EXPECT_EQ(contents, "version=99\nactive_layout=nonsense\n");
            std::filesystem::remove(entry.path());
        }
    }
    EXPECT_TRUE(quarantined_found);
}

TEST(SettingsTest, SaveFailureInReadOnlyDirectoryPreservesExistingSettings) {
#if defined(_WIN32)
    GTEST_SKIP() << "Directory write protection is ACL-based on Windows; "
                    "verified by runtime testing instead.";
#else
    if (::geteuid() == 0) {
        GTEST_SKIP() << "Running as root bypasses directory permissions.";
    }
    const std::filesystem::path directory =
        std::filesystem::temp_directory_path() / "suborno_settings_ro_test";
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);
    const std::filesystem::path path = directory / "settings.ini";

    const suborno::settings::SettingsStore store(path);
    suborno::settings::EngineSettings original;
    original.active_layout_id = "khipro";
    ASSERT_EQ(store.save(original), suborno::settings::SettingsStatus::ok);

    std::filesystem::permissions(
        directory, std::filesystem::perms::owner_read | std::filesystem::perms::owner_exec);
    suborno::settings::EngineSettings modified = original;
    modified.active_layout_id = "probhat";
    EXPECT_EQ(store.save(modified), suborno::settings::SettingsStatus::io_error);

    std::filesystem::permissions(directory, std::filesystem::perms::owner_all);
    const auto reloaded = store.load();
    ASSERT_EQ(reloaded.status, suborno::settings::SettingsStatus::ok);
    // The failed write must not have damaged the user's existing settings.
    EXPECT_EQ(reloaded.settings.active_layout_id, "khipro");
    std::filesystem::remove_all(directory);
#endif
}

TEST(SettingsTest, SystemThemeIdRoundTrips) {
    // "system" is the follow-the-Windows-app-mode theme value recognized by
    // the shell (ui::set_active_theme). The store must persist it verbatim.
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "suborno_settings_system_theme_test.ini";
    std::filesystem::remove(path);

    const suborno::settings::SettingsStore store(path);
    suborno::settings::EngineSettings settings;
    settings.ui.theme_id = "system";
    EXPECT_EQ(store.save(settings), suborno::settings::SettingsStatus::ok);
    const auto loaded = store.load();
    EXPECT_EQ(loaded.status, suborno::settings::SettingsStatus::ok);
    EXPECT_EQ(loaded.settings.ui.theme_id, "system");
    std::filesystem::remove(path);
}
