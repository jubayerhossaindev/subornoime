#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "suborno/platform/windows/app/menu_popup.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <windowsx.h>

#include "suborno/platform/windows/app/ui_accessibility.hpp"
#include "suborno/platform/windows/app/ui_theme.hpp"

namespace suborno::platform::windows::app {
namespace {

constexpr wchar_t menu_popup_class[] = L"SubornoMenuPopupWindow";
// Component-level geometry: the item height/radius/padding come from the

// shared ui token scale (ui::metrics/ui::spacing); the popup's own width and

// separator rhythm stay component-local. Width 240 so longer Bangla-capable

// family names ("Noto Sans Bengali") still fit beside the radio gutter.

constexpr int design_item_height = ui::metrics::menu_item_height;
constexpr int design_width = 240;
constexpr int design_pad_x = ui::spacing::md;
constexpr int design_radius = ui::metrics::radius_small;
constexpr int design_separator_height = 9;

// The popup state is also its UIA ElementSource: every enabled item is a

// list item the screen reader can announce (name, checked state, Invoke to

// activate). The liveness token is reset on WM_NCDESTROY so a provider UIA

// still holds answers UIA_E_ELEMENTNOTAVAILABLE instead of touching freed

// memory — the same contract the other shell surfaces follow.

struct MenuPopupState;
int sh(const MenuPopupState& state, int pixels) noexcept;
int item_top(const MenuPopupState& state, int index) noexcept;
void set_highlight(HWND window, MenuPopupState& state, int row);
void finish_modeless(HWND window, MenuPopupState& state);

struct MenuPopupState : ui_access::ElementSource {
    std::vector<MenuItem> items;
    std::vector<int> item_map;  // element index -> item index (skips separators)

    int highlighted = -1;
    int result = -1;
    UINT dpi = 96;
    HFONT font = nullptr;
    bool dismissed = false;
    bool modeless = false;
    bool closed_notified = false;
    bool mouse_tracked = false;
    HWND window_handle = nullptr;
    std::function<void(int)> on_close;
    std::shared_ptr<int> a11y_alive = std::make_shared<int>(0);

    ~MenuPopupState() override {
        if (font != nullptr) {
            ::DeleteObject(font);
            font = nullptr;
        }
    }

    int element_for_item(int item_index) const noexcept {
        for (std::size_t index = 0; index < item_map.size(); ++index) {
            if (item_map[index] == item_index) {
                return static_cast<int>(index);
            }
        }
        return -1;
    }

    // ---- UI Automation ElementSource ----

    std::wstring window_name() const override {
        return ui::text("common.menu_popup", L"Suborno menu");
    }

    int element_count() const override {
        return static_cast<int>(item_map.size());
    }

    bool element_info(int index, ui_access::ElementInfo& out) const override {
        if (index < 0 || index >= static_cast<int>(item_map.size())) {
            return false;
        }
        const int item_index = item_map[static_cast<std::size_t>(index)];
        const MenuItem& item = items[static_cast<std::size_t>(item_index)];
        const int top = item_top(*this, item_index);
        out.name = item.label;
        out.role = ui_access::ElementRole::list_item;
        out.automation_id = L"menu.item." + std::to_wstring(item_index);
        out.rect = RECT{0, top, sh(*this, design_width), top + sh(*this, design_item_height)};
        out.focusable = item.enabled;
        out.focused = highlighted == item_index;
        out.enabled = item.enabled;
        out.supports_invoke = item.enabled;
        // Radio/check markers map to the Toggle state so the checked item is

        // announced as on (a radio selection and a checkmark carry the same

        // "which one is active" information).

        out.supports_toggle = item.radio || item.checked;
        out.toggle_state_on = item.checked;
        return true;
    }

    int focused_element() const override {
        return element_for_item(highlighted);
    }

    bool focus_element(int index) override {
        if (index < 0 || index >= static_cast<int>(item_map.size())) {
            return false;
        }
        const int item_index = item_map[static_cast<std::size_t>(index)];
        if (!items[static_cast<std::size_t>(item_index)].enabled) {
            return false;
        }
        set_highlight(window_handle, *this, item_index);
        return true;
    }

    bool invoke_element(int index) override {
        if (index < 0 || index >= static_cast<int>(item_map.size())) {
            return false;
        }
        const int item_index = item_map[static_cast<std::size_t>(index)];
        if (!items[static_cast<std::size_t>(item_index)].enabled) {
            return false;
        }
        result = item_index;
        dismissed = true;
        if (modeless) {
            finish_modeless(window_handle, *this);
        }
        return true;
    }

    bool toggle_element(int index) override {
        // Toggling a checked/radio item selects it, same as invoking it.

        return invoke_element(index);
    }

    bool expand_element(int /*index*/, bool /*expand*/) override {
        return false;
    }

    bool set_element_value(int /*index*/, double /*value*/) override {
        return false;
    }
};

int sh(const MenuPopupState& state, int pixels) noexcept {
    return ui::scaled(state.dpi, pixels);
}

// Returns the item index under a client point, or -1.

int hit_item(const MenuPopupState& state, int y) noexcept {
    int cursor = sh(state, 4);
    const int item_height = sh(state, design_item_height);
    const int separator_height = sh(state, design_separator_height);
    for (std::size_t index = 0; index < state.items.size(); ++index) {
        if (state.items[index].separator) {
            cursor += separator_height;
            continue;
        }
        if (y >= cursor && y < cursor + item_height) {
            return static_cast<int>(index);
        }
        cursor += item_height;
    }
    return -1;
}

int item_top(const MenuPopupState& state, int index) noexcept {
    int cursor = sh(state, 4);
    for (int item_index = 0;
         item_index < index && item_index < static_cast<int>(state.items.size());
         ++item_index) {
        cursor += state.items[static_cast<std::size_t>(item_index)].separator
                      ? sh(state, design_separator_height)
                      : sh(state, design_item_height);
    }
    return cursor;
}

SIZE measure(const MenuPopupState& state) noexcept {
    int height = sh(state, 8);
    for (const MenuItem& item : state.items) {
        height +=
            item.separator ? sh(state, design_separator_height) : sh(state, design_item_height);
    }
    return SIZE{sh(state, design_width), height};
}

void paint(HWND window, MenuPopupState& state) {
    // `ps` keeps the local PAINTSTRUCT from shadowing this paint() function.

    PAINTSTRUCT ps{};
    HDC device_context = ::BeginPaint(window, &ps);
    if (device_context == nullptr) {
        return;
    }
    RECT client{};
    ::GetClientRect(window, &client);
    ui::fill_rounded(device_context, client, ui::theme_surface(), sh(state, design_radius));
    if (state.font != nullptr) {
        ::SelectObject(device_context, state.font);
    }
    const int item_height = sh(state, design_item_height);
    const int pad_x = sh(state, design_pad_x);
    for (std::size_t index = 0; index < state.items.size(); ++index) {
        const MenuItem& item = state.items[index];
        const int y = item_top(state, static_cast<int>(index));
        if (item.separator) {
            const int mid = y + sh(state, design_separator_height) / 2;
            RECT line{pad_x, mid, client.right - pad_x, mid + 1};
            HBRUSH brush = ::CreateSolidBrush(ui::theme_separator());
            if (brush != nullptr) {
                ::FillRect(device_context, &line, brush);
                ::DeleteObject(brush);
            }
            continue;
        }
        if (static_cast<int>(index) == state.highlighted && item.enabled) {
            RECT pill{sh(state, 4),
                      y + sh(state, 2),
                      client.right - sh(state, 4),
                      y + item_height - sh(state, 2)};
            ui::fill_rounded(device_context, pill, ui::theme_surface_hover(), sh(state, 4));
        }
        const int gutter = pad_x + sh(state, 12);
        if (item.radio && item.checked) {
            RECT dot{gutter - sh(state, 5),
                     y + item_height / 2 - sh(state, 5),
                     gutter + sh(state, 5),
                     y + item_height / 2 + sh(state, 5)};
            ui::fill_rounded(device_context, dot, ui::theme_text(), sh(state, 5));
        } else if (!item.radio && item.checked) {
            const POINT points[3]{{gutter - sh(state, 10), y + item_height / 2 + 1},
                                  {gutter - sh(state, 4), y + item_height / 2 + sh(state, 6)},
                                  {gutter + sh(state, 6), y + item_height / 2 - sh(state, 6)}};
            HPEN pen = ::CreatePen(PS_SOLID, std::max(1, sh(state, 2)), ui::theme_text());
            const HGDIOBJ previous = ::SelectObject(device_context, pen);
            ::Polyline(device_context, points, 3);
            ::SelectObject(device_context, previous);
            ::DeleteObject(pen);
        }
        RECT text_rect{gutter + sh(state, 8), y, client.right - pad_x, y + item_height};
        ui::draw_text(device_context,
                      item.label.c_str(),
                      text_rect,
                      item.enabled ? ui::theme_text() : ui::theme_text_inactive(),
                      DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    }
    ::EndPaint(window, &ps);
}

void set_highlight(HWND window, MenuPopupState& state, int row) {
    if (row != state.highlighted) {
        state.highlighted = row;
        ::InvalidateRect(window, nullptr, FALSE);
        ::UpdateWindow(window);
        // The highlight IS the menu's keyboard focus; announce every change

        // (arrow keys and hover alike) so screen readers follow the menu.

        ui_access::notify_focus_changed(window, &state, state.a11y_alive);
    }
}

void move_highlight(HWND window, MenuPopupState& state, int direction) {
    const int count = static_cast<int>(state.items.size());
    if (count == 0) {
        return;
    }
    int row = state.highlighted;
    for (int step = 0; step < count; ++step) {
        row = direction < 0 ? row - 1 : row + 1;
        if (row < 0) {
            row = count - 1;
        }
        if (row >= count) {
            row = 0;
        }
        const MenuItem& item = state.items[static_cast<std::size_t>(row)];
        if (!item.separator && item.enabled) {
            break;
        }
    }
    set_highlight(window, state, row);
}

void finish_modeless(HWND window, MenuPopupState& state) {
    if (!state.modeless || state.closed_notified) {
        return;
    }
    state.closed_notified = true;
    state.dismissed = true;
    const int chosen = state.result;
    std::function<void(int)> callback = std::move(state.on_close);
    if (window != nullptr && ::IsWindow(window) != FALSE) {
        ::DestroyWindow(window);
    }
    if (callback) {
        callback(chosen);
    }
}

LRESULT CALLBACK menu_popup_proc(HWND window, UINT message, WPARAM w_param, LPARAM l_param) {
    auto* state = reinterpret_cast<MenuPopupState*>(::GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(l_param);
        state = static_cast<MenuPopupState*>(create->lpCreateParams);
        ::SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
        state->window_handle = window;
    }
    if (state == nullptr) {
        return ::DefWindowProcW(window, message, w_param, l_param);
    }
    switch (message) {
    case WM_GETOBJECT: {
        bool handled = false;
        const LRESULT result = ui_access::handle_get_object(
            window, w_param, l_param, state, state->a11y_alive, handled);
        if (handled) {
            return result;
        }
        return ::DefWindowProcW(window, message, w_param, l_param);
    }
    case WM_DESTROY:
        if (state->font != nullptr) {
            ::DeleteObject(state->font);
            state->font = nullptr;
        }
        if (state->modeless && !state->closed_notified) {
            state->closed_notified = true;
            state->dismissed = true;
            const int chosen = state->result;
            std::function<void(int)> callback = std::move(state->on_close);
            if (callback) {
                callback(chosen);
            }
        }
        return ::DefWindowProcW(window, message, w_param, l_param);
    case WM_NCDESTROY:
        state->a11y_alive.reset();
        state->window_handle = nullptr;
        ::SetWindowLongPtrW(window, GWLP_USERDATA, 0);
        if (state->modeless) {
            delete state;
        }
        return ::DefWindowProcW(window, message, w_param, l_param);
    case WM_PAINT:
        paint(window, *state);
        return 0;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_MOUSEMOVE:
        if (!state->mouse_tracked) {
            TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE, window, 0};
            if (::TrackMouseEvent(&track) != FALSE) {
                state->mouse_tracked = true;
            }
        }
        set_highlight(window, *state, hit_item(*state, GET_Y_LPARAM(l_param)));
        return 0;
    case WM_MOUSELEAVE:
        state->mouse_tracked = false;
        // The hover pill used to stick to the last row after the pointer

        // left; clear it so only real hover highlights.

        set_highlight(window, *state, -1);
        return 0;
    case WM_LBUTTONDOWN: {
        const int row = hit_item(*state, GET_Y_LPARAM(l_param));
        if (row >= 0 && state->items[static_cast<std::size_t>(row)].enabled) {
            state->result = row;
        }
        state->dismissed = true;
        if (state->modeless) {
            finish_modeless(window, *state);
        }
        return 0;
    }
    case WM_KEYDOWN:
        if (w_param == VK_ESCAPE) {
            state->dismissed = true;
            if (state->modeless) {
                finish_modeless(window, *state);
            }
        } else if (w_param == VK_UP) {
            move_highlight(window, *state, -1);
        } else if (w_param == VK_DOWN) {
            move_highlight(window, *state, 1);
        } else if (w_param == VK_RETURN) {
            if (state->highlighted >= 0) {
                state->result = state->highlighted;
            }
            state->dismissed = true;
            if (state->modeless) {
                finish_modeless(window, *state);
            }
        }
        return 0;
    case WM_KILLFOCUS:
        state->dismissed = true;
        if (state->modeless) {
            finish_modeless(window, *state);
        }
        return 0;
    default:
        return ::DefWindowProcW(window, message, w_param, l_param);
    }
}

ATOM register_class(HINSTANCE instance) {
    static const ATOM registered = [instance] {
        WNDCLASSEXW window_class{};
        window_class.cbSize = sizeof(window_class);
        window_class.lpfnWndProc = &menu_popup_proc;
        window_class.hInstance = instance;
        window_class.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
        window_class.lpszClassName = menu_popup_class;
        return ::RegisterClassExW(&window_class);
    }();
    return registered;
}

std::unique_ptr<MenuPopupState> create_popup_state(HWND owner,
                                                     const std::vector<MenuItem>& items,
                                                     int selected,
                                                     bool modeless,
                                                     std::function<void(int)> on_close) {
    auto state = std::make_unique<MenuPopupState>();
    state->items = items;
    state->highlighted = selected;
    state->dpi = ui::system_dpi_for(owner);
    state->font = ui::create_message_font();
    state->modeless = modeless;
    state->on_close = std::move(on_close);
    for (std::size_t index = 0; index < state->items.size(); ++index) {
        if (!state->items[index].separator) {
            state->item_map.push_back(static_cast<int>(index));
        }
    }
    return state;
}

HWND create_popup_window(HINSTANCE instance, HWND owner, MenuPopupState& state, POINT anchor) {
    const SIZE size = measure(state);
    const HMONITOR monitor = ::MonitorFromPoint(anchor, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitor_info{};
    monitor_info.cbSize = sizeof(monitor_info);
    if (::GetMonitorInfoW(monitor, &monitor_info) != FALSE) {
        anchor.x = std::max(monitor_info.rcWork.left,
                            std::min(anchor.x, monitor_info.rcWork.right - size.cx));
        if (anchor.y + size.cy > monitor_info.rcWork.bottom) {
            anchor.y = std::max(monitor_info.rcWork.top, anchor.y - size.cy);
        }
    }

    const HWND window = ::CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
                                          menu_popup_class,
                                          state.window_name().c_str(),
                                          WS_POPUP,
                                          anchor.x,
                                          anchor.y,
                                          size.cx,
                                          size.cy,
                                          owner,
                                          nullptr,
                                          instance,
                                          &state);
    if (window == nullptr) {
        return nullptr;
    }
    ui::apply_rounded_region(window, size.cx, size.cy, sh(state, design_radius));
    return window;
}

}  // namespace

int show_menu_popup(HWND owner, const std::vector<MenuItem>& items, POINT anchor, int selected) {
    HINSTANCE instance = ::GetModuleHandleW(nullptr);
    if (register_class(instance) == 0) {
        return -1;
    }
    const auto state = create_popup_state(owner, items, selected, false, nullptr);
    const HWND window = create_popup_window(instance, owner, *state, anchor);
    if (window == nullptr) {
        return -1;
    }
    ::ShowWindow(window, SW_SHOWNOACTIVATE);
    ::SetFocus(window);  // receives keyboard directly (no WS_EX_NOACTIVATE)

    MSG message{};
    while (!state->dismissed && ::GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (message.message >= WM_KEYFIRST && message.message <= WM_KEYLAST
            && message.hwnd != window) {
            // Route keystrokes to the popup even when another window has focus.

            ::SendMessageW(window, message.message, message.wParam, message.lParam);
            continue;
        }
        ::TranslateMessage(&message);
        ::DispatchMessageW(&message);
    }
    ::DestroyWindow(window);
    if (state->font != nullptr) {
        ::DeleteObject(state->font);
        state->font = nullptr;
    }
    return state->result;
}

HWND open_menu_popup(HWND owner,
                     const std::vector<MenuItem>& items,
                     POINT anchor,
                     int selected,
                     std::function<void(int)> on_close,
                     bool focus_popup) {
    HINSTANCE instance = ::GetModuleHandleW(nullptr);
    if (register_class(instance) == 0) {
        return nullptr;
    }
    auto state = create_popup_state(owner, items, selected, true, std::move(on_close));
    const HWND window = create_popup_window(instance, owner, *state, anchor);
    if (window == nullptr) {
        return nullptr;
    }
    // Ownership transfers to menu_popup_proc, which deletes the state on WM_NCDESTROY.
    (void)state.release();
    ::ShowWindow(window, SW_SHOWNOACTIVATE);
    if (focus_popup && (owner == nullptr || ::GetForegroundWindow() == owner)) {
        ::SetFocus(window);
    }
    return window;
}

void close_menu_popup(HWND popup) {
    if (popup != nullptr && ::IsWindow(popup) != FALSE) {
        ::DestroyWindow(popup);
    }
}

}  // namespace suborno::platform::windows::app
