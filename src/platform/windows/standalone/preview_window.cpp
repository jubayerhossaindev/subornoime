#include "preview_window.hpp"

#include <string>

namespace suborno::platform::windows::standalone {
namespace {

constexpr wchar_t class_name[] = L"SubornoStandalonePreview";
// Dark token of the Suborno visual language: background #1B1B1F, border and

// text echo the concept art (accent #F7630C, white text).

constexpr COLORREF background_color = RGB(27, 27, 31);
constexpr COLORREF border_color = RGB(247, 99, 12);
constexpr COLORREF text_color = RGB(240, 240, 244);
constexpr BYTE preview_alpha = 235;
constexpr int margin_pixels = 10;

std::wstring utf8_to_utf16(std::string_view text_utf8) {
    if (text_utf8.empty()) {
        return {};
    }
    const int source_size = static_cast<int>(text_utf8.size());
    const int required = ::MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, text_utf8.data(), source_size, nullptr, 0);
    if (required <= 0) {
        return {};
    }
    std::wstring wide(static_cast<std::size_t>(required), L'\0');
    const int written = ::MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, text_utf8.data(), source_size, wide.data(), required);
    if (written <= 0) {
        return {};
    }
    wide.resize(static_cast<std::size_t>(written));
    return wide;
}

void clamp_to_work_area(int& x, int& y, int width, int height) {
    const POINT origin = {x, y};
    const HMONITOR monitor = ::MonitorFromPoint(origin, MONITOR_DEFAULTTONEAREST);
    if (monitor == nullptr) {
        return;
    }
    MONITORINFO info = {};
    info.cbSize = sizeof(info);
    if (!::GetMonitorInfoW(monitor, &info)) {
        return;
    }
    if (x < info.rcWork.left) {
        x = info.rcWork.left;
    }
    if (y < info.rcWork.top) {
        y = info.rcWork.top;
    }
    if (x + width > info.rcWork.right) {
        x = info.rcWork.right - width;
    }
    if (y + height > info.rcWork.bottom) {
        y = info.rcWork.bottom - height;
    }
}

}  // namespace

PreviewWindow::~PreviewWindow() {
    if (window_ != nullptr) {
        ::DestroyWindow(window_);
        window_ = nullptr;
    }
    if (font_ != nullptr) {
        ::DeleteObject(font_);
        font_ = nullptr;
    }
    if (background_ != nullptr) {
        ::DeleteObject(background_);
        background_ = nullptr;
    }
    if (border_ != nullptr) {
        ::DeleteObject(border_);
        border_ = nullptr;
    }
}

bool PreviewWindow::create(HINSTANCE instance, const PreviewFontChoice& font_choice) noexcept {
    static bool class_registered = false;
    if (!class_registered) {
        WNDCLASSEXW window_class = {};
        window_class.cbSize = sizeof(window_class);
        window_class.lpfnWndProc = &PreviewWindow::window_proc_thunk;
        window_class.hInstance = instance;
        window_class.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
        window_class.lpszClassName = class_name;
        if (::RegisterClassExW(&window_class) == 0) {
            return false;
        }
        class_registered = true;
    }

    // Weight and family come from the bundled-font policy: Hind Siliguri

    // Medium (bundled, FR_PRIVATE) on stock machines, Nirmala UI as the

    // Windows-8+ safety net. The choice's weight is a Win32 FW_* value from

    // the same constants Windows.h defines.

    font_ = ::CreateFontW(-22,
                          0,
                          0,
                          0,
                          font_choice.weight,
                          FALSE,
                          FALSE,
                          FALSE,
                          DEFAULT_CHARSET,
                          OUT_DEFAULT_PRECIS,
                          CLIP_DEFAULT_PRECIS,
                          CLEARTYPE_QUALITY,
                          DEFAULT_PITCH | FF_DONTCARE,
                          font_choice.family.c_str());
    background_ = ::CreateSolidBrush(background_color);
    border_ = ::CreateSolidBrush(border_color);
    if (font_ == nullptr || background_ == nullptr || border_ == nullptr) {
        return false;
    }

    const DWORD extended_style =
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
    window_ = ::CreateWindowExW(extended_style,
                                class_name,
                                nullptr,
                                WS_POPUP,
                                0,
                                0,
                                10,
                                10,
                                nullptr,
                                nullptr,
                                instance,
                                this);
    return window_ != nullptr;
}

void PreviewWindow::show_text(std::string_view preedit_utf8) noexcept {
    if (window_ == nullptr) {
        return;
    }
    if (preedit_utf8.empty()) {
        hide();
        return;
    }
    text_ = utf8_to_utf16(preedit_utf8);
    if (text_.empty()) {
        hide();
        return;
    }

    int width = 0;
    int height = 0;
    {
        const HDC measure = ::GetDC(window_);
        if (measure == nullptr) {
            hide();
            return;
        }
        const HGDIOBJ previous_font = ::SelectObject(measure, font_);
        SIZE extent = {};
        if (::GetTextExtentPoint32W(
                measure, text_.c_str(), static_cast<int>(text_.size()), &extent)) {
            width = extent.cx + margin_pixels * 2;
            height = extent.cy + margin_pixels * 2;
        }
        ::SelectObject(measure, previous_font);
        ::ReleaseDC(window_, measure);
        if (width <= 0 || height <= 0) {
            hide();
            return;
        }
    }

    int x = 0;
    int y = 0;
    GUITHREADINFO thread_info = {};
    thread_info.cbSize = sizeof(thread_info);
    bool anchored = false;
    if (::GetGUIThreadInfo(0, &thread_info) && thread_info.hwndCaret != nullptr) {
        POINT caret = {thread_info.rcCaret.left, thread_info.rcCaret.bottom};
        if (::ClientToScreen(thread_info.hwndCaret, &caret)) {
            x = caret.x + 2;
            y = caret.y + 4;
            anchored = true;
        }
    }
    if (!anchored) {
        // Apps without a Win32 caret (some Electron/Java windows): fall back

        // to the bottom-right of the focused window instead of hiding.

        const HWND foreground = ::GetForegroundWindow();
        RECT window_rect = {};
        if (foreground != nullptr && ::GetWindowRect(foreground, &window_rect)) {
            x = window_rect.right - width - 16;
            y = window_rect.bottom - height - 16;
            anchored = true;
        }
    }
    if (!anchored) {
        hide();
        return;
    }

    clamp_to_work_area(x, y, width, height);

    if (!::SetWindowPos(window_,
                        HWND_TOPMOST,
                        x,
                        y,
                        width,
                        height,
                        SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW)) {
        hide();
        return;
    }
    ::SetLayeredWindowAttributes(window_, 0, preview_alpha, LWA_ALPHA);
    ::InvalidateRect(window_, nullptr, FALSE);
    ::UpdateWindow(window_);
}

void PreviewWindow::hide() noexcept {
    if (window_ != nullptr) {
        ::ShowWindow(window_, SW_HIDE);
    }
}

LRESULT CALLBACK PreviewWindow::window_proc_thunk(HWND window,
                                                  UINT message,
                                                  WPARAM wparam,
                                                  LPARAM lparam) noexcept {
    if (message == WM_NCCREATE) {
        const CREATESTRUCTW* create = reinterpret_cast<const CREATESTRUCTW*>(lparam);
        if (create != nullptr && create->lpCreateParams != nullptr) {
            auto* self = static_cast<PreviewWindow*>(create->lpCreateParams);
            ::SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            self->window_ = window;
        }
    }
    auto* self = reinterpret_cast<PreviewWindow*>(::GetWindowLongPtrW(window, GWLP_USERDATA));
    if (self == nullptr) {
        return ::DefWindowProcW(window, message, wparam, lparam);
    }
    return self->window_proc(window, message, wparam, lparam);
}

LRESULT PreviewWindow::window_proc(HWND window,
                                   UINT message,
                                   WPARAM wparam,
                                   LPARAM lparam) noexcept {
    (void)wparam;
    switch (message) {
    case WM_PAINT: {
        PAINTSTRUCT paint = {};
        const HDC device_context = ::BeginPaint(window, &paint);
        if (device_context == nullptr) {
            return 0;
        }
        ::FillRect(device_context, &paint.rcPaint, background_);
        ::FrameRect(device_context, &paint.rcPaint, border_);
        RECT text_rect = paint.rcPaint;
        text_rect.left += margin_pixels;
        text_rect.right -= margin_pixels;
        const HGDIOBJ previous_font = ::SelectObject(device_context, font_);
        ::SetBkMode(device_context, TRANSPARENT);
        ::SetTextColor(device_context, text_color);
        ::DrawTextW(device_context,
                    text_.c_str(),
                    static_cast<int>(text_.size()),
                    &text_rect,
                    DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        ::SelectObject(device_context, previous_font);
        ::EndPaint(window, &paint);
        return 0;
    }
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;
    case WM_NCDESTROY:
        window_ = nullptr;
        return 0;
    default:
        break;
    }
    return ::DefWindowProcW(window, message, wparam, lparam);
}

}  // namespace suborno::platform::windows::standalone
