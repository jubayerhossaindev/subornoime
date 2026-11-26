#pragma once

// WH_KEYBOARD_LL front end of the standalone typing host.

//

// The hook translates raw low-level events into HookKeyInput values, feeds

// the TypingController, and executes the returned ControllerCommands with

// SendInput (erase latin + inject Bangla + re-send the swallowed commit key).

// Design guards that matter:

//   * LLKHF_INJECTED events are ignored — our own SendInput traffic must

//     never re-enter the policy (feedback loop).

//   * Ctrl/Alt/Win chords always pass through and cancel any live

//     composition, so shortcuts like Ctrl+C behave exactly as without us.

//   * The default toggle (Ctrl+Alt+B) is swallowed; tray menu reflects it.

#include <Windows.h>
#include <functional>

#include "typing_controller.hpp"

namespace suborno::platform::windows::standalone {

class PreviewWindow;

struct KeyboardHookConfig final {
    // Default Ctrl+Alt+B — chosen to avoid the classic conflicts: F12 is the

    // legacy default but collides with browser dev tools, Ctrl+Space collides

    // with editor autocomplete (VS IntelliSense), Alt+Shift/Win+Space are

    // reserved by Windows input-language switching.

    std::uint16_t toggle_vk = 0x42;  // 'B'

    bool toggle_ctrl = true;
    bool toggle_alt = true;
    bool toggle_shift = false;
    bool toggle_win = false;
};

class KeyboardHook final {
  public:
    struct Deps final {
        TypingController* controller = nullptr;
        PreviewWindow* preview = nullptr;
        bool preview_enabled = true;
        // Called after every toggle so the tray can repaint its state.

        std::function<void(bool)> on_engine_toggled;
    };

    KeyboardHook() = default;
    ~KeyboardHook();

    KeyboardHook(const KeyboardHook&) = delete;
    KeyboardHook& operator=(const KeyboardHook&) = delete;

    bool install(const KeyboardHookConfig& config, const Deps& deps) noexcept;
    void uninstall() noexcept;

  private:
    static constexpr LRESULT swallow_key = 1;
    static constexpr LRESULT pass_key = 0;

    static LRESULT CALLBACK low_level_proc(int code, WPARAM wparam, LPARAM lparam) noexcept;
    LRESULT handle_key_down(const KBDLLHOOKSTRUCT& info) noexcept;
    [[nodiscard]] HookKeyInput classify(std::uint16_t vk, std::uint16_t scan_code) const noexcept;

    HHOOK hook_ = nullptr;
    KeyboardHookConfig config_{};
    Deps deps_{};
    // LL hooks carry no context pointer; this host is single-instance, so one

    // static back-pointer is the accepted wiring (see standalone_main.cpp for

    // the single-instance mutex).

    static KeyboardHook* active_;
};

}  // namespace suborno::platform::windows::standalone
