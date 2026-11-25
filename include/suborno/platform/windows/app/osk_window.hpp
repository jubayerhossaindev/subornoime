#pragma once

// On-Screen Keyboard window (Borno reference: a floating clickable keyboard
// with an English/Bangla mode pair). Clean-room Win32 implementation:
// clicking a key injects the REAL virtual key via SendInput, so the TSF
// text service processes it exactly like a physical keystroke —
// transliteration, candidates, learning and encoding all behave normally.
// Bangla mode renders each key with the glyph(s) the active layout produces
// for that letter, so the OSK is a live map of the current keyboard.

#include <Windows.h>
#include <functional>
#include <string>
#include <string_view>

namespace suborno::platform::windows::app {

class OskWindow final {
  public:
    struct Host {
        // The engine's active layout id ("english" vs a Bangla layout).
        std::function<std::string()> active_layout_id;
        // Mode switches from the [English] / [বাংলা] toggle buttons.
        std::function<void(bool english)> select_mode;
        // Human-readable name of the active layout (topbar/tray naming).
        std::function<std::wstring()> mode_display_name;
        // What the active Bangla layout produces for a typed letter
        // (e.g. "k" -> "ক"); empty when nothing maps.
        std::function<std::wstring(std::string_view letter)> bangla_preview;
    };

    OskWindow() = default;
    ~OskWindow();
    OskWindow(const OskWindow&) = delete;
    OskWindow& operator=(const OskWindow&) = delete;

    bool create(HINSTANCE instance, Host host);
    void destroy();
    void show();
    void hide();
    bool visible() const;
    // Re-reads the active mode/layout and repaints key labels; the shell
    // calls it whenever the layout changes from any other surface.
    void refresh_labels();
    // Drops the cached key fonts so the next paint rebuilds them from the
    // process-wide UI font family (settings font picker changed); the shell
    // calls it together with Topbar::refresh_font().
    void refresh_font();

  public:
    /// Incomplete implementation handle; osk_window.cpp completes it.
    struct OskWindowState;

  private:
    OskWindowState* window_state_ = nullptr;
};

}  // namespace suborno::platform::windows::app
