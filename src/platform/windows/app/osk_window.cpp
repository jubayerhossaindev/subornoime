#include "suborno/platform/windows/app/osk_window.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <algorithm>
#include <array>
#include <cctype>
#include <cwchar>
#include <memory>
#include <utility>
#include <windowsx.h>

#include "suborno/platform/windows/app/ui_accessibility.hpp"
#include "suborno/platform/windows/app/ui_theme.hpp"

namespace suborno::platform::windows::app {
namespace {

constexpr wchar_t osk_window_class[] = L"SubornoOskWindow";
// 96-DPI design metrics.

constexpr int key_size = 36;
constexpr int key_gap = 5;
constexpr int key_stride = key_size + key_gap;
constexpr int osk_padding = 12;
constexpr int title_height = 36;
constexpr int mode_row_height = 34;
constexpr int osk_rows = 5;  // digits, QWERTY, home, bottom, space

struct OskKeyDef {
    UINT virtual_key;  // the key the OSK injects (its TSF-visible identity)

    wchar_t base;  // the unshifted US character ('a'..'z', '0'..'9')
};

// The physical keys the OSK exposes, in visual order.

constexpr std::array<std::array<OskKeyDef, 10>, 3> letter_rows{{
    {{
        {0x31, L'1'},
        {0x32, L'2'},
        {0x33, L'3'},
        {0x34, L'4'},
        {0x35, L'5'},
        {0x36, L'6'},
        {0x37, L'7'},
        {0x38, L'8'},
        {0x39, L'9'},
        {0x30, L'0'},
    }},
    {{
        {'Q', L'q'},
        {'W', L'w'},
        {'E', L'e'},
        {'R', L'r'},
        {'T', L't'},
        {'Y', L'y'},
        {'U', L'u'},
        {'I', L'i'},
        {'O', L'o'},
        {'P', L'p'},
    }},
    {{
        {'A', L'a'},
        {'S', L's'},
        {'D', L'd'},
        {'F', L'f'},
        {'G', L'g'},
        {'H', L'h'},
        {'J', L'j'},
        {'K', L'k'},
        {'L', L'l'},
        {0, L'\0'},
    }},
}};

std::wstring bangla_or_latin(const std::wstring& bangla, wchar_t latin) {
    return bangla.empty() ? std::wstring(1, latin) : bangla;
}

}  // namespace

struct OskWindow::OskWindowState : public ui_access::ElementSource {
    HWND window = nullptr;
    HINSTANCE instance = nullptr;
    Host host;
    UINT dpi = 96;
    HFONT font = nullptr;
    HFONT font_small = nullptr;
    bool english_mode = true;
    bool shift = false;
    bool visible_state = false;
    bool mouse_tracked = false;
    int hover_key = -1;  // flat key index, -1 = none

    int pressed_key = -1;  // key currently held down (pressed face)

    // Hovered mode button, tracked per button: 0 = English, 1 = Bangla,

    // -1 = none. A single "over either mode button" flag highlighted the

    // ACTIVE button instead of the one under the pointer.

    int hover_mode = -1;
    bool hover_close = false;
    // Liveness token for the UIA providers (reset in WM_DESTROY).

    std::shared_ptr<int> a11y_alive = std::make_shared<int>(0);

    int scaled(int pixels) const noexcept {
        return ui::scaled(dpi, pixels);
    }

    // Flat key model: three letter/digit rows plus the special bottom row.

    // Special keys: 30 = Shift, 31 = Space, 32 = Backspace, 33 = Enter.

    static constexpr int flat_key_count = 3 * 10 + 4;
    static constexpr int special_shift = 30;
    static constexpr int special_space = 31;
    static constexpr int special_backspace = 32;
    static constexpr int special_enter = 33;

    struct KeyRect {
        RECT rect;  // client pixels

        int flat = -1;  // flat index or -1
    };

    int key_base_y() const noexcept {
        return scaled(osk_padding + title_height + mode_row_height + key_gap);
    }

    RECT flat_key_rect(int flat) const noexcept {
        RECT rect{};
        const int row = flat / 10;
        const int column = flat % 10;
        const int base_y = key_base_y();
        if (row < 3) {
            rect.left = scaled(osk_padding) + column * scaled(key_stride);
            rect.top = base_y + row * scaled(key_stride);
            rect.right = rect.left + scaled(key_size);
            rect.bottom = rect.top + scaled(key_size);
            return rect;
        }
        // Bottom row: Shift / Space / Backspace / Enter in one line.

        const int bottom_y = base_y + 3 * scaled(key_stride);
        switch (flat) {
        case special_shift:
            rect = RECT{scaled(osk_padding),
                        bottom_y,
                        scaled(osk_padding) + scaled(key_stride * 2 - key_gap),
                        bottom_y + scaled(key_size)};
            break;
        case special_space:
            rect = RECT{scaled(osk_padding) + scaled(key_stride * 2),
                        bottom_y,
                        scaled(osk_padding) + scaled(key_stride * 7 - key_gap),
                        bottom_y + scaled(key_size)};
            break;
        case special_backspace:
            rect = RECT{scaled(osk_padding) + scaled(key_stride * 7),
                        bottom_y,
                        scaled(osk_padding) + scaled(key_stride * 9 - key_gap),
                        bottom_y + scaled(key_size)};
            break;
        default:  // special_enter

            rect = RECT{scaled(osk_padding) + scaled(key_stride * 9),
                        bottom_y,
                        scaled(osk_padding) + scaled(key_stride * 10 - key_gap),
                        bottom_y + scaled(key_size)};
            break;
        }
        return rect;
    }

    RECT mode_button_rect(bool english) const noexcept {
        const int top = scaled(osk_padding + title_height);
        const int width = scaled(96);
        if (english) {
            return RECT{scaled(osk_padding),
                        top,
                        scaled(osk_padding) + width,
                        top + scaled(mode_row_height - 6)};
        }
        return RECT{scaled(osk_padding) + width + scaled(6),
                    top,
                    scaled(osk_padding) + width * 2 + scaled(6),
                    top + scaled(mode_row_height - 6)};
    }

    RECT close_rect() const noexcept {
        const int right = window_width() - scaled(osk_padding);
        return RECT{right - scaled(28), scaled(6), right, scaled(30)};
    }

    int window_width() const noexcept {
        return scaled(osk_padding * 2 + key_stride * 10 - key_gap);
    }

    int window_height() const noexcept {
        return scaled(osk_padding + title_height + mode_row_height + key_gap
                      + osk_rows * key_stride);
    }

    void ensure_fonts() {
        if (font == nullptr) {
            NONCLIENTMETRICSW metrics{};
            metrics.cbSize = sizeof(metrics);
            if (::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0)
                != FALSE) {
                // Honor the configured Bangla-capable UI family (the settings

                // font picker); empty = the OS message font. Bangla key labels

                // previously relied on GDI font linking alone.

                const std::wstring& family = ui::ui_font_family();
                if (!family.empty() && family.size() < LF_FACESIZE) {
                    ::wcsncpy_s(metrics.lfMessageFont.lfFaceName,
                                LF_FACESIZE,
                                family.c_str(),
                                family.size());
                }
                metrics.lfMessageFont.lfHeight = -scaled(13);
                font = ::CreateFontIndirectW(&metrics.lfMessageFont);
                metrics.lfMessageFont.lfHeight = -scaled(11);
                font_small = ::CreateFontIndirectW(&metrics.lfMessageFont);
            }
        }
    }

    void release_fonts() {
        if (font != nullptr) {
            ::DeleteObject(font);
            font = nullptr;
        }
        if (font_small != nullptr) {
            ::DeleteObject(font_small);
            font_small = nullptr;
        }
    }

    bool is_english() const noexcept {
        return english_mode;
    }

    void sync_mode_from_host() {
        if (host.active_layout_id) {
            english_mode = host.active_layout_id() == "english";
        }
    }

    // Label for a flat key in the current mode.

    std::wstring key_label(int flat) const {
        if (flat == special_shift) {
            return shift ? L"⇧" : L"Shift";
        }
        if (flat == special_space) {
            return L"Space";
        }
        if (flat == special_backspace) {
            return L"⌫";
        }
        if (flat == special_enter) {
            return L"↵";
        }
        const int row = flat / 10;
        const int column = flat % 10;
        const OskKeyDef def =
            letter_rows[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)];
        if (def.base == L'\0') {
            return L"";
        }
        if (is_english()) {
            const wchar_t letter = shift ? static_cast<wchar_t>(::towupper(def.base)) : def.base;
            return std::wstring(1, letter);
        }
        // Bangla mode: show what the active layout actually produces for the

        // typed letter — uppercase when shift is on, so the OSK mirrors the

        // real keyboard behavior (e.g. Shift+k types the aspirated form).

        std::wstring label;
        if (host.bangla_preview) {
            const std::string letter = shift
                                           ? std::string(1, static_cast<char>(::toupper(def.base)))
                                           : std::string(1, static_cast<char>(def.base));
            label = host.bangla_preview(letter);
        }
        return bangla_or_latin(label, def.base);
    }

    UINT key_virtual_key(int flat) const noexcept {
        if (flat == special_space) {
            return VK_SPACE;
        }
        if (flat == special_backspace) {
            return VK_BACK;
        }
        if (flat == special_enter) {
            return VK_RETURN;
        }
        if (flat >= special_shift) {
            return 0;  // handled locally
        }
        const int row = flat / 10;
        const int column = flat % 10;
        return letter_rows[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)]
            .virtual_key;
    }

    void send_virtual_key(UINT virtual_key, bool shifted) {
        // Inject the real key so the TSF text service (and the host

        // application) see a genuine keystroke: transliteration, candidate

        // selection, learning and encoding all run through the normal path.

        // Worst case is four events: shift down, key down, key up, shift up

        // (the plain path sends two). The array size must match the maximum

        // count so SendInput only ever reads initialized elements.

        INPUT inputs[4]{};
        int count = 0;
        if (shifted && virtual_key != VK_SPACE && virtual_key != VK_BACK
            && virtual_key != VK_RETURN) {
            inputs[count].type = INPUT_KEYBOARD;
            inputs[count].ki.wVk = VK_SHIFT;
            inputs[count].ki.dwFlags = 0;
            ++count;
        }
        inputs[count].type = INPUT_KEYBOARD;
        inputs[count].ki.wVk = static_cast<WORD>(virtual_key);
        inputs[count].ki.wScan = static_cast<WORD>(::MapVirtualKeyW(virtual_key, MAPVK_VK_TO_VSC));
        ++count;
        inputs[count].type = INPUT_KEYBOARD;
        inputs[count].ki.wVk = static_cast<WORD>(virtual_key);
        inputs[count].ki.wScan = inputs[count - 1].ki.wScan;
        inputs[count].ki.dwFlags = KEYEVENTF_KEYUP;
        ++count;
        if (shifted && virtual_key != VK_SPACE && virtual_key != VK_BACK
            && virtual_key != VK_RETURN) {
            inputs[count].type = INPUT_KEYBOARD;
            inputs[count].ki.wVk = VK_SHIFT;
            inputs[count].ki.dwFlags = KEYEVENTF_KEYUP;
            ++count;
        }
        ::SendInput(static_cast<UINT>(count), inputs, sizeof(INPUT));
    }

    void handle_key_click(int flat) {
        if (flat == special_shift) {
            shift = !shift;
            repaint();
            return;
        }
        const UINT virtual_key = key_virtual_key(flat);
        if (virtual_key != 0) {
            send_virtual_key(virtual_key, shift);
        }
        // Modifiers do not latch (real keyboards release shift after the key).

        if (shift) {
            shift = false;
        }
        repaint();
    }

    void handle_mode_click(bool english) {
        if (host.select_mode) {
            host.select_mode(english);
        }
        sync_mode_from_host();
        repaint();
    }

    int hit_flat_key(POINT point) const noexcept {
        for (int flat = 0; flat < flat_key_count; ++flat) {
            const RECT rect = flat_key_rect(flat);
            if (point.x >= rect.left && point.x < rect.right && point.y >= rect.top
                && point.y < rect.bottom) {
                return key_is_live(flat) ? flat : -1;
            }
        }
        return -1;
    }

    // Which mode button the point is over: 0 = English, 1 = Bangla, -1 = none.

    int hit_mode_button(POINT point) const noexcept {
        const RECT rect = mode_button_rect(true);
        if (point.x >= rect.left && point.x < rect.right && point.y >= rect.top
            && point.y < rect.bottom) {
            return 0;
        }
        const RECT rect_bn = mode_button_rect(false);
        if (point.x >= rect_bn.left && point.x < rect_bn.right && point.y >= rect_bn.top
            && point.y < rect_bn.bottom) {
            return 1;
        }
        return -1;
    }

    // A letter-row slot with no physical key behind it (the trailing filler

    // in the 10-column grid) is dead: it paints nothing, never hit-tests

    // and is not exposed through UI Automation.

    bool key_is_live(int flat) const noexcept {
        if (flat < 0 || flat >= flat_key_count) {
            return false;
        }
        if (flat >= special_shift) {
            return true;  // Shift / Space / Backspace / Enter
        }
        const int row = flat / 10;
        const int column = flat % 10;
        return letter_rows[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)]
                   .virtual_key
               != 0;
    }

    void repaint() const noexcept {
        if (window != nullptr) {
            ::InvalidateRect(window, nullptr, FALSE);
        }
    }

    void draw_button(HDC device_context,
                     const RECT& rect,
                     const std::wstring& label,
                     bool active,
                     bool hover) const {
        COLORREF face = active ? ui::theme_accent_fill()
                               : (hover ? ui::theme_surface_hover() : ui::theme_surface_raised());
        ui::fill_rounded(device_context, rect, face, scaled(6));
        const COLORREF label_color = active ? ui::theme_on_accent() : ui::theme_text();
        ui::draw_text(device_context,
                      label.c_str(),
                      rect,
                      label_color,
                      DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }

    // ---- UI Automation ElementSource ----

    // The window object IS the source: its lifetime is exactly the

    // window's, so UIA callbacks after WM_GETOBJECT returns are always in

    // bounds (the liveness token catches the destruction window).

    // Element order: 0 English, 1 বাংলা, 2 Close, 3..36 = flat keys 0..33.

    static constexpr int a11y_mode_english = 0;
    static constexpr int a11y_mode_bangla = 1;
    static constexpr int a11y_close = 2;
    static constexpr int a11y_first_key = 3;

    std::wstring window_name() const override {
        return ui::text("osk.window.title", L"Suborno On-Screen Keyboard");
    }

    int element_count() const override {
        return a11y_first_key + flat_key_count;
    }

    bool element_info(int index, ui_access::ElementInfo& out) const override {
        switch (index) {
        case a11y_mode_english:
            out.name = ui::text("osk.mode.english", L"English");
            out.role = ui_access::ElementRole::button;
            out.rect = mode_button_rect(true);
            out.supports_invoke = true;
            return true;
        case a11y_mode_bangla:
            out.name = ui::text("osk.mode.bangla", L"বাংলা");
            out.role = ui_access::ElementRole::button;
            out.rect = mode_button_rect(false);
            out.supports_invoke = true;
            return true;
        case a11y_close:
            out.name = ui::text("common.close", L"Close");
            out.role = ui_access::ElementRole::button;
            out.rect = close_rect();
            out.supports_invoke = true;
            return true;
        default:
            break;
        }
        const int flat = index - a11y_first_key;
        if (flat < 0 || flat >= flat_key_count || !key_is_live(flat)) {
            return false;  // dead grid slots are not exposed as elements
        }
        out.role = ui_access::ElementRole::button;
        out.rect = flat_key_rect(flat);
        out.supports_invoke = true;
        // Words for the special keys (the display glyphs ⇧/⌫/↵ do not

        // announce well); the displayed label for letters/digits.

        switch (flat) {
        case special_shift:
            out.name = ui::text("osk.key.shift", L"Shift");
            return true;
        case special_space:
            out.name = ui::text("osk.key.space", L"Space");
            return true;
        case special_backspace:
            out.name = ui::text("osk.key.backspace", L"Backspace");
            return true;
        case special_enter:
            out.name = ui::text("osk.key.enter", L"Enter");
            return true;
        default:
            out.name = key_label(flat);
            return true;
        }
    }

    int focused_element() const override {
        return -1;  // the OSK is mouse/Invoke driven by design
    }

    bool focus_element(int) override {
        return false;
    }

    bool invoke_element(int index) override {
        switch (index) {
        case a11y_mode_english:
            handle_mode_click(true);
            return true;
        case a11y_mode_bangla:
            handle_mode_click(false);
            return true;
        case a11y_close:
            visible_state = false;
            ::ShowWindow(window, SW_HIDE);
            return true;
        default:
            break;
        }
        const int flat = index - a11y_first_key;
        if (flat < 0 || flat >= flat_key_count) {
            return false;
        }
        handle_key_click(flat);
        return true;
    }

    bool toggle_element(int) override {
        return false;
    }

    bool expand_element(int, bool) override {
        return false;
    }

    bool set_element_value(int, double) override {
        return false;
    }
};

LRESULT CALLBACK osk_window_proc(HWND window, UINT message, WPARAM w_param, LPARAM l_param) {
    auto* window_state =
        reinterpret_cast<OskWindow::OskWindowState*>(::GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(l_param);
        window_state = static_cast<OskWindow::OskWindowState*>(create->lpCreateParams);
        ::SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window_state));
        window_state->window = window;
    }
    if (window_state == nullptr) {
        return ::DefWindowProcW(window, message, w_param, l_param);
    }

    switch (message) {
    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC device_context = ::BeginPaint(window, &paint);
        if (device_context == nullptr) {
            return 0;
        }
        RECT client{};
        ::GetClientRect(window, &client);
        HBRUSH surface = ::CreateSolidBrush(ui::theme_surface_topbar());
        if (surface != nullptr) {
            ::FillRect(device_context, &client, surface);
            ::DeleteObject(surface);
        }
        window_state->ensure_fonts();
        if (window_state->font != nullptr) {
            ::SelectObject(device_context, window_state->font);
        }

        // Title row + close button.

        RECT title{window_state->scaled(osk_padding),
                   window_state->scaled(6),
                   window_state->scaled(osk_padding + 260),
                   window_state->scaled(30)};
        if (window_state->font_small != nullptr) {
            ::SelectObject(device_context, window_state->font_small);
        }
        std::wstring title_text;
        if (window_state->host.mode_display_name) {
            title_text = window_state->host.mode_display_name();
        }
        if (title_text.empty()) {
            title_text = ui::text("osk.window.title", L"Suborno On-Screen Keyboard");
        }
        ui::draw_text(device_context, title_text.c_str(), title, ui::theme_text());
        if (window_state->hover_close) {
            ui::fill_rounded(device_context,
                             window_state->close_rect(),
                             ui::theme_surface_hover(),
                             window_state->scaled(6));
        }
        const RECT close_rect = window_state->close_rect();
        const int close_cx = (close_rect.left + close_rect.right) / 2;
        const int close_cy = (close_rect.top + close_rect.bottom) / 2;
        const int close_arm = window_state->scaled(5);
        HPEN close_pen = ::CreatePen(PS_SOLID, window_state->scaled(2), ui::theme_text());
        if (close_pen != nullptr) {
            const HGDIOBJ previous = ::SelectObject(device_context, close_pen);
            ::MoveToEx(device_context, close_cx - close_arm, close_cy - close_arm, nullptr);
            ::LineTo(device_context, close_cx + close_arm, close_cy + close_arm);
            ::MoveToEx(device_context, close_cx + close_arm, close_cy - close_arm, nullptr);
            ::LineTo(device_context, close_cx - close_arm, close_cy + close_arm);
            ::SelectObject(device_context, previous);
            ::DeleteObject(close_pen);
        }

        // Mode toggle pair — a real switch between the English passthrough

        // layout and the last used Bangla layout.

        if (window_state->font != nullptr) {
            ::SelectObject(device_context, window_state->font);
        }
        window_state->draw_button(device_context,
                                  window_state->mode_button_rect(true),
                                  ui::text("osk.mode.english", L"English"),
                                  window_state->is_english(),
                                  window_state->hover_mode == 0);
        window_state->draw_button(device_context,
                                  window_state->mode_button_rect(false),
                                  ui::text("osk.mode.bangla", L"বাংলা"),
                                  !window_state->is_english(),
                                  window_state->hover_mode == 1);

        // Keys.

        for (int flat = 0; flat < OskWindow::OskWindowState::flat_key_count; ++flat) {
            if (!window_state->key_is_live(flat)) {
                continue;  // dead grid slot: no button box, no label
            }
            const RECT rect = window_state->flat_key_rect(flat);
            const std::wstring label = window_state->key_label(flat);
            const bool special = flat >= OskWindow::OskWindowState::special_shift;
            const bool active =
                flat == OskWindow::OskWindowState::special_shift && window_state->shift;
            const bool hover = flat == window_state->hover_key;
            const bool pressed = flat == window_state->pressed_key;
            // Fluent state model: pressed dims below hover (dark mode:

            // hover brightens, press dims); the accent face stays for the

            // latched Shift.

            COLORREF face = active ? ui::theme_accent_fill()
                                   : (pressed ? ui::theme_surface_pressed()
                                              : (hover ? ui::theme_surface_hover()
                                                       : (special ? ui::theme_surface_raised()
                                                                  : ui::theme_surface())));
            ui::fill_rounded(device_context, rect, face, window_state->scaled(5));
            ui::frame_rounded(device_context, rect, ui::theme_separator(), window_state->scaled(5));
            const COLORREF label_color = active ? ui::theme_on_accent() : ui::theme_text();
            ui::draw_text(device_context,
                          label.c_str(),
                          rect,
                          label_color,
                          DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        }
        ::EndPaint(window, &paint);
        return 0;
    }
    case WM_ERASEBKGND:
        return TRUE;
    case WM_MOUSEMOVE: {
        if (!window_state->mouse_tracked) {
            TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE, window, 0};
            if (::TrackMouseEvent(&track) != FALSE) {
                window_state->mouse_tracked = true;
            }
        }
        const POINT point{GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
        const int key = window_state->hit_flat_key(point);
        const int mode_now = window_state->hit_mode_button(point);
        const RECT close_rect = window_state->close_rect();
        const bool close_now = point.x >= close_rect.left && point.x < close_rect.right
                               && point.y >= close_rect.top && point.y < close_rect.bottom;
        if (key != window_state->hover_key || mode_now != window_state->hover_mode
            || close_now != window_state->hover_close) {
            window_state->hover_key = key;
            window_state->hover_mode = mode_now;
            window_state->hover_close = close_now;
            window_state->repaint();
        }
        return 0;
    }
    case WM_MOUSELEAVE: {
        window_state->mouse_tracked = false;
        window_state->hover_key = -1;
        window_state->hover_mode = -1;
        window_state->hover_close = false;
        window_state->repaint();
        return 0;
    }
    case WM_LBUTTONDOWN: {
        const POINT point{GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
        if (point.y < window_state->scaled(title_height)) {
            // Drag by the title row.

            ::ReleaseCapture();
            ::PostMessageW(window, WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x, point.y));
            return 0;
        }
        const RECT close_rect = window_state->close_rect();
        if (point.x >= close_rect.left && point.x < close_rect.right && point.y >= close_rect.top
            && point.y < close_rect.bottom) {
            window_state->visible_state = false;
            ::ShowWindow(window, SW_HIDE);
            return 0;
        }
        const int mode_hit = window_state->hit_mode_button(point);
        if (mode_hit >= 0) {
            window_state->handle_mode_click(mode_hit == 0);
            return 0;
        }
        const int flat = window_state->hit_flat_key(point);
        if (flat >= 0) {
            window_state->pressed_key = flat;
            window_state->repaint();  // pressed face while the key is held down

            window_state->handle_key_click(flat);
        }
        return 0;
    }
    case WM_LBUTTONUP:
        if (window_state->pressed_key != -1) {
            window_state->pressed_key = -1;
            window_state->repaint();
        }
        return 0;
    case WM_DPICHANGED: {
        window_state->dpi = static_cast<UINT>(HIWORD(w_param));
        window_state->release_fonts();
        if (l_param != 0) {
            const auto* suggested = reinterpret_cast<const RECT*>(l_param);
            const int width = suggested->right - suggested->left;
            const int height = suggested->bottom - suggested->top;
            ::SetWindowPos(window,
                           nullptr,
                           suggested->left,
                           suggested->top,
                           width,
                           height,
                           SWP_NOACTIVATE | SWP_NOZORDER);
            // The rounded region was cut for the old DPI geometry; re-cut it

            // for the resized window (matches the topbar's DPI handling).

            ui::apply_window_treatment(window, width, height, ui::scaled(window_state->dpi, 8));
        }
        window_state->repaint();
        return 0;
    }
    case WM_GETOBJECT: {
        bool handled = false;
        const LRESULT result = ui_access::handle_get_object(
            window, w_param, l_param, window_state, window_state->a11y_alive, handled);
        if (handled) {
            return result;
        }
        return ::DefWindowProcW(window, message, w_param, l_param);
    }
    case WM_THEMECHANGED:
        ui::refresh_active_theme();
        window_state->repaint();
        return 0;
    case WM_KEYDOWN:
        if (w_param == VK_ESCAPE) {
            window_state->visible_state = false;
            ::ShowWindow(window, SW_HIDE);
        }
        return 0;
    case WM_DESTROY:
        window_state->window = nullptr;
        window_state->a11y_alive.reset();
        window_state->release_fonts();
        return 0;
    case WM_NCDESTROY:
        ::SetWindowLongPtrW(window, GWLP_USERDATA, 0);
        delete window_state;
        return ::DefWindowProcW(window, message, w_param, l_param);
    default:
        return ::DefWindowProcW(window, message, w_param, l_param);
    }
}

ATOM register_osk_class(HINSTANCE instance) {
    static const ATOM registered = [instance] {
        WNDCLASSEXW window_class{};
        window_class.cbSize = sizeof(window_class);
        window_class.style = CS_DROPSHADOW;  // native elevation under the keyboard

        window_class.lpfnWndProc = &osk_window_proc;
        window_class.hInstance = instance;
        window_class.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
        window_class.lpszClassName = osk_window_class;
        return ::RegisterClassExW(&window_class);
    }();
    return registered;
}

OskWindow::~OskWindow() {
    destroy();
}

bool OskWindow::create(HINSTANCE instance, Host host) {
    if (window_state_ != nullptr) {
        return true;
    }
    if (register_osk_class(instance) == 0) {
        return false;
    }
    window_state_ = new (std::nothrow) OskWindowState();
    if (window_state_ == nullptr) {
        return false;
    }
    window_state_->instance = instance;
    window_state_->host = std::move(host);
    window_state_->dpi = ui::system_dpi_for(nullptr);
    window_state_->sync_mode_from_host();

    const int width = window_state_->window_width();
    const int height = window_state_->window_height();
    const HMONITOR monitor = ::MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTONEAREST);
    MONITORINFO info{};
    info.cbSize = sizeof(info);
    int x = CW_USEDEFAULT;
    int y = CW_USEDEFAULT;
    if (::GetMonitorInfoW(monitor, &info) != FALSE) {
        x = info.rcWork.right - width - ui::scaled(window_state_->dpi, 16);
        y = info.rcWork.bottom - height - ui::scaled(window_state_->dpi, 16);
    }
    window_state_->window =
        ::CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
                          osk_window_class,
                          ui::text("osk.window.title", L"Suborno On-Screen Keyboard"),
                          WS_POPUP,
                          x,
                          y,
                          width,
                          height,
                          nullptr,
                          nullptr,
                          instance,
                          window_state_);
    if (window_state_->window == nullptr) {
        delete window_state_;
        window_state_ = nullptr;
        return false;
    }
    ui::apply_window_treatment(
        window_state_->window, width, height, ui::scaled(window_state_->dpi, 8));
    return true;
}

void OskWindow::destroy() {
    if (window_state_ != nullptr && window_state_->window != nullptr) {
        ::DestroyWindow(window_state_->window);
        window_state_ = nullptr;
    }
}

void OskWindow::show() {
    if (window_state_ == nullptr) {
        return;
    }
    window_state_->sync_mode_from_host();
    window_state_->visible_state = true;
    ::ShowWindow(window_state_->window, SW_SHOWNOACTIVATE);
}

void OskWindow::hide() {
    if (window_state_ == nullptr) {
        return;
    }
    window_state_->visible_state = false;
    ::ShowWindow(window_state_->window, SW_HIDE);
}

bool OskWindow::visible() const {
    return window_state_ != nullptr && window_state_->visible_state;
}

void OskWindow::refresh_labels() {
    if (window_state_ == nullptr) {
        return;
    }
    window_state_->sync_mode_from_host();
    // The window title follows the interface language too.

    if (window_state_->window != nullptr) {
        ::SetWindowTextW(window_state_->window,
                         ui::text("osk.window.title", L"Suborno On-Screen Keyboard"));
    }
    window_state_->repaint();
}

void OskWindow::refresh_font() {
    if (window_state_ == nullptr) {
        return;
    }
    // The next WM_PAINT calls ensure_fonts(), which rebuilds both fonts from

    // the process-wide UI font family.

    window_state_->release_fonts();
    window_state_->repaint();
}

}  // namespace suborno::platform::windows::app
