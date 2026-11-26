// Final coverage-gap batch: phonetic session candidate navigation, rule-based
// constructor validation and UTF-8 lead-byte handling, national layout
// passthrough paths, the remaining punctuation key mappings, and the engine's
// load_settings correction of an unknown stored layout id.

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <vector>

#include "suborno/core/engine.hpp"
#include "suborno/dictionary/dictionary.hpp"
#include "suborno/layout-api/key_event.hpp"
#include "suborno/layouts/national/national_layout.hpp"
#include "suborno/layouts/phonetic/suborno_classic_layout.hpp"
#include "suborno/layouts/phonetic/rule_based_phonetic_layout.hpp"
#include "suborno/layouts/phonetic/suborno_phonetic_layout.hpp"
#include "suborno/settings/settings.hpp"

namespace {

using suborno::layout::KeyEvent;
using suborno::layout::PhysicalKey;
using suborno::layout::key_event_to_ascii;

std::filesystem::path temp_path(const std::string& name) {
    return std::filesystem::temp_directory_path() / name;
}

void remove_quietly(const std::filesystem::path& path) {
    std::error_code ignored;
    std::filesystem::remove_all(path, ignored);
}

suborno::core::EngineConfig config_with(const std::filesystem::path& settings_path) {
    suborno::core::EngineConfig config;
    config.settings_path = settings_path;
    return config;
}

}  // namespace

TEST(CoverageFinalGapsTest, PhoneticSessionNavigationWalksCandidatesForwardAndBackward) {
    suborno::layouts::SubornoPhoneticLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);
    // "cha" has token alternatives, so the candidate list holds several
    // distinct entries and the navigation state is observable.
    ASSERT_EQ(session->append_utf8("cha"), suborno::layout::InputStatus::updated);
    const auto snapshot = session->snapshot();
    ASSERT_GE(snapshot.candidates.size(), 2U);
    EXPECT_EQ(session->selected_candidate_index(), 0U);

    // Walk forward to the last candidate; further next-selection is refused.
    for (std::size_t expected = 1; expected < snapshot.candidates.size(); ++expected) {
        EXPECT_TRUE(session->select_next_candidate());
        EXPECT_EQ(session->selected_candidate_index(), expected);
    }
    EXPECT_FALSE(session->select_next_candidate());
    EXPECT_EQ(session->selected_candidate_index(), snapshot.candidates.size() - 1);

    // Walk back to the top; further previous-selection is refused.
    while (session->selected_candidate_index() > 0) {
        EXPECT_TRUE(session->select_previous_candidate());
    }
    EXPECT_FALSE(session->select_previous_candidate());
    EXPECT_EQ(session->selected_candidate_index(), 0U);
}

TEST(CoverageFinalGapsTest, PhoneticAppendReportsPendingForIncompleteHalantSequence) {
    suborno::layouts::SubornoPhoneticLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);
    // A trailing backslash awaits the grapheme it escapes: the parse stays
    // pending, the raw input is retained, and no composition is produced yet.
    EXPECT_EQ(session->append_utf8("k\\"), suborno::layout::InputStatus::pending);
    EXPECT_EQ(session->snapshot().raw_input_utf8, "k\\");
    EXPECT_EQ(session->snapshot().status, suborno::layout::InputStatus::pending);
    // Completing the escape resolves the pending state; the escaped 'a' is
    // emitted as literal passthrough rather than transliterated.
    EXPECT_EQ(session->append_utf8("a"), suborno::layout::InputStatus::literal_passthrough);
    EXPECT_FALSE(session->snapshot().raw_input_utf8.empty());
}

TEST(CoverageFinalGapsTest, PhoneticBackspaceLeavingTrailingEscapeStaysPending) {
    suborno::layouts::SubornoPhoneticLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);
    // "k\\a" parses fully (the backslash escapes the 'a'), so deleting the
    // final grapheme leaves "k\\" whose parse is pending again.
    ASSERT_EQ(session->append_utf8("k\\a"), suborno::layout::InputStatus::literal_passthrough);
    EXPECT_EQ(session->backspace(), suborno::layout::InputStatus::pending);
    EXPECT_EQ(session->snapshot().raw_input_utf8, "k\\");
}

TEST(CoverageFinalGapsTest, PhoneticCommitOnPendingEscapeCommitsTheLiteralText) {
    const std::filesystem::path db_path = temp_path("suborno_cov_final_pending_commit.sqlite");
    remove_quietly(db_path);
    {
        suborno::dictionary::Dictionary dictionary(db_path.string());
        suborno::layouts::SubornoPhoneticLayout layout;
        auto session = layout.create_session(&dictionary, {});
        ASSERT_NE(session, nullptr);
        ASSERT_EQ(session->append_utf8("k\\"), suborno::layout::InputStatus::pending);
        // A pending composition has no candidates, so commit() falls to the
        // direct path and records the literal commit for learning.
        const auto committed = session->commit();
        // The committed text keeps the mapped ক plus the literal backslash.
        EXPECT_EQ(committed.text_utf8, "ক\\");
        EXPECT_TRUE(session->snapshot().raw_input_utf8.empty());
    }
    remove_quietly(db_path);
}

TEST(CoverageFinalGapsTest, StatelessPhoneticLayoutWithDictionaryUsesTheSharedSession) {
    const std::filesystem::path db_path = temp_path("suborno_cov_final_stateless_dict.sqlite");
    remove_quietly(db_path);
    {
        suborno::dictionary::Dictionary dictionary(db_path.string());
        suborno::layouts::SubornoClassicLayout layout;
        // Stateless phonetic layouts delegate dictionary-backed sessions to
        // the shared LayoutInputSession implementation.
        auto session = layout.create_session(&dictionary, {});
        ASSERT_NE(session, nullptr);
        ASSERT_EQ(session->append_utf8("ka"), suborno::layout::InputStatus::updated);
        const auto committed = session->commit();
        EXPECT_EQ(committed.text_utf8, "কা");
    }
    remove_quietly(db_path);
}

TEST(CoverageFinalGapsTest, PhoneticCommitWithDictionaryRecordsLearning) {
    const std::filesystem::path db_path = temp_path("suborno_cov_final_phonetic_learning.sqlite");
    remove_quietly(db_path);
    {
        suborno::dictionary::Dictionary dictionary(db_path.string());
        suborno::layouts::SubornoPhoneticLayout layout;
        // Dictionary-backed creation takes the layout's dictionary branch and
        // the phonetic session records the commit for future transitions.
        auto session = layout.create_session(&dictionary, {});
        ASSERT_NE(session, nullptr);
        ASSERT_EQ(session->append_utf8("ami"), suborno::layout::InputStatus::updated);
        const auto committed = session->commit();
        EXPECT_EQ(committed.text_utf8, "আমি");
        EXPECT_EQ(dictionary.get_user_frequency("আমি"), 1);
        EXPECT_TRUE(session->snapshot().raw_input_utf8.empty());
    }
    remove_quietly(db_path);
}

TEST(CoverageFinalGapsTest, PhoneticSessionNavigationWithoutCandidatesReportsFalse) {
    suborno::layouts::SubornoClassicLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);
    EXPECT_FALSE(session->select_next_candidate());
    EXPECT_FALSE(session->select_previous_candidate());
    EXPECT_EQ(session->selected_candidate_index(), 0U);
    EXPECT_EQ(session->snapshot().candidates.size(), 0U);
}

TEST(CoverageFinalGapsTest, SubornoGenerateCandidatesOnEmptyInputYieldsThePrimaryCandidate) {
    suborno::layouts::SubornoPhoneticLayout layout;
    const auto candidates = layout.generate_candidates("");
    ASSERT_EQ(candidates.size(), 1U);
    EXPECT_EQ(candidates[0].text, "");
    EXPECT_EQ(candidates[0].source, suborno::layout::CandidateSource::layout);
    EXPECT_EQ(candidates[0].rationale, "primary phonetic output");
}

TEST(CoverageFinalGapsTest, SubornoGenerateCandidatesChAlternatesYieldMultipleCandidates) {
    suborno::layouts::SubornoPhoneticLayout layout;
    const auto candidates = layout.generate_candidates("cha");
    ASSERT_GE(candidates.size(), 2U);
    // The primary parsing (ch -> ছ) comes first; the alternative splits and
    // substitutions appear as additional ranked options (চ্হা, চা).
    EXPECT_EQ(candidates[0].text, "ছা");
    std::vector<std::string> texts;
    for (const auto& candidate : candidates) {
        texts.push_back(candidate.text);
    }
    EXPECT_NE(std::find(texts.begin(), texts.end(), std::string("চা")), texts.end());
    EXPECT_NE(std::find(texts.begin(), texts.end(), std::string("চ্হা")), texts.end());
    // Ranking follows the alternative order: score decays with rank, and the
    // rationale distinguishes the primary parse from alternative combinations.
    EXPECT_GT(candidates[0].score, candidates[1].score);
    EXPECT_EQ(candidates[0].rationale, "primary phonetic output");
    EXPECT_EQ(candidates[1].rationale, "phonetic alternative combination");
}

TEST(CoverageFinalGapsTest, ClassicTransliterateBackslashEscapesAndTrailingBackslashIsLiteral) {
    suborno::layouts::SubornoClassicLayout layout;
    // legacy reference IME count-based escaping: a single backslash is emitted
    // literally and the remaining text still parses.
    EXPECT_EQ(layout.transliterate("k\\a"), "\u0995\\\u0986");
    // A trailing backslash has nothing to escape and stays literal.
    EXPECT_EQ(layout.transliterate("k\\"), "\u0995\\");
}

TEST(CoverageFinalGapsTest, ClassicTransliterateHandlesTwoThreeAndFourByteUtf8Sequences) {
    suborno::layouts::SubornoClassicLayout layout;
    // é = 2-byte lead, ক = 3-byte lead, emoji = 4-byte lead: the byte-length
    // classifier must accept every well-formed lead and pass the unmapped
    // graphemes through unchanged.
    const std::string output = layout.transliterate(
        "k\xC3\xA9"
        "ক"
        "\xF0\x9F\x98\x80");
    EXPECT_NE(output.find("ক"), std::string::npos);
    EXPECT_NE(output.find("\xC3\xA9"), std::string::npos);
    EXPECT_NE(output.find("\xF0\x9F\x98\x80"), std::string::npos);
    // The mapped 'k' is still transliterated inside the same stream.
    EXPECT_NE(output.find("ক"), output.rfind("ক"));
}

TEST(CoverageFinalGapsTest, RuleBasedLayoutConstructorRejectsInvalidTokenTables) {
    using suborno::layouts::phonetic::PhoneticTokenDefinition;
    using suborno::layouts::phonetic::PhoneticTokenKind;
    using suborno::layouts::phonetic::RuleBasedPhoneticLayout;

    EXPECT_THROW(
        RuleBasedPhoneticLayout(std::span<const PhoneticTokenDefinition>{}, false, "profile"),
        std::invalid_argument);

    const std::vector<PhoneticTokenDefinition> valid{
        PhoneticTokenDefinition{"k", PhoneticTokenKind::consonant, "ক", ""}};
    EXPECT_THROW(RuleBasedPhoneticLayout(valid, false, ""), std::invalid_argument);

    const std::vector<PhoneticTokenDefinition> empty_output{
        PhoneticTokenDefinition{"k", PhoneticTokenKind::consonant, "", ""}};
    EXPECT_THROW(RuleBasedPhoneticLayout(empty_output, false, "profile"), std::invalid_argument);

    const std::vector<PhoneticTokenDefinition> duplicates{
        PhoneticTokenDefinition{"k", PhoneticTokenKind::consonant, "ক", ""},
        PhoneticTokenDefinition{"k", PhoneticTokenKind::consonant, "খ", ""}};
    EXPECT_THROW(RuleBasedPhoneticLayout(duplicates, false, "profile"), std::invalid_argument);
}

TEST(CoverageFinalGapsTest, NationalLayoutPassesUnmappedBytesAndDecodesBengaliOutput) {
    suborno::layouts::NationalLayout layout;
    // Latin digits map to Bangla digits (National digit row).
    EXPECT_EQ(layout.transliterate("123"), "১২৩");
    // Punctuation keys without a Bangla mapping pass their glyph through and
    // must not corrupt the composition state of later keys (jgj = ক + ্ + ক).
    EXPECT_EQ(layout.transliterate("[];"), "[];");
    EXPECT_EQ(layout.transliterate("jgj;"), "ক্ক;");
    // Mapped keys produce valid multi-byte Bengali output; Shift+W carries
    // য় as the decomposed NFC form (য + nukta).
    EXPECT_EQ(layout.transliterate("qw"), "ঙয");
    EXPECT_EQ(layout.transliterate("qW"), std::string("ঙ") + "\u09AF\u09BC");
}

TEST(CoverageFinalGapsTest, KeyEventAsciiMappingCoversEveryPunctuationBothShiftStates) {
    // The remaining punctuation rows: equal and minus (plus their shifted
    // forms), plus a bracket/grave sweep that pins the whole switch.
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::equal, {}}), '=');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::equal, {.shift = true}}), '+');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::minus, {}}), '-');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::minus, {.shift = true}}), '_');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::left_bracket, {.shift = true}}), '{');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::right_bracket, {}}), ']');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::grave, {.shift = true}}), '~');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::apostrophe, {.shift = true}}), '"');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::period, {}}), '.');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::period, {.shift = true}}), '>');
}

TEST(CoverageFinalGapsTest, EngineLoadSettingsCorrectsUnknownStoredLayoutId) {
    const auto settings_path = temp_path("suborno_cov_final_load_unknown_layout.ini");
    remove_quietly(settings_path);

    suborno::settings::EngineSettings stored;
    stored.active_layout_id = "layout-that-does-not-exist";
    ASSERT_EQ(suborno::settings::SettingsStore(settings_path).save(stored),
              suborno::settings::SettingsStatus::ok);

    suborno::core::Engine engine(config_with(settings_path));
    // load_settings() must replace the unknown stored id with the registry's
    // active layout instead of leaving a dangling reference.
    const auto result = engine.load_settings();
    ASSERT_EQ(result.status, suborno::settings::SettingsStatus::ok);
    EXPECT_NE(result.settings.active_layout_id, "layout-that-does-not-exist");
    EXPECT_EQ(result.settings.active_layout_id, engine.active_layout_id());

    remove_quietly(settings_path);
}
