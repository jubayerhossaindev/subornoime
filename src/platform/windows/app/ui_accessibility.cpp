// UI Automation providers for the Suborno desktop shell. See the header
// for the contract and robustness rules. The COM object follows the
// standard fragment pattern: one class serves the window root (fragment
// root + simple provider with a host provider from the HWND) and its
// elements (fragments with the Invoke/Toggle/ExpandCollapse/RangeValue
// patterns the window declares per element).

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "suborno/platform/windows/app/ui_accessibility.hpp"

#include <algorithm>
#include <atomic>
#include <memory>
#include <new>
#include <string>
#include <uiautomation.h>

#include "../com/boundary_guard.hpp"

namespace suborno::platform::windows::app::ui_access {
namespace {

// uiautomationcore.dll entry points, resolved once. Dynamic loading keeps
// the loader free of UIA imports: a system without UIA simply answers
// WM_GETOBJECT through DefWindowProc (the default HWND fallback still
// announces window names).
struct UiaFunctions {
    using ReturnRawFn = LRESULT(WINAPI*)(HWND, WPARAM, LPARAM, IRawElementProviderSimple*);
    using HostProviderFn = HRESULT(WINAPI*)(HWND, IRawElementProviderSimple**);
    using RaiseEventFn = HRESULT(WINAPI*)(IRawElementProviderSimple*, EVENTID);

    HMODULE module = nullptr;
    ReturnRawFn return_raw = nullptr;
    HostProviderFn host_provider = nullptr;
    RaiseEventFn raise_event = nullptr;
};

const UiaFunctions& uia_functions() {
    static const UiaFunctions functions = [] {
        UiaFunctions resolved;
        resolved.module = ::LoadLibraryW(L"uiautomationcore.dll");
        if (resolved.module != nullptr) {
            resolved.return_raw = reinterpret_cast<UiaFunctions::ReturnRawFn>(
                ::GetProcAddress(resolved.module, "UiaReturnRawElementProvider"));
            resolved.host_provider = reinterpret_cast<UiaFunctions::HostProviderFn>(
                ::GetProcAddress(resolved.module, "UiaHostProviderFromHwnd"));
            resolved.raise_event = reinterpret_cast<UiaFunctions::RaiseEventFn>(
                ::GetProcAddress(resolved.module, "UiaRaiseAutomationEvent"));
        }
        return resolved;
    }();
    return functions;
}

// Returns true when the entry points this module needs exist.
bool uia_available() {
    const UiaFunctions& functions = uia_functions();
    return functions.return_raw != nullptr && functions.host_provider != nullptr
           && functions.raise_event != nullptr;
}

LONG control_type_for(ElementRole role) noexcept {
    switch (role) {
    case ElementRole::window_root:
        return UIA_WindowControlTypeId;
    case ElementRole::button:
        return UIA_ButtonControlTypeId;
    case ElementRole::checkbox:
        return UIA_CheckBoxControlTypeId;
    case ElementRole::dropdown:
        return UIA_ComboBoxControlTypeId;
    case ElementRole::slider:
        return UIA_SliderControlTypeId;
    case ElementRole::list_item:
        return UIA_ListItemControlTypeId;
    case ElementRole::text:
        break;
    }
    return UIA_TextControlTypeId;
}

void variant_clear_init(VARIANT& variant) noexcept {
    ::VariantInit(&variant);
}

// The ref-counted provider. One instance either serves the window root or
// one element of the source (is_root + index).
class ElementProvider final : public IRawElementProviderSimple,
                              public IRawElementProviderFragment,
                              public IRawElementProviderFragmentRoot,
                              public IInvokeProvider,
                              public IToggleProvider,
                              public IExpandCollapseProvider,
                              public IRangeValueProvider {
  public:
    ElementProvider(
        HWND window, ElementSource* source, std::weak_ptr<int> alive_token, bool is_root, int index)
        : window_(window),
          source_(source),
          alive_(std::move(alive_token)),
          is_root_(is_root),
          index_(index),
          reference_count_(1) {}

    // ---- IUnknown ----

    IFACEMETHODIMP QueryInterface(REFIID riid, void** object) override {
        if (object == nullptr) {
            return run_inside_com_boundary([&]() -> HRESULT {
                return E_POINTER;
            });
        }
        *object = nullptr;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IRawElementProviderSimple)) {
            *object = static_cast<IRawElementProviderSimple*>(this);
        } else if (riid == __uuidof(IRawElementProviderFragment)) {
            *object = static_cast<IRawElementProviderFragment*>(this);
        } else if (is_root_ && riid == __uuidof(IRawElementProviderFragmentRoot)) {
            *object = static_cast<IRawElementProviderFragmentRoot*>(this);
        } else if (!is_root_ && riid == __uuidof(IInvokeProvider)) {
            *object = static_cast<IInvokeProvider*>(this);
        } else if (!is_root_ && riid == __uuidof(IToggleProvider)) {
            *object = static_cast<IToggleProvider*>(this);
        } else if (!is_root_ && riid == __uuidof(IExpandCollapseProvider)) {
            *object = static_cast<IExpandCollapseProvider*>(this);
        } else if (!is_root_ && riid == __uuidof(IRangeValueProvider)) {
            *object = static_cast<IRangeValueProvider*>(this);
        }
        if (*object == nullptr) {
            return E_NOINTERFACE;
        }
        AddRef();
        return S_OK;
    }

    IFACEMETHODIMP_(ULONG) AddRef() override {
        return static_cast<ULONG>(reference_count_.fetch_add(1, std::memory_order_relaxed) + 1);
    }

    IFACEMETHODIMP_(ULONG) Release() override {
        const LONG remaining = reference_count_.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (remaining == 0) {
            delete this;
        }
        return static_cast<ULONG>(remaining);
    }

    // ---- IRawElementProviderSimple ----

    IFACEMETHODIMP get_ProviderOptions(enum ProviderOptions* options) override {
        if (options == nullptr) {
            return run_inside_com_boundary([&]() -> HRESULT {
                return E_POINTER;
            });
        }
        *options = ProviderOptions_ServerSideProvider;
        return S_OK;
    }

    IFACEMETHODIMP get_HostRawElementProvider(IRawElementProviderSimple** host) override {
        if (host == nullptr) {
            return run_inside_com_boundary([&]() -> HRESULT {
                return E_POINTER;
            });
        }
        *host = nullptr;
        if (!available()) {
            return UIA_E_ELEMENTNOTAVAILABLE;
        }
        // Only the window root bridges to the HWND's default provider;
        // fragment children navigate through this class instead.
        if (!is_root_) {
            return S_OK;
        }
        const UiaFunctions& functions = uia_functions();
        if (functions.host_provider == nullptr) {
            return S_OK;
        }
        return functions.host_provider(window_, host);
    }

    IFACEMETHODIMP GetPatternProvider(PATTERNID pattern_id, IUnknown** pattern) override {
        if (pattern == nullptr) {
            return run_inside_com_boundary([&]() -> HRESULT {
                return E_POINTER;
            });
        }
        *pattern = nullptr;
        if (is_root_ || !available()) {
            return S_OK;
        }
        ElementInfo info{};
        if (!source_->element_info(index_, info)) {
            return UIA_E_ELEMENTNOTAVAILABLE;
        }
        IUnknown* matched = nullptr;
        if (pattern_id == UIA_InvokePatternId && info.supports_invoke) {
            matched = static_cast<IInvokeProvider*>(this);
        } else if (pattern_id == UIA_TogglePatternId && info.supports_toggle) {
            matched = static_cast<IToggleProvider*>(this);
        } else if (pattern_id == UIA_ExpandCollapsePatternId && info.supports_expand_collapse) {
            matched = static_cast<IExpandCollapseProvider*>(this);
        } else if (pattern_id == UIA_RangeValuePatternId && info.supports_range) {
            matched = static_cast<IRangeValueProvider*>(this);
        }
        if (matched != nullptr) {
            matched->AddRef();
            *pattern = matched;
        }
        return S_OK;
    }

    IFACEMETHODIMP GetPropertyValue(PROPERTYID property_id, VARIANT* value) override {
        if (value == nullptr) {
            return run_inside_com_boundary([&]() -> HRESULT {
                return E_POINTER;
            });
        }
        variant_clear_init(*value);
        if (!available()) {
            return UIA_E_ELEMENTNOTAVAILABLE;
        }
        try {
            switch (property_id) {
            case UIA_NamePropertyId: {
                const std::wstring name = is_root_ ? source_->window_name() : element_name();
                if ((value->bstrVal =
                         ::SysAllocStringLen(name.c_str(), static_cast<UINT>(name.size())))
                    == nullptr) {
                    return E_OUTOFMEMORY;
                }
                value->vt = VT_BSTR;
                return S_OK;
            }
            case UIA_AutomationIdPropertyId: {
                // Prefer the element's stable, developer-set id; the
                // positional form is only the fallback.
                std::wstring id;
                if (is_root_) {
                    id = L"window";
                } else {
                    ElementInfo info{};
                    if (source_->element_info(index_, info) && !info.automation_id.empty()) {
                        id = info.automation_id;
                    } else {
                        wchar_t positional[32]{};
                        swprintf_s(positional, _countof(positional), L"element-%d", index_);
                        id = positional;
                    }
                }
                if ((value->bstrVal = ::SysAllocStringLen(id.c_str(), static_cast<UINT>(id.size())))
                    == nullptr) {
                    return E_OUTOFMEMORY;
                }
                value->vt = VT_BSTR;
                return S_OK;
            }
            case UIA_HelpTextPropertyId: {
                ElementInfo info{};
                if (is_root_ || !source_->element_info(index_, info) || info.help_text.empty()) {
                    return S_OK;  // empty VARIANT: no HelpText override
                }
                if ((value->bstrVal = ::SysAllocStringLen(info.help_text.c_str(),
                                                          static_cast<UINT>(info.help_text.size())))
                    == nullptr) {
                    return E_OUTOFMEMORY;
                }
                value->vt = VT_BSTR;
                return S_OK;
            }
            case UIA_ControlTypePropertyId:
                value->vt = VT_I4;
                value->lVal = is_root_ ? UIA_WindowControlTypeId : control_type_for(element_role());
                return S_OK;
            case UIA_IsControlElementPropertyId:
            case UIA_IsContentElementPropertyId:
                value->vt = VT_BOOL;
                value->boolVal = VARIANT_TRUE;
                return S_OK;
            case UIA_IsEnabledPropertyId: {
                value->vt = VT_BOOL;
                if (is_root_) {
                    value->boolVal = VARIANT_TRUE;
                    return S_OK;
                }
                ElementInfo info{};
                value->boolVal = (source_->element_info(index_, info) && info.enabled)
                                     ? VARIANT_TRUE
                                     : VARIANT_FALSE;
                return S_OK;
            }
            case UIA_IsOffscreenPropertyId:
                value->vt = VT_BOOL;
                value->boolVal = VARIANT_FALSE;
                return S_OK;
            case UIA_FrameworkIdPropertyId:
                if ((value->bstrVal = ::SysAllocString(L"Suborno")) == nullptr) {
                    return E_OUTOFMEMORY;
                }
                value->vt = VT_BSTR;
                return S_OK;
            case UIA_ProcessIdPropertyId:
                value->vt = VT_I4;
                value->lVal = static_cast<LONG>(::GetCurrentProcessId());
                return S_OK;
            case UIA_IsKeyboardFocusablePropertyId:
                value->vt = VT_BOOL;
                value->boolVal = (is_root_ || element_focusable()) ? VARIANT_TRUE : VARIANT_FALSE;
                return S_OK;
            case UIA_HasKeyboardFocusPropertyId:
                value->vt = VT_BOOL;
                value->boolVal = element_has_focus() ? VARIANT_TRUE : VARIANT_FALSE;
                return S_OK;
            case UIA_BoundingRectanglePropertyId: {
                UiaRect rect{};
                if (FAILED(screen_rect(rect))) {
                    return S_OK;  // empty variant: UIA treats it as no rectangle
                }
                SAFEARRAY* array = ::SafeArrayCreateVector(VT_R8, 0, 4);
                if (array == nullptr) {
                    return E_OUTOFMEMORY;
                }
                double values[4] = {rect.left, rect.top, rect.width, rect.height};
                for (long index = 0; index < 4; ++index) {
                    if (FAILED(::SafeArrayPutElement(array, &index, &values[index]))) {
                        ::SafeArrayDestroy(array);
                        return E_FAIL;
                    }
                }
                value->vt = VT_ARRAY | VT_R8;
                value->parray = array;
                return S_OK;
            }
            case UIA_RangeValueIsReadOnlyPropertyId:
                if (!is_root_ && element_role() == ElementRole::slider) {
                    value->vt = VT_BOOL;
                    value->boolVal = VARIANT_FALSE;
                }
                return S_OK;
            case UIA_RangeValueValuePropertyId: {
                ElementInfo info{};
                if (is_root_ || !source_->element_info(index_, info) || !info.supports_range) {
                    return S_OK;
                }
                value->vt = VT_R8;
                value->dblVal = info.range_value;
                return S_OK;
            }
            case UIA_RangeValueMinimumPropertyId: {
                ElementInfo info{};
                if (is_root_ || !source_->element_info(index_, info) || !info.supports_range) {
                    return S_OK;
                }
                value->vt = VT_R8;
                value->dblVal = info.range_min;
                return S_OK;
            }
            case UIA_RangeValueMaximumPropertyId: {
                ElementInfo info{};
                if (is_root_ || !source_->element_info(index_, info) || !info.supports_range) {
                    return S_OK;
                }
                value->vt = VT_R8;
                value->dblVal = info.range_max;
                return S_OK;
            }
            case UIA_RangeValueSmallChangePropertyId:
                if (!is_root_ && element_role() == ElementRole::slider) {
                    value->vt = VT_R8;
                    value->dblVal = 1.0;
                }
                return S_OK;
            case UIA_RangeValueLargeChangePropertyId:
                if (!is_root_ && element_role() == ElementRole::slider) {
                    value->vt = VT_R8;
                    value->dblVal = 10.0;
                }
                return S_OK;
            default:
                return S_OK;  // empty VARIANT: property not overridden
            }
        } catch (const std::bad_alloc&) {
            return E_OUTOFMEMORY;
        } catch (...) {
            return E_FAIL;
        }
    }

    // ---- IRawElementProviderFragment ----

    IFACEMETHODIMP Navigate(enum NavigateDirection direction,
                            IRawElementProviderFragment** target) override {
        if (target == nullptr) {
            return run_inside_com_boundary([&]() -> HRESULT {
                return E_POINTER;
            });
        }
        *target = nullptr;
        if (!available()) {
            return UIA_E_ELEMENTNOTAVAILABLE;
        }
        try {
            if (is_root_) {
                switch (direction) {
                case NavigateDirection_FirstChild: {
                    const int first = next_exposed_index(0, +1);
                    if (first >= 0) {
                        *target = make_child_provider(first).release();
                    }
                    break;
                }
                case NavigateDirection_LastChild: {
                    const int last = next_exposed_index(source_->element_count() - 1, -1);
                    if (last >= 0) {
                        *target = make_child_provider(last).release();
                    }
                    break;
                }
                default:
                    break;  // the fragment root has no parent or siblings
                }
                return S_OK;
            }
            switch (direction) {
            case NavigateDirection_Parent:
                *target = make_root_provider().release();
                break;
            case NavigateDirection_NextSibling: {
                const int next = next_exposed_index(index_ + 1, +1);
                if (next >= 0) {
                    *target = make_child_provider(next).release();
                }
                break;
            }
            case NavigateDirection_PreviousSibling: {
                const int previous = next_exposed_index(index_ - 1, -1);
                if (previous >= 0) {
                    *target = make_child_provider(previous).release();
                }
                break;
            }
            default:
                break;  // elements have no children
            }
            return S_OK;
        } catch (const std::bad_alloc&) {
            return E_OUTOFMEMORY;
        } catch (...) {
            return E_FAIL;
        }
    }

    IFACEMETHODIMP GetRuntimeId(SAFEARRAY** runtime_id) override {
        if (runtime_id == nullptr) {
            return run_inside_com_boundary([&]() -> HRESULT {
                return E_POINTER;
            });
        }
        *runtime_id = nullptr;
        if (!available()) {
            return UIA_E_ELEMENTNOTAVAILABLE;
        }
        // This provider fragment is rooted directly in an HWND. Per the
        // UIA contract, a top-level element hosted by a window must return a
        // null runtime ID; the host provider supplies the HWND identity.
        // Only fragment children append a provider-defined ID.
        if (is_root_) {
            return S_OK;
        }

        // Child IDs are unique within this fragment. UI Automation scopes
        // UiaAppendRuntimeId across provider instances.
        const LONG values[] = {UiaAppendRuntimeId, static_cast<LONG>(index_ + 1)};
        SAFEARRAY* array =
            ::SafeArrayCreateVector(VT_I4, 0, static_cast<ULONG>(_countof(values)));
        if (array == nullptr) {
            return E_OUTOFMEMORY;
        }
        for (LONG index = 0; index < static_cast<LONG>(_countof(values)); ++index) {
            if (FAILED(::SafeArrayPutElement(array, &index, &values[index]))) {
                ::SafeArrayDestroy(array);
                return E_FAIL;
            }
        }
        *runtime_id = array;
        return S_OK;
    }

    IFACEMETHODIMP get_BoundingRectangle(UiaRect* rect) override {
        if (rect == nullptr) {
            return run_inside_com_boundary([&]() -> HRESULT {
                return E_POINTER;
            });
        }
        if (!available()) {
            return UIA_E_ELEMENTNOTAVAILABLE;
        }
        return screen_rect(*rect);
    }

    IFACEMETHODIMP GetEmbeddedFragmentRoots(SAFEARRAY** roots) override {
        if (roots == nullptr) {
            return run_inside_com_boundary([&]() -> HRESULT {
                return E_POINTER;
            });
        }
        // This flat custom-drawn surface does not host another UIA
        // framework. The documented result for that case is a null array.
        *roots = nullptr;
        return available() ? S_OK : UIA_E_ELEMENTNOTAVAILABLE;
    }

    IFACEMETHODIMP SetFocus() override {
        if (!available()) {
            return run_inside_com_boundary([&]() -> HRESULT {
                return UIA_E_ELEMENTNOTAVAILABLE;
            });
        }
        if (is_root_) {
            ::SetFocus(window_);
            return S_OK;
        }
        if (!source_->focus_element(index_)) {
            return E_INVALIDARG;
        }
        notify_focus_changed(window_, source_, alive_.lock());
        return S_OK;
    }

    IFACEMETHODIMP get_FragmentRoot(IRawElementProviderFragmentRoot** root) override {
        if (root == nullptr) {
            return run_inside_com_boundary([&]() -> HRESULT {
                return E_POINTER;
            });
        }
        *root = nullptr;
        if (!available()) {
            return UIA_E_ELEMENTNOTAVAILABLE;
        }
        if (is_root_) {
            *root = this;
            AddRef();
            return S_OK;
        }
        std::unique_ptr<ElementProvider> parent = make_root_provider();
        if (FAILED(parent->QueryInterface(__uuidof(IRawElementProviderFragmentRoot),
                                          reinterpret_cast<void**>(root)))) {
            return E_FAIL;
        }
        return S_OK;
    }

    // ---- IRawElementProviderFragmentRoot (window root only) ----

    IFACEMETHODIMP ElementProviderFromPoint(double x,
                                            double y,
                                            IRawElementProviderFragment** target) override {
        if (target == nullptr) {
            return run_inside_com_boundary([&]() -> HRESULT {
                return E_POINTER;
            });
        }
        *target = nullptr;
        if (!available()) {
            return UIA_E_ELEMENTNOTAVAILABLE;
        }
        try {
            // Screen -> client pixels, then hit-test the element rects in
            // declaration order (the last matching element wins, mirroring
            // the paint order: later rows draw on top).
            POINT origin{0, 0};
            ::ClientToScreen(window_, &origin);
            const int client_x = static_cast<int>(x) - origin.x;
            const int client_y = static_cast<int>(y) - origin.y;
            int matched = -1;
            const int count = source_->element_count();
            for (int index = 0; index < count; ++index) {
                ElementInfo info{};
                if (!source_->element_info(index, info)) {
                    continue;
                }
                if (client_x >= info.rect.left && client_x < info.rect.right
                    && client_y >= info.rect.top && client_y < info.rect.bottom) {
                    matched = index;
                }
            }
            if (matched < 0) {
                *target = static_cast<IRawElementProviderFragment*>(this);
                AddRef();
                return S_OK;
            }
            *target = make_child_provider(matched).release();
            return S_OK;
        } catch (const std::bad_alloc&) {
            return E_OUTOFMEMORY;
        } catch (...) {
            return E_FAIL;
        }
    }

    IFACEMETHODIMP GetFocus(IRawElementProviderFragment** focused) override {
        if (focused == nullptr) {
            return run_inside_com_boundary([&]() -> HRESULT {
                return E_POINTER;
            });
        }
        *focused = nullptr;
        if (!available()) {
            return UIA_E_ELEMENTNOTAVAILABLE;
        }
        try {
            const int focus = source_->focused_element();
            if (focus >= 0) {
                *focused = make_child_provider(focus).release();
                return S_OK;
            }
            *focused = static_cast<IRawElementProviderFragment*>(this);
            AddRef();
            return S_OK;
        } catch (const std::bad_alloc&) {
            return E_OUTOFMEMORY;
        } catch (...) {
            return E_FAIL;
        }
    }

    // ---- IInvokeProvider ----

    IFACEMETHODIMP Invoke() override {
        if (!available() || is_root_) {
            return run_inside_com_boundary([&]() -> HRESULT {
                return UIA_E_ELEMENTNOTAVAILABLE;
            });
        }
        return source_->invoke_element(index_) ? S_OK : E_INVALIDARG;
    }

    // ---- IToggleProvider ----

    IFACEMETHODIMP Toggle() override {
        if (!available() || is_root_) {
            return run_inside_com_boundary([&]() -> HRESULT {
                return UIA_E_ELEMENTNOTAVAILABLE;
            });
        }
        return source_->toggle_element(index_) ? S_OK : E_INVALIDARG;
    }

    IFACEMETHODIMP get_ToggleState(enum ToggleState* state) override {
        if (state == nullptr) {
            return run_inside_com_boundary([&]() -> HRESULT {
                return E_POINTER;
            });
        }
        *state = ToggleState_Off;
        if (!available() || is_root_) {
            return UIA_E_ELEMENTNOTAVAILABLE;
        }
        ElementInfo info{};
        if (!source_->element_info(index_, info)) {
            return UIA_E_ELEMENTNOTAVAILABLE;
        }
        *state = info.toggle_state_on ? ToggleState_On : ToggleState_Off;
        return S_OK;
    }

    // ---- IExpandCollapseProvider ----

    IFACEMETHODIMP Expand() override {
        if (!available() || is_root_) {
            return run_inside_com_boundary([&]() -> HRESULT {
                return UIA_E_ELEMENTNOTAVAILABLE;
            });
        }
        return source_->expand_element(index_, true) ? S_OK : E_INVALIDARG;
    }

    IFACEMETHODIMP Collapse() override {
        if (!available() || is_root_) {
            return run_inside_com_boundary([&]() -> HRESULT {
                return UIA_E_ELEMENTNOTAVAILABLE;
            });
        }
        return source_->expand_element(index_, false) ? S_OK : E_INVALIDARG;
    }

    IFACEMETHODIMP get_ExpandCollapseState(enum ExpandCollapseState* state) override {
        if (state == nullptr) {
            return run_inside_com_boundary([&]() -> HRESULT {
                return E_POINTER;
            });
        }
        *state = ExpandCollapseState_Collapsed;
        if (!available() || is_root_) {
            return UIA_E_ELEMENTNOTAVAILABLE;
        }
        ElementInfo info{};
        if (!source_->element_info(index_, info)) {
            return UIA_E_ELEMENTNOTAVAILABLE;
        }
        *state = info.expanded ? ExpandCollapseState_Expanded : ExpandCollapseState_Collapsed;
        return S_OK;
    }

    // ---- IRangeValueProvider ----

    IFACEMETHODIMP SetValue(double value) override {
        if (!available() || is_root_) {
            return run_inside_com_boundary([&]() -> HRESULT {
                return UIA_E_ELEMENTNOTAVAILABLE;
            });
        }
        return source_->set_element_value(index_, value) ? S_OK : E_INVALIDARG;
    }

    IFACEMETHODIMP get_Value(double* value) override {
        if (value == nullptr) {
            return run_inside_com_boundary([&]() -> HRESULT {
                return E_POINTER;
            });
        }
        *value = 0.0;
        if (!available() || is_root_) {
            return UIA_E_ELEMENTNOTAVAILABLE;
        }
        ElementInfo info{};
        if (!source_->element_info(index_, info) || !info.supports_range) {
            return UIA_E_ELEMENTNOTAVAILABLE;
        }
        *value = info.range_value;
        return S_OK;
    }

    IFACEMETHODIMP get_Minimum(double* value) override {
        return range_bound(value, &ElementInfo::range_min);
    }

    IFACEMETHODIMP get_Maximum(double* value) override {
        return range_bound(value, &ElementInfo::range_max);
    }

    IFACEMETHODIMP get_SmallChange(double* value) override {
        return fixed_range_step(value, 1.0);
    }

    IFACEMETHODIMP get_LargeChange(double* value) override {
        return fixed_range_step(value, 10.0);
    }

    IFACEMETHODIMP get_IsReadOnly(BOOL* read_only) override {
        if (read_only == nullptr) {
            return run_inside_com_boundary([&]() -> HRESULT {
                return E_POINTER;
            });
        }
        *read_only = FALSE;
        return S_OK;
    }

  private:
    using RangeMember = double ElementInfo::*;

    bool available() const noexcept {
        return source_ != nullptr && window_ != nullptr && !alive_.expired();
    }

    ElementRole element_role() const {
        ElementInfo info{};
        if (source_->element_info(index_, info)) {
            return info.role;
        }
        return ElementRole::text;
    }

    // First index at `start` (moving by `step`) whose element the source
    // exposes; -1 when the range ends without one. Sources may decline
    // individual indices (element_info false — e.g. a dead OSK grid slot);
    // tree navigation skips them instead of handing out empty fragments.
    int next_exposed_index(int start, int step) const {
        const int count = source_->element_count();
        for (int index = start; index >= 0 && index < count; index += step) {
            ElementInfo info{};
            if (source_->element_info(index, info)) {
                return index;
            }
        }
        return -1;
    }

    std::wstring element_name() const {
        ElementInfo info{};
        if (source_->element_info(index_, info)) {
            return info.name;
        }
        return {};
    }

    bool element_focusable() const {
        ElementInfo info{};
        return source_->element_info(index_, info) && info.focusable;
    }

    bool element_has_focus() const {
        if (is_root_) {
            return ::GetForegroundWindow() == window_ && source_->focused_element() < 0;
        }
        ElementInfo info{};
        return source_->element_info(index_, info) && info.focused;
    }

    HRESULT range_bound(double* value, RangeMember member) {
        if (value == nullptr) {
            return E_POINTER;
        }
        *value = 0.0;
        if (!available() || is_root_) {
            return UIA_E_ELEMENTNOTAVAILABLE;
        }
        ElementInfo info{};
        if (!source_->element_info(index_, info) || !info.supports_range) {
            return UIA_E_ELEMENTNOTAVAILABLE;
        }
        *value = info.*member;
        return S_OK;
    }

    HRESULT fixed_range_step(double* value, double step) {
        if (value == nullptr) {
            return E_POINTER;
        }
        *value = 0.0;
        if (!available() || is_root_) {
            return UIA_E_ELEMENTNOTAVAILABLE;
        }
        ElementInfo info{};
        if (!source_->element_info(index_, info) || !info.supports_range) {
            return UIA_E_ELEMENTNOTAVAILABLE;
        }
        *value = step;
        return S_OK;
    }

    // Client-pixel element rect -> screen-space UiaRect (screen px).
    HRESULT screen_rect(UiaRect& rect) {
        ElementInfo info{};
        if (!is_root_) {
            if (!source_->element_info(index_, info)) {
                return UIA_E_ELEMENTNOTAVAILABLE;
            }
        } else {
            ::GetClientRect(window_, &info.rect);
        }
        POINT origin{0, 0};
        ::ClientToScreen(window_, &origin);
        rect.left = static_cast<double>(origin.x + info.rect.left);
        rect.top = static_cast<double>(origin.y + info.rect.top);
        rect.width = static_cast<double>(info.rect.right - info.rect.left);
        rect.height = static_cast<double>(info.rect.bottom - info.rect.top);
        return S_OK;
    }

    std::unique_ptr<ElementProvider> make_root_provider() {
        return std::make_unique<ElementProvider>(window_, source_, alive_, true, -1);
    }

    std::unique_ptr<ElementProvider> make_child_provider(int index) {
        return std::make_unique<ElementProvider>(window_, source_, alive_, false, index);
    }

    HWND window_;
    ElementSource* source_;
    std::weak_ptr<int> alive_;
    bool is_root_;
    int index_;
    std::atomic<LONG> reference_count_;
};

}  // namespace

LRESULT handle_get_object(HWND window,
                          WPARAM w_param,
                          LPARAM l_param,
                          ElementSource* source,
                          const std::shared_ptr<int>& alive_token,
                          bool& handled) {
    handled = false;

    // UiaReturnRawElementProvider itself validates the object id and answers
    // exactly two requests:
    //   * UiaRootObjectId — the UI Automation provider LRESULT.
    //   * OBJID_CLIENT    — the same provider exposed through the Microsoft
    //     Active Accessibility bridge (screen readers and the UIA core's own
    //     provider re-dispatch use this object id).
    // Returning zero for OBJID_CLIENT silently severs both bridges: clients
    // then see an empty element with no children. Everything else falls back
    // to DefWindowProc below.
    const DWORD object_id = static_cast<DWORD>(l_param);
    if ((object_id != static_cast<DWORD>(UiaRootObjectId)
         && object_id != static_cast<DWORD>(OBJID_CLIENT))
        || !uia_available()) {
        return 0;
    }
    try {
        // LresultFromObject semantics: UiaReturnRawElementProvider marshals
        // the provider with its own reference, so this initial reference is
        // released right after the call.
        auto* root = new ElementProvider(window, source, alive_token, true, -1);
        const UiaFunctions& functions = uia_functions();
        const LRESULT result = functions.return_raw(window, w_param, l_param, root);
        root->Release();
        handled = true;
        return result;
    } catch (const std::bad_alloc&) {
        return 0;
    } catch (...) {
        return 0;
    }
}

void notify_focus_changed(HWND window,
                          ElementSource* source,
                          const std::shared_ptr<int>& alive_token) {
    if (!uia_available()) {
        return;
    }
    try {
        const UiaFunctions& functions = uia_functions();
        const int focus = source->focused_element();
        // Heap instances only: UIA AddRefs/releases providers it is handed,
        // so a stack-allocated provider could be deleted through Release().
        ElementProvider* target =
            focus < 0 ? new ElementProvider(window, source, alive_token, true, -1)
                      : new ElementProvider(window, source, alive_token, false, focus);
        (void)functions.raise_event(target, UIA_AutomationFocusChangedEventId);
        target->Release();
    } catch (...) {
        // Accessibility events must never disturb the window's own path.
    }
}

}  // namespace suborno::platform::windows::app::ui_access
