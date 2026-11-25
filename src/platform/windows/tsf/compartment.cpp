// BornoTSF parity components (GPL-3.0-or-later): ported from BornoTSF
// infra/Compartment.cpp (c) Jayed Ahsan Saad, BornoTSF contributors.
// See compartment.hpp for the port notes.

#include "suborno/platform/windows/tsf/compartment.hpp"

#include <assert.h>

#include "../com/boundary_guard.hpp"

namespace suborno::platform::windows {

Compartment::Compartment(IUnknown* owner, TfClientId client_id, REFGUID guid_compartment)
    : client_id_(client_id),
      guid_compartment_(guid_compartment) {
    // AddRef the owner like upstream does (_punk = punk; _punk->AddRef()).
    owner_ = owner;
}

HRESULT Compartment::get_compartment(_Outptr_ ITfCompartment** compartment) const {
    if (compartment == nullptr) {
        return E_INVALIDARG;
    }
    *compartment = nullptr;
    if (owner_ == nullptr) {
        return E_POINTER;
    }

    wil::com_ptr<ITfCompartmentMgr> compartment_mgr;
    HRESULT result = owner_->QueryInterface(IID_PPV_ARGS(compartment_mgr.put()));
    if (SUCCEEDED(result) && compartment_mgr != nullptr) {
        result = compartment_mgr->GetCompartment(guid_compartment_, compartment);
    }
    return result;
}

HRESULT Compartment::get_bool_value(_Out_ BOOL* enabled) const {
    if (enabled == nullptr) {
        return E_INVALIDARG;
    }
    *enabled = FALSE;

    wil::com_ptr<ITfCompartment> compartment;
    HRESULT result = get_compartment(compartment.put());
    if (result != S_OK) {
        return result;
    }

    VARIANT value{};
    result = compartment->GetValue(&value);
    if (result == S_OK) {
        if (value.vt == VT_I4) {
            *enabled = static_cast<BOOL>(value.lVal);
        } else {
            result = S_FALSE;
        }
    }
    return result;
}

HRESULT Compartment::set_bool_value(BOOL enabled) const {
    wil::com_ptr<ITfCompartment> compartment;
    HRESULT result = get_compartment(compartment.put());
    if (SUCCEEDED(result)) {
        VARIANT value{};
        value.vt = VT_I4;
        value.lVal = enabled ? 1 : 0;
        result = compartment->SetValue(client_id_, &value);
    }
    return result;
}

HRESULT Compartment::get_dword_value(_Out_ DWORD* compartment_value) const {
    if (compartment_value == nullptr) {
        return E_INVALIDARG;
    }
    *compartment_value = 0;

    wil::com_ptr<ITfCompartment> compartment;
    HRESULT result = get_compartment(compartment.put());
    if (SUCCEEDED(result)) {
        VARIANT variant{};
        result = compartment->GetValue(&variant);
        if (result == S_OK) {
            if (variant.vt == VT_I4) {
                *compartment_value = static_cast<DWORD>(variant.lVal);
            } else {
                result = S_FALSE;
            }
        }
    }
    return result;
}

HRESULT Compartment::set_dword_value(DWORD value) const {
    wil::com_ptr<ITfCompartment> compartment;
    HRESULT result = get_compartment(compartment.put());
    if (SUCCEEDED(result)) {
        VARIANT variant{};
        variant.vt = VT_I4;
        variant.lVal = static_cast<LONG>(value);
        result = compartment->SetValue(client_id_, &variant);
    }
    return result;
}

HRESULT Compartment::clear_value() {
    // Upstream guard (Compartment.cpp:133-138): the keyboard open/close
    // compartment must never be cleared by the IME.
    if (::IsEqualGUID(guid_compartment_, GUID_COMPARTMENT_KEYBOARD_OPENCLOSE)) {
        return S_FALSE;
    }

    if (owner_ == nullptr) {
        return E_POINTER;
    }

    wil::com_ptr<ITfCompartmentMgr> compartment_mgr;
    HRESULT result = owner_->QueryInterface(IID_PPV_ARGS(compartment_mgr.put()));
    if (result == S_OK && compartment_mgr != nullptr) {
        result = compartment_mgr->ClearCompartment(client_id_, guid_compartment_);
    }
    return result;
}

CompartmentEventSink::CompartmentEventSink(CompartmentCallback callback, void* callback_context)
    : callback_(callback),
      callback_context_(callback_context) {}

STDMETHODIMP CompartmentEventSink::QueryInterface(REFIID riid, void** object) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (object == nullptr) {
            return E_INVALIDARG;
        }
        *object = nullptr;

        if (riid == IID_IUnknown || riid == IID_ITfCompartmentEventSink) {
            *object = static_cast<CompartmentEventSink*>(this);
        }
        if (*object == nullptr) {
            return E_NOINTERFACE;
        }

        AddRef();
        return S_OK;
    });
}

STDMETHODIMP_(ULONG) CompartmentEventSink::AddRef() {
    return static_cast<ULONG>(::InterlockedIncrement(&reference_count_));
}

STDMETHODIMP_(ULONG) CompartmentEventSink::Release() {
    const LONG count = ::InterlockedDecrement(&reference_count_);
    assert(count >= 0);
    if (count == 0) {
        delete this;
    }
    return static_cast<ULONG>(count);
}

STDMETHODIMP CompartmentEventSink::OnChange(REFGUID guid_compartment) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (callback_ == nullptr) {
            return S_OK;
        }
        return callback_(callback_context_, guid_compartment);
    });
}

HRESULT CompartmentEventSink::Advise(IUnknown* owner, REFGUID guid_compartment) {
    if (owner == nullptr) {
        return E_POINTER;
    }

    wil::com_ptr<ITfCompartmentMgr> compartment_mgr;
    HRESULT result = owner->QueryInterface(IID_PPV_ARGS(compartment_mgr.put()));
    if (FAILED(result)) {
        return result;
    }

    result = compartment_mgr->GetCompartment(guid_compartment, compartment_.put());
    if (SUCCEEDED(result)) {
        wil::com_ptr<ITfSource> source;
        result = compartment_->QueryInterface(IID_PPV_ARGS(source.put()));
        if (SUCCEEDED(result)) {
            result = source->AdviseSink(IID_ITfCompartmentEventSink, this, &cookie_);
        }
    }
    return result;
}

HRESULT CompartmentEventSink::Unadvise() {
    HRESULT result = S_OK;
    if (compartment_ != nullptr) {
        wil::com_ptr<ITfSource> source;
        result = compartment_->QueryInterface(IID_PPV_ARGS(source.put()));
        if (SUCCEEDED(result)) {
            result = source->UnadviseSink(cookie_);
        }
    }
    compartment_.reset();
    cookie_ = 0;
    return result;
}

}  // namespace suborno::platform::windows
