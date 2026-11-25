// Unit tests for the standalone (zero-dependency) typing policy core.
//
// The TypingController is deliberately OS-independent: the Win32 keyboard
// hook layer translates hardware events into HookKeyInput values and executes
// the returned ControllerCommands with SendInput. These tests pin the whole
// decision policy with a scripted session, so the retroactive-replacement
// behaviour of the standalone host is verified on every platform (Ubuntu CI
// and the MSVC v143 build) without any Windows dependency.

#include <gtest/gtest.h>
#include <string>
#include <string_view>

#include "platform/windows/standalone/typing_controller.hpp"

namespace {

using suborno::layout::CompositionSnapshot;
using suborno::layout::CommitResult;
using suborno::layout::InputSession;
using suborno::layout::InputStatus;
using suborno::platform::windows::standalone::ControllerCommands;
using suborno::platform::windows::standalone::HookKeyInput;
using suborno::platform::windows::standalone::TypingController;

// Scripted session: every scripted decision is recorded so the test can
// assert exactly what the controller asked the engine to do.
class ScriptedSession final : public InputSession {
  public:
    InputStatus append_utf8(std::string_view input) override {
        raw_input_.append(input);
        return append_status_;
    }

    InputStatus backspace() override {
        if (raw_input_.empty()) {
            return InputStatus::empty;
        }
        raw_input_.pop_back();
        return raw_input_.empty() ? InputStatus::empty : backspace_status_;
    }

    CommitResult commit() override {
        commit_calls_ += 1;
        const CommitResult result = commit_result_;
        raw_input_.clear();
        return result;
    }

    CommitResult commit_candidate(std::size_t) override {
        return commit();
    }
    bool select_candidate(std::size_t) override {
        return false;
    }
    bool select_next_candidate() override {
        return false;
    }
    bool select_previous_candidate() noexcept override {
        return false;
    }
    std::size_t selected_candidate_index() const noexcept override {
        return 0;
    }
    void reset() noexcept override {
        reset_calls_ += 1;
    }

    CompositionSnapshot snapshot() const override {
        CompositionSnapshot value;
        value.raw_input_utf8 = raw_input_;
        value.preedit_utf8 = preedit_;
        return value;
    }

    std::string raw_input_;
    std::string preedit_;
    InputStatus append_status_ = InputStatus::updated;
    InputStatus backspace_status_ = InputStatus::updated;
    CommitResult commit_result_{};
    std::size_t commit_calls_ = 0;
    std::size_t reset_calls_ = 0;
};

HookKeyInput letter(char character) {
    HookKeyInput key;
    key.kind = HookKeyInput::Kind::letter;
    key.character = character;
    return key;
}

HookKeyInput digit(char character) {
    HookKeyInput key;
    key.kind = HookKeyInput::Kind::digit;
    key.character = character;
    return key;
}

HookKeyInput kind(HookKeyInput::Kind value, char character = '\0') {
    HookKeyInput key;
    key.kind = value;
    key.character = character;
    return key;
}

TEST(StandaloneTypingController, LettersPassThroughAndBuildAPreview) {
    ScriptedSession session;
    session.preedit_ = "\xE0\xA6\x86";  // আ
    TypingController controller(session);

    const ControllerCommands first = controller.on_key(letter('a'));
    EXPECT_FALSE(first.swallow);
    EXPECT_EQ(first.erase_units, 0U);
    EXPECT_TRUE(first.inject_utf8.empty());
    EXPECT_EQ(first.preedit_utf8, "\xE0\xA6\x86");
    EXPECT_TRUE(controller.composing());
    EXPECT_EQ(controller.displayed_units(), 1U);
    EXPECT_EQ(session.raw_input_, "a");

    const ControllerCommands second = controller.on_key(letter('m'));
    EXPECT_FALSE(second.swallow);
    EXPECT_EQ(controller.displayed_units(), 2U);
    EXPECT_EQ(session.raw_input_, "am");
}

TEST(StandaloneTypingController, SpaceCommitsWithRetroactiveReplacement) {
    ScriptedSession session;
    session.preedit_ = "\xE0\xA6\x86\xE0\xA6\xAE\xE0\xA6\xBF";  // আমি
    session.commit_result_.text_utf8 = "\xE0\xA6\x86\xE0\xA6\xAE\xE0\xA6\xBF";
    TypingController controller(session);

    ASSERT_EQ(controller.on_key(letter('a')).swallow, false);
    ASSERT_EQ(controller.on_key(letter('m')).swallow, false);
    ASSERT_EQ(controller.on_key(letter('i')).swallow, false);
    EXPECT_EQ(controller.displayed_units(), 3U);

    const ControllerCommands command = controller.on_key(kind(HookKeyInput::Kind::space, ' '));
    EXPECT_TRUE(command.swallow);
    EXPECT_EQ(command.erase_units, 3U);
    EXPECT_EQ(command.inject_utf8, "\xE0\xA6\x86\xE0\xA6\xAE\xE0\xA6\xBF");
    EXPECT_EQ(command.preedit_utf8, "");
    EXPECT_FALSE(controller.composing());
    EXPECT_EQ(controller.displayed_units(), 0U);
    EXPECT_EQ(session.commit_calls_, 1U);
}

TEST(StandaloneTypingController, EnterAndPunctuationAlsoCommit) {
    ScriptedSession session;
    session.commit_result_.text_utf8 = "\xE0\xA6\xAC";  // ব
    TypingController controller(session);

    ASSERT_EQ(controller.on_key(letter('b')).swallow, false);
    const ControllerCommands enter = controller.on_key(kind(HookKeyInput::Kind::enter));
    EXPECT_TRUE(enter.swallow);
    EXPECT_EQ(enter.erase_units, 1U);
    EXPECT_EQ(enter.inject_utf8, "\xE0\xA6\xAC");

    ASSERT_EQ(controller.on_key(letter('b')).swallow, false);
    const ControllerCommands punct = controller.on_key(kind(HookKeyInput::Kind::punctuation, '.'));
    EXPECT_TRUE(punct.swallow);
    EXPECT_EQ(punct.erase_units, 1U);
    EXPECT_EQ(punct.inject_utf8, "\xE0\xA6\xAC");
}

TEST(StandaloneTypingController, CommitKeyWithoutCompositionPassesThrough) {
    ScriptedSession session;
    TypingController controller(session);

    const ControllerCommands command = controller.on_key(kind(HookKeyInput::Kind::space, ' '));
    EXPECT_FALSE(command.swallow);
    EXPECT_EQ(command.erase_units, 0U);
    EXPECT_TRUE(command.inject_utf8.empty());
    EXPECT_EQ(session.commit_calls_, 0U);
}

TEST(StandaloneTypingController, BackspaceTrimsPreviewThenEndsComposition) {
    ScriptedSession session;
    session.preedit_ = "\xE0\xA6\x86";
    TypingController controller(session);

    ASSERT_EQ(controller.on_key(letter('a')).swallow, false);
    ASSERT_EQ(controller.on_key(letter('m')).swallow, false);

    const ControllerCommands first = controller.on_key(kind(HookKeyInput::Kind::backspace));
    EXPECT_FALSE(first.swallow);
    EXPECT_EQ(controller.displayed_units(), 1U);
    EXPECT_EQ(first.preedit_utf8, "\xE0\xA6\x86");
    EXPECT_TRUE(controller.composing());

    const ControllerCommands second = controller.on_key(kind(HookKeyInput::Kind::backspace));
    EXPECT_FALSE(second.swallow);
    EXPECT_FALSE(controller.composing());
    EXPECT_EQ(controller.displayed_units(), 0U);
    EXPECT_EQ(second.preedit_utf8, "");
}

TEST(StandaloneTypingController, EscapeCancelsTheComposition) {
    ScriptedSession session;
    TypingController controller(session);

    ASSERT_EQ(controller.on_key(letter('a')).swallow, false);
    const ControllerCommands command = controller.on_key(kind(HookKeyInput::Kind::escape));
    EXPECT_FALSE(command.swallow);
    EXPECT_FALSE(controller.composing());
    EXPECT_EQ(session.reset_calls_, 1U);
    // The escape itself still reaches the app.
    EXPECT_EQ(command.preedit_utf8, "");
}

TEST(StandaloneTypingController, LiteralPassthroughEndsCompositionSilently) {
    ScriptedSession session;
    session.append_status_ = InputStatus::literal_passthrough;
    TypingController controller(session);

    const ControllerCommands command = controller.on_key(letter('5'));
    EXPECT_FALSE(command.swallow);
    EXPECT_FALSE(controller.composing());
    EXPECT_EQ(controller.displayed_units(), 0U);
    EXPECT_TRUE(command.preedit_utf8.empty());
}

TEST(StandaloneTypingController, InputLimitEndsTheComposition) {
    ScriptedSession session;
    session.append_status_ = InputStatus::input_limit;
    TypingController controller(session);

    const ControllerCommands command = controller.on_key(letter('x'));
    EXPECT_FALSE(command.swallow);
    EXPECT_FALSE(controller.composing());
    EXPECT_EQ(session.reset_calls_, 1U);
}

TEST(StandaloneTypingController, DigitCommitsTheLiveWord) {
    ScriptedSession session;
    session.commit_result_.text_utf8 = "\xE0\xA6\x95";  // ক
    TypingController controller(session);

    ASSERT_EQ(controller.on_key(letter('k')).swallow, false);
    const ControllerCommands command = controller.on_key(digit('2'));
    EXPECT_TRUE(command.swallow);
    EXPECT_EQ(command.erase_units, 1U);
    EXPECT_EQ(command.inject_utf8, "\xE0\xA6\x95");
    EXPECT_FALSE(controller.composing());
    // The Win32 layer re-injects the digit itself; the controller never
    // feeds digits into the phonetic session.
    EXPECT_EQ(session.raw_input_, "");
}

TEST(StandaloneTypingController, DigitWithoutCompositionPassesThrough) {
    ScriptedSession session;
    TypingController controller(session);

    const ControllerCommands command = controller.on_key(digit('7'));
    EXPECT_FALSE(command.swallow);
    EXPECT_EQ(session.commit_calls_, 0U);
}

TEST(StandaloneTypingController, OtherKeysNeverTouchTheComposition) {
    ScriptedSession session;
    session.preedit_ = "\xE0\xA6\x86";
    TypingController controller(session);

    ASSERT_EQ(controller.on_key(letter('a')).swallow, false);
    const ControllerCommands command = controller.on_key(kind(HookKeyInput::Kind::other));
    EXPECT_FALSE(command.swallow);
    EXPECT_TRUE(controller.composing());
    EXPECT_EQ(controller.displayed_units(), 1U);
    EXPECT_EQ(command.preedit_utf8, "\xE0\xA6\x86");
}

TEST(StandaloneTypingController, FocusLostCancelsTheComposition) {
    ScriptedSession session;
    TypingController controller(session);

    ASSERT_EQ(controller.on_key(letter('a')).swallow, false);
    const ControllerCommands command = controller.on_focus_lost();
    EXPECT_FALSE(command.swallow);
    EXPECT_FALSE(controller.composing());
    EXPECT_EQ(session.reset_calls_, 1U);

    // A later commit key must pass through untouched.
    const ControllerCommands after = controller.on_key(kind(HookKeyInput::Kind::space, ' '));
    EXPECT_FALSE(after.swallow);
}

TEST(StandaloneTypingController, DisabledControllerIsPurePassthrough) {
    ScriptedSession session;
    TypingController controller(session);

    controller.set_enabled(false);
    EXPECT_FALSE(controller.enabled());
    const ControllerCommands command = controller.on_key(letter('a'));
    EXPECT_FALSE(command.swallow);
    EXPECT_TRUE(command.preedit_utf8.empty());
    EXPECT_EQ(session.raw_input_, "");

    controller.set_enabled(true);
    const ControllerCommands resumed = controller.on_key(letter('a'));
    EXPECT_FALSE(resumed.swallow);
    EXPECT_TRUE(controller.composing());
    EXPECT_EQ(session.raw_input_, "a");
}

TEST(StandaloneTypingController, TogglingOffMidCompositionDropsThePreview) {
    ScriptedSession session;
    TypingController controller(session);

    ASSERT_EQ(controller.on_key(letter('a')).swallow, false);
    controller.set_enabled(false);
    EXPECT_FALSE(controller.composing());
    EXPECT_EQ(controller.displayed_units(), 0U);
    EXPECT_EQ(session.reset_calls_, 1U);
}

TEST(StandaloneTypingController, SessionRebindingDropsTheLiveComposition) {
    ScriptedSession first_session;
    ScriptedSession second_session;
    TypingController controller(first_session);

    ASSERT_EQ(controller.on_key(letter('a')).swallow, false);
    controller.set_session(second_session);
    EXPECT_FALSE(controller.composing());
    EXPECT_EQ(first_session.reset_calls_, 1U);

    const ControllerCommands command = controller.on_key(letter('b'));
    EXPECT_EQ(second_session.raw_input_, "b");
    EXPECT_TRUE(controller.composing());
}

}  // namespace
