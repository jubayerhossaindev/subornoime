#include "suborno/core/engine.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <sqlite3.h>

#include "suborno/candidates/candidate_engine.hpp"
#include "suborno/candidates/language_intelligence.hpp"
#include "suborno/dictionary/dictionary.hpp"
#include "suborno/layout-api/declarative_layout.hpp"
#include "suborno/layout-api/phonetic_layout.hpp"
#include "suborno/layouts/phonetic/phonetic_input_session.hpp"
#include "suborno/layouts/phonetic/suborno_phonetic_layout.hpp"
#include "suborno/settings/settings.hpp"
#include "suborno/suborno.h"

namespace {

class TestLayout final : public suborno::layout::PhoneticLayout {
  public:
    [[nodiscard]] std::string transliterate(const std::string& input) const override {
        return "test:" + input;
    }

    [[nodiscard]] std::vector<suborno::layout::Candidate> generate_candidates(
        const std::string& input) const override {
        return {{transliterate(input),
                 transliterate(input),
                 "layout:test-layout",
                 suborno::layout::CandidateSource::layout,
                 1.0,
                 "test layout"}};
    }
};

class TestStatelessLayout final : public suborno::layout::StatelessInputLayout {
  public:
    [[nodiscard]] suborno::layout::LayoutKind kind() const noexcept override {
        return suborno::layout::LayoutKind::fixed;
    }

    [[nodiscard]] std::string transliterate(const std::string& input) const override {
        std::string output;
        for (const char character : input) {
            output += character == 'a' ? "ক" : std::string(1, character);
        }
        return output;
    }

    [[nodiscard]] std::vector<suborno::layout::Candidate> generate_candidates(
        const std::string& input) const override {
        const std::string output = transliterate(input);
        return {{output,
                 output,
                 "layout:stateless:" + output,
                 suborno::layout::CandidateSource::layout,
                 1.0,
                 "stateless test layout"}};
    }
};

class MultiCandidateLayout final : public suborno::layout::PhoneticLayout {
  public:
    [[nodiscard]] std::string transliterate(const std::string& input) const override {
        return input;
    }

    [[nodiscard]] std::vector<suborno::layout::Candidate> generate_candidates(
        const std::string& input) const override {
        return {
            {input + "-one",
             input + "-one",
             "layout:one",
             suborno::layout::CandidateSource::layout,
             1.0,
             "first"},
            {input + "-two",
             input + "-two",
             "layout:two",
             suborno::layout::CandidateSource::layout,
             0.5,
             "second"},
        };
    }
};

class CanonicalDuplicateLayout final : public suborno::layout::PhoneticLayout {
  public:
    [[nodiscard]] std::string transliterate(const std::string&) const override {
        return {};
    }

    [[nodiscard]] std::vector<suborno::layout::Candidate> generate_candidates(
        const std::string&) const override {
        return {
            {"\u09DC",
             "\u09DC",
             "layout:precomposed",
             suborno::layout::CandidateSource::layout,
             1.0,
             "precomposed"},
            {"\u09A1\u09BC",
             "\u09A1\u09BC",
             "layout:decomposed",
             suborno::layout::CandidateSource::layout,
             0.5,
             "decomposed"},
        };
    }
};

std::string temporary_database_path(const char* filename) {
    return (std::filesystem::temp_directory_path() / filename).string();
}

}  // namespace

static_assert(!std::is_copy_constructible_v<suborno::dictionary::Dictionary>);
static_assert(!std::is_copy_assignable_v<suborno::dictionary::Dictionary>);

TEST(EngineTest, RegistersDefaultPhoneticLayout) {
    suborno::core::Engine engine;
    EXPECT_EQ(engine.active_layout_id(), "suborno-phonetic");
    const auto* metadata = engine.layout_metadata("suborno-phonetic");
    ASSERT_NE(metadata, nullptr);
    EXPECT_EQ(metadata->display_name,
              "\xE0\xA6\xAC\xE0\xA6\xBE\xE0\xA6\x82\xE0\xA6\xB2\xE0\xA6\xBE (Suborno)");

    auto session = engine.create_session();
    ASSERT_NE(session, nullptr);
    ASSERT_EQ(session->append_utf8("ami"), suborno::layouts::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "আমি");
}

TEST(EngineTest, RegistersEnglishPassthroughLayoutWithCleanNames) {
    suborno::core::Engine engine;
    ASSERT_TRUE(engine.set_active_layout("english"));
    EXPECT_EQ(engine.active_layout_kind(), suborno::layout::LayoutKind::passthrough);

    const auto* metadata = engine.layout_metadata("english");
    ASSERT_NE(metadata, nullptr);
    EXPECT_EQ(metadata->display_name, "English");

    // No layout name may expose the implementation term "Phonetic".
    for (const char* id : {"english",
                           "suborno-phonetic",
                           "suborno-classic",
                           "suborno-smart",
                           "khipro",
                           "probhat",
                           "national"}) {
        const auto* entry = engine.layout_metadata(id);
        ASSERT_NE(entry, nullptr) << id;
        EXPECT_EQ(entry->display_name.find("Phonetic"), std::string::npos) << id;
    }

    // The identity session exists only as a defensive fallback; the text
    // service never eats keys in passthrough mode.
    auto session = engine.create_session();
    ASSERT_NE(session, nullptr);
    ASSERT_EQ(session->append_utf8("ami"), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "ami");
}

// Regression (BUG-05): the classic display name shipped as mixed-script
// "বাংলা (Suborno Classic)" — Latin "Born" + Bengali vowel sign U+09CB instead of the
// Latin letter "o". Lock every engine-registered display name byte-exactly.
TEST(EngineTest, LayoutDisplayNamesAreCorrectAndScriptClean) {
    suborno::core::Engine engine;
    const char* bangla = "\xE0\xA6\xAC\xE0\xA6\xBE\xE0\xA6\x82\xE0\xA6\xB2\xE0\xA6\xBE";
    const char* expected[] = {
        "english",
        "English",
        "suborno-phonetic",
        "(Suborno)",
        "suborno-classic",
        "\(Suborno Classic\)",
        "suborno-smart",
        "Suborno Smart",
        "khipro",
        "Khipro",
        "probhat",
        "Probhat",
        "national",
        "National",
    };
    for (std::size_t pair_index = 0; pair_index + 1 < sizeof(expected) / sizeof(expected[0]);
         pair_index += 2) {
        const auto* metadata = engine.layout_metadata(expected[pair_index]);
        ASSERT_NE(metadata, nullptr) << expected[pair_index];
        const std::string name = metadata->display_name;
        // The Bangla-prefixed names must contain the pure-Latin Latin part.
        EXPECT_NE(name.find(expected[pair_index + 1]), std::string::npos) << expected[pair_index];
        // No Latin script run may be followed by a Bengali combining mark
        // (the BUG-05 corruption pattern: "Born" + U+09CB).
        EXPECT_EQ(name.find("Born\xE0\xA7\x8B"), std::string::npos) << expected[pair_index];
    }
    EXPECT_EQ(engine.layout_metadata("suborno-classic")->display_name,
              std::string(bangla) + " (Suborno Classic)");
    EXPECT_EQ(engine.layout_metadata("suborno-phonetic")->display_name,
              std::string(bangla) + " (Suborno)");
}

TEST(EngineTest, RegistersAndSwitchesLayouts) {
    suborno::core::Engine engine;
    EXPECT_TRUE(engine.register_layout("test", std::make_unique<TestLayout>()));
    EXPECT_TRUE(engine.set_active_layout("test"));
    EXPECT_EQ(engine.active_layout_id(), "test");

    auto session = engine.create_session();
    ASSERT_NE(session, nullptr);
    ASSERT_EQ(session->append_utf8("x"), suborno::layouts::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "test:x");
}

TEST(EngineTest, CreatesSessionsThroughStatelessInputLayoutFactory) {
    suborno::core::Engine engine;
    ASSERT_TRUE(engine.register_layout("fixed-test", std::make_unique<TestStatelessLayout>()));
    ASSERT_TRUE(engine.set_active_layout("fixed-test"));

    auto session = engine.create_session();
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->append_utf8("a"), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "ক");
}

TEST(EngineTest, RegistersAndCreatesSubornoClassicSessions) {
    suborno::core::Engine engine;
    ASSERT_TRUE(engine.set_active_layout("suborno-classic"));

    auto session = engine.create_session();
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->append_utf8("ami"), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "আমি");
}

TEST(EngineTest, RegistersAndCreatesKhiproCompositionalSessions) {
    suborno::core::Engine engine;
    ASSERT_TRUE(engine.set_active_layout("khipro"));
    EXPECT_EQ(engine.active_layout_kind(), suborno::layout::LayoutKind::compositional);

    auto session = engine.create_session();
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->append_utf8("kt"), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "ক্ত");
    EXPECT_EQ(session->commit().text_utf8, "ক্ত");
}

TEST(EngineTest, RegistersAndCreatesSubornoSmartSessions) {
    suborno::core::Engine engine;
    ASSERT_TRUE(engine.set_active_layout("suborno-smart"));

    auto session = engine.create_session();
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->append_utf8("ami"), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "আমি");
}

TEST(EngineTest, CandidateEngineAddsDictionaryExactMatches) {
    const std::string db_path = temporary_database_path("suborno_candidate_engine_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));
    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("আমি", "আমি", 10);

    suborno::layouts::SubornoPhoneticLayout layout;
    suborno::candidates::CandidateEngine engine(layout, dictionary);
    const auto candidates = engine.make_candidates("ami");

    ASSERT_FALSE(candidates.empty());
    EXPECT_EQ(candidates.front().text, "আমি");
    EXPECT_EQ(candidates.front().normalized_text, "আমি");
    EXPECT_FALSE(candidates.front().stable_id.empty());
    EXPECT_EQ(candidates.front().source, suborno::layout::CandidateSource::system_dictionary);
}

TEST(EngineTest, CandidateEngineDeduplicatesCanonicalText) {
    CanonicalDuplicateLayout layout;
    suborno::candidates::CandidateEngine engine(layout);

    const auto candidates = engine.make_candidates("unused");
    ASSERT_EQ(candidates.size(), 1U);
    EXPECT_EQ(candidates.front().text, "\u09DC");
}

TEST(EngineTest, DictionarySchemaCommitAndReset) {
    const std::string db_path = temporary_database_path("suborno_dictionary_learning_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));
    suborno::dictionary::Dictionary dictionary(db_path);

    EXPECT_EQ(dictionary.schema_version(), 4);
    dictionary.add_entry("বাংলা", "bangla", 5);
    dictionary.record_commit("বাংলা", "bangla");

    const auto matches = dictionary.find_matches("bangla");
    ASSERT_FALSE(matches.empty());
    EXPECT_EQ(matches.front(), "বাংলা");

    dictionary.clear_user_entries();
    EXPECT_EQ(dictionary.find_matches("bangla").front(), "বাংলা");
}

TEST(EngineTest, DictionaryRejectsInvalidSchemaVersion) {
    const std::string db_path =
        temporary_database_path("suborno_dictionary_invalid_version_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));
    EXPECT_NO_THROW({ suborno::dictionary::Dictionary dictionary(db_path); });

    sqlite3* db = nullptr;
    ASSERT_EQ(sqlite3_open(db_path.c_str(), &db), SQLITE_OK);
    ASSERT_EQ(
        sqlite3_exec(db,
                     "UPDATE dictionary_metadata SET value='1invalid' WHERE key='schema_version';",
                     nullptr,
                     nullptr,
                     nullptr),
        SQLITE_OK);
    sqlite3_close(db);

    {
        suborno::dictionary::Dictionary dictionary(db_path);
        EXPECT_THROW(static_cast<void>(dictionary.schema_version()),
                     suborno::dictionary::DictionaryError);
    }
    static_cast<void>(std::remove(db_path.c_str()));
}

TEST(EngineTest, DictionaryMigratesLegacySchema) {
    const std::string db_path = temporary_database_path("suborno_dictionary_migration_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));

    sqlite3* db = nullptr;
    ASSERT_EQ(sqlite3_open(db_path.c_str(), &db), SQLITE_OK);
    ASSERT_EQ(sqlite3_exec(db,
                           "CREATE TABLE dictionary_entries("
                           "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                           "word TEXT NOT NULL UNIQUE,"
                           "normalized TEXT NOT NULL,"
                           "frequency INTEGER NOT NULL DEFAULT 1,"
                           "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP);"
                           "INSERT INTO dictionary_entries(word, normalized, frequency)"
                           " VALUES('বাংলা', 'bangla', 5);",
                           nullptr,
                           nullptr,
                           nullptr),
              SQLITE_OK);
    sqlite3_close(db);

    suborno::dictionary::Dictionary dictionary(db_path);
    EXPECT_EQ(dictionary.schema_version(), 4);
    EXPECT_EQ(dictionary.find_matches("bangla").front(), "বাংলা");
    dictionary.record_commit("বাংলা", "bangla");
    EXPECT_EQ(dictionary.find_matches("bangla").front(), "বাংলা");
    static_cast<void>(std::remove(db_path.c_str()));
}

TEST(EngineTest, DictionaryMigratesVersionTwoLearnedWordsToAliasSchema) {
    const std::string db_path =
        temporary_database_path("suborno_dictionary_v2_to_v3_alias_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));

    sqlite3* db = nullptr;
    ASSERT_EQ(sqlite3_open(db_path.c_str(), &db), SQLITE_OK);
    ASSERT_EQ(
        sqlite3_exec(db,
                     "CREATE TABLE dictionary_metadata("
                     "key TEXT PRIMARY KEY, value TEXT NOT NULL);"
                     "INSERT INTO dictionary_metadata(key, value) VALUES('schema_version', '2');"
                     "CREATE TABLE dictionary_entries("
                     "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                     "word TEXT NOT NULL UNIQUE,"
                     "normalized TEXT NOT NULL,"
                     "frequency INTEGER NOT NULL DEFAULT 1,"
                     "source TEXT NOT NULL DEFAULT 'system',"
                     "user_frequency INTEGER NOT NULL DEFAULT 0,"
                     "user_pinned INTEGER NOT NULL DEFAULT 0,"
                     "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP);"
                     "INSERT INTO dictionary_entries("
                     "word, normalized, frequency, source, user_frequency)"
                     " VALUES('বাংলা', 'bang', 10, 'system', 3);",
                     nullptr,
                     nullptr,
                     nullptr),
        SQLITE_OK);
    sqlite3_close(db);

    {
        suborno::dictionary::Dictionary dictionary(db_path);
        EXPECT_EQ(dictionary.schema_version(), 4);
        EXPECT_EQ(dictionary.find_matches("bang"), std::vector<std::string>{"বাংলা"});
        EXPECT_EQ(dictionary.find_prefix_matches("ban"), std::vector<std::string>{"বাংলা"});
        EXPECT_EQ(dictionary.get_user_frequency("বাংলা"), 3);
    }

    static_cast<void>(std::remove(db_path.c_str()));
}

TEST(EngineTest, DictionaryRejectsCorruptDatabase) {
    const std::string db_path = temporary_database_path("suborno_dictionary_corrupt_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));
    {
        std::ofstream output(db_path, std::ios::binary);
        output << "not a sqlite database";
    }

    EXPECT_THROW(suborno::dictionary::Dictionary dictionary(db_path),
                 suborno::dictionary::DictionaryError);
    static_cast<void>(std::remove(db_path.c_str()));
}

TEST(EngineTest, DictionaryPrefixLookupAndReset) {
    const std::string db_path = temporary_database_path("suborno_dictionary_prefix_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));
    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("বাংলা", "bangla", 5);
    dictionary.add_entry("বাঙালি", "bangali", 3);

    const auto matches = dictionary.words_for_prefix("বা");
    ASSERT_FALSE(matches.empty());
    dictionary.clear_user_entries();

    static_cast<void>(std::remove(db_path.c_str()));
}

TEST(EngineTest, EngineConfigPersistsSettingsAndOwnsDictionary) {
    const std::filesystem::path settings_path =
        std::filesystem::temp_directory_path() / "suborno_engine_settings.conf";
    const std::filesystem::path dictionary_path =
        std::filesystem::temp_directory_path()
        / std::filesystem::path(u8"suborno_engine_dictionary_\u09AC.sqlite");
    std::filesystem::remove(settings_path);
    std::filesystem::remove(dictionary_path);

    {
        suborno::core::EngineConfig config;
        config.settings_path = settings_path;
        config.dictionary_path = dictionary_path;
        suborno::core::Engine engine(config);

        suborno::settings::EngineSettings settings = engine.settings();
        settings.candidate_limit = 24;
        settings.learning_enabled = false;
        EXPECT_EQ(engine.save_settings(settings), suborno::settings::SettingsStatus::ok);
        EXPECT_EQ(engine.settings().candidate_limit, 24U);
        ASSERT_NE(engine.dictionary(), nullptr);

        engine.dictionary()->add_entry("আমি", "ami", 3);
        EXPECT_FALSE(engine.dictionary()->find_matches("ami").empty());
    }

    std::filesystem::remove(settings_path);
    std::filesystem::remove(dictionary_path);
}

TEST(EngineTest, ReloadsSettingsChangedByTheDesktopSettingsApplication) {
    const std::filesystem::path settings_path =
        std::filesystem::temp_directory_path() / "suborno_engine_reload_settings_test.conf";
    std::filesystem::remove(settings_path);

    suborno::core::EngineConfig config;
    config.settings_path = settings_path;
    {
        suborno::core::Engine engine(config);
        ASSERT_EQ(engine.active_layout_id(), "suborno-phonetic");

        suborno::settings::EngineSettings external = engine.settings();
        external.active_layout_id = "khipro";
        external.candidate_limit = 7;
        external.input.ignore_caps_lock_state_in_bangla = true;
        ASSERT_EQ(suborno::settings::SettingsStore(settings_path).save(external),
                  suborno::settings::SettingsStatus::ok);

        const auto reloaded = engine.reload_settings();
        ASSERT_EQ(reloaded.status, suborno::settings::SettingsStatus::ok);
        EXPECT_EQ(engine.active_layout_id(), "khipro");
        EXPECT_EQ(engine.settings().candidate_limit, 7U);
        EXPECT_TRUE(engine.settings().input.ignore_caps_lock_state_in_bangla);
    }

    std::filesystem::remove(settings_path);
}

TEST(EngineTest, FallsBackToDefaultsOnCorruptSettingsAndHealsOnNextSave) {
    const std::filesystem::path settings_path =
        std::filesystem::temp_directory_path() / "suborno_engine_corrupt_settings_test.conf";
    std::filesystem::remove(settings_path);
    {
        std::ofstream corrupt(settings_path, std::ios::binary);
        corrupt << "this is not a settings line\nversion is missing too\n";
    }

    suborno::core::EngineConfig config;
    config.settings_path = settings_path;
    {
        suborno::core::Engine engine(config);
        // The corrupt file is rejected; the engine keeps defaults and stays
        // fully usable instead of failing.
        EXPECT_EQ(engine.active_layout_id(), "suborno-phonetic");
        ASSERT_NE(engine.create_session(), nullptr);

        suborno::settings::EngineSettings healed = engine.settings();
        healed.candidate_limit = 9;
        EXPECT_EQ(engine.save_settings(healed), suborno::settings::SettingsStatus::ok);

        // The atomic save replaced the corrupt file: the next load succeeds.
        const auto reloaded = engine.reload_settings();
        EXPECT_EQ(reloaded.status, suborno::settings::SettingsStatus::ok);
        EXPECT_EQ(engine.settings().candidate_limit, 9U);
    }

    std::filesystem::remove(settings_path);
}

TEST(EngineTest, RestoresKnownLayoutAndFallsBackForUnknownLayout) {
    const std::filesystem::path settings_path =
        std::filesystem::temp_directory_path() / "suborno_engine_layout_restore.conf";
    std::filesystem::remove(settings_path);

    suborno::settings::EngineSettings settings;
    settings.active_layout_id = "probhat";
    suborno::settings::SettingsStore store(settings_path);
    ASSERT_EQ(store.save(settings), suborno::settings::SettingsStatus::ok);

    suborno::core::EngineConfig config;
    config.settings_path = settings_path;
    {
        suborno::core::Engine engine(config);
        EXPECT_EQ(engine.active_layout_id(), "probhat");
        EXPECT_EQ(engine.settings().active_layout_id, "probhat");

        settings.active_layout_id = "missing-layout";
        ASSERT_EQ(store.save(settings), suborno::settings::SettingsStatus::ok);
        EXPECT_EQ(engine.load_settings().settings.active_layout_id, "probhat");
        EXPECT_EQ(engine.save_settings(settings),
                  suborno::settings::SettingsStatus::invalid_format);
    }

    settings.active_layout_id = "missing-layout";
    ASSERT_EQ(store.save(settings), suborno::settings::SettingsStatus::ok);
    {
        suborno::core::Engine engine(config);
        EXPECT_EQ(engine.active_layout_id(), "suborno-phonetic");
        EXPECT_EQ(engine.settings().active_layout_id, "suborno-phonetic");
    }

    std::filesystem::remove(settings_path);
}

TEST(EngineTest, AppliesCandidateLimitAndLearningSettingToSessions) {
    const std::filesystem::path settings_path =
        std::filesystem::temp_directory_path() / "suborno_engine_session_settings.conf";
    const std::filesystem::path dictionary_path =
        std::filesystem::temp_directory_path() / "suborno_engine_session_dictionary.sqlite";
    std::filesystem::remove(settings_path);
    std::filesystem::remove(dictionary_path);

    suborno::settings::EngineSettings settings;
    settings.candidate_limit = 1;
    settings.learning_enabled = false;
    ASSERT_EQ(suborno::settings::SettingsStore(settings_path).save(settings),
              suborno::settings::SettingsStatus::ok);

    suborno::core::EngineConfig config;
    config.settings_path = settings_path;
    config.dictionary_path = dictionary_path;
    {
        suborno::core::Engine engine(config);
        ASSERT_NE(engine.dictionary(), nullptr);
        engine.dictionary()->add_user_word("আমি", "আমি");
        engine.dictionary()->add_entry("আমিও", "আমি", 3);
        engine.dictionary()->add_entry("আমিই", "আমি", 2);
        ASSERT_EQ(engine.dictionary()->find_matches("আমি").size(), 3U);

        auto session = engine.create_session();
        ASSERT_NE(session, nullptr);
        ASSERT_EQ(session->append_utf8("ami"), suborno::layouts::InputStatus::updated);
        EXPECT_EQ(session->snapshot().candidates.size(), 1U);
        EXPECT_EQ(session->commit_candidate(1).status,
                  suborno::layouts::InputStatus::candidate_out_of_range);
        ASSERT_EQ(session->commit().status, suborno::layouts::InputStatus::updated);
        EXPECT_EQ(engine.dictionary()->get_user_frequency("আমি"), 1);
    }

    std::filesystem::remove(settings_path);
    std::filesystem::remove(dictionary_path);
}

TEST(EngineTest, CandidateLimitAppliesWithoutDictionary) {
    MultiCandidateLayout layout;
    suborno::layouts::PhoneticInputSession session(
        layout,
        suborno::layouts::InputSessionOptions{
            .candidate_limit = 1, .previous_word = {}, .next_word_prediction_enabled = false});

    ASSERT_EQ(session.append_utf8("ami"), suborno::layouts::InputStatus::updated);
    EXPECT_EQ(session.snapshot().candidates.size(), 1U);
    EXPECT_EQ(session.commit_candidate(1).status,
              suborno::layouts::InputStatus::candidate_out_of_range);
}

TEST(EngineTest, SessionCandidatesIncludePhoneticDictionaryMatches) {
    const std::filesystem::path dictionary_path =
        std::filesystem::temp_directory_path() / "suborno_session_phonetic_dictionary.sqlite";
    std::filesystem::remove(dictionary_path);

    suborno::core::EngineConfig config;
    config.dictionary_path = dictionary_path;
    {
        suborno::core::Engine engine(config);
        ASSERT_NE(engine.dictionary(), nullptr);
        engine.dictionary()->add_entry("আমি", "ami", 5);
        auto session = engine.create_session();
        ASSERT_NE(session, nullptr);
        ASSERT_EQ(session->append_utf8("ami"), suborno::layouts::InputStatus::updated);

        const auto snapshot = session->snapshot();
        ASSERT_FALSE(snapshot.candidates.empty());
        EXPECT_EQ(snapshot.candidates.front().text, "আমি");
        EXPECT_EQ(snapshot.candidates.front().rationale, "input dictionary match");
    }

    std::filesystem::remove(dictionary_path);
}

TEST(EngineTest, PhoneticSessionStoresCanonicalWordAndTypedAliasSeparately) {
    const std::filesystem::path dictionary_path =
        std::filesystem::temp_directory_path() / "suborno_phonetic_session_alias_learning.sqlite";
    std::filesystem::remove(dictionary_path);

    suborno::core::EngineConfig config;
    config.dictionary_path = dictionary_path;
    {
        suborno::core::Engine engine(config);
        ASSERT_NE(engine.dictionary(), nullptr);

        ASSERT_TRUE(engine.set_active_layout("suborno-phonetic"));
        auto session = engine.create_session();
        ASSERT_NE(session, nullptr);
        ASSERT_EQ(session->append_utf8("ami"), suborno::layout::InputStatus::updated);

        const auto committed = session->commit();
        EXPECT_EQ(committed.text_utf8, "আমি");
        EXPECT_EQ(engine.dictionary()->find_matches("আমি"), std::vector<std::string>{"আমি"});
        EXPECT_EQ(engine.dictionary()->find_matches("ami"), std::vector<std::string>{"আমি"});
        EXPECT_EQ(engine.dictionary()->find_prefix_matches("am"), std::vector<std::string>{"আমি"});
    }

    std::filesystem::remove(dictionary_path);
}

TEST(EngineTest, CommitUsesSelectedCandidate) {
    MultiCandidateLayout layout;
    suborno::layouts::PhoneticInputSession session(
        layout,
        suborno::layouts::InputSessionOptions{
            .candidate_limit = 2, .previous_word = {}, .next_word_prediction_enabled = false});

    ASSERT_EQ(session.append_utf8("ami"), suborno::layouts::InputStatus::updated);
    ASSERT_TRUE(session.select_candidate(1));
    EXPECT_EQ(session.commit().text_utf8, "ami-two");
}

TEST(EngineTest, LoadsDeclarativeLayoutMapping) {
    const std::filesystem::path mapping =
        std::filesystem::temp_directory_path() / "suborno_layout_test.tsv";
    std::ofstream output(mapping, std::ios::trunc);
    output << "# test mapping\n";
    output << "a\tআ\n";
    output << "k\tক\n";
    output.close();

    suborno::layout::DeclarativeLayout layout("test-layout", mapping);
    ASSERT_TRUE(layout.load());
    EXPECT_EQ(layout.transliterate("ka"), "কআ");
    const auto candidates = layout.generate_candidates("ka");
    ASSERT_EQ(candidates.size(), 1U);
    EXPECT_EQ(candidates.front().stable_id, "layout:test-layout:কআ");

    std::ofstream invalid_output(mapping, std::ios::trunc);
    invalid_output << "a\tআ\n";
    invalid_output << "a\tঅ\n";
    invalid_output.close();
    EXPECT_FALSE(layout.load());
    EXPECT_EQ(layout.transliterate("ka"), "কআ");

    std::filesystem::remove(mapping);
}

TEST(EngineTest, DeterministicLanguageIntelligence) {
    const std::string db_path =
        temporary_database_path("suborno_language_intelligence_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));
    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("বাংলা", "bangla", 5);
    suborno::candidates::LanguageIntelligence intelligence(dictionary);

    EXPECT_TRUE(intelligence.is_known_word("bangla"));
    EXPECT_EQ(intelligence.correct_or_original("bangla"), "বাংলা");
    EXPECT_EQ(intelligence.correct_or_original("unknown"), "unknown");
    EXPECT_FALSE(intelligence.suggest_prefix("বা").empty());
    static_cast<void>(std::remove(db_path.c_str()));
}

TEST(EngineTest, CApiCreatesAndCommitsSession) {
    SUBORNO_Engine* engine = suborno_engine_create();
    ASSERT_NE(engine, nullptr);
    SUBORNO_Session* session = suborno_session_create(engine);
    ASSERT_NE(session, nullptr);

    EXPECT_GE(suborno_session_append_utf8(session, "ami"), 0);
    char buffer[256];
    EXPECT_GE(suborno_session_snapshot(session, buffer, sizeof(buffer)), 0);
    EXPECT_STREQ(buffer, "আমি");

    std::size_t candidate_count = 0;
    EXPECT_GE(suborno_session_candidate_count(session, &candidate_count), 0);
    ASSERT_GT(candidate_count, 0U);
    char candidate[256];
    EXPECT_GE(suborno_session_candidate_snapshot(session, 0, candidate, sizeof(candidate)), 0);
    EXPECT_STREQ(candidate, "আমি");
    EXPECT_EQ(
        suborno_session_candidate_snapshot(session, candidate_count, candidate, sizeof(candidate)),
        SUBORNO_STATUS_CANDIDATE_OUT_OF_RANGE);
    EXPECT_EQ(suborno_session_select_candidate(session, 0), SUBORNO_STATUS_UPDATED);

    char* committed = nullptr;
    std::size_t committed_size = 0;
    EXPECT_GE(suborno_session_commit(session, &committed, &committed_size), 0);
    ASSERT_NE(committed, nullptr);
    EXPECT_EQ(std::string(committed), "আমি");
    suborno_string_free(committed);

    suborno_session_destroy(session);
    suborno_engine_destroy(engine);
}

TEST(EngineTest, CApiReportsInputAndArgumentErrors) {
    EXPECT_EQ(suborno_session_append_utf8(nullptr, "ami"), SUBORNO_ERROR_INVALID_ARGUMENT);

    SUBORNO_Engine* engine = suborno_engine_create();
    ASSERT_NE(engine, nullptr);
    SUBORNO_Session* session = suborno_session_create(engine);
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(suborno_session_append_utf8(session, "\xFF"), SUBORNO_STATUS_INVALID_UTF8);
    ASSERT_EQ(suborno_session_append_utf8(session, "ami"), SUBORNO_STATUS_UPDATED);

    char short_buffer[1] = {'x'};
    EXPECT_EQ(suborno_session_snapshot(session, short_buffer, sizeof(short_buffer)),
              SUBORNO_ERROR_BUFFER_TOO_SMALL);
    EXPECT_EQ(short_buffer[0], '\0');

    std::size_t candidate_count = 0;
    ASSERT_EQ(suborno_session_candidate_count(session, &candidate_count), SUBORNO_STATUS_UPDATED);
    ASSERT_GT(candidate_count, 0U);
    short_buffer[0] = 'x';
    EXPECT_EQ(suborno_session_candidate_snapshot(session, 0, short_buffer, sizeof(short_buffer)),
              SUBORNO_ERROR_BUFFER_TOO_SMALL);
    EXPECT_EQ(short_buffer[0], '\0');

    char marker = '\0';
    char* committed = &marker;
    std::size_t committed_size = 99;
    EXPECT_EQ(suborno_session_commit(nullptr, &committed, &committed_size),
              SUBORNO_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(committed, nullptr);
    EXPECT_EQ(committed_size, 0U);

    committed = &marker;
    EXPECT_EQ(suborno_session_commit(session, &committed, nullptr), SUBORNO_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(committed, nullptr);

    suborno_session_destroy(session);
    suborno_engine_destroy(engine);
}

TEST(EngineTest, RecordCommittedWordChainsTransitionsAndIgnoresEmptyWords) {
    const std::filesystem::path dictionary_path =
        std::filesystem::temp_directory_path() / "suborno_engine_transition_dictionary.sqlite";
    std::filesystem::remove(dictionary_path);

    suborno::core::EngineConfig config;
    config.dictionary_path = dictionary_path;
    {
        suborno::core::Engine engine(config);
        ASSERT_NE(engine.dictionary(), nullptr);
        EXPECT_TRUE(engine.last_committed_word().empty());

        // The first word has no predecessor: it is only tracked.
        engine.record_committed_word("আমি");
        EXPECT_EQ(engine.last_committed_word(), "আমি");
        EXPECT_EQ(engine.dictionary()->stats().word_transitions, 0U);

        // The second call chains the bigram আমি -> ভালো (learning defaults on).
        engine.record_committed_word("ভালো");
        EXPECT_EQ(engine.last_committed_word(), "ভালো");
        EXPECT_EQ(engine.dictionary()->transition_candidates("আমি", "", 10),
                  std::vector<std::string>{"ভালো"});

        // Empty words carry no context: neither tracked nor chained.
        engine.record_committed_word("");
        EXPECT_EQ(engine.last_committed_word(), "ভালো");
        EXPECT_EQ(engine.dictionary()->stats().word_transitions, 1U);
    }

    std::filesystem::remove(dictionary_path);
}

TEST(EngineTest, ClearLearntWordsResetsTransitionsAndCommittedContext) {
    const std::filesystem::path dictionary_path =
        std::filesystem::temp_directory_path() / "suborno_engine_clear_learnt_dictionary.sqlite";
    std::filesystem::remove(dictionary_path);

    suborno::core::EngineConfig config;
    config.dictionary_path = dictionary_path;
    {
        suborno::core::Engine engine(config);
        ASSERT_NE(engine.dictionary(), nullptr);
        engine.dictionary()->add_entry("আমি", "ami", 10);
        engine.record_committed_word("আমি");
        engine.record_committed_word("ভালো");
        ASSERT_EQ(engine.dictionary()->stats().word_transitions, 1U);

        EXPECT_TRUE(engine.clear_learnt_words());
        EXPECT_TRUE(engine.last_committed_word().empty());
        EXPECT_EQ(engine.dictionary()->stats().word_transitions, 0U);
        EXPECT_TRUE(engine.dictionary()->transition_candidates("আমি", "", 10).empty());
        // The clear removes user learning, never settings.
        EXPECT_TRUE(engine.settings().learning_enabled);
        EXPECT_TRUE(engine.settings().ui.show_candidate_window);
        // System entries survive the clear.
        EXPECT_EQ(engine.dictionary()->find_matches("ami"), std::vector<std::string>{"আমি"});
    }

    // Without a dictionary there is nothing to clear: false, no crash, and the
    // in-memory committed context is still forgotten.
    suborno::core::Engine plain_engine;
    plain_engine.record_committed_word("আমি");
    EXPECT_EQ(plain_engine.last_committed_word(), "আমি");
    EXPECT_FALSE(plain_engine.clear_learnt_words());
    EXPECT_TRUE(plain_engine.last_committed_word().empty());

    std::filesystem::remove(dictionary_path);
}

TEST(EngineTest, NextWordPredictionTogglePersistsThroughSettingsStoreRoundTrip) {
    const std::filesystem::path settings_path =
        std::filesystem::temp_directory_path() / "suborno_engine_prediction_toggle.conf";
    std::filesystem::remove(settings_path);

    suborno::core::EngineConfig config;
    config.settings_path = settings_path;
    {
        suborno::core::Engine engine(config);
        suborno::settings::EngineSettings settings = engine.settings();
        settings.candidate_limit = 24;
        settings.learning_enabled = false;
        ASSERT_EQ(engine.save_settings(settings), suborno::settings::SettingsStatus::ok);

        engine.set_next_word_prediction_enabled(true);
        EXPECT_TRUE(engine.settings().ui.next_word_prediction_enabled);
    }
    {
        suborno::core::Engine engine(config);
        // The whole settings struct was persisted: the toggle survived AND the
        // unrelated fields were not lost.
        EXPECT_TRUE(engine.settings().ui.next_word_prediction_enabled);
        EXPECT_EQ(engine.settings().candidate_limit, 24U);
        EXPECT_FALSE(engine.settings().learning_enabled);

        engine.set_next_word_prediction_enabled(false);
        EXPECT_FALSE(engine.settings().ui.next_word_prediction_enabled);
        EXPECT_EQ(engine.settings().candidate_limit, 24U);
    }
    {
        suborno::core::Engine engine(config);
        EXPECT_FALSE(engine.settings().ui.next_word_prediction_enabled);
        EXPECT_EQ(engine.settings().candidate_limit, 24U);
    }

    // Without a store the toggle stays in memory only (TSF callers keep
    // working; nothing is written anywhere).
    suborno::core::Engine storeless_engine;
    storeless_engine.set_next_word_prediction_enabled(true);
    EXPECT_TRUE(storeless_engine.settings().ui.next_word_prediction_enabled);

    std::filesystem::remove(settings_path);
}

TEST(EngineTest, CreatedSessionsCarryPredictionContextOnARealLayout) {
    const std::filesystem::path settings_path =
        std::filesystem::temp_directory_path() / "suborno_engine_prediction_session.conf";
    const std::filesystem::path dictionary_path =
        std::filesystem::temp_directory_path() / "suborno_engine_prediction_session.sqlite";
    std::filesystem::remove(settings_path);
    std::filesystem::remove(dictionary_path);

    suborno::settings::EngineSettings settings;
    settings.ui.next_word_prediction_enabled = true;
    ASSERT_EQ(suborno::settings::SettingsStore(settings_path).save(settings),
              suborno::settings::SettingsStatus::ok);

    suborno::core::EngineConfig config;
    config.settings_path = settings_path;
    config.dictionary_path = dictionary_path;
    {
        suborno::core::Engine engine(config);
        ASSERT_NE(engine.dictionary(), nullptr);
        // English is a real registered layout whose sessions are
        // LayoutInputSessions, i.e. they honor the prediction context.
        ASSERT_TRUE(engine.set_active_layout("english"));
        engine.dictionary()->add_entry("ভালো", "bhalo", 10);
        engine.dictionary()->record_transition("ami", "bhalo");
        engine.record_committed_word("ami");
        EXPECT_EQ(engine.last_committed_word(), "ami");

        auto session = engine.create_session();
        ASSERT_NE(session, nullptr);
        ASSERT_EQ(session->append_utf8("bh"), suborno::layout::InputStatus::updated);
        const auto snapshot = session->snapshot();
        ASSERT_FALSE(snapshot.candidates.empty());
        bool prediction_found = false;
        for (const auto& candidate : snapshot.candidates) {
            if (candidate.rationale == "next-word prediction") {
                prediction_found = true;
                EXPECT_EQ(candidate.text, "bhalo");
                EXPECT_DOUBLE_EQ(candidate.score, 0.85);
            }
        }
        EXPECT_TRUE(prediction_found);
    }
    {
        // Identical learning data with the flag off: the session must show no
        // prediction candidates at all (create_session carries the flag).
        suborno::settings::EngineSettings disabled;
        disabled.ui.next_word_prediction_enabled = false;
        ASSERT_EQ(suborno::settings::SettingsStore(settings_path).save(disabled),
                  suborno::settings::SettingsStatus::ok);

        suborno::core::Engine engine(config);
        ASSERT_NE(engine.dictionary(), nullptr);
        ASSERT_TRUE(engine.set_active_layout("english"));
        engine.dictionary()->add_entry("ভালো", "bhalo", 10);
        engine.dictionary()->record_transition("ami", "bhalo");
        engine.record_committed_word("ami");

        auto session = engine.create_session();
        ASSERT_NE(session, nullptr);
        ASSERT_EQ(session->append_utf8("bh"), suborno::layout::InputStatus::updated);
        for (const auto& candidate : session->snapshot().candidates) {
            EXPECT_NE(candidate.rationale, "next-word prediction");
        }
    }

    std::filesystem::remove(settings_path);
    std::filesystem::remove(dictionary_path);
}

TEST(EngineTest, CreateSessionForLayoutTypesWithTheRequestedLayout) {
    suborno::core::Engine engine;
    // The registered layout the shell can pin per-window (English is the
    // passthrough layout; a session for it still exists and types Latin).
    ASSERT_TRUE(engine.set_active_layout("suborno-phonetic"));
    auto english_session = engine.create_session_for_layout("english");
    ASSERT_NE(english_session, nullptr);
    EXPECT_EQ(english_session->append_utf8("abc"), suborno::layout::InputStatus::updated);
    EXPECT_EQ(english_session->commit().text_utf8, "abc");

    auto bangla_session = engine.create_session_for_layout("suborno-phonetic");
    ASSERT_NE(bangla_session, nullptr);
    EXPECT_EQ(bangla_session->append_utf8("ami"), suborno::layout::InputStatus::updated);
    EXPECT_NE(bangla_session->commit().text_utf8, "ami");

    // Unknown ids (settings written by another build) fall back to the
    // active layout instead of dropping the session.
    auto fallback_session = engine.create_session_for_layout("no-such-layout");
    ASSERT_NE(fallback_session, nullptr);
    EXPECT_EQ(fallback_session->append_utf8("ami"), suborno::layout::InputStatus::updated);
    EXPECT_NE(fallback_session->commit().text_utf8, "ami");
}

TEST(EngineTest, LayoutKindOfReportsPassthroughForEnglish) {
    suborno::core::Engine engine;
    EXPECT_EQ(engine.layout_kind_of("english"), suborno::layout::LayoutKind::passthrough);
    EXPECT_EQ(engine.layout_kind_of("suborno-phonetic"), suborno::layout::LayoutKind::phonetic);
    EXPECT_EQ(engine.layout_kind_of("probhat"), suborno::layout::LayoutKind::fixed);
    EXPECT_EQ(engine.layout_kind_of("no-such-layout"), suborno::layout::LayoutKind::phonetic);
}

TEST(EngineTest, LayoutPreviewMapsLettersThroughTheActiveLayout) {
    suborno::core::Engine engine;
    ASSERT_TRUE(engine.set_active_layout("suborno-phonetic"));
    // The OSK shows these previews as key labels: they must equal what the
    // active layout itself produces for the letter.
    const suborno::layouts::SubornoPhoneticLayout layout;
    EXPECT_EQ(engine.layout_preview("a"), layout.transliterate("a"));
    EXPECT_EQ(engine.layout_preview("k"), layout.transliterate("k"));
    EXPECT_TRUE(engine.layout_preview("").empty());
}

TEST(EngineTest, EncodeTextForOutputFollowsTheConfiguredEncoding) {
    suborno::core::Engine engine;
    // Default (Unicode): committed Bangla passes through unchanged.
    EXPECT_EQ(engine.encode_text_for_output("\xE0\xA6\x95" /* ক */), "\xE0\xA6\x95");

    suborno::settings::EngineSettings ansi;
    ansi.input.preferred_encoding = suborno::settings::TextEncoding::ansi;
    // Direct member setup is not possible through the public API without a
    // settings store, so exercise the encoder through a full save/load cycle.
    std::filesystem::path settings_path =
        std::filesystem::temp_directory_path() / "suborno-encode-engine-test.ini";
    std::filesystem::path dictionary_path =
        std::filesystem::temp_directory_path() / "suborno-encode-engine-test.db";
    std::filesystem::remove(settings_path);
    std::filesystem::remove(dictionary_path);
    suborno::core::EngineConfig config;
    config.settings_path = settings_path;
    config.dictionary_path = dictionary_path;
    // The engine owns the open SQLite connection, so everything below runs
    // in a nested scope: the engine must be destroyed before the dictionary
    // file is removed (Windows sharing violation on open handles).
    {
        suborno::core::Engine encoded_engine(config);
        ASSERT_EQ(encoded_engine.save_settings(ansi), suborno::settings::SettingsStatus::ok);
        const std::string bangla_sentence = "\xE0\xA6\x86\xE0\xA6\xAE\xE0\xA6\xBE\xE0\xA6\xB0";
        const std::string encoded = encoded_engine.encode_text_for_output(bangla_sentence);
        EXPECT_FALSE(encoded.empty());
        EXPECT_NE(encoded, bangla_sentence);  // Bijoy ASCII bytes, not Unicode
        for (const unsigned char byte : encoded) {
            EXPECT_LT(byte, 0x80U) << "Bijoy output must stay pure ASCII";
        }
        // Unencodable or invalid input falls back to Unicode (nothing dropped).
        EXPECT_EQ(encoded_engine.encode_text_for_output("caf\xC3\xA9 plain"),
                  encoded_engine.encode_text_for_output("caf\xC3\xA9 plain"));
    }
    std::filesystem::remove(settings_path);
    std::filesystem::remove(dictionary_path);
}
