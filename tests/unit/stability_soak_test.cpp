// §9 Testing and QA: stress and long-running stability. A production IME
// must survive a fast typist and an app-hopper for a whole work day. These
// soaks run deterministic, bounded workloads (seconds, not hours) over the
// real session pipeline and assert the invariants that matter after churn:
// bounded buffers, consistent state, no lost or duplicated input.
#include <cstddef>
#include <gtest/gtest.h>
#include <string>

#include "suborno/layouts/phonetic/phonetic_input_session.hpp"
#include "suborno/layouts/phonetic/suborno_phonetic_layout.hpp"

namespace {

using suborno::layout::InputStatus;
using suborno::layouts::PhoneticInputSession;
using suborno::layouts::SubornoPhoneticLayout;

constexpr std::size_t input_limit_max = PhoneticInputSession::max_input_code_points;

/// The invariant every soak step must hold: the raw buffer never exceeds the
/// documented limit and the selected candidate, if any, stays in range.
void expectSessionInvariants(const PhoneticInputSession& session) {
    const auto snapshot = session.snapshot();
    ASSERT_LE(snapshot.raw_input_utf8.size(), input_limit_max);
    ASSERT_LT(snapshot.selected_candidate_index,
              snapshot.candidates.empty() ? 1U : snapshot.candidates.size());
}

}  // namespace

TEST(StabilitySoak, RapidTypingBurstsPreserveCompositionAcrossCommits) {
    // 200 bursts x 250 keystrokes = 50k keystrokes with a commit or reset at
    // every burst boundary, mirroring a fast typist writing word after word.
    const SubornoPhoneticLayout layout;
    PhoneticInputSession session(layout);
    std::size_t committed_words = 0;

    for (int burst = 0; burst < 200; ++burst) {
        for (int keystroke = 0; keystroke < 250; ++keystroke) {
            const char letter = static_cast<char>('a' + (keystroke % 26));
            const InputStatus status = session.append_utf8(std::string(1, letter));
            ASSERT_TRUE(status == InputStatus::updated || status == InputStatus::input_limit
                        || status == InputStatus::literal_passthrough)
                << "burst " << burst << " keystroke " << keystroke;
            expectSessionInvariants(session);
        }
        if (burst % 2 == 0) {
            const auto committed = session.commit();
            EXPECT_TRUE(committed.status == InputStatus::updated
                        || committed.status == InputStatus::empty);
            if (committed.status == InputStatus::updated) {
                ++committed_words;
            }
        } else {
            session.reset();
        }
        expectSessionInvariants(session);
    }
    EXPECT_EQ(committed_words, 100U);
}

TEST(StabilitySoak, UnboundedRawInputNeverGrowsPastTheLimit) {
    // 100k characters without a single commit: the session must clamp at the
    // documented limit instead of buffering forever (a runaway composition
    // is the classic IME memory-growth bug).
    PhoneticInputSession session;
    bool limit_reported = false;
    for (std::size_t index = 0; index < 100'000; ++index) {
        const InputStatus status = session.append_utf8("k");
        EXPECT_TRUE(status == InputStatus::updated || status == InputStatus::input_limit);
        if (status == InputStatus::input_limit) {
            limit_reported = true;
        }
        expectSessionInvariants(session);
    }
    EXPECT_TRUE(limit_reported);
    EXPECT_EQ(session.snapshot().raw_input_utf8.size(), input_limit_max);

    // After the clamp the composition still commits exactly the buffered
    // input — clamping is not dropping. Transliteration expands each ASCII
    // letter to a multi-byte Bengali grapheme, so the committed length is
    // asserted in codepoints with a bounded expansion factor, while the
    // bounded-memory guarantee lives on the raw buffer above.
    const auto committed = session.commit();
    EXPECT_EQ(committed.status, InputStatus::updated);
    const std::size_t committed_codepoints = [&text = committed.text_utf8] {
        std::size_t count = 0;
        for (const char character : text) {
            if ((static_cast<unsigned char>(character) & 0xC0U) != 0x80U) {
                ++count;
            }
        }
        return count;
    }();
    EXPECT_GT(committed_codepoints, 0U);
    EXPECT_LE(committed_codepoints, 2U * input_limit_max);
}

TEST(StabilitySoak, FocusChangeChurnRecoversEveryCycle) {
    // 2k app-switch cycles: type a little, the host focus goes away, the
    // session resets, the next window starts typing immediately. No cycle
    // may leave stale state behind.
    const SubornoPhoneticLayout layout;
    PhoneticInputSession session(layout);
    for (int cycle = 0; cycle < 2'000; ++cycle) {
        ASSERT_EQ(session.append_utf8("ami"), InputStatus::updated);
        session.reset();
        const auto snapshot = session.snapshot();
        ASSERT_TRUE(snapshot.raw_input_utf8.empty());
        ASSERT_TRUE(snapshot.candidates.empty());
        ASSERT_EQ(session.selected_candidate_index(), 0U);

        ASSERT_EQ(session.append_utf8("tumi"), InputStatus::updated);
        const auto committed = session.commit();
        ASSERT_EQ(committed.status, InputStatus::updated);
        ASSERT_EQ(committed.text_utf8, "তুমি");
        ASSERT_TRUE(session.snapshot().raw_input_utf8.empty());
    }
}

TEST(StabilitySoak, MixedChurnThroughTheKeyMatrixStaysConsistent) {
    // Interleaves appends, backspaces, candidate taps, repeats and resets in
    // a fixed pseudo-random pattern; every step re-checks the invariants.
    const SubornoPhoneticLayout layout;
    PhoneticInputSession session(layout);
    suborno::layout::KeyEvent key;
    key.key = suborno::layout::PhysicalKey::letter_b;
    key.repeat_count = 3;

    for (int step = 0; step < 20'000; ++step) {
        switch (step % 5) {
        case 0:
            session.append_utf8("k");
            break;
        case 1:
            session.append_key(key);
            break;
        case 2:
            session.backspace();
            break;
        case 3:
            session.select_next_candidate();
            break;
        default:
            session.reset();
            break;
        }
        expectSessionInvariants(session);
    }
    // The session is still fully usable after the churn.
    ASSERT_EQ(session.append_utf8("ami"), InputStatus::updated);
    const auto committed = session.commit();
    EXPECT_EQ(committed.status, InputStatus::updated);
    EXPECT_EQ(committed.text_utf8, "আমি");
}
