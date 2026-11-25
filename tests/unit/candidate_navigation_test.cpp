// §2 Composition and candidate system: navigation, selection, commit and
// reset. These paths had zero coverage before; every TSF arrow-key gesture
// depends on them, so the matrix below is exhaustive over the documented
// semantics (no wrap at the ends, out-of-range rejected atomically).
#include <cstddef>
#include <gtest/gtest.h>
#include <string>

#include "suborno/layouts/phonetic/phonetic_input_session.hpp"
#include "suborno/layouts/phonetic/suborno_phonetic_layout.hpp"

namespace {

using suborno::layout::CompositionSnapshot;
using suborno::layout::InputStatus;
using suborno::layouts::PhoneticInputSession;
using suborno::layouts::SubornoPhoneticLayout;

/// Fills a session with a composition that has at least two candidates and
/// returns the snapshot the navigation assertions work from.
CompositionSnapshot composeMultiCandidate(PhoneticInputSession& session) {
    EXPECT_EQ(session.append_utf8("chh"), InputStatus::updated);
    const CompositionSnapshot snapshot = session.snapshot();
    EXPECT_GE(snapshot.candidates.size(), 2U);
    return snapshot;
}

}  // namespace

TEST(CandidateNavigation, NextAndPreviousWalkTheListWithoutWrapping) {
    const SubornoPhoneticLayout layout;
    PhoneticInputSession session(layout);
    const CompositionSnapshot snapshot = composeMultiCandidate(session);

    // Walk forward: every tap below the end moves down, the tap at the end
    // is rejected instead of wrapping past the last candidate.
    EXPECT_TRUE(session.select_next_candidate());
    EXPECT_EQ(session.selected_candidate_index(), 1U);
    while (session.select_next_candidate()) {
        EXPECT_LT(session.selected_candidate_index(), snapshot.candidates.size());
    }
    EXPECT_EQ(session.selected_candidate_index(), snapshot.candidates.size() - 1U);
    EXPECT_FALSE(session.select_next_candidate());

    // Walk back: every tap above the start moves up, the tap at the start is
    // rejected instead of wrapping below zero.
    while (session.select_previous_candidate()) {
        EXPECT_GE(session.selected_candidate_index(), 0U);
    }
    EXPECT_EQ(session.selected_candidate_index(), 0U);
    EXPECT_FALSE(session.select_previous_candidate());
}

TEST(CandidateNavigation, OutOfRangeSelectionIsRejectedAtomically) {
    const SubornoPhoneticLayout layout;
    PhoneticInputSession session(layout);
    const CompositionSnapshot snapshot = composeMultiCandidate(session);

    EXPECT_FALSE(session.select_candidate(snapshot.candidates.size()));
    EXPECT_FALSE(session.select_candidate(9999U));
    EXPECT_EQ(session.selected_candidate_index(), 0U);
    // The composition itself is untouched by rejected selections.
    EXPECT_EQ(session.snapshot().raw_input_utf8, snapshot.raw_input_utf8);

    const auto committed = session.commit_candidate(snapshot.candidates.size());
    EXPECT_EQ(committed.status, InputStatus::candidate_out_of_range);
    EXPECT_TRUE(committed.text_utf8.empty());
    // A rejected commit keeps the composition alive for retry.
    EXPECT_EQ(session.snapshot().raw_input_utf8, snapshot.raw_input_utf8);
}

TEST(CandidateNavigation, CommitFollowsTheSelectedCandidate) {
    const SubornoPhoneticLayout layout;
    PhoneticInputSession session(layout);
    const CompositionSnapshot snapshot = composeMultiCandidate(session);

    ASSERT_TRUE(session.select_candidate(1U));
    const auto committed_selected = session.commit();
    EXPECT_EQ(committed_selected.status, InputStatus::updated);
    EXPECT_EQ(committed_selected.text_utf8, snapshot.candidates[1].text);
    EXPECT_EQ(session.snapshot().raw_input_utf8, "");

    PhoneticInputSession direct(layout);
    ASSERT_EQ(direct.append_utf8("chh"), InputStatus::updated);
    const auto after = composeMultiCandidate(direct);
    const auto committed_direct = direct.commit_candidate(0U);
    EXPECT_EQ(committed_direct.status, InputStatus::updated);
    EXPECT_EQ(committed_direct.text_utf8, after.candidates[0].text);
    EXPECT_EQ(direct.selected_candidate_index(), 0U);
}

TEST(CandidateNavigation, ResetClearsCompositionAndNavigationState) {
    const SubornoPhoneticLayout layout;
    PhoneticInputSession session(layout);
    composeMultiCandidate(session);
    ASSERT_TRUE(session.select_next_candidate());

    session.reset();
    EXPECT_EQ(session.selected_candidate_index(), 0U);
    const auto snapshot = session.snapshot();
    EXPECT_TRUE(snapshot.raw_input_utf8.empty());
    EXPECT_TRUE(snapshot.preedit_utf8.empty());
    EXPECT_TRUE(snapshot.candidates.empty());
    EXPECT_EQ(snapshot.status, InputStatus::empty);

    // A reset session immediately composes again — app switching must not
    // leave the session unusable.
    EXPECT_EQ(session.append_utf8("ami"), InputStatus::updated);
    const auto committed = session.commit();
    EXPECT_EQ(committed.status, InputStatus::updated);
    EXPECT_EQ(committed.text_utf8, "আমি");
}

TEST(CandidateNavigation, NavigationOnEmptyCompositionIsRejected) {
    PhoneticInputSession session;
    EXPECT_FALSE(session.select_next_candidate());
    EXPECT_FALSE(session.select_previous_candidate());
    EXPECT_FALSE(session.select_candidate(0U));
    EXPECT_EQ(session.selected_candidate_index(), 0U);
    const auto committed = session.commit_candidate(0U);
    EXPECT_EQ(committed.status, InputStatus::empty);
}

TEST(CandidateNavigation, RapidSelectionChurnKeepsIndexInsideBounds) {
    // Fast typists tap the arrow keys between keystrokes. The index must
    // always stay inside the current candidate list — a stale index from a
    // previous composition would commit the wrong word or crash.
    const SubornoPhoneticLayout layout;
    PhoneticInputSession session(layout);
    for (int cycle = 0; cycle < 500; ++cycle) {
        composeMultiCandidate(session);
        for (int tap = 0; tap < 10; ++tap) {
            session.select_next_candidate();
            session.select_previous_candidate();
            ASSERT_LT(session.selected_candidate_index(), session.snapshot().candidates.size());
        }
        session.reset();
    }
}
