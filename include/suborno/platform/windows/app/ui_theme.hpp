#pragma once

// Shared visual system for the Suborno desktop shell (settings window,

// about dialog, floating topbar, on-screen keyboard, menus, candidate

// window). The dark values are the measured Borno-reference palette

// documented in docs/design/SUBORNO_REFERENCE_DESIGN_SPEC.md; the

// implementation is an independent clean-room Win32 rendering of that

// specification. The light theme is Suborno's own palette, tuned for the

// same accent hue at light-surface contrast.

//

// Token model (three tiers, mirroring the design-system token hierarchy):

//   primitive   — the literal RGB values in suborno_dark / suborno_light

//   semantic    — the named ThemePalette roles + the theme_*() accessors

//                 below; every drawn surface reads ONLY through these

//   component   — per-surface geometry constants (topbar pill, menu item,

//                 OSK keys) which reference the shared ui::metrics scale

// where they overlap.

//

// Contrast floors (WCAG AA): primary/secondary text >= 4.5:1 against its

// surface in BOTH themes; non-text boundaries (borders, focus rings,

// icons) >= 3:1; text drawn on accent fills uses theme_on_accent() against

// theme_accent_fill(), chosen per theme to stay >= 4.5:1. When Windows High

// Contrast is active the palette is derived from the system colors instead

// (see system_color_palette()), so accessibility modes keep working.

#include <Windows.h>
#include <algorithm>
#include <cwchar>
#include <filesystem>
#include <map>
#include <string>
#include <utility>

#include "suborno/core/ui_strings.hpp"

namespace suborno::platform::windows::app::ui {

// One named theme. `theme_id` values match the settings schema

/// (suborno-dark | suborno-light | system).

struct ThemePalette {
    /// Surfaces

    COLORREF surface = 0;  ///< window/page/menu background

    COLORREF surface_topbar = 0;  ///< topbar pill background

    COLORREF surface_raised = 0;  ///< selected pill / resting control face

    COLORREF surface_hover = 0;  ///< hover state

    COLORREF surface_pressed = 0;  ///< pressed state (deeper than hover)

    /// Text

    COLORREF text = 0;  ///< primary text

    COLORREF text_inactive = 0;  ///< secondary text / captions (>= 4.5:1)

    /// Accent family

    COLORREF accent = 0;  ///< focus outlines, icons, non-text accent (>= 3:1)

    COLORREF accent_fill = 0;  ///< text-bearing accent fills (>= 4.5:1 with on_accent)

    COLORREF accent_hover = 0;  ///< hover face of accent-filled controls

    COLORREF on_accent = 0;  ///< text/glyphs drawn on accent_fill

    COLORREF accent_warm = 0;  ///< the reference topbar wrench glyph (brand accent)

    /// Boundaries

    COLORREF separator = 0;  ///< separators / outlines

    COLORREF control_border = 0;  ///< checkbox box, dropdown edge, slider handle (>= 3:1)

    COLORREF track = 0;  ///< slider unfilled track

    COLORREF focus = 0;  ///< keyboard focus ring

    /// Status colors (always paired with text — never color alone)

    COLORREF success = 0;  ///< enabled / healthy states

    COLORREF warning = 0;  ///< attention states (disabled IME)

    COLORREF error = 0;  ///< failure states (reserved; errors use native dialogs)

    COLORREF information = 0;  ///< informational accents (alias of accent)
};

// Measured dark palette (docs/research/BORNOTSF_UI_UX_REFERENCE_ANALYSIS.md §3).

/// New state/semantic tokens extend the measured set without changing it.

constexpr ThemePalette suborno_dark{
    .surface = RGB(0x2C, 0x2C, 0x2C),
    .surface_topbar = RGB(0x2B, 0x2B, 0x2B),
    .surface_raised = RGB(0x38, 0x38, 0x38),
    .surface_hover = RGB(0x33, 0x33, 0x33),
    .surface_pressed = RGB(0x26, 0x26, 0x26),
    .text = RGB(0xFF, 0xFF, 0xFF),
    // Recomputed against the measured surface #2C2C2C: #8C8C8C measured only

    // 4.15:1, below the documented 4.5:1 floor. #969696 measures 4.72:1 and

    // stays visually in the same weight class. PAIRING RULE: this token is

    // valid on `surface` (and hover, 4.27:1 for large/secondary use) — every

    // current consumer draws on those. If a future control ever sets

    // secondary text on `surface_raised`, raise the token to #A6A6A6

    // (surface 5.74 / raised 4.82 / hover 5.19) instead of overriding locally.

    .text_inactive = RGB(0x96, 0x96, 0x96),
    .accent = RGB(0x58, 0x86, 0xFE),
    .accent_fill = RGB(0x44, 0x6C, 0xE8),
    .accent_hover = RGB(0x51, 0x78, 0xDE),
    .on_accent = RGB(0xFF, 0xFF, 0xFF),
    .accent_warm = RGB(0xE0, 0x6C, 0x3A),
    .separator = RGB(0x3F, 0x3F, 0x3F),
    .control_border = RGB(0x8C, 0x8C, 0x8C),
    .track = RGB(0x27, 0x27, 0x27),
    .focus = RGB(0x58, 0x86, 0xFE),
    .success = RGB(0x6C, 0xCB, 0x5F),
    .warning = RGB(0xFC, 0xA9, 0x38),
    .error = RGB(0xF1, 0x70, 0x7A),
    .information = RGB(0x58, 0x86, 0xFE),
};

// Suborno light theme: same geometry, WCAG-friendly dark-on-light text and a

/// slightly deeper accent so focus outlines stay visible on light surfaces.

constexpr ThemePalette suborno_light{
    .surface = RGB(0xF5, 0xF5, 0xF5),
    .surface_topbar = RGB(0xFB, 0xFB, 0xFB),
    .surface_raised = RGB(0xE9, 0xE9, 0xE9),
    .surface_hover = RGB(0xE2, 0xE2, 0xE2),
    .surface_pressed = RGB(0xD6, 0xD6, 0xD6),
    .text = RGB(0x1F, 0x1F, 0x1F),
    // Darkened from 0x6C to keep secondary text at >= 4.5:1 on 0xF5 surfaces.

    .text_inactive = RGB(0x65, 0x65, 0x65),
    .accent = RGB(0x35, 0x6F, 0xE3),
    .accent_fill = RGB(0x35, 0x6F, 0xE3),
    .accent_hover = RGB(0x2F, 0x64, 0xD0),
    .on_accent = RGB(0xFF, 0xFF, 0xFF),
    .accent_warm = RGB(0xE0, 0x6C, 0x3A),
    .separator = RGB(0xD8, 0xD8, 0xD8),
    .control_border = RGB(0x82, 0x82, 0x82),
    .track = RGB(0xDE, 0xDE, 0xDE),
    .focus = RGB(0x35, 0x6F, 0xE3),
    .success = RGB(0x0E, 0x7A, 0x0E),
    .warning = RGB(0x9A, 0x5B, 0x00),
    .error = RGB(0xB1, 0x0E, 0x0E),
    .information = RGB(0x35, 0x6F, 0xE3),
};

// Palette built from the active Windows High Contrast system colors. Custom

// surfaces keep their layout but render with the user's accessibility

// scheme: text/status colors collapse to full-contrast system colors and

/// color-coded states keep their meaning through the existing text labels.

inline ThemePalette system_color_palette() noexcept {
    ThemePalette palette{};
    const COLORREF window = ::GetSysColor(COLOR_WINDOW);
    const COLORREF window_text = ::GetSysColor(COLOR_WINDOWTEXT);
    const COLORREF highlight = ::GetSysColor(COLOR_HIGHLIGHT);
    const COLORREF highlight_text = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
    palette.surface = window;
    palette.surface_topbar = window;
    palette.surface_raised = ::GetSysColor(COLOR_3DFACE);
    palette.surface_hover = ::GetSysColor(COLOR_3DFACE);
    palette.surface_pressed = ::GetSysColor(COLOR_3DSHADOW);
    palette.text = window_text;
    // High Contrast: never dim text; announcements rely on labels anyway.

    palette.text_inactive = window_text;
    palette.accent = highlight;
    palette.accent_fill = highlight;
    palette.accent_hover = highlight;
    palette.on_accent = highlight_text;
    palette.accent_warm = highlight;
    palette.separator = ::GetSysColor(COLOR_3DSHADOW);
    palette.control_border = window_text;
    palette.track = ::GetSysColor(COLOR_3DFACE);
    palette.focus = highlight;
    // Color alone never carries meaning (the status line also says

    // enabled/disabled in words), so full-contrast text is correct here.

    palette.success = window_text;
    palette.warning = window_text;
    palette.error = window_text;
    palette.information = window_text;
    return palette;
}

/// True when a Windows High Contrast scheme is active.

inline bool high_contrast_active() noexcept {
    HIGHCONTRASTW info{};
    info.cbSize = sizeof(info);
    if (::SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(info), &info, 0) == FALSE) {
        return false;
    }
    return (info.dwFlags & HCF_HIGHCONTRASTON) != 0;
}

// Resolves the Windows "system theme preference" (Settings > Personalization

// > Colors > Choose your default app mode). Registry value AppsUseLightTheme:

// 1 = light apps, 0 = dark apps; a missing value falls back to dark, which

/// matches the default theme of this application.

inline bool windows_app_light_theme() noexcept {
    DWORD light = 0;
    DWORD size = sizeof(light);
    if (::RegGetValueW(HKEY_CURRENT_USER,
                       L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                       L"AppsUseLightTheme",
                       RRF_RT_REG_DWORD,
                       nullptr,
                       &light,
                       &size)
        == ERROR_SUCCESS) {
        return light != 0;
    }
    return false;
}

// Storage slot for the active theme pointer. Kept as a separate inline

/// function so set_active_theme stays exception-free and header-inline.

inline const ThemePalette*& palette_slot() noexcept {
    static const ThemePalette* slot = &suborno_dark;
    return slot;
}

// The active theme for the whole process. The desktop shell sets it from the

// saved settings at startup and whenever the user saves a new theme; every

// custom-drawn surface reads tokens through the accessors below so a theme

/// switch repaints everything uniformly. Defaults to dark (the reference look).

inline const ThemePalette& active_theme_palette() noexcept {
    return *palette_slot();
}

// The last theme id handed to set_active_theme ("suborno-dark" initially —

// the shell's default). Kept so the theme can be re-resolved when Windows

// itself changes: High Contrast toggled on/off, or the light/dark app-mode

/// preference switched while the shell is running.

inline std::wstring& active_theme_id() noexcept {
    static std::wstring stored = L"suborno-dark";
    return stored;
}

// Recognized theme ids: "suborno-dark", "suborno-light" and "system"

// (follow the Windows app-mode preference). When Windows High Contrast is

// active it wins over every saved theme so accessibility schemes always

/// render; the saved value is untouched and resumes when HC is turned off.

inline void set_active_theme(const std::wstring& theme_id) noexcept {
    static const ThemePalette dark = suborno_dark;
    static const ThemePalette light = suborno_light;
    active_theme_id() = theme_id;
    if (high_contrast_active()) {
        static ThemePalette high_contrast = system_color_palette();
        high_contrast = system_color_palette();  ///< re-read live system colors

        palette_slot() = &high_contrast;
        return;
    }
    if (theme_id == L"suborno-light") {
        palette_slot() = &light;
        return;
    }
    if (theme_id == L"system") {
        palette_slot() = windows_app_light_theme() ? &light : &dark;
        return;
    }
    palette_slot() = &dark;
}

// Re-runs set_active_theme with the remembered theme id. Call this on

// WM_THEMECHANGED from every live surface; the palette is process-global,

/// so one call re-derives it everywhere and each window repaints itself.

inline void refresh_active_theme() noexcept {
    set_active_theme(active_theme_id());
}

/// Token accessors — the single read path for every drawn surface.

inline COLORREF theme_surface() noexcept {
    return active_theme_palette().surface;
}
inline COLORREF theme_surface_topbar() noexcept {
    return active_theme_palette().surface_topbar;
}
inline COLORREF theme_surface_raised() noexcept {
    return active_theme_palette().surface_raised;
}
inline COLORREF theme_surface_hover() noexcept {
    return active_theme_palette().surface_hover;
}
inline COLORREF theme_surface_pressed() noexcept {
    return active_theme_palette().surface_pressed;
}
inline COLORREF theme_text() noexcept {
    return active_theme_palette().text;
}
inline COLORREF theme_text_inactive() noexcept {
    return active_theme_palette().text_inactive;
}
inline COLORREF theme_accent() noexcept {
    return active_theme_palette().accent;
}
inline COLORREF theme_accent_fill() noexcept {
    return active_theme_palette().accent_fill;
}
inline COLORREF theme_accent_hover() noexcept {
    return active_theme_palette().accent_hover;
}
inline COLORREF theme_on_accent() noexcept {
    return active_theme_palette().on_accent;
}
inline COLORREF theme_accent_warm() noexcept {
    return active_theme_palette().accent_warm;
}
inline COLORREF theme_separator() noexcept {
    return active_theme_palette().separator;
}
inline COLORREF theme_control_border() noexcept {
    return active_theme_palette().control_border;
}
inline COLORREF theme_track() noexcept {
    return active_theme_palette().track;
}
inline COLORREF theme_focus() noexcept {
    return active_theme_palette().focus;
}
inline COLORREF theme_success() noexcept {
    return active_theme_palette().success;
}
inline COLORREF theme_warning() noexcept {
    return active_theme_palette().warning;
}
inline COLORREF theme_error() noexcept {
    return active_theme_palette().error;
}
inline COLORREF theme_information() noexcept {
    return active_theme_palette().information;
}

// Shared spacing scale (96-DPI design pixels). 4/8 grid with section tiers;

/// new layout code should use these instead of bare literals.

namespace spacing {
constexpr int xs = 4;  ///< icon/control inner gaps

constexpr int sm = 8;  ///< control-to-control

constexpr int md = 12;  ///< label-to-control, pill insets

constexpr int lg = 16;  ///< grouped rows

constexpr int xl = 24;  ///< between related groups

constexpr int section = 32;  ///< before a new section

constexpr int page = 48;  ///< page margins

}  // namespace spacing

// Shared component geometry (96-DPI design pixels) for the controls that

/// repeat across surfaces.

namespace metrics {
constexpr int checkbox_height = 24;
// Buttons, dropdowns and the settings footer actions share one height so

/// controls on the same page align ("control heights move together").

constexpr int button_height = 34;
constexpr int dropdown_height = 34;
constexpr int menu_item_height = 28;
constexpr int slider_row_height = 24;
constexpr int icon_size = 16;
constexpr int radius_small = 4;  ///< menu item pills, focus rects

constexpr int radius_medium = 6;  ///< buttons, dropdowns, inputs

constexpr int radius_large = 8;  ///< sidebar pills, panels

constexpr int radius_pill = 10;  ///< topbar pill, OSK

}  // namespace metrics

/// Scales a 96-DPI design pixel measurement to a window's DPI.

inline int scaled(UINT dpi, int pixels) noexcept {
    return pixels * static_cast<int>(dpi) / 96;
}

// Resolves the DPI of a window (Windows 10 1607+ GetDpiForWindow) with a

/// legacy GDI fallback; returns the primary-display DPI for nullptr.

inline UINT system_dpi_for(HWND window) noexcept {
    using GetDpiForWindowFn = UINT(WINAPI*)(HWND);
    static const GetDpiForWindowFn get_dpi_for_window = []() -> GetDpiForWindowFn {
        const HMODULE user32 = ::GetModuleHandleW(L"user32.dll");
        return user32 == nullptr ? nullptr
                                 : reinterpret_cast<GetDpiForWindowFn>(
                                       ::GetProcAddress(user32, "GetDpiForWindow"));
    }();
    if (get_dpi_for_window != nullptr && window != nullptr) {
        const UINT dpi = get_dpi_for_window(window);
        if (dpi != 0) {
            return dpi;
        }
    }
    if (HDC device_context = ::GetDC(nullptr); device_context != nullptr) {
        const UINT dpi = static_cast<UINT>(::GetDeviceCaps(device_context, LOGPIXELSY));
        ::ReleaseDC(nullptr, device_context);
        if (dpi != 0) {
            return dpi;
        }
    }
    return 96;
}

// Caps a fixed-layout window's design DPI so the whole window fits inside a

// monitor work area with a small margin. The shell's dialogs use absolute

// 96-DPI design coordinates scaled by one DPI factor; scaling purely by the

// system DPI overflows short screens (the 620-px settings window measures

// 775 device px on a 768-tall laptop at 125% and 1240 px on 1080p at 200%,

// pushing Save/Cancel off-screen). The returned DPI never exceeds the

// requested one and never drops below 96 — below the authored design size a

/// fixed layout cannot render meaningfully anyway.

inline UINT dpi_fitting(const RECT& work_area,
                        int design_width,
                        int design_height,
                        UINT requested_dpi) noexcept {
    const LONG area_width = work_area.right - work_area.left;
    const LONG area_height = work_area.bottom - work_area.top;
    UINT fitted = requested_dpi;
    if (design_height > 0 && area_height > 0) {
        const UINT by_height = static_cast<UINT>(area_height * 92L * 96L / (100L * design_height));
        fitted = fitted < by_height ? fitted : by_height;
    }
    if (design_width > 0 && area_width > 0) {
        const UINT by_width = static_cast<UINT>(area_width * 92L * 96L / (100L * design_width));
        fitted = fitted < by_width ? fitted : by_width;
    }
    return fitted < 96U ? 96U : fitted;
}

// Per-process UI font family applied by create_message_font(). Empty = the

// OS default (Segoe UI message font). The desktop shell sets it from the

// saved settings at startup and whenever the font changes; every window that

/// (re)creates its message font afterwards picks the family up.

inline std::wstring& ui_font_family() noexcept {
    static std::wstring family;
    return family;
}

inline void set_ui_font_family(const wchar_t* family) noexcept {
    ui_font_family().assign(family == nullptr ? L"" : family);
}

// Per-user message font (Segoe UI by default, or the configured UI font

/// family), used for body text on every surface.

inline HFONT create_message_font() noexcept {
    NONCLIENTMETRICSW metrics{};
    metrics.cbSize = sizeof(metrics);
    if (::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0) != FALSE) {
        const std::wstring& family = ui_font_family();
        if (!family.empty() && family.size() < LF_FACESIZE) {
            ::wcsncpy_s(
                metrics.lfMessageFont.lfFaceName, LF_FACESIZE, family.c_str(), family.size());
        }
        return ::CreateFontIndirectW(&metrics.lfMessageFont);
    }
    return nullptr;
}

// Typography scale (96-DPI design px, all on the configured UI family):

//   heading 20 — window titles (settings, about)

//   title   16 — page/section emphasis

//   message  — the system message font size, body text

///   caption 12 — descriptions, secondary hints, status notes

inline HFONT create_heading_font(UINT dpi) noexcept {
    NONCLIENTMETRICSW metrics{};
    metrics.cbSize = sizeof(metrics);
    if (::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0) == FALSE) {
        return nullptr;
    }
    metrics.lfMessageFont.lfHeight = -scaled(dpi, 20);
    metrics.lfMessageFont.lfWeight = FW_SEMIBOLD;
    return ::CreateFontIndirectW(&metrics.lfMessageFont);
}

inline HFONT create_title_font(UINT dpi) noexcept {
    NONCLIENTMETRICSW metrics{};
    metrics.cbSize = sizeof(metrics);
    if (::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0) == FALSE) {
        return nullptr;
    }
    metrics.lfMessageFont.lfHeight = -scaled(dpi, 16);
    metrics.lfMessageFont.lfWeight = FW_SEMIBOLD;
    return ::CreateFontIndirectW(&metrics.lfMessageFont);
}

inline HFONT create_caption_font(UINT dpi) noexcept {
    NONCLIENTMETRICSW metrics{};
    metrics.cbSize = sizeof(metrics);
    if (::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0) == FALSE) {
        return nullptr;
    }
    metrics.lfMessageFont.lfHeight = -scaled(dpi, 12);
    metrics.lfMessageFont.lfWeight = FW_NORMAL;
    return ::CreateFontIndirectW(&metrics.lfMessageFont);
}

// Fills an axis-aligned rounded rectangle with a solid color. Region-free:

// a NULL-pen RoundRect paints the same geometry as the previous

// CreateRoundRectRgn+FillRgn pair without allocating a GDI region per call

/// (fill_rounded runs 10-30 times per paint on the busier surfaces).

inline void fill_rounded(HDC device_context, RECT rect, COLORREF color, int radius) noexcept {
    HBRUSH brush = ::CreateSolidBrush(color);
    if (brush == nullptr) {
        return;
    }
    const HGDIOBJ previous_brush = ::SelectObject(device_context, brush);
    const HGDIOBJ previous_pen = ::SelectObject(device_context, ::GetStockObject(NULL_PEN));
    if (radius > 0) {
        /// Clamp so the corner ellipse never exceeds the rectangle.

        const int max_radius =
            static_cast<int>(std::min(rect.right - rect.left, rect.bottom - rect.top) / 2);
        if (max_radius > 0) {
            const int clamped = radius < max_radius ? radius : max_radius;
            ::RoundRect(
                device_context, rect.left, rect.top, rect.right, rect.bottom, clamped, clamped);
        } else {
            ::FillRect(device_context, &rect, brush);
        }
    } else {
        ::FillRect(device_context, &rect, brush);
    }
    ::SelectObject(device_context, previous_pen);
    ::SelectObject(device_context, previous_brush);
    ::DeleteObject(brush);
}

/// Outlines an axis-aligned rounded rectangle with a solid color.

inline void frame_rounded(HDC device_context, RECT rect, COLORREF color, int radius) noexcept {
    HPEN pen = ::CreatePen(PS_SOLID, 1, color);
    if (pen == nullptr) {
        return;
    }
    const HGDIOBJ previous = ::SelectObject(device_context, pen);
    const HGDIOBJ previous_brush = ::SelectObject(device_context, ::GetStockObject(NULL_BRUSH));
    ::RoundRect(device_context, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
    ::SelectObject(device_context, previous_brush);
    ::SelectObject(device_context, previous);
    ::DeleteObject(pen);
}

/// Single-line theme-aware text helper.

inline void draw_text(HDC device_context,
                      const wchar_t* text,
                      RECT rect,
                      COLORREF color,
                      UINT flags = DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX) noexcept {
    const COLORREF previous = ::SetTextColor(device_context, color);
    ::SetBkMode(device_context, TRANSPARENT);
    ::DrawTextW(device_context, text, -1, &rect, flags);
    ::SetTextColor(device_context, previous);
}

// Applies a rounded-corner window region so frameless dark windows match

/// the reference dialog silhouette.

inline void apply_rounded_region(HWND window, int width, int height, int radius) noexcept {
    HRGN region = ::CreateRoundRectRgn(0, 0, width + 1, height + 1, radius, radius);
    if (region != nullptr) {
        // On success the system owns the region handle; never delete it.

        ::SetWindowRgn(window, region, TRUE);
    }
}

// Modern window treatment for the frameless shell windows. On Windows 11

// (build 22000+) the system rounds the corners itself through the

// documented DWM attribute DWMWA_WINDOW_CORNER_PREFERENCE = DWMWCP_ROUND —

// crisper anti-aliased corners than a hand-cut region, and they

// automatically square off when the window is maximized or snapped, exactly

// like native windows. On Windows 10 the attribute does not exist, the call

// fails, and the hand-cut rounded region (Win10-era fallback) is applied

// instead. Returns true when DWM handled the corners (no region wanted).

/// dwmapi.dll is loaded dynamically so no import dependency is added.

inline bool apply_window_treatment(HWND window, int width, int height, int radius) noexcept {
    using DwmSetWindowAttributeFn = LONG(WINAPI*)(HWND, DWORD, const void*, DWORD);
    static const DwmSetWindowAttributeFn set_attribute = []() -> DwmSetWindowAttributeFn {
        const HMODULE dwmapi = ::LoadLibraryW(L"dwmapi.dll");
        return dwmapi == nullptr ? nullptr
                                 : reinterpret_cast<DwmSetWindowAttributeFn>(
                                       ::GetProcAddress(dwmapi, "DwmSetWindowAttribute"));
    }();
    if (set_attribute != nullptr) {
        constexpr DWORD dwmwa_window_corner_preference = 33;  ///< DWMWA_WINDOW_CORNER_PREFERENCE

        constexpr DWORD dwmwcp_round = 2;  ///< DWMWCP_ROUND

        const DWORD preference = dwmwcp_round;
        if (set_attribute(window, dwmwa_window_corner_preference, &preference, sizeof(preference))
            == 0) {
            return true;
        }
    }
    apply_rounded_region(window, width, height, radius);
    return false;
}

// Fluent System Icons draw strokes with rounded ends and joins; the shell's

// hand-drawn glyphs (close X, wrench, checkmark) match that convention

// through a geometric pen with round caps. Cosmetic pens (width 0) cannot

/// carry end caps, so the width is clamped to >= 1.

inline HPEN create_round_pen(int width, COLORREF color) noexcept {
    LOGBRUSH brush{};
    brush.lbStyle = BS_SOLID;
    brush.lbColor = color;
    const DWORD clamped = width < 1 ? 1 : static_cast<DWORD>(width);
    return ::ExtCreatePen(
        PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_ROUND | PS_JOIN_ROUND, clamped, &brush, 0, nullptr);
}

// ---- Localized UI text (the single read path for every drawn surface) ----

// Converts UTF-8 (the storage format of data/i18n/*/ui.json) to UTF-16 for

/// Win32 text APIs. Returns an empty string for invalid input.

inline std::wstring utf8_to_wide(const std::string& text) noexcept {
    if (text.empty()) {
        return {};
    }
    const int count = ::MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (count <= 0) {
        return {};
    }
    std::wstring wide(static_cast<std::size_t>(count), L'\0');
    const int written = ::MultiByteToWideChar(CP_UTF8,
                                              MB_ERR_INVALID_CHARS,
                                              text.data(),
                                              static_cast<int>(text.size()),
                                              wide.data(),
                                              count);
    if (written != count) {
        return {};
    }
    return wide;
}

// The localized text for `key`, falling back to the compiled English

// `fallback` when data/i18n carries no value for it (missing file, missing

// key, or an unparsable file). Results are cached per key: the locale table

// changes only through load_ui_strings(), which clears the cache, so every

// lookup after the first is a stable map hit and the returned pointer stays

/// valid for the process lifetime.

inline std::map<std::string, std::wstring>& text_cache() noexcept {
    static std::map<std::string, std::wstring> entries;
    return entries;
}

inline const wchar_t* text(const char* key, const wchar_t* fallback) noexcept {
    if (key == nullptr) {
        return fallback == nullptr ? L"" : fallback;
    }
    std::map<std::string, std::wstring>& cache = text_cache();
    const auto found = cache.find(key);
    if (found != cache.end()) {
        return found->second.c_str();
    }
    std::wstring value;
    const std::string& stored = suborno::core::ui_strings::get(key);
    if (!stored.empty()) {
        value = utf8_to_wide(stored);
    }
    if (value.empty() && fallback != nullptr) {
        value.assign(fallback);
    }
    return cache.emplace(key, std::move(value)).first->second.c_str();
}

/// Drops the per-key cache after the locale table changed.

inline void clear_text_cache() noexcept {
    text_cache().clear();
}

// Replaces one {placeholder} in a localized template with a runtime value

// (e.g. the current layout name in the status line). Returns the expanded

/// text; callers hold the result locally (it is not cached).

inline std::wstring format_text(const wchar_t* templ,
                                const wchar_t* placeholder,
                                const wchar_t* value) noexcept {
    if (templ == nullptr) {
        return {};
    }
    std::wstring out;
    out.reserve(wcslen(templ) + (value == nullptr ? 0 : wcslen(value)) + 8);
    const std::wstring needle = placeholder == nullptr ? std::wstring() : placeholder;
    const wchar_t* cursor = templ;
    if (!needle.empty()) {
        while (*cursor != L'\0') {
            const wchar_t* hit = wcsstr(cursor, needle.c_str());
            if (hit == nullptr) {
                break;
            }
            out.append(cursor, hit);
            if (value != nullptr) {
                out.append(value);
            }
            cursor = hit + needle.size();
        }
    }
    out.append(cursor);
    return out;
}

// Loads the locale table for a settings "ui.language" value ("system"

// follows the Windows display language; "en"/"bn" pick a shipped locale).

// Failure keeps whatever strings are active — the shell never renders

/// worse than the compiled English fallbacks.

inline void load_ui_strings(const std::string& language,
                            const std::filesystem::path& i18n_root) noexcept {
    using suborno::core::ui_strings::Locale;
    const bool windows_is_bangla = ::GetUserDefaultUILanguage() == MAKELANGID(0x45, 0x02);
    Locale locale = Locale::english;
    if (language == "bn") {
        locale = Locale::bangla;
    } else if (language == "system") {
        locale = windows_is_bangla ? Locale::bangla : Locale::english;
    }
    std::string error;
    static_cast<void>(suborno::core::ui_strings::load(
        suborno::core::ui_strings::locale_file(i18n_root, locale), error));
}

}  // namespace suborno::platform::windows::app::ui
