#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "suborno/layout-api/candidate.hpp"
#include "suborno/layout-api/input_session.hpp"

namespace suborno::dictionary {
class Dictionary;
}

namespace suborno::layout {

enum class LayoutKind : std::uint8_t {
    phonetic,
    fixed,
    compositional,
    // English/passthrough: the text service never eats keys, so the host
    // application receives the raw Latin keyboard output unchanged.
    passthrough,
};

class InputLayout {
  public:
    virtual ~InputLayout() = default;

    InputLayout(const InputLayout&) = delete;
    InputLayout& operator=(const InputLayout&) = delete;
    InputLayout(InputLayout&&) = delete;
    InputLayout& operator=(InputLayout&&) = delete;

    InputLayout() = default;

    [[nodiscard]] virtual LayoutKind kind() const noexcept = 0;
    [[nodiscard]] virtual std::string transliterate(const std::string& input) const = 0;
    [[nodiscard]] virtual std::vector<Candidate> generate_candidates(
        const std::string& input) const = 0;
    virtual std::unique_ptr<InputSession> create_session(dictionary::Dictionary* dictionary,
                                                         InputSessionOptions options) const = 0;
};

/// Stateless layouts rebuild their output from the complete key sequence.
/// Stateful compositional layouts may implement their own InputSession.
class StatelessInputLayout : public InputLayout {
  public:
    std::unique_ptr<InputSession> create_session(dictionary::Dictionary* dictionary,
                                                 InputSessionOptions options) const override;
};

}  // namespace suborno::layout
