// Coverage tests for src/layout-api/declarative_layout.cpp and
// src/layout-api/layout_registry.cpp. Each test names the measured gap
// (uncovered line or branch) it closes. The registry tests reuse the real
// SubornoClassicLayout exactly the way engine.cpp registers it, so no
// InputLayout test double is needed.

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>

#include "suborno/layout-api/declarative_layout.hpp"
#include "suborno/layout-api/layout_registry.hpp"
#include "suborno/layouts/phonetic/suborno_classic_layout.hpp"

namespace {

// Writes a declarative mapping table (one key<TAB>value per line) to a
// temporary file and returns its path. Callers remove the file afterwards.
std::filesystem::path write_mapping_file(const std::string& contents) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "suborno_declarative_layout_coverage.tsv";
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output << contents;
    return path;
}

void remove_quietly(const std::filesystem::path& path) {
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
}

std::unique_ptr<suborno::layouts::SubornoClassicLayout> make_classic_layout() {
    return std::make_unique<suborno::layouts::SubornoClassicLayout>();
}

}  // namespace

// Covers declarative_layout.cpp:17-19 — load() on a missing mapping file.
TEST(LayoutApiCoverage, LoadReturnsFalseForAMissingMappingFile) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "suborno_declarative_missing_mapping.tsv";
    remove_quietly(path);

    suborno::layout::DeclarativeLayout layout("decl-missing", path);
    EXPECT_FALSE(layout.load());
    remove_quietly(path);
}

// Covers declarative_layout.cpp:46-47 — the id() accessor returns the
// configured identifier without requiring a successful load.
TEST(LayoutApiCoverage, IdReturnsTheConfiguredIdentifier) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "suborno_declarative_id_probe.tsv";
    remove_quietly(path);

    const suborno::layout::DeclarativeLayout layout("decl-id-probe", path);
    EXPECT_EQ(layout.id(), "decl-id-probe");
    remove_quietly(path);
}

// Covers declarative_layout.cpp:24-43,57-75,79 — a valid table loads: comment
// and blank lines are skipped, tab separated pairs are stored, and load()
// returns true only when the stream is clean.
TEST(LayoutApiCoverage, LoadParsesCommentsBlankLinesAndTabSeparatedMappings) {
    const std::filesystem::path path = write_mapping_file(
        "# declarative coverage table\n"
        "\n"
        "k\tক\n"
        "\n"
        "kh\tখ\n");

    suborno::layout::DeclarativeLayout layout("decl-coverage", path);
    EXPECT_TRUE(layout.load());
    EXPECT_EQ(layout.transliterate("kh"), "খ");
    EXPECT_EQ(layout.transliterate("k"), "ক");
    remove_quietly(path);
}

// Covers declarative_layout.cpp:57-58 — the longest matching key wins over its
// shorter prefix, at any offset of the input.
TEST(LayoutApiCoverage, TransliteratePrefersTheLongestMatchingKey) {
    const std::filesystem::path path = write_mapping_file("k\tক\nkh\tখ\n");

    suborno::layout::DeclarativeLayout layout("decl-longest", path);
    ASSERT_TRUE(layout.load());
    EXPECT_EQ(layout.transliterate("kh"), "খ");
    EXPECT_EQ(layout.transliterate("k"), "ক");
    EXPECT_EQ(layout.transliterate("kkh"), "কখ");
    EXPECT_EQ(layout.transliterate("hkh"), "hখ");
    remove_quietly(path);
}

// Covers declarative_layout.cpp:63-73 — unmapped bytes pass through with the
// UTF-8 lead length (1, 2, 3 or 4 bytes), so multibyte characters stay intact.
TEST(LayoutApiCoverage, TransliteratePassesThroughUnmappedAsciiAndMultiByteUtf8) {
    const std::filesystem::path path = write_mapping_file("k\tক\n");

    suborno::layout::DeclarativeLayout layout("decl-passthrough", path);
    ASSERT_TRUE(layout.load());
    EXPECT_EQ(layout.transliterate("Z"), "Z");  // 1-byte lead.
    EXPECT_EQ(layout.transliterate("\xC3\xA9"), "\xC3\xA9");  // é: 2-byte lead.
    EXPECT_EQ(layout.transliterate("ক"), "ক");  // unmapped: 3-byte lead.
    EXPECT_EQ(layout.transliterate("\xF0\x9F\x98\x80"), "\xF0\x9F\x98\x80");  // emoji: 4-byte lead.
    EXPECT_EQ(layout.transliterate("a\xC3\xA9ক\xF0\x9F\x98\x80k"), "a\xC3\xA9ক\xF0\x9F\x98\x80ক");
    remove_quietly(path);
}

// Covers declarative_layout.cpp:79 — the output is NFC-normalized after
// mapping, so tables may store decomposed two-part matras (ে + া) and still
// print ো; decomposed matras in passthrough output compose too.
TEST(LayoutApiCoverage, TransliterateNormalizesMappedOutputToNfc) {
    const std::filesystem::path path = write_mapping_file(
        "o\t\u09C7\u09BE\n"
        "ko\t\u0995\u09C7\u09BE\n");

    suborno::layout::DeclarativeLayout layout("decl-nfc", path);
    ASSERT_TRUE(layout.load());
    EXPECT_EQ(layout.transliterate("o"), "ো");
    EXPECT_EQ(layout.transliterate("ko"), "কো");
    EXPECT_EQ(layout.transliterate("z\u09C7\u09BE"), "zো");
    remove_quietly(path);
}

// Covers declarative_layout.cpp:53,62 — before load() (empty mapping) and for
// empty input, transliterate is pure passthrough plus NFC normalization.
TEST(LayoutApiCoverage, TransliteratePassesThroughBeforeLoadAndForEmptyInput) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "suborno_declarative_never_loaded.tsv";
    remove_quietly(path);

    suborno::layout::DeclarativeLayout layout("decl-unloaded", path);
    ASSERT_FALSE(layout.load());
    EXPECT_EQ(layout.transliterate(""), "");
    EXPECT_EQ(layout.transliterate("ami"), "ami");
    EXPECT_EQ(layout.transliterate("আমি"), "আমি");
    remove_quietly(path);
}

// Covers declarative_layout.cpp:65,67,69 — non-UTF-8 lead bytes fall back to a
// single byte (none of the 2/3/4-byte lead tests match) and the final NFC pass
// then rejects the invalid output with std::invalid_argument.
TEST(LayoutApiCoverage, TransliterateSingleByteFallbackFeedsNfcWhichRejectsInvalidUtf8) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "suborno_declarative_invalid_passthrough.tsv";
    remove_quietly(path);

    suborno::layout::DeclarativeLayout layout("decl-invalid", path);
    ASSERT_FALSE(layout.load());
    EXPECT_THROW(layout.transliterate("\xFF"), std::invalid_argument);
    EXPECT_THROW(layout.transliterate("\x80"), std::invalid_argument);
    remove_quietly(path);
}

// Covers declarative_layout.cpp:29-30 — a line without a tab separator aborts
// the load and leaves the mapping from before the failed load untouched.
TEST(LayoutApiCoverage, LoadRejectsLinesWithoutTabSeparator) {
    const std::filesystem::path path = write_mapping_file("k\tক\nbroken-line\n");

    suborno::layout::DeclarativeLayout layout("decl-broken", path);
    EXPECT_FALSE(layout.load());
    EXPECT_EQ(layout.transliterate("k"), "k");
    remove_quietly(path);
}

// Covers declarative_layout.cpp:29 — an empty key (leading tab) and an empty
// value (trailing tab) are both rejected by the same separator guard.
TEST(LayoutApiCoverage, LoadRejectsEmptyKeysAndEmptyValues) {
    const std::filesystem::path empty_key = write_mapping_file("\tক\n");
    suborno::layout::DeclarativeLayout empty_key_layout("decl-empty-key", empty_key);
    EXPECT_FALSE(empty_key_layout.load());
    remove_quietly(empty_key);

    const std::filesystem::path empty_value = write_mapping_file("k\t\n");
    suborno::layout::DeclarativeLayout empty_value_layout("decl-empty-value", empty_value);
    EXPECT_FALSE(empty_value_layout.load());
    remove_quietly(empty_value);
}

// Covers declarative_layout.cpp:34-36 — invalid UTF-8 in a key or in a value
// rejects the whole table.
TEST(LayoutApiCoverage, LoadRejectsInvalidUtf8KeysAndValues) {
    const std::filesystem::path bad_key = write_mapping_file("\xFF\tক\n");
    suborno::layout::DeclarativeLayout bad_key_layout("decl-bad-key", bad_key);
    EXPECT_FALSE(bad_key_layout.load());
    remove_quietly(bad_key);

    const std::filesystem::path bad_value = write_mapping_file("k\t\xFF\n");
    suborno::layout::DeclarativeLayout bad_value_layout("decl-bad-value", bad_value);
    EXPECT_FALSE(bad_value_layout.load());
    remove_quietly(bad_value);
}

// Covers declarative_layout.cpp:35-36 — a duplicate key aborts the load via
// the failed emplace branch.
TEST(LayoutApiCoverage, LoadRejectsDuplicateKeys) {
    const std::filesystem::path path = write_mapping_file("k\tv1\nk\tv2\n");

    suborno::layout::DeclarativeLayout layout("decl-duplicate", path);
    EXPECT_FALSE(layout.load());
    remove_quietly(path);
}

// Covers declarative_layout.cpp:39-40 — a mapping path that cannot be read
// fails load(): on POSIX the directory opens and the stream goes bad, on
// Windows it never opens (declarative_layout.cpp:19); both return false.
TEST(LayoutApiCoverage, LoadFailsWhenTheMappingPathIsADirectory) {
    const std::filesystem::path directory =
        std::filesystem::temp_directory_path() / "suborno_declarative_dir_coverage";
    std::error_code ignored;
    std::filesystem::create_directories(directory, ignored);

    suborno::layout::DeclarativeLayout layout("decl-directory", directory);
    EXPECT_FALSE(layout.load());

    std::filesystem::remove_all(directory, ignored);
}

// Covers declarative_layout.cpp:83-90 — generate_candidates mirrors the
// transliteration in a single candidate carrying the layout identity.
TEST(LayoutApiCoverage, GenerateCandidatesMirrorTransliterationWithLayoutIdentity) {
    const std::filesystem::path path = write_mapping_file("k\tক\nkh\tখ\n");

    suborno::layout::DeclarativeLayout layout("decl-coverage", path);
    ASSERT_TRUE(layout.load());

    const auto candidates = layout.generate_candidates("kh");
    ASSERT_EQ(candidates.size(), 1U);
    EXPECT_EQ(candidates.front().text, "খ");
    EXPECT_EQ(candidates.front().normalized_text, "খ");
    EXPECT_EQ(candidates.front().stable_id, "layout:decl-coverage:খ");
    EXPECT_EQ(candidates.front().source, suborno::layout::CandidateSource::layout);
    EXPECT_DOUBLE_EQ(candidates.front().score, 1.0);
    EXPECT_EQ(candidates.front().rationale, "declarative layout");

    const auto empty = layout.generate_candidates("");
    ASSERT_EQ(empty.size(), 1U);
    EXPECT_EQ(empty.front().text, "");
    EXPECT_EQ(empty.front().normalized_text, "");
    EXPECT_EQ(empty.front().stable_id, "layout:decl-coverage:");
    EXPECT_DOUBLE_EQ(empty.front().score, 1.0);
    remove_quietly(path);
}

// Covers layout_registry.cpp:8-9,55-61 — a fresh registry starts with no
// active layout and rejects empty ids as well as null layouts.
TEST(LayoutApiCoverage, EmptyRegistryStartsInactiveAndRejectsEmptyIdsAndNullLayouts) {
    suborno::layout::LayoutRegistry registry;

    EXPECT_TRUE(registry.active_id().empty());
    EXPECT_EQ(registry.active(), nullptr);

    EXPECT_FALSE(registry.register_layout("", make_classic_layout()));
    EXPECT_FALSE(registry.register_layout("suborno-classic", nullptr));

    EXPECT_TRUE(registry.active_id().empty());
    EXPECT_EQ(registry.active(), nullptr);
}

// Covers layout_registry.cpp:15-17 — the first registration becomes active and
// the stored layout is a fully working InputLayout.
TEST(LayoutApiCoverage, FirstRegistrationBecomesTheActiveLayout) {
    suborno::layout::LayoutRegistry registry;

    ASSERT_TRUE(registry.register_layout("suborno-classic", make_classic_layout()));
    EXPECT_EQ(registry.active_id(), "suborno-classic");

    const suborno::layout::InputLayout* active = registry.active();
    ASSERT_NE(active, nullptr);
    EXPECT_EQ(active, registry.find("suborno-classic"));
    EXPECT_EQ(active->transliterate("ka"), "কা");
}

// Covers layout_registry.cpp:11-13 — a duplicate id is rejected and the
// original layout is kept. register_layout takes the unique_ptr by value, so
// the argument is consumed on every call (standard C++ move semantics); the
// meaningful contract is the false return plus the untouched registry state.
TEST(LayoutApiCoverage, DuplicateRegistrationIsRejectedAndKeepsTheOriginal) {
    suborno::layout::LayoutRegistry registry;

    ASSERT_TRUE(registry.register_layout("suborno-classic", make_classic_layout()));
    const suborno::layout::InputLayout* original = registry.find("suborno-classic");
    ASSERT_NE(original, nullptr);

    EXPECT_FALSE(registry.register_layout("suborno-classic", make_classic_layout()));

    EXPECT_EQ(registry.find("suborno-classic"), original);
    EXPECT_EQ(original->transliterate("ka"), "কা");
    EXPECT_EQ(registry.active_id(), "suborno-classic");
}

// Covers layout_registry.cpp:42-46 — set_active switches between registered
// layouts; a later registration never steals the active slot (16-17).
TEST(LayoutApiCoverage, SetActiveSwitchesBetweenRegisteredLayouts) {
    suborno::layout::LayoutRegistry registry;

    ASSERT_TRUE(registry.register_layout("classic-a", make_classic_layout()));
    ASSERT_TRUE(registry.register_layout("classic-b", make_classic_layout()));
    EXPECT_EQ(registry.active_id(), "classic-a");

    EXPECT_TRUE(registry.set_active("classic-b"));
    EXPECT_EQ(registry.active_id(), "classic-b");
    EXPECT_EQ(registry.active(), registry.find("classic-b"));
}

// Covers layout_registry.cpp:43-44 — set_active to an unknown or empty id is
// rejected and leaves the current selection unchanged.
TEST(LayoutApiCoverage, SetActiveUnknownOrEmptyIdIsRejectedAndKeepsSelection) {
    suborno::layout::LayoutRegistry registry;

    EXPECT_FALSE(registry.set_active("ghost"));
    EXPECT_FALSE(registry.set_active(""));
    EXPECT_EQ(registry.active(), nullptr);

    ASSERT_TRUE(registry.register_layout("classic-a", make_classic_layout()));
    EXPECT_FALSE(registry.set_active("ghost"));
    EXPECT_EQ(registry.active_id(), "classic-a");
    EXPECT_EQ(registry.active(), registry.find("classic-a"));
}

// Covers layout_registry.cpp:37-38 — metadata lookup returns the stored entry
// or nullptr for unknown ids and for layouts registered without metadata.
TEST(LayoutApiCoverage, FindMetadataReturnsRegisteredEntryOrNullOrMissing) {
    suborno::layout::LayoutRegistry registry;

    const suborno::layout::LayoutMetadata metadata{"", "Suborno Classic", "2"};
    ASSERT_TRUE(registry.register_layout("suborno-classic", make_classic_layout(), metadata));
    ASSERT_TRUE(registry.register_layout("plain-classic", make_classic_layout()));

    const suborno::layout::LayoutMetadata* found = registry.find_metadata("suborno-classic");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->display_name, "Suborno Classic");
    EXPECT_EQ(found->version, "2");

    EXPECT_EQ(registry.find_metadata("plain-classic"), nullptr);
    EXPECT_EQ(registry.find_metadata("ghost"), nullptr);
}

// Covers layout_registry.cpp:25-27 — an empty metadata id defaults to the
// registry key; an explicit metadata id is preserved verbatim.
TEST(LayoutApiCoverage, MetadataIdDefaultsToTheRegistryKeyAndExplicitIdIsKept) {
    suborno::layout::LayoutRegistry registry;

    const suborno::layout::LayoutMetadata defaulted{"", "Classic", "1"};
    ASSERT_TRUE(registry.register_layout("suborno-classic", make_classic_layout(), defaulted));
    const suborno::layout::LayoutMetadata* defaulted_found =
        registry.find_metadata("suborno-classic");
    ASSERT_NE(defaulted_found, nullptr);
    EXPECT_EQ(defaulted_found->id, "suborno-classic");

    const suborno::layout::LayoutMetadata explicit_id{"custom-id", "Classic", "1"};
    ASSERT_TRUE(registry.register_layout("other-classic", make_classic_layout(), explicit_id));
    const suborno::layout::LayoutMetadata* explicit_found = registry.find_metadata("other-classic");
    ASSERT_NE(explicit_found, nullptr);
    EXPECT_EQ(explicit_found->id, "custom-id");
}

// Covers layout_registry.cpp:29-32 — a failed three-argument registration
// propagates the failure and never overwrites the stored metadata.
TEST(LayoutApiCoverage, FailedMetadataRegistrationKeepsTheFirstMetadata) {
    suborno::layout::LayoutRegistry registry;

    const suborno::layout::LayoutMetadata first{"suborno-classic", "First", "1"};
    ASSERT_TRUE(registry.register_layout("suborno-classic", make_classic_layout(), first));

    const suborno::layout::LayoutMetadata second{"suborno-classic", "Second", "2"};
    EXPECT_FALSE(registry.register_layout("suborno-classic", make_classic_layout(), second));

    const suborno::layout::LayoutMetadata* stored = registry.find_metadata("suborno-classic");
    ASSERT_NE(stored, nullptr);
    EXPECT_EQ(stored->display_name, "First");
    EXPECT_EQ(stored->version, "1");
    EXPECT_EQ(registry.active_id(), "suborno-classic");
}

// Covers layout_registry.cpp:50-52 — find() returns the registered layout or
// nullptr for unknown and empty ids.
TEST(LayoutApiCoverage, FindReturnsRegisteredLayoutOrNull) {
    suborno::layout::LayoutRegistry registry;

    EXPECT_EQ(registry.find("ghost"), nullptr);
    EXPECT_EQ(registry.find(""), nullptr);

    ASSERT_TRUE(registry.register_layout("suborno-classic", make_classic_layout()));
    EXPECT_NE(registry.find("suborno-classic"), nullptr);
    EXPECT_EQ(registry.find("ghost"), nullptr);
    EXPECT_EQ(registry.find(""), nullptr);
}
