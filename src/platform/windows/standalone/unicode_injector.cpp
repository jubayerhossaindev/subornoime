#include "unicode_injector.hpp"

#include <Windows.h>
#include <string>
#include <vector>

namespace suborno::platform::windows::standalone::injector {
namespace {

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

void flush_inputs(const std::vector<INPUT>& inputs) {
    if (inputs.empty()) {
        return;
    }
    const UINT sent = ::SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
    if (sent != inputs.size()) {
        ::OutputDebugStringW(L"Suborno standalone: SendInput delivered fewer events than built\n");
    }
}

}  // namespace

void send_backspaces(std::size_t count) {
    if (count == 0) {
        return;
    }
    std::vector<INPUT> inputs;
    inputs.reserve(count * 2);
    for (std::size_t index = 0; index < count; ++index) {
        INPUT down = {};
        down.type = INPUT_KEYBOARD;
        down.ki.wVk = VK_BACK;
        inputs.push_back(down);

        INPUT up = {};
        up.type = INPUT_KEYBOARD;
        up.ki.wVk = VK_BACK;
        up.ki.dwFlags = KEYEVENTF_KEYUP;
        inputs.push_back(up);
    }
    flush_inputs(inputs);
}

void send_text_utf8(std::string_view text_utf8) {
    const std::wstring wide = utf8_to_utf16(text_utf8);
    if (wide.empty()) {
        return;
    }
    std::vector<INPUT> inputs;
    inputs.reserve(wide.size() * 2);
    for (const wchar_t unit : wide) {
        INPUT down = {};
        down.type = INPUT_KEYBOARD;
        down.ki.wScan = static_cast<WORD>(unit);
        down.ki.dwFlags = KEYEVENTF_UNICODE;
        inputs.push_back(down);

        INPUT up = {};
        up.type = INPUT_KEYBOARD;
        up.ki.wScan = static_cast<WORD>(unit);
        up.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
        inputs.push_back(up);
    }
    flush_inputs(inputs);
}

void send_virtual_key(std::uint16_t virtual_key) {
    INPUT down = {};
    down.type = INPUT_KEYBOARD;
    down.ki.wVk = static_cast<WORD>(virtual_key);

    INPUT up = {};
    up.type = INPUT_KEYBOARD;
    up.ki.wVk = static_cast<WORD>(virtual_key);
    up.ki.dwFlags = KEYEVENTF_KEYUP;

    const INPUT pair[2] = {down, up};
    const UINT sent = ::SendInput(2, pair, sizeof(INPUT));
    if (sent != 2) {
        ::OutputDebugStringW(L"Suborno standalone: SendInput delivered fewer events than built\n");
    }
}

}  // namespace suborno::platform::windows::standalone::injector
