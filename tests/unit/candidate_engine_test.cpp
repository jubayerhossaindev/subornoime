#include "suborno/candidates/candidate_engine.hpp"

#include <filesystem>
#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "suborno/dictionary/dictionary.hpp"
#include "suborno/layout-api/input_layout.hpp"

namespace {

class TestFixedLayout final : public suborno::layout::StatelessInputLayout {
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
             "fixed:বাংলা",
             suborno::layout::CandidateSource::layout,
             1.0,
             "fixed"},
            {"বই", "বই", "fixed:বই", suborno::layout::CandidateSource::layout, 0.5, "fixed"},
        };
    }
};

std::string temporary_database_path(const char* filename) {
    return (std::filesystem::temp_directory_path() / filename).string();
}

}  // namespace

TEST(CandidateEngineTest, SupportsFixedLayoutsAndMergesDictionaryCandidates) {
    const std::string db_path =
        temporary_database_path("suborno_candidate_engine_fixed_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("বাংলা", "বাংলা", 10);
    dictionary.add_entry("ভাষা", "ab", 5);

    const TestFixedLayout layout;
    const suborno::candidates::CandidateEngine engine(layout, dictionary);
    const auto candidates = engine.make_candidates("ab", 3);

    ASSERT_EQ(candidates.size(), 3U);
    EXPECT_EQ(candidates[0].text, "বাংলা");
    EXPECT_EQ(candidates[0].source, suborno::layout::CandidateSource::system_dictionary);
    EXPECT_EQ(candidates[1].text, "ভাষা");
    EXPECT_EQ(candidates[1].source, suborno::layout::CandidateSource::system_dictionary);
    EXPECT_EQ(candidates[2].text, "বই");
    EXPECT_EQ(candidates[2].source, suborno::layout::CandidateSource::layout);

    static_cast<void>(std::remove(db_path.c_str()));
}

TEST(CandidateEngineTest, AppliesTheCandidateLimitAndSkipsDuplicateLayoutOutput) {
    const std::string db_path =
        temporary_database_path("suborno_candidate_engine_limit_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("বাংলা", "বাংলা", 10);

    const TestFixedLayout layout;
    const suborno::candidates::CandidateEngine engine(layout, &dictionary);
    const auto candidates = engine.make_candidates("বাংলা", 2);

    ASSERT_EQ(candidates.size(), 2U);
    EXPECT_EQ(candidates[0].text, "বাংলা");
    EXPECT_EQ(candidates[1].text, "বই");
    EXPECT_TRUE(engine.make_candidates("বাংলা", 0).empty());

    static_cast<void>(std::remove(db_path.c_str()));
}

TEST(CandidateEngineTest, IncludesNormalizedPrefixPredictionsBelowExactMatches) {
    const std::string db_path =
        temporary_database_path("suborno_candidate_engine_prefix_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("বাংলা", "bangla", 10);
    dictionary.add_entry("বাংলাদেশ", "bangladesh", 8);

    const TestFixedLayout layout;
    const suborno::candidates::CandidateEngine engine(layout, dictionary);
    const auto candidates = engine.make_candidates("bang", 4);

    ASSERT_GE(candidates.size(), 2U);
    EXPECT_EQ(candidates[0].text, "বাংলা");
    EXPECT_EQ(candidates[1].text, "বাংলাদেশ");
    EXPECT_EQ(candidates[0].rationale, "normalized input prefix match");

    static_cast<void>(std::remove(db_path.c_str()));
}

TEST(CandidateEngineTest, NextWordPredictionInsertsTransitionFollowersAbovePrefixMatches) {
    const std::string db_path =
        temporary_database_path("suborno_candidate_engine_prediction_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("বাংলা", "বাংলা", 10);
    // Normalized "abx" keeps বাংলাদেশ out of both exact stages: without a
    // transition it can only surface as a plain prefix completion (see the
    // regression test below). The transition history makes it a prediction.
    dictionary.add_entry("বাংলাদেশ", "abx", 5);
    dictionary.record_transition("ami", "বাংলাদেশ");

    const TestFixedLayout layout;
    const suborno::candidates::CandidateEngine engine(layout, dictionary);
    const auto candidates = engine.make_candidates("ab", 5, "ami");

    ASSERT_EQ(candidates.size(), 3U);
    // Exact matches still outrank prediction (1.0)...
    EXPECT_EQ(candidates[0].text, "বাংলা");
    EXPECT_EQ(candidates[0].rationale, "exact dictionary match");
    EXPECT_DOUBLE_EQ(candidates[0].score, 1.0);
    // ...and the transition follower lands between exact and prefix stages
    // with its own rationale and the 0.85 prediction score.
    EXPECT_EQ(candidates[1].text, "বাংলাদেশ");
    EXPECT_EQ(candidates[1].source, suborno::layout::CandidateSource::system_dictionary);
    EXPECT_EQ(candidates[1].rationale, "next-word prediction");
    EXPECT_DOUBLE_EQ(candidates[1].score, 0.85);
    // Layout alternatives keep their guaranteed slot after the dictionary.
    EXPECT_EQ(candidates[2].text, "বই");

    static_cast<void>(std::remove(db_path.c_str()));
}

TEST(CandidateEngineTest, PredictionWordThatAlsoMatchesExactAppearsOnce) {
    const std::string db_path =
        temporary_database_path("suborno_candidate_engine_prediction_dedup_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("বাংলা", "বাংলা", 10);
    // The transition follower is the same word the exact stage already found.
    dictionary.record_transition("ami", "বাংলা");

    const TestFixedLayout layout;
    const suborno::candidates::CandidateEngine engine(layout, dictionary);
    const auto candidates = engine.make_candidates("ab", 5, "ami");

    ASSERT_FALSE(candidates.empty());
    // The exact match owns the slot (it ran first); the prediction stage must
    // not push a duplicate through the shared seen-set.
    EXPECT_EQ(candidates[0].text, "বাংলা");
    EXPECT_EQ(candidates[0].rationale, "exact dictionary match");
    std::size_t occurrences = 0;
    for (const auto& candidate : candidates) {
        if (candidate.text == "বাংলা") {
            ++occurrences;
        }
        EXPECT_NE(candidate.rationale, "next-word prediction");
    }
    EXPECT_EQ(occurrences, 1U);

    static_cast<void>(std::remove(db_path.c_str()));
}

TEST(CandidateEngineTest, EmptyPreviousWordKeepsThePrePredictionCandidateList) {
    const std::string db_path =
        temporary_database_path("suborno_candidate_engine_prediction_off_test.sqlite");
    static_cast<void>(std::remove(db_path.c_str()));

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("বাংলা", "বাংলা", 10);
    dictionary.add_entry("বাংলাদেশ", "abx", 5);
    // Transition history exists, but without a previous word it must be inert.
    dictionary.record_transition("ami", "বাংলাদেশ");

    const TestFixedLayout layout;
    const suborno::candidates::CandidateEngine engine(layout, dictionary);
    const auto without_previous = engine.make_candidates("ab", 5);
    const auto explicit_empty = engine.make_candidates("ab", 5, "");
    const auto same_candidates = [](const std::vector<suborno::layout::Candidate>& left,
                                    const std::vector<suborno::layout::Candidate>& right) {
        if (left.size() != right.size()) {
            return false;
        }
        for (std::size_t index = 0; index < left.size(); ++index) {
            if (left[index].text != right[index].text
                || left[index].normalized_text != right[index].normalized_text
                || left[index].stable_id != right[index].stable_id
                || left[index].source != right[index].source
                || left[index].score != right[index].score
                || left[index].rationale != right[index].rationale) {
                return false;
            }
        }
        return true;
    };
    // Default and explicit-empty previous word agree (regression guard), and
    // the list is exactly the pre-prediction one.
    EXPECT_TRUE(same_candidates(without_previous, explicit_empty));
    ASSERT_EQ(without_previous.size(), 3U);
    EXPECT_EQ(without_previous[0].text, "বাংলা");
    EXPECT_EQ(without_previous[0].rationale, "exact dictionary match");
    EXPECT_EQ(without_previous[1].text, "বাংলাদেশ");
    EXPECT_EQ(without_previous[1].rationale, "normalized input prefix match");
    EXPECT_EQ(without_previous[2].text, "বই");
    for (const auto& candidate : without_previous) {
        EXPECT_NE(candidate.rationale, "next-word prediction");
    }

    static_cast<void>(std::remove(db_path.c_str()));
}
