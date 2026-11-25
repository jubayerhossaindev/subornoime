#pragma once

// Portable decision core of the Suborno standalone typing host
// ("Zero Bengali Language Pack Dependency" mode).
//
// The standalone host follows the hook-and-inject model used by popular
// standalone Bangla IMEs (low-level keyboard hook + retroactive Unicode
// injection) instead of the TSF text-service model, so it
// never depends on the Bengali language pack, the language bar, or Win+Space.
// This header holds ALL of the typing policy in a portable, unit-testable
// class: what to append, when to commit, how many characters to erase, and
// what to show in the floating preview. The Win32 layer (keyboard_hook.cpp)
// only translates hardware events into HookKeyInput values and executes the
// returned ControllerCommands with SendInput.
//
// Threading: the hook, the controller and the session all live on the host's
// UI thread. The controller owns no OS resources, which keeps the policy
// testable on any platform (see tests/unit/standalone_typing_controller_test.cpp).

#include <cstddef>
#include <cstdint>
#include <string>

#include "suborno/layout-api/input_session.hpp"

namespace suborno::platform::windows::standalone {

// A text-relevant key already classified by the Win32 layer (ToUnicodeEx).
// Only ASCII source characters are ever fed to the phonetic engine; other
// keys are reported as `other` and pass through untouched.
struct HookKeyInput final {
    enum class Kind : std::uint8_t {
        letter,
        digit,
        space,
        enter,
        punctuation,
        backspace,
        escape,
        other,
    };

    Kind kind = Kind::other;
    // The character the application would receive (ToUnicodeEx result) for
    // letter/digit/space/punctuation keys; '\0' otherwise.
    char character = '\0';
};

// Everything the Win32 layer must do for one key event. Absence of a command
// is meaningful: swallow=false means "let the physical key reach the app".
struct ControllerCommands final {
    // The hook must swallow the physical key (return 1) because it was
    // consumed by the commit flow; the layer re-injects the replacement.
    bool swallow = false;
    // Number of UTF-16 units the app already received for the live
    // composition and must be erased with injected backspaces before the
    // Bangla text is injected.
    std::size_t erase_units = 0;
    // Bangla text to inject (UTF-8). Empty = nothing to inject.
    std::string inject_utf8;
    // Floating preview update (UTF-8). Empty = hide the preview window.
    std::string preedit_utf8;
};

class TypingController final {
  public:
    // `session` must outlive the controller (the host recreates it on layout
    // changes and rebinds via set_session()).
    explicit TypingController(layout::InputSession& session) noexcept
        : session_(&session) {}

    // Rebinds the underlying session (layout switch in the settings file).
    // Any live composition is dropped, matching the TSF per-window switching.
    void set_session(layout::InputSession& session) noexcept {
        reset_composition();
        session_ = &session;
    }

    [[nodiscard]] bool enabled() const noexcept {
        return enabled_;
    }

    // Turning the engine off silently drops any live composition: the latin
    // text already shown in the app simply stays as the user typed it.
    void set_enabled(bool value) noexcept {
        if (!value) {
            reset_composition();
        }
        enabled_ = value;
    }

    [[nodiscard]] bool composing() const noexcept {
        return composing_;
    }

    // UTF-16 units of the live composition currently displayed by the app.
    [[nodiscard]] std::size_t displayed_units() const noexcept {
        return displayed_units_;
    }

    [[nodiscard]] ControllerCommands on_key(const HookKeyInput& key) {
        if (!enabled_) {
            ControllerCommands passthrough;
            return passthrough;
        }

        switch (key.kind) {
        case HookKeyInput::Kind::letter:
            return on_letter(key.character);
        case HookKeyInput::Kind::digit:
            return on_digit(key.character);
        case HookKeyInput::Kind::space:
        case HookKeyInput::Kind::enter:
        case HookKeyInput::Kind::punctuation:
            return on_commit_key(key.character);
        case HookKeyInput::Kind::backspace:
            return on_backspace();
        case HookKeyInput::Kind::escape:
            return on_escape();
        case HookKeyInput::Kind::other:
        default:
            // Modifier taps, F-keys, arrows, ... never touch the composition.
            ControllerCommands passthrough;
            if (composing_) {
                passthrough.preedit_utf8 = current_preedit();
            }
            return passthrough;
        }
    }

    // Window focus moved somewhere else: TSF would have ended the composition
    // through its edit cookie; the standalone host cancels it explicitly.
    [[nodiscard]] ControllerCommands on_focus_lost() {
        reset_composition();
        ControllerCommands passthrough;
        return passthrough;
    }

  private:
    [[nodiscard]] ControllerCommands on_letter(char character) {
        ControllerCommands commands;
        const char text[1] = {character};
        const layout::InputStatus status = session_->append_utf8(std::string_view(text, 1));
        switch (status) {
        case layout::InputStatus::updated:
        case layout::InputStatus::pending:
            // The latin letter flows through so the user always sees their
            // keystrokes; the retroactive replacement happens at commit.
            composing_ = true;
            displayed_units_ += 1;
            commands.preedit_utf8 = current_preedit();
            return commands;
        case layout::InputStatus::literal_passthrough:
        case layout::InputStatus::input_limit:
        case layout::InputStatus::invalid_utf8:
            // The engine can never map this word — stop composing and leave
            // what the user typed exactly as it is.
            reset_composition();
            return commands;
        case layout::InputStatus::empty:
        case layout::InputStatus::candidate_out_of_range:
        default:
            return commands;
        }
    }

    // v1 policy: digits never enter the composition. While a composition is
    // live a digit commits the word (Avro-style "type digit = finish word"),
    // the Win32 layer re-injects the digit itself afterwards.
    [[nodiscard]] ControllerCommands on_digit(char /*character*/) {
        if (!composing_) {
            ControllerCommands passthrough;
            return passthrough;
        }
        return commit_word();
    }

    [[nodiscard]] ControllerCommands on_commit_key(char /*character*/) {
        if (!composing_) {
            ControllerCommands passthrough;
            return passthrough;
        }
        return commit_word();
    }

    [[nodiscard]] ControllerCommands on_backspace() {
        ControllerCommands commands;
        if (!composing_) {
            return commands;
        }
        const layout::InputStatus status = session_->backspace();
        // The app already deleted one visible character with the physical
        // key; keep our counter in lockstep (one passthrough unit per key).
        if (displayed_units_ > 0) {
            displayed_units_ -= 1;
        }
        if (status == layout::InputStatus::empty) {
            composing_ = false;
            displayed_units_ = 0;
            commands.preedit_utf8.clear();
            return commands;
        }
        commands.preedit_utf8 = current_preedit();
        return commands;
    }

    [[nodiscard]] ControllerCommands on_escape() {
        ControllerCommands commands;
        if (!composing_) {
            return commands;
        }
        reset_composition();
        return commands;
    }

    // Swallows the commit key, erases the latin preview the app already
    // received, injects the committed Bangla text. The Win32 layer then
    // re-injects the swallowed key (space/enter as a virtual key, punctuation
    // as a Unicode character) so sentence flow is never interrupted.
    [[nodiscard]] ControllerCommands commit_word() {
        ControllerCommands commands;
        const layout::CommitResult committed = session_->commit();
        commands.swallow = true;
        commands.erase_units = displayed_units_;
        commands.inject_utf8 = committed.text_utf8;
        reset_composition();
        return commands;
    }

    void reset_composition() noexcept {
        session_->reset();
        composing_ = false;
        displayed_units_ = 0;
    }

    [[nodiscard]] std::string current_preedit() const {
        const layout::CompositionSnapshot snapshot = session_->snapshot();
        return snapshot.preedit_utf8;
    }

    layout::InputSession* session_ = nullptr;
    bool enabled_ = true;
    bool composing_ = false;
    std::size_t displayed_units_ = 0;
};

}  // namespace suborno::platform::windows::standalone
