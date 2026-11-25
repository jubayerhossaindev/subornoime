#pragma once

// Themed toast notifications for the Suborno desktop shell.
//
// The Phase-01 audit's UI inventory honestly listed "notification toasts"
// as not present — status changes (layout switched, encoding toggled,
// settings saved) were visible only through the tray tooltip or the
// settings status line, neither of which is on screen when the change is
// made from the tray menu or a hotkey. This component shows a small,
// frameless, theme-aware popup near the notification area that auto-
// dismisses; it never takes keyboard focus (WS_EX_NOACTIVATE) so typing
// and the TSF composition are never interrupted.
//
// Toasts use the shared token palette (surface_raised / separator / text
// plus the success/information/warning status colors with a text label, so
// color is never the only carrier of meaning). Errors deliberately keep
// using native MessageBox dialogs (documented decision) — toasts are for
// confirmations, not failures.

#include <Windows.h>

namespace suborno::platform::windows::app {

enum class ToastKind {
    success,  // enabled/applied states (theme_success token)
    information,  // neutral confirmations (theme_information token)
    warning,  // disabled/unavailable states (theme_warning token)
};

class ToastWindow final {
  public:
    ToastWindow();
    ~ToastWindow();
    ToastWindow(const ToastWindow&) = delete;
    ToastWindow& operator=(const ToastWindow&) = delete;

    /// The name is public because TU-local helpers in the implementation
    /// (measure, place_above_tray, paint_toast, the window proc) take
    /// ToastWindowState references — same pattern as TopbarWindowState.
    struct ToastWindowState;

    // Creates the hidden toast window. Repeated calls are a no-op; false
    // means the window could not be registered or created (toasts are
    // optional UI — callers proceed silently).
    bool create(HINSTANCE instance);

    // Destroys the window (called by the owner before its own teardown).
    void shutdown();

    // Shows one toast near the notification area, replacing any visible
    // one and restarting the auto-dismiss timer. `message` is the fully
    // expanded, localized text.
    void show(HWND owner, const wchar_t* message, ToastKind kind);

  private:
    ToastWindowState* window_state_ = nullptr;
};

}  // namespace suborno::platform::windows::app
