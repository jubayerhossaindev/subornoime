#pragma once

#include <cstdint>
#include <string>

namespace suborno::platform::linux {

enum class LinuxAdapterStatus : std::uint8_t {
    unavailable,
    available,
};

class LinuxAdapter {
  public:
    virtual ~LinuxAdapter() = default;

    LinuxAdapter(const LinuxAdapter&) = delete;
    LinuxAdapter& operator=(const LinuxAdapter&) = delete;
    LinuxAdapter(LinuxAdapter&&) = delete;
    LinuxAdapter& operator=(LinuxAdapter&&) = delete;

    LinuxAdapter() = default;
    [[nodiscard]] virtual LinuxAdapterStatus status() const noexcept = 0;
};

class IBusAdapter final : public LinuxAdapter {
  public:
    [[nodiscard]] LinuxAdapterStatus status() const noexcept override;
};

class Fcitx5Adapter final : public LinuxAdapter {
  public:
    [[nodiscard]] LinuxAdapterStatus status() const noexcept override;
};

}  // namespace suborno::platform::linux
