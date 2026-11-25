#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "suborno/layout-api/candidate.hpp"
#include "suborno/layout-api/key_event.hpp"

namespace suborno::layout {

enum class InputStatus : std::uint8_t {
    updated,
    pending,
    literal_passthrough,
    invalid_utf8,
    input_limit,
    candidate_out_of_range,
    empty,
};

struct CompositionSnapshot {
    std::string raw_input_utf8{};
    std::string preedit_utf8{};
    std::vector<Candidate> candidates{};
    std::size_t selected_candidate_index = 0;
    InputStatus status = InputStatus::empty;
};

struct CommitResult {
    std::string text_utf8{};
    InputStatus status = InputStatus::empty;
};

struct InputSessionOptions {
    std::size_t candidate_limit = 16;
    bool learning_enabled = true;
    // Normalized form of the word committed immediately before this session

    // started (empty = none). Together with next_word_prediction_enabled it

    // feeds the candidate engine's next-word prediction stage; the word is

    // only forwarded when prediction is enabled, so old callers that never

    /// set it keep the exact pre-prediction candidate list.

    std::string previous_word;
    bool next_word_prediction_enabled = false;
};

class InputSession {
  public:
    static constexpr std::size_t max_input_code_points = 256;

    virtual ~InputSession() = default;

    InputSession(const InputSession&) = delete;
    InputSession& operator=(const InputSession&) = delete;
    InputSession(InputSession&&) = delete;
    InputSession& operator=(InputSession&&) = delete;

    InputSession() = default;

    virtual InputStatus append_utf8(std::string_view input) = 0;

    // The default maps a physical key event to its US-layout ASCII character.

    // Fixed-layout sessions may override this to use the physical key directly.

    //

    // Precedence: a non-empty logical_text_utf8 always wins (a keyboard

    // layout that translated the key provides the Bangla text even without

    // setting logical_text_available); the flag only matters when the layout

    /// returns no text, in which case we fall back to the physical-key ASCII.

    virtual InputStatus append_key(const KeyEvent& event) {
        if (event.modifiers.control || event.modifiers.alt || event.modifiers.windows
            || !event.key_down) {
            return InputStatus::empty;
        }
        if (event.logical_text_available) {
            return event.logical_text_utf8.empty() ? InputStatus::empty
                                                   : append_utf8(event.logical_text_utf8);
        }
        if (!event.logical_text_utf8.empty()) {
            return append_utf8(event.logical_text_utf8);
        }

        const std::optional<char> character = key_event_to_ascii(event);
        if (!character.has_value()) {
            return InputStatus::empty;
        }
        const char value = *character;

        // Windows sends WM_KEYDOWN with repeat_count >= 1 while a key is held
        // down. Every repeat is a real keystroke the user expects to see, so
        // the character is appended once per repeat; capping the loop at the
        // session limit keeps a stuck key from exceeding the buffer. The
        // fixed layouts (national, probhat) apply the same rule to their
        // composed outputs.
        const std::size_t repeat_count = event.repeat_count == 0 ? 1 : event.repeat_count;
        if (repeat_count > max_input_code_points) {
            return InputStatus::input_limit;
        }
        const std::string_view text(&value, 1);
        InputStatus status = InputStatus::empty;
        for (std::size_t index = 0; index < repeat_count; ++index) {
            status = append_utf8(text);
            if (status == InputStatus::input_limit || status == InputStatus::invalid_utf8) {
                break;
            }
        }
        return status;
    }

    virtual InputStatus backspace() = 0;
    virtual CommitResult commit() = 0;
    virtual CommitResult commit_candidate(std::size_t index) = 0;
    virtual bool select_candidate(std::size_t index) = 0;
    virtual bool select_next_candidate() = 0;
    virtual bool select_previous_candidate() noexcept = 0;
    [[nodiscard]] virtual std::size_t selected_candidate_index() const noexcept = 0;
    virtual void reset() noexcept = 0;
    [[nodiscard]] virtual CompositionSnapshot snapshot() const = 0;
};

}  // namespace suborno::layout
