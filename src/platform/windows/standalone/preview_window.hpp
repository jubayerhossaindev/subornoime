#pragma once

// Caret-following floating preview for the standalone typing host — the same
// UX Avro Keyboard's "Floating Preview Window" popularised: while the user
// types latin letters, the live Bangla conversion appears right at the text
// caret before the commit key is pressed. The window never steals focus
// (WS_EX_NOACTIVATE), so typing flow is uninterrupted.

#include <Windows.h>
#include <string>
#include <string_view>

#include "preview_font_choice.hpp"

namespace suborno::platform::windows::standalone {

class PreviewWindow final {
  public:
    PreviewWindow() = default;
    ~PreviewWindow();

    PreviewWindow(const PreviewWindow&) = delete;
    PreviewWindow& operator=(const PreviewWindow&) = delete;

    // The preview renders the live Bangla preedit, so its face must cover the
    // whole Bengali block. Callers resolve the family through the bundled
    // font policy (preview_font_choice.hpp) and pass it in; the window
    // deliberately does not decide fonts itself.
    bool create(HINSTANCE instance, const PreviewFontChoice& font_choice) noexcept;
    // Shows (or updates) the preview with the current preedit text, placed at
    // the caret of the focused application. Empty text hides the window.
    void show_text(std::string_view preedit_utf8) noexcept;
    void hide() noexcept;

  private:
    static LRESULT CALLBACK window_proc_thunk(HWND window,
                                              UINT message,
                                              WPARAM wparam,
                                              LPARAM lparam) noexcept;
    LRESULT window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) noexcept;

    HWND window_ = nullptr;
    HFONT font_ = nullptr;
    HBRUSH background_ = nullptr;
    HBRUSH border_ = nullptr;
    std::wstring text_;
};

}  // namespace suborno::platform::windows::standalone
