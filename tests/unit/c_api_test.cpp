#include <array>
#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "suborno/suborno.h"

TEST(CApiTest, CreatesGenericInputSessionAndCommitsBengaliText) {
    std::unique_ptr<SUBORNO_Engine, decltype(&suborno_engine_destroy)> engine(
        suborno_engine_create(), &suborno_engine_destroy);
    ASSERT_NE(engine, nullptr);

    std::unique_ptr<SUBORNO_Session, decltype(&suborno_session_destroy)> session(
        suborno_session_create(engine.get()), &suborno_session_destroy);
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(suborno_session_append_utf8(session.get(), "ami"), SUBORNO_STATUS_UPDATED);

    std::array<char, 64> preedit{};
    EXPECT_EQ(suborno_session_snapshot(session.get(), preedit.data(), preedit.size()),
              SUBORNO_STATUS_UPDATED);
    EXPECT_STREQ(preedit.data(), "আমি");

    char* committed_text = nullptr;
    std::size_t committed_size = 0;
    const int commit_status =
        suborno_session_commit(session.get(), &committed_text, &committed_size);
    std::unique_ptr<char, decltype(&suborno_string_free)> committed(committed_text,
                                                                    &suborno_string_free);

    ASSERT_EQ(commit_status, SUBORNO_STATUS_UPDATED);
    ASSERT_NE(committed, nullptr);
    EXPECT_EQ(std::string(committed.get(), committed_size), "আমি");
}

TEST(CApiTest, SessionKeepsItsEngineAliveAfterEngineHandleIsDestroyed) {
    std::unique_ptr<SUBORNO_Engine, decltype(&suborno_engine_destroy)> engine(
        suborno_engine_create(), &suborno_engine_destroy);
    ASSERT_NE(engine, nullptr);

    std::unique_ptr<SUBORNO_Session, decltype(&suborno_session_destroy)> session(
        suborno_session_create(engine.get()), &suborno_session_destroy);
    ASSERT_NE(session, nullptr);

    engine.reset();

    EXPECT_EQ(suborno_session_append_utf8(session.get(), "ami"), SUBORNO_STATUS_UPDATED);

    std::array<char, 64> preedit{};
    EXPECT_EQ(suborno_session_snapshot(session.get(), preedit.data(), preedit.size()),
              SUBORNO_STATUS_UPDATED);
    EXPECT_STREQ(preedit.data(), "আমি");
}

TEST(CApiTest, RejectsNullArgumentsAtTheBoundary) {
    std::unique_ptr<SUBORNO_Engine, decltype(&suborno_engine_destroy)> engine(
        suborno_engine_create(), &suborno_engine_destroy);
    ASSERT_NE(engine, nullptr);

    std::unique_ptr<SUBORNO_Session, decltype(&suborno_session_destroy)> session(
        suborno_session_create(engine.get()), &suborno_session_destroy);
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(suborno_session_append_utf8(nullptr, "ami"), SUBORNO_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(suborno_session_append_utf8(session.get(), nullptr), SUBORNO_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(suborno_session_snapshot(nullptr, nullptr, 64), SUBORNO_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(suborno_session_snapshot(session.get(), nullptr, 64), SUBORNO_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(suborno_session_backspace(nullptr), SUBORNO_ERROR_INVALID_ARGUMENT);

    char* committed = nullptr;
    size_t committed_size = 0;
    EXPECT_EQ(suborno_session_commit(session.get(), nullptr, &committed_size),
              SUBORNO_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(suborno_session_commit(session.get(), &committed, nullptr),
              SUBORNO_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(committed, nullptr);
    EXPECT_EQ(committed_size, 0U);
}

TEST(CApiTest, ReportsBufferTooSmallForThePreeditSnapshot) {
    std::unique_ptr<SUBORNO_Engine, decltype(&suborno_engine_destroy)> engine(
        suborno_engine_create(), &suborno_engine_destroy);
    ASSERT_NE(engine, nullptr);

    std::unique_ptr<SUBORNO_Session, decltype(&suborno_session_destroy)> session(
        suborno_session_create(engine.get()), &suborno_session_destroy);
    ASSERT_NE(session, nullptr);

    ASSERT_EQ(suborno_session_append_utf8(session.get(), "ami"), SUBORNO_STATUS_UPDATED);

    // "আমি" needs 9 bytes plus the NUL terminator.
    std::array<char, 4> small_buffer{};
    EXPECT_EQ(suborno_session_snapshot(session.get(), small_buffer.data(), small_buffer.size()),
              SUBORNO_ERROR_BUFFER_TOO_SMALL);
    std::array<char, 10> exact_buffer{};
    EXPECT_EQ(suborno_session_snapshot(session.get(), exact_buffer.data(), exact_buffer.size()),
              SUBORNO_STATUS_UPDATED);
    EXPECT_STREQ(exact_buffer.data(), "আমি");
}

TEST(CApiTest, RejectsOutOfRangeCandidatesAndCommitsTheSelectedOne) {
    std::unique_ptr<SUBORNO_Engine, decltype(&suborno_engine_destroy)> engine(
        suborno_engine_create(), &suborno_engine_destroy);
    ASSERT_NE(engine, nullptr);

    std::unique_ptr<SUBORNO_Session, decltype(&suborno_session_destroy)> session(
        suborno_session_create(engine.get()), &suborno_session_destroy);
    ASSERT_NE(session, nullptr);

    ASSERT_EQ(suborno_session_append_utf8(session.get(), "ami"), SUBORNO_STATUS_UPDATED);

    size_t candidate_count = 0;
    ASSERT_EQ(suborno_session_candidate_count(session.get(), &candidate_count),
              SUBORNO_STATUS_UPDATED);
    ASSERT_GT(candidate_count, 0U);

    std::array<char, 64> candidate_text{};
    EXPECT_EQ(suborno_session_candidate_snapshot(
                  session.get(), candidate_count, candidate_text.data(), candidate_text.size()),
              SUBORNO_STATUS_CANDIDATE_OUT_OF_RANGE);

    EXPECT_EQ(suborno_session_select_candidate(session.get(), candidate_count),
              SUBORNO_STATUS_CANDIDATE_OUT_OF_RANGE);

    char* committed = nullptr;
    size_t committed_size = 0;
    ASSERT_EQ(suborno_session_commit_candidate(session.get(), 0, &committed, &committed_size),
              SUBORNO_STATUS_UPDATED);
    ASSERT_NE(committed, nullptr);
    EXPECT_EQ(std::string(committed, committed_size), "আমি");
    suborno_string_free(committed);
}
