#include <filesystem>
#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <vector>

#include "suborno/dictionary/dictionary.hpp"

namespace {

std::string temporary_database_path(const char* filename) {
    return (std::filesystem::temp_directory_path() / filename).string();
}

}  // namespace

TEST(PersonalDictionaryTest, AddAndListUserWords) {
    const std::string db_path = temporary_database_path("suborno_personal_add_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));

    suborno::dictionary::Dictionary dictionary(db_path);

    dictionary.add_user_word("সুইডেন", "sweden");
    dictionary.add_user_word("নরওয়ে", "norway");

    const auto user_words = dictionary.list_user_words();
    ASSERT_EQ(user_words.size(), 2U);

    static_cast<void>(std::remove(db_path.c_str()));
}

TEST(PersonalDictionaryTest, GetUserFrequency) {
    const std::string db_path = temporary_database_path("suborno_personal_freq_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));

    suborno::dictionary::Dictionary dictionary(db_path);

    EXPECT_EQ(dictionary.get_user_frequency("সুইডেন"), 0);

    dictionary.add_user_word("সুইডেন", "sweden");
    EXPECT_EQ(dictionary.get_user_frequency("সুইডেন"), 1);

    dictionary.add_user_word("সুইডেন", "sweden");
    EXPECT_EQ(dictionary.get_user_frequency("সুইডেন"), 2);

    static_cast<void>(std::remove(db_path.c_str()));
}

TEST(PersonalDictionaryTest, RemoveUserWord) {
    const std::string db_path = temporary_database_path("suborno_personal_remove_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));

    suborno::dictionary::Dictionary dictionary(db_path);

    dictionary.add_user_word("সুইডেন", "sweden");
    EXPECT_EQ(dictionary.get_user_frequency("সুইডেন"), 1);

    dictionary.remove_user_word("সুইডেন");
    EXPECT_EQ(dictionary.get_user_frequency("সুইডেন"), 0);

    const auto user_words = dictionary.list_user_words();
    EXPECT_TRUE(user_words.empty());

    static_cast<void>(std::remove(db_path.c_str()));
}

TEST(PersonalDictionaryTest, UserWordsRankedInFindMatches) {
    const std::string db_path = temporary_database_path("suborno_personal_rank_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));

    suborno::dictionary::Dictionary dictionary(db_path);

    dictionary.add_entry("বাংলা", "bangla", 5);
    dictionary.add_user_word("বাংলাদেশ", "bangladesh");

    const auto matches = dictionary.find_matches("bangla");
    ASSERT_FALSE(matches.empty());

    static_cast<void>(std::remove(db_path.c_str()));
}

TEST(PersonalDictionaryTest, WordsForPrefixMatchesBanglaWordsLiterally) {
    const std::string db_path = temporary_database_path("suborno_personal_prefix_word.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("বাংলা", "bangla", 3);
    dictionary.add_entry("বাংলাদেশ", "bangladesh", 2);
    dictionary.add_entry("বাঙালি", "bangali", 2);
    dictionary.add_entry("কলকাতা", "kolkata", 1);

    EXPECT_EQ(dictionary.words_for_prefix("বাংলা"),
              (std::vector<std::string>{"বাংলা", "বাংলাদেশ"}));
    EXPECT_EQ(dictionary.words_for_prefix("বাঙা"), std::vector<std::string>{"বাঙালি"});
    EXPECT_EQ(dictionary.words_for_prefix("কল"), std::vector<std::string>{"কলকাতা"});
    EXPECT_TRUE(dictionary.words_for_prefix("জাপান").empty());

    static_cast<void>(std::remove(db_path.c_str()));
}

TEST(PersonalDictionaryTest, LimitsFindMatchesAtTheSqlQueryBoundary) {
    const std::string db_path = temporary_database_path("suborno_personal_query_limit_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("বাংলা", "shared", 9);
    dictionary.add_entry("বাংলাদেশ", "shared", 7);
    dictionary.add_entry("বাংলাদেশি", "shared", 5);

    const auto limited = dictionary.find_matches("shared", 2);
    ASSERT_EQ(limited.size(), 2U);
    EXPECT_EQ(limited[0], "বাংলা");
    EXPECT_EQ(limited[1], "বাংলাদেশ");
    EXPECT_TRUE(dictionary.find_matches("shared", 0).empty());

    static_cast<void>(std::remove(db_path.c_str()));
}

TEST(PersonalDictionaryTest, PinningAndRemovingSystemWordPreservesSystemDictionaryEntry) {
    const std::string db_path = temporary_database_path("suborno_personal_system_pin_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("বাংলা", "bangla", 10);

    dictionary.add_user_word("বাংলা", "bangla");
    ASSERT_EQ(dictionary.list_user_words(), std::vector<std::string>{"বাংলা"});
    EXPECT_EQ(dictionary.find_matches("bangla").front(), "বাংলা");

    dictionary.remove_user_word("বাংলা");
    EXPECT_TRUE(dictionary.list_user_words().empty());
    EXPECT_EQ(dictionary.find_matches("bangla").front(), "বাংলা");
    EXPECT_EQ(dictionary.get_user_frequency("বাংলা"), 0);

    static_cast<void>(std::remove(db_path.c_str()));
}

TEST(PersonalDictionaryTest, ClearingUserEntriesUnpinsSystemWordsAndDeletesUserOnlyWords) {
    const std::string db_path = temporary_database_path("suborno_personal_clear_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("বাংলা", "bangla", 10);
    dictionary.add_user_word("বাংলা", "bangla");
    dictionary.add_user_word("নিজস্ব", "nijoshsho");
    dictionary.record_commit("বাংলা", "bangla");

    ASSERT_EQ(dictionary.list_user_words().size(), 2U);
    EXPECT_EQ(dictionary.get_user_frequency("বাংলা"), 2);

    dictionary.clear_user_entries();

    EXPECT_TRUE(dictionary.list_user_words().empty());
    EXPECT_EQ(dictionary.get_user_frequency("বাংলা"), 0);
    EXPECT_EQ(dictionary.find_matches("bangla").front(), "বাংলা");
    EXPECT_TRUE(dictionary.find_matches("nijoshsho").empty());

    static_cast<void>(std::remove(db_path.c_str()));
}

TEST(PersonalDictionaryTest, FindsCandidatesByNormalizedPhoneticPrefix) {
    const std::string db_path =
        temporary_database_path("suborno_dictionary_normalized_prefix_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("বাংলা", "bangla", 10);
    dictionary.add_entry("বাংলাদেশ", "bangladesh", 8);
    dictionary.add_entry("বাংলাদেশি", "bangladeshi", 6);
    dictionary.add_entry("ভারত", "bharat", 4);

    const auto matches = dictionary.find_prefix_matches("bang", 2);
    ASSERT_EQ(matches.size(), 2U);
    EXPECT_EQ(matches[0], "বাংলা");
    EXPECT_EQ(matches[1], "বাংলাদেশ");
    EXPECT_TRUE(dictionary.find_prefix_matches("bang", 0).empty());
    EXPECT_TRUE(dictionary.find_prefix_matches("", 10).empty());

    static_cast<void>(std::remove(db_path.c_str()));
}

TEST(PersonalDictionaryTest, EscapesSqlWildcardsInNormalizedPrefixSearch) {
    const std::string db_path =
        temporary_database_path("suborno_dictionary_normalized_prefix_wildcard_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));

    suborno::dictionary::Dictionary dictionary(db_path);
    // Bangla words whose normalized keys contain SQL wildcard characters —

    // the prefix search must escape them instead of treating them as patterns.

    dictionary.add_entry("শতকরা", "p%word", 2);
    dictionary.add_entry("নিম্নরেখা", "p_word", 2);
    dictionary.add_entry("ব্যাকস্ল্যাশ", "p\\word", 2);
    dictionary.add_entry("অন্যান্য", "pXword", 1);

    EXPECT_EQ(dictionary.find_prefix_matches("p%", 10), std::vector<std::string>{"শতকরা"});
    EXPECT_EQ(dictionary.find_prefix_matches("p_", 10), std::vector<std::string>{"নিম্নরেখা"});
    EXPECT_EQ(dictionary.find_prefix_matches("p\\", 10), std::vector<std::string>{"ব্যাকস্ল্যাশ"});

    static_cast<void>(std::remove(db_path.c_str()));
}

TEST(PersonalDictionaryTest, RecordsNewWordsAsLearnedPersonalEntries) {
    const std::string db_path =
        temporary_database_path("suborno_personal_learned_word_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.record_commit("নিজস্বশব্দ", "nijoshsho");

    EXPECT_EQ(dictionary.get_user_frequency("নিজস্বশব্দ"), 1);
    EXPECT_EQ(dictionary.list_user_words(), std::vector<std::string>{"নিজস্বশব্দ"});

    dictionary.record_commit("নিজস্বশব্দ", "nijoshsho");
    EXPECT_EQ(dictionary.get_user_frequency("নিজস্বশব্দ"), 2);

    static_cast<void>(std::remove(db_path.c_str()));
}

TEST(PersonalDictionaryTest, StoresUserInputAsAliasWithoutOverwritingCanonicalNormalization) {
    const std::string db_path =
        temporary_database_path("suborno_dictionary_alias_preserves_canonical_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("বাংলা", "bangla", 10);
    dictionary.record_commit("বাংলা", "bangla", "bang");

    EXPECT_EQ(dictionary.find_matches("bangla"), std::vector<std::string>{"বাংলা"});
    EXPECT_EQ(dictionary.find_matches("bang"), std::vector<std::string>{"বাংলা"});
    EXPECT_EQ(dictionary.find_prefix_matches("ban"), std::vector<std::string>{"বাংলা"});
    EXPECT_EQ(dictionary.find_prefix_matches("bang"), std::vector<std::string>{"বাংলা"});
    EXPECT_EQ(dictionary.get_user_frequency("বাংলা"), 1);
    EXPECT_EQ(dictionary.list_user_words(), std::vector<std::string>{"বাংলা"});

    dictionary.record_commit("বাংলা", "bangla", "bngla");
    EXPECT_EQ(dictionary.find_matches("bangla"), std::vector<std::string>{"বাংলা"});
    EXPECT_EQ(dictionary.find_matches("bang"), std::vector<std::string>{"বাংলা"});
    EXPECT_EQ(dictionary.find_matches("bngla"), std::vector<std::string>{"বাংলা"});
    EXPECT_EQ(dictionary.get_user_frequency("বাংলা"), 2);

    dictionary.remove_user_word("বাংলা");
    EXPECT_TRUE(dictionary.find_matches("bang").empty());
    EXPECT_TRUE(dictionary.find_matches("bngla").empty());
    EXPECT_EQ(dictionary.find_matches("bangla"), std::vector<std::string>{"বাংলা"});
    EXPECT_TRUE(dictionary.list_user_words().empty());

    static_cast<void>(std::remove(db_path.c_str()));
}

TEST(PersonalDictionaryTest, LearnedSystemWordsAppearInPersonalListAndCanBeRemoved) {
    const std::string db_path =
        temporary_database_path("suborno_learned_system_word_lifecycle_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("বাংলা", "bangla", 10);
    dictionary.record_commit("বাংলা", "bangla");

    EXPECT_EQ(dictionary.get_user_frequency("বাংলা"), 1);
    EXPECT_EQ(dictionary.list_user_words(), std::vector<std::string>{"বাংলা"});

    dictionary.remove_user_word("বাংলা");

    EXPECT_EQ(dictionary.get_user_frequency("বাংলা"), 0);
    EXPECT_TRUE(dictionary.list_user_words().empty());
    EXPECT_EQ(dictionary.find_matches("bangla"), std::vector<std::string>{"বাংলা"});

    static_cast<void>(std::remove(db_path.c_str()));
}

TEST(PersonalDictionaryTest, LearnedFrequencyChangesDictionaryRanking) {
    const std::string db_path =
        temporary_database_path("suborno_personal_learned_rank_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("আগেরশব্দ", "shared", 5);
    dictionary.add_entry("শেখাশব্দ", "shared", 1);

    ASSERT_EQ(dictionary.find_matches("shared").front(), "আগেরশব্দ");
    for (int count = 0; count < 5; ++count) {
        dictionary.record_commit("শেখাশব্দ", "shared");
    }

    const auto matches = dictionary.find_matches("shared");
    ASSERT_EQ(matches.size(), 2U);
    EXPECT_EQ(matches.front(), "শেখাশব্দ");
    EXPECT_EQ(dictionary.get_user_frequency("শেখাশব্দ"), 5);

    static_cast<void>(std::remove(db_path.c_str()));
}

TEST(PersonalDictionaryTest, ConcurrentSessionsCanCommitThroughASharedDictionary) {
    // Per-window sessions share one Dictionary (one SQLite connection). The

    // write mutex must keep the BEGIN IMMEDIATE...COMMIT pairs from two

    // threads interleaving, which used to fail the second thread's commit.

    const std::string db_path = temporary_database_path("suborno_personal_concurrent_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));

    suborno::dictionary::Dictionary dictionary(db_path);

    // Valid digit-free Bangla words: 10 prefixes x 10 suffixes = 100 unique

    // learned words.

    static const char* prefixes[] = {
        "শব্দ", "কথা", "গল্প", "ছবি", "সুর", "ফুল", "নদী", "পাখি", "স্বপ্ন", "পথ"};
    static const char* suffixes[] = {
        "মালা", "বীনা", "পত্র", "সমূহ", "বাহক", "গুচ্ছ", "কান্ত", "জাল", "মালিনী", "রঙিণ"};
    constexpr int commit_thread_count = 4;
    constexpr int commits_per_thread = 50;
    std::vector<std::thread> worker_threads;
    worker_threads.reserve(commit_thread_count);
    for (int thread_index = 0; thread_index < commit_thread_count; ++thread_index) {
        worker_threads.emplace_back([&dictionary, thread_index] {
            for (int commit_index = 0; commit_index < commits_per_thread; ++commit_index) {
                const std::string word = std::string(prefixes[commit_index % 10])
                                         + suffixes[(commit_index / 10) % 10]
                                         + prefixes[thread_index % 10];
                dictionary.record_commit(word, word);
                const std::string extra =
                    std::string(suffixes[commit_index % 10]) + prefixes[(commit_index / 10) % 10];
                dictionary.add_entry(extra, extra, commit_index);
            }
        });
    }
    for (std::thread& worker_thread : worker_threads) {
        worker_thread.join();
    }

    for (int thread_index = 0; thread_index < commit_thread_count; ++thread_index) {
        for (int commit_index = 0; commit_index < commits_per_thread; ++commit_index) {
            const std::string word = std::string(prefixes[commit_index % 10])
                                     + suffixes[(commit_index / 10) % 10]
                                     + prefixes[thread_index % 10];
            EXPECT_EQ(dictionary.get_user_frequency(word), 1) << word;
        }
    }

    static_cast<void>(std::remove(db_path.c_str()));
}

TEST(DictionaryV4Test, SchemaVersionIsFourWithManagementColumns) {
    const std::string db_path = temporary_database_path("suborno_v4_schema_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));

    suborno::dictionary::Dictionary dictionary(db_path);
    EXPECT_EQ(dictionary.schema_version(), 4);

    const auto records = dictionary.all_entries();
    EXPECT_TRUE(records.empty());

    static_cast<void>(std::remove(db_path.c_str()));
}

TEST(DictionaryV4Test, CategoriesAndRecordsRoundTrip) {
    const std::string db_path = temporary_database_path("suborno_v4_category_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("স্বপ্ন", "swapon", 900, "conjunct", "cholito", "seed");
    dictionary.add_entry("গৃহ", "griho", 300, "synonym", "sadhu", "seed");

    const auto records = dictionary.all_entries();
    ASSERT_EQ(records.size(), 2U);
    EXPECT_EQ(records.front().word, "গৃহ");
    EXPECT_EQ(records.front().category, "synonym");
    EXPECT_EQ(records.front().word_register, "sadhu");
    EXPECT_EQ(records.front().origin, "seed");

    EXPECT_EQ(dictionary.words_by_category("conjunct"), std::vector<std::string>{"স্বপ্ন"});

    dictionary.set_word_category("স্বপ্ন", "literary");
    EXPECT_EQ(dictionary.words_by_category("conjunct").empty(), true);
    EXPECT_EQ(dictionary.words_by_category("literary"), std::vector<std::string>{"স্বপ্ন"});
    EXPECT_THROW(dictionary.set_word_category("অজানাশব্দ", "noun"), std::exception);

    const suborno::dictionary::DictionaryStats stats = dictionary.stats();
    EXPECT_EQ(stats.total_entries, 2U);
    EXPECT_EQ(stats.system_entries, 2U);

    static_cast<void>(std::remove(db_path.c_str()));
}

TEST(DictionaryV4Test, MergeUpsertKeepsHigherFrequencyAndFillsEmptyMetadata) {
    const std::string db_path = temporary_database_path("suborno_v4_merge_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("শব্দকল্প", "shobdokolpo", 700, "literary", "cholito", "seed");

    // Incremental update: lower frequency must not downgrade the entry, and

    // empty metadata fills while existing metadata is preserved.

    dictionary.add_entry_merge("শব্দকল্প", "shobdokolpo", 400, "", "", "corpus");
    const auto records = dictionary.all_entries();
    ASSERT_EQ(records.size(), 1U);
    EXPECT_EQ(records.front().frequency, 700);
    EXPECT_EQ(records.front().category, "literary");
    EXPECT_EQ(records.front().origin, "seed");

    // A higher frequency wins; category stays because it is non-empty.

    dictionary.add_entry_merge("শব্দকল্প", "shobdokolpo", 1200, "noun", "", "corpus");
    const auto updated = dictionary.all_entries();
    ASSERT_EQ(updated.size(), 1U);
    EXPECT_EQ(updated.front().frequency, 1200);
    EXPECT_EQ(updated.front().category, "literary");
    EXPECT_EQ(updated.front().origin, "seed");

    static_cast<void>(std::remove(db_path.c_str()));
}

TEST(DictionaryV4Test, RelationsAndBackupRoundTrip) {
    const std::string db_path = temporary_database_path("suborno_v4_relations_test.sqlite");
    const std::string backup_path = temporary_database_path("suborno_v4_backup_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));
    static_cast<void>(std::remove(backup_path.c_str()));

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("শান্তি", "shanti", 800, "noun", "cholito", "seed");
    dictionary.add_word_relation("শান্তি", "প্রশান্তি", "synonym");
    dictionary.add_word_relation("শান্তি", "অশান্তি", "antonym");

    EXPECT_EQ(dictionary.related_words("শান্তি"), (std::vector<std::string>{"প্রশান্তি", "অশান্তি"}));
    EXPECT_EQ(dictionary.related_words("শান্তি", "synonym"), std::vector<std::string>{"প্রশান্তি"});
    EXPECT_EQ(dictionary.related_words("শান্তি", "variant").empty(), true);
    EXPECT_THROW(dictionary.add_word_relation("শান্তি", "অন্য", "unknown"), std::exception);

    const suborno::dictionary::DictionaryStats stats = dictionary.stats();
    EXPECT_EQ(stats.word_relations, 2U);
    EXPECT_GE(stats.changelog_entries, 2U);

    dictionary.backup_to(backup_path);
    suborno::dictionary::Dictionary restored(backup_path);
    EXPECT_EQ(restored.schema_version(), 4);
    EXPECT_EQ(restored.find_matches("shanti"), std::vector<std::string>{"শান্তি"});
    EXPECT_EQ(restored.related_words("শান্তি"), (std::vector<std::string>{"প্রশান্তি", "অশান্তি"}));

    static_cast<void>(std::remove(db_path.c_str()));
    static_cast<void>(std::remove(backup_path.c_str()));
}

TEST(DictionaryV4Test, WordValidationRejectsNonBanglaEntries) {
    EXPECT_TRUE(suborno::dictionary::Dictionary::is_valid_word("স্বপ্ন"));
    EXPECT_TRUE(suborno::dictionary::Dictionary::is_valid_word("আমি"));
    EXPECT_FALSE(suborno::dictionary::Dictionary::is_valid_word(""));
    EXPECT_FALSE(suborno::dictionary::Dictionary::is_valid_word("ami"));
    EXPECT_FALSE(suborno::dictionary::Dictionary::is_valid_word("আমি1"));
    EXPECT_FALSE(suborno::dictionary::Dictionary::is_valid_word("আমি তুমি"));
    // A multi-character literal ('\xe0\xa6\x86') is an int whose value is

    // truncated per copy (C4305/C4309); 50 copies of the final byte encode the

    // same invalid-UTF-8 filler the original literal produced on both MSVC and

    // GCC.

    EXPECT_FALSE(suborno::dictionary::Dictionary::is_valid_word(std::string(50, '\x86')));
    EXPECT_TRUE(suborno::dictionary::Dictionary::is_valid_normalized("ami bhai"));
    EXPECT_FALSE(suborno::dictionary::Dictionary::is_valid_normalized(""));
}

TEST(DictionaryV4Test, FindDuplicateNormalizedReportsSharedKeys) {
    const std::string db_path = temporary_database_path("suborno_v4_dupes_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("ভাল", "bhalo", 900, "adjective", "cholito", "seed");
    dictionary.add_entry("ভালো", "bhalo", 1400, "adjective", "cholito", "seed");
    dictionary.add_entry("কথা", "kotha", 1200, "noun", "cholito", "seed");

    const auto groups = dictionary.find_duplicate_normalized();
    ASSERT_EQ(groups.size(), 1U);
    EXPECT_EQ(groups.front().normalized, "bhalo");
    EXPECT_EQ(groups.front().words, (std::vector<std::string>{"ভালো", "ভাল"}));

    static_cast<void>(std::remove(db_path.c_str()));
}

TEST(DictionaryV4Test, CacheIsInvalidatedByWrites) {
    const std::string db_path = temporary_database_path("suborno_v4_cache_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));

    suborno::dictionary::Dictionary dictionary(db_path);
    EXPECT_TRUE(dictionary.find_matches("swapon").empty());

    dictionary.add_entry("স্বপ্ন", "swapon", 900, "conjunct", "cholito", "seed");
    EXPECT_EQ(dictionary.find_matches("swapon"), std::vector<std::string>{"স্বপ্ন"});

    dictionary.record_commit("স্বপ্ন", "swapon");
    EXPECT_EQ(dictionary.get_user_frequency("স্বপ্ন"), 1);
    EXPECT_EQ(dictionary.find_matches("swapon"), std::vector<std::string>{"স্বপ্ন"});

    // Repeat the lookup: served from the LRU cache, identical results.

    EXPECT_EQ(dictionary.find_matches("swapon"), std::vector<std::string>{"স্বপ্ন"});

    static_cast<void>(std::remove(db_path.c_str()));
}

TEST(DictionaryTransitionsTest, RecordTransitionUpsertsAndIncrementsFrequency) {
    const std::string db_path = temporary_database_path("suborno_transitions_upsert_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.record_transition("ami", "ভালো");
    dictionary.record_transition("ami", "ভালো");
    dictionary.record_transition("ami", "ভালো");
    dictionary.record_transition("ami", "ধন্যবাদ");

    // The repeated pair must stay a single row whose frequency grew, so the

    // ordering is frequency-driven (3 commits) over the single one.

    const auto followers = dictionary.transition_candidates("ami", "", 10);
    ASSERT_EQ(followers.size(), 2U);
    EXPECT_EQ(followers[0], "ভালো");
    EXPECT_EQ(followers[1], "ধন্যবাদ");

    // Empty inputs are ignored: they must not poison the bigram table.

    dictionary.record_transition("", "ভালো");
    dictionary.record_transition("ami", "");
    EXPECT_EQ(dictionary.transition_candidates("ami", "", 10).size(), 2U);

    static_cast<void>(std::remove(db_path.c_str()));
}

TEST(DictionaryTransitionsTest, TransitionCandidatesOrderFilterAndLimit) {
    const std::string db_path = temporary_database_path("suborno_transitions_query_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));

    suborno::dictionary::Dictionary dictionary(db_path);
    for (int count = 0; count < 5; ++count) {
        dictionary.record_transition("ami", "ভালো");
    }
    for (int count = 0; count < 3; ++count) {
        dictionary.record_transition("ami", "ভাল");
    }
    dictionary.record_transition("ami", "ধন্যবাদ");
    // A different previous word must never leak into "ami" predictions.

    dictionary.record_transition("tumi", "ভালো");

    // Frequency DESC ordering (5 > 3 > 1).

    EXPECT_EQ(dictionary.transition_candidates("ami", "", 10),
              (std::vector<std::string>{"ভালো", "ভাল", "ধন্যবাদ"}));

    // Prefix filter keeps only the ভা- words.

    EXPECT_EQ(dictionary.transition_candidates("ami", "ভা", 10),
              (std::vector<std::string>{"ভালো", "ভাল"}));

    // Limit caps the result.

    EXPECT_EQ(dictionary.transition_candidates("ami", "", 2),
              (std::vector<std::string>{"ভালো", "ভাল"}));

    // Degenerate queries return empty instead of throwing. The separate

    // previous word keeps its own follower (no cross-word leakage).

    EXPECT_EQ(dictionary.transition_candidates("tumi", "", 10), std::vector<std::string>{"ভালো"});
    EXPECT_TRUE(dictionary.transition_candidates("", "", 10).empty());
    EXPECT_TRUE(dictionary.transition_candidates("ami", "", 0).empty());

    static_cast<void>(std::remove(db_path.c_str()));
}

TEST(DictionaryTransitionsTest, ClearUserEntriesAlsoClearsTransitions) {
    const std::string db_path = temporary_database_path("suborno_transitions_clear_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("আমি", "ami", 10);
    dictionary.record_commit("আমি", "ami");
    dictionary.record_transition("ami", "ভালো");
    dictionary.add_user_word("নিজস্ব", "nijoshsho");

    ASSERT_EQ(dictionary.stats().word_transitions, 1U);
    ASSERT_FALSE(dictionary.transition_candidates("ami", "", 10).empty());

    dictionary.clear_user_entries();

    EXPECT_EQ(dictionary.stats().word_transitions, 0U);
    EXPECT_TRUE(dictionary.transition_candidates("ami", "", 10).empty());
    EXPECT_TRUE(dictionary.list_user_words().empty());
    // System entries survive the clear, as before.

    EXPECT_EQ(dictionary.find_matches("ami"), std::vector<std::string>{"আমি"});

    static_cast<void>(std::remove(db_path.c_str()));
}

TEST(DictionaryTransitionsTest, TransitionsSurviveReopenAndKeepSchemaVersionFour) {
    const std::string db_path = temporary_database_path("suborno_transitions_reopen_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));
    {
        suborno::dictionary::Dictionary dictionary(db_path);
        dictionary.record_transition("ami", "ভালো");
        dictionary.record_transition("ami", "ভালো");
        dictionary.record_transition("ami", "ধন্যবাদ");
    }

    // Reopening the same file must migrate transparently (IF NOT EXISTS is a

    // no-op on the existing table) without bumping the schema version.

    suborno::dictionary::Dictionary reopened(db_path);
    EXPECT_EQ(reopened.schema_version(), 4);
    EXPECT_EQ(reopened.stats().word_transitions, 2U);
    EXPECT_EQ(reopened.transition_candidates("ami", "", 10),
              (std::vector<std::string>{"ভালো", "ধন্যবাদ"}));

    static_cast<void>(std::remove(db_path.c_str()));
}
