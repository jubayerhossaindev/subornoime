#include "suborno/platform/windows/tsf/text_edit_session.hpp"

#include <climits>
#include <memory>
#include <new>
#include <utility>

#include "../com/boundary_guard.hpp"

namespace suborno::platform::windows {
namespace {

// Requests a synchronous read-write edit session. Synchronous sessions are

// only granted when the caller holds the document lock (e.g. from a key event

// sink); when the host application is busy the request fails with

// TF_E_SYNCHRONOUS. For sessions that do not need to return data immediately

// (composition updates and commits, including those issued from Deactivate),

// fall back to an asynchronous request so the edit still happens instead of

// the composition text being stranded.

HRESULT request_session(ITfContext* context,
                        TfClientId client_id,
                        ITfEditSession* session,
                        bool allow_async_fallback) {
    HRESULT session_result = E_FAIL;
    const HRESULT request_result = context->RequestEditSession(
        client_id, session, TF_ES_SYNC | TF_ES_READWRITE, &session_result);
    const HRESULT sync_result = FAILED(request_result) ? request_result : session_result;
    if (SUCCEEDED(sync_result) || !allow_async_fallback) {
        return sync_result;
    }

    session_result = E_FAIL;
    const HRESULT async_request = context->RequestEditSession(
        client_id, session, TF_ES_ASYNC | TF_ES_READWRITE, &session_result);
    if (FAILED(async_request)) {
        return sync_result;
    }
    return session_result;
}

HRESULT set_composition_text(TfEditCookie edit_cookie,
                             ITfContext* context,
                             ITfComposition* composition,
                             const std::wstring& text) {
    wil::com_ptr<ITfRange> range;
    HRESULT result = composition->GetRange(range.put());
    if (FAILED(result)) {
        return result;
    }

    result = range->SetText(edit_cookie, 0, text.c_str(), static_cast<LONG>(text.size()));
    if (FAILED(result)) {
        return result;
    }

    result = range->Collapse(edit_cookie, TF_ANCHOR_END);
    if (FAILED(result)) {
        return result;
    }

    TF_SELECTION selection{};
    selection.range = range.get();
    selection.style.ase = TF_AE_END;
    selection.style.fInterimChar = FALSE;
    return context->SetSelection(edit_cookie, 1, &selection);
}

// Sets or clears the display attribute on the composition range — the

// BornoTSF parity helpers _SetCompositionDisplayAttributes and

// _ClearCompositionDisplayAttributes (display/DisplayAttribute.cpp).

HRESULT set_display_attribute(TfEditCookie edit_cookie,
                              ITfContext* context,
                              ITfComposition* composition,
                              TfGuidAtom display_attribute) {
    wil::com_ptr<ITfRange> range;
    HRESULT result = composition->GetRange(range.put());
    if (FAILED(result)) {
        return result;
    }

    wil::com_ptr<ITfProperty> display_attribute_property;
    result = context->GetProperty(GUID_PROP_ATTRIBUTE, display_attribute_property.put());
    if (SUCCEEDED(result) && display_attribute_property != nullptr) {
        VARIANT value{};
        value.vt = VT_I4;
        value.lVal = static_cast<LONG>(display_attribute);
        result = display_attribute_property->SetValue(edit_cookie, range.get(), &value);
    }
    return result;
}

HRESULT clear_display_attribute(TfEditCookie edit_cookie,
                                ITfContext* context,
                                ITfComposition* composition) {
    wil::com_ptr<ITfRange> range;
    HRESULT result = composition->GetRange(range.put());
    if (FAILED(result)) {
        return result;
    }

    wil::com_ptr<ITfProperty> display_attribute_property;
    if (SUCCEEDED(context->GetProperty(GUID_PROP_ATTRIBUTE, display_attribute_property.put()))
        && display_attribute_property != nullptr) {
        static_cast<void>(display_attribute_property->Clear(edit_cookie, range.get()));
    }
    return S_OK;
}

}  // namespace

TextInsertionEditSession::TextInsertionEditSession(ITfContext* context, std::wstring text)
    : context_(context),
      text_(std::move(text)) {}

HRESULT TextInsertionEditSession::insert_text(ITfContext* context,
                                              TfClientId client_id,
                                              std::wstring text) {
    if (context == nullptr || text.size() > static_cast<std::size_t>(LONG_MAX)) {
        return E_INVALIDARG;
    }

    auto* session = new (std::nothrow) TextInsertionEditSession(context, std::move(text));
    if (session == nullptr) {
        return E_OUTOFMEMORY;
    }
    // Insertions originate from the key event sink (document lock held), so a

    // synchronous request is expected to succeed; no async fallback needed.

    const HRESULT result = request_session(context, client_id, session, false);
    session->Release();
    return result;
}

STDMETHODIMP TextInsertionEditSession::QueryInterface(REFIID riid, void** object) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (object == nullptr) {
            return E_POINTER;
        }
        *object = nullptr;

        if (riid == IID_IUnknown || riid == IID_ITfEditSession) {
            *object = static_cast<ITfEditSession*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    });
}

STDMETHODIMP_(ULONG) TextInsertionEditSession::AddRef() {
    return static_cast<ULONG>(::InterlockedIncrement(&reference_count_));
}

STDMETHODIMP_(ULONG) TextInsertionEditSession::Release() {
    const ULONG count = static_cast<ULONG>(::InterlockedDecrement(&reference_count_));
    if (count == 0) {
        delete this;
    }
    return count;
}

STDMETHODIMP TextInsertionEditSession::DoEditSession(TfEditCookie edit_cookie) {
    return run_inside_com_boundary([&]() -> HRESULT {
        wil::com_ptr<ITfInsertAtSelection> insertion;
        const HRESULT query_result = context_->QueryInterface(IID_PPV_ARGS(insertion.put()));
        if (FAILED(query_result)) {
            return query_result;
        }

        wil::com_ptr<ITfRange> inserted_range;
        return insertion->InsertTextAtSelection(edit_cookie,
                                                TF_IAS_NOQUERY,
                                                text_.c_str(),
                                                static_cast<LONG>(text_.size()),
                                                inserted_range.put());
    });
}

TextCompositionEditSession::TextCompositionEditSession(ITfContext* context,
                                                       ITfComposition* composition,
                                                       ITfCompositionSink* sink,
                                                       std::wstring text,
                                                       bool finish,
                                                       bool allow_async_fallback,
                                                       TfGuidAtom display_attribute,
                                                       bool clear_display_attribute)
    : context_(context),
      composition_(composition),
      sink_(sink),
      text_(std::move(text)),
      finish_(finish),
      allow_async_fallback_(allow_async_fallback),
      display_attribute_(display_attribute),
      clear_display_attribute_(clear_display_attribute) {}

HRESULT TextCompositionEditSession::start_composition(ITfContext* context,
                                                      TfClientId client_id,
                                                      ITfCompositionSink* sink,
                                                      std::wstring text,
                                                      wil::com_ptr<ITfComposition>& composition,
                                                      TfGuidAtom display_attribute) {
    if (context == nullptr || sink == nullptr || text.size() > static_cast<std::size_t>(LONG_MAX)) {
        return E_INVALIDARG;
    }
    auto* session = new (std::nothrow) TextCompositionEditSession(
        context, nullptr, sink, std::move(text), false, false, display_attribute, false);
    if (session == nullptr) {
        return E_OUTOFMEMORY;
    }
    const HRESULT result = session->request_edit_session(client_id, std::addressof(composition));
    session->Release();
    return result;
}

HRESULT TextCompositionEditSession::update_composition(ITfContext* context,
                                                       TfClientId client_id,
                                                       ITfComposition* composition,
                                                       std::wstring text,
                                                       TfGuidAtom display_attribute) {
    if (context == nullptr || composition == nullptr
        || text.size() > static_cast<std::size_t>(LONG_MAX)) {
        return E_INVALIDARG;
    }
    auto* session = new (std::nothrow) TextCompositionEditSession(
        context, composition, nullptr, std::move(text), false, true, display_attribute, false);
    if (session == nullptr) {
        return E_OUTOFMEMORY;
    }
    const HRESULT result = session->request_edit_session(client_id, nullptr);
    session->Release();
    return result;
}

HRESULT TextCompositionEditSession::finish_composition(ITfContext* context,
                                                       TfClientId client_id,
                                                       ITfComposition* composition,
                                                       std::wstring text,
                                                       bool clear_display_attribute) {
    if (context == nullptr || composition == nullptr
        || text.size() > static_cast<std::size_t>(LONG_MAX)) {
        return E_INVALIDARG;
    }
    auto* session = new (std::nothrow) TextCompositionEditSession(context,
                                                                  composition,
                                                                  nullptr,
                                                                  std::move(text),
                                                                  true,
                                                                  true,
                                                                  no_display_attribute,
                                                                  clear_display_attribute);
    if (session == nullptr) {
        return E_OUTOFMEMORY;
    }
    const HRESULT result = session->request_edit_session(client_id, nullptr);
    session->Release();
    return result;
}

HRESULT TextCompositionEditSession::terminate_composition(ITfContext* context,
                                                          TfClientId client_id,
                                                          ITfComposition* composition) {
    if (context == nullptr || composition == nullptr) {
        return E_INVALIDARG;
    }
    auto* session = new (std::nothrow) TextCompositionEditSession(
        context, composition, nullptr, L"", true, true, no_display_attribute, true);
    if (session == nullptr) {
        return E_OUTOFMEMORY;
    }
    session->terminate_ = true;
    const HRESULT result = session->request_edit_session(client_id, nullptr);
    session->Release();
    return result;
}

HRESULT TextCompositionEditSession::request_edit_session(
    TfClientId client_id, wil::com_ptr<ITfComposition>* composition) {
    HRESULT result = request_session(context_.get(), client_id, this, allow_async_fallback_);
    if (SUCCEEDED(result) && composition != nullptr) {
        *composition = composition_;
    }
    return result;
}

STDMETHODIMP TextCompositionEditSession::QueryInterface(REFIID riid, void** object) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (object == nullptr) {
            return E_POINTER;
        }
        *object = nullptr;

        if (riid == IID_IUnknown || riid == IID_ITfEditSession) {
            *object = static_cast<ITfEditSession*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    });
}

STDMETHODIMP_(ULONG) TextCompositionEditSession::AddRef() {
    return static_cast<ULONG>(::InterlockedIncrement(&reference_count_));
}

STDMETHODIMP_(ULONG) TextCompositionEditSession::Release() {
    const ULONG count = static_cast<ULONG>(::InterlockedDecrement(&reference_count_));
    if (count == 0) {
        delete this;
    }
    return count;
}

STDMETHODIMP TextCompositionEditSession::DoEditSession(TfEditCookie edit_cookie) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (terminate_) {
            // BornoTSF _TerminateComposition (composition/EndComposition.cpp):

            // clear the display attribute first, then end the composition. The

            // composed text is left exactly as it is on screen.

            static_cast<void>(
                clear_display_attribute(edit_cookie, context_.get(), composition_.get()));
            const HRESULT result = composition_->EndComposition(edit_cookie);
            if (SUCCEEDED(result)) {
                composition_.reset();
            }
            return result;
        }

        if (composition_ == nullptr) {
            TF_SELECTION selection{};
            ULONG fetched = 0;
            HRESULT result =
                context_->GetSelection(edit_cookie, TF_DEFAULT_SELECTION, 1, &selection, &fetched);
            if (FAILED(result)) {
                return result;
            }
            if (fetched != 1 || selection.range == nullptr) {
                if (selection.range != nullptr) {
                    selection.range->Release();
                }
                return E_UNEXPECTED;
            }

            wil::com_ptr<ITfContextComposition> context_composition;
            result = context_->QueryInterface(IID_PPV_ARGS(context_composition.put()));
            if (SUCCEEDED(result)) {
                result = context_composition->StartComposition(
                    edit_cookie, selection.range, sink_.get(), composition_.put());
            }
            selection.range->Release();
            if (FAILED(result)) {
                return result;
            }
        }

        if (clear_display_attribute_) {
            // BornoTSF clears the attribute before ending the composition

            // (EndComposition.cpp:28 _ClearCompositionDisplayAttributes).

            static_cast<void>(
                clear_display_attribute(edit_cookie, context_.get(), composition_.get()));
        }

        HRESULT result =
            set_composition_text(edit_cookie, context_.get(), composition_.get(), text_);
        if (FAILED(result)) {
            if (!finish_) {
                composition_->EndComposition(edit_cookie);
            }
            return result;
        }
        if (display_attribute_ != no_display_attribute) {
            // BornoTSF sets the input attribute after the text update in the same

            // edit session (composition/Composition.cpp:173).

            static_cast<void>(set_display_attribute(
                edit_cookie, context_.get(), composition_.get(), display_attribute_));
        }
        if (finish_) {
            result = composition_->EndComposition(edit_cookie);
            if (SUCCEEDED(result)) {
                composition_.reset();
            }
        }
        return result;
    });
}

}  // namespace suborno::platform::windows
