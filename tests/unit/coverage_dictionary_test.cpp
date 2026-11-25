// Dictionary + spell coverage batch (task 2-b).

//

// Targets the line/branch coverage gaps reported for

// src/dictionary/dictionary.cpp and src/spell/spell_checker.cpp:

//   * word/normalized validation across every UTF-8 shape and boundary,

//   * validation, clamp and upsert semantics of add_entry / add_entry_merge,

//   * bulk-write transaction guards (nested begin, orphan commit, rollback),

//   * vacuum + backup failure paths,

//   * schema migration from v1/v3 databases and rejection of future schemas,

//   * SQL-fault injection through a second SQLite connection (dropped tables,

//     CHECK constraints, RAISE(ABORT) triggers) so prepare/step/rollback error

//     paths are exercised through the public API only,

//   * suggestion ranking, tie-breaking, suggestion caps and edit cap in the

//     spell checker.

//

// Deterministic: no sleeps, no network, no skipped tests.

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <gtest/gtest.h>
#include <limits>
#include <sqlite3.h>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "suborno/dictionary/dictionary.hpp"
#include "suborno/spell/spell_checker.hpp"

namespace {

std::string temporary_database_path(const std::string& filename) {
    return (std::filesystem::temp_directory_path() / filename).string();
}

void remove_database_files(const std::string& base_path) {
    static_cast<void>(std::remove(base_path.c_str()));
    static_cast<void>(std::remove((base_path + "-wal").c_str()));
    static_cast<void>(std::remove((base_path + "-shm").c_str()));
}

// RAII second connection used to inject SQL-level faults (drop tables, add

// CHECK constraints, RAISE(ABORT) triggers) against a live dictionary file.

class RawDatabase {
  public:
    explicit RawDatabase(const std::string& path) {
        const int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
        if (sqlite3_open_v2(path.c_str(), &db_, flags, nullptr) != SQLITE_OK) {
            const std::string message =
                db_ != nullptr ? sqlite3_errmsg(db_) : "sqlite3_open_v2 failed";
            if (db_ != nullptr) {
                sqlite3_close(db_);
                db_ = nullptr;
            }
            throw std::runtime_error("RawDatabase open failed: " + message);
        }
    }
    ~RawDatabase() {
        if (db_ != nullptr) {
            sqlite3_close(db_);
        }
    }
    RawDatabase(const RawDatabase&) = delete;
    RawDatabase& operator=(const RawDatabase&) = delete;

    void exec(const std::string& sql) {
        char* error = nullptr;
        const int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &error);
        if (rc != SQLITE_OK) {
            const std::string message = error != nullptr ? error : "unknown SQLite error";
            sqlite3_free(error);
            throw std::runtime_error("RawDatabase exec failed (" + std::to_string(rc)
                                     + "): " + message + " [" + sql + "]");
        }
    }

  private:
    sqlite3* db_ = nullptr;
};

// Runs `action`, expects a DictionaryError and returns it for inspection.

suborno::dictionary::DictionaryError expect_dictionary_error(const std::function<void()>& action) {
    try {
        action();
    } catch (const suborno::dictionary::DictionaryError& error) {
        return error;
    }
    ADD_FAILURE() << "expected DictionaryError but no exception was thrown";
    throw std::runtime_error("expected DictionaryError but no exception was thrown");
}

// Recreates dictionary_entries with a CHECK that every normal write violates,

// so sqlite3_step returns SQLITE_CONSTRAINT through the public write APIs.

constexpr const char* entries_table_with_impossible_check =
    "DROP TABLE dictionary_entries;"
    "CREATE TABLE dictionary_entries("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  word TEXT NOT NULL UNIQUE,"
    "  normalized TEXT NOT NULL,"
    "  frequency INTEGER NOT NULL DEFAULT 1,"
    "  source TEXT NOT NULL DEFAULT 'system',"
    "  user_frequency INTEGER NOT NULL DEFAULT 0,"
    "  user_pinned INTEGER NOT NULL DEFAULT 0,"
    "  created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
    "  category TEXT NOT NULL DEFAULT '',"
    "  register TEXT NOT NULL DEFAULT '',"
    "  origin TEXT NOT NULL DEFAULT 'seed',"
    "  CHECK(frequency > 1000000000));";

class DictionaryCoverageTest : public ::testing::Test {
  protected:
    void SetUp() override {
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        db_path_ =
            temporary_database_path(std::string("suborno_cov_dict_") + info->name() + ".sqlite");
        remove_database_files(db_path_);
    }
    void TearDown() override {
        remove_database_files(db_path_);
    }

    std::string db_path_;
};

class SpellCheckerCoverageTest : public ::testing::Test {
  protected:
    void SetUp() override {
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        db_path_ =
            temporary_database_path(std::string("suborno_cov_spell_") + info->name() + ".sqlite");
        remove_database_files(db_path_);
    }
    void TearDown() override {
        remove_database_files(db_path_);
    }

    std::string db_path_;
};

// ZWNJ as explicit bytes (U+200C) so no source-charset assumptions are needed.

std::string zwnj() {
    return {static_cast<char>(0xE2), static_cast<char>(0x80), static_cast<char>(0x8C)};
}

// Control bytes are prepended with push_back so C++ hex-escape greediness can

// never swallow following hex digits ('b'-'f') of the trailing text.

std::string with_control_byte(char byte, const std::string& rest) {
    std::string out;
    out.push_back(byte);
    out += rest;
    return out;
}

}  // namespace

// ---------------------------------------------------------------------------

// Static validation (dictionary.cpp lines 139-164 and their UTF-8 branches)

// ---------------------------------------------------------------------------

TEST(DictionaryValidationTest, WordValidationCoversEveryUtf8ShapeAndBoundary) {
    using suborno::dictionary::Dictionary;
    // Empty and ASCII-only inputs.

    EXPECT_FALSE(Dictionary::is_valid_word(""));
    EXPECT_FALSE(Dictionary::is_valid_word("ami"));

    // Two-byte sequence (U+00E9): exercises the 2-byte lead branch, but has no

    // Bengali character.

    const std::string two_byte = {static_cast<char>(0xC3), static_cast<char>(0xA9)};
    EXPECT_FALSE(Dictionary::is_valid_word(two_byte));

    // Three-byte Bengali: the accepted shape.

    EXPECT_TRUE(Dictionary::is_valid_word("স্বপ্ন"));

    // Four-byte sequence (U+1F600): exercises the 4-byte lead branch.

    const std::string four_byte = {static_cast<char>(0xF0),
                                   static_cast<char>(0x9F),
                                   static_cast<char>(0x98),
                                   static_cast<char>(0x80)};
    EXPECT_FALSE(Dictionary::is_valid_word(four_byte));

    // Truncated multi-byte sequence (missing continuation byte).

    const std::string truncated = {static_cast<char>(0xE0), static_cast<char>(0xA6)};
    EXPECT_FALSE(Dictionary::is_valid_word(truncated));

    // Broken continuation byte inside an otherwise valid 3-byte sequence.

    const std::string bad_continuation = {static_cast<char>(0xE0), 'A', static_cast<char>(0xA6)};
    EXPECT_FALSE(Dictionary::is_valid_word(bad_continuation));

    // Invalid lead byte (0x80 can never start a sequence).

    const std::string bad_lead = {static_cast<char>(0x80)};
    EXPECT_FALSE(Dictionary::is_valid_word(bad_lead));

    // Bengali digits are explicitly rejected inside words.

    EXPECT_FALSE(Dictionary::is_valid_word("০৯"));

    // ZWNJ/ZWJ combinators are allowed, but only together with Bengali text.

    EXPECT_TRUE(Dictionary::is_valid_word("ক" + zwnj() + "ষ"));
    EXPECT_FALSE(Dictionary::is_valid_word(zwnj()));

    // Byte-length boundary: 96 bytes (32 Bengali letters) is valid, 99 is not.

    std::string exactly_96_bytes;
    std::string over_96_bytes;
    for (int index = 0; index < 32; ++index) {
        exactly_96_bytes += "ক";
    }
    for (int index = 0; index < 33; ++index) {
        over_96_bytes += "ক";
    }
    EXPECT_TRUE(Dictionary::is_valid_word(exactly_96_bytes));
    EXPECT_FALSE(Dictionary::is_valid_word(over_96_bytes));
}

TEST(DictionaryValidationTest, NormalizedValidationRejectsControlsDelAndOversize) {
    using suborno::dictionary::Dictionary;
    EXPECT_TRUE(Dictionary::is_valid_normalized("ami bhai"));
    EXPECT_TRUE(Dictionary::is_valid_normalized("বাংলা"));
    EXPECT_FALSE(Dictionary::is_valid_normalized(""));
    EXPECT_FALSE(Dictionary::is_valid_normalized(std::string("\x7F")));  // DEL

    EXPECT_FALSE(Dictionary::is_valid_normalized(std::string("\x1F")));  // control char

    EXPECT_FALSE(Dictionary::is_valid_normalized(std::string(97, 'k')));
    EXPECT_TRUE(Dictionary::is_valid_normalized(std::string(96, 'k')));
}

// ---------------------------------------------------------------------------

// Dictionary error plumbing and validation paths

// ---------------------------------------------------------------------------

TEST_F(DictionaryCoverageTest, DictionaryErrorExposesCategoryAndSqliteCode) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    const suborno::dictionary::DictionaryError error = expect_dictionary_error([&] {
        dictionary.add_entry("ami", "ami");
    });
    EXPECT_EQ(error.category(), suborno::dictionary::DictionaryErrorCategory::validation);
    EXPECT_EQ(error.sqlite_code(), SQLITE_MISMATCH);
    EXPECT_NE(std::string(error.what()).find("Invalid dictionary word"), std::string::npos);
}

TEST_F(DictionaryCoverageTest, AddEntryRejectsInvalidWordsNormalizedKeysAndDigits) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    expect_dictionary_error([&] {
        dictionary.add_entry("ami", "ami");
    });
    expect_dictionary_error([&] {
        dictionary.add_entry("", "bangla");
    });
    expect_dictionary_error([&] {
        dictionary.add_entry("বাংলা", "");
    });
    expect_dictionary_error([&] {
        dictionary.add_entry("বাংলা", with_control_byte(0x01, "bangla"));
    });
    expect_dictionary_error([&] {
        dictionary.add_entry("বাংলা", with_control_byte(0x7F, "bangla"));
    });
    // Bengali digits never occur inside a word (rejected by is_valid_word).

    expect_dictionary_error([&] {
        dictionary.add_entry("বাংলা০৯", "banglanine");
    });
    // The database stays untouched by every rejected call.

    EXPECT_TRUE(dictionary.all_entries().empty());
}

TEST_F(DictionaryCoverageTest, AddEntryClampsFrequenciesIntoStorageRange) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    dictionary.add_entry("কমশব্দ", "low", -5);
    dictionary.add_entry("সীমাশব্দ", "max", std::numeric_limits<int>::max());
    dictionary.add_entry("শূন্যশব্দ", "zero", 0);

    const auto records = dictionary.all_entries();
    ASSERT_EQ(records.size(), 3U);
    for (const auto& record : records) {
        if (record.normalized == "low") {
            EXPECT_EQ(record.frequency, 0);  // negative clamped to zero

        } else if (record.normalized == "max") {
            EXPECT_EQ(record.frequency, std::numeric_limits<int>::max());
        } else {
            EXPECT_EQ(record.normalized, "zero");
            EXPECT_EQ(record.frequency, 0);
        }
    }
}

TEST_F(DictionaryCoverageTest, AddEntryMergeRejectsInvalidInputsAndClampsFrequency) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    expect_dictionary_error([&] {
        dictionary.add_entry_merge("ami", "ami");
    });
    expect_dictionary_error([&] {
        dictionary.add_entry_merge("বাংলা", with_control_byte(0x1F, "bangla"));
    });

    dictionary.add_entry_merge("মার্জশব্দ", "merged", -9);
    dictionary.add_entry_merge("টপশব্দ", "topped", std::numeric_limits<int>::max());
    const auto records = dictionary.all_entries();
    ASSERT_EQ(records.size(), 2U);
    for (const auto& record : records) {
        if (record.normalized == "merged") {
            EXPECT_EQ(record.frequency, 0);
        } else {
            EXPECT_EQ(record.normalized, "topped");
            EXPECT_EQ(record.frequency, std::numeric_limits<int>::max());
        }
    }
}

// ---------------------------------------------------------------------------

// Constructor / open paths

// ---------------------------------------------------------------------------

TEST_F(DictionaryCoverageTest, ConstructorFailsWhenParentDirectoryIsAFile) {
    const std::string blocker_path = temporary_database_path("suborno_cov_dict_blocker_file");
    {
        std::ofstream blocker(blocker_path, std::ios::binary);
        blocker << "not a directory";
    }
    const std::string nested_path =
        (std::filesystem::path(blocker_path) / "nested" / "dict.sqlite").string();
    const auto error = expect_dictionary_error([&] {
        suborno::dictionary::Dictionary d(nested_path);
    });
    EXPECT_EQ(error.category(), suborno::dictionary::DictionaryErrorCategory::open);
    EXPECT_NE(std::string(error.what()).find("Unable to create dictionary directory"),
              std::string::npos);
    static_cast<void>(std::remove(blocker_path.c_str()));
    remove_database_files(nested_path);
}

TEST_F(DictionaryCoverageTest, ConstructorFailsWhenDatabasePathIsADirectory) {
    const std::string directory_path = temporary_database_path("suborno_cov_dict_blocker_dir");
    static_cast<void>(std::filesystem::create_directory(directory_path));
    const auto error = expect_dictionary_error([&] {
        suborno::dictionary::Dictionary d(directory_path);
    });
    EXPECT_EQ(error.category(), suborno::dictionary::DictionaryErrorCategory::open);
    EXPECT_FALSE(std::string(error.what()).empty());
    static_cast<void>(std::filesystem::remove(directory_path));
}

TEST_F(DictionaryCoverageTest, ConstructorAcceptsBareFilenameWithoutParentDirectory) {
    // A path with no directory component skips create_directories entirely

    // (the parent_path().empty() branch) and opens the file in the CWD.

    const std::string bare_name = "suborno_cov_dict_relative.sqlite";
    const std::filesystem::path created = std::filesystem::current_path() / bare_name;
    static_cast<void>(std::remove(created.string().c_str()));

    {
        suborno::dictionary::Dictionary dictionary(bare_name);
        EXPECT_EQ(dictionary.schema_version(), 4);
        dictionary.add_entry("বাংলা", "bangla", 1);
        EXPECT_EQ(dictionary.find_matches("bangla"), std::vector<std::string>{"বাংলা"});
    }
    static_cast<void>(std::remove((created.string() + "-wal").c_str()));
    static_cast<void>(std::remove((created.string() + "-shm").c_str()));
    static_cast<void>(std::remove(created.string().c_str()));
}

// ---------------------------------------------------------------------------

// Dropped-table injection: sqlite3_prepare_v2 defers schema errors to the

// first sqlite3_step, so these failures surface under the step category.

// ---------------------------------------------------------------------------

TEST_F(DictionaryCoverageTest, CachedLookupPrepareFailureWhenEntriesTableMissing) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    {
        RawDatabase raw(db_path_);
        raw.exec("DROP TABLE dictionary_entries;");
    }
    const auto match_error = expect_dictionary_error([&] {
        static_cast<void>(dictionary.find_matches("bangla"));
    });
    EXPECT_EQ(match_error.category(), suborno::dictionary::DictionaryErrorCategory::step);
    EXPECT_EQ(match_error.sqlite_code(), SQLITE_ERROR);
    EXPECT_NE(std::string(match_error.what()).find("no such table"), std::string::npos);

    expect_dictionary_error([&] {
        static_cast<void>(dictionary.find_prefix_matches("bang"));
    });
    expect_dictionary_error([&] {
        static_cast<void>(dictionary.words_for_prefix("বাং"));
    });
    expect_dictionary_error([&] {
        static_cast<void>(dictionary.list_user_words());
    });
    expect_dictionary_error([&] {
        static_cast<void>(dictionary.get_user_frequency("বাংলা"));
    });
}

TEST_F(DictionaryCoverageTest, WriterPrepareFailuresWhenEntriesTableMissing) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    {
        RawDatabase raw(db_path_);
        raw.exec("DROP TABLE dictionary_entries;");
    }
    expect_dictionary_error([&] {
        dictionary.add_entry("বাংলা", "bangla", 1);
    });
    expect_dictionary_error([&] {
        dictionary.add_entry_merge("বাংলা", "bangla", 1);
    });
    expect_dictionary_error([&] {
        dictionary.set_word_category("বাংলা", "noun");
    });
    expect_dictionary_error([&] {
        static_cast<void>(dictionary.all_entries());
    });
    expect_dictionary_error([&] {
        static_cast<void>(dictionary.words_by_category("noun"));
    });
    expect_dictionary_error([&] {
        static_cast<void>(dictionary.find_duplicate_normalized());
    });
    expect_dictionary_error([&] {
        static_cast<void>(dictionary.stats());
    });
}

TEST_F(DictionaryCoverageTest, RecordTransitionThrowsWhenTransitionsTableMissing) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    {
        RawDatabase raw(db_path_);
        raw.exec("DROP TABLE word_transitions;");
    }
    // sqlite3_prepare_v2 defers the "no such table" error to the first

    // sqlite3_step, so the failure surfaces under the step category.

    const auto error = expect_dictionary_error([&] {
        dictionary.record_transition("ami", "ভালো");
    });
    EXPECT_EQ(error.category(), suborno::dictionary::DictionaryErrorCategory::step);
    EXPECT_NE(std::string(error.what()).find("no such table"), std::string::npos);
    // The lazily-prepared prediction query fails the same way (step category).

    expect_dictionary_error([&] {
        static_cast<void>(dictionary.transition_candidates("ami", "", 5));
    });
}

TEST_F(DictionaryCoverageTest, RemoveUserWordThrowsWhenAliasTableMissing) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    dictionary.add_user_word("স্বপ্ন", "swapon");
    {
        RawDatabase raw(db_path_);
        raw.exec("DROP TABLE dictionary_user_aliases;");
    }
    const auto error = expect_dictionary_error([&] {
        dictionary.remove_user_word("স্বপ্ন");
    });
    EXPECT_EQ(error.category(), suborno::dictionary::DictionaryErrorCategory::step);
    EXPECT_NE(std::string(error.what()).find("no such table"), std::string::npos);
    // The transaction rolled back: the user word is still pinned.

    EXPECT_EQ(dictionary.get_user_frequency("স্বপ্ন"), 1);
    EXPECT_EQ(dictionary.list_user_words(), std::vector<std::string>{"স্বপ্ন"});
}

TEST_F(DictionaryCoverageTest, RelatedWordsThrowsWhenRelationsTableMissing) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    dictionary.add_entry("শান্তি", "shanti", 1);
    {
        RawDatabase raw(db_path_);
        raw.exec("DROP TABLE word_relations;");
    }
    const auto error = expect_dictionary_error([&] {
        static_cast<void>(dictionary.related_words("শান্তি"));
    });
    EXPECT_EQ(error.category(), suborno::dictionary::DictionaryErrorCategory::step);
    EXPECT_NE(std::string(error.what()).find("no such table"), std::string::npos);
}

TEST_F(DictionaryCoverageTest, AddWordRelationThrowsWhenRelationsTableMissing) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    dictionary.add_entry("শান্তি", "shanti", 1);
    dictionary.add_entry("প্রশান্তি", "proshanti", 1);
    {
        RawDatabase raw(db_path_);
        raw.exec("DROP TABLE word_relations;");
    }
    expect_dictionary_error([&] {
        dictionary.add_word_relation("শান্তি", "প্রশান্তি", "synonym");
    });
    expect_dictionary_error([&] {
        static_cast<void>(dictionary.all_relations());
    });
}

TEST_F(DictionaryCoverageTest, AllRelationsRoundTripEmptyAndPrepareFailure) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    // Fresh database: the relations query must return an empty list.

    EXPECT_TRUE(dictionary.all_relations().empty());

    dictionary.add_entry("শান্তি", "shanti", 1);
    dictionary.add_entry("প্রশান্তি", "proshanti", 1);
    dictionary.add_entry("আলো", "alo", 1);
    dictionary.add_entry("আন্ধার", "andhar", 1);
    dictionary.add_word_relation("শান্তি", "প্রশান্তি", "synonym");
    dictionary.add_word_relation("শান্তি", "অশান্তি", "antonym");
    dictionary.add_word_relation("আলো", "আন্ধার", "antonym");

    // ORDER BY word, kind, related with the "kind:related" payload column.

    EXPECT_EQ(
        dictionary.all_relations(),
        (std::vector<std::pair<std::string, std::string>>{
            {"আলো", "antonym:আন্ধার"}, {"শান্তি", "antonym:অশান্তি"}, {"শান্তি", "synonym:প্রশান্তি"}}));

    {
        RawDatabase raw(db_path_);
        raw.exec("DROP TABLE word_relations;");
    }
    expect_dictionary_error([&] {
        static_cast<void>(dictionary.all_relations());
    });
}

TEST_F(DictionaryCoverageTest, SchemaVersionThrowsWhenMetadataMissing) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    {
        RawDatabase raw(db_path_);
        raw.exec("DROP TABLE dictionary_metadata;");
    }
    const auto error = expect_dictionary_error([&] {
        static_cast<void>(dictionary.schema_version());
    });
    EXPECT_EQ(error.category(), suborno::dictionary::DictionaryErrorCategory::step);
    EXPECT_NE(std::string(error.what()).find("no such table"), std::string::npos);
}

// ---------------------------------------------------------------------------

// Step-failure injection (CHECK constraint on the recreated entries table)

// ---------------------------------------------------------------------------

TEST_F(DictionaryCoverageTest, AddEntryAndMergeThrowWhenInsertViolatesConstraint) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    {
        RawDatabase raw(db_path_);
        raw.exec(entries_table_with_impossible_check);
    }
    const auto add_error = expect_dictionary_error([&] {
        dictionary.add_entry("বাংলা", "bangla", 5);
    });
    EXPECT_EQ(add_error.category(), suborno::dictionary::DictionaryErrorCategory::step);
    EXPECT_EQ(add_error.sqlite_code(), SQLITE_CONSTRAINT);
    expect_dictionary_error([&] {
        dictionary.add_entry_merge("ভারত", "bharat", 3);
    });
    // Neither insert survived: the statements aborted before committing.

    EXPECT_TRUE(dictionary.all_entries().empty());
}

TEST_F(DictionaryCoverageTest, RecordCommitRollsBackWhenEntryInsertViolatesConstraint) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    {
        RawDatabase raw(db_path_);
        raw.exec(entries_table_with_impossible_check);
    }
    const auto error = expect_dictionary_error([&] {
        dictionary.record_commit("স্বপ্ন", "swapon");
    });
    EXPECT_EQ(error.category(), suborno::dictionary::DictionaryErrorCategory::step);
    // The ROLLBACK in record_commit's catch block undid the whole transaction.

    EXPECT_EQ(dictionary.get_user_frequency("স্বপ্ন"), 0);
    EXPECT_TRUE(dictionary.list_user_words().empty());
}

TEST_F(DictionaryCoverageTest, AddUserWordRollsBackWhenInsertViolatesConstraint) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    {
        RawDatabase raw(db_path_);
        raw.exec(entries_table_with_impossible_check);
    }
    const auto error = expect_dictionary_error([&] {
        dictionary.add_user_word("স্বপ্ন", "swapon");
    });
    EXPECT_EQ(error.category(), suborno::dictionary::DictionaryErrorCategory::step);
    EXPECT_EQ(dictionary.get_user_frequency("স্বপ্ন"), 0);
    EXPECT_TRUE(dictionary.list_user_words().empty());
}

// ---------------------------------------------------------------------------

// Trigger-based step failures (RAISE(ABORT))

// ---------------------------------------------------------------------------

TEST_F(DictionaryCoverageTest, RemoveUserWordRollsBackWhenDeleteIsBlocked) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    dictionary.add_user_word("স্বপ্ন", "swapon");
    {
        RawDatabase raw(db_path_);
        raw.exec(
            "CREATE TRIGGER block_alias_delete BEFORE DELETE ON dictionary_user_aliases"
            " BEGIN SELECT RAISE(ABORT, 'alias delete blocked'); END;");
    }
    const auto error = expect_dictionary_error([&] {
        dictionary.remove_user_word("স্বপ্ন");
    });
    EXPECT_EQ(error.category(), suborno::dictionary::DictionaryErrorCategory::step);
    EXPECT_EQ(error.sqlite_code(), SQLITE_CONSTRAINT);
    // Rollback restored the pre-transaction learning state.

    EXPECT_EQ(dictionary.get_user_frequency("স্বপ্ন"), 1);
    EXPECT_EQ(dictionary.list_user_words(), std::vector<std::string>{"স্বপ্ন"});
}

TEST_F(DictionaryCoverageTest, ClearUserEntriesRollsBackWhenPurgeIsBlocked) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    dictionary.add_user_word("স্বপ্ন", "swapon");
    {
        RawDatabase raw(db_path_);
        raw.exec(
            "CREATE TRIGGER block_alias_delete BEFORE DELETE ON dictionary_user_aliases"
            " BEGIN SELECT RAISE(ABORT, 'purge blocked'); END;");
    }
    const auto error = expect_dictionary_error([&] {
        dictionary.clear_user_entries();
    });
    EXPECT_EQ(error.category(), suborno::dictionary::DictionaryErrorCategory::step);
    EXPECT_EQ(error.sqlite_code(), SQLITE_CONSTRAINT);
    // Nothing was purged: aliases and user rows survive the rollback.

    EXPECT_EQ(dictionary.list_user_words(), std::vector<std::string>{"স্বপ্ন"});
    EXPECT_EQ(dictionary.stats().user_aliases, 1U);
}

TEST_F(DictionaryCoverageTest, SetWordCategoryRollsBackWhenUpdateIsBlocked) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    dictionary.add_entry("স্বপ্ন", "swapon", 5, "old");
    {
        RawDatabase raw(db_path_);
        raw.exec(
            "CREATE TRIGGER block_entry_update BEFORE UPDATE ON dictionary_entries"
            " BEGIN SELECT RAISE(ABORT, 'category update blocked'); END;");
    }
    const auto error = expect_dictionary_error([&] {
        dictionary.set_word_category("স্বপ্ন", "new");
    });
    EXPECT_EQ(error.category(), suborno::dictionary::DictionaryErrorCategory::step);
    EXPECT_EQ(error.sqlite_code(), SQLITE_CONSTRAINT);
    const auto records = dictionary.all_entries();
    ASSERT_EQ(records.size(), 1U);
    EXPECT_EQ(records.front().category, "old");
}

TEST_F(DictionaryCoverageTest, AddWordRelationThrowsWhenInsertIsBlocked) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    dictionary.add_entry("শান্তি", "shanti", 1);
    dictionary.add_entry("প্রশান্তি", "proshanti", 1);
    {
        RawDatabase raw(db_path_);
        raw.exec(
            "CREATE TRIGGER block_relation_insert BEFORE INSERT ON word_relations"
            " BEGIN SELECT RAISE(ABORT, 'relation blocked'); END;");
    }
    const auto error = expect_dictionary_error([&] {
        dictionary.add_word_relation("শান্তি", "প্রশান্তি", "synonym");
    });
    EXPECT_EQ(error.category(), suborno::dictionary::DictionaryErrorCategory::step);
    EXPECT_EQ(error.sqlite_code(), SQLITE_CONSTRAINT);
    EXPECT_TRUE(dictionary.related_words("শান্তি").empty());
}

TEST_F(DictionaryCoverageTest, AddWordRelationRejectsInvalidWordsAndKinds) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    dictionary.add_entry("শব্দ", "shobdo", 1);
    dictionary.add_entry("অন্য", "onno", 1);
    // Roman "words" are invalid on either side of the relation.

    expect_dictionary_error([&] {
        dictionary.add_word_relation("ami", "শব্দ", "synonym");
    });
    expect_dictionary_error([&] {
        dictionary.add_word_relation("শব্দ", "ami", "synonym");
    });
    // Only synonym/variant/antonym are accepted kinds.

    expect_dictionary_error([&] {
        dictionary.add_word_relation("শব্দ", "অন্য", "hypernym");
    });
    EXPECT_TRUE(dictionary.all_relations().empty());
}

// ---------------------------------------------------------------------------

// Changelog bookkeeping is best-effort

// ---------------------------------------------------------------------------

TEST_F(DictionaryCoverageTest, LogEventFailureIsBestEffortForUserWrites) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    {
        RawDatabase raw(db_path_);
        raw.exec("DROP TABLE dictionary_changelog;");
    }
    // prepare failure inside log_event must not break the real operation.

    dictionary.add_user_word("নিজস্ব", "nijoshsho");
    dictionary.record_commit("স্বপ্ন", "swapon");
    EXPECT_EQ(dictionary.get_user_frequency("নিজস্ব"), 1);
    EXPECT_EQ(dictionary.get_user_frequency("স্বপ্ন"), 1);
    // Both words carry user_frequency 1: ties are ordered by word ascending

    // (Bengali code-point order: ন < স).

    EXPECT_EQ(dictionary.list_user_words(), (std::vector<std::string>{"নিজস্ব", "স্বপ্ন"}));
}

// ---------------------------------------------------------------------------

// Bulk write transaction guards

// ---------------------------------------------------------------------------

TEST_F(DictionaryCoverageTest, BulkWriteCommitPersistsEntries) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    dictionary.begin_bulk_write();
    dictionary.add_entry("এক", "one", 1);
    dictionary.add_entry("দুই", "two", 2);
    dictionary.add_entry("তিন", "three", 3);
    dictionary.commit_bulk_write();

    EXPECT_EQ(dictionary.find_matches("two"), std::vector<std::string>{"দুই"});
    EXPECT_EQ(dictionary.all_entries().size(), 3U);
    EXPECT_EQ(dictionary.schema_version(), 4);
}

TEST_F(DictionaryCoverageTest, BulkWriteRollbackDiscardsPendingEntries) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    dictionary.begin_bulk_write();
    dictionary.add_entry("রোলব্যাক", "rollback", 1);
    dictionary.rollback_bulk_write();

    EXPECT_TRUE(dictionary.find_matches("rollback").empty());
    EXPECT_TRUE(dictionary.all_entries().empty());
}

TEST_F(DictionaryCoverageTest, BulkWriteBoundariesRejectNestedBeginAndOrphanCommit) {
    suborno::dictionary::Dictionary dictionary(db_path_);

    // Orphan commit with no open transaction.

    const auto commit_error = expect_dictionary_error([&] {
        dictionary.commit_bulk_write();
    });
    EXPECT_EQ(commit_error.category(), suborno::dictionary::DictionaryErrorCategory::step);
    EXPECT_NE(std::string(commit_error.what()).find("no transaction"), std::string::npos);

    // Nested BEGIN inside an already open bulk transaction.

    dictionary.begin_bulk_write();
    const auto begin_error = expect_dictionary_error([&] {
        dictionary.begin_bulk_write();
    });
    EXPECT_EQ(begin_error.category(), suborno::dictionary::DictionaryErrorCategory::step);
    EXPECT_NE(std::string(begin_error.what()).find("within a transaction"), std::string::npos);
    dictionary.rollback_bulk_write();

    // The connection recovered: a complete bulk cycle still persists data.

    dictionary.begin_bulk_write();
    dictionary.add_entry("পুনরুদ্ধার", "recovered", 1);
    dictionary.commit_bulk_write();
    EXPECT_EQ(dictionary.find_matches("recovered"), std::vector<std::string>{"পুনরুদ্ধার"});
}

// ---------------------------------------------------------------------------

// Vacuum and backup maintenance paths

// ---------------------------------------------------------------------------

TEST_F(DictionaryCoverageTest, VacuumCompactsAndPreservesData) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    dictionary.add_entry("এক", "one", 1);
    dictionary.add_entry("দুই", "two", 2);
    dictionary.add_entry("তিন", "three", 3);
    dictionary.add_word_relation("এক", "দুই", "variant");
    dictionary.remove_user_word("এক");  // no-op on system entries, exercises logging

    dictionary.add_user_word("নিজস্ব", "nijoshsho");
    dictionary.remove_user_word("নিজস্ব");

    dictionary.vacuum();

    EXPECT_EQ(dictionary.schema_version(), 4);
    EXPECT_EQ(dictionary.find_matches("one"), std::vector<std::string>{"এক"});
    EXPECT_EQ(dictionary.find_matches("two"), std::vector<std::string>{"দুই"});
    EXPECT_TRUE(dictionary.list_user_words().empty());
    const auto stats = dictionary.stats();
    EXPECT_EQ(stats.total_entries, 3U);
    EXPECT_EQ(stats.word_relations, 1U);
}

TEST_F(DictionaryCoverageTest, VacuumThrowsInsideOpenBulkTransaction) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    dictionary.begin_bulk_write();
    const auto error = expect_dictionary_error([&] {
        dictionary.vacuum();
    });
    EXPECT_EQ(error.category(), suborno::dictionary::DictionaryErrorCategory::step);
    EXPECT_NE(std::string(error.what()).find("transaction"), std::string::npos);
    dictionary.rollback_bulk_write();
}

TEST_F(DictionaryCoverageTest, BackupRejectsDirectoryDestination) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    dictionary.add_entry("বাংলা", "bangla", 1);
    const std::string directory_path = temporary_database_path("suborno_cov_dict_backup_dir");
    static_cast<void>(std::filesystem::create_directory(directory_path));
    const auto error = expect_dictionary_error([&] {
        dictionary.backup_to(directory_path);
    });
    EXPECT_EQ(error.category(), suborno::dictionary::DictionaryErrorCategory::open);
    EXPECT_EQ(error.sqlite_code(), SQLITE_CANTOPEN);
    static_cast<void>(std::filesystem::remove(directory_path));
}

TEST_F(DictionaryCoverageTest, BackupFailsWhenDestinationIsLocked) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    dictionary.add_entry("বাংলা", "bangla", 1);
    const std::string backup_path = db_path_ + ".backup";
    remove_database_files(backup_path);
    {
        RawDatabase locked_destination(backup_path);
        locked_destination.exec("BEGIN IMMEDIATE;");
        const auto error = expect_dictionary_error([&] {
            dictionary.backup_to(backup_path);
        });
        EXPECT_TRUE(error.category() == suborno::dictionary::DictionaryErrorCategory::open
                    || error.category() == suborno::dictionary::DictionaryErrorCategory::step)
            << "unexpected category " << static_cast<int>(error.category());
        locked_destination.exec("ROLLBACK;");
    }
    remove_database_files(backup_path);
}

// ---------------------------------------------------------------------------

// Schema version handling and migrations

// ---------------------------------------------------------------------------

TEST_F(DictionaryCoverageTest, ConstructorRejectsNewerSchemaVersion) {
    {
        suborno::dictionary::Dictionary dictionary(db_path_);
        ASSERT_EQ(dictionary.schema_version(), 4);
    }
    {
        RawDatabase raw(db_path_);
        raw.exec("UPDATE dictionary_metadata SET value='5' WHERE key='schema_version';");
    }
    const auto error = expect_dictionary_error([&] {
        suborno::dictionary::Dictionary reopened(db_path_);
    });
    EXPECT_EQ(error.category(), suborno::dictionary::DictionaryErrorCategory::schema);
    EXPECT_NE(std::string(error.what()).find("newer than this engine"), std::string::npos);
}

TEST_F(DictionaryCoverageTest, ConstructorSurvivesUnparsableVersionButQueriesFail) {
    {
        suborno::dictionary::Dictionary dictionary(db_path_);
        ASSERT_EQ(dictionary.schema_version(), 4);
    }
    {
        RawDatabase raw(db_path_);
        raw.exec("UPDATE dictionary_metadata SET value='nonsense' WHERE key='schema_version';");
    }
    // Deferred contract: construction succeeds, the error surfaces on demand.

    suborno::dictionary::Dictionary dictionary(db_path_);
    const auto error = expect_dictionary_error([&] {
        static_cast<void>(dictionary.schema_version());
    });
    EXPECT_EQ(error.category(), suborno::dictionary::DictionaryErrorCategory::schema);
    EXPECT_NE(std::string(error.what()).find("Invalid dictionary schema version"),
              std::string::npos);
}

TEST_F(DictionaryCoverageTest, MigrationFromV1SchemaAddsAllManagementColumns) {
    {
        RawDatabase raw(db_path_);
        raw.exec(
            "CREATE TABLE dictionary_metadata(key TEXT PRIMARY KEY, value TEXT NOT NULL);"
            "INSERT INTO dictionary_metadata(key, value) VALUES('schema_version', '1');"
            "CREATE TABLE dictionary_entries("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  word TEXT NOT NULL UNIQUE,"
            "  normalized TEXT NOT NULL,"
            "  frequency INTEGER NOT NULL DEFAULT 1,"
            "  created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP);"
            "INSERT INTO dictionary_entries(word, normalized, frequency)"
            " VALUES('বাংলা', 'bangla', 7);"
            "INSERT INTO dictionary_entries(word, normalized, frequency)"
            " VALUES('ভারত', 'bharat', 3);");
    }
    suborno::dictionary::Dictionary dictionary(db_path_);
    ASSERT_EQ(dictionary.schema_version(), 4);

    const auto records = dictionary.all_entries();
    ASSERT_EQ(records.size(), 2U);
    for (const auto& record : records) {
        EXPECT_EQ(record.source, "system");
        EXPECT_EQ(record.user_frequency, 0);
        EXPECT_FALSE(record.user_pinned);
        EXPECT_EQ(record.category, "");
        EXPECT_EQ(record.word_register, "");
        EXPECT_EQ(record.origin, "seed");
        if (record.word == "বাংলা") {
            EXPECT_EQ(record.frequency, 7);
        } else {
            EXPECT_EQ(record.word, "ভারত");
            EXPECT_EQ(record.frequency, 3);
        }
    }
    // The migrated columns are writable through the normal API.

    dictionary.add_entry("নতুনশব্দ", "notun", 4, "কিতাব", "cholito", "corpus");
    const auto updated = dictionary.all_entries();
    ASSERT_EQ(updated.size(), 3U);
    bool found = false;
    for (const auto& record : updated) {
        if (record.word == "নতুনশব্দ") {
            found = true;
            EXPECT_EQ(record.category, "কিতাব");
            EXPECT_EQ(record.word_register, "cholito");
            EXPECT_EQ(record.origin, "corpus");
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(DictionaryCoverageTest, MigrationFromV3SchemaAddsOnlyMissingColumns) {
    {
        RawDatabase raw(db_path_);
        raw.exec(
            "CREATE TABLE dictionary_metadata(key TEXT PRIMARY KEY, value TEXT NOT NULL);"
            "INSERT INTO dictionary_metadata(key, value) VALUES('schema_version', '3');"
            "CREATE TABLE dictionary_entries("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  word TEXT NOT NULL UNIQUE,"
            "  normalized TEXT NOT NULL,"
            "  frequency INTEGER NOT NULL DEFAULT 1,"
            "  source TEXT NOT NULL DEFAULT 'system',"
            "  user_frequency INTEGER NOT NULL DEFAULT 0,"
            "  user_pinned INTEGER NOT NULL DEFAULT 0,"
            "  created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP);"
            "INSERT INTO dictionary_entries(word, normalized, frequency)"
            " VALUES('বাংলা', 'bangla', 7);");
    }
    suborno::dictionary::Dictionary dictionary(db_path_);
    ASSERT_EQ(dictionary.schema_version(), 4);
    const auto records = dictionary.all_entries();
    ASSERT_EQ(records.size(), 1U);
    EXPECT_EQ(records.front().word, "বাংলা");
    EXPECT_EQ(records.front().category, "");
    EXPECT_EQ(records.front().origin, "seed");
    // category/register/origin columns added by the migration are usable.

    dictionary.set_word_category("বাংলা", "literary");
    EXPECT_EQ(dictionary.words_by_category("literary"), std::vector<std::string>{"বাংলা"});
}

TEST_F(DictionaryCoverageTest, ConstructorFailsWhenVersionBumpToTwoIsBlocked) {
    {
        suborno::dictionary::Dictionary dictionary(db_path_);
        ASSERT_EQ(dictionary.schema_version(), 4);
    }
    {
        RawDatabase raw(db_path_);
        raw.exec(
            "UPDATE dictionary_metadata SET value='1' WHERE key='schema_version';"
            "CREATE TRIGGER block_version_update BEFORE UPDATE ON dictionary_metadata"
            " BEGIN SELECT RAISE(ABORT, 'version bump blocked'); END;");
    }
    const auto error = expect_dictionary_error([&] {
        suborno::dictionary::Dictionary reopened(db_path_);
    });
    EXPECT_EQ(error.category(), suborno::dictionary::DictionaryErrorCategory::schema);
    EXPECT_NE(std::string(error.what()).find("Unable to update dictionary schema version"),
              std::string::npos);
}

TEST_F(DictionaryCoverageTest, ConstructorFailsWhenVersionBumpToThreeIsBlocked) {
    {
        suborno::dictionary::Dictionary dictionary(db_path_);
        ASSERT_EQ(dictionary.schema_version(), 4);
    }
    {
        RawDatabase raw(db_path_);
        raw.exec(
            "UPDATE dictionary_metadata SET value='2' WHERE key='schema_version';"
            "CREATE TRIGGER block_version_update BEFORE UPDATE ON dictionary_metadata"
            " BEGIN SELECT RAISE(ABORT, 'version bump blocked'); END;");
    }
    expect_dictionary_error([&] {
        suborno::dictionary::Dictionary reopened(db_path_);
    });
}

TEST_F(DictionaryCoverageTest, ConstructorFailsWhenVersionBumpToFourIsBlocked) {
    {
        suborno::dictionary::Dictionary dictionary(db_path_);
        ASSERT_EQ(dictionary.schema_version(), 4);
    }
    {
        RawDatabase raw(db_path_);
        raw.exec(
            "UPDATE dictionary_metadata SET value='3' WHERE key='schema_version';"
            "CREATE TRIGGER block_version_update BEFORE UPDATE ON dictionary_metadata"
            " BEGIN SELECT RAISE(ABORT, 'version bump blocked'); END;");
    }
    expect_dictionary_error([&] {
        suborno::dictionary::Dictionary reopened(db_path_);
    });
}

TEST_F(DictionaryCoverageTest, ConstructorFailsWhenLearnedAliasMigrationIsBlocked) {
    {
        suborno::dictionary::Dictionary dictionary(db_path_);
        ASSERT_EQ(dictionary.schema_version(), 4);
    }
    {
        RawDatabase raw(db_path_);
        raw.exec(
            "INSERT INTO dictionary_entries(word, normalized, frequency, source, user_frequency)"
            " VALUES('আমি', 'ami', 1, 'system', 2);"
            "CREATE TRIGGER block_alias_migration BEFORE INSERT ON dictionary_user_aliases"
            " BEGIN SELECT RAISE(ABORT, 'alias migration blocked'); END;");
    }
    const auto error = expect_dictionary_error([&] {
        suborno::dictionary::Dictionary reopened(db_path_);
    });
    EXPECT_EQ(error.category(), suborno::dictionary::DictionaryErrorCategory::schema);
    EXPECT_NE(std::string(error.what()).find("Unable to migrate learned dictionary aliases"),
              std::string::npos);
}

// ---------------------------------------------------------------------------

// Lookup edge cases and LRU cache behaviour

// ---------------------------------------------------------------------------

TEST_F(DictionaryCoverageTest, FindMatchesRejectsInvalidKeysAndZeroLimit) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    dictionary.add_entry("বাংলা", "bangla", 1);
    EXPECT_TRUE(dictionary.find_matches(with_control_byte(0x01, "junk")).empty());
    EXPECT_TRUE(dictionary.find_matches(with_control_byte(0x7F, "key")).empty());
    EXPECT_TRUE(dictionary.find_matches("", 0).empty());
    EXPECT_TRUE(dictionary.find_matches("bangla", 0).empty());
    EXPECT_EQ(dictionary.find_matches("bangla"), std::vector<std::string>{"বাংলা"});
}

TEST_F(DictionaryCoverageTest, FindPrefixMatchesRejectsEmptyInvalidAndZeroLimit) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    dictionary.add_entry("বাংলা", "bangla", 1);
    EXPECT_TRUE(dictionary.find_prefix_matches("", 5).empty());
    EXPECT_TRUE(dictionary.find_prefix_matches(with_control_byte(0x1F, "bad"), 5).empty());
    EXPECT_TRUE(dictionary.find_prefix_matches(std::string(1, 0x7F), 5).empty());
    EXPECT_TRUE(dictionary.find_prefix_matches("bang", 0).empty());
    EXPECT_EQ(dictionary.find_prefix_matches("bang"), std::vector<std::string>{"বাংলা"});
}

TEST_F(DictionaryCoverageTest, WordsForPrefixServesCacheHitsAndRejectsInvalidPrefixes) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    dictionary.add_entry("বাংলা", "bangla", 3);
    dictionary.add_entry("বাংলাদেশ", "bangladesh", 2);

    const std::vector<std::string> expected{"বাংলা", "বাংলাদেশ"};
    EXPECT_EQ(dictionary.words_for_prefix("বাং"), expected);
    // Second identical call is served from the LRU cache with equal results.

    EXPECT_EQ(dictionary.words_for_prefix("বাং"), expected);

    EXPECT_TRUE(dictionary.words_for_prefix("").empty());
    EXPECT_TRUE(dictionary.words_for_prefix(with_control_byte(0x01, "bad")).empty());
    EXPECT_TRUE(dictionary.words_for_prefix("জাপান").empty());
}

TEST_F(DictionaryCoverageTest, QueryCacheEvictsLeastRecentlyUsedBeyondCapacity) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    dictionary.add_entry("বাংলা", "bangla", 1);

    // cache_capacity is 512: pushing 520 distinct keys must evict the oldest.

    for (int index = 0; index < 520; ++index) {
        const std::string probe = "probe" + std::to_string(index);
        EXPECT_TRUE(dictionary.find_matches(probe).empty()) << probe;
    }
    // The first probe was evicted; re-querying it still returns consistent data.

    EXPECT_TRUE(dictionary.find_matches("probe0").empty());
    EXPECT_EQ(dictionary.find_matches("bangla"), std::vector<std::string>{"বাংলা"});
}

TEST_F(DictionaryCoverageTest, TransitionCandidatesEscapeWildcardPrefixes) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    dictionary.record_transition("prev", "a_b");
    dictionary.record_transition("prev", "axb");
    dictionary.record_transition("prev", "a%b");
    dictionary.record_transition("other", "a_b");

    // Wildcards in the prefix must be matched literally, not as patterns.

    EXPECT_EQ(dictionary.transition_candidates("prev", "a_", 5), std::vector<std::string>{"a_b"});
    EXPECT_EQ(dictionary.transition_candidates("prev", "a%", 5), std::vector<std::string>{"a%b"});
    EXPECT_EQ(dictionary.transition_candidates("prev", "axb", 5), std::vector<std::string>{"axb"});
    // Unfiltered ties are ordered by next_normalized ascending.

    EXPECT_EQ(dictionary.transition_candidates("prev", "", 5),
              (std::vector<std::string>{"a%b", "a_b", "axb"}));
    // A different previous word never leaks into the prediction.

    EXPECT_EQ(dictionary.transition_candidates("other", "a_", 5), std::vector<std::string>{"a_b"});
}

TEST_F(DictionaryCoverageTest, RelatedWordsHonorsKindFilterAndLimit) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    dictionary.add_entry("শান্তি", "shanti", 1);
    dictionary.add_entry("প্রশান্তি", "proshanti", 1);
    dictionary.add_entry("অশান্তি", "oshanti", 1);
    dictionary.add_entry("স্থিরতা", "sthirota", 1);
    dictionary.add_word_relation("শান্তি", "প্রশান্তি", "synonym");
    dictionary.add_word_relation("শান্তি", "অশান্তি", "antonym");
    dictionary.add_word_relation("শান্তি", "স্থিরতা", "variant");

    // Insertion (rowid) order without a kind filter, capped by the limit.

    EXPECT_EQ(dictionary.related_words("শান্তি", "", 2),
              (std::vector<std::string>{"প্রশান্তি", "অশান্তি"}));
    EXPECT_EQ(dictionary.related_words("শান্তি", "synonym"), std::vector<std::string>{"প্রশান্তি"});
    EXPECT_EQ(dictionary.related_words("শান্তি", "variant"), std::vector<std::string>{"স্থিরতা"});
    EXPECT_TRUE(dictionary.related_words("শান্তি", "variant", 0).empty());
    EXPECT_TRUE(dictionary.related_words("পথশব্দ").empty());
}

TEST_F(DictionaryCoverageTest, WordsByCategoryZeroLimitAndUnknownCategory) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    dictionary.add_entry("স্বপ্ন", "swapon", 900, "conjunct");
    EXPECT_TRUE(dictionary.words_by_category("conjunct", 0).empty());
    EXPECT_TRUE(dictionary.words_by_category("missing").empty());
    EXPECT_EQ(dictionary.words_by_category("conjunct"), std::vector<std::string>{"স্বপ্ন"});
}

TEST_F(DictionaryCoverageTest, FindDuplicateNormalizedOrdersGroupsByCopies) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    dictionary.add_entry("এক", "shared", 10);
    dictionary.add_entry("দুই", "shared", 20);
    dictionary.add_entry("তিন", "shared", 30);
    dictionary.add_entry("চার", "other", 40);
    dictionary.add_entry("পাঁচ", "other", 50);

    const auto groups = dictionary.find_duplicate_normalized();
    ASSERT_EQ(groups.size(), 2U);
    // Groups come ordered by duplicate count descending.

    EXPECT_EQ(groups[0].normalized, "shared");
    // Members are ordered by frequency + user_frequency descending.

    EXPECT_EQ(groups[0].words, (std::vector<std::string>{"তিন", "দুই", "এক"}));
    EXPECT_EQ(groups[1].normalized, "other");
    EXPECT_EQ(groups[1].words, (std::vector<std::string>{"পাঁচ", "চার"}));
}

// ---------------------------------------------------------------------------

// Stats content

// ---------------------------------------------------------------------------

TEST_F(DictionaryCoverageTest, StatsOnEmptyDatabaseReportsZeros) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    const auto stats = dictionary.stats();
    EXPECT_EQ(stats.total_entries, 0U);
    EXPECT_EQ(stats.system_entries, 0U);
    EXPECT_EQ(stats.user_entries, 0U);
    EXPECT_EQ(stats.user_aliases, 0U);
    EXPECT_EQ(stats.word_relations, 0U);
    EXPECT_EQ(stats.word_transitions, 0U);
    EXPECT_EQ(stats.changelog_entries, 0U);
    EXPECT_TRUE(stats.top_categories.empty());
    ASSERT_EQ(stats.frequency_tiers.size(), 5U);
    const std::vector<std::string> expected_labels{">=1000", "500-999", "100-499", "50-99", "<50"};
    for (std::size_t index = 0; index < stats.frequency_tiers.size(); ++index) {
        EXPECT_EQ(stats.frequency_tiers[index].first, expected_labels[index]);
        EXPECT_EQ(stats.frequency_tiers[index].second, 0U);
    }
}

TEST_F(DictionaryCoverageTest, StatsPopulatesTiersAndTopCategories) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    dictionary.add_entry("সহস্রশব্দ", "thousand", 1500, "noun");
    dictionary.add_entry("সপ্তশব্দ", "seven-hundred", 700, "noun");
    dictionary.add_entry("দুইশশব্দ", "two-hundred", 250, "noun");
    dictionary.add_entry("ষাটশব্দ", "sixty", 60, "verb");
    dictionary.add_entry("দশশব্দ", "ten", 10);

    const auto stats = dictionary.stats();
    EXPECT_EQ(stats.total_entries, 5U);
    EXPECT_EQ(stats.system_entries, 5U);
    EXPECT_EQ(stats.user_entries, 0U);
    EXPECT_EQ(stats.word_transitions, 0U);

    // One entry lands in each frequency tier.

    ASSERT_EQ(stats.frequency_tiers.size(), 5U);
    EXPECT_EQ(stats.frequency_tiers[0], (std::pair<std::string, std::uint64_t>{">=1000", 1U}));
    EXPECT_EQ(stats.frequency_tiers[1], (std::pair<std::string, std::uint64_t>{"500-999", 1U}));
    EXPECT_EQ(stats.frequency_tiers[2], (std::pair<std::string, std::uint64_t>{"100-499", 1U}));
    EXPECT_EQ(stats.frequency_tiers[3], (std::pair<std::string, std::uint64_t>{"50-99", 1U}));
    EXPECT_EQ(stats.frequency_tiers[4], (std::pair<std::string, std::uint64_t>{"<50", 1U}));

    // Categories are bucketed with empty categories reported as uncategorized.

    ASSERT_EQ(stats.top_categories.size(), 3U);
    EXPECT_EQ(stats.top_categories[0], (std::pair<std::string, std::uint64_t>{"noun", 3U}));
    std::uint64_t smaller_buckets = 0;
    for (std::size_t index = 1; index < stats.top_categories.size(); ++index) {
        EXPECT_EQ(stats.top_categories[index].second, 1U);
        smaller_buckets += stats.top_categories[index].second;
    }
    EXPECT_EQ(smaller_buckets, 2U);  // verb + uncategorized
}

// ---------------------------------------------------------------------------

// Spell checker coverage

// ---------------------------------------------------------------------------

TEST_F(SpellCheckerCoverageTest, CheckRejectsEmptyAndMalformedInput) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    const suborno::spell::SpellChecker checker(dictionary);

    const auto empty_result = checker.check("");
    EXPECT_FALSE(empty_result.is_correct);
    EXPECT_TRUE(empty_result.suggestions.empty());

    const auto invalid_utf8 = checker.check(std::string({static_cast<char>(0xFF)}));
    EXPECT_FALSE(invalid_utf8.is_correct);
    EXPECT_TRUE(invalid_utf8.suggestions.empty());

    const auto truncated_utf8 = checker.check(std::string({static_cast<char>(0xC3)}));
    EXPECT_FALSE(truncated_utf8.is_correct);
    EXPECT_TRUE(truncated_utf8.suggestions.empty());
}

TEST_F(SpellCheckerCoverageTest, CheckAcceptsUserWordsThroughAlias) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    dictionary.add_user_word("নিজস্ব", "nijoshsho");
    const suborno::spell::SpellChecker checker(dictionary);
    const auto result = checker.check("nijoshsho");
    EXPECT_TRUE(result.is_correct);
    EXPECT_TRUE(result.suggestions.empty());
}

TEST_F(SpellCheckerCoverageTest, CheckRanksSuggestionsByScoreThenAlphabet) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    // Every deletion of "abcde" plus a two-edit neighbour ("bcd", reached only

    // through the second edit pass) is stored as a user word.

    dictionary.add_user_word("bcde", "bcde");
    dictionary.add_user_word("acde", "acde");
    dictionary.add_user_word("abde", "abde");
    dictionary.add_user_word("abce", "abce");
    dictionary.add_user_word("abcd", "abcd");
    dictionary.add_user_word("bcd", "bcd");

    const suborno::spell::SpellChecker checker(dictionary);
    const auto result = checker.check("abcde");

    EXPECT_FALSE(result.is_correct);
    ASSERT_EQ(result.suggestions.size(), 6U);
    // Distance-1 candidates share a score and fall back to alphabetical

    // ordering; the distance-2 candidate ranks strictly lower.

    std::vector<std::string> texts;
    for (const auto& suggestion : result.suggestions) {
        texts.push_back(suggestion.text);
    }
    EXPECT_EQ(texts, (std::vector<std::string>{"abcd", "abce", "abde", "acde", "bcde", "bcd"}));
    for (std::size_t index = 0; index < 5; ++index) {
        EXPECT_DOUBLE_EQ(result.suggestions[index].score, 0.5);
    }
    EXPECT_DOUBLE_EQ(result.suggestions[5].score, 1.0 / 3.0);

    // Suggestion payload: dictionary word twice, "spell:"-stable id, user

    // dictionary source, fixed rationale.

    const auto& first = result.suggestions[0];
    EXPECT_EQ(first.text, "abcd");
    EXPECT_EQ(first.normalized_text, "abcd");
    EXPECT_EQ(first.stable_id, "spell:abcd");
    EXPECT_EQ(first.source, suborno::layout::CandidateSource::user_dictionary);
    EXPECT_EQ(first.rationale, "spelling suggestion");
}

TEST_F(SpellCheckerCoverageTest, CheckTruncatesSuggestionsAtTen) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    // Eleven distance-1 deletion neighbours; only ten may surface.

    const char* deletions[] = {"bcdefghijk",
                               "acdefghijk",
                               "abdefghijk",
                               "abcefghijk",
                               "abcdfghijk",
                               "abcdeghijk",
                               "abcdefhijk",
                               "abcdefgijk",
                               "abcdefghjk",
                               "abcdefghik",
                               "abcdefghij"};
    for (const char* word : deletions) {
        dictionary.add_user_word(word, word);
    }

    const suborno::spell::SpellChecker checker(dictionary);
    const auto result = checker.check("abcdefghijk");
    ASSERT_EQ(result.suggestions.size(), 10U);
    std::vector<std::string> texts;
    for (const auto& suggestion : result.suggestions) {
        texts.push_back(suggestion.text);
        EXPECT_DOUBLE_EQ(suggestion.score, 0.5);
    }
    EXPECT_EQ(texts,
              (std::vector<std::string>{"abcdefghij",
                                        "abcdefghik",
                                        "abcdefghjk",
                                        "abcdefgijk",
                                        "abcdefhijk",
                                        "abcdeghijk",
                                        "abcdfghijk",
                                        "abcefghijk",
                                        "abdefghijk",
                                        "acdefghijk"}));
    // The alphabetically largest neighbour was cut by the cap.

    EXPECT_EQ(std::find(texts.begin(), texts.end(), std::string("bcdefghijk")), texts.end());
}

TEST_F(SpellCheckerCoverageTest, CheckScoresAliasMatchesAndBreaksScoreTiesAlphabetically) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    // Both alias keys are one edit away from "aab" ("aa" by deletion, "ab" by

    // deleting the middle 'a'). The distance is measured against the edit

    // string, so both alias words score 0.5 and the tie breaks alphabetically.

    dictionary.add_user_word(std::string(96, 'a'), "ab");
    dictionary.add_user_word("aa", "aa");

    const suborno::spell::SpellChecker checker(dictionary);
    const auto result = checker.check("aab");
    EXPECT_FALSE(result.is_correct);
    ASSERT_EQ(result.suggestions.size(), 2U);
    EXPECT_EQ(result.suggestions[0].text, "aa");
    EXPECT_DOUBLE_EQ(result.suggestions[0].score, 0.5);
    EXPECT_EQ(result.suggestions[1].text, std::string(96, 'a'));
    EXPECT_DOUBLE_EQ(result.suggestions[1].score, 0.5);
}

TEST_F(SpellCheckerCoverageTest, CheckSkipsCandidateEditsBeyondTheGraphemeLimit) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    // A 33-grapheme entry is only reachable as an insertion edit of a

    // 32-grapheme word. edit_distance() throws length_error on such edits and

    // consider_candidate() must swallow it instead of escaping check().

    dictionary.add_user_word(std::string(33, 'a'), std::string(33, 'a'));

    const suborno::spell::SpellChecker checker(dictionary);
    const auto result = checker.check(std::string(32, 'a'));
    EXPECT_FALSE(result.is_correct);
    // The over-limit insertion edit is skipped; the remaining edits ("a"*31,

    // replacement alphabet) have no dictionary entries.

    EXPECT_TRUE(result.suggestions.empty());
}

TEST_F(SpellCheckerCoverageTest, CheckSuggestsBanglaCorrectionsFromBanglaKeys) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    // The normalized key is the Bangla word itself so generated Bangla edits

    // can match it (exercises the Bengali replacement table path).

    dictionary.add_entry("বাংলা", "বাংলা", 5);

    const suborno::spell::SpellChecker checker(dictionary);
    const auto edits = checker.generate_edits("বাংলাa");
    EXPECT_NE(std::find(edits.begin(), edits.end(), std::string("বাংলা")), edits.end());

    const auto result = checker.check("বাংলাa");
    EXPECT_FALSE(result.is_correct);
    ASSERT_EQ(result.suggestions.size(), 1U);
    EXPECT_EQ(result.suggestions[0].text, "বাংলা");
    EXPECT_DOUBLE_EQ(result.suggestions[0].score, 0.5);
}

TEST_F(SpellCheckerCoverageTest, GenerateEditsCapsAtMaximumGeneratedEdits) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    const suborno::spell::SpellChecker checker(dictionary);
    // Six distinct Bengali consonants: the raw candidate space (~660) exceeds

    // maximum_generated_edits (512), so every cap/break branch fires.

    const auto edits = checker.generate_edits("কখগঘঙচ");
    ASSERT_EQ(edits.size(), 512U);
    // The seen-set must guarantee uniqueness under the cap.

    const std::unordered_set<std::string> unique(edits.begin(), edits.end());
    EXPECT_EQ(unique.size(), 512U);
    // Transposition, deletion and the self-substitution are all present.

    EXPECT_NE(std::find(edits.begin(), edits.end(), std::string("খকগঘঙচ")), edits.end());
    EXPECT_NE(std::find(edits.begin(), edits.end(), std::string("কখগঘঙ")), edits.end());
    EXPECT_NE(std::find(edits.begin(), edits.end(), std::string("কখগঘঙচ")), edits.end());
}

TEST_F(SpellCheckerCoverageTest, GenerateEditsReplacementsMatchAsciiCase) {
    suborno::dictionary::Dictionary dictionary(db_path_);
    const suborno::spell::SpellChecker checker(dictionary);

    const auto lower = checker.generate_edits("ab");
    EXPECT_NE(std::find(lower.begin(), lower.end(), std::string("cb")), lower.end());
    EXPECT_EQ(std::find(lower.begin(), lower.end(), std::string("Cb")), lower.end());

    const auto upper = checker.generate_edits("AB");
    EXPECT_NE(std::find(upper.begin(), upper.end(), std::string("AC")), upper.end());
    EXPECT_EQ(std::find(upper.begin(), upper.end(), std::string("Ac")), upper.end());
}
