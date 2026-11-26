#pragma once

// Custom-drawn dark popup menu / dropdown list, matching the reference
// screenshots (uniform dark surface, rounded corners, hover pill, radio
// dots, checkmarks, separators). Clean-room implementation; native
// Win32 popup menus stay light-themed unless an app manifest opts into
// dark mode, so the shell draws its own menus to match the reference.

#include <Windows.h>
#include <functional>
#include <string>
#include <vector>

namespace suborno::platform::windows::app {

struct MenuItem {
    std::wstring label;
    bool separator = false;
    bool checked = false;  ///< checkmark (toggle items)
    bool radio = false;  ///< radio dot (single-choice items)
    bool enabled = true;
};

// Shows a modal popup list anchored near a screen point and returns the
// index of the chosen item, or -1 when dismissed. `selected` marks the
// pre-selected/highlighted row (-1 for none). Blocks until dismissed.
int show_menu_popup(HWND owner, const std::vector<MenuItem>& items, POINT anchor, int selected);

// Opens a modeless popup list anchored near `anchor` and returns its window
// handle (or nullptr on failure). `on_close` is called once with the chosen
// item index (or -1 if dismissed) when the popup closes.
HWND open_menu_popup(HWND owner,
                     const std::vector<MenuItem>& items,
                     POINT anchor,
                     int selected,
                     std::function<void(int)> on_close,
                     bool focus_popup = true);

// Programmatically dismisses a modeless popup opened by open_menu_popup().
void close_menu_popup(HWND popup);

}  // namespace suborno::platform::windows::app
