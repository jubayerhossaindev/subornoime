// §1 Core IME functionality: the full key-event matrix. Every editing,
// navigation and modifier path is exercised through a real phonetic session;
// each test asserts that the composition state stays consistent, that no
// keystroke is lost (auto-repeat) and that no unexpected character appears.
#include <cstdint>
#include <gtest/gtest.h>
#include <string>

#include "suborno/layout-api/input_session.hpp"
#include "suborno/layout-api/key_event.hpp"
#include "suborno/layouts/phonetic/phonetic_input_session.hpp"

namespace {

using suborno::layout::InputStatus;
using suborno::layout::KeyEvent;
using suborno::layout::KeyModifiers;
using suborno::layout::PhysicalKey;
using suborno::layouts::PhoneticInputSession;

KeyEvent make_key(PhysicalKey key,
                  const KeyModifiers& modifiers = {},
                  std::uint16_t repeat_count = 1,
                  bool key_down = true) {
    KeyEvent event;
    event.key = key;
    event.modifiers = modifiers;
    event.repeat_count = repeat_count;
    event.key_down = key_down;
    return event;
}

constexpr PhysicalKey navigation_keys[] = {PhysicalKey::arrow_left,
                                           PhysicalKey::arrow_up,
                                           PhysicalKey::arrow_right,
                                           PhysicalKey::arrow_down,
                                           PhysicalKey::home,
                                           PhysicalKey::end,
                                           PhysicalKey::page_up,
                                           PhysicalKey::page_down};

constexpr PhysicalKey action_keys[] = {
    PhysicalKey::return_key, PhysicalKey::escape, PhysicalKey::tab};

constexpr PhysicalKey editing_keys[] = {PhysicalKey::backspace_key, PhysicalKey::delete_key};

constexpr PhysicalKey all_non_text_keys[] = {PhysicalKey::tab,
                                             PhysicalKey::return_key,
                                             PhysicalKey::escape,
                                             PhysicalKey::backspace_key,
                                             PhysicalKey::delete_key,
                                             PhysicalKey::insert_key,
                                             PhysicalKey::home,
                                             PhysicalKey::end,
                                             PhysicalKey::page_up,
                                             PhysicalKey::page_down,
                                             PhysicalKey::arrow_left,
                                             PhysicalKey::arrow_up,
                                             PhysicalKey::arrow_right,
                                             PhysicalKey::arrow_down};

constexpr bool is_suppressor(const KeyModifiers& modifiers) noexcept {
    return modifiers.control || modifiers.alt || modifiers.windows;
}

}  // namespace

TEST(KeyEventMatrix, EditingNavigationAndActionKeysAreClassified) {
    for (const PhysicalKey key : editing_keys) {
        EXPECT_TRUE(suborno::layout::is_editing_key(key));
    }
    for (const PhysicalKey key : navigation_keys) {
        EXPECT_TRUE(suborno::layout::is_navigation_key(key));
    }
    for (const PhysicalKey key : action_keys) {
        EXPECT_TRUE(suborno::layout::is_action_key(key));
    }
    // Every classification is disjoint: an arrow is never an editing key and
    // so on, so the TSF routing switch cannot double-handle a key.
    for (const PhysicalKey key : navigation_keys) {
        EXPECT_FALSE(suborno::layout::is_editing_key(key));
        EXPECT_FALSE(suborno::layout::is_action_key(key));
    }
    for (const PhysicalKey key : action_keys) {
        EXPECT_FALSE(suborno::layout::is_editing_key(key));
        EXPECT_FALSE(suborno::layout::is_navigation_key(key));
    }
    EXPECT_FALSE(suborno::layout::is_editing_key(PhysicalKey::letter_a));
    EXPECT_FALSE(suborno::layout::is_navigation_key(PhysicalKey::letter_a));
}

TEST(KeyEventMatrix, NonTextKeysNeverMapToAscii) {
    KeyModifiers shifted;
    shifted.shift = true;
    for (const PhysicalKey key : all_non_text_keys) {
        EXPECT_FALSE(suborno::layout::key_event_to_ascii(make_key(key)).has_value());
        EXPECT_FALSE(suborno::layout::key_event_to_ascii(make_key(key, shifted)).has_value());
    }
}

TEST(KeyEventMatrix, AutoRepeatTypesOneCharacterPerRepeat) {
    // Holding a key down produces WM_KEYDOWN with repeat_count >= 2. Every
    // repeat is a real keystroke; dropping them would silently lose input.
    // The committed text must be identical to the text-driven path for the
    // same raw input — key-driven typing is never weaker than paste.
    PhoneticInputSession session;
    EXPECT_EQ(session.append_key(make_key(PhysicalKey::letter_a, {}, 5)), InputStatus::updated);
    EXPECT_EQ(session.snapshot().raw_input_utf8, "aaaaa");

    PhoneticInputSession text_path;
    ASSERT_EQ(text_path.append_utf8("aaaaa"), InputStatus::updated);
    const auto committed = session.commit();
    EXPECT_EQ(committed.status, InputStatus::updated);
    EXPECT_EQ(committed.text_utf8, text_path.snapshot().preedit_utf8);
}

TEST(KeyEventMatrix, AutoRepeatZeroMeansOneKeystroke) {
    PhoneticInputSession session;
    EXPECT_EQ(session.append_key(make_key(PhysicalKey::letter_m, {}, 0)), InputStatus::updated);
    EXPECT_EQ(session.snapshot().raw_input_utf8, "m");
}

TEST(KeyEventMatrix, AutoRepeatAboveSessionLimitIsRejectedAtomically) {
    PhoneticInputSession session;
    ASSERT_EQ(session.append_utf8("am"), InputStatus::updated);
    // A stuck key must not overflow the composition buffer: the whole event
    // is rejected and the previous state survives untouched.
    EXPECT_EQ(session.append_key(make_key(
                  PhysicalKey::letter_i,
                  {},
                  static_cast<std::uint16_t>(PhoneticInputSession::max_input_code_points + 1U))),
              InputStatus::input_limit);
    EXPECT_EQ(session.snapshot().raw_input_utf8, "am");
}

TEST(KeyEventMatrix, ModifierMatrixSuppressesKeysAndLeavesCompositionUntouched) {
    const KeyModifiers suppressors[] = {
        [] {
            KeyModifiers modifiers;
            modifiers.control = true;
            return modifiers;
        }(),
        [] {
            KeyModifiers modifiers;
            modifiers.alt = true;
            return modifiers;
        }(),
        [] {
            KeyModifiers modifiers;
            modifiers.windows = true;
            return modifiers;
        }(),
    };

    for (const KeyModifiers& modifiers : suppressors) {
        PhoneticInputSession session;
        ASSERT_EQ(session.append_utf8("ami"), InputStatus::updated);
        const auto before = session.snapshot();
        EXPECT_EQ(session.append_key(make_key(PhysicalKey::letter_a, modifiers)),
                  InputStatus::empty);
        const auto after = session.snapshot();
        EXPECT_EQ(after.raw_input_utf8, before.raw_input_utf8);
        EXPECT_EQ(after.preedit_utf8, before.preedit_utf8);
        EXPECT_EQ(after.candidates.size(), before.candidates.size());
        const auto committed = session.commit();
        EXPECT_EQ(committed.status, InputStatus::updated);
        EXPECT_EQ(committed.text_utf8, "আমি");
    }
}

TEST(KeyEventMatrix, KeyUpNeverDuplicatesCharacters) {
    PhoneticInputSession session;
    // WM_KEYUP (key_down=false) with repeat_count=3: released keys must not
    // inject text, no matter how the repeat counter reports.
    EXPECT_EQ(session.append_key(make_key(PhysicalKey::letter_k, {}, 3, false)),
              InputStatus::empty);
    EXPECT_EQ(session.snapshot().raw_input_utf8, "");
}

TEST(KeyEventMatrix, ShiftAndCapsLockTransformAsciiPredictably) {
    PhoneticInputSession session;
    KeyModifiers shifted;
    shifted.shift = true;
    EXPECT_EQ(session.append_key(make_key(PhysicalKey::letter_a, shifted)), InputStatus::updated);
    EXPECT_EQ(session.snapshot().raw_input_utf8, "A");

    session.reset();
    KeyModifiers caps;
    caps.caps_lock = true;
    EXPECT_EQ(session.append_key(make_key(PhysicalKey::letter_a, caps)), InputStatus::updated);
    EXPECT_EQ(session.snapshot().raw_input_utf8, "A");

    session.reset();
    KeyModifiers shift_and_caps;
    shift_and_caps.shift = true;
    shift_and_caps.caps_lock = true;
    EXPECT_EQ(session.append_key(make_key(PhysicalKey::letter_a, shift_and_caps)),
              InputStatus::updated);
    EXPECT_EQ(session.snapshot().raw_input_utf8, "a");

    session.reset();
    EXPECT_EQ(session.append_key(make_key(PhysicalKey::digit_1, shifted)),
              InputStatus::literal_passthrough);
    EXPECT_EQ(session.snapshot().raw_input_utf8, "!");
}

TEST(KeyEventMatrix, NavigationAndActionKeysDuringCompositionPreserveState) {
    for (const PhysicalKey key : all_non_text_keys) {
        if (suborno::layout::is_editing_key(key)) {
            continue;  // Backspace edits the composition; covered below.
        }
        PhoneticInputSession session;
        ASSERT_EQ(session.append_utf8("ami"), InputStatus::updated);
        const auto before = session.snapshot();
        EXPECT_EQ(session.append_key(make_key(key)), InputStatus::empty);
        const auto after = session.snapshot();
        EXPECT_EQ(after.raw_input_utf8, before.raw_input_utf8) << "key " << static_cast<int>(key);
        EXPECT_EQ(after.preedit_utf8, before.preedit_utf8) << "key " << static_cast<int>(key);
        ASSERT_EQ(after.candidates.size(), before.candidates.size())
            << "key " << static_cast<int>(key);
        const auto committed = session.commit();
        EXPECT_EQ(committed.status, InputStatus::updated);
        EXPECT_EQ(committed.text_utf8, "আমি") << "key " << static_cast<int>(key);
    }
}

TEST(KeyEventMatrix, BackspaceRemovesRawInputThenReportsEmpty) {
    // The backspace contract: exactly one grapheme per call while raw input
    // exists, terminating at an empty composition. The reported status tracks
    // the parse state of what remains; clearing the last grapheme reports
    // empty, the same status a fresh session uses.
    const auto is_backspace_status = [](InputStatus status) {
        return status == InputStatus::updated || status == InputStatus::pending
               || status == InputStatus::literal_passthrough || status == InputStatus::empty;
    };

    PhoneticInputSession session;
    ASSERT_EQ(session.append_utf8("ami"), InputStatus::updated);

    EXPECT_TRUE(is_backspace_status(session.backspace()));
    EXPECT_EQ(session.snapshot().raw_input_utf8, "am");
    EXPECT_TRUE(is_backspace_status(session.backspace()));
    EXPECT_EQ(session.snapshot().raw_input_utf8, "a");
    EXPECT_TRUE(is_backspace_status(session.backspace()));
    EXPECT_EQ(session.snapshot().raw_input_utf8, "");
    EXPECT_EQ(session.backspace(), InputStatus::empty);
    EXPECT_EQ(session.snapshot().raw_input_utf8, "");
}

TEST(KeyEventMatrix, EveryPhysicalKeyPassesThroughWithoutCorruption) {
    // Exhaustive sweep across the whole PhysicalKey domain: the session must
    // never crash, never produce non-ASCII raw input from a physical key and
    // always end in a consistent state.
    for (std::uint16_t value = 0; value <= 200; ++value) {
        const auto key = static_cast<PhysicalKey>(static_cast<std::uint8_t>(value));
        PhoneticInputSession session;
        const InputStatus status = session.append_key(make_key(key));
        const auto snapshot = session.snapshot();
        for (const char character : snapshot.raw_input_utf8) {
            const auto byte = static_cast<unsigned char>(character);
            EXPECT_GE(byte, 32U) << "control byte for key " << value;
            EXPECT_LE(byte, 126U) << "non-ascii byte for key " << value;
        }
        const auto committed = session.commit();
        // Letters update the composition, punctuation passes through as
        // literals, some symbols (backslash) leave the parser pending, and
        // anything unhandled must report empty — never a crash or garbage.
        EXPECT_TRUE(status == InputStatus::empty || status == InputStatus::updated
                    || status == InputStatus::input_limit
                    || status == InputStatus::literal_passthrough || status == InputStatus::pending)
            << "key " << value;
        // The text survives with a passthrough status instead of being
        // absorbed or dropped.
        EXPECT_TRUE(committed.text_utf8.empty() || committed.status == InputStatus::updated
                    || committed.status == InputStatus::literal_passthrough
                    || committed.status == InputStatus::pending)
            << "key " << value;
    }
}
