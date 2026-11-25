#pragma once

// BornoTSF parity components (GPL-3.0-or-later): ported from BornoTSF
// infra/Compartment.{h,cpp} (c) Jayed Ahsan Saad, BornoTSF contributors.
//
// Two pieces:
//   1. Compartment — typed get/set helpers over an ITfCompartmentMgr owner
//      (thread manager or context). Mirrors BornoTSF CCompartment:
//      _GetCompartmentBOOL / _SetCompartmentBOOL / _GetCompartmentDWORD /
//      _SetCompartmentDWORD / _ClearCompartment, including the upstream
//      guard that refuses to clear GUID_COMPARTMENT_KEYBOARD_OPENCLOSE.
//   2. CompartmentEventSink — ITfCompartmentEventSink with a callback+
//      context pair (upstream CESCALLBACK), plus _Advise/_Unadvise.
//      BornoTSF advises this sink on GUID_COMPARTMENT_KEYBOARD_OPENCLOSE and
//      GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION from
//      CCompositionProcessorEngine::SetupLanguageBar; its callback body is
//      an empty stub. Suborno wires the same advise sites but routes the
//      callback into the input-indicator refresh so an external open/close
//      change (user toggling another keyboard) re-syncs the indicator.
//
// Lifetime model matches upstream: the object starts with refcount 1, the
// owner deletes it directly after _Unadvise (nobody else holds a reference).

#include <msctf.h>
#include <wil/com.h>

namespace suborno::platform::windows {

class Compartment {
  public:
    Compartment(IUnknown* owner, TfClientId client_id, REFGUID guid_compartment);
    ~Compartment() = default;

    Compartment(const Compartment&) = delete;
    Compartment& operator=(const Compartment&) = delete;

    HRESULT get_compartment(_Outptr_ ITfCompartment** compartment) const;
    HRESULT get_bool_value(_Out_ BOOL* enabled) const;
    HRESULT set_bool_value(BOOL enabled) const;
    HRESULT get_dword_value(_Out_ DWORD* compartment_value) const;
    HRESULT set_dword_value(DWORD compartment_value) const;
    HRESULT clear_value();

  private:
    wil::com_ptr<IUnknown> owner_;
    TfClientId client_id_;
    GUID guid_compartment_;
};

using CompartmentCallback = HRESULT (*)(void* context, REFGUID guid_compartment);

class CompartmentEventSink : public ITfCompartmentEventSink {
  public:
    CompartmentEventSink(CompartmentCallback callback, void* callback_context);

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void** object) override;
    STDMETHOD_(ULONG, AddRef)() override;
    STDMETHOD_(ULONG, Release)() override;

    // ITfCompartmentEventSink
    STDMETHOD(OnChange)(REFGUID guid_compartment) override;

    HRESULT Advise(IUnknown* owner, REFGUID guid_compartment);
    HRESULT Unadvise();

  private:
    CompartmentCallback callback_;
    void* callback_context_;
    wil::com_ptr<ITfCompartment> compartment_;
    DWORD cookie_ = 0;
    volatile LONG reference_count_ = 1;
};

}  // namespace suborno::platform::windows
