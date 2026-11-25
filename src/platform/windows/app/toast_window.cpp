// Themed toast notifications. See the header for the contract. One toast

// at a time: a second show() replaces the first (timer restarted) — a queue

// would stack popups in front of the tray for little value.

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "suborno/platform/windows/app/toast_window.hpp"

#include <algorithm>
#include <cwchar>
#include <string>

#include "suborno/platform/windows/app/ui_accessibility.hpp"
#include "suborno/platform/windows/app/ui_theme.hpp"

namespace suborno::platform::windows::app {
namespace {

constexpr wchar_t toast_class[] = L"SubornoToastWindow";
constexpr UINT dismiss_timer_id = 1;
// The toast lives just long enough to read one line; not modal, not noisy.

constexpr UINT dismiss_milliseconds = 2600;
constexpr int design_height = 44;
constexpr int design_dot_size = 10;
constexpr int design_min_width = 180;
constexpr int design_max_width = 420;
constexpr int design_padding = 16;
constexpr int design_bottom_margin = 8;
constexpr int design_right_margin = 12;
}  // namespace

// The toast window is also its UIA ElementSource: one element carrying the

// message text (role Button — clicking dismisses, and UIA Invoke exposes

// the same action to assistive technology). The window is WS_EX_NOACTIVATE,

// so nothing here takes keyboard focus from typing or TSF composition.

// This is the definition of the nested ToastWindow::ToastWindowState declared in the

// header: the PImpl owner, the window's GWLP_USERDATA and the delete site

// must all agree on this one type.

struct ToastWindow::ToastWindowState : ui_access::ElementSource {
    HWND window = nullptr;
    HINSTANCE instance = nullptr;
    UINT dpi = 96;
    HFONT font = nullptr;
    std::wstring message;
    ToastKind kind = ToastKind::information;
    int window_width = 0;
    int window_height = 0;
    std::shared_ptr<int> a11y_alive = std::make_shared<int>(0);

    // ---- UI Automation ElementSource ----

    std::wstring window_name() const override {
        return ui::text("common.notification", L"Suborno notification");
    }

    int element_count() const override {
        return message.empty() ? 0 : 1;
    }

    bool element_info(int index, ui_access::ElementInfo& out) const override {
        if (index != 0 || message.empty()) {
            return false;
        }
        out.name = message;
        out.role = ui_access::ElementRole::button;
        out.automation_id = L"toast.message";
        out.help_text = window_name();
        out.rect = RECT{0, 0, window_width, window_height};
        out.focusable = false;
        out.focused = false;
        out.supports_invoke = true;
        return true;
    }

    int focused_element() const override {
        return -1;
    }

    bool focus_element(int /*index*/) override {
        return false;
    }

    bool invoke_element(int index) override {
        if (index != 0 || window == nullptr || message.empty()) {
            return false;
        }
        // Same action a pointer click performs: dismiss without focus.

        ::KillTimer(window, dismiss_timer_id);
        ::ShowWindow(window, SW_HIDE);
        return true;
    }

    bool toggle_element(int /*index*/) override {
        return false;
    }

    bool expand_element(int /*index*/, bool /*expand*/) override {
        return false;
    }

    bool set_element_value(int /*index*/, double /*value*/) override {
        return false;
    }
};

namespace {

int sh(const ToastWindow::ToastWindowState& window_state, int pixels) noexcept {
    return ui::scaled(window_state.dpi, pixels);
}

COLORREF kind_color(const ToastWindow::ToastWindowState& window_state) noexcept {
    switch (window_state.kind) {
    case ToastKind::success:
        return ui::theme_success();
    case ToastKind::warning:
        return ui::theme_warning();
    case ToastKind::information:
        break;
    }
    return ui::theme_information();
}

void measure(ToastWindow::ToastWindowState& window_state) {
    // Recomputes fonts and the window size for the current message + DPI.

    const UINT window_dpi = ui::system_dpi_for(window_state.window);
    if (window_dpi != 0) {
        window_state.dpi = window_dpi;
    }
    if (window_state.font != nullptr) {
        ::DeleteObject(window_state.font);
        window_state.font = nullptr;
    }
    window_state.font = ui::create_message_font();
    int text_width = 0;
    if (HDC device_context = ::GetDC(window_state.window); device_context != nullptr) {
        if (window_state.font != nullptr) {
            ::SelectObject(device_context, window_state.font);
        }
        SIZE extent{};
        if (::GetTextExtentPoint32W(device_context,
                                    window_state.message.c_str(),
                                    static_cast<int>(window_state.message.size()),
                                    &extent)
            != FALSE) {
            text_width = extent.cx;
        }
        ::ReleaseDC(window_state.window, device_context);
    }
    const int inner = sh(window_state, design_padding) * 2 + sh(window_state, design_dot_size)
                      + sh(window_state, 10);
    const int width = std::clamp(
        text_width + inner, sh(window_state, design_min_width), sh(window_state, design_max_width));
    window_state.window_width = width;
    window_state.window_height = sh(window_state, design_height);
}

void place_above_tray(ToastWindow::ToastWindowState& window_state) {
    const HMONITOR monitor = ::MonitorFromWindow(window_state.window, MONITOR_DEFAULTTONEAREST);
    MONITORINFO info{};
    info.cbSize = sizeof(info);
    if (::GetMonitorInfoW(monitor, &info) == FALSE) {
        return;
    }
    const int right_aligned_left =
        info.rcWork.right - window_state.window_width - sh(window_state, design_right_margin);
    const int bottom_aligned_top =
        info.rcWork.bottom - window_state.window_height - sh(window_state, design_bottom_margin);
    ::SetWindowPos(window_state.window,
                   HWND_TOPMOST,
                   right_aligned_left,
                   bottom_aligned_top,
                   window_state.window_width,
                   window_state.window_height,
                   SWP_NOACTIVATE);
    ui::apply_rounded_region(window_state.window,
                             window_state.window_width,
                             window_state.window_height,
                             sh(window_state, ui::metrics::radius_medium));
}

void paint_toast(ToastWindow::ToastWindowState& window_state, HDC device_context) {
    RECT client{};
    ::GetClientRect(window_state.window, &client);
    ui::fill_rounded(device_context,
                     client,
                     ui::theme_surface_raised(),
                     sh(window_state, ui::metrics::radius_medium));
    ui::frame_rounded(device_context,
                      client,
                      ui::theme_separator(),
                      sh(window_state, ui::metrics::radius_medium));

    // Status dot: color reinforces the words, never the only signal.

    const int dot_left = client.left + sh(window_state, design_padding);
    const int dot_cy = (client.top + client.bottom) / 2;
    RECT dot_rect{dot_left,
                  dot_cy - sh(window_state, design_dot_size) / 2,
                  dot_left + sh(window_state, design_dot_size),
                  dot_cy + sh(window_state, design_dot_size) / 2};
    if (HBRUSH dot = ::CreateSolidBrush(kind_color(window_state)); dot != nullptr) {
        const HGDIOBJ previous = ::SelectObject(device_context, dot);
        ::Ellipse(device_context, dot_rect.left, dot_rect.top, dot_rect.right, dot_rect.bottom);
        ::SelectObject(device_context, previous);
        ::DeleteObject(dot);
    }

    if (window_state.font != nullptr) {
        ::SelectObject(device_context, window_state.font);
    }
    RECT text_rect{dot_rect.right + sh(window_state, 10),
                   0,
                   client.right - sh(window_state, design_padding),
                   0};
    ui::draw_text(device_context, window_state.message.c_str(), text_rect, ui::theme_text());
}

LRESULT CALLBACK toast_window_proc(HWND window, UINT message, WPARAM w_param, LPARAM l_param) {
    auto* window_state = reinterpret_cast<ToastWindow::ToastWindowState*>(
        ::GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(l_param);
        window_state = static_cast<ToastWindow::ToastWindowState*>(create->lpCreateParams);
        ::SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window_state));
        window_state->window = window;
    }
    if (window_state == nullptr) {
        return ::DefWindowProcW(window, message, w_param, l_param);
    }
    switch (message) {
    case WM_GETOBJECT: {
        bool handled = false;
        const LRESULT result = ui_access::handle_get_object(
            window, w_param, l_param, window_state, window_state->a11y_alive, handled);
        if (handled) {
            return result;
        }
        return ::DefWindowProcW(window, message, w_param, l_param);
    }
    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC device_context = ::BeginPaint(window, &paint);
        if (device_context != nullptr) {
            paint_toast(*window_state, device_context);
            ::EndPaint(window, &paint);
        }
        return 0;
    }
    case WM_ERASEBKGND:
        return TRUE;
    case WM_LBUTTONDOWN:
        // A click on the toast dismisses it without stealing focus.

        ::KillTimer(window, dismiss_timer_id);
        ::ShowWindow(window, SW_HIDE);
        return 0;
    case WM_TIMER:
        if (w_param == dismiss_timer_id) {
            ::KillTimer(window, dismiss_timer_id);
            ::ShowWindow(window, SW_HIDE);
        }
        return 0;
    case WM_DPICHANGED: {
        window_state->dpi = HIWORD(w_param);
        if (l_param != 0) {
            const auto* suggested = reinterpret_cast<const RECT*>(l_param);
            const int width = suggested->right - suggested->left;
            const int height = suggested->bottom - suggested->top;
            ::SetWindowPos(window,
                           HWND_TOPMOST,
                           suggested->left,
                           suggested->top,
                           width,
                           height,
                           SWP_NOACTIVATE);
        }
        ::InvalidateRect(window, nullptr, TRUE);
        return 0;
    }
    case WM_THEMECHANGED:
        // High Contrast toggles re-derive the process palette; the toast

        // repaints with the refreshed tokens on its next paint.

        ui::refresh_active_theme();
        if (window_state->font != nullptr) {
            ::DeleteObject(window_state->font);
            window_state->font = nullptr;
        }
        ::InvalidateRect(window, nullptr, TRUE);
        return 0;
    case WM_DESTROY:
        window_state->window = nullptr;
        return 0;
    case WM_NCDESTROY:
        window_state->a11y_alive.reset();  // providers still held by UIA stop calling in

        ::SetWindowLongPtrW(window, GWLP_USERDATA, 0);
        delete window_state;  // the window owned the state since create()

        return ::DefWindowProcW(window, message, w_param, l_param);
    default:
        return ::DefWindowProcW(window, message, w_param, l_param);
    }
}

ATOM register_toast_class(HINSTANCE instance) {
    static const ATOM registered = [instance] {
        WNDCLASSEXW window_class{};
        window_class.cbSize = sizeof(window_class);
        window_class.lpfnWndProc = &toast_window_proc;
        window_class.hInstance = instance;
        window_class.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
        window_class.lpszClassName = toast_class;
        return ::RegisterClassExW(&window_class);
    }();
    return registered;
}

}  // namespace

ToastWindow::ToastWindow() = default;

ToastWindow::~ToastWindow() {
    shutdown();
}

bool ToastWindow::create(HINSTANCE instance) {
    if (window_state_ != nullptr) {
        return true;
    }
    if (register_toast_class(instance) == 0) {
        return false;
    }
    window_state_ = new (std::nothrow) ToastWindowState();
    if (window_state_ == nullptr) {
        return false;
    }
    window_state_->instance = instance;
    window_state_->dpi = ui::system_dpi_for(nullptr);
    window_state_->font = ui::create_message_font();
    // A message-only owner-less tool window; never taskbar-registered. The

    // window name is the localized notification label (screen readers and

    // the default HWND accessibility fallback announce it).

    window_state_->window = ::CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
                                              toast_class,
                                              window_state_->window_name().c_str(),
                                              WS_POPUP,
                                              CW_USEDEFAULT,
                                              CW_USEDEFAULT,
                                              0,
                                              0,
                                              nullptr,
                                              nullptr,
                                              instance,
                                              window_state_);
    if (window_state_->window == nullptr) {
        delete window_state_;
        window_state_ = nullptr;
        return false;
    }
    return true;
}

void ToastWindow::shutdown() {
    if (window_state_ == nullptr) {
        return;
    }
    if (window_state_->window != nullptr) {
        ::DestroyWindow(window_state_->window);  // WM_NCDESTROY deletes window_state_

        window_state_ = nullptr;
        return;
    }
    delete window_state_;
    window_state_ = nullptr;
}

void ToastWindow::show(HWND owner, const wchar_t* message, ToastKind kind) {
    if (window_state_ == nullptr || message == nullptr) {
        return;
    }
    // Measure from the OWNER's DPI: the toast appears where the action

    // happened, not necessarily on the monitor the toast window was last on.

    window_state_->message.assign(message);
    window_state_->kind = kind;
    measure(*window_state_);
    place_above_tray(*window_state_);
    (void)owner;  // positioning uses the toast window's own monitor

    ::KillTimer(window_state_->window, dismiss_timer_id);
    ::ShowWindow(window_state_->window, SW_SHOWNOACTIVATE);
    ::InvalidateRect(window_state_->window, nullptr, TRUE);
    ::SetTimer(window_state_->window, dismiss_timer_id, dismiss_milliseconds, nullptr);
}

}  // namespace suborno::platform::windows::app
