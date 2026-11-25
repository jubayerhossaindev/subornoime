#pragma once

#include <msctf.h>
#include <string>
#include <wil/com.h>

namespace suborno::platform::windows {

// TfGuidAtom value meaning "no display attribute change". Atoms are

/// DWORDs issued by ITfCategoryMgr::RegisterGUID; 0 is never issued.

constexpr TfGuidAtom no_display_attribute = 0;

class TextInsertionEditSession final : public ITfEditSession {
  public:
    static HRESULT insert_text(ITfContext* context, TfClientId client_id, std::wstring text);

    STDMETHOD(QueryInterface)(REFIID riid, void** object) override;
    STDMETHOD_(ULONG, AddRef)() override;
    STDMETHOD_(ULONG, Release)() override;
    STDMETHOD(DoEditSession)(TfEditCookie edit_cookie) override;

  private:
    TextInsertionEditSession(ITfContext* context, std::wstring text);
    ~TextInsertionEditSession() = default;

    volatile LONG reference_count_ = 1;
    wil::com_ptr<ITfContext> context_;
    std::wstring text_;
};

class TextCompositionEditSession final : public ITfEditSession {
  public:
    // display_attribute (when not no_display_attribute) sets the composition

    // range's GUID_PROP_ATTRIBUTE property to that guid atom inside the same

    // edit session — the BornoTSF parity behaviour

    /// (_SetCompositionDisplayAttributes in display/DisplayAttribute.cpp).

    static HRESULT start_composition(ITfContext* context,
                                     TfClientId client_id,
                                     ITfCompositionSink* sink,
                                     std::wstring text,
                                     wil::com_ptr<ITfComposition>& composition,
                                     TfGuidAtom display_attribute = no_display_attribute);
    static HRESULT update_composition(ITfContext* context,
                                      TfClientId client_id,
                                      ITfComposition* composition,
                                      std::wstring text,
                                      TfGuidAtom display_attribute = no_display_attribute);
    static HRESULT finish_composition(ITfContext* context,
                                      TfClientId client_id,
                                      ITfComposition* composition,
                                      std::wstring text,
                                      bool clear_display_attribute = false);
    // Ends the composition without touching its text: clears the display

    // attribute on the composition range, then EndComposition. The BornoTSF

    // parity behaviour of _TerminateComposition/_EndComposition

    // (composition/EndComposition.cpp) used when the selection escapes the

    /// composition (OnEndEdit) — the composed text stays as-is.

    static HRESULT terminate_composition(ITfContext* context,
                                         TfClientId client_id,
                                         ITfComposition* composition);

    STDMETHOD(QueryInterface)(REFIID riid, void** object) override;
    STDMETHOD_(ULONG, AddRef)() override;
    STDMETHOD_(ULONG, Release)() override;
    STDMETHOD(DoEditSession)(TfEditCookie edit_cookie) override;

  private:
    TextCompositionEditSession(ITfContext* context,
                               ITfComposition* composition,
                               ITfCompositionSink* sink,
                               std::wstring text,
                               bool finish,
                               bool allow_async_fallback,
                               TfGuidAtom display_attribute,
                               bool clear_display_attribute);
    ~TextCompositionEditSession() = default;

    HRESULT Request(TfClientId client_id, wil::com_ptr<ITfComposition>* composition);

    volatile LONG reference_count_ = 1;
    wil::com_ptr<ITfContext> context_;
    wil::com_ptr<ITfComposition> composition_;
    wil::com_ptr<ITfCompositionSink> sink_;
    std::wstring text_;
    bool finish_ = false;
    bool allow_async_fallback_ = false;
    TfGuidAtom display_attribute_ = no_display_attribute;
    bool clear_display_attribute_ = false;
    bool terminate_ = false;
};

}  // namespace suborno::platform::windows
