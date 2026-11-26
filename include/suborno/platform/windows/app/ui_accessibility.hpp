#pragma once

// UI Automation providers for the Suborno desktop shell.
//
// The Phase-01 audit's largest open accessibility item: every surface is
// custom-drawn with no child HWNDs, so screen readers see one opaque
// window. Accessible window names (an earlier pass) fixed the frame title;
// THIS module exposes the content itself as UI Automation elements —
// per-control names, roles, states, bounding rectangles, focus, and the
// Invoke / Toggle / ExpandCollapse / RangeValue patterns — driven by each
// window's own model (design rects, labels, staged state).
//
// Shape (the standard MS pattern for owner-drawn Win32 content):
//   * the window implements `ElementSource` — a plain C++ view of its
//     elements (no COM);
//   * on WM_GETOBJECT the window calls handle_get_object(), which returns
//     a ref-counted provider implementing IRawElementProviderSimple +
//     IRawElementProviderFragment(+Root) and, per element kind, the
//     matching control patterns;
//   * focus changes call notify_focus_changed() so Narrator announces the
//     newly focused control.
//
// Robustness rules:
//   * uiautomationcore.dll entry points are resolved dynamically — on a
//     system without UIA (or if the DLL refuses to load) WM_GETOBJECT
//     falls through to DefWindowProc and the default HWND accessibility
//     fallback still announces the window name;
//   * every provider holds the window's liveness token (a shared_ptr the
//     window resets on WM_DESTROY): a provider UIA still holds after the
//     window dies answers UIA_E_ELEMENTNOTAVAILABLE instead of touching
//     freed memory;
//   * all provider methods are exception barriers — UIA callbacks must
//     never let a C++ exception cross a COM boundary.
//
// The TSF candidate window is deliberately NOT converted: it already
// carries an accessible window name via the default fallback and it lives
// inside the protected TSF/COM surface.

#include <Windows.h>
#include <cstdint>
#include <memory>
#include <string>

namespace suborno::platform::windows::app::ui_access {

/// UIA control-type mapping for an element.
enum class ElementRole {
    window_root,
    button,
    checkbox,
    dropdown,  // ComboBox (ExpandCollapse)
    slider,  // RangeValue
    list_item,  // sidebar page entries
    text,  // section headers / status line (announced, not focusable)
};

/// One element of a surface, in the window's own coordinates. The window
/// fills this from its model; the provider converts to screen space and
/// maps roles/patterns onto UIA.
struct ElementInfo {
    std::wstring name;
    ElementRole role = ElementRole::text;
    RECT rect{
        0,
        0,
        0,
        0};  ///< client device pixels    /// Stable, developer-set selector (the winapp-ui-automation skill's
    /// preferred targeting form: survives layout changes and localization).
    /// Empty = the provider falls back to its positional id.
    std::wstring automation_id;
    /// Accessible description (UIA HelpText); empty = not provided.
    std::wstring help_text;
    /// Explicit collision-free runtime-ID discriminator within this surface.
    /// 0 = provider falls back to positional (index + 1).
    std::uint32_t runtime_id = 0;
    bool focusable = false;
    bool focused = false;
    /// Enabled state (UIA IsEnabled). Defaults to true so existing sources
    /// keep their behavior; sources with inactive items (disabled menu
    /// entries) report false so screen readers announce them as unavailable.
    bool enabled = true;
    /// Toggle pattern (checkbox).
    bool supports_toggle = false;
    bool toggle_state_on = false;
    /// RangeValue pattern (slider).
    bool supports_range = false;
    double range_min = 0.0;
    double range_max = 0.0;
    double range_value = 0.0;
    /// ExpandCollapse pattern (dropdown).
    bool supports_expand_collapse = false;
    bool expanded = false;
    /// Read-only Value pattern (dropdown selected label).
    bool supports_value = false;
    std::wstring value_text;
    /// Invoke pattern (buttons, OSK keys, topbar zones).
    bool supports_invoke = false;
};

/// The window-side half of the provider. Implementations must outlive every
/// WM_GETOBJECT they serve (they own the liveness token below).
class ElementSource {
  public:
    virtual ~ElementSource() = default;

    virtual std::wstring window_name() const = 0;
    virtual int element_count() const = 0;
    virtual bool element_info(int index, ElementInfo& out) const = 0;
    virtual int focused_element() const = 0;
    /// Monotonic generation incremented when fragment children change structurally
    /// (e.g. tab switch or page rebuild), invalidating prior child proxies.
    virtual std::uint64_t tree_generation() const {
        return 0;
    }

    // Actions; index is the element index. Return false when the window
    // cannot act (out of range, wrong kind, transient state).
    virtual bool focus_element(int index) = 0;
    virtual bool invoke_element(int index) = 0;
    virtual bool toggle_element(int index) = 0;
    virtual bool expand_element(int index, bool expand) = 0;
    virtual bool set_element_value(int index, double value) = 0;
};

/// WM_GETOBJECT helper. Returns the value the window procedure should
/// return and sets `handled` (always set it from the returned value).
LRESULT handle_get_object(HWND window,
                          WPARAM w_param,
                          LPARAM l_param,
                          ElementSource* source,
                          const std::shared_ptr<int>& alive_token,
                          bool& handled);

/// Raises the UIA focus-changed event for the currently focused element
/// (call after keyboard or mouse focus moves). No-op without UIA.
void notify_focus_changed(HWND window,
                          ElementSource* source,
                          const std::shared_ptr<int>& alive_token);

/// Raises UIA structure-changed (ChildrenInvalidated) on the window root
/// when the fragment subtree changes (e.g. tab switch or page rebuild).
void notify_structure_changed(HWND window,
                              ElementSource* source,
                              const std::shared_ptr<int>& alive_token);

}  // namespace suborno::platform::windows::app::ui_access
