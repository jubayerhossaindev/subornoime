#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "suborno/platform/windows/app/application.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <commctrl.h>
#include <cwchar>
#include <exception>
#include <filesystem>
#include <memory>
#include <msctf.h>
#include <new>
#include <optional>
#include <shellapi.h>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>
#include <wil/com.h>
#include <windowsx.h>

#include "persistence/atomic_file.hpp"
#include "suborno/core/engine.hpp"
#include "suborno/platform/windows/app/menu_popup.hpp"
#include "suborno/platform/windows/app/osk_window.hpp"
#include "suborno/platform/windows/app/resource.h"
#include "suborno/platform/windows/app/toast_window.hpp"
#include "suborno/platform/windows/app/topbar.hpp"
#include "suborno/platform/windows/app/ui_accessibility.hpp"
#include "suborno/platform/windows/app/ui_theme.hpp"
#include "suborno/platform/windows/app_paths.hpp"
#include "suborno/platform/windows/tsf/guids.hpp"
#include "suborno_version.hpp"

namespace suborno::platform::windows::app {
namespace {

constexpr wchar_t window_class[] = L"SubornoDesktopWindow";
constexpr UINT tray_message = WM_APP + 1;
constexpr UINT tray_icon_id = 1;
constexpr UINT second_instance_show_topbar = WM_APP + 2;
constexpr UINT layout_hotkey_id = 1;
constexpr UINT encoding_hotkey_id = 2;
constexpr LANGID bengali_bangladesh = MAKELANGID(0x45, 0x02);
constexpr wchar_t startup_key[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr wchar_t startup_value[] = L"Suborno";
// Real web destinations for the About/menus update + documentation entries.

constexpr wchar_t releases_url[] = L"https://github.com/jubayerhossaingalaxy/SUBORNOIME/releases";
constexpr wchar_t documentation_url[] =
    L"https://github.com/jubayerhossaingalaxy/SUBORNOIME#readme";

enum MenuId : UINT {
    menu_open = 2001,
    menu_toggle_input = 2002,
    menu_open_settings = 2003,
    menu_apps = 2004,
    menu_exit = 2005,
    menu_show_topbar = 2006,
    menu_osk = 2007,
    menu_updates = 2008,
    menu_documentation = 2009,
    menu_clear_learnt = 2010,
    menu_layout_first = 2100,
    menu_font_default = 2200,
    menu_font_first = 2201,
};

// Settings pages, in the reference sidebar order (General, Interface,

// Topbar, Input).

enum class PageId : int {
    general = 0,
    interface = 1,
    topbar = 2,
    input = 3
};
constexpr int page_count = 4;

// Custom-drawn control kinds used by the reference-styled settings pages.

enum class CtrlKind : int {
    section_label,
    checkbox,
    dropdown,
    slider,
    status,
    // A real action button (immediate effect, not staged): e.g. "Clear

    // Learnt Words" runs the destructive operation the moment it is clicked

    // (with a confirmation), independent of Save/Cancel.

    button
};

// Semantic actions for interactive controls.

enum CtrlAction : int {
    action_remember_layout = 1,
    action_start_with_windows = 2,
    action_layout_dropdown = 3,
    action_show_candidate_window = 4,
    action_idle_dimming = 5,
    action_idle_opacity = 6,
    action_idle_time = 7,
    action_map_numpad = 8,
    action_ignore_caps_lock = 9,
    action_number_keys = 10,
    action_start_mode = 11,
    action_font_dropdown = 12,
    action_use_os_default_font = 13,
    action_remember_topbar_position = 14,
    action_layout_shortcut = 15,
    action_theme_dropdown = 16,
    action_next_word_prediction = 17,
    action_orientation = 18,
    action_fixed_height = 19,
    action_sliding_effect = 20,
    action_follow_caret = 21,
    action_follow_mouse = 22,
    action_switching_mode = 23,
    action_change_locale = 24,
    action_encoding = 25,
    action_disable_encoding_shortcut = 26,
    action_learning = 27,
    action_clear_learnt = 28,
    action_ui_language = 29,
    action_phonetic_correction = 30,
    action_typing_style = 31,
};

struct LayoutOption {
    const char* id;
    const wchar_t* name;
};

// User-facing names: no implementation jargon ("Phonetic"), the English

// passthrough mode listed first. Kept in sync with the engine metadata in

// src/core/engine.cpp.

constexpr std::array<LayoutOption, 7> layouts{{
    {"english", L"English"},
    {"suborno-phonetic", L"বাংলা (Suborno)"},
    {"borno-phonetic", L"বাংলা (Borno)"},
    {"avro-phonetic", L"Avro"},
    {"khipro", L"Khipro"},
    {"probhat", L"Probhat"},
    {"national", L"National"},
}};

HRESULT get_profiles(wil::com_ptr<ITfInputProcessorProfiles>& profiles) {
    return ::CoCreateInstance(CLSID_TF_InputProcessorProfiles,
                              nullptr,
                              CLSCTX_INPROC_SERVER,
                              IID_PPV_ARGS(profiles.put()));
}

HRESULT profile_enabled(bool& enabled) {
    enabled = false;
    wil::com_ptr<ITfInputProcessorProfiles> profiles;
    HRESULT result = get_profiles(profiles);
    if (FAILED(result)) {
        return result;
    }
    BOOL is_enabled = FALSE;
    result = profiles->IsEnabledLanguageProfile(
        suborno::platform::windows::CLSID_SubornoTextService,
        bengali_bangladesh,
        suborno::platform::windows::GUID_LANGUAGE_PROFILE_SUBORNO_BENGALI,
        &is_enabled);
    if (SUCCEEDED(result)) {
        enabled = is_enabled != FALSE;
    }
    return result;
}

HRESULT set_profile_enabled(bool enabled) {
    wil::com_ptr<ITfInputProcessorProfiles> profiles;
    const HRESULT result = get_profiles(profiles);
    if (FAILED(result)) {
        return result;
    }
    return profiles->EnableLanguageProfile(
        suborno::platform::windows::CLSID_SubornoTextService,
        bengali_bangladesh,
        suborno::platform::windows::GUID_LANGUAGE_PROFILE_SUBORNO_BENGALI,
        enabled ? TRUE : FALSE);
}

HRESULT refresh_active_profile() {
    wil::com_ptr<ITfInputProcessorProfiles> profiles;
    HRESULT result = get_profiles(profiles);
    if (FAILED(result)) {
        return result;
    }

    LANGID language = 0;
    GUID profile = GUID_NULL;
    result = profiles->GetActiveLanguageProfile(
        suborno::platform::windows::CLSID_SubornoTextService, &language, &profile);
    if (result == S_FALSE) {
        return S_OK;
    }
    if (FAILED(result)) {
        return result;
    }
    return profiles->ActivateLanguageProfile(
        suborno::platform::windows::CLSID_SubornoTextService, language, profile);
}

HRESULT executable_path(std::wstring& path) {
    // Use heap allocation to avoid stack overflow (C6262)

    std::vector<wchar_t> buffer(32768);
    const DWORD length =
        ::GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0) {
        return HRESULT_FROM_WIN32(::GetLastError());
    }
    if (length >= static_cast<DWORD>(buffer.size())) {
        // Path exceeds buffer size; try a larger allocation

        std::vector<wchar_t> larger_buffer(length + 1);
        const DWORD larger_length = ::GetModuleFileNameW(
            nullptr, larger_buffer.data(), static_cast<DWORD>(larger_buffer.size()));
        if (larger_length == 0) {
            return HRESULT_FROM_WIN32(::GetLastError());
        }
        path.assign(larger_buffer.data(), larger_length);
        return S_OK;
    }
    path.assign(buffer.data(), length);
    return S_OK;
}

HRESULT read_startup_preference(bool& enabled) {
    enabled = false;
    HKEY key = nullptr;
    const LONG open_result =
        ::RegOpenKeyExW(HKEY_CURRENT_USER, startup_key, 0, KEY_QUERY_VALUE, &key);
    if (open_result == ERROR_FILE_NOT_FOUND || open_result == ERROR_PATH_NOT_FOUND) {
        return S_OK;
    }
    if (open_result != ERROR_SUCCESS) {
        return HRESULT_FROM_WIN32(open_result);
    }

    DWORD type = 0;
    DWORD size = 0;
    const LONG query_result =
        ::RegQueryValueExW(key, startup_value, nullptr, &type, nullptr, &size);
    if (query_result == ERROR_FILE_NOT_FOUND) {
        ::RegCloseKey(key);
        return S_OK;
    }
    if (query_result != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ)) {
        ::RegCloseKey(key);
        return query_result == ERROR_SUCCESS ? HRESULT_FROM_WIN32(ERROR_INVALID_DATA)
                                             : HRESULT_FROM_WIN32(query_result);
    }
    enabled = size > sizeof(wchar_t);
    ::RegCloseKey(key);
    return S_OK;
}

HRESULT write_startup_preference(bool enabled) {
    HKEY key = nullptr;
    const LONG create_result = ::RegCreateKeyExW(
        HKEY_CURRENT_USER, startup_key, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr);
    if (create_result != ERROR_SUCCESS) {
        return HRESULT_FROM_WIN32(create_result);
    }

    LONG result = ERROR_SUCCESS;
    if (enabled) {
        std::wstring path;
        const HRESULT path_result = executable_path(path);
        if (FAILED(path_result)) {
            ::RegCloseKey(key);
            return path_result;
        }
        const std::wstring command = L"\"" + path + L"\" --startup";
        result = ::RegSetValueExW(key,
                                  startup_value,
                                  0,
                                  REG_SZ,
                                  reinterpret_cast<const BYTE*>(command.c_str()),
                                  static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t)));
    } else {
        result = ::RegDeleteValueW(key, startup_value);
        if (result == ERROR_FILE_NOT_FOUND) {
            result = ERROR_SUCCESS;
        }
    }
    ::RegCloseKey(key);
    return result == ERROR_SUCCESS ? S_OK : HRESULT_FROM_WIN32(result);
}

HRESULT open_uri(const wchar_t* uri) {
    const HINSTANCE result =
        ::ShellExecuteW(nullptr, L"open", uri, nullptr, nullptr, SW_SHOWNORMAL);
    return reinterpret_cast<INT_PTR>(result) > 32 ? S_OK : E_FAIL;
}

void show_error(HWND owner, const wchar_t* message, HRESULT result) {
    wchar_t code[16]{};
    swprintf_s(code, _countof(code), L"0x%08lX", static_cast<unsigned long>(result));
    const std::wstring code_line =
        ui::format_text(ui::text("error.code_line", L"Error: {code}"), L"{code}", code);
    wchar_t text[512]{};
    swprintf_s(text, _countof(text), L"%s\n\n%s", message, code_line.c_str());
    ::MessageBoxW(owner, text, ui::text("common.app_name", L"Suborno"), MB_OK | MB_ICONERROR);
}

void report_error(HWND owner, const wchar_t* message, HRESULT result, bool show_dialog) {
    if (show_dialog) {
        show_error(owner, message, result);
        return;
    }

    wchar_t text[512]{};
    swprintf_s(text,
               _countof(text),
               L"%s (HRESULT: 0x%08lX)\n",
               message,
               static_cast<unsigned long>(result));
    ::OutputDebugStringW(text);
}

// Runs before the locale table is loaded (the engine that reads the language

// setting is created by initialize(), which is where this message reports),

// so it stays the compiled English string by design; every dialog that can

// appear after the shell is up localizes through ui::text.

constexpr wchar_t settings_failure_message[] = L"Suborno could not load its settings.";

// Reports a settings-initialization failure with enough identity to act on:

// the operation that failed, the path it touched, and both the HRESULT and

// the native Win32 error code. The paths shown are the user's own settings

// locations on their machine; no credentials or configuration values are

// ever included.

HRESULT report_settings_failure(const wchar_t* operation,
                                const std::filesystem::path& path,
                                HRESULT result,
                                bool show_dialog) {
    wchar_t text[1024]{};
    swprintf_s(text,
               _countof(text),
               L"%s\n\nOperation: %s\nPath: %s\nError: 0x%08lX (Win32 error %lu)",
               settings_failure_message,
               operation,
               path.c_str(),
               static_cast<unsigned long>(result),
               static_cast<unsigned long>(HRESULT_CODE(result)));
    ::OutputDebugStringW(text);
    if (show_dialog) {
        ::MessageBoxW(nullptr, text, ui::text("common.app_name", L"Suborno"), MB_OK | MB_ICONERROR);
    }
    return result;
}

int smoke_test_exit_code(HRESULT result) noexcept {
    const DWORD code = HRESULT_CODE(result);
    return code == ERROR_SUCCESS ? 1 : static_cast<int>(code);
}

std::string wide_to_utf8(const std::wstring& text) {
    if (text.empty()) {
        return {};
    }
    const int required = ::WideCharToMultiByte(
        CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (required <= 0) {
        return {};
    }
    std::string converted(static_cast<std::size_t>(required), '\0');
    if (::WideCharToMultiByte(CP_UTF8,
                              0,
                              text.c_str(),
                              static_cast<int>(text.size()),
                              converted.data(),
                              required,
                              nullptr,
                              nullptr)
        != required) {
        return {};
    }
    return converted;
}

std::wstring utf8_to_wide(const std::string& text) {
    if (text.empty()) {
        return {};
    }
    const int required = ::MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
    if (required <= 0) {
        return {};
    }
    std::wstring converted(static_cast<std::size_t>(required), L'\0');
    if (::MultiByteToWideChar(CP_UTF8,
                              MB_ERR_INVALID_CHARS,
                              text.c_str(),
                              static_cast<int>(text.size()),
                              converted.data(),
                              required)
        == 0) {
        return {};
    }
    return converted;
}

// ---- Bangla-capable font enumeration for the font pickers ----

bool font_family_matches_bangla_keywords(const std::wstring& family) {
    constexpr std::wstring_view keywords[]{L"Bengali",
                                           L"Bangla",
                                           L"Siliguri",
                                           L"Ekushey",
                                           L"Kalpurush",
                                           L"Solaiman",
                                           L"Rupali",
                                           L"Mukti",
                                           L"Likhan",
                                           L"Padma",
                                           L"Shonar",
                                           L"Charu",
                                           L"Mitradut"};
    for (const std::wstring_view keyword : keywords) {
        if (family.find(keyword) != std::wstring::npos) {
            return true;
        }
    }
    return false;
}

// GDI callback collecting every installed font family name.

int CALLBACK collect_font_family(const LOGFONTW* logical_font,
                                 const TEXTMETRICW*,
                                 DWORD,
                                 LPARAM l_param) {
    auto* list = reinterpret_cast<std::vector<std::wstring>*>(l_param);
    if (logical_font != nullptr && logical_font->lfFaceName[0] != L'\0') {
        list->push_back(logical_font->lfFaceName);
    }
    return 1;  // continue enumeration
}

// Curated, known Bangla-capable Windows families are offered first; any other

// installed family whose name references Bangla script follows. The list is

// capped so the menu popup stays comfortably on screen.

const std::vector<std::wstring>& bangla_font_options() {
    static const std::vector<std::wstring> cached = [] {
        constexpr wchar_t curated[][24]{L"Nirmala UI",
                                        L"Noto Sans Bengali",
                                        L"Noto Serif Bengali",
                                        L"Vrinda",
                                        L"Shonar Bangla",
                                        L"Hind Siliguri",
                                        L"Kalpurush",
                                        L"SolaimanLipi",
                                        L"Siyam Rupali",
                                        L"Mukti",
                                        L"Likhan"};
        HDC screen = ::GetDC(nullptr);
        std::vector<std::wstring> installed;
        if (screen != nullptr) {
            LOGFONTW probe{};
            probe.lfCharSet = DEFAULT_CHARSET;
            ::EnumFontFamiliesExW(
                screen, &probe, collect_font_family, reinterpret_cast<LPARAM>(&installed), 0);
            ::ReleaseDC(nullptr, screen);
        }
        const auto contains = [&installed](const wchar_t* name) {
            return std::any_of(
                installed.begin(), installed.end(), [name](const std::wstring& item) {
                    return ::_wcsicmp(item.c_str(), name) == 0;
                });
        };
        std::vector<std::wstring> options;
        for (const wchar_t* name : curated) {
            if (contains(name)) {
                options.emplace_back(name);
            }
        }
        for (const std::wstring& family : installed) {
            const bool already_listed =
                std::any_of(options.begin(), options.end(), [&](const std::wstring& item) {
                    return ::_wcsicmp(item.c_str(), family.c_str()) == 0;
                });
            if (!already_listed && font_family_matches_bangla_keywords(family)) {
                options.push_back(family);
            }
        }
        if (options.size() > 16) {
            options.resize(16);
        }
        return options;
    }();
    return cached;
}

// The shortcut list shown in Settings; every entry here is actually

// registered as a global hotkey by the desktop shell. Alt+Shift and

// Win+Space stay valid in the settings file (schema-compatible) but are

// reserved by Windows itself, so they are not offered in the UI.

constexpr std::array<std::string_view, 7> registerable_shortcuts{
    "F1", "F2", "F3", "F4", "F10", "F12", "Ctrl+Space"};

struct HotkeyBinding {
    UINT modifiers;
    UINT virtual_key;
};

std::optional<HotkeyBinding> hotkey_for_shortcut(const std::string& shortcut) {
    if (shortcut == "F1") {
        return HotkeyBinding{0, VK_F1};
    }
    if (shortcut == "F2") {
        return HotkeyBinding{0, VK_F2};
    }
    if (shortcut == "F3") {
        return HotkeyBinding{0, VK_F3};
    }
    if (shortcut == "F4") {
        return HotkeyBinding{0, VK_F4};
    }
    if (shortcut == "F10") {
        return HotkeyBinding{0, VK_F10};
    }
    if (shortcut == "F12") {
        return HotkeyBinding{0, VK_F12};
    }
    if (shortcut == "Ctrl+Space") {
        return HotkeyBinding{MOD_CONTROL, VK_SPACE};
    }
    return std::nullopt;
}

// The settings window is laid out in 96-DPI design pixels; everything is

// multiplied by this factor so 125-200% Windows display scaling produces a

// proportionally sized window instead of a small, cramped one.

UINT system_dpi() noexcept {
    if (HDC device_context = ::GetDC(nullptr); device_context != nullptr) {
        const UINT dpi = static_cast<UINT>(::GetDeviceCaps(device_context, LOGPIXELSY));
        ::ReleaseDC(nullptr, device_context);
        if (dpi != 0) {
            return dpi;
        }
    }
    return 96;
}

// Declares per-Monitor v2 DPI awareness so every surface receives

// WM_DPICHANGED when dragged across monitors (or when a display's scale

// changes) and rescales live instead of being bitmap-stretched. The older

// system-aware modes remain the fallback for pre-1703 Windows versions,

// and a total failure keeps the previous DPI-unaware behavior.

void enable_process_dpi_awareness() noexcept {
    constexpr int process_system_dpi_aware = 2;  // PROCESS_SYSTEM_DPI_AWARE

    if (HMODULE user32 = ::GetModuleHandleW(L"user32.dll"); user32 != nullptr) {
        using SetContextFn = BOOL(WINAPI*)(HANDLE);
        if (const auto set_context = reinterpret_cast<SetContextFn>(
                ::GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
            set_context != nullptr
            && set_context(reinterpret_cast<HANDLE>(static_cast<INT_PTR>(-4))
                           /* PER_MONITOR_AWARE_V2 */)) {
            return;
        }
        if (const auto set_context = reinterpret_cast<SetContextFn>(
                ::GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
            set_context != nullptr
            && set_context(reinterpret_cast<HANDLE>(static_cast<INT_PTR>(-2))
                           /* SYSTEM_DPI_AWARE */)) {
            return;
        }
    }
    if (HMODULE shcore = ::LoadLibraryW(L"shcore.dll"); shcore != nullptr) {
        using SetAwarenessFn = HRESULT(WINAPI*)(int);
        if (const auto set_awareness = reinterpret_cast<SetAwarenessFn>(
                ::GetProcAddress(shcore, "SetProcessDpiAwareness"));
            set_awareness != nullptr && SUCCEEDED(set_awareness(process_system_dpi_aware))) {
            ::FreeLibrary(shcore);
            return;
        }
        ::FreeLibrary(shcore);
    }
    if (HMODULE user32 = ::GetModuleHandleW(L"user32.dll"); user32 != nullptr) {
        using SetAwareFn = BOOL(WINAPI*)();
        if (const auto set_aware =
                reinterpret_cast<SetAwareFn>(::GetProcAddress(user32, "SetProcessDpiAware"));
            set_aware != nullptr) {
            set_aware();
        }
    }
}

// ---- About dialog (reference Screenshot_19 layout, dark theme) ----

constexpr wchar_t about_class[] = L"SubornoAboutWindow";
constexpr int about_width = 560;
constexpr int about_height = 430;

// Keyboard-reachable hotspots in tab order (96-DPI design coordinates); the

// same rectangles the mouse handlers hit-test, defined once.

constexpr RECT about_hotspots[]{
    RECT{60, 308, 230, 342},  // 0: Check for Updates

    RECT{250, 308, 460, 342},  // 1: Documentation

    RECT{60, 350, 230, 384},  // 2: License (MIT)

    RECT{250, 350, 460, 384},  // 3: Open Source Notices

    RECT{508, 8, 540, 34},  // 4: Close

};
constexpr int about_hotspot_count = 5;

struct AboutState;
void activate_about_hotspot(AboutState& state);

struct AboutState final : public ui_access::ElementSource {
    HWND window = nullptr;
    UINT dpi = 96;
    HFONT font = nullptr;
    HFONT title_font = nullptr;
    HICON icon = nullptr;
    bool close_hover = false;
    bool license_hover = false;
    bool notices_hover = false;
    bool updates_hover = false;
    bool docs_hover = false;
    // Keyboard focus among about_hotspots (tab order), -1 = none. The dialog

    // previously answered only Esc, so its four actions + close were

    // mouse-only.

    int focus_index = -1;
    // Liveness token for the UIA providers (reset in WM_DESTROY).

    std::shared_ptr<int> accessibility_alive = std::make_shared<int>(0);

    int sh(int pixels) const noexcept {
        return ui::scaled(dpi, pixels);
    }

    // ---- UI Automation ElementSource ----

    // The dialog IS the source (its lifetime is the window's): the five

    // hotspots are announced as buttons with the Invoke pattern, in tab

    // order, with live focus state.

    std::wstring window_name() const override {
        return ui::text("about.dialog.title", L"About Suborno");
    }

    int element_count() const override {
        return about_hotspot_count;
    }

    bool element_info(int index, ui_access::ElementInfo& out) const override {
        if (index < 0 || index >= about_hotspot_count) {
            return false;
        }
        out.role = ui_access::ElementRole::button;
        out.supports_invoke = true;
        out.focusable = true;
        out.focused = focus_index == index;
        const RECT design = about_hotspots[index];
        out.rect = RECT{sh(design.left), sh(design.top), sh(design.right), sh(design.bottom)};
        switch (index) {
        case 0:
            out.name = ui::text("about.check_for_updates", L"Check for Updates");
            return true;
        case 1:
            out.name = ui::text("about.documentation", L"Documentation");
            return true;
        case 2:
            out.name = ui::text("about.license", L"License (MIT)");
            return true;
        case 3:
            out.name = ui::text("about.open_source_notices", L"Open Source Notices");
            return true;
        default:
            out.name = ui::text("common.close", L"Close");
            return true;
        }
    }

    int focused_element() const override {
        return focus_index;
    }

    bool focus_element(int index) override {
        if (index < 0 || index >= about_hotspot_count) {
            return false;
        }
        focus_index = index;
        ::InvalidateRect(window, nullptr, FALSE);
        return true;
    }

    bool invoke_element(int index) override {
        if (index < 0 || index >= about_hotspot_count) {
            return false;
        }
        focus_index = index;
        activate_about_hotspot(*this);
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

int about_sh(const AboutState& state, int pixels) noexcept {
    return ui::scaled(state.dpi, pixels);
}

void open_license_document(HWND owner, const wchar_t* file_name) {
    std::vector<wchar_t> buffer(32768);
    const DWORD length =
        ::GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0) {
        show_error(owner,
                   ui::text("error.license_locate_failed",
                            L"Suborno could not locate the license document."),
                   E_FAIL);
        return;
    }
    std::filesystem::path directory(buffer.data());
    directory = directory.parent_path() / file_name;
    if (!std::filesystem::exists(directory)) {
        show_error(owner,
                   ui::text("error.license_missing",
                            L"The license document is not present in the installation directory."),
                   HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
        return;
    }
    const HINSTANCE result =
        ::ShellExecuteW(owner, L"open", directory.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(result) <= 32) {
        show_error(
            owner,
            ui::text("error.license_open_failed", L"Suborno could not open the license document."),
            E_FAIL);
    }
}

void open_web_link(HWND owner, const wchar_t* url, const wchar_t* failure_message) {
    const HINSTANCE result = ::ShellExecuteW(owner, L"open", url, nullptr, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(result) <= 32) {
        show_error(owner, failure_message, E_FAIL);
    }
}

int about_hotspot_at(const AboutState& state, const POINT& point) noexcept {
    for (int index = 0; index < about_hotspot_count; ++index) {
        const RECT rect = about_hotspots[index];
        if (point.x >= about_sh(state, rect.left) && point.x < about_sh(state, rect.right)
            && point.y >= about_sh(state, rect.top) && point.y < about_sh(state, rect.bottom)) {
            return index;
        }
    }
    return -1;
}

void activate_about_hotspot(AboutState& state) {
    if (state.focus_index < 0 || state.focus_index >= about_hotspot_count) {
        return;
    }
    switch (state.focus_index) {
    case 0:
        open_web_link(
            state.window,
            releases_url,
            ui::text("error.releases_open_failed", L"Suborno could not open the releases page."));
        break;
    case 1:
        open_web_link(state.window,
                      documentation_url,
                      ui::text("error.documentation_open_failed",
                               L"Suborno could not open the documentation."));
        break;
    case 2:
        open_license_document(state.window, L"LICENSE");
        break;
    case 3:
        open_license_document(state.window, L"THIRD_PARTY_NOTICES.md");
        break;
    case 4:
        ::DestroyWindow(state.window);
        break;
    default:
        break;
    }
}

void draw_about_focus_ring(AboutState& state, HDC device_context, RECT design_rect) {
    RECT rect{about_sh(state, design_rect.left),
              about_sh(state, design_rect.top),
              about_sh(state, design_rect.right),
              about_sh(state, design_rect.bottom)};
    ::InflateRect(&rect, about_sh(state, ui::spacing::sm), about_sh(state, ui::spacing::sm));
    HPEN pen = ::CreatePen(PS_SOLID, std::max(1, about_sh(state, 2)), ui::theme_focus());
    if (pen == nullptr) {
        return;
    }
    const HGDIOBJ previous = ::SelectObject(device_context, pen);
    const HGDIOBJ previous_brush = ::SelectObject(device_context, ::GetStockObject(NULL_BRUSH));
    ::RoundRect(device_context,
                rect.left,
                rect.top,
                rect.right,
                rect.bottom,
                about_sh(state, 9),
                about_sh(state, 9));
    ::SelectObject(device_context, previous_brush);
    ::SelectObject(device_context, previous);
    ::DeleteObject(pen);
}

void paint_about_window(AboutState& state, HDC device_context) {
    RECT client{};
    ::GetClientRect(state.window, &client);
    HBRUSH surface = ::CreateSolidBrush(ui::theme_surface());
    if (surface != nullptr) {
        ::FillRect(device_context, &client, surface);
        ::DeleteObject(surface);
    }

    // Title row.

    RECT title_rect{
        about_sh(state, 24), about_sh(state, 8), about_sh(state, 300), about_sh(state, 34)};
    if (state.title_font != nullptr) {
        ::SelectObject(device_context, state.title_font);
    }
    ui::draw_text(device_context,
                  ui::text("about.dialog.title", L"About Suborno"),
                  title_rect,
                  ui::theme_text());
    if (state.font != nullptr) {
        ::SelectObject(device_context, state.font);
    }
    const RECT close_rect{
        about_sh(state, 508), about_sh(state, 8), about_sh(state, 540), about_sh(state, 34)};
    if (state.close_hover) {
        ui::fill_rounded(device_context, close_rect, ui::theme_surface_hover(), about_sh(state, 6));
    }
    const int close_cx = (close_rect.left + close_rect.right) / 2;
    const int close_cy = (close_rect.top + close_rect.bottom) / 2;
    const int close_arm = about_sh(state, 6);
    HPEN close_pen = ui::create_round_pen(about_sh(state, 2), ui::theme_text());
    if (close_pen != nullptr) {
        const HGDIOBJ previous = ::SelectObject(device_context, close_pen);
        ::MoveToEx(device_context, close_cx - close_arm, close_cy - close_arm, nullptr);
        ::LineTo(device_context, close_cx + close_arm, close_cy + close_arm);
        ::MoveToEx(device_context, close_cx + close_arm, close_cy - close_arm, nullptr);
        ::LineTo(device_context, close_cx - close_arm, close_cy + close_arm);
        ::SelectObject(device_context, previous);
        ::DeleteObject(close_pen);
    }

    // Identity block: icon, name, tagline, version, edition.

    if (state.icon != nullptr) {
        ::DrawIconEx(device_context,
                     about_sh(state, 48),
                     about_sh(state, 84),
                     state.icon,
                     about_sh(state, 48),
                     about_sh(state, 48),
                     0,
                     nullptr,
                     DI_NORMAL);
    }
    RECT name_rect{
        about_sh(state, 124), about_sh(state, 84), about_sh(state, 460), about_sh(state, 116)};
    if (state.title_font != nullptr) {
        ::SelectObject(device_context, state.title_font);
    }
    ui::draw_text(device_context, ui::text("about.name", L"Suborno"), name_rect, ui::theme_text());
    if (state.font != nullptr) {
        ::SelectObject(device_context, state.font);
    }
    ui::draw_text(
        device_context,
        ui::text("about.tagline", L"A FREE Bangla IME"),
        RECT{
            about_sh(state, 124), about_sh(state, 124), about_sh(state, 460), about_sh(state, 146)},
        ui::theme_text());
    ui::draw_text(
        device_context,
        ui::format_text(
            ui::text("about.version", L"Version {version}"), L"{version}", SUBORNO_VERSION_WIDE)
            .c_str(),
        RECT{
            about_sh(state, 124), about_sh(state, 148), about_sh(state, 460), about_sh(state, 170)},
        ui::theme_text());
    ui::draw_text(
        device_context,
        ui::text("about.free", L"FREE"),
        RECT{
            about_sh(state, 124), about_sh(state, 172), about_sh(state, 460), about_sh(state, 194)},
        ui::theme_text());

    ui::draw_text(device_context,
                  ui::text("about.copyright", L"© SUBORNO contributors. MIT License."),
                  RECT{0, about_sh(state, 396), about_sh(state, about_width), about_sh(state, 418)},
                  ui::theme_text_inactive(),
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    // Action row: Check for Updates + Documentation (real web destinations).

    const RECT updates_rect{
        about_sh(state, 60), about_sh(state, 308), about_sh(state, 230), about_sh(state, 342)};
    const RECT docs_rect{
        about_sh(state, 250), about_sh(state, 308), about_sh(state, 460), about_sh(state, 342)};
    ui::fill_rounded(device_context,
                     updates_rect,
                     state.updates_hover ? ui::theme_surface_hover() : ui::theme_surface_raised(),
                     about_sh(state, 6));
    ui::fill_rounded(device_context,
                     docs_rect,
                     state.docs_hover ? ui::theme_surface_hover() : ui::theme_surface_raised(),
                     about_sh(state, 6));
    ui::draw_text(device_context,
                  ui::text("about.check_for_updates", L"Check for Updates"),
                  updates_rect,
                  ui::theme_text(),
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    ui::draw_text(device_context,
                  ui::text("about.documentation", L"Documentation"),
                  docs_rect,
                  ui::theme_text(),
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    // License document buttons.

    const RECT license_rect{
        about_sh(state, 60), about_sh(state, 350), about_sh(state, 230), about_sh(state, 384)};
    const RECT notices_rect{
        about_sh(state, 250), about_sh(state, 350), about_sh(state, 460), about_sh(state, 384)};
    ui::fill_rounded(device_context,
                     license_rect,
                     state.license_hover ? ui::theme_surface_hover() : ui::theme_surface_raised(),
                     about_sh(state, 6));
    ui::fill_rounded(device_context,
                     notices_rect,
                     state.notices_hover ? ui::theme_surface_hover() : ui::theme_surface_raised(),
                     about_sh(state, 6));
    ui::draw_text(device_context,
                  ui::text("about.license", L"License (MIT)"),
                  license_rect,
                  ui::theme_text(),
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    ui::draw_text(device_context,
                  ui::text("about.open_source_notices", L"Open Source Notices"),
                  notices_rect,
                  ui::theme_text(),
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    // Keyboard focus ring around the tab-order hotspot (about_sh scales the

    // design coordinates; see about_hotspots).

    if (state.focus_index >= 0 && state.focus_index < about_hotspot_count) {
        draw_about_focus_ring(state, device_context, about_hotspots[state.focus_index]);
    }
}

LRESULT CALLBACK about_window_proc(HWND window, UINT message, WPARAM w_param, LPARAM l_param) {
    auto* state = reinterpret_cast<AboutState*>(::GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(l_param);
        state = static_cast<AboutState*>(create->lpCreateParams);
        ::SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
        state->window = window;
    }
    if (state == nullptr) {
        return ::DefWindowProcW(window, message, w_param, l_param);
    }
    switch (message) {
    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC device_context = ::BeginPaint(window, &paint);
        if (device_context != nullptr) {
            paint_about_window(*state, device_context);
            ::EndPaint(window, &paint);
        }
        return 0;
    }
    case WM_ERASEBKGND:
        return TRUE;
    case WM_MOUSEMOVE: {
        const POINT point{GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
        const bool close_now = point.x >= about_sh(*state, 508) && point.x <= about_sh(*state, 540)
                               && point.y >= about_sh(*state, 8) && point.y <= about_sh(*state, 34);
        const bool license_now = point.x >= about_sh(*state, 60) && point.x <= about_sh(*state, 230)
                                 && point.y >= about_sh(*state, 350)
                                 && point.y <= about_sh(*state, 384);
        const bool notices_now =
            point.x >= about_sh(*state, 250) && point.x <= about_sh(*state, 460)
            && point.y >= about_sh(*state, 350) && point.y <= about_sh(*state, 384);
        const bool updates_now = point.x >= about_sh(*state, 60) && point.x <= about_sh(*state, 230)
                                 && point.y >= about_sh(*state, 308)
                                 && point.y <= about_sh(*state, 342);
        const bool docs_now = point.x >= about_sh(*state, 250) && point.x <= about_sh(*state, 460)
                              && point.y >= about_sh(*state, 308)
                              && point.y <= about_sh(*state, 342);
        if (state->close_hover != close_now || state->license_hover != license_now
            || state->notices_hover != notices_now || state->updates_hover != updates_now
            || state->docs_hover != docs_now) {
            state->close_hover = close_now;
            state->license_hover = license_now;
            state->notices_hover = notices_now;
            state->updates_hover = updates_now;
            state->docs_hover = docs_now;
            ::InvalidateRect(window, nullptr, FALSE);
        }
        return 0;
    }
    case WM_LBUTTONDOWN: {
        const POINT point{GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
        if (point.y >= about_sh(*state, 8) && point.y <= about_sh(*state, 34)
            && point.x <= about_sh(*state, 508)) {
            ::ReleaseCapture();
            ::PostMessageW(window, WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x, point.y));
            return 0;
        }
        // Mouse and keyboard share one focus model: clicking a hotspot moves

        // the focus ring there, then fires the same action Enter/Space would.

        const int hotspot = about_hotspot_at(*state, point);
        if (hotspot >= 0) {
            state->focus_index = hotspot;
            activate_about_hotspot(*state);
        }
        return 0;
    }
    case WM_KEYDOWN:
        if (w_param == VK_ESCAPE) {
            ::DestroyWindow(window);
            return 0;
        }
        if (w_param == VK_TAB) {
            const bool backwards = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
            // Every hotspot is focusable; cycle in declared tab order.

            int next = state->focus_index < 0
                           ? (backwards ? about_hotspot_count - 1 : 0)
                           : (backwards ? state->focus_index - 1 : state->focus_index + 1);
            if (next < 0) {
                next = about_hotspot_count - 1;
            }
            if (next >= about_hotspot_count) {
                next = 0;
            }
            state->focus_index = next;
            ::InvalidateRect(window, nullptr, FALSE);
            return 0;
        }
        if (w_param == VK_SPACE || w_param == VK_RETURN) {
            activate_about_hotspot(*state);
        }
        return 0;
    case WM_GETOBJECT: {
        bool handled = false;
        const LRESULT result = ui_access::handle_get_object(
            window, w_param, l_param, state, state->accessibility_alive, handled);
        if (handled) {
            return result;
        }
        return ::DefWindowProcW(window, message, w_param, l_param);
    }
    case WM_DPICHANGED: {
        // Per-Monitor v2: rescale the fixed design layout on the new

        // monitor (fonts, region and geometry; same pattern as the topbar).

        state->dpi = HIWORD(w_param);
        if (state->font != nullptr) {
            ::DeleteObject(state->font);
            state->font = nullptr;
        }
        if (state->title_font != nullptr) {
            ::DeleteObject(state->title_font);
            state->title_font = nullptr;
        }
        state->font = ui::create_message_font();
        state->title_font = ui::create_title_font(state->dpi);
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
            ui::apply_window_treatment(window, width, height, about_sh(*state, 8));
        }
        ::InvalidateRect(window, nullptr, TRUE);
        return 0;
    }
    case WM_THEMECHANGED:
        ui::refresh_active_theme();
        ::InvalidateRect(window, nullptr, TRUE);
        return 0;
    case WM_DESTROY:
        state->window = nullptr;
        state->accessibility_alive.reset();
        if (state->icon != nullptr) {
            ::DestroyIcon(state->icon);
            state->icon = nullptr;
        }
        return 0;
    case WM_NCDESTROY:
        ::SetWindowLongPtrW(window, GWLP_USERDATA, 0);
        delete state;  // the window owned the state since show_about_dialog()

        return ::DefWindowProcW(window, message, w_param, l_param);
    default:
        return ::DefWindowProcW(window, message, w_param, l_param);
    }
}

ATOM register_about_class(HINSTANCE instance) {
    static const ATOM registered = [instance] {
        WNDCLASSEXW window_class{};
        window_class.cbSize = sizeof(window_class);
        // Native elevation: the small DWM drop shadow under the frameless

        // popup (CS_DROPSHADOW is a documented class style, available on

        // every supported Windows version).

        window_class.style = CS_DROPSHADOW;
        window_class.lpfnWndProc = &about_window_proc;
        window_class.hInstance = instance;
        window_class.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
        window_class.lpszClassName = about_class;
        return ::RegisterClassExW(&window_class);
    }();
    return registered;
}

// Modeless About dialog; a second invocation focuses the existing window.

void show_about_dialog(HINSTANCE instance, HWND owner) {
    if (register_about_class(instance) == 0) {
        show_error(owner,
                   ui::text("error.about_dialog_create_failed",
                            L"Suborno could not create its About dialog."),
                   E_FAIL);
        return;
    }
    if (const HWND existing = ::FindWindowW(about_class, nullptr); existing != nullptr) {
        ::ShowWindow(existing, SW_RESTORE);
        ::SetForegroundWindow(existing);
        return;
    }
    auto state = std::make_unique<AboutState>();
    // Fit cap: the 560x430 dialog must not overflow short work areas at high

    // DPI (same reasoning as the settings window in run()).

    UINT about_dpi = ui::system_dpi_for(owner);
    {
        const HMONITOR monitor = ::MonitorFromWindow(
            owner != nullptr ? owner : ::GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO info{};
        info.cbSize = sizeof(info);
        if (monitor != nullptr && ::GetMonitorInfoW(monitor, &info) != FALSE) {
            about_dpi = ui::dpi_fitting(info.rcWork, about_width, about_height, about_dpi);
        }
    }
    state->dpi = about_dpi;
    state->font = ui::create_message_font();
    state->title_font = ui::create_title_font(state->dpi);
    state->focus_index = 0;  // the window takes keyboard focus below

    state->icon = static_cast<HICON>(
        ::LoadImageW(instance, MAKEINTRESOURCEW(IDI_SUBORNO), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
    const int width = about_sh(*state, about_width);
    const int height = about_sh(*state, about_height);
    const HWND window = ::CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
                                          about_class,
                                          ui::text("about.dialog.title", L"About Suborno"),
                                          WS_POPUP,
                                          CW_USEDEFAULT,
                                          CW_USEDEFAULT,
                                          width,
                                          height,
                                          owner,
                                          nullptr,
                                          instance,
                                          state.get());
    if (window == nullptr) {
        show_error(owner,
                   ui::text("error.about_dialog_create_failed",
                            L"Suborno could not create its About dialog."),
                   E_FAIL);
        return;
    }
    ui::apply_window_treatment(window, width, height, about_sh(*state, 8));
    const HMONITOR monitor = ::MonitorFromWindow(owner ? owner : window, MONITOR_DEFAULTTONEAREST);
    MONITORINFO info{};
    info.cbSize = sizeof(info);
    if (::GetMonitorInfoW(monitor, &info) != FALSE) {
        const int x = info.rcWork.left + (info.rcWork.right - info.rcWork.left - width) / 2;
        const int y = info.rcWork.top + (info.rcWork.bottom - info.rcWork.top - height) / 2;
        ::SetWindowPos(window, nullptr, x, y, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
    }
    ::ShowWindow(window, SW_SHOW);
    ::SetFocus(window);
    // Ownership passes to the window procedure; it deletes the state on

    // WM_NCDESTROY. On the failure paths above the unique_ptr cleans up.

    (void)state.release();
}

class Application final : public ui_access::ElementSource {
  public:
    Application(HINSTANCE instance, bool startup, bool smoke_test)
        : instance_(instance),
          startup_(startup),
          smoke_test_(smoke_test),
          taskbar_created_message_(::RegisterWindowMessageW(L"TaskbarCreated")) {}

    ~Application() {
        if (message_font_ != nullptr) {
            ::DeleteObject(message_font_);
            message_font_ = nullptr;
        }
        if (font_title_ != nullptr) {
            ::DeleteObject(font_title_);
            font_title_ = nullptr;
        }
        if (font_heading_ != nullptr) {
            ::DeleteObject(font_heading_);
            font_heading_ = nullptr;
        }
        if (font_caption_ != nullptr) {
            ::DeleteObject(font_caption_);
            font_caption_ = nullptr;
        }
    }

    // Scales a 96-DPI design pixel measurement to the system DPI the window

    // was created with.

    int scaled(int pixels) const noexcept {
        return pixels * static_cast<int>(ui_dpi_) / 96;
    }

    // ---- UI Automation ElementSource (settings window) ----

    // The window object IS the source, so UIA callbacks after WM_GETOBJECT

    // are always in bounds; a11y_alive_ catches the destruction window.

    // Element order: 0..3 sidebar pages, 4.. the current page's controls,

    // then Save, Cancel and Close.

    static constexpr int a11y_sidebar_count = 4;

    int a11y_save_index() const {
        return a11y_sidebar_count + static_cast<int>(pages_[static_cast<int>(page_)].size());
    }
    int a11y_cancel_index() const {
        return a11y_save_index() + 1;
    }
    int a11y_close_index() const {
        return a11y_cancel_index() + 1;
    }

    // Stable UIA selectors (the winapp-ui-automation skill's preferred

    // targeting form) are built by control_automation_id() below; it needs

    // UiControl as a parameter type, so it is declared after the struct.

    static const wchar_t* action_automation_name(int action) {
        switch (action) {
        case action_remember_layout:
            return L"remember-layout";
        case action_start_with_windows:
            return L"start-with-windows";
        case action_layout_dropdown:
            return L"default-layout";
        case action_show_candidate_window:
            return L"show-candidate-window";
        case action_idle_dimming:
            return L"idle-dimming";
        case action_idle_opacity:
            return L"idle-opacity";
        case action_idle_time:
            return L"idle-time";
        case action_map_numpad:
            return L"map-numpad";
        case action_ignore_caps_lock:
            return L"ignore-caps-lock";
        case action_number_keys:
            return L"number-keys";
        case action_start_mode:
            return L"start-mode";
        case action_font_dropdown:
            return L"font";
        case action_use_os_default_font:
            return L"use-os-default-font";
        case action_remember_topbar_position:
            return L"remember-topbar-position";
        case action_layout_shortcut:
            return L"layout-shortcut";
        case action_theme_dropdown:
            return L"theme";
        case action_next_word_prediction:
            return L"next-word-prediction";
        case action_orientation:
            return L"orientation";
        case action_fixed_height:
            return L"candidate-fixed-height";
        case action_sliding_effect:
            return L"sliding-effect";
        case action_follow_caret:
            return L"follow-caret";
        case action_follow_mouse:
            return L"follow-mouse";
        case action_switching_mode:
            return L"switching-mode";
        case action_change_locale:
            return L"change-locale";
        case action_encoding:
            return L"encoding";
        case action_disable_encoding_shortcut:
            return L"disable-encoding-shortcut";
        case action_learning:
            return L"learning";
        case action_clear_learnt:
            return L"clear-learnt";
        case action_ui_language:
            return L"ui-language";
        default:
            return L"unknown";
        }
    }

    static constexpr const wchar_t* sidebar_automation_ids[page_count]{
        L"sidebar.general", L"sidebar.interface", L"sidebar.topbar", L"sidebar.input"};

    std::wstring window_name() const override {
        return ui::text("settings.window.title", L"Suborno Settings");
    }

    int element_count() const override {
        return a11y_close_index() + 1;
    }

    bool element_info(int index, ui_access::ElementInfo& out) const override {
        const std::vector<UiControl>& controls = pages_[static_cast<int>(page_)];
        if (index >= 0 && index < a11y_sidebar_count) {
            out.name = sidebar_title(index);
            out.role = ui_access::ElementRole::list_item;
            out.automation_id = sidebar_automation_ids[index];
            out.rect = scale_rect(design_rect(sidebar_left,
                                              sidebar_top + index * sidebar_stride,
                                              sidebar_width,
                                              sidebar_pill_height));
            out.supports_invoke = true;
            out.focusable = true;
            out.focused = static_cast<int>(page_) == index;
            return true;
        }
        const int control_index = index - a11y_sidebar_count;
        if (control_index >= 0 && control_index < static_cast<int>(controls.size())) {
            const UiControl& control = controls[static_cast<std::size_t>(control_index)];
            out.rect = scale_rect(control.rect);
            out.automation_id = control_automation_id(control, control_index);
            out.focusable =
                control.kind != CtrlKind::section_label && control.kind != CtrlKind::status;
            out.focused = out.focusable && focus_footer_ == 0 && focus_index_ == control_index;
            switch (control.kind) {
            case CtrlKind::section_label:
            case CtrlKind::status:
                out.role = ui_access::ElementRole::text;
                out.name = control.label;
                return true;
            case CtrlKind::checkbox:
                out.role = ui_access::ElementRole::checkbox;
                out.name = control.label;
                out.supports_toggle = true;
                out.toggle_state_on = control.checked;
                return true;
            case CtrlKind::dropdown:
                out.role = ui_access::ElementRole::dropdown;
                out.name = dropdown_name(control_index);
                out.supports_expand_collapse = true;
                out.expanded = false;  // the popup is transient

                return true;
            case CtrlKind::slider:
                out.role = ui_access::ElementRole::slider;
                out.name = control.label;
                out.supports_range = true;
                out.range_min = static_cast<double>(control.value_min);
                out.range_max = static_cast<double>(control.value_max);
                out.range_value = static_cast<double>(control.value);
                return true;
            case CtrlKind::button:
                out.role = ui_access::ElementRole::button;
                out.name = control.label;
                out.supports_invoke = true;
                return true;
            }
            return false;
        }
        if (index == a11y_save_index()) {
            out.name = ui::text("settings.action.save", L"Save");
            out.role = ui_access::ElementRole::button;
            out.automation_id = L"action.save";
            out.rect = scale_rect(save_button_rect());
            out.supports_invoke = true;
            out.focusable = true;
            out.focused = focus_footer_ == 1;
            return true;
        }
        if (index == a11y_cancel_index()) {
            out.name = ui::text("settings.action.cancel", L"Cancel");
            out.role = ui_access::ElementRole::button;
            out.automation_id = L"action.cancel";
            out.rect = scale_rect(cancel_button_rect());
            out.supports_invoke = true;
            out.focusable = true;
            out.focused = focus_footer_ == 2;
            return true;
        }
        if (index == a11y_close_index()) {
            out.name = ui::text("common.close", L"Close");
            out.role = ui_access::ElementRole::button;
            out.automation_id = L"action.close";
            out.rect = scale_rect(design_rect(696, 4, 32, 32));
            out.supports_invoke = true;
            return true;
        }
        return false;
    }

    int focused_element() const override {
        if (focus_footer_ == 1) {
            return a11y_save_index();
        }
        if (focus_footer_ == 2) {
            return a11y_cancel_index();
        }
        if (focus_index_ >= 0) {
            return a11y_sidebar_count + focus_index_;
        }
        return -1;
    }

    bool focus_element(int index) override {
        if (index >= 0 && index < a11y_sidebar_count) {
            page_ = static_cast<PageId>(index);
            focus_index_ = -1;
            focus_footer_ = 0;
            ::InvalidateRect(window_, nullptr, TRUE);
            return true;
        }
        const int control_index = index - a11y_sidebar_count;
        if (control_index >= 0
            && control_index < static_cast<int>(pages_[static_cast<int>(page_)].size())) {
            focus_index_ = control_index;
            focus_footer_ = 0;
            ::InvalidateRect(window_, nullptr, FALSE);
            return true;
        }
        if (index == a11y_save_index()) {
            focus_footer_ = 1;
            focus_index_ = -1;
            ::InvalidateRect(window_, nullptr, FALSE);
            return true;
        }
        if (index == a11y_cancel_index()) {
            focus_footer_ = 2;
            focus_index_ = -1;
            ::InvalidateRect(window_, nullptr, FALSE);
            return true;
        }
        return false;
    }

    bool invoke_element(int index) override {
        if (index >= 0 && index < a11y_sidebar_count) {
            page_ = static_cast<PageId>(index);
            focus_index_ = -1;
            ::InvalidateRect(window_, nullptr, TRUE);
            return true;
        }
        const int control_index = index - a11y_sidebar_count;
        std::vector<UiControl>& controls = page_controls();
        if (control_index >= 0 && control_index < static_cast<int>(controls.size())) {
            UiControl& control = controls[static_cast<std::size_t>(control_index)];
            focus_index_ = control_index;
            switch (control.kind) {
            case CtrlKind::checkbox:
                toggle_staged_checkbox(control);
                return true;
            case CtrlKind::dropdown:
                open_dropdown(control);
                return true;
            case CtrlKind::button:
                run_button_action(control.action);
                return true;
            default:
                return false;  // static text and sliders have no invoke action
            }
        }
        if (index == a11y_save_index()) {
            save_settings_from_ui();
            return true;
        }
        if (index == a11y_cancel_index() || index == a11y_close_index()) {
            cancel_settings();
            return true;
        }
        return false;
    }

    bool toggle_element(int index) override {
        const int control_index = index - a11y_sidebar_count;
        std::vector<UiControl>& controls = page_controls();
        if (control_index < 0 || control_index >= static_cast<int>(controls.size())) {
            return false;
        }
        UiControl& control = controls[static_cast<std::size_t>(control_index)];
        if (control.kind != CtrlKind::checkbox) {
            return false;
        }
        focus_index_ = control_index;
        toggle_staged_checkbox(control);
        return true;
    }

    bool expand_element(int index, bool expand) override {
        if (!expand) {
            return false;  // the transient popup menu owns the collapse
        }
        const int control_index = index - a11y_sidebar_count;
        std::vector<UiControl>& controls = page_controls();
        if (control_index < 0 || control_index >= static_cast<int>(controls.size())) {
            return false;
        }
        UiControl& control = controls[static_cast<std::size_t>(control_index)];
        if (control.kind != CtrlKind::dropdown) {
            return false;
        }
        focus_index_ = control_index;
        open_dropdown(control);
        return true;
    }

    bool set_element_value(int index, double value) override {
        const int control_index = index - a11y_sidebar_count;
        std::vector<UiControl>& controls = page_controls();
        if (control_index < 0 || control_index >= static_cast<int>(controls.size())) {
            return false;
        }
        UiControl& control = controls[static_cast<std::size_t>(control_index)];
        if (control.kind != CtrlKind::slider) {
            return false;
        }
        apply_staged_slider(control, static_cast<int>(std::lround(value)));
        return true;
    }

    int run(int show_command) {
        (void)show_command;  // start surface is chosen by apply_start_mode()

        enable_process_dpi_awareness();
        // Primary work area first: the settings window is a fixed 736x620

        // design layout, so its scale factor is capped to keep Save/Cancel on

        // screen at high DPI on short displays (125% laptops, 200% desktops).

        RECT work_area{};
        if (const HMONITOR monitor = ::MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY);
            monitor != nullptr) {
            MONITORINFO info{};
            info.cbSize = sizeof(info);
            if (::GetMonitorInfoW(monitor, &info) != FALSE) {
                work_area = info.rcWork;
            }
        }
        ui_dpi_ = ui::dpi_fitting(work_area, window_width, window_height, system_dpi());
        message_font_ = ui::create_message_font();
        font_title_ = ui::create_title_font(ui_dpi_);
        font_heading_ = ui::create_heading_font(ui_dpi_);
        font_caption_ = ui::create_caption_font(ui_dpi_);
        HRESULT result = initialize();
        if (FAILED(result)) {
            // initialize() has already reported the specific failing

            // operation, path, and error codes; only echo the failure code

            // here (visible in debuggers and to the smoke-test harness).

            report_error(nullptr, settings_failure_message, result, false);
            return smoke_test_ ? smoke_test_exit_code(result) : 1;
        }
        // Interface language: data/i18n ships next to the executable; a

        // missing or unparsable file falls back to the compiled English

        // strings, so this can never break the first paint.

        apply_ui_language(engine_->settings().ui.language);

        WNDCLASSEXW window_class{};
        window_class.cbSize = sizeof(window_class);
        window_class.style = CS_DROPSHADOW;  // native elevation under the frameless window

        window_class.hInstance = instance_;
        window_class.lpfnWndProc = &Application::window_proc;
        window_class.lpszClassName = window_class;
        window_class.hIcon = ::LoadIconW(instance_, MAKEINTRESOURCEW(IDI_SUBORNO));
        window_class.hIconSm = window_class.hIcon;
        window_class.hCursor = ::LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
        window_class.hbrBackground = nullptr;  // fully custom-painted dark window

        if (window_class.hIcon == nullptr) {
            report_error(nullptr,
                         ui::text("error.icon_load_failed",
                                  L"Suborno's application icon could not be loaded."),
                         E_FAIL,
                         !smoke_test_);
            return smoke_test_ ? smoke_test_exit_code(E_FAIL) : 1;
        }
        if (::RegisterClassExW(&window_class) == 0) {
            const HRESULT error = HRESULT_FROM_WIN32(::GetLastError());
            report_error(nullptr,
                         ui::text("error.window_class_register_failed",
                                  L"Suborno's application window could not be registered."),
                         error,
                         !smoke_test_);
            return smoke_test_ ? smoke_test_exit_code(error) : 1;
        }

        // Frameless dark window (reference settings dialog): 736x620 design

        // px, centered on the primary work area, rounded corners applied

        // after creation. The work area (and the DPI fitted to it) was

        // resolved at the top of run().

        const int window_width = scaled(window_width);
        const int window_height = scaled(window_height);
        const int window_x =
            work_area.left + std::max(0L, ((work_area.right - work_area.left) - window_width) / 2);
        const int window_y =
            work_area.top + std::max(0L, ((work_area.bottom - work_area.top) - window_height) / 2);
        window_ = ::CreateWindowExW(0,
                                    window_class,
                                    ui::text("settings.window.title", L"Suborno Settings"),
                                    WS_POPUP,
                                    window_x,
                                    window_y,
                                    window_width,
                                    window_height,
                                    nullptr,
                                    nullptr,
                                    instance_,
                                    this);
        if (window_ == nullptr) {
            const HRESULT error = HRESULT_FROM_WIN32(::GetLastError());
            report_error(nullptr,
                         ui::text("error.window_create_failed",
                                  L"Suborno's application window could not be created."),
                         error,
                         !smoke_test_);
            ::UnregisterClassW(window_class, instance_);
            return smoke_test_ ? smoke_test_exit_code(error) : 1;
        }
        ui::apply_window_treatment(window_, window_width, window_height, scaled(8));
        staged_ = engine_->settings();
        build_pages();
        if (smoke_test_) {
            ::DestroyWindow(window_);
            ::UnregisterClassW(window_class, instance_);
            return 0;
        }

        // Apply the saved theme before any window paints, register the

        // layout/encoding hotkeys, and start in the surface the user

        // configured (topbar / tray / settings window / last state).

        apply_theme(engine_->settings().ui.theme_id);
        refresh_app_fonts();
        register_layout_hotkey();
        register_encoding_hotkey();
        apply_start_mode();
        setup_topbar();
        setup_osk();
        toast_.create(instance_);  // optional UI: failure just means no toasts

        MSG message{};
        int message_result = 0;
        // The custom window implements its own Tab/Space/Enter/Esc keyboard

        // handling, so messages are dispatched directly.

        while ((message_result = ::GetMessageW(&message, nullptr, 0, 0)) > 0) {
            ::TranslateMessage(&message);
            ::DispatchMessageW(&message);
        }
        ::UnregisterClassW(window_class, instance_);
        if (message_result == -1) {
            return 1;
        }
        return static_cast<int>(message.wParam);
    }

  private:
    HRESULT initialize() {
        try {
            std::filesystem::path user_data;
            std::filesystem::path legacy_user_data;
            HRESULT result = get_user_data_directory(user_data);
            if (FAILED(result)) {
                return report_settings_failure(
                    L"Resolve and create the RoamingAppData user directory",
                    user_data,
                    result,
                    !smoke_test_);
            }
            result = get_legacy_user_data_directory(legacy_user_data);
            if (FAILED(result)) {
                return report_settings_failure(L"Resolve the legacy LocalAppData directory",
                                               legacy_user_data,
                                               result,
                                               !smoke_test_);
            }
            result = prepare_user_data_directory(user_data, legacy_user_data);
            if (FAILED(result)) {
                return report_settings_failure(
                    L"Create the user directory hierarchy or migrate legacy data",
                    user_data,
                    result,
                    !smoke_test_);
            }

            suborno::core::EngineConfig config;
            std::filesystem::path settings_path;
            result = get_user_settings_path(settings_path);
            if (FAILED(result)) {
                return report_settings_failure(
                    L"Resolve the settings file path", settings_path, result, !smoke_test_);
            }
            config.settings_path = settings_path;
            std::filesystem::path dictionary_path;
            result = get_user_dictionary_path(dictionary_path);
            if (FAILED(result)) {
                return report_settings_failure(
                    L"Resolve the user dictionary path", dictionary_path, result, !smoke_test_);
            }
            config.dictionary_path = dictionary_path;
            std::filesystem::path assets_path;
            static_cast<void>(get_shared_assets_directory(assets_path));
            config.assets_path = assets_path;
            engine_ = std::make_unique<suborno::core::Engine>(config);
            auto loaded = engine_->load_settings();
            if (loaded.status == suborno::settings::SettingsStatus::invalid_format
                || loaded.status == suborno::settings::SettingsStatus::unsupported_version) {
                // The settings file exists but cannot be honored. Quarantine

                // it (contents preserved beside itself under a .corrupt- name)

                // and start from defaults — the user's data is neither

                // overwritten nor deleted.

                const std::error_code quarantined =
                    suborno::persistence::quarantine_file(settings_path);
                if (quarantined) {
                    return report_settings_failure(
                        L"Quarantine a settings file that could not be parsed",
                        settings_path,
                        HRESULT_FROM_WIN32(static_cast<DWORD>(quarantined.value())),
                        !smoke_test_);
                }
                engine_ = std::make_unique<suborno::core::Engine>(config);
                loaded = engine_->load_settings();
            }
            if (loaded.status != suborno::settings::SettingsStatus::ok) {
                wchar_t operation[192]{};
                swprintf_s(
                    operation,
                    _countof(operation),
                    L"Read the settings file (the file %s)",
                    loaded.file_existed ? L"exists but could not be opened" : L"did not exist");
                return report_settings_failure(
                    operation, settings_path, HRESULT_FROM_WIN32(ERROR_INVALID_DATA), !smoke_test_);
            }
            if (!loaded.file_existed) {
                // First launch: persist the defaults so the settings file

                // exists for later launches and restarts. A failure here is

                // reported for diagnostics but is not fatal — the engine

                // keeps running with the same values in memory.

                if (engine_->save_settings(engine_->settings())
                    != suborno::settings::SettingsStatus::ok) {
                    const std::wstring note = L"Suborno could not write its default settings file: "
                                              + settings_path.wstring() + L"\n";
                    ::OutputDebugStringW(note.c_str());
                }
            }
            result = read_startup_preference(startup_enabled_);
            if (FAILED(result)) {
                return result;
            }

            auto settings = engine_->settings();
            if (settings.general.start_with_windows != startup_enabled_) {
                settings.general.start_with_windows = startup_enabled_;
                if (engine_->save_settings(settings) != suborno::settings::SettingsStatus::ok) {
                    return HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
                }
            }
            return S_OK;
        } catch (const std::bad_alloc&) {
            return E_OUTOFMEMORY;
        } catch (const std::exception&) {
            return E_FAIL;
        }
    }

    static LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM w_param, LPARAM l_param) {
        Application* app =
            reinterpret_cast<Application*>(::GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            const auto* create = reinterpret_cast<const CREATESTRUCTW*>(l_param);
            app = static_cast<Application*>(create->lpCreateParams);
            app->window_ = window;
            ::SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        }
        if (app == nullptr) {
            return ::DefWindowProcW(window, message, w_param, l_param);
        }
        return app->handle_message(message, w_param, l_param);
    }

    LRESULT handle_message(UINT message, WPARAM w_param, LPARAM l_param) {
        if (message == taskbar_created_message_) {
            const HRESULT result = add_tray_icon();
            if (FAILED(result)) {
                show_error(window_,
                           ui::text("error.tray_icon_restore_failed",
                                    L"Suborno could not restore its notification area icon."),
                           result);
            }
            refresh_status();
            return 0;
        }
        switch (message) {
        case WM_CREATE:
            if (!smoke_test_) {
                const HRESULT result = add_tray_icon();
                if (FAILED(result)) {
                    show_error(window_,
                               ui::text("error.tray_icon_add_failed",
                                        L"Suborno could not add its notification area icon."),
                               result);
                }
            }
            refresh_status();
            return 0;
        case WM_ACTIVATE:
            if (LOWORD(w_param) != WA_INACTIVE) {
                refresh_status();
            }
            return 0;
        case WM_COMMAND:
            handle_menu_command(LOWORD(w_param));
            return 0;
        case WM_HOTKEY:
            if (w_param == layout_hotkey_id) {
                cycle_layout();
            } else if (w_param == encoding_hotkey_id) {
                toggle_encoding();
            }
            return 0;
        case second_instance_show_topbar:
            // The user launched Suborno again (desktop icon / start menu):

            // reveal the floating topbar — the main UI — instead of the

            // settings window.

            show_topbar();
            return 0;
        case WM_CLOSE:
            // Cancel semantics: discard staged changes and hide to the tray.

            if (!smoke_test_ && tray_added_) {
                ::ShowWindow(window_, SW_HIDE);
                return 0;
            }
            ::DestroyWindow(window_);
            return 0;
        case tray_message:
            handle_tray_message(l_param);
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT paint{};
            HDC device_context = ::BeginPaint(window_, &paint);
            if (device_context == nullptr) {
                return 0;
            }
            paint_settings_window(device_context);
            ::EndPaint(window_, &paint);
            return 0;
        }
        case WM_ERASEBKGND:
            return TRUE;
        case WM_MOUSEMOVE:
            if (!mouse_tracked_) {
                TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE, window_, 0};
                if (::TrackMouseEvent(&track) != FALSE) {
                    mouse_tracked_ = true;
                }
            }
            handle_settings_mouse_move(GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param));
            return 0;
        case WM_MOUSELEAVE:
            mouse_tracked_ = false;
            handle_settings_mouse_leave();
            return 0;
        case WM_LBUTTONDOWN:
            handle_settings_mouse_down(GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param));
            return 0;
        case WM_LBUTTONUP:
            handle_settings_mouse_up(GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param));
            return 0;
        case WM_KEYDOWN:
            handle_settings_key_down(static_cast<int>(w_param));
            return 0;
        case WM_GETOBJECT: {
            bool handled = false;
            const LRESULT result =
                ui_access::handle_get_object(window_, w_param, l_param, this, a11y_alive_, handled);
            if (handled) {
                return result;
            }
            return ::DefWindowProcW(window_, message, w_param, l_param);
        }
        case WM_DPICHANGED:
            handle_settings_dpi_changed(w_param, l_param);
            return 0;
        case WM_THEMECHANGED:
            // High Contrast toggled on/off (or the system light/dark switch):

            // re-derive the shared palette and repaint every live surface.

            ui::refresh_active_theme();
            ::InvalidateRect(window_, nullptr, TRUE);
            return 0;
        case WM_DESTROY:
            a11y_alive_.reset();
            toast_.shutdown();
            remove_tray_icon();
            if (hotkey_registered_) {
                ::UnregisterHotKey(window_, layout_hotkey_id);
                hotkey_registered_ = false;
            }
            if (encoding_hotkey_registered_) {
                ::UnregisterHotKey(window_, encoding_hotkey_id);
                encoding_hotkey_registered_ = false;
            }
            ::PostQuitMessage(0);
            return 0;
        default:
            return ::DefWindowProcW(window_, message, w_param, l_param);
        }
    }

    // ---- Reference-styled dark settings UI (custom-drawn, clean-room) ----

    static constexpr int content_left = 232;
    static constexpr int sidebar_left = 28;
    static constexpr int sidebar_width = 132;
    static constexpr int sidebar_top = 64;
    static constexpr int sidebar_stride = 44;
    static constexpr int sidebar_pill_height = 36;
    static constexpr int title_height = 40;
    static constexpr int window_width = 736;
    static constexpr int window_height = 620;

    struct UiControl {
        CtrlKind kind = CtrlKind::section_label;
        int action = 0;  // semantic action; 0 = static

        RECT rect{0, 0, 0, 0};  // 96-DPI design coordinates

        std::wstring label;
        std::wstring description;  // section hint drawn inline after the title

        bool checked = false;  // checkbox (staged)

        int dropdown_index = -1;
        std::vector<std::wstring> dropdown_items;
        int value = 0;  // slider (staged)

        int value_min = 0;
        int value_max = 100;
        bool hover = false;
        bool focus = false;
    };

    // Stable UIA selectors (the winapp-ui-automation skill's preferred

    // targeting form): semantic, derived from the control's semantic action

    // constant, so they survive layout changes and localization.

    std::wstring control_automation_id(const UiControl& control, int control_index) const {
        switch (control.kind) {
        case CtrlKind::section_label:
        case CtrlKind::status:
            return L"text." + std::to_wstring(control_index);
        case CtrlKind::checkbox:
        case CtrlKind::dropdown:
        case CtrlKind::slider:
        case CtrlKind::button:
            return std::wstring(L"action.") + action_automation_name(control.action);
        }
        return L"";
    }

    RECT design_rect(int x, int y, int width, int height) const noexcept {
        return RECT{scaled(x), scaled(y), scaled(x + width), scaled(y + height)};
    }

    // Converts a design-coordinate rectangle to client device pixels.

    RECT scale_rect(const RECT& design) const noexcept {
        return RECT{
            scaled(design.left), scaled(design.top), scaled(design.right), scaled(design.bottom)};
    }

    // Sidebar titles come from the locale table (the sidebar is the only

    // label rendered from a constant table elsewhere).

    static const wchar_t* sidebar_title(int index) {
        switch (index) {
        case 0:
            return ui::text("settings.sidebar.general", L"General");
        case 1:
            return ui::text("settings.sidebar.interface", L"Interface");
        case 2:
            return ui::text("settings.sidebar.topbar", L"Topbar");
        default:
            return ui::text("settings.sidebar.input", L"Input");
        }
    }

    // The visible name of a dropdown under UIA is the section heading it

    // belongs to (dropdowns have no separate label row).

    std::wstring dropdown_name(int control_index) const {
        const std::vector<UiControl>& controls = pages_[static_cast<int>(page_)];
        for (int scan = control_index; scan >= 0; --scan) {
            if (controls[static_cast<std::size_t>(scan)].kind == CtrlKind::section_label) {
                return controls[static_cast<std::size_t>(scan)].label;
            }
        }
        const UiControl& control = controls[static_cast<std::size_t>(control_index)];
        if (control.dropdown_index >= 0
            && control.dropdown_index < static_cast<int>(control.dropdown_items.size())) {
            return control.dropdown_items[static_cast<std::size_t>(control.dropdown_index)];
        }
        return L"";
    }

    // Converts client pixels to 96-DPI design coordinates.

    POINT to_design(int x, int y) const noexcept {
        return POINT{x * 96 / static_cast<int>(ui_dpi_), y * 96 / static_cast<int>(ui_dpi_)};
    }

    static UiControl make_section(int y,
                                  const wchar_t* text,
                                  const wchar_t* description = nullptr) {
        UiControl control;
        control.kind = CtrlKind::section_label;
        control.rect = RECT{content_left, y, content_left + 470, y + 24};
        control.label = text;
        if (description != nullptr) {
            control.description = description;
        }
        return control;
    }

    // Footer button rectangles, shared by painting, hit-testing and the

    // press/release logic so the geometry is defined exactly once.

    RECT save_button_rect() const noexcept {
        return design_rect(498, 560, 110, 34);
    }
    RECT cancel_button_rect() const noexcept {
        return design_rect(616, 560, 100, 34);
    }

    // Draws the keyboard focus ring just outside a control's rectangle.

    void draw_focus_ring(HDC device_context, RECT rect) const {
        ::InflateRect(&rect, scaled(ui::spacing::sm), scaled(ui::spacing::sm));
        HPEN pen = ::CreatePen(PS_SOLID, std::max(1, scaled(2)), ui::theme_focus());
        if (pen == nullptr) {
            return;
        }
        const HGDIOBJ previous = ::SelectObject(device_context, pen);
        const HGDIOBJ previous_brush = ::SelectObject(device_context, ::GetStockObject(NULL_BRUSH));
        ::RoundRect(
            device_context, rect.left, rect.top, rect.right, rect.bottom, scaled(9), scaled(9));
        ::SelectObject(device_context, previous_brush);
        ::SelectObject(device_context, previous);
        ::DeleteObject(pen);
    }

    static UiControl make_checkbox(
        int action, int y, int width, const wchar_t* text, bool checked) {
        UiControl control;
        control.kind = CtrlKind::checkbox;
        control.action = action;
        control.rect =
            RECT{content_left, y, content_left + width, y + ui::metrics::checkbox_height};
        control.label = text;
        control.checked = checked;
        return control;
    }

    static UiControl make_button(int action, int y, int width, const wchar_t* text) {
        UiControl control;
        control.kind = CtrlKind::button;
        control.action = action;
        control.rect = RECT{content_left, y, content_left + width, y + ui::metrics::button_height};
        control.label = text;
        return control;
    }

    static UiControl make_dropdown(int action, int y, int width) {
        UiControl control;
        control.kind = CtrlKind::dropdown;
        control.action = action;
        control.rect =
            RECT{content_left, y, content_left + width, y + ui::metrics::dropdown_height};
        return control;
    }

    std::vector<UiControl>& page_controls() noexcept {
        return pages_[static_cast<int>(page_)];
    }

    void build_pages() {
        for (auto& page : pages_) {
            page.clear();
        }

        auto& general = pages_[static_cast<int>(PageId::general)];
        general.push_back(make_section(
            88,
            ui::text("settings.section.behavior", L"Behavior"),
            ui::text("settings.hint.behavior", L"How Suborno starts and remembers your choices.")));
        // Distinct rows: the two checkboxes used to share the same Y and

        // rendered on top of each other.

        general.push_back(make_checkbox(
            action_remember_layout,
            118,
            320,
            ui::text("settings.general.remember_layout", L"Remember Last Input Layout"),
            staged_.general.remember_last_input_layout));
        general.push_back(
            make_checkbox(action_start_with_windows,
                          148,
                          320,
                          ui::text("settings.general.start_with_windows", L"Start with Windows"),
                          staged_.general.start_with_windows));
        general.push_back(
            make_section(186,
                         ui::text("settings.section.start_mode", L"When Windows starts, show"),
                         ui::text("settings.hint.start_mode",
                                  L"Which Suborno surface appears after you sign in.")));
        {
            UiControl dropdown = make_dropdown(action_start_mode, 214, 300);
            dropdown.dropdown_items = {
                ui::text("settings.start_mode.remember_last", L"Remember last state"),
                ui::text("settings.start_mode.topbar", L"Topbar"),
                ui::text("settings.start_mode.tray", L"System tray"),
                ui::text("settings.start_mode.window", L"Settings window")};
            dropdown.dropdown_index = static_cast<int>(staged_.general.start_mode);
            general.push_back(std::move(dropdown));
        }
        general.push_back(make_section(
            262,
            ui::text("settings.section.switching_mode", L"Layout switching mode"),
            ui::text(
                "settings.hint.switching_mode",
                L"Whether a switch applies everywhere or only in the window you switched in.")));
        {
            UiControl dropdown = make_dropdown(action_switching_mode, 290, 300);
            dropdown.dropdown_items = {
                ui::text("settings.switching_mode.per_window", L"Per application window"),
                ui::text("settings.switching_mode.global", L"Global (same everywhere)")};
            dropdown.dropdown_index =
                staged_.general.switching_mode == suborno::settings::SwitchingMode::global ? 1 : 0;
            general.push_back(std::move(dropdown));
        }
        general.push_back(make_section(
            338,
            ui::text("settings.section.default_layout", L"Default keyboard layout"),
            ui::text("settings.hint.default_layout", L"The layout Suborno uses when it starts.")));
        {
            UiControl dropdown = make_dropdown(action_layout_dropdown, 366, 300);
            for (const LayoutOption& layout : layouts) {
                dropdown.dropdown_items.emplace_back(layout.name);
            }
            dropdown.dropdown_index = staged_layout_index();
            general.push_back(std::move(dropdown));
        }

        auto& interface_page = pages_[static_cast<int>(PageId::interface)];
        interface_page.push_back(make_section(
            88,
            ui::text("settings.section.candidate_window", L"Candidate Window"),
            ui::text("settings.hint.candidate_window", L"How suggestions appear while you type.")));
        interface_page.push_back(make_checkbox(
            action_show_candidate_window,
            116,
            320,
            ui::text("settings.interface.show_candidate_window", L"Show Candidate Window"),
            staged_.ui.show_candidate_window));
        interface_page.push_back(make_checkbox(
            action_next_word_prediction,
            144,
            320,
            ui::text("settings.interface.next_word_prediction", L"Next Word Prediction"),
            staged_.ui.next_word_prediction_enabled));
        interface_page.push_back(make_section(
            172,
            ui::text("settings.section.orientation", L"Orientation"),
            ui::text("settings.hint.orientation", L"Vertical list or one horizontal row.")));
        {
            UiControl dropdown = make_dropdown(action_orientation, 198, 300);
            dropdown.dropdown_items = {
                ui::text("settings.candidate_orientation.vertical", L"Vertical list"),
                ui::text("settings.candidate_orientation.horizontal", L"Horizontal row")};
            dropdown.dropdown_index = staged_.ui.candidate_orientation
                                              == suborno::settings::CandidateOrientation::horizontal
                                          ? 1
                                          : 0;
            interface_page.push_back(std::move(dropdown));
        }
        interface_page.push_back(make_checkbox(action_fixed_height,
                                               246,
                                               320,
                                               ui::text("settings.interface.candidate_fixed_height",
                                                        L"Fixed Height (page five at a time)"),
                                               staged_.ui.candidate_fixed_height));
        interface_page.push_back(
            make_checkbox(action_sliding_effect,
                          274,
                          320,
                          ui::text("settings.interface.enable_sliding_effect", L"Sliding Effect"),
                          staged_.ui.enable_sliding_effect));
        interface_page.push_back(make_section(
            302,
            ui::text("settings.section.positioning", L"Positioning"),
            ui::text("settings.hint.positioning", L"Where the candidate window appears.")));
        interface_page.push_back(
            make_checkbox(action_follow_caret,
                          328,
                          380,
                          ui::text("settings.interface.follow_caret", L"Follow the Caret"),
                          staged_.ui.follow_caret));
        interface_page.push_back(make_checkbox(
            action_follow_mouse,
            356,
            420,
            ui::text("settings.interface.follow_mouse_pointer_if_caret_tracking_fails",
                     L"Follow Mouse Pointer if Caret Tracking Fails"),
            staged_.ui.follow_mouse_pointer_if_caret_tracking_fails));
        interface_page.push_back(
            make_section(384,
                         ui::text("settings.section.appearance", L"Appearance"),
                         ui::text("settings.hint.appearance",
                                  L"Theme, language, and the font used across the app.")));
        {
            UiControl dropdown = make_dropdown(action_theme_dropdown, 410, 300);
            dropdown.dropdown_items = {
                ui::text("settings.ui_theme.suborno_dark", L"Suborno Dark"),
                ui::text("settings.ui_theme.suborno_light", L"Suborno Light"),
                ui::text("settings.ui_theme.system", L"System (Windows)")};
            dropdown.dropdown_index = staged_.ui.theme_id == "suborno-light" ? 1
                                      : staged_.ui.theme_id == "system"      ? 2
                                                                             : 0;
            interface_page.push_back(std::move(dropdown));
        }
        {
            UiControl dropdown = make_dropdown(action_ui_language, 448, 300);
            dropdown.dropdown_items = {
                ui::text("settings.interface.language.system", L"Follow system language"),
                ui::text("settings.interface.language.english", L"English"),
                ui::text("settings.interface.language.bangla", L"বাংলা")};
            dropdown.dropdown_index = staged_.ui.language == "en"   ? 1
                                      : staged_.ui.language == "bn" ? 2
                                                                    : 0;
            interface_page.push_back(std::move(dropdown));
        }
        interface_page.push_back(make_checkbox(
            action_use_os_default_font,
            488,
            320,
            ui::text("settings.interface.use_os_default_font", L"Use OS Default Bangla Font"),
            staged_.ui.use_os_default_bangla_font));
        {
            UiControl dropdown = make_dropdown(action_font_dropdown, 514, 300);
            dropdown.dropdown_items.emplace_back(
                ui::text("settings.interface.font_os_default_item", L"OS Default (Recommended)"));
            for (const std::wstring& family : bangla_font_options()) {
                dropdown.dropdown_items.push_back(family);
            }
            dropdown.dropdown_index = staged_font_index(dropdown.dropdown_items);
            interface_page.push_back(std::move(dropdown));
        }

        auto& topbar_page = pages_[static_cast<int>(PageId::topbar)];
        topbar_page.push_back(
            make_section(88,
                         ui::text("settings.section.topbar_visibility", L"Topbar Visibility"),
                         ui::text("settings.hint.topbar_visibility",
                                  L"How the floating bar behaves when idle.")));
        topbar_page.push_back(
            make_checkbox(action_idle_dimming,
                          118,
                          300,
                          ui::text("settings.topbar.idle_dimming_enabled", L"Enable Idle Dimming"),
                          staged_.topbar.idle_dimming_enabled));
        {
            UiControl slider;
            slider.kind = CtrlKind::slider;
            slider.action = action_idle_opacity;
            slider.rect = RECT{content_left, 150, content_left + 470, 174};
            slider.label = ui::text("settings.topbar.idle_opacity_percent", L"Idle Opacity (%)");
            slider.value = static_cast<int>(staged_.topbar.idle_opacity_percent);
            slider.value_min = 0;
            slider.value_max = 100;
            slider.value = std::clamp(slider.value, slider.value_min, slider.value_max);
            topbar_page.push_back(std::move(slider));
        }
        {
            UiControl slider;
            slider.kind = CtrlKind::slider;
            slider.action = action_idle_time;
            slider.rect = RECT{content_left, 198, content_left + 470, 222};
            slider.label = ui::text("settings.topbar.idle_time_seconds", L"Idle Time (Seconds)");
            slider.value = static_cast<int>(staged_.topbar.idle_time_seconds);
            slider.value_min = 1;
            slider.value_max = 60;
            slider.value = std::clamp(slider.value, slider.value_min, slider.value_max);
            topbar_page.push_back(std::move(slider));
        }
        topbar_page.push_back(
            make_section(246,
                         ui::text("settings.section.position", L"Position"),
                         ui::text("settings.hint.position",
                                  L"Whether the floating bar keeps its dragged position.")));
        topbar_page.push_back(make_checkbox(
            action_remember_topbar_position,
            274,
            320,
            ui::text("settings.topbar.remember_position", L"Remember Topbar Position"),
            staged_.topbar.remember_position));

        auto& input = pages_[static_cast<int>(PageId::input)];
        input.push_back(
            make_section(88,
                         ui::text("settings.section.input_preferences", L"Input preferences"),
                         ui::text("settings.hint.input_preferences",
                                  L"Keys and number handling in Bangla layouts.")));
        input.push_back(make_checkbox(action_map_numpad,
                                      116,
                                      340,
                                      ui::text("settings.input.map_numpad_to_bangla_numbers",
                                               L"Map Numpad to Bangla Numbers"),
                                      staged_.input.map_numpad_to_bangla_numbers));
        input.push_back(make_checkbox(action_ignore_caps_lock,
                                      144,
                                      380,
                                      ui::text("settings.input.ignore_caps_lock_state_in_bangla",
                                               L"Ignore Caps Lock state in Bangla layouts"),
                                      staged_.input.ignore_caps_lock_state_in_bangla));
        input.push_back(make_section(
            172,
            ui::text("settings.section.suggestions", L"Suggestions & Learning"),
            ui::text("settings.hint.suggestions", L"Whether new words you type are remembered.")));
        {
            UiControl dropdown = make_dropdown(action_learning, 198, 300);
            dropdown.dropdown_items = {
                ui::text("settings.learning.on", L"Learn new words (recommended)"),
                ui::text("settings.learning.off", L"Off (suggestions from the dictionary only)")};
            dropdown.dropdown_index = staged_.learning_enabled ? 0 : 1;
            input.push_back(std::move(dropdown));
        }
        input.push_back(
            make_button(action_clear_learnt,
                        240,
                        240,
                        ui::text("settings.input.clear_learnt", L"Clear Learnt Words…")));
        input.push_back(
            make_section(280,
                         ui::text("settings.section.candidate_selection", L"Candidate Selection"),
                         ui::text("settings.hint.candidate_selection",
                                  L"How you pick a suggestion from the candidate window.")));
        input.push_back(make_checkbox(action_number_keys,
                                      306,
                                      340,
                                      ui::text("settings.input.select_candidate_using_number_keys",
                                               L"Select candidate using number keys"),
                                      staged_.input.select_candidate_using_number_keys));
        input.push_back(make_section(
            344,
            ui::text("settings.section.shortcut", L"Layout Switching Shortcut"),
            ui::text("settings.hint.shortcut", L"A global key that cycles layouts anywhere.")));
        {
            UiControl dropdown = make_dropdown(action_layout_shortcut, 370, 300);
            for (const std::string_view shortcut : registerable_shortcuts) {
                dropdown.dropdown_items.emplace_back(shortcut.begin(), shortcut.end());
            }
            // A settings file may still carry a Windows-reserved shortcut

            // (Alt+Shift / Win+Space): keep it visible so the control shows

            // the value that is actually saved.

            const std::wstring staged_shortcut = utf8_to_wide(staged_.input.layout_switch_shortcut);
            int index = -1;
            for (std::size_t item = 0; item < dropdown.dropdown_items.size(); ++item) {
                if (dropdown.dropdown_items[item] == staged_shortcut) {
                    index = static_cast<int>(item);
                    break;
                }
            }
            if (index < 0) {
                dropdown.dropdown_items.push_back(staged_shortcut);
                index = static_cast<int>(dropdown.dropdown_items.size()) - 1;
            }
            dropdown.dropdown_index = index;
            input.push_back(std::move(dropdown));
        }
        input.push_back(
            make_section(408,
                         ui::text("settings.section.encoding", L"Encoding & Locale"),
                         ui::text("settings.hint.encoding",
                                  L"Output encoding, and the Windows input language.")));
        {
            UiControl dropdown = make_dropdown(action_encoding, 434, 300);
            dropdown.dropdown_items = {
                ui::text("settings.preferred_encoding.unicode", L"Unicode (recommended)"),
                ui::text("settings.preferred_encoding.ansi", L"Bijoy ANSI (legacy)")};
            dropdown.dropdown_index =
                staged_.input.preferred_encoding == suborno::settings::TextEncoding::ansi ? 1 : 0;
            input.push_back(std::move(dropdown));
        }
        input.push_back(
            make_checkbox(action_change_locale,
                          480,
                          420,
                          ui::text("settings.input.change_locale_to_bn_bd_when_layout_not_english",
                                   L"Switch input language to bn-BD for Bangla layouts"),
                          staged_.input.change_locale_to_bn_bd_when_layout_not_english));
        input.push_back(
            make_checkbox(action_disable_encoding_shortcut,
                          508,
                          420,
                          ui::text("settings.input.disable_encoding_shortcut",
                                   L"Disable the encoding toggle shortcut (Ctrl+Alt+E)"),
                          staged_.input.disable_encoding_shortcut));
        input.push_back(make_section(
            536,
            ui::text("settings.section.borno_compatibility", L"BornoTSF Compatibility"),
            ui::text("settings.hint.borno_compatibility",
                     L"Typing behaviour shared with the BornoTSF keyboard.")));
        input.push_back(make_checkbox(
            action_phonetic_correction,
            562,
            420,
            ui::text("settings.input.phonetic_correction", L"Phonetic Auto Correction"),
            staged_.phonetic_correction));
        input.push_back(make_checkbox(
            action_typing_style,
            590,
            420,
            ui::text("settings.input.traditional_typing_style", L"Traditional Typing Style"),
            staged_.typing_style == "traditional"));
    }

    int staged_font_index(const std::vector<std::wstring>& items) const noexcept {
        if (staged_.ui.use_os_default_bangla_font || staged_.ui.font_family.empty()) {
            return 0;
        }
        const std::wstring staged = utf8_to_wide(staged_.ui.font_family);
        for (std::size_t index = 1; index < items.size(); ++index) {
            if (::_wcsicmp(items[index].c_str(), staged.c_str()) == 0) {
                return static_cast<int>(index);
            }
        }
        return 0;
    }

    int staged_layout_index() const noexcept {
        for (std::size_t index = 0; index < layouts.size(); ++index) {
            if (staged_.active_layout_id == layouts[index].id) {
                return static_cast<int>(index);
            }
        }
        return 0;
    }

    void toggle_staged_checkbox(UiControl& control) {
        control.checked = !control.checked;
        switch (control.action) {
        case action_remember_layout:
            staged_.general.remember_last_input_layout = control.checked;
            break;
        case action_start_with_windows:
            staged_.general.start_with_windows = control.checked;
            break;
        case action_show_candidate_window:
            staged_.ui.show_candidate_window = control.checked;
            break;
        case action_use_os_default_font:
            staged_.ui.use_os_default_bangla_font = control.checked;
            if (control.checked) {
                // The OS-default choice and a specific family are mutually

                // exclusive; clearing the family keeps the effective font in

                // sync with what the dropdown will show next.

                staged_.ui.font_family.clear();
            }
            break;
        case action_idle_dimming:
            staged_.topbar.idle_dimming_enabled = control.checked;
            break;
        case action_remember_topbar_position:
            staged_.topbar.remember_position = control.checked;
            break;
        case action_map_numpad:
            staged_.input.map_numpad_to_bangla_numbers = control.checked;
            break;
        case action_ignore_caps_lock:
            staged_.input.ignore_caps_lock_state_in_bangla = control.checked;
            break;
        case action_number_keys:
            staged_.input.select_candidate_using_number_keys = control.checked;
            break;
        case action_next_word_prediction:
            staged_.ui.next_word_prediction_enabled = control.checked;
            break;
        case action_fixed_height:
            staged_.ui.candidate_fixed_height = control.checked;
            break;
        case action_sliding_effect:
            staged_.ui.enable_sliding_effect = control.checked;
            break;
        case action_follow_caret:
            staged_.ui.follow_caret = control.checked;
            break;
        case action_follow_mouse:
            staged_.ui.follow_mouse_pointer_if_caret_tracking_fails = control.checked;
            break;
        case action_change_locale:
            staged_.input.change_locale_to_bn_bd_when_layout_not_english = control.checked;
            break;
        case action_disable_encoding_shortcut:
            staged_.input.disable_encoding_shortcut = control.checked;
            break;
        case action_phonetic_correction:
            staged_.phonetic_correction = control.checked;
            break;
        case action_typing_style:
            staged_.typing_style = control.checked ? "traditional" : "normal";
            break;
        default:
            break;
        }
        ::InvalidateRect(window_, nullptr, FALSE);
    }

    void apply_staged_slider(UiControl& control, int value) {
        control.value = std::max(control.value_min, std::min(control.value_max, value));
        switch (control.action) {
        case action_idle_opacity:
            staged_.topbar.idle_opacity_percent = static_cast<std::uint32_t>(control.value);
            break;
        case action_idle_time:
            staged_.topbar.idle_time_seconds = static_cast<std::uint32_t>(control.value);
            break;
        default:
            break;
        }
        ::InvalidateRect(window_, nullptr, FALSE);
    }

    void open_dropdown(UiControl& control) {
        RECT window_rect{};
        ::GetWindowRect(window_, &window_rect);
        // Anchor below the field: design rect -> client pixels -> screen.

        POINT anchor{window_rect.left + scaled(control.rect.left),
                     window_rect.top + scaled(control.rect.bottom) + scaled(4)};
        std::vector<MenuItem> items;
        items.reserve(control.dropdown_items.size());
        for (const std::wstring& label : control.dropdown_items) {
            items.push_back(MenuItem{label});
        }
        const int chosen = show_menu_popup(window_, items, anchor, control.dropdown_index);
        if (chosen >= 0 && chosen < static_cast<int>(control.dropdown_items.size())) {
            control.dropdown_index = chosen;
            bool rebuild = false;
            switch (control.action) {
            case action_layout_dropdown:
                if (chosen < static_cast<int>(layouts.size())) {
                    staged_.active_layout_id = layouts[static_cast<std::size_t>(chosen)].id;
                }
                break;
            case action_start_mode:
                if (chosen <= static_cast<int>(suborno::settings::StartMode::main_window)) {
                    staged_.general.start_mode = static_cast<suborno::settings::StartMode>(chosen);
                }
                break;
            case action_layout_shortcut:
                staged_.input.layout_switch_shortcut =
                    wide_to_utf8(control.dropdown_items[static_cast<std::size_t>(chosen)]);
                rebuild = true;
                break;
            case action_font_dropdown:
                apply_font_choice(chosen);
                rebuild = true;
                break;
            case action_theme_dropdown:
                staged_.ui.theme_id = chosen == 1   ? "suborno-light"
                                      : chosen == 2 ? "system"
                                                    : "suborno-dark";
                break;
            case action_ui_language:
                staged_.ui.language = chosen == 1 ? "en" : chosen == 2 ? "bn" : "system";
                break;
            case action_orientation:
                staged_.ui.candidate_orientation =
                    chosen == 1 ? suborno::settings::CandidateOrientation::horizontal
                                : suborno::settings::CandidateOrientation::vertical;
                break;
            case action_switching_mode:
                staged_.general.switching_mode =
                    chosen == 1 ? suborno::settings::SwitchingMode::global
                                : suborno::settings::SwitchingMode::per_application_window;
                break;
            case action_encoding:
                staged_.input.preferred_encoding = chosen == 1
                                                       ? suborno::settings::TextEncoding::ansi
                                                       : suborno::settings::TextEncoding::unicode;
                break;
            case action_learning:
                staged_.learning_enabled = chosen == 0;
                break;
            default:
                break;
            }
            if (rebuild) {
                // Keep the paired controls (font checkbox, shortcut display)

                // consistent with the staged settings.

                focus_index_ = -1;
                build_pages();
            }
            ::InvalidateRect(window_, nullptr, FALSE);
        }
    }

    // Maps a font dropdown selection onto the staged settings: index 0 is the

    // OS default, any other index selects the family from the cached list.

    void apply_font_choice(int chosen) {
        if (chosen == 0) {
            staged_.ui.use_os_default_bangla_font = true;
            staged_.ui.font_family.clear();
            return;
        }
        const std::vector<std::wstring>& fonts = bangla_font_options();
        const std::size_t font_index = static_cast<std::size_t>(chosen) - 1;
        if (font_index >= fonts.size()) {
            return;
        }
        staged_.ui.use_os_default_bangla_font = false;
        staged_.ui.font_family = wide_to_utf8(fonts[font_index]);
    }

    // Hit tests in design coordinates; -1 when nothing hit.

    int sidebar_hit(POINT design_point) const noexcept {
        const int column = design_point.x - sidebar_left;
        if (column < 0 || column >= sidebar_width) {
            return -1;
        }
        const int row = (design_point.y - sidebar_top) / sidebar_stride;
        const int offset = (design_point.y - sidebar_top) % sidebar_stride;
        if (row < 0 || row >= page_count || offset >= sidebar_pill_height) {
            return -1;
        }
        return row;
    }

    int control_hit(POINT design_point) const noexcept {
        std::vector<UiControl>& controls = const_cast<Application*>(this)->page_controls();
        for (std::size_t index = 0; index < controls.size(); ++index) {
            const RECT& rect = controls[index].rect;
            if (design_point.x >= rect.left && design_point.x < rect.right
                && design_point.y >= rect.top && design_point.y < rect.bottom) {
                return static_cast<int>(index);
            }
        }
        return -1;
    }

    bool close_hit(POINT design_point) const noexcept {
        return design_point.x >= 696 && design_point.x <= 728 && design_point.y >= 4
               && design_point.y <= 36;
    }

    bool save_button_hit(POINT design_point) const noexcept {
        const RECT rect = save_button_rect();
        return design_point.x >= rect.left && design_point.y >= rect.top
               && design_point.x < rect.right && design_point.y < rect.bottom;
    }

    bool cancel_button_hit(POINT design_point) const noexcept {
        const RECT rect = cancel_button_rect();
        return design_point.x >= rect.left && design_point.y >= rect.top
               && design_point.x < rect.right && design_point.y < rect.bottom;
    }

    void handle_settings_mouse_move(int x, int y) {
        const POINT design_point = to_design(x, y);
        bool changed = false;
        std::vector<UiControl>& controls = page_controls();
        for (std::size_t index = 0; index < controls.size(); ++index) {
            UiControl& control = controls[index];
            const bool inside =
                control.kind != CtrlKind::section_label && design_point.x >= control.rect.left
                && design_point.x < control.rect.right && design_point.y >= control.rect.top
                && design_point.y < control.rect.bottom;
            if (control.hover != inside) {
                control.hover = inside;
                changed = true;
            }
        }
        const bool close_now = close_hit(design_point);
        const bool save_now = save_button_hit(design_point);
        const bool cancel_now = cancel_button_hit(design_point);
        if (close_hover_ != close_now || save_hover_ != save_now || cancel_hover_ != cancel_now) {
            close_hover_ = close_now;
            save_hover_ = save_now;
            cancel_hover_ = cancel_now;
            changed = true;
        }
        int sidebar = sidebar_hit(design_point);
        if (sidebar != static_cast<int>(page_)) {
            sidebar = -1;
        }
        if (sidebar_hover_ != sidebar) {
            sidebar_hover_ = sidebar;
            changed = true;
        }
        // Dragging off a pressed footer button cancels the press (the action

        // fires only when the release lands inside the button).

        if (save_pressed_ && !save_button_hit(design_point)) {
            save_pressed_ = false;
            changed = true;
        }
        if (cancel_pressed_ && !cancel_button_hit(design_point)) {
            cancel_pressed_ = false;
            changed = true;
        }
        if (slider_drag_ >= 0 && slider_drag_ < static_cast<int>(controls.size())) {
            UiControl& slider = controls[static_cast<std::size_t>(slider_drag_)];
            const int track_left = scaled(content_left + 164);
            const int track_right = scaled(content_left + 444);
            if (track_right > track_left) {
                // x is a client coordinate of the whole window; clamp IT to

                // the track. (The previous expression clamped track_left + x,

                // double-counting the track origin, so every press on the

                // visible track saturated the value at the maximum.)

                const int clamped = std::max(track_left, std::min(x, track_right));
                const int value = slider.value_min
                                  + (clamped - track_left) * (slider.value_max - slider.value_min)
                                        / (track_right - track_left);
                apply_staged_slider(slider, value);
            }
        }
        if (changed || slider_drag_ >= 0) {
            ::InvalidateRect(window_, nullptr, FALSE);
        }
    }

    void handle_settings_mouse_leave() {
        bool changed = false;
        for (auto& page : pages_) {
            for (UiControl& control : page) {
                if (control.hover) {
                    control.hover = false;
                    changed = true;
                }
            }
        }
        if (close_hover_ || save_hover_ || cancel_hover_ || sidebar_hover_ >= 0) {
            close_hover_ = false;
            save_hover_ = false;
            cancel_hover_ = false;
            sidebar_hover_ = -1;
            changed = true;
        }
        if (changed) {
            ::InvalidateRect(window_, nullptr, FALSE);
        }
    }

    void handle_settings_mouse_down(int x, int y) {
        const POINT design_point = to_design(x, y);
        if (close_hit(design_point)) {
            cancel_settings();
            return;
        }
        if (design_point.y < title_height) {
            // Custom title row: hand the press to the native move loop.

            ::ReleaseCapture();
            ::PostMessageW(window_, WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(x, y));
            return;
        }
        if (save_button_hit(design_point)) {
            // Press-and-release button semantics: the action fires on mouse-up

            // inside the button, matching native Windows behavior.

            save_pressed_ = true;
            focus_footer_ = 1;
            focus_index_ = -1;
            ::SetCapture(window_);
            ::InvalidateRect(window_, nullptr, FALSE);
            return;
        }
        if (cancel_button_hit(design_point)) {
            cancel_pressed_ = true;
            focus_footer_ = 2;
            focus_index_ = -1;
            ::SetCapture(window_);
            ::InvalidateRect(window_, nullptr, FALSE);
            return;
        }
        const int sidebar = sidebar_hit(design_point);
        if (sidebar >= 0) {
            page_ = static_cast<PageId>(sidebar);
            focus_index_ = -1;
            sidebar_hover_ = -1;
            ::InvalidateRect(window_, nullptr, TRUE);
            return;
        }
        std::vector<UiControl>& controls = page_controls();
        const int hit = control_hit(design_point);
        if (hit < 0) {
            return;
        }
        UiControl& control = controls[static_cast<std::size_t>(hit)];
        focus_index_ = hit;
        switch (control.kind) {
        case CtrlKind::checkbox:
            toggle_staged_checkbox(control);
            break;
        case CtrlKind::dropdown:
            open_dropdown(control);
            break;
        case CtrlKind::slider:
            slider_drag_ = hit;
            ::SetCapture(window_);
            handle_settings_mouse_move(x, y);
            break;
        case CtrlKind::button:
            // Buttons act immediately — they are not part of the staged

            // Save/Cancel flow.

            run_button_action(control.action);
            break;
        default:
            break;
        }
        ::InvalidateRect(window_, nullptr, FALSE);
        ui_access::notify_focus_changed(window_, this, a11y_alive_);
    }

    // Immediate (non-staged) button actions in the settings pages.

    void run_button_action(int action) {
        if (action == action_clear_learnt) {
            clear_learnt_words_from_ui();
        }
    }

    // Destructive, immediate: wipes user words, user frequencies and the

    // transition history after an explicit confirmation. Not part of the

    // staged settings snapshot, so Save/Cancel never re-runs or undoes it.

    void clear_learnt_words_from_ui() {
        const int choice =
            ::MessageBoxW(window_,
                          ui::text("confirm.clear_learnt.text",
                                   L"Remove every word Suborno has learned from "
                                   L"your typing (user words, frequencies and "
                                   L"word-pair history)? This cannot be undone."),
                          ui::text("confirm.clear_learnt.title", L"Clear Learnt Words"),
                          MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
        if (choice != IDYES) {
            return;
        }
        if (!engine_->clear_learnt_words()) {
            show_error(
                window_,
                ui::text("error.clear_learnt_failed", L"Suborno could not clear the learnt words."),
                E_FAIL);
            return;
        }
        refresh_status();
        show_toast("toast.learnt_cleared", L"Learnt words cleared", ToastKind::success);
    }

    void handle_settings_mouse_up(int x, int y) {
        const POINT design_point = to_design(x, y);
        if (save_pressed_) {
            save_pressed_ = false;
            if (::GetCapture() == window_) {
                ::ReleaseCapture();
            }
            if (save_button_hit(design_point)) {
                save_settings_from_ui();
                return;
            }
            ::InvalidateRect(window_, nullptr, FALSE);
        } else if (cancel_pressed_) {
            cancel_pressed_ = false;
            if (::GetCapture() == window_) {
                ::ReleaseCapture();
            }
            if (cancel_button_hit(design_point)) {
                cancel_settings();
                return;
            }
            ::InvalidateRect(window_, nullptr, FALSE);
        }
        if (slider_drag_ >= 0) {
            slider_drag_ = -1;
            if (::GetCapture() == window_) {
                ::ReleaseCapture();
            }
            ::InvalidateRect(window_, nullptr, FALSE);
        }
    }

    void handle_settings_key_down(int virtual_key) {
        std::vector<UiControl>& controls = page_controls();
        const int control_count = static_cast<int>(controls.size());
        // Ctrl+Tab / Ctrl+Shift+Tab cycle pages (standard dialog navigation).

        if (virtual_key == VK_TAB && (::GetKeyState(VK_CONTROL) & 0x8000) != 0) {
            const bool backwards = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
            int page = static_cast<int>(page_) + (backwards ? -1 : 1);
            if (page < 0) {
                page = page_count - 1;
            }
            if (page >= page_count) {
                page = 0;
            }
            page_ = static_cast<PageId>(page);
            focus_index_ = -1;
            focus_footer_ = 0;
            ::InvalidateRect(window_, nullptr, TRUE);
            ui_access::notify_focus_changed(window_, this, a11y_alive_);
            return;
        }
        if (virtual_key == VK_TAB) {
            const bool backwards = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
            // Focus order: page controls, then Save, then Cancel. Position -1

            // means no control focused; control_count is Save and control

            // count + 1 is Cancel.

            const int total = control_count + 2;
            const auto focusable = [this, &controls, control_count](int position) {
                if (position == control_count || position == control_count + 1) {
                    return true;
                }
                if (position < 0 || position >= control_count) {
                    return false;
                }
                const CtrlKind kind = controls[static_cast<std::size_t>(position)].kind;
                return kind != CtrlKind::section_label && kind != CtrlKind::status;
            };
            int current = focus_footer_ == 1   ? control_count
                          : focus_footer_ == 2 ? control_count + 1
                                               : focus_index_;
            int next = current;
            for (int step = 0; step <= total; ++step) {
                next = backwards ? next - 1 : next + 1;
                if (next < 0) {
                    next = total - 1;
                }
                if (next >= total) {
                    next = 0;
                }
                if (focusable(next)) {
                    break;
                }
            }
            focus_footer_ = next == control_count ? 1 : next == control_count + 1 ? 2 : 0;
            focus_index_ = focus_footer_ == 0 ? next : -1;
            ::InvalidateRect(window_, nullptr, FALSE);
            ui_access::notify_focus_changed(window_, this, a11y_alive_);
            return;
        }
        if (focus_index_ >= 0 && focus_index_ < static_cast<int>(controls.size())) {
            UiControl& control = controls[static_cast<std::size_t>(focus_index_)];
            if (virtual_key == VK_SPACE) {
                if (control.kind == CtrlKind::checkbox) {
                    toggle_staged_checkbox(control);
                    return;
                }
                if (control.kind == CtrlKind::dropdown) {
                    open_dropdown(control);
                    return;
                }
                if (control.kind == CtrlKind::button) {
                    run_button_action(control.action);
                    return;
                }
            }
            if (control.kind == CtrlKind::slider) {
                if (virtual_key == VK_LEFT) {
                    apply_staged_slider(control, control.value - 1);
                    return;
                }
                if (virtual_key == VK_RIGHT) {
                    apply_staged_slider(control, control.value + 1);
                    return;
                }
                if (virtual_key == VK_PRIOR) {
                    apply_staged_slider(control, control.value - 10);
                    return;
                }
                if (virtual_key == VK_NEXT) {
                    apply_staged_slider(control, control.value + 10);
                    return;
                }
                if (virtual_key == VK_HOME) {
                    apply_staged_slider(control, control.value_min);
                    return;
                }
                if (virtual_key == VK_END) {
                    apply_staged_slider(control, control.value_max);
                    return;
                }
            }
        }
        if (virtual_key == VK_RETURN) {
            // Enter activates the focused footer button; otherwise it saves.

            if (focus_footer_ == 2) {
                cancel_settings();
            } else {
                save_settings_from_ui();
            }
            return;
        }
        if (virtual_key == VK_ESCAPE) {
            cancel_settings();
            return;
        }
    }

    void cancel_settings() {
        staged_ = engine_->settings();
        build_pages();
        focus_index_ = -1;
        focus_footer_ = 0;
        save_pressed_ = false;
        cancel_pressed_ = false;
        slider_drag_ = -1;
        if (tray_added_ && !smoke_test_) {
            ::ShowWindow(window_, SW_HIDE);
        }
        ::InvalidateRect(window_, nullptr, TRUE);
    }

    void save_settings_from_ui() {
        const std::string previous_language = engine_->settings().ui.language;
        auto settings = staged_;
        const bool startup_now = settings.general.start_with_windows;
        HRESULT result = write_startup_preference(startup_now);
        if (SUCCEEDED(result)) {
            const auto save_status = engine_->save_settings(settings);
            if (save_status != suborno::settings::SettingsStatus::ok) {
                const HRESULT rollback_result = write_startup_preference(startup_enabled_);
                if (FAILED(rollback_result)) {
                    result = rollback_result;
                } else {
                    result = HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
                }
                show_error(
                    window_,
                    ui::text("error.settings_save_failed", L"Suborno could not save the settings."),
                    result);
                return;
            }
        } else {
            show_error(window_,
                       ui::text("error.startup_pref_failed",
                                L"Suborno could not update the sign-in launch preference."),
                       result);
            return;
        }
        startup_enabled_ = startup_now;
        const HRESULT refresh_result = refresh_active_profile();
        if (FAILED(refresh_result)) {
            show_error(
                window_,
                ui::text("error.profile_refresh_after_save",
                         L"Settings saved, but Windows could not refresh the active profile."),
                refresh_result);
        }
        // Apply everything the shell renders live: theme, language, hotkeys,

        // fonts, topbar behavior and its label, and the Windows-facing status

        // line.

        apply_theme(engine_->settings().ui.theme_id);
        apply_ui_language(engine_->settings().ui.language);
        register_layout_hotkey();
        register_encoding_hotkey();
        refresh_app_fonts();
        if (topbar_ != nullptr) {
            TopbarSettingsSnapshot snapshot;
            snapshot.idle_dimming_enabled = settings.topbar.idle_dimming_enabled;
            snapshot.idle_opacity_percent = settings.topbar.idle_opacity_percent;
            snapshot.idle_time_seconds = settings.topbar.idle_time_seconds;
            snapshot.remember_position = settings.topbar.remember_position;
            topbar_->apply_settings(snapshot);
            topbar_->refresh_layout_label();
        }
        refresh_status();
        // The language switch is the news worth announcing when it changed:

        // the toast fires only on the success path (post-save, post-apply).

        if (settings.ui.language != previous_language) {
            show_toast(
                "toast.language_changed", L"Interface language changed", ToastKind::information);
        } else {
            show_toast("toast.settings_saved", L"Settings saved", ToastKind::success);
        }
        staged_ = engine_->settings();
        build_pages();
        focus_footer_ = 0;
        save_pressed_ = false;
        cancel_pressed_ = false;
        if (tray_added_ && !smoke_test_) {
            ::ShowWindow(window_, SW_HIDE);
        }
    }

    // ---- Start mode, hotkey, and font plumbing ----

    // Chooses the surface shown at launch. Runs from run() before the topbar

    // exists: it only sets topbar_visible_ and may open the settings window.

    void apply_start_mode() {
        const auto settings = engine_->settings();
        suborno::settings::StartMode mode = settings.general.start_mode;
        if (startup_ && mode == suborno::settings::StartMode::main_window) {
            // Launched from the Windows sign-in hook: opening the settings

            // window on top of the user's work would be intrusive.

            mode = suborno::settings::StartMode::topbar;
        }
        switch (mode) {
        case suborno::settings::StartMode::topbar:
            topbar_visible_ = true;
            break;
        case suborno::settings::StartMode::system_tray:
            topbar_visible_ = false;
            break;
        case suborno::settings::StartMode::main_window:
            topbar_visible_ = true;
            show_window();
            break;
        case suborno::settings::StartMode::last_state:
            topbar_visible_ = settings.general.last_ui_mode != "tray";
            if (settings.general.last_ui_mode == "window") {
                show_window();
            }
            break;
        }
    }

    void persist_last_ui_mode(const char* mode) {
        auto settings = engine_->settings();
        if (settings.general.last_ui_mode == mode) {
            return;
        }
        settings.general.last_ui_mode = mode;
        engine_->save_settings(settings);
    }

    std::wstring effective_font_family(const suborno::settings::EngineSettings& settings) const {
        if (settings.ui.use_os_default_bangla_font || settings.ui.font_family.empty()) {
            return {};
        }
        return utf8_to_wide(settings.ui.font_family);
    }

    // Applies the effective font family to the process-wide UI font and

    // recreates every cached font object the desktop shell owns.

    void refresh_app_fonts() {
        const std::wstring family = effective_font_family(engine_->settings());
        ui::set_ui_font_family(family.c_str());
        if (message_font_ != nullptr) {
            ::DeleteObject(message_font_);
            message_font_ = nullptr;
        }
        if (font_title_ != nullptr) {
            ::DeleteObject(font_title_);
            font_title_ = nullptr;
        }
        if (font_heading_ != nullptr) {
            ::DeleteObject(font_heading_);
            font_heading_ = nullptr;
        }
        if (font_caption_ != nullptr) {
            ::DeleteObject(font_caption_);
            font_caption_ = nullptr;
        }
        // ui::create_message_font() applies the process-wide family; the

        // anonymous-namespace helper below predates it and would ignore the

        // configured font.

        message_font_ = ui::create_message_font();
        font_title_ = ui::create_title_font(ui_dpi_);
        font_heading_ = ui::create_heading_font(ui_dpi_);
        font_caption_ = ui::create_caption_font(ui_dpi_);
        if (topbar_ != nullptr) {
            topbar_->refresh_font();
        }
        if (osk_ != nullptr) {
            osk_->refresh_font();
        }
        ::InvalidateRect(window_, nullptr, TRUE);
    }

    // Applies the saved theme to the process-wide palette and repaints every

    // live surface (settings window, topbar, OSK) so the switch is visible

    // immediately.

    void apply_theme(const std::string& theme_id) {
        ui::set_active_theme(utf8_to_wide(theme_id));
        ::InvalidateRect(window_, nullptr, TRUE);
        if (topbar_ != nullptr && topbar_->visible()) {
            topbar_->refresh_layout_label();
        }
        if (osk_ != nullptr) {
            osk_->refresh_labels();
        }
    }

    // Loads the locale table for the interface language, clears the per-key

    // text cache and refreshes the surfaces whose titles live in code. Page

    // controls rebuild through build_pages() at their next refresh.

    void apply_ui_language(const std::string& language) {
        std::filesystem::path i18n_root;
        std::vector<wchar_t> executable(32768);
        if (::GetModuleFileNameW(nullptr, executable.data(), static_cast<DWORD>(executable.size()))
            != 0) {
            i18n_root = std::filesystem::path(executable.data()).parent_path() / L"data" / L"i18n";
        }
        ui::load_ui_strings(language, i18n_root);
        ui::clear_text_cache();
        if (window_ != nullptr) {
            ::SetWindowTextW(window_, ui::text("settings.window.title", L"Suborno Settings"));
        }
        if (osk_ != nullptr) {
            osk_->refresh_labels();
        }
        ::InvalidateRect(window_, nullptr, TRUE);
    }

    // Per-Monitor v2 reflow for the fixed-design settings window: refit the

    // DPI to the NEW monitor's work area, recreate the fonts, re-cut the

    // rounded region and move to the system-suggested rectangle.

    void handle_settings_dpi_changed(WPARAM w_param, LPARAM l_param) {
        const UINT new_dpi = HIWORD(w_param);
        RECT work_area{};
        if (l_param != 0) {
            const auto* suggested = reinterpret_cast<const RECT*>(l_param);
            const HMONITOR monitor = ::MonitorFromRect(suggested, MONITOR_DEFAULTTONEAREST);
            MONITORINFO info{};
            info.cbSize = sizeof(info);
            if (monitor != nullptr && ::GetMonitorInfoW(monitor, &info) != FALSE) {
                work_area = info.rcWork;
            }
        }
        ui_dpi_ = ui::dpi_fitting(work_area, window_width, window_height, new_dpi);
        refresh_app_fonts();  // recreates every cached font at the new DPI

        if (l_param != 0) {
            const auto* suggested = reinterpret_cast<const RECT*>(l_param);
            const int width = suggested->right - suggested->left;
            const int height = suggested->bottom - suggested->top;
            ::SetWindowPos(window_,
                           nullptr,
                           suggested->left,
                           suggested->top,
                           width,
                           height,
                           SWP_NOACTIVATE | SWP_NOZORDER);
            ui::apply_window_treatment(window_, width, height, scaled(8));
        }
        ::InvalidateRect(window_, nullptr, TRUE);
    }

    // One themed toast near the notification area (optional UI — silently

    // absent when the toast window could not be created). `key` is the

    // narrow i18n lookup key (ui::text's key type); `fallback` and the

    // optional {placeholder}/{value} pair are the wide display text.

    void show_toast(const char* key,
                    const wchar_t* fallback,
                    ToastKind kind,
                    const wchar_t* placeholder = nullptr,
                    const wchar_t* value = nullptr) {
        if (window_ == nullptr || smoke_test_) {
            return;
        }
        const std::wstring message = ui::format_text(ui::text(key, fallback), placeholder, value);
        toast_.show(window_, message.c_str(), kind);
    }

    // Persists a font picked from the tray menu / topbar menu immediately.

    void save_font(const std::wstring& family) {
        auto settings = engine_->settings();
        if (family.empty()) {
            settings.ui.use_os_default_bangla_font = true;
            settings.ui.font_family.clear();
        } else {
            settings.ui.use_os_default_bangla_font = false;
            settings.ui.font_family = wide_to_utf8(family);
        }
        const auto result = engine_->save_settings(settings);
        if (result != suborno::settings::SettingsStatus::ok) {
            show_error(window_,
                       ui::text("error.font_save_failed", L"Could not save the selected font."),
                       E_FAIL);
            return;
        }
        refresh_app_fonts();
        refresh_status();
        staged_ = engine_->settings();
        build_pages();
        show_toast("toast.font_changed",
                   L"Font: {font}",
                   ToastKind::information,
                   L"{font}",
                   family.empty() ? ui::text("topbar.menu.font_system_default", L"System Default")
                                  : family.c_str());
    }

    void register_layout_hotkey() {
        if (window_ == nullptr || smoke_test_) {
            return;
        }
        if (hotkey_registered_) {
            ::UnregisterHotKey(window_, layout_hotkey_id);
            hotkey_registered_ = false;
        }
        const auto binding = hotkey_for_shortcut(engine_->settings().input.layout_switch_shortcut);
        if (!binding.has_value()) {
            return;
        }
        hotkey_registered_ =
            ::RegisterHotKey(
                window_, layout_hotkey_id, binding->modifiers | MOD_NOREPEAT, binding->virtual_key)
            != FALSE;
        if (!hotkey_registered_) {
            ::OutputDebugStringW(L"Suborno: layout switching hotkey is already in use.\n");
        }
    }

    // Ctrl+Alt+E toggles the output encoding (Unicode ↔ Bijoy ANSI) from

    // anywhere; the Input tab's "Disable the encoding toggle shortcut" turns

    // the registration off.

    void register_encoding_hotkey() {
        if (window_ == nullptr || smoke_test_) {
            return;
        }
        if (encoding_hotkey_registered_) {
            ::UnregisterHotKey(window_, encoding_hotkey_id);
            encoding_hotkey_registered_ = false;
        }
        if (engine_->settings().input.disable_encoding_shortcut) {
            return;
        }
        encoding_hotkey_registered_ =
            ::RegisterHotKey(window_, encoding_hotkey_id, MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, 'E')
            != FALSE;
        if (!encoding_hotkey_registered_) {
            ::OutputDebugStringW(L"Suborno: encoding toggle hotkey is already in use.\n");
        }
    }

    // Hotkey action: flip the persisted encoding and announce it through the

    // tray tooltip / status line so the switch is visible without opening

    // Settings.

    void toggle_encoding() {
        auto settings = engine_->settings();
        settings.input.preferred_encoding =
            settings.input.preferred_encoding == suborno::settings::TextEncoding::ansi
                ? suborno::settings::TextEncoding::unicode
                : suborno::settings::TextEncoding::ansi;
        if (engine_->save_settings(settings) != suborno::settings::SettingsStatus::ok) {
            show_error(window_,
                       ui::text("error.encoding_switch_failed",
                                L"Suborno could not switch the text encoding."),
                       E_FAIL);
            return;
        }
        staged_ = engine_->settings();
        build_pages();
        refresh_status();
        show_toast(settings.input.preferred_encoding == suborno::settings::TextEncoding::ansi
                       ? "toast.encoding_bijoy"
                       : "toast.encoding_unicode",
                   settings.input.preferred_encoding == suborno::settings::TextEncoding::ansi
                       ? L"Encoding: Bijoy ANSI"
                       : L"Encoding: Unicode",
                   ToastKind::information);
    }

    // Global shortcut: advance to the next layout in the visible order

    // (English → বাংলা (Suborno) → বাংলা (Borno) → …).

    void cycle_layout() {
        const std::string active = engine_->active_layout_id();
        for (std::size_t index = 0; index < layouts.size(); ++index) {
            if (active == layouts[index].id) {
                const std::size_t next = (index + 1) % layouts.size();
                save_layout(layouts[next].id);
                return;
            }
        }
        save_layout(layouts[0].id);
    }

    void paint_settings_window(HDC device_context) {
        RECT client{};
        ::GetClientRect(window_, &client);
        HBRUSH surface = ::CreateSolidBrush(ui::theme_surface());
        if (surface != nullptr) {
            ::FillRect(device_context, &client, surface);
            ::DeleteObject(surface);
        }
        if (message_font_ != nullptr) {
            ::SelectObject(device_context, message_font_);
        }

        // Custom title row (heading scale, see ui::create_heading_font).

        const RECT title_rect = design_rect(30, 4, 320, 32);
        if (font_heading_ != nullptr) {
            ::SelectObject(device_context, font_heading_);
        }
        ui::draw_text(device_context,
                      ui::text("settings.window.title", L"Suborno Settings"),
                      title_rect,
                      ui::theme_text());
        if (message_font_ != nullptr) {
            ::SelectObject(device_context, message_font_);
        }
        const RECT close_rect = design_rect(696, 4, 32, 32);
        if (close_hover_) {
            ui::fill_rounded(device_context, close_rect, ui::theme_surface_hover(), scaled(6));
        }
        const int close_cx = (close_rect.left + close_rect.right) / 2;
        const int close_cy = (close_rect.top + close_rect.bottom) / 2;
        const int close_arm = scaled(6);
        HPEN close_pen = ui::create_round_pen(scaled(2), ui::theme_text());
        if (close_pen != nullptr) {
            const HGDIOBJ previous = ::SelectObject(device_context, close_pen);
            ::MoveToEx(device_context, close_cx - close_arm, close_cy - close_arm, nullptr);
            ::LineTo(device_context, close_cx + close_arm, close_cy + close_arm);
            ::MoveToEx(device_context, close_cx + close_arm, close_cy - close_arm, nullptr);
            ::LineTo(device_context, close_cx - close_arm, close_cy + close_arm);
            ::SelectObject(device_context, previous);
            ::DeleteObject(close_pen);
        }

        // Sidebar pages.

        for (int index = 0; index < page_count; ++index) {
            const RECT pill = design_rect(sidebar_left,
                                          sidebar_top + index * sidebar_stride,
                                          sidebar_width,
                                          sidebar_pill_height);
            if (static_cast<int>(page_) == index) {
                ui::fill_rounded(device_context, pill, ui::theme_surface_raised(), scaled(8));
            } else if (sidebar_hover_ == index) {
                ui::fill_rounded(device_context, pill, ui::theme_surface_hover(), scaled(8));
            }
            RECT text_rect{pill.left + scaled(12), pill.top, pill.right, pill.bottom};
            ui::draw_text(device_context, sidebar_title(index), text_rect, ui::theme_text());
        }

        // Content panel outline.

        const RECT panel = design_rect(208, 56, 512, 496);
        ui::frame_rounded(device_context, panel, ui::theme_separator(), scaled(8));

        // Controls.

        std::vector<UiControl>& controls = page_controls();
        for (std::size_t index = 0; index < controls.size(); ++index) {
            paint_control(device_context, controls[index], index == focus_index_);
        }
        if (page_ == PageId::general && !status_line_.empty()) {
            // Status color reinforces the words (success = enabled,

            // warning = disabled or unavailable); never color alone.

            ui::draw_text(device_context,
                          status_line_.c_str(),
                          design_rect(content_left, 496, 470, 40),
                          status_enabled_ ? ui::theme_success() : ui::theme_warning(),
                          DT_LEFT | DT_TOP | DT_WORDBREAK | DT_NOPREFIX);
        }

        // Save / Cancel buttons (bottom-right, reference placement). Save is

        // the primary action: accent fill with hover/pressed/focus faces;

        // Cancel is the neutral secondary.

        const RECT save_rect = save_button_rect();
        const RECT cancel_rect = cancel_button_rect();
        const COLORREF save_face = save_pressed_ ? ui::theme_accent_hover()
                                   : save_hover_ ? ui::theme_accent()
                                                 : ui::theme_accent_fill();
        ui::fill_rounded(device_context, save_rect, save_face, scaled(6));
        ui::fill_rounded(device_context,
                         cancel_rect,
                         cancel_pressed_ ? ui::theme_surface_pressed()
                         : cancel_hover_ ? ui::theme_surface_hover()
                                         : ui::theme_surface_raised(),
                         scaled(6));
        ui::draw_text(device_context,
                      ui::text("settings.action.save", L"Save"),
                      save_rect,
                      ui::theme_on_accent(),
                      DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        ui::draw_text(device_context,
                      ui::text("settings.action.cancel", L"Cancel"),
                      cancel_rect,
                      ui::theme_text(),
                      DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        if (focus_footer_ == 1) {
            draw_focus_ring(device_context, save_rect);
        } else if (focus_footer_ == 2) {
            draw_focus_ring(device_context, cancel_rect);
        }
    }

    void paint_control(HDC device_context, UiControl& control, bool focused) {
        // control.rect holds 96-DPI design coordinates; convert once here.

        const RECT px{scaled(control.rect.left),
                      scaled(control.rect.top),
                      scaled(control.rect.right),
                      scaled(control.rect.bottom)};
        switch (control.kind) {
        case CtrlKind::section_label: {
            // Section header on the title scale with an inline caption

            // description (no extra rows: the hint sits beside the title).

            if (font_title_ != nullptr) {
                ::SelectObject(device_context, font_title_);
            }
            RECT title_rect = px;
            ::DrawTextW(device_context,
                        control.label.c_str(),
                        -1,
                        &title_rect,
                        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_CALCRECT | DT_NOPREFIX);
            ui::draw_text(device_context, control.label.c_str(), px, ui::theme_text());
            if (font_caption_ != nullptr) {
                ::SelectObject(device_context, font_caption_);
            }
            if (!control.description.empty()) {
                RECT description_rect{title_rect.right + scaled(12), px.top, px.right, px.bottom};
                ui::draw_text(device_context,
                              control.description.c_str(),
                              description_rect,
                              ui::theme_text_inactive());
            }
            if (message_font_ != nullptr) {
                ::SelectObject(device_context, message_font_);
            }
            break;
        }
        case CtrlKind::checkbox: {
            // Hover feedback for the whole row, matching dropdown/button.

            if (control.hover) {
                ui::fill_rounded(device_context, px, ui::theme_surface_hover(), scaled(4));
            }
            RECT box{px.left, px.top + scaled(3), px.left + scaled(18), px.top + scaled(21)};
            if (control.checked) {
                ui::fill_rounded(device_context, box, ui::theme_accent_fill(), scaled(4));
                const int cx = (box.left + box.right) / 2;
                const int cy = (box.top + box.bottom) / 2;
                const POINT check[3]{{cx - scaled(5), cy},
                                     {cx - scaled(1), cy + scaled(4)},
                                     {cx + scaled(6), cy - scaled(4)}};
                HPEN pen = ui::create_round_pen(scaled(2), ui::theme_on_accent());
                if (pen != nullptr) {
                    const HGDIOBJ previous = ::SelectObject(device_context, pen);
                    ::Polyline(device_context, check, 3);
                    ::SelectObject(device_context, previous);
                    ::DeleteObject(pen);
                }
            } else {
                ui::frame_rounded(device_context, box, ui::theme_control_border(), scaled(4));
            }
            RECT label_rect{box.right + scaled(12), px.top, px.right, px.bottom};
            ui::draw_text(device_context, control.label.c_str(), label_rect, ui::theme_text());
            if (focused) {
                draw_focus_ring(device_context, px);
            }
            break;
        }
        case CtrlKind::dropdown: {
            ui::fill_rounded(device_context,
                             px,
                             control.hover ? ui::theme_surface_hover() : ui::theme_surface_raised(),
                             scaled(6));
            ui::frame_rounded(device_context, px, ui::theme_control_border(), scaled(6));
            RECT label_rect{px.left + scaled(10), px.top, px.right - scaled(28), px.bottom};
            const wchar_t* text =
                control.dropdown_index >= 0
                        && control.dropdown_index < static_cast<int>(control.dropdown_items.size())
                    ? control.dropdown_items[static_cast<std::size_t>(control.dropdown_index)]
                          .c_str()
                    : L"";
            ui::draw_text(device_context, text, label_rect, ui::theme_text());
            const int chevron_cx = px.right - scaled(16);
            const int chevron_cy = (px.top + px.bottom) / 2;
            const POINT chevron[3]{{chevron_cx - scaled(5), chevron_cy - scaled(3)},
                                   {chevron_cx + scaled(5), chevron_cy - scaled(3)},
                                   {chevron_cx, chevron_cy + scaled(3)}};
            HBRUSH brush = ::CreateSolidBrush(ui::theme_text());
            if (brush != nullptr) {
                HPEN pen = ::CreatePen(PS_SOLID, 0, ui::theme_text());
                const HGDIOBJ previous_pen = ::SelectObject(device_context, pen);
                const HGDIOBJ previous_brush = ::SelectObject(device_context, brush);
                ::Polygon(device_context, chevron, 3);
                ::SelectObject(device_context, previous_brush);
                ::SelectObject(device_context, previous_pen);
                ::DeleteObject(pen);
                ::DeleteObject(brush);
            }
            if (focused) {
                draw_focus_ring(device_context, px);
            }
            break;
        }
        case CtrlKind::slider: {
            RECT label_rect{px.left, px.top, px.left + scaled(160), px.bottom};
            ui::draw_text(device_context, control.label.c_str(), label_rect, ui::theme_text());
            const int track_y = (px.top + px.bottom) / 2 - scaled(2);
            const int track_left = px.left + scaled(164);
            const int track_right = px.left + scaled(444);
            const int range = control.value_max - control.value_min;
            const int position = range <= 0
                                     ? track_left
                                     : track_left
                                           + (track_right - track_left)
                                                 * (control.value - control.value_min) / range;
            // Filled portion in the accent fill, remainder on the track.

            if (position > track_left) {
                RECT filled{track_left, track_y, position, track_y + scaled(4)};
                ui::fill_rounded(device_context, filled, ui::theme_accent_fill(), scaled(2));
            }
            RECT track{position, track_y, track_right, track_y + scaled(4)};
            if (track.right > track.left) {
                ui::fill_rounded(device_context, track, ui::theme_track(), scaled(2));
            }
            RECT handle{position - scaled(8),
                        track_y - scaled(6),
                        position + scaled(8),
                        track_y + scaled(10)};
            ui::fill_rounded(device_context, handle, ui::theme_surface_raised(), scaled(8));
            ui::frame_rounded(device_context, handle, ui::theme_control_border(), scaled(8));
            RECT value_rect{track_right + scaled(12), px.top, px.right, px.bottom};
            ui::draw_text(device_context,
                          std::to_wstring(control.value).c_str(),
                          value_rect,
                          ui::theme_text());
            if (focused) {
                draw_focus_ring(device_context, px);
            }
            break;
        }
        case CtrlKind::status:
            ui::draw_text(device_context, control.label.c_str(), px, ui::theme_text_inactive());
            break;
        case CtrlKind::button: {
            ui::fill_rounded(device_context,
                             px,
                             control.hover ? ui::theme_surface_hover() : ui::theme_surface_raised(),
                             scaled(6));
            ui::frame_rounded(device_context, px, ui::theme_control_border(), scaled(6));
            ui::draw_text(device_context, control.label.c_str(), px, ui::theme_text());
            if (focused) {
                draw_focus_ring(device_context, px);
            }
            break;
        }
        }
    }

    void refresh_status() {
        // The status line reflects the REAL Windows input-method state.

        bool enabled = false;
        const HRESULT result = profile_enabled(enabled);
        const auto layout = std::find_if(layouts.begin(), layouts.end(), [this](const auto& item) {
            return engine_->settings().active_layout_id == item.id;
        });
        const wchar_t* layout_name = layout == layouts.end() ? L"Unknown" : layout->name;
        if (FAILED(result)) {
            wchar_t code[16]{};
            swprintf_s(code, _countof(code), L"0x%08lX", static_cast<unsigned long>(result));
            status_line_ =
                ui::format_text(ui::text("settings.status.unavailable",
                                         L"Windows input status is unavailable (code {code})."),
                                L"{code}",
                                code);
        } else {
            status_line_ = ui::format_text(
                enabled
                    ? ui::text(
                          "settings.status.enabled",
                          L"Suborno is enabled in Windows input methods. Current layout: {layout}.")
                    : ui::text(
                          "settings.status.disabled",
                          L"Suborno is disabled in Windows input methods. Current layout: {layout}."),
                L"{layout}",
                layout_name);
        }
        update_tray_tooltip(enabled, layout_name);
        // Color + text: the status words carry the meaning, the token color

        // reinforces it (success = enabled, warning = disabled/unavailable).

        status_enabled_ = !FAILED(result) && enabled;
        if (topbar_ != nullptr) {
            topbar_->refresh_layout_label();
        }
    }

    void save_layout(const char* layout_id) {
        auto settings = engine_->settings();
        settings.active_layout_id = layout_id;
        if (std::string_view(layout_id) != "english") {
            // Remember the Bangla layout the English↔বাংলা mode toggle should

            // return to (topbar icon / OSK / tray).

            settings.general.last_bangla_layout_id = layout_id;
        }
        const auto result = engine_->save_settings(settings);
        if (result != suborno::settings::SettingsStatus::ok) {
            show_error(window_,
                       ui::text("error.layout_save_failed",
                                L"Could not save the selected keyboard layout."),
                       E_FAIL);
            return;
        }
        const HRESULT refresh_result = refresh_active_profile();
        if (FAILED(refresh_result)) {
            show_error(window_,
                       ui::text("error.profile_refresh_after_layout",
                                L"Layout saved, but Windows could not refresh the active profile."),
                       refresh_result);
        }
        refresh_status();
        if (osk_ != nullptr) {
            osk_->refresh_labels();
        }
        // Announce the switch so the change is visible even when the change

        // came from a hotkey.

        show_toast("toast.layout_switched",
                   L"Layout: {layout}",
                   ToastKind::information,
                   L"{layout}",
                   layout_display_name(layout_id));
    }

    const wchar_t* layout_display_name(const char* layout_id) const noexcept {
        for (const LayoutOption& layout : layouts) {
            if (std::string_view(layout.id) == layout_id) {
                return layout.name;
            }
        }
        return L"Suborno";
    }
    void toggle_english_bangla() {
        if (engine_->active_layout_id() == "english") {
            std::string bangla = engine_->settings().general.last_bangla_layout_id;
            if (bangla.empty() || bangla == "english") {
                bangla = "suborno-phonetic";
            }
            save_layout(bangla.c_str());
        } else {
            save_layout("english");
        }
    }

    void open_uninstall_options() {
        const HINSTANCE result =
            ::ShellExecuteW(nullptr, L"open", L"appwiz.cpl", nullptr, nullptr, SW_SHOWNORMAL);
        if (reinterpret_cast<INT_PTR>(result) <= 32) {
            show_error(window_,
                       ui::text("error.uninstall_open_failed",
                                L"Could not open Windows uninstall options."),
                       E_FAIL);
        }
    }

    void toggle_profile() {
        bool enabled = false;
        HRESULT result = profile_enabled(enabled);
        if (SUCCEEDED(result)) {
            result = set_profile_enabled(!enabled);
        }
        if (FAILED(result)) {
            show_error(window_,
                       ui::text("error.profile_toggle_failed",
                                L"Could not update Suborno's Windows input-method status."),
                       result);
            return;
        }
        refresh_status();
        bool enabled_now = false;
        if (SUCCEEDED(profile_enabled(enabled_now))) {
            show_toast(
                enabled_now ? "toast.profile_enabled" : "toast.profile_disabled",
                enabled_now ? L"Suborno is enabled in Windows" : L"Suborno is disabled in Windows",
                enabled_now ? ToastKind::success : ToastKind::warning);
        }
    }

    HRESULT add_tray_icon() {
        tray_data_.cbSize = sizeof(tray_data_);
        tray_data_.hWnd = window_;
        tray_data_.uID = tray_icon_id;
        tray_data_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        tray_data_.uCallbackMessage = tray_message;
        tray_data_.hIcon = ::LoadIconW(instance_, MAKEINTRESOURCEW(IDI_SUBORNO));
        if (tray_data_.hIcon == nullptr) {
            return E_FAIL;
        }
        ::wcscpy_s(tray_data_.szTip, _countof(tray_data_.szTip), L"Suborno");
        tray_added_ = ::Shell_NotifyIconW(NIM_ADD, &tray_data_) != FALSE;
        if (!tray_added_) {
            const DWORD error = ::GetLastError();
            return error == ERROR_SUCCESS ? E_FAIL : HRESULT_FROM_WIN32(error);
        }
        tray_data_.uVersion = NOTIFYICON_VERSION_4;
        if (!::Shell_NotifyIconW(NIM_SETVERSION, &tray_data_)) {
            const DWORD error = ::GetLastError();
            const HRESULT result = error == ERROR_SUCCESS ? E_FAIL : HRESULT_FROM_WIN32(error);
            remove_tray_icon();
            return result;
        }
        return S_OK;
    }

    void remove_tray_icon() {
        if (tray_added_) {
            ::Shell_NotifyIconW(NIM_DELETE, &tray_data_);
            tray_added_ = false;
        }
    }

    void update_tray_tooltip(bool enabled, const wchar_t* layout) {
        if (!tray_added_) {
            return;
        }
        tray_data_.uFlags = NIF_TIP;
        swprintf_s(tray_data_.szTip,
                   _countof(tray_data_.szTip),
                   L"Suborno - %s - %s",
                   enabled ? ui::text("settings.status.word_enabled", L"enabled")
                           : ui::text("settings.status.word_disabled", L"disabled"),
                   layout);
        ::Shell_NotifyIconW(NIM_MODIFY, &tray_data_);
    }

    void handle_tray_message(LPARAM event) {
        const UINT event_code = LOWORD(event);
        if (event_code == WM_LBUTTONUP || event_code == WM_LBUTTONDBLCLK || event_code == NIN_SELECT
            || event_code == NIN_KEYSELECT) {
            // Left click reveals the floating topbar — the app's main UI.

            show_topbar();
        } else if (event_code == WM_RBUTTONUP || event_code == WM_CONTEXTMENU) {
            show_tray_menu();
        }
    }

    void show_tray_menu() {
        POINT cursor{};
        ::GetCursorPos(&cursor);
        show_tray_menu_at(cursor);
    }

    // Builds and tracks the full menu at an explicit screen point (tray

    // right-click and the topbar context menu share it).

    void show_tray_menu_at(POINT at) {
        // Reference tray-menu order (Screenshot_24) mapped onto real Suborno

        // actions; Enable/Disable and Uninstall are preserved features. The

        // same builder backs the topbar right-click context menu.

        HMENU menu = ::CreatePopupMenu();
        HMENU layouts = ::CreatePopupMenu();
        HMENU fonts = ::CreatePopupMenu();
        if (menu == nullptr || layouts == nullptr || fonts == nullptr) {
            if (menu != nullptr) {
                ::DestroyMenu(menu);
            }
            if (layouts != nullptr) {
                ::DestroyMenu(layouts);
            }
            if (fonts != nullptr) {
                ::DestroyMenu(fonts);
            }
            return;
        }
        bool enabled = false;
        const HRESULT result = profile_enabled(enabled);
        const wchar_t* toggle_label =
            SUCCEEDED(result) && enabled
                ? ui::text("topbar.menu.disable", L"Disable Suborno in Windows")
                : ui::text("topbar.menu.enable", L"Enable Suborno in Windows");
        ::AppendMenuW(
            menu, MF_STRING, menu_show_topbar, ui::text("topbar.menu.show_topbar", L"Show Topbar"));
        ::CheckMenuItem(menu,
                        menu_show_topbar,
                        topbar_visible_ ? MF_BYCOMMAND | MF_CHECKED : MF_BYCOMMAND | MF_UNCHECKED);
        for (std::size_t index = 0; index < layouts.size(); ++index) {
            const bool selected = engine_->settings().active_layout_id == layouts[index].id;
            ::AppendMenuW(layouts,
                          MF_STRING | (selected ? MF_CHECKED : MF_UNCHECKED),
                          menu_layout_first + static_cast<UINT>(index),
                          layouts[index].name);
        }
        ::AppendMenuW(menu,
                      MF_POPUP,
                      reinterpret_cast<UINT_PTR>(layouts),
                      ui::text("topbar.menu.layout", L"Keyboard Layout"));

        // Font submenu: same options as Settings → Interface, applied live.

        const std::wstring active_font = effective_font_family(engine_->settings());
        ::AppendMenuW(fonts,
                      MF_STRING | (active_font.empty() ? MF_CHECKED : MF_UNCHECKED),
                      menu_font_default,
                      ui::text("topbar.menu.font_system_default", L"System Default"));
        const std::vector<std::wstring>& font_options = bangla_font_options();
        for (std::size_t index = 0; index < font_options.size(); ++index) {
            const bool selected = ::_wcsicmp(active_font.c_str(), font_options[index].c_str()) == 0;
            ::AppendMenuW(fonts,
                          MF_STRING | (selected ? MF_CHECKED : MF_UNCHECKED),
                          menu_font_first + static_cast<UINT>(index),
                          font_options[index].c_str());
        }
        ::AppendMenuW(menu,
                      MF_POPUP,
                      reinterpret_cast<UINT_PTR>(fonts),
                      ui::text("topbar.menu.font", L"Font"));

        ::AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        ::AppendMenuW(
            menu, MF_STRING, menu_osk, ui::text("topbar.menu.osk", L"On-Screen Keyboard…"));
        ::AppendMenuW(
            menu, MF_STRING, menu_open_settings, ui::text("topbar.menu.settings", L"Settings…"));
        ::AppendMenuW(menu, MF_STRING, menu_toggle_input, toggle_label);
        ::AppendMenuW(
            menu, MF_STRING, menu_updates, ui::text("topbar.menu.updates", L"Check for Updates…"));
        ::AppendMenuW(menu,
                      MF_STRING,
                      menu_documentation,
                      ui::text("topbar.menu.documentation", L"Documentation"));
        ::AppendMenuW(menu,
                      MF_STRING,
                      menu_clear_learnt,
                      ui::text("topbar.menu.clear_learnt", L"Clear Learnt Words…"));
        ::AppendMenuW(
            menu, MF_STRING, menu_apps, ui::text("topbar.menu.uninstall", L"Uninstall Suborno"));
        ::AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        ::AppendMenuW(menu, MF_STRING, menu_exit, ui::text("topbar.menu.exit", L"Exit"));

        ::SetForegroundWindow(window_);
        ::TrackPopupMenu(menu,
                         TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_LEFTALIGN,
                         at.x,
                         at.y,
                         0,
                         window_,
                         nullptr);
        ::PostMessageW(window_, WM_NULL, 0, 0);
        ::DestroyMenu(menu);
    }

    // The full desktop context menu for the topbar right-click (all entries

    // with real handlers — the same items as the tray menu).

    void show_desktop_context_menu(POINT screen) {
        show_tray_menu_at(screen);
    }

    void handle_menu_command(UINT id) {
        if (id == menu_font_default) {
            save_font({});
            return;
        }
        if (id >= menu_font_first) {
            const std::size_t index = static_cast<std::size_t>(id - menu_font_first);
            const std::vector<std::wstring>& fonts = bangla_font_options();
            if (index < fonts.size()) {
                save_font(fonts[index]);
            }
            return;
        }
        switch (id) {
        case menu_open:
        case menu_open_settings:
            show_window();
            break;
        case menu_osk:
            toggle_osk();
            break;
        case menu_updates:
            open_web_link(window_,
                          releases_url,
                          ui::text("error.releases_open_failed",
                                   L"Suborno could not open the releases page."));
            break;
        case menu_documentation:
            open_web_link(window_,
                          documentation_url,
                          ui::text("error.documentation_open_failed",
                                   L"Suborno could not open the documentation."));
            break;
        case menu_clear_learnt:
            clear_learnt_words_from_ui();
            break;
        case menu_toggle_input:
            toggle_profile();
            break;
        case menu_apps:
            open_uninstall_options();
            break;
        case menu_show_topbar:
            toggle_topbar();
            break;
        case menu_exit:
            ::DestroyWindow(window_);
            break;
        default:
            if (id >= menu_layout_first
                && id < menu_layout_first + static_cast<int>(layouts.size())) {
                save_layout(layouts[static_cast<std::size_t>(id - menu_layout_first)].id);
            }
            break;
        }
    }

    void toggle_topbar() {
        topbar_visible_ = !topbar_visible_;
        if (topbar_ != nullptr) {
            if (topbar_visible_) {
                topbar_->refresh_layout_label();
                topbar_->show();
            } else {
                topbar_->hide();
            }
        }
        persist_last_ui_mode(topbar_visible_ ? "topbar" : "tray");
    }

    void show_topbar() {
        topbar_visible_ = true;
        if (topbar_ != nullptr) {
            topbar_->refresh_layout_label();
            topbar_->show();
        }
        persist_last_ui_mode("topbar");
    }

    void setup_topbar() {
        if (topbar_ != nullptr) {
            return;
        }
        Topbar::Host host;
        host.active_layout_id = [this]() -> std::string {
            return std::string(engine_->active_layout_id());
        };
        host.layouts = []() {
            std::vector<std::pair<std::string, std::wstring>> options;
            options.reserve(layouts.size());
            for (const LayoutOption& layout : layouts) {
                options.emplace_back(layout.id, layout.name);
            }
            return options;
        };
        host.select_layout = [this](const std::string& layout_id) {
            save_layout(layout_id.c_str());
        };
        host.toggle_english_bangla = [this]() {
            toggle_english_bangla();
        };
        host.show_context_menu = [this](POINT screen) {
            show_desktop_context_menu(screen);
        };
        host.open_osk = [this]() {
            toggle_osk();
        };
        host.open_settings = [this]() {
            show_window();
        };
        host.open_about = [this]() {
            open_about();
        };
        host.fonts = []() {
            return bangla_font_options();
        };
        host.active_font = [this]() {
            return effective_font_family(engine_->settings());
        };
        host.select_font = [this](const std::wstring& family) {
            save_font(family);
        };
        host.saved_topbar_position = [this]() -> std::optional<POINT> {
            const auto& settings = engine_->settings();
            if (!settings.topbar.remember_position || settings.topbar.position_x < 0
                || settings.topbar.position_y < 0) {
                return std::nullopt;
            }
            return POINT{settings.topbar.position_x, settings.topbar.position_y};
        };
        host.save_topbar_position = [this](int x, int y) {
            auto settings = engine_->settings();
            if (!settings.topbar.remember_position) {
                return;
            }
            if (settings.topbar.position_x == x && settings.topbar.position_y == y) {
                return;
            }
            settings.topbar.position_x = x;
            settings.topbar.position_y = y;
            engine_->save_settings(settings);
        };
        host.switch_to_tray = [this]() {
            topbar_visible_ = false;
            topbar_->hide();
            persist_last_ui_mode("tray");
        };
        host.exit_application = [this]() {
            ::DestroyWindow(window_);
        };
        auto topbar = std::make_unique<Topbar>();
        if (topbar->create(instance_, std::move(host))) {
            TopbarSettingsSnapshot snapshot;
            const auto& settings = engine_->settings();
            snapshot.idle_dimming_enabled = settings.topbar.idle_dimming_enabled;
            snapshot.idle_opacity_percent = settings.topbar.idle_opacity_percent;
            snapshot.idle_time_seconds = settings.topbar.idle_time_seconds;
            snapshot.remember_position = settings.topbar.remember_position;
            topbar->apply_settings(snapshot);
            topbar->refresh_layout_label();
            if (topbar_visible_) {
                topbar->show();
            }
            topbar_ = std::move(topbar);
        }
    }

    void open_about() {
        show_about_dialog(instance_, window_);
    }

    void setup_osk() {
        if (osk_ != nullptr) {
            return;
        }
        OskWindow::Host host;
        host.active_layout_id = [this]() -> std::string {
            return engine_->active_layout_id();
        };
        host.select_mode = [this](bool english) {
            if (english) {
                save_layout("english");
            } else {
                std::string bangla = engine_->settings().general.last_bangla_layout_id;
                if (bangla.empty() || bangla == "english") {
                    bangla = "suborno-phonetic";
                }
                save_layout(bangla.c_str());
            }
        };
        host.mode_display_name = [this]() -> std::wstring {
            const std::string active = engine_->active_layout_id();
            for (const LayoutOption& layout : layouts) {
                if (layout.id == active) {
                    return layout.name;
                }
            }
            return L"Suborno";
        };
        host.bangla_preview = [this](std::string_view letter) -> std::wstring {
            return utf8_to_wide(engine_->layout_preview(letter));
        };
        auto osk = std::make_unique<OskWindow>();
        if (osk->create(instance_, std::move(host))) {
            osk_ = std::move(osk);
        } else {
            ::OutputDebugStringW(L"Suborno: the on-screen keyboard could not be created.\n");
        }
    }

    // The OSK is a modeless tool window: menu entries toggle it, Esc or the

    // ✕ hides it, and layout switches from any surface refresh its labels.

    void toggle_osk() {
        if (osk_ == nullptr) {
            setup_osk();
        }
        if (osk_ == nullptr) {
            show_error(window_,
                       ui::text("error.osk_create_failed",
                                L"Suborno could not open the on-screen keyboard."),
                       E_FAIL);
            return;
        }
        if (osk_->visible()) {
            osk_->hide();
        } else {
            osk_->refresh_labels();
            osk_->show();
        }
    }

    void show_window() {
        staged_ = engine_->settings();
        build_pages();
        focus_index_ = -1;
        focus_footer_ = 0;
        save_pressed_ = false;
        cancel_pressed_ = false;
        slider_drag_ = -1;
        ::InvalidateRect(window_, nullptr, TRUE);
        ::ShowWindow(window_, SW_RESTORE);
        ::SetForegroundWindow(window_);
        ::SetFocus(window_);
        persist_last_ui_mode("window");
    }

    HINSTANCE instance_ = nullptr;
    HWND window_ = nullptr;
    HFONT font_title_ = nullptr;
    HFONT font_heading_ = nullptr;
    HFONT font_caption_ = nullptr;
    HFONT message_font_ = nullptr;
    UINT ui_dpi_ = 96;
    ToastWindow toast_;
    std::shared_ptr<int> a11y_alive_ = std::make_shared<int>(0);
    std::array<std::vector<UiControl>, page_count> pages_;
    PageId page_ = PageId::general;
    int focus_index_ = -1;
    // Keyboard focus for the footer buttons: 0 = none, 1 = Save, 2 = Cancel.

    // Tab walks page controls and then these two buttons (they were only

    // reachable by mouse before the focus footer existed).

    int focus_footer_ = 0;
    int slider_drag_ = -1;
    int sidebar_hover_ = -1;
    bool close_hover_ = false;
    bool save_hover_ = false;
    bool cancel_hover_ = false;
    bool save_pressed_ = false;
    bool cancel_pressed_ = false;
    bool mouse_tracked_ = false;
    std::wstring status_line_;
    // Live Windows input-method state for the status line color: enabled

    // renders with the success token, disabled/unavailable with warning.

    bool status_enabled_ = false;
    suborno::settings::EngineSettings staged_;
    std::unique_ptr<Topbar> topbar_;
    bool topbar_visible_ = true;
    bool startup_ = false;
    bool smoke_test_ = false;
    bool startup_enabled_ = false;
    bool tray_added_ = false;
    bool hotkey_registered_ = false;
    UINT taskbar_created_message_ = 0;
    NOTIFYICONDATAW tray_data_{};
    std::unique_ptr<suborno::core::Engine> engine_;
    std::unique_ptr<OskWindow> osk_;
    bool encoding_hotkey_registered_ = false;
};

}  // namespace

int run(HINSTANCE instance, PWSTR command_line, int show_command) {
    const bool startup =
        command_line != nullptr && std::wcsstr(command_line, L"--startup") != nullptr;
    const bool smoke_test =
        command_line != nullptr && std::wcsstr(command_line, L"--smoke-test") != nullptr;

    HANDLE instance_mutex = ::CreateMutexW(nullptr, TRUE, L"Local\\Suborno.Desktop.Application");
    if (instance_mutex == nullptr) {
        const HRESULT error = HRESULT_FROM_WIN32(::GetLastError());
        report_error(
            nullptr, L"Suborno could not establish its single-instance lock.", error, !smoke_test);
        return smoke_test ? smoke_test_exit_code(error) : 1;
    }
    if (::GetLastError() == ERROR_ALREADY_EXISTS) {
        for (int attempt = 0; attempt < 20; ++attempt) {
            HWND existing = ::FindWindowW(window_class, nullptr);
            if (existing != nullptr) {
                // Double-clicking the desktop icon again reveals the main UI

                // (the floating topbar) of the running instance instead of

                // resurrecting the settings window.

                ::PostMessageW(existing, second_instance_show_topbar, 0, 0);
                break;
            }
            ::Sleep(50);
        }
        ::CloseHandle(instance_mutex);
        return 0;
    }

    const HRESULT com_result = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(com_result)) {
        report_error(
            nullptr, L"Suborno could not initialize Windows services.", com_result, !smoke_test);
        ::ReleaseMutex(instance_mutex);
        ::CloseHandle(instance_mutex);
        return smoke_test ? smoke_test_exit_code(com_result) : 1;
    }
    int exit_code = 1;
    try {
        Application application(instance, startup, smoke_test);
        exit_code = application.run(show_command);
    } catch (const std::bad_alloc&) {
        report_error(nullptr, L"Suborno ran out of memory.", E_OUTOFMEMORY, !smoke_test);
        exit_code = smoke_test ? smoke_test_exit_code(E_OUTOFMEMORY) : 1;
    } catch (const std::filesystem::filesystem_error&) {
        report_error(nullptr, L"Suborno could not access its user data.", E_FAIL, !smoke_test);
        exit_code = smoke_test ? smoke_test_exit_code(E_FAIL) : 1;
    } catch (const std::exception&) {
        report_error(
            nullptr, L"Suborno could not initialize its input engine.", E_FAIL, !smoke_test);
        exit_code = smoke_test ? smoke_test_exit_code(E_FAIL) : 1;
    }

    ::CoUninitialize();
    ::ReleaseMutex(instance_mutex);
    ::CloseHandle(instance_mutex);
    return exit_code;
}

}  // namespace suborno::platform::windows::app
