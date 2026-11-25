#pragma once

// Floating desktop topbar pill (reference screenshots 5-7, 22): a small
// always-on-top bar showing the active input mode with three click zones —
// mode icon (English↔বাংলা quick toggle), label (keyboard-layout menu),
// wrench (settings/about), and close (switch to tray / exit). Right-click
// opens the full desktop context menu. Draggable; dims after system-wide
// idle when enabled in settings.

#include <Windows.h>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace suborno::platform::windows::app {

/// The subset of topbar settings the shell consumes at runtime.
struct TopbarSettingsSnapshot {
    bool idle_dimming_enabled = true;
    std::uint32_t idle_opacity_percent = 50;
    std::uint32_t idle_time_seconds = 10;
    bool remember_position = true;
};

class Topbar final {
  public:
    struct Host {
        std::function<std::string()> active_layout_id;
        std::function<std::vector<std::pair<std::string, std::wstring>>()> layouts;
        std::function<void(const std::string&)> select_layout;
        // Quick English↔Bangla mode switch (the mode-icon zone): toggles
        // between the English passthrough layout and the last Bangla layout.
        std::function<void()> toggle_english_bangla;
        // Right-click anywhere on the pill: the full desktop context menu
        // (layouts, font, OSK, settings, enable/disable, updates, about,
        // tray, exit) built by the application, in screen coordinates.
        std::function<void(POINT)> show_context_menu;
        std::function<void()> open_settings;
        std::function<void()> open_about;
        std::function<void()> open_osk;
        // Font picker (wrench menu): the installed Bangla-capable families,
        // the active family (empty = OS default), and the change handler.
        std::function<std::vector<std::wstring>()> fonts;
        std::function<std::wstring()> active_font;
        std::function<void(const std::wstring&)> select_font;
        // Position memory: the last dragged position (empty = none) and the
        // saver invoked after the user releases a drag.
        std::function<std::optional<POINT>()> saved_topbar_position;
        std::function<void(int, int)> save_topbar_position;
        std::function<void()> switch_to_tray;
        std::function<void()> exit_application;
    };

    Topbar() = default;
    ~Topbar();
    Topbar(const Topbar&) = delete;
    Topbar& operator=(const Topbar&) = delete;

    bool create(HINSTANCE instance, Host host);
    void destroy();
    void show();
    void hide();
    bool visible() const;
    void apply_settings(const TopbarSettingsSnapshot& settings);
    void refresh_layout_label();
    // Recreates the label font from the process-wide UI font family and
    // re-measures the pill so a longer/shorter label still fits.
    void refresh_font();

  public:
    /// Incomplete implementation handle; topbar.cpp completes the definition.
    /// The name is public because TU-local helpers there take TopbarWindowState references.
    struct TopbarWindowState;

  private:
    TopbarWindowState* window_state_ = nullptr;
};

}  // namespace suborno::platform::windows::app
