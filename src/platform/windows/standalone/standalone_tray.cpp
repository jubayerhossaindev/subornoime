#include "standalone_tray.hpp"

#include <shellapi.h>

namespace suborno::platform::windows::standalone {
namespace {

constexpr wchar_t class_name[] = L"SubornoStandaloneTray";
constexpr UINT tray_callback_message = WM_APP + 1;
constexpr int icon_resource_id = 101;  // IDI_SUBORNO in the shared .rc template

constexpr int menu_toggle = 2001;
constexpr int menu_settings = 2002;
constexpr int menu_exit = 2003;

}  // namespace

StandaloneTray::~StandaloneTray() {
    destroy();
}

bool StandaloneTray::create(HINSTANCE instance,
                            const std::wstring& tooltip,
                            const Callbacks& callbacks) {
    instance_ = instance;
    tooltip_ = tooltip;
    callbacks_ = callbacks;
    bangla_on_ = true;

    WNDCLASSEXW window_class = {};
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = &StandaloneTray::proc_thunk;
    window_class.hInstance = instance;
    window_class.lpszClassName = class_name;
    if (::RegisterClassExW(&window_class) == 0) {
        return false;
    }

    // A plain hidden window (not a message-only window): the tray needs the

    // TaskbarCreated broadcast so the icon survives explorer.exe restarts.

    window_ = ::CreateWindowExW(WS_EX_TOOLWINDOW,
                                class_name,
                                L"Suborno Standalone",
                                WS_OVERLAPPED,
                                0,
                                0,
                                0,
                                0,
                                nullptr,
                                nullptr,
                                instance,
                                this);
    if (window_ == nullptr) {
        return false;
    }
    ::ShowWindow(window_, SW_HIDE);

    icon_ = ::LoadIconW(instance, MAKEINTRESOURCEW(icon_resource_id));
    if (icon_ == nullptr) {
        icon_ = ::LoadIconW(nullptr, IDI_APPLICATION);
    }

    taskbar_created_message_ = ::RegisterWindowMessageW(L"TaskbarCreated");

    notify_.cbSize = sizeof(notify_);
    notify_.hWnd = window_;
    notify_.uID = 1;
    notify_.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    notify_.uCallbackMessage = tray_callback_message;
    notify_.hIcon = icon_;
    if (tooltip_.size() >= 128) {
        tooltip_.resize(127);
    }
    ::lstrcpynW(notify_.szTip, tooltip_.c_str(), 128);
    add_or_modify_icon(NIM_ADD);
    return icon_added_;
}

void StandaloneTray::add_or_modify_icon(DWORD message) noexcept {
    const BOOL result = ::Shell_NotifyIconW(message, &notify_);
    icon_added_ = (result != FALSE) || icon_added_;
}

void StandaloneTray::set_enabled_state(bool bangla_on) noexcept {
    bangla_on_ = bangla_on;
    if (!icon_added_) {
        return;
    }
    const wchar_t* state = bangla_on_ ? L"ON" : L"OFF";
    const std::wstring tip = tooltip_ + L" [" + state + L"]";
    if (tip.size() >= 128) {
        ::lstrcpynW(notify_.szTip, tip.c_str(), 128);
    } else {
        ::lstrcpynW(notify_.szTip, tip.c_str(), 128);
    }
    ::Shell_NotifyIconW(NIM_MODIFY, &notify_);
}

void StandaloneTray::destroy() noexcept {
    if (window_ != nullptr) {
        ::DestroyWindow(window_);
        window_ = nullptr;
    }
}

void StandaloneTray::show_context_menu() noexcept {
    const HMENU menu = ::CreatePopupMenu();
    if (menu == nullptr) {
        return;
    }
    const wchar_t* toggle_label =
        bangla_on_ ? L"বাংলা টাইপিং বন্ধ (Ctrl+Alt+B)" : L"বাংলা টাইপিং চালু (Ctrl+Alt+B)";
    ::AppendMenuW(menu, MF_STRING, menu_toggle, toggle_label);
    if (bangla_on_) {
        ::CheckMenuItem(menu, menu_toggle, MF_BYCOMMAND | MF_CHECKED);
    }
    ::AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    ::AppendMenuW(menu, MF_STRING, menu_settings, L"Suborno সেটিংস খুলুন…");
    ::AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    ::AppendMenuW(menu, MF_STRING, menu_exit, L"প্রস্থান");

    // The classic KB fix: the menu must be foregrounded before tracking or it

    // refuses to close when the mouse leaves it.

    ::SetForegroundWindow(window_);
    POINT cursor = {};
    ::GetCursorPos(&cursor);
    const BOOL tracked = ::TrackPopupMenuEx(
        menu, TPM_RIGHTBUTTON | TPM_RETURNCMD, cursor.x, cursor.y, window_, nullptr);
    ::PostMessageW(window_, WM_NULL, 0, 0);
    ::DestroyMenu(menu);
    if (tracked == 0) {
        return;
    }
    switch (tracked) {
    case menu_toggle:
        if (callbacks_.on_toggle) {
            callbacks_.on_toggle();
        }
        break;
    case menu_settings:
        if (callbacks_.on_open_settings) {
            callbacks_.on_open_settings();
        }
        break;
    case menu_exit:
        if (callbacks_.on_exit) {
            callbacks_.on_exit();
        }
        break;
    default:
        break;
    }
}

LRESULT CALLBACK StandaloneTray::proc_thunk(HWND window,
                                            UINT message,
                                            WPARAM wparam,
                                            LPARAM lparam) noexcept {
    if (message == WM_NCCREATE) {
        const CREATESTRUCTW* create = reinterpret_cast<const CREATESTRUCTW*>(lparam);
        if (create != nullptr && create->lpCreateParams != nullptr) {
            auto* self = static_cast<StandaloneTray*>(create->lpCreateParams);
            ::SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
    }
    auto* self = reinterpret_cast<StandaloneTray*>(::GetWindowLongPtrW(window, GWLP_USERDATA));
    if (self == nullptr) {
        return ::DefWindowProcW(window, message, wparam, lparam);
    }
    return self->proc(window, message, wparam, lparam);
}

LRESULT StandaloneTray::proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) noexcept {
    (void)wparam;
    switch (message) {
    case tray_callback_message:
        if (LOWORD(lparam) == WM_LBUTTONUP || LOWORD(lparam) == WM_RBUTTONUP
            || LOWORD(lparam) == WM_CONTEXTMENU) {
            show_context_menu();
        }
        return 0;
    case WM_COMMAND:
        return 0;
    case WM_DESTROY:
        if (icon_added_) {
            ::Shell_NotifyIconW(NIM_DELETE, &notify_);
            icon_added_ = false;
        }
        return 0;
    default:
        if (taskbar_created_message_ != 0 && message == taskbar_created_message_) {
            // explorer.exe restarted — re-register the tray icon.

            add_or_modify_icon(NIM_ADD);
            return 0;
        }
        break;
    }
    return ::DefWindowProcW(window, message, wparam, lparam);
}

}  // namespace suborno::platform::windows::standalone
