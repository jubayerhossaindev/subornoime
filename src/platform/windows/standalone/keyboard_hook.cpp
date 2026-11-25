#include "keyboard_hook.hpp"

#include "preview_window.hpp"
#include "unicode_injector.hpp"

namespace suborno::platform::windows::standalone {

KeyboardHook* KeyboardHook::active_ = nullptr;

KeyboardHook::~KeyboardHook() {
    uninstall();
}

bool KeyboardHook::install(const KeyboardHookConfig& config, const Deps& deps) noexcept {
    if (deps.controller == nullptr || active_ != nullptr) {
        return false;
    }
    config_ = config;
    deps_ = deps;
    hook_ = ::SetWindowsHookExW(WH_KEYBOARD_LL, &KeyboardHook::low_level_proc, nullptr, 0);
    if (hook_ == nullptr) {
        return false;
    }
    active_ = this;
    return true;
}

void KeyboardHook::uninstall() noexcept {
    if (hook_ != nullptr) {
        ::UnhookWindowsHookEx(hook_);
        hook_ = nullptr;
    }
    if (active_ == this) {
        active_ = nullptr;
    }
}

LRESULT CALLBACK KeyboardHook::low_level_proc(int code, WPARAM wparam, LPARAM lparam) noexcept {
    if (active_ == nullptr || code < 0) {
        return ::CallNextHookEx(nullptr, code, wparam, lparam);
    }
    if (wparam == WM_KEYDOWN || wparam == WM_SYSKEYDOWN) {
        const KBDLLHOOKSTRUCT* info = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lparam);
        if (info != nullptr && active_->handle_key_down(*info) == swallow_key) {
            return 1;
        }
    }
    return ::CallNextHookEx(nullptr, code, wparam, lparam);
}

HookKeyInput KeyboardHook::classify(std::uint16_t vk, std::uint16_t scan_code) const noexcept {
    HookKeyInput key;
    switch (vk) {
    case VK_BACK:
        key.kind = HookKeyInput::Kind::backspace;
        return key;
    case VK_ESCAPE:
        key.kind = HookKeyInput::Kind::escape;
        return key;
    case VK_RETURN:
        key.kind = HookKeyInput::Kind::enter;
        return key;
    case VK_SPACE:
        key.kind = HookKeyInput::Kind::space;
        key.character = ' ';
        return key;
    default:
        break;
    }

    // Keys that never carry text — pass through without touching the

    // keyboard state at all.

    if ((vk >= VK_F1 && vk <= VK_F24) || (vk >= VK_PRIOR && vk <= VK_DOWN) || vk == VK_TAB
        || vk == VK_INSERT || vk == VK_DELETE || vk == VK_HOME || vk == VK_END
        || (vk >= VK_SHIFT && vk <= VK_MENU) || vk == VK_LWIN || vk == VK_RWIN || vk == VK_APPS
        || vk == VK_CAPITAL || vk == VK_NUMLOCK || vk == VK_SCROLL) {
        return key;
    }

    BYTE key_state[256] = {};
    if (::GetKeyboardState(key_state) == FALSE) {
        return key;
    }
    wchar_t produced[8] = {};
    const int produced_count = ::ToUnicodeEx(vk, scan_code, key_state, produced, 8, 0, nullptr);
    if (produced_count != 1) {
        // Dead keys (-1) and multi-character outputs pass through untouched.

        return key;
    }
    const wchar_t character = produced[0];
    if ((character >= L'a' && character <= L'z') || (character >= L'A' && character <= L'Z')) {
        key.kind = HookKeyInput::Kind::letter;
        key.character = static_cast<char>(character);
    } else if (character >= L'0' && character <= L'9') {
        key.kind = HookKeyInput::Kind::digit;
        key.character = static_cast<char>(character);
    } else if (character >= 0x20 && character < 0x7F) {
        key.kind = HookKeyInput::Kind::punctuation;
        key.character = static_cast<char>(character);
    }
    // Non-ASCII output (another layout's characters) stays Kind::other.

    return key;
}

LRESULT KeyboardHook::handle_key_down(const KBDLLHOOKSTRUCT& info) noexcept {
    // Our own injected traffic — never re-enter the policy.

    if ((info.flags & LLKHF_INJECTED) != 0) {
        return pass_key;
    }

    const std::uint16_t vk = static_cast<std::uint16_t>(info.vkCode);
    const bool ctrl = (::GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    const bool alt = (::GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
    const bool shift = (::GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    const bool win = ((::GetAsyncKeyState(VK_LWIN) | ::GetAsyncKeyState(VK_RWIN)) & 0x8000) != 0;

    if (vk == config_.toggle_vk && ctrl == config_.toggle_ctrl && alt == config_.toggle_alt
        && shift == config_.toggle_shift && win == config_.toggle_win) {
        if (deps_.controller != nullptr) {
            deps_.controller->set_enabled(!deps_.controller->enabled());
            if (deps_.on_engine_toggled) {
                deps_.on_engine_toggled(deps_.controller->enabled());
            }
        }
        if (deps_.preview != nullptr) {
            deps_.preview->hide();
        }
        return swallow_key;
    }

    // Application shortcuts (Ctrl/Alt/Win chords) are none of our business:

    // pass through and cancel whatever word was being composed.

    if (ctrl || alt || win) {
        if (deps_.controller != nullptr && deps_.controller->composing()) {
            deps_.controller->on_focus_lost();
            if (deps_.preview != nullptr) {
                deps_.preview->hide();
            }
        }
        return pass_key;
    }

    if (deps_.controller == nullptr) {
        return pass_key;
    }

    const HookKeyInput key = classify(vk, static_cast<std::uint16_t>(info.scanCode));
    const ControllerCommands commands = deps_.controller->on_key(key);

    if (commands.swallow) {
        // Retroactive replacement: erase what the app already displayed,

        // inject the Bangla word, then replay the swallowed key so the

        // sentence flow (space, enter, punctuation) is preserved.

        injector::send_backspaces(commands.erase_units);
        injector::send_text_utf8(commands.inject_utf8);
        if (key.kind == HookKeyInput::Kind::space || key.kind == HookKeyInput::Kind::enter) {
            injector::send_virtual_key(vk);
        } else if (key.kind == HookKeyInput::Kind::punctuation) {
            const char replay[1] = {key.character};
            injector::send_text_utf8(std::string_view(replay, 1));
        }
        if (deps_.preview != nullptr) {
            deps_.preview->hide();
        }
        return swallow_key;
    }

    if (deps_.preview != nullptr) {
        if (deps_.preview_enabled && deps_.controller->composing()
            && !commands.preedit_utf8.empty()) {
            deps_.preview->show_text(commands.preedit_utf8);
        } else {
            deps_.preview->hide();
        }
    }
    return pass_key;
}

}  // namespace suborno::platform::windows::standalone
