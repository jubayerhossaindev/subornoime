// Coverage tests for src/core/c_api.cpp (suite CApiCoverage).
// Each test names the previously uncovered gap lines it exercises; the happy
// path already covered by c_api_test.cpp is extended here with the status
// mappings, argument guards and buffer rules that were previously untested.

#include <array>
#include <cstddef>
#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "suborno/suborno.h"

namespace {

using SubornoEngineHandle = std::unique_ptr<SUBORNO_Engine, decltype(&suborno_engine_destroy)>;
using SubornoSessionHandle = std::unique_ptr<SUBORNO_Session, decltype(&suborno_session_destroy)>;

SubornoEngineHandle make_engine() {
    return SubornoEngineHandle(suborno_engine_create(), &suborno_engine_destroy);
}

SubornoSessionHandle make_session(SUBORNO_Engine* engine) {
    return SubornoSessionHandle(suborno_session_create(engine), &suborno_session_destroy);
}

}  // namespace

TEST(CApiCoverage, SessionCreateRejectsNullEngineAndDestroyAcceptsNull) {
    // Gap: c_api.cpp:93-96, 109-111 — null engine rejected; both destroys are no-ops on null.
    EXPECT_EQ(suborno_session_create(nullptr), nullptr);
    suborno_engine_destroy(nullptr);
    suborno_session_destroy(nullptr);
}

TEST(CApiCoverage, FullLifecycleExercisesEveryOutputParameter) {
    // Gap: c_api.cpp:117-128 — append and backspace happy paths through the C boundary.
    SubornoEngineHandle engine = make_engine();
    ASSERT_NE(engine, nullptr);
    SubornoSessionHandle session = make_session(engine.get());
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(suborno_session_append_utf8(session.get(), "ami"), SUBORNO_STATUS_UPDATED);
    // Backspace drops the last grapheme: "ami" -> "am" -> preedit "আম".
    EXPECT_EQ(suborno_session_backspace(session.get()), SUBORNO_STATUS_UPDATED);

    std::array<char, 64> preedit{};
    EXPECT_EQ(suborno_session_snapshot(session.get(), preedit.data(), preedit.size()),
              SUBORNO_STATUS_UPDATED);
    EXPECT_STREQ(preedit.data(), "আম");

    EXPECT_EQ(suborno_session_append_utf8(session.get(), "i"), SUBORNO_STATUS_UPDATED);

    std::size_t candidate_count = 0;
    EXPECT_EQ(suborno_session_candidate_count(session.get(), &candidate_count),
              SUBORNO_STATUS_UPDATED);
    ASSERT_GT(candidate_count, 0U);

    std::array<char, 64> candidate{};
    EXPECT_EQ(
        suborno_session_candidate_snapshot(session.get(), 0, candidate.data(), candidate.size()),
        SUBORNO_STATUS_UPDATED);
    EXPECT_STREQ(candidate.data(), "আমি");

    EXPECT_EQ(suborno_session_select_candidate(session.get(), 0), SUBORNO_STATUS_UPDATED);

    char* committed = nullptr;
    std::size_t committed_size = 0;
    EXPECT_EQ(suborno_session_commit(session.get(), &committed, &committed_size),
              SUBORNO_STATUS_UPDATED);
    ASSERT_NE(committed, nullptr);
    EXPECT_EQ(std::string(committed, committed_size), "আমি");
    EXPECT_EQ(committed_size, 9U);  // three Bengali code points, 3 UTF-8 bytes each
    suborno_string_free(committed);
}

TEST(CApiCoverage, AppendSurfacesPendingLiteralPassthroughLimitAndEmptyStatuses) {
    // Gap: c_api.cpp:26-37 — the pending, literal_passthrough, input_limit and empty
    // cases of the InputStatus -> SUBORNO_Status mapping.
    SubornoEngineHandle engine = make_engine();
    ASSERT_NE(engine, nullptr);

    {
        // A trailing ":" opens a control token: the composition stays pending.
        SubornoSessionHandle session = make_session(engine.get());
        ASSERT_NE(session, nullptr);
        EXPECT_EQ(suborno_session_append_utf8(session.get(), ":"), SUBORNO_STATUS_PENDING);
        std::size_t count = 99;
        EXPECT_EQ(suborno_session_candidate_count(session.get(), &count), SUBORNO_STATUS_PENDING);
        EXPECT_EQ(count, 0U);  // pending compositions produce no candidates yet
        // Completing the token turns the same composition into a danda.
        EXPECT_EQ(suborno_session_append_utf8(session.get(), "danda:"), SUBORNO_STATUS_UPDATED);
        std::array<char, 16> preedit{};
        EXPECT_EQ(suborno_session_snapshot(session.get(), preedit.data(), preedit.size()),
                  SUBORNO_STATUS_UPDATED);
        EXPECT_STREQ(preedit.data(), "।");
    }
    {
        // Characters the phonetic parser does not know pass through literally.
        SubornoSessionHandle session = make_session(engine.get());
        ASSERT_NE(session, nullptr);
        EXPECT_EQ(suborno_session_append_utf8(session.get(), "@"),
                  SUBORNO_STATUS_LITERAL_PASSTHROUGH);
        std::size_t count = 0;
        EXPECT_EQ(suborno_session_candidate_count(session.get(), &count),
                  SUBORNO_STATUS_LITERAL_PASSTHROUGH);
        ASSERT_GT(count, 0U);
        std::array<char, 16> candidate{};
        EXPECT_EQ(suborno_session_candidate_snapshot(
                      session.get(), 0, candidate.data(), candidate.size()),
                  SUBORNO_STATUS_UPDATED);
        EXPECT_STREQ(candidate.data(), "@");
    }
    {
        // 257 code points exceed the 256-code-point composition limit.
        SubornoSessionHandle session = make_session(engine.get());
        ASSERT_NE(session, nullptr);
        const std::string oversized(257, 'a');
        EXPECT_EQ(suborno_session_append_utf8(session.get(), oversized.c_str()),
                  SUBORNO_STATUS_INPUT_LIMIT);
        // The rejected input never entered the composition.
        std::array<char, 8> preedit{};
        EXPECT_EQ(suborno_session_snapshot(session.get(), preedit.data(), preedit.size()),
                  SUBORNO_STATUS_EMPTY);
        EXPECT_STREQ(preedit.data(), "");
    }
    {
        // An empty composition reports SUBORNO_STATUS_EMPTY with zero candidates.
        SubornoSessionHandle session = make_session(engine.get());
        ASSERT_NE(session, nullptr);
        std::size_t count = 42;
        EXPECT_EQ(suborno_session_candidate_count(session.get(), &count), SUBORNO_STATUS_EMPTY);
        EXPECT_EQ(count, 0U);
        EXPECT_EQ(suborno_session_append_utf8(session.get(), ""), SUBORNO_STATUS_EMPTY);
    }
}

TEST(CApiCoverage, CandidateCountAndSnapshotValidateArgumentsAndBuffers) {
    // Gap: c_api.cpp:147-176 — candidate_count zeroing/guards and candidate_snapshot
    // guards, bounds and buffer-size handling.
    SubornoEngineHandle engine = make_engine();
    ASSERT_NE(engine, nullptr);
    SubornoSessionHandle session = make_session(engine.get());
    ASSERT_NE(session, nullptr);
    ASSERT_EQ(suborno_session_append_utf8(session.get(), "ami"), SUBORNO_STATUS_UPDATED);

    std::size_t count = 7;
    EXPECT_EQ(suborno_session_candidate_count(session.get(), nullptr),
              SUBORNO_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(count, 7U);  // a null count pointer leaves caller memory untouched
    EXPECT_EQ(suborno_session_candidate_count(nullptr, &count), SUBORNO_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(count, 0U);  // the count is zeroed even when the call is rejected

    std::array<char, 64> candidate{};
    EXPECT_EQ(suborno_session_candidate_snapshot(nullptr, 0, candidate.data(), candidate.size()),
              SUBORNO_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(suborno_session_candidate_snapshot(session.get(), 0, nullptr, candidate.size()),
              SUBORNO_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(suborno_session_candidate_snapshot(session.get(), 0, candidate.data(), 0),
              SUBORNO_ERROR_INVALID_ARGUMENT);
    // "আমি" needs 9 bytes plus the terminating NUL.
    EXPECT_EQ(suborno_session_candidate_snapshot(session.get(), 0, candidate.data(), 4),
              SUBORNO_ERROR_BUFFER_TOO_SMALL);
    EXPECT_EQ(candidate.data()[0], '\0');  // the buffer is emptied before the size check
    EXPECT_EQ(suborno_session_candidate_snapshot(session.get(), 0, candidate.data(), 10),
              SUBORNO_STATUS_UPDATED);
    EXPECT_STREQ(candidate.data(), "আমি");
    EXPECT_EQ(suborno_session_candidate_count(session.get(), &count), SUBORNO_STATUS_UPDATED);
    ASSERT_GT(count, 0U);
    EXPECT_EQ(suborno_session_candidate_snapshot(
                  session.get(), count, candidate.data(), candidate.size()),
              SUBORNO_STATUS_CANDIDATE_OUT_OF_RANGE);
}

TEST(CApiCoverage, SelectCandidateGuardsNullSessionAndIndexBounds) {
    // Gap: c_api.cpp:178-186 — the null-session guard and bounds on empty and
    // populated candidate lists.
    SubornoEngineHandle engine = make_engine();
    ASSERT_NE(engine, nullptr);
    SubornoSessionHandle session = make_session(engine.get());
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(suborno_session_select_candidate(nullptr, 0), SUBORNO_ERROR_INVALID_ARGUMENT);
    // No composition yet: any index is out of range.
    EXPECT_EQ(suborno_session_select_candidate(session.get(), 0),
              SUBORNO_STATUS_CANDIDATE_OUT_OF_RANGE);

    ASSERT_EQ(suborno_session_append_utf8(session.get(), "ami"), SUBORNO_STATUS_UPDATED);
    std::size_t count = 0;
    ASSERT_EQ(suborno_session_candidate_count(session.get(), &count), SUBORNO_STATUS_UPDATED);
    ASSERT_GT(count, 0U);
    EXPECT_EQ(suborno_session_select_candidate(session.get(), 0), SUBORNO_STATUS_UPDATED);
    EXPECT_EQ(suborno_session_select_candidate(session.get(), count),
              SUBORNO_STATUS_CANDIDATE_OUT_OF_RANGE);
}

TEST(CApiCoverage, CommitGuardsArgumentsAndReportsEmptyCompositions) {
    // Gap: c_api.cpp:188-220 — commit/commit_candidate argument guards (195-197, 213-215)
    // and the empty-composition commit path (54-65 via InputStatus::empty).
    SubornoEngineHandle engine = make_engine();
    ASSERT_NE(engine, nullptr);
    SubornoSessionHandle session = make_session(engine.get());
    ASSERT_NE(session, nullptr);

    char marker = '\0';
    char* committed = &marker;
    std::size_t committed_size = 99;
    EXPECT_EQ(suborno_session_commit(nullptr, &committed, &committed_size),
              SUBORNO_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(committed, nullptr);
    EXPECT_EQ(committed_size, 0U);

    committed_size = 99;
    EXPECT_EQ(suborno_session_commit(session.get(), nullptr, &committed_size),
              SUBORNO_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(committed_size, 0U);

    committed = &marker;
    EXPECT_EQ(suborno_session_commit(session.get(), &committed, nullptr),
              SUBORNO_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(committed, nullptr);

    // commit_candidate mirrors the same argument guards.
    committed = &marker;
    committed_size = 99;
    EXPECT_EQ(suborno_session_commit_candidate(nullptr, 0, &committed, &committed_size),
              SUBORNO_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(committed, nullptr);
    EXPECT_EQ(committed_size, 0U);
    EXPECT_EQ(suborno_session_commit_candidate(session.get(), 0, nullptr, &committed_size),
              SUBORNO_ERROR_INVALID_ARGUMENT);
    committed = &marker;
    EXPECT_EQ(suborno_session_commit_candidate(session.get(), 0, &committed, nullptr),
              SUBORNO_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(committed, nullptr);

    // Empty composition: SUBORNO_STATUS_EMPTY with an allocated empty string.
    // The header's "NULL and 0 on failure" contract applies to negative error
    // codes only — the caller must still free this output.
    EXPECT_EQ(suborno_session_commit(session.get(), &committed, &committed_size),
              SUBORNO_STATUS_EMPTY);
    ASSERT_NE(committed, nullptr);
    EXPECT_EQ(committed_size, 0U);
    EXPECT_EQ(std::string(committed, committed_size), "");
    suborno_string_free(committed);

    EXPECT_EQ(suborno_session_commit_candidate(session.get(), 0, &committed, &committed_size),
              SUBORNO_STATUS_EMPTY);
    ASSERT_NE(committed, nullptr);
    EXPECT_EQ(committed_size, 0U);
    suborno_string_free(committed);
}

TEST(CApiCoverage, SessionsCreatedFromOneEngineStayIndependent) {
    // Gap: c_api.cpp:98-107 — every session_create wires a new session to the
    // engine shared by all sessions.
    SubornoEngineHandle engine = make_engine();
    ASSERT_NE(engine, nullptr);
    SubornoSessionHandle first = make_session(engine.get());
    SubornoSessionHandle second = make_session(engine.get());
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);

    EXPECT_EQ(suborno_session_append_utf8(first.get(), "ami"), SUBORNO_STATUS_UPDATED);
    EXPECT_EQ(suborno_session_append_utf8(second.get(), "tumi"), SUBORNO_STATUS_UPDATED);

    std::array<char, 64> first_preedit{};
    std::array<char, 64> second_preedit{};
    ASSERT_EQ(suborno_session_snapshot(first.get(), first_preedit.data(), first_preedit.size()),
              SUBORNO_STATUS_UPDATED);
    ASSERT_EQ(suborno_session_snapshot(second.get(), second_preedit.data(), second_preedit.size()),
              SUBORNO_STATUS_UPDATED);
    EXPECT_STREQ(first_preedit.data(), "আমি");
    EXPECT_STREQ(second_preedit.data(), "তুমি");
}

TEST(CApiCoverage, StringFreeAcceptsNull) {
    // Gap: c_api.cpp:222-224 — suborno_string_free(null) is a safe no-op.
    suborno_string_free(nullptr);
}
