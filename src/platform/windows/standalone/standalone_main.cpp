// SubornoStandalone.exe — "Zero Bengali Language Pack Dependency" host.

//

// WHY THIS EXISTS

// The TSF IME (suborno_windows_tsf.dll) is the "full IME" experience, but it

// lives inside Windows' input-language model: the Bengali input language must

// exist in Settings, and Win+Space (or the language bar) switches to it. The

// tools this project competes with — Avro Keyboard (hook + SendInput +

// floating preview), instant-switch hook-based IMEs, keyboard-layout

// drivers such as Bijoy — are usable the moment they are installed, with no language pack

// and no Win+Space. This host gives Suborno the same property: it reuses the

// SAME portable engine (suborno_core) as the TSF IME, but drives it from a

// WH_KEYBOARD_LL hook and rewrites the typed word with SendInput Unicode

// injection. No TSF registration, no language bar, no Win+Space, no Bengali

// language pack.

//

// LIFECYCLE

//   single-instance mutex -> engine (shared settings.ini) -> tray -> LL hook

//   -> foreground-change WinEvent hook (cancel composition, reload settings,

//   follow layout switches) -> message pump.

//

// Everything typed stays on the machine: no network, no services.

#include <Windows.h>
#include <filesystem>
#include <memory>
#include <shellapi.h>
#include <string>

#include "keyboard_hook.hpp"
#include "preview_window.hpp"
#include "standalone_tray.hpp"
#include "suborno/core/engine.hpp"
#include "suborno/platform/windows/app_paths.hpp"
#include "typing_controller.hpp"

namespace {

constexpr wchar_t single_instance_mutex_name[] = L"Local\\Suborno.Standalone.SingleInstance";
constexpr wchar_t ini_file_name[] = L"standalone.ini";
constexpr wchar_t ini_section[] = L"standalone";
constexpr wchar_t default_hotkey[] = L"ctrl+alt+b";

// Foreground-change handling needs plain function pointers (WinEventProc),

// and the host is single-instance, so the wiring lives in file-scope state.

suborno::platform::windows::standalone::TypingController* g_controller = nullptr;
suborno::platform::windows::standalone::PreviewWindow* g_preview = nullptr;
suborno::core::Engine* g_engine = nullptr;
std::unique_ptr<suborno::layout::InputSession>* g_session_slot = nullptr;
std::unique_ptr<std::wstring> g_active_layout_id;

std::filesystem::path ini_path_from(const std::filesystem::path& settings_path) {
    return settings_path.parent_path() / ini_file_name;
}

std::wstring read_ini_string(const std::filesystem::path& ini_path,
                             const wchar_t* key,
                             const wchar_t* fallback) {
    std::wstring value(256, L'\0');
    const DWORD copied = ::GetPrivateProfileStringW(ini_section,
                                                    key,
                                                    fallback,
                                                    value.data(),
                                                    static_cast<DWORD>(value.size()),
                                                    ini_path.c_str());
    value.resize(copied);
    return value;
}

bool read_ini_bool(const std::filesystem::path& ini_path, const wchar_t* key, bool fallback) {
    const std::wstring raw = read_ini_string(ini_path, key, fallback ? L"1" : L"0");
    return raw == L"1" || raw == L"true" || raw == L"yes" || raw == L"on";
}

std::uint16_t virtual_key_from_name(const std::wstring& name, bool& valid) {
    valid = true;
    if (name.size() == 1) {
        const wchar_t character = name[0];
        if (character >= L'a' && character <= L'z') {
            return static_cast<std::uint16_t>(L'A' + (character - L'a'));
        }
        if (character >= L'A' && character <= L'Z') {
            return static_cast<std::uint16_t>(character);
        }
        if (character >= L'0' && character <= L'9') {
            return static_cast<std::uint16_t>(character);
        }
    }
    if (name == L"space") {
        return VK_SPACE;
    }
    if (name == L"esc" || name == L"escape") {
        return VK_ESCAPE;
    }
    if (name == L"oem_comma") {
        return VK_OEM_COMMA;
    }
    if (name == L"oem_period") {
        return VK_OEM_PERIOD;
    }
    if (name == L"oem_minus") {
        return VK_OEM_MINUS;
    }
    if (name == L"oem_plus") {
        return VK_OEM_PLUS;
    }
    if (name.size() >= 2 && name[0] == L'f') {
        const std::wstring digits = name.substr(1);
        bool digits_only = !digits.empty();
        for (const wchar_t digit : digits) {
            if (digit < L'0' || digit > L'9') {
                digits_only = false;
                break;
            }
        }
        if (digits_only) {
            const int index = ::_wtoi(digits.c_str());
            if (index >= 1 && index <= 24) {
                return static_cast<std::uint16_t>(VK_F1 + (index - 1));
            }
        }
    }
    valid = false;
    return 0;
}

suborno::platform::windows::standalone::KeyboardHookConfig parse_hotkey(const std::wstring& spec) {
    using suborno::platform::windows::standalone::KeyboardHookConfig;
    KeyboardHookConfig config;
    if (spec.empty()) {
        return config;
    }

    bool ctrl = false;
    bool alt = false;
    bool shift = false;
    bool win = false;
    std::wstring key_name;

    std::size_t start = 0;
    while (start <= spec.size()) {
        const std::size_t separator = spec.find(L'+', start);
        const std::wstring token = separator == std::wstring::npos
                                       ? spec.substr(start)
                                       : spec.substr(start, separator - start);
        if (separator == std::wstring::npos) {
            key_name = token;
            break;
        }
        if (token == L"ctrl" || token == L"control") {
            ctrl = true;
        } else if (token == L"alt") {
            alt = true;
        } else if (token == L"shift") {
            shift = true;
        } else if (token == L"win" || token == L"windows") {
            win = true;
        }
        start = separator + 1;
    }

    bool valid = false;
    const std::uint16_t vk = virtual_key_from_name(key_name, valid);
    if (valid && (ctrl || alt || shift || win)) {
        config.toggle_vk = vk;
        config.toggle_ctrl = ctrl;
        config.toggle_alt = alt;
        config.toggle_shift = shift;
        config.toggle_win = win;
    }
    return config;
}

void CALLBACK foreground_changed(HWINEVENTHOOK, DWORD, HWND, LONG, LONG, DWORD, DWORD) {
    if (g_controller == nullptr) {
        return;
    }
    // A new window means a new edit context: drop the live composition and

    // hide the preview (TSF would have done this through its edit cookie).

    g_controller->on_focus_lost();
    if (g_preview != nullptr) {
        g_preview->hide();
    }
    if (g_engine == nullptr || g_session_slot == nullptr) {
        return;
    }
    // Follow settings changes made by the Suborno settings app, including

    // layout switches — the standalone host shares settings.ini with the

    // TSF IME and the desktop shell.

    (void)g_engine->reload_settings_if_changed();
    if (g_active_layout_id != nullptr && g_engine->active_layout_id() != *g_active_layout_id) {
        std::unique_ptr<suborno::layout::InputSession> replacement = g_engine->create_session();
        if (replacement != nullptr) {
            g_controller->set_session(*replacement);
            *g_session_slot = std::move(replacement);
            *g_active_layout_id = g_engine->active_layout_id();
        }
    }
}

void open_settings_app() {
    wchar_t module_path[MAX_PATH] = {};
    if (::GetModuleFileNameW(nullptr, module_path, MAX_PATH) == 0) {
        return;
    }
    const std::filesystem::path settings_exe =
        std::filesystem::path(module_path).parent_path() / L"Suborno.exe";
    if (!std::filesystem::exists(settings_exe)) {
        ::MessageBoxW(nullptr,
                      L"SubornoStandalone.exe-র পাশে Suborno.exe পাওয়া যায়নি। "
                      L"TSF IME ইনস্টল থাকলে সেটিংস অ্যাপ এখানে থাকবে।",
                      L"Suborno Standalone",
                      MB_OK | MB_ICONINFORMATION);
        return;
    }
    ::ShellExecuteW(nullptr, L"open", settings_exe.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

}  // namespace

int WINAPI wWinMain(_In_ HINSTANCE instance,
                    _In_opt_ HINSTANCE,
                    _In_ PWSTR command_line,
                    _In_ int show_command) {
    (void)command_line;
    (void)show_command;

    const HANDLE mutex = ::CreateMutexW(nullptr, TRUE, single_instance_mutex_name);
    if (mutex == nullptr) {
        return 1;
    }
    if (::GetLastError() == ERROR_ALREADY_EXISTS) {
        ::CloseHandle(mutex);
        return 0;
    }
    ::SetProcessDPIAware();

    // Shared user data: the standalone host reads/writes the same

    // settings.ini and dictionary the TSF IME uses — one engine, two shells.

    std::filesystem::path settings_path;
    std::filesystem::path dictionary_path;
    std::filesystem::path assets_path;
    std::unique_ptr<suborno::core::Engine> engine;
    static_cast<void>(suborno::platform::windows::get_shared_assets_directory(assets_path));
    if (SUCCEEDED(suborno::platform::windows::get_user_settings_path(settings_path))
        && SUCCEEDED(suborno::platform::windows::get_user_dictionary_path(dictionary_path))) {
        const suborno::core::EngineConfig config{settings_path, dictionary_path, assets_path};
        engine = std::make_unique<suborno::core::Engine>(config);
    } else {
        engine = std::make_unique<suborno::core::Engine>();
    }

    std::unique_ptr<suborno::layout::InputSession> session = engine->create_session();
    if (session == nullptr) {
        ::CloseHandle(mutex);
        return 1;
    }

    suborno::platform::windows::standalone::TypingController controller(*session);
    g_controller = &controller;
    g_engine = engine.get();
    g_session_slot = &session;
    g_active_layout_id = std::make_unique<std::wstring>(engine->active_layout_id());

    const std::filesystem::path ini_path = ini_path_from(settings_path);
    const bool preview_enabled = read_ini_bool(ini_path, L"preview", true);
    const bool enabled_on_start = read_ini_bool(ini_path, L"enabled_on_start", true);
    const std::wstring hotkey_spec = read_ini_string(ini_path, L"hotkey", default_hotkey);
    const suborno::platform::windows::standalone::KeyboardHookConfig hook_config =
        parse_hotkey(hotkey_spec);
    controller.set_enabled(enabled_on_start);

    suborno::platform::windows::standalone::PreviewWindow preview;
    g_preview = &preview;
    if (!preview.create(instance)) {
        ::CloseHandle(mutex);
        return 1;
    }

    suborno::platform::windows::standalone::StandaloneTray tray;
    suborno::platform::windows::standalone::StandaloneTray::Callbacks tray_callbacks;
    tray_callbacks.on_toggle = [&controller, &tray, &preview]() {
        controller.set_enabled(!controller.enabled());
        tray.set_enabled_state(controller.enabled());
        preview.hide();
    };
    tray_callbacks.on_open_settings = []() {
        open_settings_app();
    };
    tray_callbacks.on_exit = []() {
        ::PostQuitMessage(0);
    };

    const std::wstring tooltip = L"Suborno Standalone — বাংলা টাইপিং (Ctrl+Alt+B টগল)";
    if (!tray.create(instance, tooltip, tray_callbacks)) {
        ::CloseHandle(mutex);
        return 1;
    }
    tray.set_enabled_state(controller.enabled());

    suborno::platform::windows::standalone::KeyboardHook hook;
    suborno::platform::windows::standalone::KeyboardHook::Deps deps;
    deps.controller = &controller;
    deps.preview = &preview;
    deps.preview_enabled = preview_enabled;
    deps.on_engine_toggled = [&controller, &tray](bool) {
        tray.set_enabled_state(controller.enabled());
    };
    if (!hook.install(hook_config, deps)) {
        tray.destroy();
        ::CloseHandle(mutex);
        return 1;
    }

    const HWINEVENTHOOK event_hook = ::SetWinEventHook(EVENT_SYSTEM_FOREGROUND,
                                                       EVENT_SYSTEM_FOREGROUND,
                                                       nullptr,
                                                       &foreground_changed,
                                                       0,
                                                       0,
                                                       WINEVENT_OUTOFCONTEXT);

    MSG message = {};
    while (::GetMessageW(&message, nullptr, 0, 0) > 0) {
        ::TranslateMessage(&message);
        ::DispatchMessageW(&message);
    }

    if (event_hook != nullptr) {
        ::UnhookWinEvent(event_hook);
    }
    hook.uninstall();
    tray.destroy();
    preview.hide();
    g_controller = nullptr;
    g_preview = nullptr;
    g_engine = nullptr;
    g_session_slot = nullptr;
    g_active_layout_id.reset();
    ::ReleaseMutex(mutex);
    ::CloseHandle(mutex);
    return 0;
}
