#pragma once

// Bundled-font loader for the standalone typing host.
//
// WHY THIS EXISTS
// The zero-dependency promise ("install -> run -> type Bangla anywhere") also
// covers how Suborno's own UI *looks*. Shipping the OFL-licensed Bangla faces
// (Hind Siliguri, Baloo Da 2, Noto Sans Bengali) in assets/fonts and loading
// them per-process means the floating preview renders in the brand face on
// any Windows 8+ machine — no font install, no admin, no Bengali language
// pack, and nothing is written to the system font tables.
//
// HOW THE LOADING WORKS
// AddFontResourceExW(path, FR_PRIVATE, nullptr) registers each TTF for THIS
// process only: GDI font matching (CreateFontW) resolves the bundled family
// names inside SubornoStandalone.exe, while every other application is
// untouched. The registration lasts until RemoveFontResourceExW or process
// exit — there is deliberately no cleanup step: the process-exit teardown is
// the whole uninstall story.
//
// SYSTEM-WIDE (OPTIONAL) USE
// install_fonts.cmd registers the same files as per-user fonts
// (HKCU ...\Fonts, Win10 1809+). That makes the families visible to the TSF
// IME's candidate list and the settings font picker in every process of this
// user. The standalone host does not require it — FR_PRIVATE wins.

#include <Windows.h>

#include "preview_font_choice.hpp"

namespace suborno::platform::windows::standalone {

// Loads every .ttf shipped next to the executable. Search order:
//   1. <exe_dir>\fonts          (dev build: CMake POST_BUILD copy;
//                                 installed layout: %LOCALAPPDATA%\Suborno\fonts
//                                 lives next to bin\, reached via probe 2)
//   2. <exe_dir>\..\fonts       (install layout from install_standalone.cmd)
// Returns true when at least one file registered; callers must treat a false
// return as "policy falls back to Nirmala UI", not as an error.
class BundledFonts final {
  public:
    BundledFonts() = default;

    bool load() noexcept;

    const BundledFontState& state() const noexcept {
        return state_;
    }

  private:
    int load_directory(const wchar_t* directory) noexcept;
    BundledFontState state_;
};

// Settings-aware wrapper: converts the shared settings value (UTF-8) and
// applies the same rule the TSF candidate list uses — a user's explicit
// family choice (settings.ui.font_family, with use_os_default_bangla_font
// off) beats the bundled brand font; anything else resolves through the
// policy in preview_font_choice.hpp.
PreviewFontChoice resolve_preview_font_from_settings(const BundledFontState& state,
                                                     bool use_os_default_bangla_font,
                                                     const std::string& family_utf8) noexcept;

}  // namespace suborno::platform::windows::standalone
