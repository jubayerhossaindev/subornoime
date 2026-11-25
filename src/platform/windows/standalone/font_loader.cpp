#include "font_loader.hpp"

#include <filesystem>
#include <set>
#include <string>

namespace suborno::platform::windows::standalone {
namespace {

// Vendored file-name prefixes -> which BundledFontState flag they satisfy.
// The preview policy needs the Hind Siliguri Regular/Medium faces; Noto Sans
// Bengali is recorded for future surfaces (candidate list options) and the
// Baloo Da 2 display faces are brand-only, so they carry no flag.
bool note_family(const std::wstring& file_name, BundledFontState& state) {
    bool known = false;
    if (file_name.rfind(L"HindSiliguri-", 0) == 0) {
        known = true;
        if (file_name.find(L"-Regular.ttf") != std::wstring::npos) {
            state.hind_siliguri_regular = true;
        }
        if (file_name.find(L"-Medium.ttf") != std::wstring::npos) {
            state.hind_siliguri_medium = true;
        }
    } else if (file_name.rfind(L"NotoSansBengali-", 0) == 0) {
        known = true;
        state.noto_sans_bengali = true;
    } else if (file_name.rfind(L"BalooDa2-", 0) == 0) {
        known = true;
    }
    return known;
}

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

}  // namespace

int BundledFonts::load_directory(const wchar_t* directory) noexcept {
    int loaded = 0;
    std::error_code error;
    const std::filesystem::path dir(directory);
    if (!std::filesystem::is_directory(dir, error)) {
        return 0;
    }
    // Deterministic order: face names must not depend on directory iteration
    // order (addfont order decides nothing, but deterministic logging does).
    std::set<std::wstring> files;
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::directory_iterator(dir, error)) {
        if (entry.path().extension() == L".ttf") {
            files.insert(entry.path().filename().wstring());
        }
    }
    for (const std::wstring& name : files) {
        const std::filesystem::path full = dir / name;
        // FR_PRIVATE: process-scoped registration; nothing is written to
        // HKLM/HKCU font tables and no other application can see the face.
        const UINT added = ::AddFontResourceExW(full.c_str(), FR_PRIVATE, nullptr);
        if (added > 0) {
            note_family(name, state_);
            ++loaded;
        }
    }
    return loaded;
}

bool BundledFonts::load() noexcept {
    state_ = BundledFontState{};
    wchar_t module_path[MAX_PATH] = {};
    if (::GetModuleFileNameW(nullptr, module_path, MAX_PATH) == 0) {
        return false;
    }
    const std::filesystem::path exe_dir = std::filesystem::path(module_path).parent_path();

    // Probe 1: fonts next to the exe (dev POST_BUILD copy; also the layout
    // used by a user who unzips the portable build and runs it in place).
    // Probe 2: %LOCALAPPDATA%\Suborno\fonts for bin\SubornoStandalone.exe
    // (the layout install_standalone.cmd creates).
    int loaded = load_directory((exe_dir / L"fonts").c_str());
    if (exe_dir.has_parent_path()) {
        loaded += load_directory((exe_dir.parent_path() / L"fonts").c_str());
    }
    state_.files_loaded = loaded;

    // Deduplicate double-registration when both probes resolve to the same
    // directory is unnecessary: AddFontResourceExW is idempotent per face in
    // one process, and load_directory only ran the second probe's files once.
    return loaded > 0;
}

PreviewFontChoice resolve_preview_font_from_settings(const BundledFontState& state,
                                                     bool use_os_default_bangla_font,
                                                     const std::string& family_utf8) noexcept {
    // Same precedence as the TSF candidate list
    // (src/platform/windows/tsf/text_service.cpp): use_os_default_bangla_font
    // or an empty family means "no user override", so the bundled brand font
    // applies. A user-picked family beats it — settings.ui.font_family is
    // validated by the settings loader, and GDI ignores unknown names, so a
    // stale value can never break rendering (the policy still falls back to
    // Nirmala UI when GDI substitutes).
    std::wstring user_family;
    if (!use_os_default_bangla_font) {
        user_family = utf8_to_utf16(family_utf8);
    }
    return resolve_preview_font(state, user_family);
}

}  // namespace suborno::platform::windows::standalone
