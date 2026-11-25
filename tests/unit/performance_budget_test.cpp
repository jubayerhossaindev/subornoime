// §7 Performance and stability: latency budgets for the typing hot path.
// These are regression fences, not micro-benchmarks: the ceilings are set an
// order of magnitude above the measured steady state (single-digit
// microseconds per keystroke) so CI jitter never flakes them, yet any gross
// regression — an accidental O(n²) rescan, a lock in the append path, a
// dictionary reload per keystroke — trips them long before a user feels it.
// The google/benchmark suites in benchmarks/ produce the precise numbers on
// release hardware; this file keeps the CI build honest every commit.
//
// Part III verification note (D-003): a hard every-op guarantee flaked on a
// contended 2-core runner — the OS descheduled single ~10 µs appends for
// 19–24 ms. The per-op ceilings are unchanged; the looped fences are made
// statistical (median under the ceiling, at most 1% descheduling outliers),
// which catches every gross regression the fence was built for without
// flaking on scheduler noise.
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "suborno/core/engine.hpp"
#include "suborno/layouts/phonetic/phonetic_input_session.hpp"
#include "suborno/layouts/phonetic/suborno_phonetic_layout.hpp"

namespace {

using suborno::layout::InputStatus;
using suborno::layouts::PhoneticInputSession;
using suborno::layouts::SubornoPhoneticLayout;
using Clock = std::chrono::steady_clock;

/// Per-operation ceiling the hot path must respect on any build machine.
constexpr auto single_op_budget = std::chrono::milliseconds{5};
/// Whole-loop ceiling for the pipeline workloads below.
constexpr auto pipeline_budget = std::chrono::seconds{2};

template <typename Operation>
void expectWithinBudget(Operation&& operation,
                        std::chrono::milliseconds budget,
                        const char* label) {
    const auto started = Clock::now();
    operation();
    const auto elapsed = Clock::now() - started;
    EXPECT_LT(elapsed, budget)
        << label << " took "
        << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() << "ms";
}

/// Outlier-tolerant fence for looped operations: the median operation must
/// stay under the ceiling and at most 1% of operations may be descheduling
/// artifacts. The ceiling itself is never relaxed.
template <typename Operation>
void expectWithinBudgetWithOutliers(Operation&& operation,
                                    int iterations,
                                    std::chrono::milliseconds budget,
                                    const char* label) {
    std::vector<std::chrono::nanoseconds> samples;
    samples.reserve(static_cast<std::size_t>(iterations));
    for (int index = 0; index < iterations; ++index) {
        const auto started = Clock::now();
        operation();
        samples.push_back(Clock::now() - started);
    }
    const auto median_iter = samples.begin() + static_cast<std::ptrdiff_t>(samples.size() / 2);
    std::nth_element(samples.begin(), median_iter, samples.end());
    const auto median = *median_iter;
    const std::size_t outliers = static_cast<std::size_t>(std::count_if(
        samples.begin(), samples.end(), [budget](const std::chrono::nanoseconds& sample) {
            return sample >= budget;
        }));
    const std::size_t allowed = std::max<std::size_t>(1, samples.size() / 100);
    EXPECT_LT(median, budget)
        << label << " median took "
        << std::chrono::duration_cast<std::chrono::milliseconds>(median).count() << "ms";
    EXPECT_LE(outliers, allowed) << label << " exceeded the " << budget.count() << "ms ceiling in "
                                 << outliers << "/" << samples.size()
                                 << " ops (allowed: " << allowed << ")";
}

}  // namespace

TEST(PerformanceBudget, SingleKeystrokeAppendStaysWithinCeiling) {
    PhoneticInputSession session;
    expectWithinBudgetWithOutliers(
        [&session] {
            // Commit whenever the clamp reports a full buffer so the loop
            // keeps exercising the append path itself.
            if (session.append_utf8("a") == InputStatus::input_limit) {
                (void)session.commit();
            }
        },
        1'000,
        single_op_budget,
        "append_utf8");
}

TEST(PerformanceBudget, BackspaceAndSnapshotStayWithinCeiling) {
    PhoneticInputSession session;
    ASSERT_EQ(session.append_utf8("ami"), InputStatus::updated);
    expectWithinBudgetWithOutliers(
        [&session] {
            (void)session.backspace();
        },
        1'000,
        single_op_budget,
        "backspace");
    expectWithinBudgetWithOutliers(
        [&session] {
            (void)session.snapshot();
        },
        1'000,
        single_op_budget,
        "snapshot");
}

TEST(PerformanceBudget, CompositionCommitPipelineStaysWithinCeiling) {
    const SubornoPhoneticLayout layout;
    PhoneticInputSession session(layout);
    expectWithinBudget(
        [&session] {
            for (int index = 0; index < 1'000; ++index) {
                (void)session.append_utf8("ami");
                const auto committed = session.commit();
                ASSERT_EQ(committed.status, InputStatus::updated);
                ASSERT_EQ(committed.text_utf8, "আমি");
            }
        },
        pipeline_budget,
        "1000 append+commit cycles");
}

TEST(PerformanceBudget, CandidateNavigationStaysWithinCeiling) {
    const SubornoPhoneticLayout layout;
    PhoneticInputSession session(layout);
    ASSERT_EQ(session.append_utf8("chh"), InputStatus::updated);
    expectWithinBudget(
        [&session] {
            for (int index = 0; index < 10'000; ++index) {
                (void)session.select_next_candidate();
                (void)session.select_previous_candidate();
            }
        },
        pipeline_budget,
        "20k candidate taps");
}

TEST(PerformanceBudget, EngineSessionPipelineStaysWithinCeiling) {
    suborno::core::Engine engine;
    expectWithinBudget(
        [&engine] {
            for (int index = 0; index < 200; ++index) {
                auto session = engine.create_session();
                ASSERT_EQ(session->append_utf8("ami"), InputStatus::updated);
                const auto committed = session->commit();
                ASSERT_EQ(committed.status, InputStatus::updated);
            }
        },
        pipeline_budget,
        "200 engine session pipelines");
}

TEST(PerformanceBudget, AutoRepeatLoopStaysWithinCeiling) {
    // The held-key repeat loop must stay linear in repeat_count: one stuck
    // key event with the maximum repeat must not stall the typing thread.
    // 32 sampled events keep the fence statistical under scheduler noise
    // (same rationale as the looped fences above); the ceiling is unchanged.
    PhoneticInputSession session;
    expectWithinBudgetWithOutliers(
        [&session] {
            (void)session.append_utf8(std::string(200, 'a'));
            suborno::layout::KeyEvent repeat;
            repeat.key = suborno::layout::PhysicalKey::letter_b;
            repeat.repeat_count = 256;
            (void)session.append_key(repeat);
        },
        32,
        single_op_budget,
        "max repeat event");
}
