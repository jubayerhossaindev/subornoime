#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "suborno/platform/windows/app/topbar.hpp"

#include <algorithm>
#include <cmath>
#include <commctrl.h>
#include <cstdlib>
#include <cwchar>
#include <memory>
#include <string>
#include <windowsx.h>

#include "suborno/platform/windows/app/menu_popup.hpp"
#include "suborno/platform/windows/app/ui_accessibility.hpp"
#include "suborno/platform/windows/app/ui_theme.hpp"

namespace suborno::platform::windows::app {
namespace {

constexpr wchar_t topbar_class[] = L"SubornoTopbarWindow";
constexpr UINT idle_timer_id = 1;
constexpr int design_height = 32;
constexpr int design_radius = ui::metrics::radius_pill;
// Pill geometry (96-DPI design px): icon 12..28, label from 34; the wrench

// and close zones are anchored to the RIGHT edge so the pill can auto-size

// to its label (long Bangla layout names like "বাংলা (Borno)" stay visible).

// Zone widths follow the native caption-button convention (46x32 design px

// would be exact; 40 keeps the pill compact while giving the icon-only

// zones a usable hit target per the Fluent 40x40 hit-target guidance).

constexpr int design_min_width = 150;
constexpr int design_max_width = 360;
constexpr int design_label_left = 34;
constexpr int design_label_right_gap = 92;
constexpr int design_close_span = 40;
constexpr int design_wrench_span = 84;

// Wrench glyph color comes from the theme token accent_warm (the measured

// reference value for both palettes, HC derives its own).

}  // namespace

namespace {
// Pill helpers defined below, with the rest of the pill code. The TopbarWindowState's

// UIA and input paths call them, so they must be declared before the TopbarWindowState:

// member function bodies resolve free functions at their point of use.

const wchar_t* tooltip_text(int zone);
void handle_click(Topbar::TopbarWindowState& window_state, int zone);
void update_tooltip_layout(Topbar::TopbarWindowState& window_state);
}  // namespace

struct Topbar::TopbarWindowState : public ui_access::ElementSource {
    Host host;
    HWND window = nullptr;
    HINSTANCE instance = nullptr;
    UINT dpi = 96;
    HFONT font = nullptr;
    std::wstring layout_label;
    int window_width = 0;  // current client width in device pixels

    TopbarSettingsSnapshot settings;
    bool drag_pending = false;
    POINT drag_start{};
    bool moved_while_down = false;
    int down_zone = -1;
    // Hover feedback for the click zones (wrench/close were invisible until

    // clicked) and the last alpha sent to SetLayeredWindowAttributes, so the

    // 500 ms idle timer skips redundant layered-attribute calls.

    int hover_zone = -1;
    bool mouse_tracked = false;
    int current_alpha = -1;
    // Native tooltips for the icon-only zones (the Win32 accessibility

    // guidance: icon-only controls need tooltips alongside accessible

    // names). TTF_SUBCLASS rect tools — no message relay needed.

    HWND tooltip = nullptr;
    std::wstring tooltip_texts[4];
    // Liveness token for the UIA providers (reset in WM_DESTROY).

    std::shared_ptr<int> a11y_alive = std::make_shared<int>(0);

    // ---- UI Automation ElementSource ----

    // The four click zones as invokable buttons: 0 = layout label (layout

    // menu), 1 = wrench (app menu), 2 = close (tray/exit menu), 3 = mode

    // icon (English↔বাংলা toggle). The pill itself is drag/click driven,

    // so zones are announced but not keyboard-focusable.

    std::wstring window_name() const override {
        return L"Suborno";
    }

    int element_count() const override {
        return 4;
    }

    bool element_info(int index, ui_access::ElementInfo& out) const override {
        const int height = ui::scaled(dpi, design_height);
        const int inset = ui::scaled(dpi, 3);
        out.role = ui_access::ElementRole::button;
        out.supports_invoke = true;
        // The accessible description doubles as the tooltip text: the same

        // localized sentence serves pointer discovery and screen readers.

        out.help_text = tooltip_text(index);
        switch (index) {
        case 0:  // label: opens the keyboard-layout menu

            out.name = layout_label.empty() ? ui::text("topbar.menu.layout", L"Keyboard layout")
                                            : layout_label;
            out.automation_id = L"zone.layout";
            out.rect = RECT{ui::scaled(dpi, design_label_left),
                            inset,
                            window_width - ui::scaled(dpi, design_wrench_span),
                            height - inset};
            return true;
        case 1:  // wrench: app menu (primary action = Settings)

            out.name = ui::text("topbar.menu.settings", L"Settings…");
            out.automation_id = L"zone.wrench";
            out.rect = RECT{window_width - ui::scaled(dpi, design_wrench_span),
                            inset,
                            window_width - ui::scaled(dpi, design_close_span),
                            height - inset};
            return true;
        case 2:  // close: menu (primary action = System Tray)

            out.name = ui::text("topbar.menu.switch_to_tray", L"System Tray");
            out.automation_id = L"zone.close";
            out.rect = RECT{window_width - ui::scaled(dpi, design_close_span),
                            inset,
                            window_width,
                            height - inset};
            return true;
        case 3:  // mode icon: English↔বাংলা quick toggle

            out.name = ui::text("topbar.mode.toggle", L"Toggle English or Bangla");
            out.automation_id = L"zone.mode";
            out.rect = RECT{inset, inset, ui::scaled(dpi, design_label_left), height - inset};
            return true;
        default:
            return false;
        }
    }

    int focused_element() const override {
        return -1;
    }

    bool focus_element(int) override {
        return false;
    }

    bool invoke_element(int index) override {
        if (index < 0 || index > 3) {
            return false;
        }
        handle_click(*this, index);
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

namespace {
// Localized tooltip / accessible description for a click zone.

const wchar_t* tooltip_text(int zone) {
    switch (zone) {
    case 0:
        return ui::text("topbar.tooltip.layout", L"Switch the keyboard layout");
    case 1:
        return ui::text("topbar.tooltip.settings", L"Settings, font, and more");
    case 2:
        return ui::text("topbar.tooltip.tray", L"Send to the system tray or exit");
    default:
        return ui::text("topbar.tooltip.toggle", L"Switch between English and Bangla");
    }
}
}  // namespace

namespace {

int sh(const Topbar::TopbarWindowState& window_state, int pixels) noexcept {
    return ui::scaled(window_state.dpi, pixels);
}

// Measures the label with the pill's font and returns the required client

// width in device pixels, clamped to the design range.

int measure_width(const Topbar::TopbarWindowState& window_state) {
    int label_px = 0;
    if (!window_state.layout_label.empty()) {
        HDC device_context = ::GetDC(nullptr);
        if (device_context != nullptr) {
            HGDIOBJ previous_font = nullptr;
            if (window_state.font != nullptr) {
                previous_font = ::SelectObject(device_context, window_state.font);
            }
            SIZE extent{};
            if (::GetTextExtentPoint32W(device_context,
                                        window_state.layout_label.c_str(),
                                        static_cast<int>(window_state.layout_label.size()),
                                        &extent)
                != FALSE) {
                label_px = extent.cx;
            }
            if (previous_font != nullptr) {
                ::SelectObject(device_context, previous_font);
            }
            ::ReleaseDC(nullptr, device_context);
        }
    }
    const int minimum = sh(window_state, design_min_width);
    const int maximum = sh(window_state, design_max_width);
    const int required =
        sh(window_state, design_label_left) + label_px + sh(window_state, design_label_right_gap);
    return std::max(minimum, std::min(maximum, required));
}

// Resizes the pill to fit the current label and refreshes its rounded region.

void update_window_width(Topbar::TopbarWindowState& window_state) {
    if (window_state.window == nullptr) {
        return;
    }
    const int width = measure_width(window_state);
    const int height = sh(window_state, design_height);
    if (width != window_state.window_width) {
        window_state.window_width = width;
        RECT window_rect{};
        ::GetWindowRect(window_state.window, &window_rect);
        ::SetWindowPos(window_state.window,
                       HWND_TOPMOST,
                       window_rect.left,
                       window_rect.top,
                       width,
                       height,
                       SWP_NOACTIVATE);
        ui::apply_window_treatment(
            window_state.window, width, height, sh(window_state, design_radius));
        update_tooltip_layout(window_state);
    }
}

// Click zones in client coordinates (device px): 0 = label (layout menu),

// 1 = wrench, 2 = close, 3 = mode icon (English↔বাংলা quick toggle). Wrench

// and close hug the right edge of the auto-sized pill.

int zone_from_point(const Topbar::TopbarWindowState& window_state, int x, int y) noexcept {
    const int height = sh(window_state, design_height);
    if (y < 0 || y >= height || x < 0 || x >= window_state.window_width) {
        return -1;
    }
    if (x >= window_state.window_width - sh(window_state, design_close_span)) {
        return 2;
    }
    if (x >= window_state.window_width - sh(window_state, design_wrench_span)) {
        return 1;
    }
    if (x < sh(window_state, design_label_left)) {
        return 3;  // the mode icon toggles English ↔ Bangla directly
    }
    return 0;  // the label opens the keyboard-layout menu
}

// The client-rect tool rectangle for one zone (mirrors zone_from_point and

// the UIA rects — one geometry, three consumers).

RECT zone_tool_rect(const Topbar::TopbarWindowState& window_state, int zone) noexcept {
    const int height = sh(window_state, design_height);
    const int inset = sh(window_state, 3);
    switch (zone) {
    case 3:
        return RECT{inset, inset, sh(window_state, design_label_left), height - inset};
    case 1:
        return RECT{window_state.window_width - sh(window_state, design_wrench_span),
                    inset,
                    window_state.window_width - sh(window_state, design_close_span),
                    height - inset};
    case 2:
        return RECT{window_state.window_width - sh(window_state, design_close_span),
                    inset,
                    window_state.window_width,
                    height - inset};
    default:
        return RECT{sh(window_state, design_label_left),
                    inset,
                    window_state.window_width - sh(window_state, design_wrench_span),
                    height - inset};
    }
}

// Refreshes the localized tool texts (on creation and after a language

// switch). Text storage lives in the TopbarWindowState so the pointers stay valid.

void update_tooltip_texts(Topbar::TopbarWindowState& window_state) {
    if (window_state.tooltip == nullptr) {
        return;
    }
    for (int zone = 0; zone < 4; ++zone) {
        window_state.tooltip_texts[zone] = tooltip_text(zone);
        TOOLINFO info{};
        info.cbSize = sizeof(info);
        info.uFlags = TTF_SUBCLASS;
        info.hwnd = window_state.window;
        info.uId = static_cast<UINT_PTR>(zone);
        info.rect = zone_tool_rect(window_state, zone);
        info.lpszText = window_state.tooltip_texts[zone].data();
        if (::SendMessageW(window_state.tooltip, TTM_GETTOOLCOUNT, 0, 0) <= zone) {
            ::SendMessageW(window_state.tooltip, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&info));
        } else {
            ::SendMessageW(
                window_state.tooltip, TTM_SETTOOLINFO, 0, reinterpret_cast<LPARAM>(&info));
        }
    }
}

// Re-syncs the tool rectangles after a width or DPI change.

void update_tooltip_layout(Topbar::TopbarWindowState& window_state) {
    if (window_state.tooltip == nullptr) {
        return;
    }
    for (int zone = 0; zone < 4; ++zone) {
        TOOLINFO info{};
        info.cbSize = sizeof(info);
        info.hwnd = window_state.window;
        info.uId = static_cast<UINT_PTR>(zone);
        info.rect = zone_tool_rect(window_state, zone);
        ::SendMessageW(window_state.tooltip, TTM_NEWTOOLRECT, 0, reinterpret_cast<LPARAM>(&info));
    }
}

// Creates the native tooltip control for the four icon-only zones. Rect

// tools with TTF_SUBCLASS track the pointer themselves — no relay wiring,

// no timers, no extra message traffic in the paint path.

void create_tooltip(Topbar::TopbarWindowState& window_state) {
    if (window_state.tooltip != nullptr || window_state.window == nullptr) {
        return;
    }
    // The tooltip is a bar-class common control: the common-control DLL

    // registers TOOLTIPS_CLASS only after InitCommonControlsEx (see MSDN,

    // "Creating Common Controls"). One idempotent process-wide init is

    // enough; a failure just leaves tooltip == nullptr (optional UI).

    static const bool common_controls_registered = [] {
        INITCOMMONCONTROLSEX init{sizeof(INITCOMMONCONTROLSEX), ICC_BAR_CLASSES};
        return ::InitCommonControlsEx(&init) != FALSE;
    }();
    (void)common_controls_registered;
    window_state.tooltip = ::CreateWindowExW(WS_EX_TOPMOST,
                                             TOOLTIPS_CLASS,
                                             nullptr,
                                             // TTS_ALWAYSTIP: the pill is never active

                                             // long enough to take focus, so the

                                             // tooltip must show while it is inactive.

                                             WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
                                             CW_USEDEFAULT,
                                             CW_USEDEFAULT,
                                             CW_USEDEFAULT,
                                             CW_USEDEFAULT,
                                             window_state.window,
                                             nullptr,
                                             window_state.instance,
                                             nullptr);
    if (window_state.tooltip == nullptr) {
        return;
    }
    ::SendMessageW(window_state.tooltip, TTM_SETMAXTIPWIDTH, 0, sh(window_state, 220));
    update_tooltip_texts(window_state);
}

void paint(Topbar::TopbarWindowState& window_state, HDC device_context) {
    RECT client{};
    ::GetClientRect(window_state.window, &client);
    HBRUSH background = ::CreateSolidBrush(ui::theme_surface_topbar());
    if (background != nullptr) {
        ::FillRect(device_context, &client, background);
        ::DeleteObject(background);
    }
    ui::frame_rounded(
        device_context, client, ui::theme_separator(), sh(window_state, design_radius));

    // Interaction pill for the click zones; zone geometry mirrors

    // zone_from_point(). Fluent state model: hover brightens, press dims

    // (theme_surface_pressed) — the pressed face is drawn on top of the

    // hover face for the zone currently held down.

    const auto zone_pill = [&window_state](int zone) {
        const int height = sh(window_state, design_height);
        const int inset = sh(window_state, 3);
        switch (zone) {
        case 3:  // mode icon

            return RECT{inset, inset, sh(window_state, design_label_left), height - inset};
        case 0:  // label

            return RECT{sh(window_state, design_label_left),
                        inset,
                        window_state.window_width - sh(window_state, design_wrench_span),
                        height - inset};
        case 1:  // wrench

            return RECT{window_state.window_width - sh(window_state, design_wrench_span),
                        inset,
                        window_state.window_width - sh(window_state, design_close_span),
                        height - inset};
        case 2:  // close

            return RECT{window_state.window_width - sh(window_state, design_close_span),
                        inset,
                        window_state.window_width,
                        height - inset};
        default:
            return RECT{0, 0, 0, 0};
        }
    };
    for (const int zone : {window_state.hover_zone, window_state.down_zone}) {
        if (zone < 0) {
            continue;
        }
        const RECT zone_rect = zone_pill(zone);
        if (zone_rect.right > zone_rect.left) {
            ui::fill_rounded(device_context,
                             zone_rect,
                             zone == window_state.down_zone ? ui::theme_surface_pressed()
                                                            : ui::theme_surface_hover(),
                             sh(window_state, 4));
        }
    }

    // Mode icon: accent circle whose glyph shows the CURRENT typing mode —

    // "A" for English passthrough, ব for a Bangla layout. Clicking it is the

    // one-tap English↔বাংলা toggle (feature F-01 from the UI audit findings).

    const int icon_cx = sh(window_state, 20);
    const int icon_cy = sh(window_state, design_height) / 2;
    const int radius = sh(window_state, 8);
    HBRUSH accent = ::CreateSolidBrush(ui::theme_accent_fill());
    if (accent != nullptr) {
        HPEN pen = ::CreatePen(PS_SOLID, 0, ui::theme_accent());
        const HGDIOBJ previous_pen = ::SelectObject(device_context, pen);
        const HGDIOBJ previous_brush = ::SelectObject(device_context, accent);
        ::Ellipse(
            device_context, icon_cx - radius, icon_cy - radius, icon_cx + radius, icon_cy + radius);
        ::SelectObject(device_context, previous_brush);
        ::SelectObject(device_context, previous_pen);
        ::DeleteObject(pen);
        ::DeleteObject(accent);
    }
    const bool english_mode = window_state.host.active_layout_id != nullptr
                              && window_state.host.active_layout_id() == "english";
    const wchar_t* mode_glyph = english_mode ? L"A" : L"ব";
    const RECT mode_glyph_rect{
        icon_cx - radius, icon_cy - radius, icon_cx + radius, icon_cy + radius};
    ui::draw_text(device_context,
                  mode_glyph,
                  mode_glyph_rect,
                  ui::theme_on_accent(),
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    if (window_state.font != nullptr) {
        ::SelectObject(device_context, window_state.font);
    }
    RECT label_rect{sh(window_state, design_label_left),
                    0,
                    window_state.window_width - sh(window_state, design_label_right_gap),
                    sh(window_state, design_height)};
    ui::draw_text(device_context, window_state.layout_label.c_str(), label_rect, ui::theme_text());

    // Wrench glyph (simple GDI wrench silhouette), right-anchored.

    const int wx =
        window_state.window_width - sh(window_state, (design_wrench_span + design_close_span) / 2);
    const int wy = sh(window_state, design_height) / 2;
    HPEN wrench_pen = ::CreatePen(PS_SOLID, sh(window_state, 3), ui::theme_accent_warm());
    if (wrench_pen != nullptr) {
        const HGDIOBJ previous = ::SelectObject(device_context, wrench_pen);
        ::MoveToEx(device_context, wx - sh(window_state, 4), wy + sh(window_state, 4), nullptr);
        ::LineTo(device_context, wx + sh(window_state, 4), wy - sh(window_state, 4));
        // Wrench head: small open circle at the top-right end.

        HBRUSH fill = ::CreateSolidBrush(ui::theme_surface_topbar());
        HPEN outline = ::CreatePen(PS_SOLID, sh(window_state, 2), ui::theme_accent_warm());
        if (fill != nullptr && outline != nullptr) {
            const HGDIOBJ previous_pen = ::SelectObject(device_context, outline);
            const HGDIOBJ previous_brush = ::SelectObject(device_context, fill);
            ::Ellipse(device_context,
                      wx + sh(window_state, 1),
                      wy - sh(window_state, 7),
                      wx + sh(window_state, 8),
                      wy);
            ::SelectObject(device_context, previous_brush);
            ::SelectObject(device_context, previous_pen);
        }
        if (fill != nullptr) {
            ::DeleteObject(fill);
        }
        if (outline != nullptr) {
            ::DeleteObject(outline);
        }
        ::SelectObject(device_context, previous);
        ::DeleteObject(wrench_pen);
    }

    // Close glyph: two crossing lines, right-anchored.

    const int cx = window_state.window_width - sh(window_state, design_close_span / 2);
    const int cy = sh(window_state, design_height) / 2;
    const int arm = sh(window_state, 5);
    HPEN close_pen = ui::create_round_pen(sh(window_state, 2), ui::theme_text());
    if (close_pen != nullptr) {
        const HGDIOBJ previous = ::SelectObject(device_context, close_pen);
        ::MoveToEx(device_context, cx - arm, cy - arm, nullptr);
        ::LineTo(device_context, cx + arm, cy + arm);
        ::MoveToEx(device_context, cx + arm, cy - arm, nullptr);
        ::LineTo(device_context, cx - arm, cy + arm);
        ::SelectObject(device_context, previous);
        ::DeleteObject(close_pen);
    }
}

void anchor_for_zone(const Topbar::TopbarWindowState& window_state,
                     int zone,
                     POINT& anchor) noexcept {
    RECT window_rect{};
    ::GetWindowRect(window_state.window, &window_rect);
    int x = 0;
    if (zone == 1) {
        x = window_state.window_width - sh(window_state, design_wrench_span);
    } else if (zone == 2) {
        x = window_state.window_width - sh(window_state, design_close_span);
    }
    anchor.x = window_rect.left + x;
    anchor.y = window_rect.bottom + sh(window_state, 4);
}

// The wrench menu's "Change Font…" entry opens this secondary list anchored

// under the pill; picking a family applies it across the whole app.

void show_font_menu(Topbar::TopbarWindowState& window_state, const POINT& anchor) {
    if (!window_state.host.fonts || !window_state.host.active_font
        || !window_state.host.select_font) {
        return;
    }
    const std::vector<std::wstring> fonts = window_state.host.fonts();
    const std::wstring active = window_state.host.active_font();
    std::vector<MenuItem> items;
    items.reserve(fonts.size() + 1);
    {
        MenuItem system_default;
        system_default.label = ui::text("topbar.menu.font_system_default", L"System Default");
        system_default.radio = true;
        system_default.checked = active.empty();
        items.push_back(std::move(system_default));
    }
    int selected = active.empty() ? 0 : -1;
    for (const std::wstring& family : fonts) {
        MenuItem item;
        item.label = family;
        item.radio = true;
        item.checked = ::_wcsicmp(active.c_str(), family.c_str()) == 0;
        if (item.checked) {
            selected = static_cast<int>(items.size());
        }
        items.push_back(std::move(item));
    }
    const int chosen = show_menu_popup(window_state.window, items, anchor, selected);
    if (chosen == 0) {
        window_state.host.select_font(std::wstring());
    } else if (chosen > 0 && chosen - 1 < static_cast<int>(fonts.size())) {
        window_state.host.select_font(fonts[static_cast<std::size_t>(chosen - 1)]);
    }
}

void handle_click(Topbar::TopbarWindowState& window_state, int zone) {
    POINT anchor{};
    anchor_for_zone(window_state, zone, anchor);
    if (zone == 0) {
        std::vector<std::pair<std::string, std::wstring>> layouts;
        if (window_state.host.layouts != nullptr) {
            layouts = window_state.host.layouts();
        }
        std::vector<MenuItem> items;
        items.reserve(layouts.size());
        const std::string active = window_state.host.active_layout_id
                                       ? window_state.host.active_layout_id()
                                       : std::string();
        int selected = -1;
        for (std::size_t index = 0; index < layouts.size(); ++index) {
            MenuItem item;
            item.label = layouts[index].second;
            item.radio = true;
            item.checked = layouts[index].first == active;
            if (item.checked) {
                selected = static_cast<int>(index);
            }
            items.push_back(std::move(item));
        }
        const int chosen = show_menu_popup(window_state.window, items, anchor, selected);
        if (chosen >= 0 && chosen < static_cast<int>(layouts.size())) {
            window_state.host.select_layout(layouts[static_cast<std::size_t>(chosen)].first);
        }
    } else if (zone == 1) {
        std::vector<MenuItem> items;
        items.push_back(MenuItem{ui::text("topbar.menu.settings", L"Settings…")});
        items.push_back(MenuItem{ui::text("topbar.menu.osk", L"On-Screen Keyboard…")});
        items.push_back(MenuItem{ui::text("topbar.menu.change_font", L"Change Font…")});
        items.push_back(MenuItem{ui::text("topbar.menu.about", L"About Suborno…")});
        const int chosen = show_menu_popup(window_state.window, items, anchor, -1);
        if (chosen == 0 && window_state.host.open_settings != nullptr) {
            window_state.host.open_settings();
        } else if (chosen == 1 && window_state.host.open_osk != nullptr) {
            window_state.host.open_osk();
        } else if (chosen == 2) {
            show_font_menu(window_state, anchor);
        } else if (chosen == 3 && window_state.host.open_about != nullptr) {
            window_state.host.open_about();
        }
    } else if (zone == 2) {
        std::vector<MenuItem> items;
        items.push_back(MenuItem{ui::text("topbar.menu.switch_to_tray", L"System Tray")});
        items.push_back(MenuItem{ui::text("topbar.menu.exit", L"Exit")});
        const int chosen = show_menu_popup(window_state.window, items, anchor, -1);
        if (chosen == 0 && window_state.host.switch_to_tray != nullptr) {
            window_state.host.switch_to_tray();
        } else if (chosen == 1 && window_state.host.exit_application != nullptr) {
            window_state.host.exit_application();
        }
    } else if (zone == 3 && window_state.host.toggle_english_bangla != nullptr) {
        window_state.host.toggle_english_bangla();
    }
}

void update_idle_state(Topbar::TopbarWindowState& window_state) {
    if (window_state.window == nullptr) {
        return;
    }
    BYTE alpha = 255;
    if (window_state.settings.idle_dimming_enabled) {
        LASTINPUTINFO info{};
        info.cbSize = sizeof(info);
        if (::GetLastInputInfo(&info) != FALSE) {
            // GetTickCount64 (C28159): 64-bit ticks never wrap the way the

            // 32-bit GetTickCount does after 49 days of uptime.

            const ULONGLONG idle_milliseconds = ::GetTickCount64() - info.dwTime;
            if (idle_milliseconds > window_state.settings.idle_time_seconds * 1000ULL
                && window_state.settings.idle_opacity_percent < 100) {
                alpha = static_cast<BYTE>(window_state.settings.idle_opacity_percent * 255U / 100U);
            }
        }
    }
    // The timer runs every 500 ms; skip the layered-attribute call when the

    // alpha has not changed since the last update.

    if (static_cast<int>(alpha) != window_state.current_alpha) {
        window_state.current_alpha = static_cast<int>(alpha);
        ::SetLayeredWindowAttributes(window_state.window, 0, alpha, LWA_ALPHA);
    }
}

void reposition_to_default(Topbar::TopbarWindowState& window_state) {
    if (window_state.window == nullptr) {
        return;
    }
    const HMONITOR monitor = ::MonitorFromWindow(window_state.window, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO monitor_info{};
    monitor_info.cbSize = sizeof(monitor_info);
    if (::GetMonitorInfoW(monitor, &monitor_info) == FALSE) {
        return;
    }
    const int width = window_state.window_width != 0 ? window_state.window_width
                                                     : sh(window_state, design_min_width);
    const int height = sh(window_state, design_height);
    const int default_left = monitor_info.rcWork.left
                             + (monitor_info.rcWork.right - monitor_info.rcWork.left - width) / 2;
    const int default_top = monitor_info.rcWork.top + sh(window_state, 8);
    window_state.window_width = width;
    ::SetWindowPos(window_state.window,
                   HWND_TOPMOST,
                   default_left,
                   default_top,
                   width,
                   height,
                   SWP_NOACTIVATE);
    ui::apply_window_treatment(window_state.window, width, height, sh(window_state, design_radius));
    update_tooltip_layout(window_state);
}

// Restores the remembered pill position, clamped fully into the nearest

// monitor's work area.

void restore_saved_position(Topbar::TopbarWindowState& window_state) {
    if (window_state.window == nullptr || !window_state.host.saved_topbar_position) {
        return;
    }
    const std::optional<POINT> saved_position = window_state.host.saved_topbar_position();
    if (!saved_position.has_value()) {
        return;
    }
    const HMONITOR monitor = ::MonitorFromPoint(*saved_position, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitor_info{};
    monitor_info.cbSize = sizeof(monitor_info);
    if (::GetMonitorInfoW(monitor, &monitor_info) == FALSE) {
        return;
    }
    const int width = window_state.window_width;
    const int height = sh(window_state, design_height);
    const int restored_left = std::max(
        monitor_info.rcWork.left, std::min(saved_position->x, monitor_info.rcWork.right - width));
    const int restored_top = std::max(
        monitor_info.rcWork.top, std::min(saved_position->y, monitor_info.rcWork.bottom - height));
    ::SetWindowPos(window_state.window,
                   HWND_TOPMOST,
                   restored_left,
                   restored_top,
                   width,
                   height,
                   SWP_NOACTIVATE);
}

LRESULT CALLBACK topbar_proc(HWND window, UINT message, WPARAM w_param, LPARAM l_param) {
    auto* window_state =
        reinterpret_cast<Topbar::TopbarWindowState*>(::GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(l_param);
        window_state = static_cast<Topbar::TopbarWindowState*>(create->lpCreateParams);
        ::SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window_state));
        window_state->window = window;
    }
    if (window_state == nullptr) {
        return ::DefWindowProcW(window, message, w_param, l_param);
    }
    switch (message) {
    case WM_PAINT: {
        // `ps` keeps the local PAINTSTRUCT from shadowing the paint() helper

        // defined above (MSVC C2064 regression caught in the Release build).

        PAINTSTRUCT ps{};
        HDC device_context = ::BeginPaint(window, &ps);
        if (device_context != nullptr) {
            paint(*window_state, device_context);
            ::EndPaint(window, &ps);
        }
        return 0;
    }
    case WM_ERASEBKGND:
        return TRUE;
    case WM_LBUTTONDOWN: {
        const POINT point{GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
        const int zone = zone_from_point(*window_state, point.x, point.y);
        if (zone != window_state->down_zone) {
            window_state->down_zone = zone;
            // The pressed face paints as soon as the zone is held down.

            ::InvalidateRect(window, nullptr, FALSE);
        }
        window_state->drag_start = point;
        window_state->moved_while_down = false;
        if (window_state->down_zone == 0 || window_state->down_zone == 3) {
            // Mode zones: distinguish click (toggle / menu) from drag by

            // movement.

            window_state->drag_pending = true;
            ::SetCapture(window);
        } else if (window_state->down_zone > 0) {
            // Wrench / close zones open their menus immediately.

            handle_click(*window_state, window_state->down_zone);
        }
        return 0;
    }
    case WM_MOUSEMOVE: {
        const POINT point{GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
        // Hover feedback (also starts mouse tracking so the pill can clear

        // the highlight when the pointer leaves).

        if (!window_state->mouse_tracked) {
            TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE, window, 0};
            if (::TrackMouseEvent(&track) != FALSE) {
                window_state->mouse_tracked = true;
            }
        }
        const int hover_zone = zone_from_point(*window_state, point.x, point.y);
        if (hover_zone != window_state->hover_zone) {
            window_state->hover_zone = hover_zone;
            ::InvalidateRect(window, nullptr, FALSE);
        }
        if (!window_state->drag_pending) {
            return 0;
        }
        if (std::abs(point.x - window_state->drag_start.x)
                + std::abs(point.y - window_state->drag_start.y)
            > sh(*window_state, 4)) {
            window_state->drag_pending = false;
            window_state->moved_while_down = true;
            // Hand the rest of the drag to the native move loop.

            ::ReleaseCapture();
            ::PostMessageW(window, WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x, point.y));
        }
        return 0;
    }
    case WM_MOUSELEAVE:
        window_state->mouse_tracked = false;
        if (window_state->hover_zone != -1) {
            window_state->hover_zone = -1;
            ::InvalidateRect(window, nullptr, FALSE);
        }
        return 0;
    case WM_LBUTTONUP: {
        if (window_state->drag_pending) {
            window_state->drag_pending = false;
            ::ReleaseCapture();
            if (!window_state->moved_while_down) {
                handle_click(*window_state, window_state->down_zone);
            }
        }
        if (window_state->down_zone != -1) {
            window_state->down_zone = -1;
            ::InvalidateRect(window, nullptr, FALSE);
        }
        return 0;
    }
    case WM_EXITSIZEMOVE: {
        // Clamp the pill fully into the work area after a drag, then persist

        // the position so it survives restarts (when remembering is on).

        RECT window_rect{};
        ::GetWindowRect(window, &window_rect);
        const HMONITOR monitor = ::MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
        MONITORINFO monitor_info{};
        monitor_info.cbSize = sizeof(monitor_info);
        if (::GetMonitorInfoW(monitor, &monitor_info) != FALSE) {
            const int width = window_rect.right - window_rect.left;
            const int height = window_rect.bottom - window_rect.top;
            const int clamped_left =
                std::max(monitor_info.rcWork.left,
                         std::min(window_rect.left, monitor_info.rcWork.right - width));
            const int clamped_top =
                std::max(monitor_info.rcWork.top,
                         std::min(window_rect.top, monitor_info.rcWork.bottom - height));
            ::SetWindowPos(
                window, HWND_TOPMOST, clamped_left, clamped_top, width, height, SWP_NOACTIVATE);
            if (window_state->host.save_topbar_position != nullptr) {
                window_state->host.save_topbar_position(clamped_left, clamped_top);
            }
        }
        return 0;
    }
    case WM_CONTEXTMENU: {
        // Right-click anywhere on the pill: the full desktop context menu,

        // built and owned by the application (layouts, font, OSK, settings,

        // enable/disable, updates, about, tray, exit).

        if (window_state->host.show_context_menu != nullptr) {
            const POINT screen{GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
            POINT at = screen;
            // WM_CONTEXTMENU passes -1 coordinates when invoked from the

            // keyboard; fall back to the pill's position.

            if (at.x == -1 || at.y == -1) {
                RECT window_rect{};
                ::GetWindowRect(window, &window_rect);
                at = POINT{window_rect.left, window_rect.bottom};
            }
            window_state->host.show_context_menu(at);
        }
        return 0;
    }
    case WM_TIMER:
        if (w_param == idle_timer_id) {
            update_idle_state(*window_state);
        }
        return 0;
    case WM_DPICHANGED: {
        window_state->dpi = HIWORD(w_param);
        if (window_state->font != nullptr) {
            ::DeleteObject(window_state->font);
            window_state->font = nullptr;
        }
        window_state->font = ui::create_message_font();
        if (l_param != 0) {
            const auto* suggested = reinterpret_cast<const RECT*>(l_param);
            const int width = suggested->right - suggested->left;
            const int height = suggested->bottom - suggested->top;
            window_state->window_width = width;
            ui::apply_window_treatment(window, width, height, sh(*window_state, design_radius));
            update_tooltip_layout(*window_state);
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
        ::InvalidateRect(window, nullptr, TRUE);
        return 0;
    case WM_DESTROY:
        ::KillTimer(window, idle_timer_id);
        if (window_state->tooltip != nullptr) {
            ::DestroyWindow(window_state->tooltip);
            window_state->tooltip = nullptr;
        }
        window_state->window = nullptr;
        window_state->a11y_alive.reset();
        return 0;
    case WM_NCDESTROY:
        ::SetWindowLongPtrW(window, GWLP_USERDATA, 0);
        return ::DefWindowProcW(window, message, w_param, l_param);
    default:
        return ::DefWindowProcW(window, message, w_param, l_param);
    }
}

ATOM register_topbar_class(HINSTANCE instance) {
    static const ATOM registered = [instance] {
        WNDCLASSEXW window_class{};
        window_class.cbSize = sizeof(window_class);
        window_class.style = CS_DROPSHADOW;  // native elevation under the pill

        window_class.lpfnWndProc = &topbar_proc;
        window_class.hInstance = instance;
        window_class.hCursor = ::LoadCursorW(nullptr, IDC_HAND);
        window_class.lpszClassName = topbar_class;
        return ::RegisterClassExW(&window_class);
    }();
    return registered;
}

}  // namespace

Topbar::~Topbar() {
    destroy();
}

bool Topbar::create(HINSTANCE instance, Host host) {
    if (window_state_ != nullptr) {
        return true;
    }
    if (register_topbar_class(instance) == 0) {
        return false;
    }
    window_state_ = new (std::nothrow) TopbarWindowState();
    if (window_state_ == nullptr) {
        return false;
    }
    window_state_->host = std::move(host);
    window_state_->instance = instance;
    window_state_->dpi = ui::system_dpi_for(nullptr);
    window_state_->font = ui::create_message_font();
    window_state_->layout_label = L"English";
    window_state_->window_width = measure_width(*window_state_);
    const int width = window_state_->window_width;
    const int height = sh(*window_state_, design_height);
    const HWND window = ::CreateWindowExW(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
                                          topbar_class,
                                          L"Suborno",
                                          WS_POPUP,
                                          CW_USEDEFAULT,
                                          CW_USEDEFAULT,
                                          width,
                                          height,
                                          nullptr,
                                          nullptr,
                                          instance,
                                          window_state_);
    if (window == nullptr) {
        delete window_state_;
        window_state_ = nullptr;
        return false;
    }
    ui::apply_window_treatment(window, width, height, sh(*window_state_, design_radius));
    ::SetLayeredWindowAttributes(window, 0, 255, LWA_ALPHA);
    window_state_->current_alpha = 255;
    ::SetTimer(window, idle_timer_id, 500, nullptr);
    create_tooltip(*window_state_);
    update_tooltip_layout(*window_state_);
    reposition_to_default(*window_state_);
    restore_saved_position(*window_state_);
    return true;
}

void Topbar::destroy() {
    if (window_state_ == nullptr) {
        return;
    }
    if (window_state_->window != nullptr && ::IsWindow(window_state_->window) != FALSE) {
        ::DestroyWindow(window_state_->window);
    }
    if (window_state_->font != nullptr) {
        ::DeleteObject(window_state_->font);
        window_state_->font = nullptr;
    }
    delete window_state_;
    window_state_ = nullptr;
}

void Topbar::show() {
    if (window_state_ != nullptr && window_state_->window != nullptr) {
        if (window_state_->window_width == 0) {
            reposition_to_default(*window_state_);
        }
        ::ShowWindow(window_state_->window, SW_SHOWNOACTIVATE);
        update_idle_state(*window_state_);
    }
}

void Topbar::hide() {
    if (window_state_ != nullptr && window_state_->window != nullptr) {
        ::ShowWindow(window_state_->window, SW_HIDE);
    }
}

bool Topbar::visible() const {
    return window_state_ != nullptr && window_state_->window != nullptr
           && ::IsWindowVisible(window_state_->window) != FALSE;
}

void Topbar::apply_settings(const TopbarSettingsSnapshot& settings) {
    if (window_state_ != nullptr) {
        window_state_->settings = settings;
        update_idle_state(*window_state_);
    }
}

void Topbar::refresh_layout_label() {
    if (window_state_ == nullptr || window_state_->window == nullptr) {
        return;
    }
    const std::string active = window_state_->host.active_layout_id
                                   ? window_state_->host.active_layout_id()
                                   : std::string();
    std::wstring label = L"English";
    if (window_state_->host.layouts != nullptr) {
        for (const auto& layout : window_state_->host.layouts()) {
            if (layout.first == active) {
                label = layout.second;
                break;
            }
        }
    }
    window_state_->layout_label = label;
    update_window_width(*window_state_);
    // The label width moved the zone boundaries; the tooltip rects and the

    // localized texts follow (refresh_layout_label also runs after a

    // language switch, so the texts refresh here as well).

    update_tooltip_layout(*window_state_);
    update_tooltip_texts(*window_state_);
    ::InvalidateRect(window_state_->window, nullptr, TRUE);
    ::UpdateWindow(window_state_->window);
}

void Topbar::refresh_font() {
    if (window_state_ == nullptr) {
        return;
    }
    if (window_state_->font != nullptr) {
        ::DeleteObject(window_state_->font);
        window_state_->font = nullptr;
    }
    window_state_->font = ui::create_message_font();
    if (window_state_->window != nullptr) {
        update_window_width(*window_state_);
        ::InvalidateRect(window_state_->window, nullptr, TRUE);
        ::UpdateWindow(window_state_->window);
    }
}

}  // namespace suborno::platform::windows::app
