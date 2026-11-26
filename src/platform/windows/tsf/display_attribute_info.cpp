// BornoTSF parity components (GPL-3.0-or-later): ported from BornoTSF

// display/DisplayAttributeInfo.cpp and display/EnumDisplayAttributeInfo.cpp

// (c) Jayed Ahsan Saad, BornoTSF contributors.

//

// The Input/Converted static tables and the GetAttributeInfo override below

// reproduce the upstream code path exactly (see the header comment for why

// the override is kept despite looking like dead data).

#include "suborno/platform/windows/tsf/display_attribute_info.hpp"

#include <comutil.h>
#include <cstddef>
#include <cstring>
#include <new>

#include "../com/boundary_guard.hpp"
#include "../com/module_lifetime.hpp"
#include "suborno/platform/windows/tsf/guids.hpp"

namespace suborno::platform::windows {
namespace {

// Upstream Windows-11-style tables (display/DisplayAttributeInfo.cpp:22-40).

// They are stored, returned by Reset()-supporting code paths structurally,

// but GetAttributeInfo overwrites the caller-visible value exactly like

// upstream does — kept for behavioural parity, not for style.

constexpr TF_DISPLAYATTRIBUTE input_style_table = {
    /*crText*/ {TF_CT_COLORREF, RGB(0, 103, 206)},
    /*crBk*/ {TF_CT_NONE, 0},
    /*lsStyle*/ TF_LS_DOT,
    /*fBoldLine*/ FALSE,
    /*crLine*/ {TF_CT_COLORREF, RGB(0, 103, 206)},
    /*bAttr*/ TF_ATTR_INPUT,
};

constexpr TF_DISPLAYATTRIBUTE converted_style_table = {
    /*crText*/ {TF_CT_COLORREF, RGB(255, 255, 255)},
    /*crBk*/ {TF_CT_COLORREF, RGB(0, 255, 255)},
    /*lsStyle*/ TF_LS_NONE,
    /*fBoldLine*/ FALSE,
    /*crLine*/ {TF_CT_NONE, 0},
    /*bAttr*/ TF_ATTR_TARGET_CONVERTED,
};

constexpr int max_display_attribute_info = 2;

}  // namespace

const GUID* DisplayAttributeInfoInput::guid() {
    return &GUID_DISPLAY_ATTRIBUTE_INPUT;
}

const TF_DISPLAYATTRIBUTE* DisplayAttributeInfoInput::attribute() {
    return &input_style_table;
}

const wchar_t* DisplayAttributeInfoInput::description() {
    return L"Suborno Text Service Display Attribute Input";
}

const GUID* DisplayAttributeInfoConverted::guid() {
    return &GUID_DISPLAY_ATTRIBUTE_CONVERTED;
}

const TF_DISPLAYATTRIBUTE* DisplayAttributeInfoConverted::attribute() {
    return &converted_style_table;
}

const wchar_t* DisplayAttributeInfoConverted::description() {
    return L"Suborno Text Service Display Attribute Converted";
}

DisplayAttributeInfo::DisplayAttributeInfo(const GUID* guid,
                                           const TF_DISPLAYATTRIBUTE* display_attribute,
                                           const wchar_t* description)
    : guid_(guid),
      display_attribute_(display_attribute),
      description_(description != nullptr ? description : L"") {
    com::module_lifetime().object_created();
}

DisplayAttributeInfo::~DisplayAttributeInfo() {
    com::module_lifetime().object_destroyed();
}

STDMETHODIMP DisplayAttributeInfo::QueryInterface(REFIID riid, void** object) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (object == nullptr) {
            return E_INVALIDARG;
        }
        *object = nullptr;

        if (riid == IID_IUnknown || riid == IID_ITfDisplayAttributeInfo) {
            *object = static_cast<ITfDisplayAttributeInfo*>(this);
        }
        if (*object == nullptr) {
            return E_NOINTERFACE;
        }

        AddRef();
        return S_OK;
    });
}

STDMETHODIMP_(ULONG) DisplayAttributeInfo::AddRef() {
    return static_cast<ULONG>(::InterlockedIncrement(&reference_count_));
}

STDMETHODIMP_(ULONG) DisplayAttributeInfo::Release() {
    const ULONG count = static_cast<ULONG>(::InterlockedDecrement(&reference_count_));
    if (count == 0) {
        delete this;
    }
    return count;
}

STDMETHODIMP DisplayAttributeInfo::GetGUID(_Out_ GUID* guid) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (guid == nullptr) {
            return E_INVALIDARG;
        }
        if (guid_ == nullptr) {
            return E_FAIL;
        }
        *guid = *guid_;
        return S_OK;
    });
}

STDMETHODIMP DisplayAttributeInfo::GetDescription(_Out_ BSTR* description) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (description == nullptr) {
            return E_INVALIDARG;
        }
        *description = nullptr;
        const BSTR allocated =
            ::SysAllocStringLen(description_.c_str(), static_cast<UINT>(description_.size()));
        if (allocated == nullptr) {
            return E_OUTOFMEMORY;
        }
        *description = allocated;
        return S_OK;
    });
}

STDMETHODIMP DisplayAttributeInfo::GetAttributeInfo(_Out_ TF_DISPLAYATTRIBUTE* attribute) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (attribute == nullptr) {
            return E_INVALIDARG;
        }

        // Upstream code path (DisplayAttributeInfo.cpp:142-172): copy the static

        // table, zero it, then rebuild the effective squiggle style. The net

        // effect for both attributes: black squiggle underline, TF_ATTR_INPUT.

        *attribute = *display_attribute_;
        ::memset(attribute, 0, sizeof(TF_DISPLAYATTRIBUTE));

        attribute->crText.type = TF_CT_NONE;
        attribute->crBk.type = TF_CT_NONE;
        attribute->crLine.type = TF_CT_COLORREF;
        attribute->crLine.cr = RGB(0, 0, 0);
        attribute->lsStyle = TF_LS_SQUIGGLE;
        attribute->fBoldLine = FALSE;
        attribute->bAttr = TF_ATTR_INPUT;

        return S_OK;
    });
}

STDMETHODIMP DisplayAttributeInfo::SetAttributeInfo(_In_ const TF_DISPLAYATTRIBUTE* attribute) {
    return run_inside_com_boundary([&]() -> HRESULT {
        (void)attribute;
        return E_NOTIMPL;
    });
}

STDMETHODIMP DisplayAttributeInfo::Reset() {
    return run_inside_com_boundary([&]() -> HRESULT {
        return SetAttributeInfo(display_attribute_);
    });
}

EnumDisplayAttributeInfo::EnumDisplayAttributeInfo() {
    com::module_lifetime().object_created();
}

EnumDisplayAttributeInfo::~EnumDisplayAttributeInfo() {
    com::module_lifetime().object_destroyed();
}

STDMETHODIMP EnumDisplayAttributeInfo::QueryInterface(REFIID riid, void** object) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (object == nullptr) {
            return E_INVALIDARG;
        }
        *object = nullptr;

        if (riid == IID_IUnknown || riid == IID_IEnumTfDisplayAttributeInfo) {
            *object = static_cast<IEnumTfDisplayAttributeInfo*>(this);
        }
        if (*object == nullptr) {
            return E_NOINTERFACE;
        }

        AddRef();
        return S_OK;
    });
}

STDMETHODIMP_(ULONG) EnumDisplayAttributeInfo::AddRef() {
    return static_cast<ULONG>(::InterlockedIncrement(&reference_count_));
}

STDMETHODIMP_(ULONG) EnumDisplayAttributeInfo::Release() {
    const ULONG count = static_cast<ULONG>(::InterlockedDecrement(&reference_count_));
    if (count == 0) {
        delete this;
    }
    return count;
}

STDMETHODIMP EnumDisplayAttributeInfo::Clone(_Out_ IEnumTfDisplayAttributeInfo** enumerator) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (enumerator == nullptr) {
            return E_INVALIDARG;
        }
        *enumerator = nullptr;

        auto* clone = new (std::nothrow) EnumDisplayAttributeInfo();
        if (clone == nullptr) {
            return E_OUTOFMEMORY;
        }
        clone->index_ = index_;
        *enumerator = clone;
        return S_OK;
    });
}

STDMETHODIMP EnumDisplayAttributeInfo::Next(ULONG count,
                                            __RPC__out_ecount_part(count, *fetched)
                                                ITfDisplayAttributeInfo** info,
                                            __RPC__out ULONG* fetched) {
    return run_inside_com_boundary([&]() -> HRESULT {
        ULONG fetched_count = 0;

        if (count == 0) {
            return S_OK;
        }
        if (info == nullptr) {
            return E_INVALIDARG;
        }
        *info = nullptr;

        while (fetched_count < count) {
            ITfDisplayAttributeInfo* attribute_info = nullptr;

            if (index_ == 0) {
                attribute_info = new (std::nothrow) DisplayAttributeInfoInput();
            } else if (index_ == 1) {
                attribute_info = new (std::nothrow) DisplayAttributeInfoConverted();
            } else {
                break;
            }
            if (attribute_info == nullptr) {
                return E_OUTOFMEMORY;
            }

            *info = attribute_info;
            ++info;
            ++fetched_count;
            ++index_;
        }

        if (fetched != nullptr) {
            *fetched = fetched_count;
        }
        return fetched_count == count ? S_OK : S_FALSE;
    });
}

STDMETHODIMP EnumDisplayAttributeInfo::Reset() {
    return run_inside_com_boundary([&]() -> HRESULT {
        index_ = 0;
        return S_OK;
    });
}

STDMETHODIMP EnumDisplayAttributeInfo::Skip(ULONG count) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (count + index_ > static_cast<ULONG>(max_display_attribute_info)
            || count + index_ < count) {
            index_ = static_cast<ULONG>(max_display_attribute_info);
            return S_FALSE;
        }
        index_ += count;
        return S_OK;
    });
}

}  // namespace suborno::platform::windows
