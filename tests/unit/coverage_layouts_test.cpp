// Coverage tests for the layout cluster: the Khipro compositional layout, the

// Probhat fixed layout, the shared LayoutInputSession, the declarative mapping

// layout, the layout registry, the English passthrough layout, and the

// key-event / session contract headers. Each test asserts real behavior for a

// measured coverage gap (the per-test comment names the closed lines/branches).

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "suborno/dictionary/dictionary.hpp"
#include "suborno/layout-api/candidate.hpp"
#include "suborno/layout-api/declarative_layout.hpp"
#include "suborno/layout-api/input_layout.hpp"
#include "suborno/layout-api/input_session.hpp"
#include "suborno/layout-api/key_event.hpp"
#include "suborno/layout-api/layout_registry.hpp"
#include "suborno/layouts/english/english_layout.hpp"
#include "suborno/layouts/khipro/khipro_layout.hpp"
#include "suborno/layouts/probhat/probhat_layout.hpp"
#include "suborno/platform/linux/linux_adapter.hpp"

namespace {

using suborno::layout::Candidate;
using suborno::layout::CandidateSource;
using suborno::layout::InputSession;
using suborno::layout::InputStatus;
using suborno::layout::KeyEvent;
using suborno::layout::PhysicalKey;

// Builds a Probhat-style physical key event (modifiers off unless requested).

KeyEvent key_event(std::uint16_t scan_code,
                   bool shift = false,
                   bool caps_lock = false,
                   std::uint16_t repeat_count = 1) {
    return {.modifiers = {.shift = shift, .caps_lock = caps_lock},
            .scan_code = scan_code,
            .repeat_count = repeat_count,
            .key_down = true};
}

void remove_quietly(const std::filesystem::path& path) {
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
}

// Writes a declarative mapping table (one key<TAB>value per line) to a

// temporary file and returns its path. Callers remove the file afterwards.

std::filesystem::path write_mapping_file(const std::string& file_name,
                                         const std::string& contents) {
    const std::filesystem::path path = std::filesystem::temp_directory_path() / file_name;
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output << contents;
    return path;
}

// Deterministic stub layout for the shared session contract: 'k' maps to ক,

// 'z' maps to য, everything else passes through. Candidate generation is

// input-dependent so tests can observe empty, single, and multi-candidate

// lists (raw input containing 'z' produces no candidates at all, 'w' produces

// two layout variants, 'v' produces three).

class StubCandidateLayout final : public suborno::layout::StatelessInputLayout {
  public:
    [[nodiscard]] suborno::layout::LayoutKind kind() const noexcept override {
        return suborno::layout::LayoutKind::fixed;
    }

    [[nodiscard]] std::string transliterate(const std::string& input) const override {
        std::string output;
        output.reserve(input.size());
        for (const char character : input) {
            if (character == 'k') {
                output += "ক";
            } else if (character == 'z') {
                output += "য";
            } else {
                output.push_back(character);
            }
        }
        return output;
    }

    [[nodiscard]] std::vector<Candidate> generate_candidates(
        const std::string& input) const override {
        if (input.find('z') != std::string::npos) {
            return {};
        }
        if (input.find('w') != std::string::npos) {
            return {stub_candidate("ক", "stub-one"), stub_candidate("ম", "stub-two")};
        }
        if (input.find('v') != std::string::npos) {
            return {stub_candidate("ক", "stub-one"),
                    stub_candidate("খ", "stub-two"),
                    stub_candidate("গ", "stub-three")};
        }
        return {stub_candidate(transliterate(input), "stub-output")};
    }

  private:
    static Candidate stub_candidate(std::string text, const char* variant) {
        return {.text = std::move(text),
                .normalized_text = {},
                .stable_id = std::string("stub:") + variant,
                .source = CandidateSource::layout,
                .score = 1.0,
                .rationale = "stub layout variant"};
    }
};

// Header-only adapter double: deriving from LinuxAdapter and destroying the

// instance runs the interface's defaulted constructor and virtual destructor

// without linking any host adapter library.

class CoverageLinuxAdapter final : public suborno::platform::linux::LinuxAdapter {
  public:
    [[nodiscard]] suborno::platform::linux::LinuxAdapterStatus status() const noexcept override {
        return suborno::platform::linux::LinuxAdapterStatus::available;
    }
};

}  // namespace

// ---------------------------------------------------------------------------

// Khipro compositional layout (src/layouts/khipro/khipro_layout.cpp)

// ---------------------------------------------------------------------------

// Covers khipro_layout.cpp:163-169 ("..."), 179-185 (".."), 196-202 ("$$")

// and the single-dot danda fallback for contrast.

TEST(CoverageKhiproTest, TransliteratesEllipsisAndEscapedDotAndDollarSequences) {
    const suborno::layouts::KhiproLayout layout;

    // bn-khipro.mim spec: the dot/dollar slicers emit one character fewer

    // than Suborno's earlier native table.

    EXPECT_EQ(layout.transliterate("..."), "..");
    EXPECT_EQ(layout.transliterate(".."), ".");
    EXPECT_EQ(layout.transliterate("$$"), "$");
    EXPECT_EQ(layout.transliterate("."), "।");
    EXPECT_EQ(layout.transliterate("k.."), "ক.");
    EXPECT_EQ(layout.transliterate("ki..."), "কি..");
}

// Covers khipro_layout.cpp:220-226 ("+f"), 228-234 ("-f"), 236-242 ("=f");

// bare operators must stay literal.

TEST(CoverageKhiproTest, TransliteratesMultiplicationDivisionAndInequalitySequences) {
    const suborno::layouts::KhiproLayout layout;

    EXPECT_EQ(layout.transliterate("+f"), "×");
    EXPECT_EQ(layout.transliterate("-f"), "÷");
    EXPECT_EQ(layout.transliterate("=f"), "≠");
    EXPECT_EQ(layout.transliterate("k+f"), "ক×");
    EXPECT_EQ(layout.transliterate("+"), "+");
    EXPECT_EQ(layout.transliterate("-"), "-");
    EXPECT_EQ(layout.transliterate("="), "=");
}

// Covers khipro_layout.cpp:151-158 (the "/" branch with no joiner to erase)

// and the vowel guards at 270-280: the inherent-"o" drop only fires before

// another consonant, and "wi" only takes its forced split before "f".

TEST(CoverageKhiproTest, SlashWithoutJoinerAndUnforcedVowelFormsStayLiteral) {
    const suborno::layouts::KhiproLayout layout;

    // A slash with no joiner to erase stays literal in the spec.

    EXPECT_EQ(layout.transliterate("k/"), "ক/");
    EXPECT_EQ(layout.transliterate("ako"), "আক");
    EXPECT_EQ(layout.transliterate("kwi"), "কৈ");
    EXPECT_EQ(layout.transliterate("kor"), "কর");
}

// Covers khipro_layout.cpp:116-118 (invalid UTF-8 short-circuit), 364-368

// (generate_candidates with empty output) and 369-377 (candidate identity).

TEST(CoverageKhiproTest, InvalidUtf8AndEmptyInputYieldNoOutputAndNoCandidates) {
    const suborno::layouts::KhiproLayout layout;

    // Suborno layout contract: invalid UTF-8 is rejected up front.

    EXPECT_THROW(static_cast<void>(layout.transliterate("\xFF")), std::invalid_argument);
    EXPECT_THROW(static_cast<void>(layout.generate_candidates("\xFF")), std::invalid_argument);
    EXPECT_TRUE(layout.generate_candidates("").empty());

    const auto candidates = layout.generate_candidates("ami");
    ASSERT_EQ(candidates.size(), 1U);
    EXPECT_EQ(candidates.front().text, "আমি");
    EXPECT_EQ(candidates.front().normalized_text, "আমি");
    EXPECT_EQ(candidates.front().stable_id, "layout:khipro:আমি");
    EXPECT_EQ(candidates.front().source, CandidateSource::layout);
    EXPECT_DOUBLE_EQ(candidates.front().score, 1.0);
    EXPECT_EQ(candidates.front().rationale, "Khipro compositional output (legacy reference IME mim engine)");
}

// Covers khipro_layout.cpp — characters with no mapping fall through byte

// for byte; Latin digits are now mapped to Bangla digits (inventory fix).

TEST(CoverageKhiproTest, UnmappedCharactersPassThroughUnchanged) {
    const suborno::layouts::KhiproLayout layout;

    EXPECT_EQ(layout.transliterate("123"), "১২৩");
    EXPECT_EQ(layout.transliterate("ami123"), "আমি১২৩");
    EXPECT_EQ(layout.transliterate("k1"), "ক১");
}

// Covers khipro_layout.cpp:323-349 — every KhiproInputSession forwarding

// method against the shared LayoutInputSession implementation.

TEST(CoverageKhiproTest, ForwardsEverySessionOperationToTheSharedImplementation) {
    const suborno::layouts::KhiproLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(session->append_key({PhysicalKey::letter_k, {}}), InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "ক");
    EXPECT_EQ(session->snapshot().raw_input_utf8, "k");

    EXPECT_EQ(session->append_utf8("t"), InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "ক্ত");

    EXPECT_EQ(session->backspace(), InputStatus::updated);
    EXPECT_EQ(session->snapshot().raw_input_utf8, "k");
    EXPECT_EQ(session->snapshot().preedit_utf8, "ক");

    ASSERT_EQ(session->snapshot().candidates.size(), 1U);
    EXPECT_TRUE(session->select_candidate(0));
    EXPECT_EQ(session->selected_candidate_index(), 0U);
    EXPECT_FALSE(session->select_next_candidate());
    EXPECT_FALSE(session->select_previous_candidate());

    EXPECT_EQ(session->commit_candidate(7).status, InputStatus::candidate_out_of_range);
    EXPECT_EQ(session->commit().text_utf8, "ক");
    EXPECT_TRUE(session->snapshot().raw_input_utf8.empty());

    ASSERT_EQ(session->append_utf8("kt"), InputStatus::updated);
    session->reset();
    EXPECT_EQ(session->snapshot().status, InputStatus::empty);
    EXPECT_TRUE(session->snapshot().raw_input_utf8.empty());
    EXPECT_EQ(session->selected_candidate_index(), 0U);
}

// Covers khipro_layout.cpp:310-318 — the dictionary-backed session branch of

// the KhiproInputSession constructor.

TEST(CoverageKhiproTest, CreatesDictionaryBackedSessionsThroughTheDictionaryBranch) {
    const std::filesystem::path db_path = std::filesystem::temp_directory_path()
                                          / "suborno_coverage_layouts_khipro_dictionary.sqlite";
    remove_quietly(db_path);
    {
        suborno::dictionary::Dictionary dictionary(db_path.string());
        dictionary.add_entry("ক্ত", "ক্ত", 5);

        const suborno::layouts::KhiproLayout layout;
        auto session = layout.create_session(&dictionary, {});
        ASSERT_NE(session, nullptr);
        EXPECT_EQ(session->append_utf8("kt"), InputStatus::updated);
        EXPECT_EQ(session->snapshot().preedit_utf8, "ক্ত");
        ASSERT_FALSE(session->snapshot().candidates.empty());
        EXPECT_EQ(session->snapshot().candidates.front().text, "ক্ত");
        EXPECT_EQ(session->commit().text_utf8, "ক্ত");
    }
    remove_quietly(db_path);
}

// ---------------------------------------------------------------------------

// Probhat fixed layout (src/layouts/probhat/probhat_layout.cpp)

// ---------------------------------------------------------------------------

// Covers probhat_layout.cpp:185-188 (invalid UTF-8) and 149-156, 192-199

// (bytes with no physical key pass through untouched).

TEST(CoverageProbhatTest, InvalidUtf8YieldsEmptyAndUnmappedBytesPassThrough) {
    const suborno::layouts::ProbhatLayout layout;

    EXPECT_EQ(layout.transliterate("\xFF"), "");
    EXPECT_EQ(layout.transliterate("\n"), "\n");
    EXPECT_EQ(layout.transliterate("\t"), "\t");
    EXPECT_EQ(layout.transliterate("k\nm"), "ক\nম");
}

// Covers probhat_layout.cpp:205-222 (append_utf8 validation and per-byte

// event mapping) and 297-304 (the composition limit inside append_output).

TEST(CoverageProbhatTest, AppendUtf8ValidatesInvalidEmptyAndOverLimitInput) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(session->append_utf8("\xFF"), InputStatus::invalid_utf8);
    EXPECT_EQ(session->append_utf8(""), InputStatus::empty);
    EXPECT_TRUE(session->snapshot().raw_input_utf8.empty());

    EXPECT_EQ(session->append_utf8("ka"), InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "কা");

    EXPECT_EQ(session->append_utf8("\n"), InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "কা\n");

    EXPECT_EQ(session->append_utf8(std::string(300, 'a')), InputStatus::input_limit);
    EXPECT_EQ(session->snapshot().preedit_utf8, "কা\n");
}

// Covers probhat_layout.cpp:297-304 — exactly max_input_code_points code points

// is accepted (the boundary of the over-limit guard).

TEST(CoverageProbhatTest, AppendUtf8AcceptsInputAtTheExactCodePointLimit) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    const std::string limit_input(InputSession::max_input_code_points, 'a');
    EXPECT_EQ(session->append_utf8(limit_input), InputStatus::updated);

    std::string expected;
    const std::string matra = "া";
    for (std::size_t index = 0; index < InputSession::max_input_code_points; ++index) {
        expected += matra;
    }
    EXPECT_EQ(session->snapshot().preedit_utf8, expected);
    EXPECT_EQ(session->snapshot().status, InputStatus::updated);
}

// Covers probhat_layout.cpp:224-234 — the Windows key and key-up events are

// shortcuts, never text.

TEST(CoverageProbhatTest, AppendKeyRejectsWindowsChordsAndKeyUpEvents) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    const KeyEvent windows_key{.modifiers = {.windows = true}, .scan_code = 0x25};
    EXPECT_EQ(session->append_key(windows_key), InputStatus::empty);

    const KeyEvent key_up{.scan_code = 0x25, .key_down = false};
    EXPECT_EQ(session->append_key(key_up), InputStatus::empty);

    EXPECT_TRUE(session->snapshot().raw_input_utf8.empty());
}

// Covers probhat_layout.cpp:235-244 — repeat_count 0 means one press, larger

// counts repeat the same output, and repeats beyond the shared input limit

// are refused without touching the composition.

TEST(CoverageProbhatTest, AppendKeyHonorsZeroSmallAndOverLimitRepeatCounts) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(session->append_key(key_event(0x25, false, false, 0)), InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "ক");

    session->reset();
    EXPECT_EQ(session->append_key(key_event(0x21, false, false, 3)), InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "ততত");

    session->reset();
    EXPECT_EQ(session->append_key(key_event(0x25, false, false, 300)), InputStatus::input_limit);
    EXPECT_TRUE(session->snapshot().preedit_utf8.empty());
}

// Covers probhat_layout.cpp:247-258, 260-267, 269-283 and 287-294 — every

// no-composition answer of the fixed-layout session.

TEST(CoverageProbhatTest, EmptySessionsReportEmptyStatusesForEveryOperation) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(session->backspace(), InputStatus::empty);
    const auto committed = session->commit();
    EXPECT_TRUE(committed.text_utf8.empty());
    EXPECT_EQ(committed.status, InputStatus::empty);
    EXPECT_EQ(session->commit_candidate(0).status, InputStatus::candidate_out_of_range);
    EXPECT_FALSE(session->select_candidate(0));
    EXPECT_FALSE(session->select_next_candidate());
    EXPECT_FALSE(session->select_previous_candidate());
    EXPECT_EQ(session->selected_candidate_index(), 0U);

    const auto snapshot = session->snapshot();
    EXPECT_TRUE(snapshot.raw_input_utf8.empty());
    EXPECT_TRUE(snapshot.preedit_utf8.empty());
    EXPECT_TRUE(snapshot.candidates.empty());
    EXPECT_EQ(snapshot.status, InputStatus::empty);
}

// Covers probhat_layout.cpp:315-328 — generate_candidates mirrors the

// transliteration with the layout identity; empty output yields no candidates.

TEST(CoverageProbhatTest, GenerateCandidatesMirrorTransliteration) {
    const suborno::layouts::ProbhatLayout layout;

    EXPECT_TRUE(layout.generate_candidates("").empty());
    EXPECT_TRUE(layout.generate_candidates("\xFF").empty());

    const auto candidates = layout.generate_candidates("k");
    ASSERT_EQ(candidates.size(), 1U);
    EXPECT_EQ(candidates.front().text, "ক");
    EXPECT_EQ(candidates.front().normalized_text, "ক");
    EXPECT_EQ(candidates.front().stable_id, "layout:probhat:ক");
    EXPECT_EQ(candidates.front().source, CandidateSource::layout);
    EXPECT_DOUBLE_EQ(candidates.front().score, 1.0);
    EXPECT_EQ(candidates.front().rationale, "Probhat physical-key output");
}

// Covers probhat_layout.cpp:178-182 — shift XOR caps_lock, including keys

// that CapsLock must not affect (no CAPLOK/SGCAPS flags in the KLC table).

TEST(CoverageProbhatTest, ShiftAndCapsLockCombineAsXorOnKeysMarkedInTheKlc) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(session->append_key(key_event(0x10, true, true)), InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "দ");

    session->reset();
    EXPECT_EQ(session->append_key(key_event(0x0C, true, true)), InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "_");

    session->reset();
    EXPECT_EQ(session->append_key(key_event(0x0C, false, false)), InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "-");
}

// Covers probhat_layout.cpp:172-176 — keys whose AltGr+Shift column is empty

// stay text-less even with Shift held.

TEST(CoverageProbhatTest, EmptyAltGrLayersProduceNoPreedit) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    const KeyEvent altgr{.modifiers = {.shift = true, .control = true, .alt = true},
                         .scan_code = 0x13};
    EXPECT_EQ(session->append_key(altgr), InputStatus::empty);
    EXPECT_TRUE(session->snapshot().preedit_utf8.empty());
}

// ---------------------------------------------------------------------------

// Shared LayoutInputSession (src/layouts/layout_input_session.cpp)

// ---------------------------------------------------------------------------

// Covers layout_input_session.cpp:27-29 — empty appends report empty and

// never touch the composition.

TEST(CoverageLayoutSessionTest, AppendUtf8EmptyInputReportsEmptyWithoutMutatingState) {
    StubCandidateLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    ASSERT_EQ(session->append_utf8("k"), InputStatus::updated);
    EXPECT_EQ(session->append_utf8(""), InputStatus::empty);
    EXPECT_EQ(session->snapshot().raw_input_utf8, "k");
    EXPECT_EQ(session->snapshot().preedit_utf8, "ক");
}

// Covers layout_input_session.cpp:43-61 — backspace on an empty composition

// and on a single-grapheme composition both report empty.

TEST(CoverageLayoutSessionTest, BackspaceReportsEmptyForEmptyAndSingleGraphemeInput) {
    StubCandidateLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(session->backspace(), InputStatus::empty);

    ASSERT_EQ(session->append_utf8("km"), InputStatus::updated);
    EXPECT_EQ(session->backspace(), InputStatus::updated);
    EXPECT_EQ(session->snapshot().raw_input_utf8, "k");
    EXPECT_EQ(session->backspace(), InputStatus::empty);
    EXPECT_TRUE(session->snapshot().raw_input_utf8.empty());
}

// Covers layout_input_session.cpp:75-81 (commit) and 102-108

// (commit_candidate) on an empty session.

TEST(CoverageLayoutSessionTest, CommitAndCommitCandidateOnEmptySessionsReportEmpty) {
    StubCandidateLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    const auto committed = session->commit();
    EXPECT_TRUE(committed.text_utf8.empty());
    EXPECT_EQ(committed.status, InputStatus::empty);

    const auto candidate_commit = session->commit_candidate(0);
    EXPECT_TRUE(candidate_commit.text_utf8.empty());
    EXPECT_EQ(candidate_commit.status, InputStatus::empty);
}

// Covers layout_input_session.cpp:83-99 — commit with no candidates falls

// back to direct transliteration; learning only records when a dictionary is

// attached and learning is enabled (90-91), and the raw composition clears.

TEST(CoverageLayoutSessionTest, CommitWithoutCandidatesFallsBackToDirectTransliterationAndLearns) {
    const std::filesystem::path db_path =
        std::filesystem::temp_directory_path() / "suborno_coverage_layouts_commit_learning.sqlite";
    remove_quietly(db_path);
    {
        suborno::dictionary::Dictionary dictionary(db_path.string());
        StubCandidateLayout layout;

        // Learning disabled: the direct path must not record anything.

        auto muted =
            layout.create_session(&dictionary, {.learning_enabled = false, .previous_word = {}});
        ASSERT_NE(muted, nullptr);
        ASSERT_EQ(muted->append_utf8("z"), InputStatus::updated);
        EXPECT_TRUE(muted->snapshot().candidates.empty());
        EXPECT_EQ(muted->snapshot().selected_candidate_index, 0U);
        EXPECT_EQ(muted->snapshot().preedit_utf8, "য");
        const auto muted_result = muted->commit();
        EXPECT_EQ(muted_result.text_utf8, "য");
        EXPECT_EQ(muted_result.status, InputStatus::updated);
        EXPECT_EQ(dictionary.get_user_frequency("য"), 0);

        // Learning enabled: the same direct path records the commit.

        auto learner = layout.create_session(&dictionary, {});
        ASSERT_NE(learner, nullptr);
        ASSERT_EQ(learner->append_utf8("z"), InputStatus::updated);
        const auto learned = learner->commit();
        EXPECT_EQ(learned.text_utf8, "য");
        EXPECT_EQ(learned.status, InputStatus::updated);
        EXPECT_EQ(dictionary.get_user_frequency("য"), 1);
        EXPECT_TRUE(learner->snapshot().raw_input_utf8.empty());
        EXPECT_EQ(learner->snapshot().status, InputStatus::empty);
    }
    {
        // No dictionary: the direct path still commits the transliteration.

        StubCandidateLayout layout;
        auto session = layout.create_session(nullptr, {});
        ASSERT_NE(session, nullptr);
        ASSERT_EQ(session->append_utf8("z"), InputStatus::updated);
        const auto result = session->commit();
        EXPECT_EQ(result.text_utf8, "য");
        EXPECT_EQ(result.status, InputStatus::updated);
        EXPECT_TRUE(session->snapshot().raw_input_utf8.empty());
        EXPECT_EQ(session->snapshot().status, InputStatus::empty);
    }
    remove_quietly(db_path);
}

// Covers layout_input_session.cpp:110-133 — commit_candidate commits the

// selected candidate text and records learning (121-124).

TEST(CoverageLayoutSessionTest, CommitCandidateCommitsTheDictionaryMatchAndRecordsLearning) {
    const std::filesystem::path db_path =
        std::filesystem::temp_directory_path() / "suborno_coverage_layouts_candidate_commit.sqlite";
    remove_quietly(db_path);
    {
        suborno::dictionary::Dictionary dictionary(db_path.string());
        // The candidate engine looks dictionary words up by the normalized

        // transliteration of the raw input (layout_input_session feeds the

        // composition through the engine), so "কম" must be stored under the

        // key the stub produces for "km" — the documented mechanism by which

        // transliteration keys reach the dictionary at typing time.

        dictionary.add_entry("কম", "কm", 9);

        StubCandidateLayout layout;
        auto session = layout.create_session(&dictionary, {});
        ASSERT_NE(session, nullptr);
        ASSERT_EQ(session->append_utf8("km"), InputStatus::updated);

        const auto snapshot = session->snapshot();
        // Two candidates: the exact dictionary match (score 1.0) outranks the

        // stub layout's transliteration candidate.

        ASSERT_EQ(snapshot.candidates.size(), 2U);
        EXPECT_EQ(snapshot.candidates[0].text, "কম");
        EXPECT_EQ(snapshot.candidates[0].source, CandidateSource::system_dictionary);
        EXPECT_EQ(snapshot.candidates[1].text, "কm");
        EXPECT_EQ(snapshot.selected_candidate_index, 0U);

        const auto result = session->commit_candidate(0);
        EXPECT_EQ(result.text_utf8, "কম");
        EXPECT_EQ(result.status, InputStatus::updated);
        EXPECT_EQ(dictionary.get_user_frequency("কম"), 1);
        EXPECT_TRUE(session->snapshot().raw_input_utf8.empty());
        EXPECT_EQ(session->snapshot().status, InputStatus::empty);
    }
    remove_quietly(db_path);
}

// Covers layout_input_session.cpp:111-116 — an out-of-range index reports

// candidate_out_of_range and keeps the composition intact.

TEST(CoverageLayoutSessionTest, CommitCandidateOutOfRangeKeepsTheComposition) {
    StubCandidateLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    ASSERT_EQ(session->append_utf8("k"), InputStatus::updated);
    const auto result = session->commit_candidate(9);
    EXPECT_TRUE(result.text_utf8.empty());
    EXPECT_EQ(result.status, InputStatus::candidate_out_of_range);
    EXPECT_EQ(session->snapshot().raw_input_utf8, "k");
    EXPECT_EQ(session->snapshot().status, InputStatus::updated);
    EXPECT_EQ(session->selected_candidate_index(), 0U);
}

// Covers layout_input_session.cpp:118-132 — committing a valid layout

// candidate without a dictionary skips learning and still returns the text.

TEST(CoverageLayoutSessionTest, CommitCandidateWithoutDictionarySkipsLearning) {
    StubCandidateLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    ASSERT_EQ(session->append_utf8("k"), InputStatus::updated);
    const auto result = session->commit_candidate(0);
    EXPECT_EQ(result.text_utf8, "ক");
    EXPECT_EQ(result.status, InputStatus::updated);
    EXPECT_EQ(session->snapshot().status, InputStatus::empty);
}

// Covers layout_input_session.cpp:135-141 — out-of-range selections are

// rejected without moving the selection.

TEST(CoverageLayoutSessionTest, SelectCandidateRejectsOutOfRangeIndicesAndAcceptsValidOnes) {
    StubCandidateLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    ASSERT_EQ(session->append_utf8("k"), InputStatus::updated);
    EXPECT_FALSE(session->select_candidate(4));
    EXPECT_EQ(session->selected_candidate_index(), 0U);
    EXPECT_TRUE(session->select_candidate(0));
    EXPECT_EQ(session->selected_candidate_index(), 0U);
}

// Covers layout_input_session.cpp:143-157 and 160-161 — next/previous

// navigation clamps at both ends of the candidate list.

TEST(CoverageLayoutSessionTest, CandidateNavigationWalksForwardAndBackward) {
    StubCandidateLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    ASSERT_EQ(session->append_utf8("k"), InputStatus::updated);
    ASSERT_EQ(session->snapshot().candidates.size(), 1U);
    EXPECT_FALSE(session->select_next_candidate());
    EXPECT_FALSE(session->select_previous_candidate());
    EXPECT_EQ(session->selected_candidate_index(), 0U);

    ASSERT_EQ(session->append_utf8("w"), InputStatus::updated);
    ASSERT_EQ(session->snapshot().candidates.size(), 2U);
    EXPECT_TRUE(session->select_next_candidate());
    EXPECT_EQ(session->selected_candidate_index(), 1U);
    EXPECT_FALSE(session->select_next_candidate());
    EXPECT_TRUE(session->select_previous_candidate());
    EXPECT_EQ(session->selected_candidate_index(), 0U);
    EXPECT_FALSE(session->select_previous_candidate());
}

// Covers layout_input_session.cpp:164-167 — reset clears the composition and

// the selection, and the session stays usable afterwards.

TEST(CoverageLayoutSessionTest, ResetClearsCompositionAndSelectionState) {
    StubCandidateLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    ASSERT_EQ(session->append_utf8("km"), InputStatus::updated);
    ASSERT_TRUE(session->select_candidate(0));
    session->reset();
    EXPECT_TRUE(session->snapshot().raw_input_utf8.empty());
    EXPECT_EQ(session->snapshot().status, InputStatus::empty);
    EXPECT_EQ(session->selected_candidate_index(), 0U);
    EXPECT_EQ(session->append_utf8("k"), InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "ক");
}

// Covers layout_input_session.cpp:184-187 — snapshot reports 0 for empty

// candidate lists and the current selection otherwise; composition changes

// deliberately restart the selection at the top.

TEST(CoverageLayoutSessionTest, SnapshotReportsSelectionConsistentlyAcrossListChanges) {
    StubCandidateLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    ASSERT_EQ(session->append_utf8("kv"), InputStatus::updated);
    ASSERT_EQ(session->snapshot().candidates.size(), 3U);
    ASSERT_TRUE(session->select_candidate(2));
    EXPECT_EQ(session->snapshot().selected_candidate_index, 2U);

    // A composition with no candidates reports selection 0.

    StubCandidateLayout empty_layout;
    auto empty_session = empty_layout.create_session(nullptr, {});
    ASSERT_NE(empty_session, nullptr);
    ASSERT_EQ(empty_session->append_utf8("z"), InputStatus::updated);
    const auto empty_snapshot = empty_session->snapshot();
    EXPECT_TRUE(empty_snapshot.candidates.empty());
    EXPECT_EQ(empty_snapshot.selected_candidate_index, 0U);
    EXPECT_EQ(empty_snapshot.status, InputStatus::updated);
    EXPECT_EQ(empty_snapshot.preedit_utf8, "য");

    // Any composition change restarts the selection at the top of the list.

    ASSERT_EQ(session->append_utf8("w"), InputStatus::updated);
    ASSERT_EQ(session->snapshot().candidates.size(), 2U);
    EXPECT_EQ(session->snapshot().selected_candidate_index, 0U);
}

// Covers layout_input_session.cpp:69-73 — the previous word only steers

// next-word prediction when next_word_prediction_enabled is set; with the

// flag off, the same previous word leaves the candidate list untouched.

TEST(CoverageLayoutSessionTest, NextWordPredictionOnlyForwardsPreviousWordWhenEnabled) {
    const std::filesystem::path db_path =
        std::filesystem::temp_directory_path() / "suborno_coverage_layouts_prediction.sqlite";
    remove_quietly(db_path);
    {
        suborno::dictionary::Dictionary dictionary(db_path.string());
        dictionary.add_entry("ক", "ক", 5);
        // The transition target is intentionally not a dictionary word, so

        // only the prediction stage can ever surface it.

        dictionary.record_transition("ভালো", "কবিতা");

        StubCandidateLayout layout;
        auto predicting = layout.create_session(&dictionary,
                                                {.candidate_limit = 8,
                                                 .learning_enabled = false,
                                                 .previous_word = "ভালো",
                                                 .next_word_prediction_enabled = true});
        ASSERT_NE(predicting, nullptr);
        ASSERT_EQ(predicting->append_utf8("k"), InputStatus::updated);
        const auto predicted = predicting->snapshot();
        ASSERT_EQ(predicted.candidates.size(), 2U);
        EXPECT_EQ(predicted.candidates[0].text, "ক");
        EXPECT_EQ(predicted.candidates[1].text, "কবিতা");
        EXPECT_EQ(predicted.candidates[1].rationale, "next-word prediction");

        auto plain = layout.create_session(&dictionary,
                                           {.candidate_limit = 8,
                                            .learning_enabled = false,
                                            .previous_word = "ভালো",
                                            .next_word_prediction_enabled = false});
        ASSERT_NE(plain, nullptr);
        ASSERT_EQ(plain->append_utf8("k"), InputStatus::updated);
        const auto plain_snapshot = plain->snapshot();
        ASSERT_EQ(plain_snapshot.candidates.size(), 1U);
        EXPECT_EQ(plain_snapshot.candidates[0].text, "ক");
        // With prediction off the only candidate is the exact dictionary match;

        // the transition word never reaches the list.

        EXPECT_EQ(plain_snapshot.candidates[0].rationale, "exact dictionary match");
    }
    remove_quietly(db_path);
}

// ---------------------------------------------------------------------------

// InputSession::append_key default contract

// (include/suborno/layout-api/input_session.hpp:72-91)

// ---------------------------------------------------------------------------

// Covers input_session.hpp:77-83 — flagged logical text is appended, and

// unflagged logical text is appended too (LayoutInputSession has no

// append_key override, so the default implementation runs).

TEST(CoverageInputSessionContractTest, ForwardsLogicalTextIntoComposition) {
    StubCandidateLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    const KeyEvent flagged{
        .key = PhysicalKey::letter_a, .logical_text_available = true, .logical_text_utf8 = "আ"};
    EXPECT_EQ(session->append_key(flagged), InputStatus::updated);
    EXPECT_EQ(session->snapshot().raw_input_utf8, "আ");

    const KeyEvent unflagged{.key = PhysicalKey::letter_b, .logical_text_utf8 = "খ"};
    EXPECT_EQ(session->append_key(unflagged), InputStatus::updated);
    EXPECT_EQ(session->snapshot().raw_input_utf8, "আখ");
}

// Covers input_session.hpp:85-91 — the US-layout ASCII fallback for plain

// physical keys (shift handled through the shared modifiers).

TEST(CoverageInputSessionContractTest, FallsBackToUsAsciiForPlainTextKeys) {
    StubCandidateLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    const KeyEvent caps_b{.key = PhysicalKey::letter_b, .modifiers = {.caps_lock = true}};
    EXPECT_EQ(session->append_key(caps_b), InputStatus::updated);
    const KeyEvent semicolon{.key = PhysicalKey::semicolon};
    EXPECT_EQ(session->append_key(semicolon), InputStatus::updated);
    EXPECT_EQ(session->snapshot().raw_input_utf8, "B;");
    EXPECT_EQ(session->snapshot().preedit_utf8, "B;");
}

// Covers input_session.hpp:72-76 — Alt, the Windows key and key-up events are

// shortcuts (Ctrl alone is exercised by the existing session tests).

TEST(CoverageInputSessionContractTest, RejectsAltWindowsAndKeyUpEventsAsShortcuts) {
    StubCandidateLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(session->append_key({PhysicalKey::letter_k, {.alt = true}}), InputStatus::empty);
    EXPECT_EQ(session->append_key({PhysicalKey::letter_k, {.windows = true}}), InputStatus::empty);
    const KeyEvent key_up{.key = PhysicalKey::letter_k, .key_down = false};
    EXPECT_EQ(session->append_key(key_up), InputStatus::empty);
    EXPECT_TRUE(session->snapshot().raw_input_utf8.empty());
}

// Covers input_session.hpp:85-88 — keys without an ASCII mapping (a numpad

// digit with NumLock off) report empty instead of appending.

TEST(CoverageInputSessionContractTest, ReturnsEmptyForKeysWithoutAnAsciiMapping) {
    StubCandidateLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(session->append_key({PhysicalKey::numpad_5, {}}), InputStatus::empty);
    EXPECT_TRUE(session->snapshot().raw_input_utf8.empty());
}

// ---------------------------------------------------------------------------

// key_event.hpp ASCII mapping (include/suborno/layout-api/key_event.hpp)

// ---------------------------------------------------------------------------

// Covers key_event.hpp:173-192 — the punctuation switch cases that were not

// exercised before, in both shift states.

TEST(CoverageKeyEventTest, MapsEveryPunctuationKeyInBothShiftStates) {
    using suborno::layout::key_event_to_ascii;

    EXPECT_EQ(key_event_to_ascii({PhysicalKey::equal, {}}).value_or('\0'), '=');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::equal, {.shift = true}}).value_or('\0'), '+');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::minus, {}}).value_or('\0'), '-');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::minus, {.shift = true}}).value_or('\0'), '_');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::period, {}}).value_or('\0'), '.');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::period, {.shift = true}}).value_or('\0'), '>');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::grave, {}}).value_or('\0'), '`');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::grave, {.shift = true}}).value_or('\0'), '~');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::left_bracket, {}}).value_or('\0'), '[');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::left_bracket, {.shift = true}}).value_or('\0'), '{');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::right_bracket, {}}).value_or('\0'), ']');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::right_bracket, {.shift = true}}).value_or('\0'),
              '}');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::apostrophe, {}}).value_or('\0'), '\'');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::apostrophe, {.shift = true}}).value_or('\0'), '"');
}

// Covers key_event.hpp:193-194 — space maps to a space in both shift states.

TEST(CoverageKeyEventTest, MapsSpaceToSpaceInBothShiftStates) {
    using suborno::layout::key_event_to_ascii;

    EXPECT_EQ(key_event_to_ascii({PhysicalKey::space, {}}).value_or('\0'), ' ');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::space, {.shift = true}}).value_or('\0'), ' ');
}

// Covers key_event.hpp:195-196 — physical keys outside every mapped range

// have no ASCII character.

TEST(CoverageKeyEventTest, UnknownPhysicalKeysHaveNoAsciiMapping) {
    using suborno::layout::key_event_to_ascii;

    EXPECT_FALSE(key_event_to_ascii({PhysicalKey::unknown, {}}).has_value());
    const auto media_key = static_cast<PhysicalKey>(200);
    EXPECT_FALSE(key_event_to_ascii({media_key, {}}).has_value());
}

// Covers key_event.hpp:145-157 — the letter (shift XOR caps_lock) and digit

// (shift-only) ranges over their full domains.

TEST(CoverageKeyEventTest, SweepsEveryLetterAndTopRowDigitInBothCases) {
    using suborno::layout::key_event_to_ascii;

    for (char letter = 'a'; letter <= 'z'; ++letter) {
        const auto physical = static_cast<PhysicalKey>(letter);

        KeyEvent lower{};
        lower.key = physical;
        EXPECT_EQ(key_event_to_ascii(lower).value_or('\0'), letter);

        KeyEvent shifted{};
        shifted.key = physical;
        shifted.modifiers.shift = true;
        EXPECT_EQ(key_event_to_ascii(shifted).value_or('\0'),
                  static_cast<char>(letter - 'a' + 'A'));

        KeyEvent caps{};
        caps.key = physical;
        caps.modifiers.caps_lock = true;
        EXPECT_EQ(key_event_to_ascii(caps).value_or('\0'), static_cast<char>(letter - 'a' + 'A'));

        KeyEvent shift_caps{};
        shift_caps.key = physical;
        shift_caps.modifiers.shift = true;
        shift_caps.modifiers.caps_lock = true;
        EXPECT_EQ(key_event_to_ascii(shift_caps).value_or('\0'), letter);
    }

    constexpr char shifted_digits[] = ")!@#$%^&*(";
    for (std::uint8_t digit = 0; digit <= 9; ++digit) {
        const auto physical = static_cast<PhysicalKey>(static_cast<char>('0' + digit));
        const char unshifted = static_cast<char>('0' + digit);

        KeyEvent base{};
        base.key = physical;
        EXPECT_EQ(key_event_to_ascii(base).value_or('\0'), unshifted);

        KeyEvent shifted{};
        shifted.key = physical;
        shifted.modifiers.shift = true;
        EXPECT_EQ(key_event_to_ascii(shifted).value_or('\0'), shifted_digits[digit]);
    }
}

// Covers key_event.hpp:138-140 — Ctrl, Alt, the Windows key, and AltGr

// (Ctrl+Alt together) all suppress ASCII mapping.

TEST(CoverageKeyEventTest, ShortcutModifiersAndAltGrSuppressAsciiMapping) {
    using suborno::layout::key_event_to_ascii;

    EXPECT_FALSE(key_event_to_ascii({PhysicalKey::letter_a, {.control = true}}).has_value());
    EXPECT_FALSE(key_event_to_ascii({PhysicalKey::letter_a, {.alt = true}}).has_value());
    EXPECT_FALSE(key_event_to_ascii({PhysicalKey::letter_a, {.windows = true}}).has_value());
    EXPECT_FALSE(
        key_event_to_ascii({PhysicalKey::letter_a, {.control = true, .alt = true}}).has_value());
}

// Covers key_event.hpp:99-104, 118-135 and 160-167 — numpad digits need

// NumLock, the remap guard rejects top-row keys (with and without NumLock),

// and the remap clears shift/caps_lock and the logical-text fields.

TEST(CoverageKeyEventTest, NumpadKeysNeedNumLockAndTopRowKeysNeverRemap) {
    using suborno::layout::is_numpad_key;
    using suborno::layout::key_event_to_ascii;
    using suborno::layout::remap_numpad_to_top_row;

    EXPECT_EQ(key_event_to_ascii({PhysicalKey::numpad_9, {.num_lock = true}}).value_or('\0'), '9');
    EXPECT_FALSE(key_event_to_ascii({PhysicalKey::numpad_0, {}}).has_value());
    EXPECT_FALSE(key_event_to_ascii({PhysicalKey::numpad_5, {.num_lock = false}}).has_value());

    EXPECT_TRUE(is_numpad_key(PhysicalKey::numpad_0));
    EXPECT_TRUE(is_numpad_key(PhysicalKey::numpad_9));
    EXPECT_FALSE(is_numpad_key(PhysicalKey::digit_0));
    EXPECT_FALSE(is_numpad_key(PhysicalKey::semicolon));
    EXPECT_FALSE(is_numpad_key(PhysicalKey::space));

    KeyEvent top_row_with_num_lock{.key = PhysicalKey::letter_m, .modifiers = {.num_lock = true}};
    EXPECT_FALSE(remap_numpad_to_top_row(top_row_with_num_lock));
    EXPECT_EQ(top_row_with_num_lock.key, PhysicalKey::letter_m);
    EXPECT_EQ(top_row_with_num_lock.scan_code, 0U);

    KeyEvent top_row_without_num_lock{.key = PhysicalKey::letter_m};
    EXPECT_FALSE(remap_numpad_to_top_row(top_row_without_num_lock));
    EXPECT_EQ(top_row_without_num_lock.key, PhysicalKey::letter_m);

    KeyEvent numpad_nine{
        .key = PhysicalKey::numpad_9,
        .modifiers = {.shift = true, .caps_lock = true, .num_lock = true},
        .scan_code = 0x59,
    };
    ASSERT_TRUE(remap_numpad_to_top_row(numpad_nine));
    EXPECT_EQ(numpad_nine.key, PhysicalKey::digit_9);
    EXPECT_EQ(numpad_nine.scan_code, 0x0AU);
    EXPECT_FALSE(numpad_nine.modifiers.shift);
    EXPECT_FALSE(numpad_nine.modifiers.caps_lock);
    EXPECT_FALSE(numpad_nine.logical_text_available);
    EXPECT_TRUE(numpad_nine.logical_text_utf8.empty());
    EXPECT_EQ(key_event_to_ascii(numpad_nine).value_or('\0'), '9');
}

// ---------------------------------------------------------------------------

// Declarative mapping layout (src/layout-api/declarative_layout.cpp)

// ---------------------------------------------------------------------------

// Covers declarative_layout.cpp:16-20 — load() on a missing mapping file.

TEST(CoverageDeclarativeLayoutTest, LoadReturnsFalseForAMissingMappingFile) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "suborno_coverage_layouts_declarative_missing.tsv";
    remove_quietly(path);

    suborno::layout::DeclarativeLayout layout("coverage-decl-missing", path);
    EXPECT_FALSE(layout.load());
    remove_quietly(path);
}

// Covers declarative_layout.cpp:38-41 — on POSIX a directory opens but the

// read fails, so the stream goes bad and load() rejects it (on Windows the

// file never opens and 16-20 rejects it instead; both return false).

TEST(CoverageDeclarativeLayoutTest, LoadFailsWhenTheMappingPathIsADirectory) {
    const std::filesystem::path directory =
        std::filesystem::temp_directory_path() / "suborno_coverage_layouts_declarative_dir";
    std::error_code ignored;
    std::filesystem::create_directories(directory, ignored);

    suborno::layout::DeclarativeLayout layout("coverage-decl-dir", directory);
    EXPECT_FALSE(layout.load());

    std::filesystem::remove_all(directory, ignored);
}

// Covers declarative_layout.cpp:45-48 — id() returns the configured

// identifier and needs no successful load.

TEST(CoverageDeclarativeLayoutTest, IdReturnsTheConfiguredIdentifier) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "suborno_coverage_layouts_declarative_id.tsv";
    remove_quietly(path);

    const suborno::layout::DeclarativeLayout layout("coverage-decl-id", path);
    EXPECT_EQ(layout.id(), "coverage-decl-id");
    remove_quietly(path);
}

// Covers declarative_layout.cpp:53-77 — longest-match wins, and unmapped

// bytes pass through with their UTF-8 lead length (1, 2, 3 and 4 bytes).

TEST(CoverageDeclarativeLayoutTest, TransliteratePassesThroughUnmappedUtf8LeadLengths) {
    const auto path =
        write_mapping_file("suborno_coverage_layouts_declarative_passthrough.tsv", "k\tক\nkh\tখ\n");

    suborno::layout::DeclarativeLayout layout("coverage-decl-passthrough", path);
    ASSERT_TRUE(layout.load());

    EXPECT_EQ(layout.transliterate("Z"), "Z");
    EXPECT_EQ(layout.transliterate("\xC3\xA9"), "\xC3\xA9");
    EXPECT_EQ(layout.transliterate("ক"), "ক");
    EXPECT_EQ(layout.transliterate("\xF0\x9F\x98\x80"), "\xF0\x9F\x98\x80");
    EXPECT_EQ(layout.transliterate("a\xC3\xA9ক\xF0\x9F\x98\x80k"), "a\xC3\xA9ক\xF0\x9F\x98\x80ক");
    EXPECT_EQ(layout.transliterate("khZ"), "খZ");
    EXPECT_EQ(layout.transliterate("kkh"), "কখ");
    remove_quietly(path);
}

// Covers declarative_layout.cpp:28-37 — lines without a tab separator, empty

// values, and duplicate keys all abort the load with false.

TEST(CoverageDeclarativeLayoutTest, LoadRejectsBrokenLinesEmptyValuesAndDuplicateKeys) {
    const auto broken =
        write_mapping_file("suborno_coverage_layouts_decl_broken.tsv", "k\tক\nbroken-line\n");
    suborno::layout::DeclarativeLayout broken_layout("coverage-decl-broken", broken);
    EXPECT_FALSE(broken_layout.load());
    EXPECT_EQ(broken_layout.transliterate("k"), "k");
    remove_quietly(broken);

    const auto empty_value =
        write_mapping_file("suborno_coverage_layouts_decl_empty_value.tsv", "k\t\n");
    suborno::layout::DeclarativeLayout empty_value_layout("coverage-decl-empty-value", empty_value);
    EXPECT_FALSE(empty_value_layout.load());
    remove_quietly(empty_value);

    const auto duplicate =
        write_mapping_file("suborno_coverage_layouts_decl_duplicate.tsv", "k\tv1\nk\tv2\n");
    suborno::layout::DeclarativeLayout duplicate_layout("coverage-decl-duplicate", duplicate);
    EXPECT_FALSE(duplicate_layout.load());
    remove_quietly(duplicate);
}

// ---------------------------------------------------------------------------

// Layout registry (src/layout-api/layout_registry.cpp)

// ---------------------------------------------------------------------------

// Covers layout_registry.cpp:7-20, 41-48 and 50-57 — empty ids and null

// layouts are rejected, duplicates are rejected with caller ownership kept,

// the first registration becomes active, set_active switches, and unknown

// ids are never active.

TEST(CoverageLayoutRegistryTest, RejectsEmptyIdsNullLayoutsAndDuplicateRegistrations) {
    suborno::layout::LayoutRegistry registry;

    EXPECT_TRUE(registry.active_id().empty());
    EXPECT_EQ(registry.active(), nullptr);

    EXPECT_FALSE(registry.register_layout("", std::make_unique<suborno::layouts::ProbhatLayout>()));
    EXPECT_FALSE(registry.register_layout("probhat", nullptr));
    EXPECT_FALSE(registry.set_active("probhat"));
    EXPECT_EQ(registry.find("probhat"), nullptr);
    EXPECT_EQ(registry.find(""), nullptr);
    EXPECT_EQ(registry.active(), nullptr);

    ASSERT_TRUE(
        registry.register_layout("probhat", std::make_unique<suborno::layouts::ProbhatLayout>()));
    EXPECT_EQ(registry.active_id(), "probhat");
    ASSERT_NE(registry.active(), nullptr);
    EXPECT_EQ(registry.active()->kind(), suborno::layout::LayoutKind::fixed);
    EXPECT_EQ(registry.active(), registry.find("probhat"));

    // register_layout takes the unique_ptr by value, so the argument is

    // consumed by every call (standard move semantics); the meaningful

    // contract is the false return plus the untouched registry state.

    EXPECT_FALSE(
        registry.register_layout("probhat", std::make_unique<suborno::layouts::ProbhatLayout>()));
    EXPECT_EQ(registry.active_id(), "probhat");
    EXPECT_EQ(registry.active()->transliterate("k"), "ক");

    ASSERT_TRUE(
        registry.register_layout("english", std::make_unique<suborno::layouts::EnglishLayout>()));
    EXPECT_EQ(registry.active_id(), "probhat");

    EXPECT_TRUE(registry.set_active("english"));
    EXPECT_EQ(registry.active()->kind(), suborno::layout::LayoutKind::passthrough);
    EXPECT_FALSE(registry.set_active("ghost"));
    EXPECT_EQ(registry.active_id(), "english");
}

// Covers layout_registry.cpp:22-39 — an empty metadata id defaults to the

// registry key, explicit ids survive, unknown lookups return nullptr, and a

// rejected metadata registration keeps the original metadata.

TEST(CoverageLayoutRegistryTest, MetadataIdDefaultsToKeySurvivesExplicitIdsAndFailedRegistration) {
    suborno::layout::LayoutRegistry registry;

    const suborno::layout::LayoutMetadata defaulted{};
    ASSERT_TRUE(registry.register_layout(
        "probhat", std::make_unique<suborno::layouts::ProbhatLayout>(), defaulted));
    const suborno::layout::LayoutMetadata* defaulted_found = registry.find_metadata("probhat");
    ASSERT_NE(defaulted_found, nullptr);
    EXPECT_EQ(defaulted_found->id, "probhat");
    EXPECT_EQ(defaulted_found->display_name, "");
    EXPECT_EQ(defaulted_found->version, "1");

    const suborno::layout::LayoutMetadata explicit_metadata{"probe-id", "Probhat", "7"};
    ASSERT_TRUE(registry.register_layout("probhat-explicit",
                                         std::make_unique<suborno::layouts::ProbhatLayout>(),
                                         explicit_metadata));
    const suborno::layout::LayoutMetadata* explicit_found =
        registry.find_metadata("probhat-explicit");
    ASSERT_NE(explicit_found, nullptr);
    EXPECT_EQ(explicit_found->id, "probe-id");

    EXPECT_EQ(registry.find_metadata("ghost"), nullptr);

    const suborno::layout::LayoutMetadata rejected{"probhat", "Second", "9"};
    EXPECT_FALSE(registry.register_layout(
        "probhat", std::make_unique<suborno::layouts::ProbhatLayout>(), rejected));
    const suborno::layout::LayoutMetadata* stored = registry.find_metadata("probhat");
    ASSERT_NE(stored, nullptr);
    EXPECT_EQ(stored->version, "1");
    EXPECT_EQ(registry.active_id(), "probhat");
}

// ---------------------------------------------------------------------------

// English passthrough layout (src/layouts/english/english_layout.cpp)

// ---------------------------------------------------------------------------

// Covers english_layout.cpp:17-20 — empty input yields no candidates; other

// input echoes back with the layout identity.

TEST(CoverageEnglishLayoutTest, GenerateCandidatesRejectEmptyInputAndEchoOtherwise) {
    const suborno::layouts::EnglishLayout layout;

    EXPECT_EQ(layout.kind(), suborno::layout::LayoutKind::passthrough);
    EXPECT_TRUE(layout.generate_candidates("").empty());
    EXPECT_EQ(layout.transliterate("Hello, world!"), "Hello, world!");

    const auto candidates = layout.generate_candidates("Hello");
    ASSERT_EQ(candidates.size(), 1U);
    EXPECT_EQ(candidates.front().text, "Hello");
    EXPECT_EQ(candidates.front().normalized_text, "Hello");
    EXPECT_EQ(candidates.front().stable_id, "english:Hello");
    EXPECT_EQ(candidates.front().source, CandidateSource::layout);
    EXPECT_DOUBLE_EQ(candidates.front().score, 1.0);
    EXPECT_EQ(candidates.front().rationale, "passthrough output");
}

// ---------------------------------------------------------------------------

// Linux adapter interface (include/suborno/platform/linux/linux_adapter.hpp)

// ---------------------------------------------------------------------------

// Covers linux_adapter.hpp:13-24 — a derived adapter constructs, answers

// status() through the base interface, and runs the interface's virtual

// destructor on scope exit and through a base-owned pointer.

TEST(CoverageLinuxAdapterTest, DerivedAdaptersConstructAndDestructThroughTheInterface) {
    auto observed = suborno::platform::linux::LinuxAdapterStatus::unavailable;
    {
        const CoverageLinuxAdapter adapter;
        observed = adapter.status();
    }
    EXPECT_EQ(observed, suborno::platform::linux::LinuxAdapterStatus::available);

    auto heap_adapter = std::make_unique<CoverageLinuxAdapter>();
    const suborno::platform::linux::LinuxAdapter& interface_ref = *heap_adapter;
    EXPECT_EQ(interface_ref.status(), suborno::platform::linux::LinuxAdapterStatus::available);
    heap_adapter.reset();
}
