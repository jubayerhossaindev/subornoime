#pragma once

// System-tray shell of the standalone typing host. The tray is the only
// visible surface of the zero-dependency mode: it shows whether Bangla
// typing is on, exposes the toggle, opens the existing Suborno settings app
// when the TSF IME is installed, and exits the host.

#include <Windows.h>
#include <functional>
#include <string>

namespace suborno::platform::windows::standalone {

class StandaloneTray final {
  public:
    struct Callbacks final {
        // The user asked to flip Bangla typing on/off (tray menu).
        std::function<void()> on_toggle;
        // The user asked for the full Suborno settings window.
        std::function<void()> on_open_settings;
        // The user asked the host to quit.
        std::function<void()> on_exit;
    };

    StandaloneTray() = default;
    ~StandaloneTray();

    StandaloneTray(const StandaloneTray&) = delete;
    StandaloneTray& operator=(const StandaloneTray&) = delete;

    bool create(HINSTANCE instance, const std::wstring& tooltip, const Callbacks& callbacks);
    // Reflects the engine state in the tooltip and menu checkmark.
    void set_enabled_state(bool bangla_on) noexcept;
    void destroy() noexcept;

  private:
    static LRESULT CALLBACK proc_thunk(HWND window,
                                       UINT message,
                                       WPARAM wparam,
                                       LPARAM lparam) noexcept;
    LRESULT proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) noexcept;
    void show_context_menu() noexcept;
    void add_or_modify_icon(DWORD message) noexcept;

    HWND window_ = nullptr;
    HINSTANCE instance_ = nullptr;
    HICON icon_ = nullptr;
    NOTIFYICONDATAW notify_ = {};
    bool icon_added_ = false;
    bool bangla_on_ = true;
    UINT taskbar_created_message_ = 0;
    std::wstring tooltip_;
    Callbacks callbacks_;
};

}  // namespace suborno::platform::windows::standalone
