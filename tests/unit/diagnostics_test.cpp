// §8 Error handling and recovery: the diagnostic log. The privacy contract
// is enforced here exactly as the header promises it: user-typed text never
// enters the log (the API cannot even accept it), scrub_user_text turns any
// text payload into a count marker, and the ring plus file sink stay bounded
// and thread-safe under concurrent producers.
#include "suborno/core/diagnostics.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {

using suborno::core::DiagnosticLog;
using suborno::core::DiagnosticSeverity;

/// Collects every logged message of a snapshot into one searchable string.
std::string joined_messages(const std::vector<suborno::core::DiagnosticEntry>& entries) {
    std::string joined;
    for (const auto& entry : entries) {
        joined += entry.message;
        joined += '\n';
        joined += entry.category;
        joined += '\n';
    }
    return joined;
}

/// Scoped isolate: one shared process-wide logger means each test resets it.
class IsolatedLog {
  public:
    IsolatedLog() {
        DiagnosticLog::instance().clear();
        DiagnosticLog::instance().set_min_severity(DiagnosticSeverity::debug);
        DiagnosticLog::instance().clear_file_sink();
    }
    ~IsolatedLog() {
        DiagnosticLog::instance().clear();
        DiagnosticLog::instance().set_min_severity(DiagnosticSeverity::debug);
        DiagnosticLog::instance().clear_file_sink();
    }
};

}  // namespace

TEST(DiagnosticsTest, SeverityThresholdFiltersLowerLevels) {
    IsolatedLog isolated;
    auto& logger = DiagnosticLog::instance();
    logger.set_min_severity(DiagnosticSeverity::warning);

    logger.log(DiagnosticSeverity::debug, "session", "debug event");
    logger.log(DiagnosticSeverity::info, "session", "info event");
    logger.log(DiagnosticSeverity::warning, "settings", "settings fallback");
    logger.log(DiagnosticSeverity::error, "tsf", "commit failed");

    const auto entries = logger.snapshot();
    ASSERT_EQ(entries.size(), 2U);
    EXPECT_EQ(entries[0].severity, DiagnosticSeverity::warning);
    EXPECT_EQ(entries[0].category, "settings");
    EXPECT_EQ(entries[1].severity, DiagnosticSeverity::error);
    EXPECT_EQ(entries[1].category, "tsf");
}

TEST(DiagnosticsTest, ScrubUserTextEmitsOnlyACodepointMarker) {
    // The marker must carry a truthful count and none of the characters, for
    // ASCII input and for multi-byte Bengali alike.
    const std::string ascii = DiagnosticLog::scrub_user_text("hello world");
    EXPECT_EQ(ascii, "[redacted 11 codepoints]");
    EXPECT_EQ(ascii.find("hello"), std::string::npos);

    const std::string bengali = DiagnosticLog::scrub_user_text("আমি বাংলা টাইপ করি");
    EXPECT_EQ(bengali, "[redacted 18 codepoints]");
    EXPECT_EQ(bengali.find("আমি"), std::string::npos);
    EXPECT_EQ(bengali.find("বাংলা"), std::string::npos);

    // A truncated multi-byte tail is counted by its lead bytes without
    // crashing: E0 A6 95 E0 A6 has two lead bytes (E0, E0).
    const std::string truncated = DiagnosticLog::scrub_user_text("\xE0\xA6\x95\xE0\xA6");
    EXPECT_EQ(truncated, "[redacted 2 codepoints]");

    EXPECT_EQ(DiagnosticLog::scrub_user_text(""), "[redacted 0 codepoints]");
}

TEST(DiagnosticsTest, RingBufferStaysBoundedUnderHeavyLogging) {
    IsolatedLog isolated;
    auto& logger = DiagnosticLog::instance();
    // The process-wide sequence keeps rising across tests on purpose; the
    // ring assertions are relative to the baseline this test starts from.
    const std::uint64_t base = logger.last_sequence();
    for (int index = 0; index < 10'000; ++index) {
        logger.log(DiagnosticSeverity::info, "soak", "entry");
    }
    const auto entries = logger.snapshot();
    EXPECT_EQ(entries.size(), DiagnosticLog::ring_capacity);
    // Sequences are globally ordered, so the snapshot is oldest-first.
    EXPECT_EQ(entries.front().sequence,
              base + static_cast<std::uint64_t>(10'000) - DiagnosticLog::ring_capacity);
    EXPECT_EQ(entries.back().sequence, base + 9'999U);
}

TEST(DiagnosticsTest, ConcurrentProducersNeverLoseOrCorruptEntries) {
    IsolatedLog isolated;
    auto& logger = DiagnosticLog::instance();
    constexpr int producer_count = 8;
    constexpr int messages_per_producer = 2'000;

    // The logger is process-wide and monotonic: earlier tests (and the
    // producers themselves) consume sequence numbers, so this test anchors
    // every absolute expectation on the baseline it starts from.
    const std::uint64_t base_sequence = logger.last_sequence();

    std::vector<std::thread> producers;
    for (int producer = 0; producer < producer_count; ++producer) {
        producers.emplace_back([&logger, producer] {
            for (int index = 0; index < messages_per_producer; ++index) {
                logger.log(
                    DiagnosticSeverity::info,
                    "thread-test",
                    "producer " + std::to_string(producer) + " entry " + std::to_string(index));
            }
        });
    }
    // Concurrent readers exercise the snapshot path while producers run.
    std::thread reader([&logger] {
        for (int index = 0; index < 200; ++index) {
            const auto entries = logger.snapshot();
            EXPECT_LE(entries.size(), DiagnosticLog::ring_capacity);
        }
    });
    for (auto& thread : producers) {
        thread.join();
    }
    reader.join();

    const auto entries = logger.snapshot();
    ASSERT_EQ(entries.size(), DiagnosticLog::ring_capacity);
    // Sequence numbers are unique across producers: 16k entries logged, the
    // ring keeps the newest 512 and the last sequence lands exactly at the
    // baseline plus the total logged by this test.
    EXPECT_EQ(
        entries.back().sequence,
        base_sequence + static_cast<std::uint64_t>(producer_count) * messages_per_producer - 1U);
    for (std::size_t index = 1; index < entries.size(); ++index) {
        EXPECT_GT(entries[index].sequence, entries[index - 1].sequence);
    }
}

TEST(DiagnosticsTest, FileSinkWritesEntriesAndClosesCleanly) {
    IsolatedLog isolated;
    const auto sink_path = std::filesystem::temp_directory_path() / "suborno_diag_sink_test.log";
    std::filesystem::remove(sink_path);

    auto& logger = DiagnosticLog::instance();
    ASSERT_TRUE(logger.set_file_sink(sink_path));
    logger.log(DiagnosticSeverity::warning, "installer", "profile registration retried");
    logger.clear_file_sink();

    std::ifstream sink(sink_path);
    ASSERT_TRUE(sink.is_open());
    std::stringstream contents;
    contents << sink.rdbuf();
    const std::string text = contents.str();
    EXPECT_NE(text.find("warning|installer|profile registration retried"), std::string::npos);
    // After clear_file_sink nothing further is written.
    logger.log(DiagnosticSeverity::error, "installer", "after close");
    std::ifstream reopened(sink_path);
    std::stringstream reopened_contents;
    reopened_contents << reopened.rdbuf();
    EXPECT_EQ(reopened_contents.str().find("after close"), std::string::npos);
    EXPECT_FALSE(logger.file_sink_active());

    std::error_code ignored;
    std::filesystem::remove(sink_path, ignored);
}

TEST(DiagnosticsTest, SinkFailureIsReportedAndLoggingContinues) {
    IsolatedLog isolated;
    auto& logger = DiagnosticLog::instance();
    // A directory cannot be opened for appending.
    const auto directory = std::filesystem::temp_directory_path();
    EXPECT_FALSE(logger.set_file_sink(directory));
    EXPECT_FALSE(logger.file_sink_active());
    logger.log(DiagnosticSeverity::error, "session", "still works");
    EXPECT_EQ(logger.snapshot().size(), 1U);
}

TEST(DiagnosticsTest, NoUserTextEverReachesTheLog) {
    // The documented workflow for any diagnostic that must mention text
    // volume: scrub first, log the marker. This test is the enforcement
    // point — the log must never contain the user's characters.
    IsolatedLog isolated;
    auto& logger = DiagnosticLog::instance();
    const std::string user_input = "আমার গোপন টেক্সট secret123";
    logger.log(DiagnosticSeverity::info,
               "pipeline",
               "composition dropped: " + DiagnosticLog::scrub_user_text(user_input));

    const std::string logged = joined_messages(logger.snapshot());
    EXPECT_NE(logged.find("composition dropped: [redacted 26 codepoints]"), std::string::npos);
    EXPECT_EQ(logged.find("secret123"), std::string::npos);
    EXPECT_EQ(logged.find("গোপন"), std::string::npos);
    EXPECT_EQ(logged.find("আমার"), std::string::npos);
}
