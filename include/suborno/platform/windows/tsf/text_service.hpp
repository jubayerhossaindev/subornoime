#pragma once

#include <memory>
#include <msctf.h>
#include <string>
#include <unordered_map>
#include <wil/com.h>
#include <wil/resource.h>

#include "suborno/core/engine.hpp"
#include "suborno/layout-api/input_session.hpp"
#include "suborno/platform/windows/tsf/candidate_list_ui_element.hpp"
#include "suborno/platform/windows/tsf/compartment.hpp"

namespace suborno::platform::windows {

/// TSF Text Service implementation
///
/// legacy reference IME parity sink set (the four behaviours the V16 integration audit
/// found missing): ITfTextEditSink, ITfThreadFocusSink,
/// ITfDisplayAttributeProvider and the ITfCompartmentEventSink advises are
/// implemented here alongside the pre-existing sinks.
class SubornoTextService final : public ITfTextInputProcessor,
                                 public ITfKeyEventSink,
                                 public ITfThreadMgrEventSink,
                                 public ITfCompositionSink,
                                 public ITfTextEditSink,
                                 public ITfThreadFocusSink,
                                 public ITfDisplayAttributeProvider {
  public:
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void** ppvObj) override;
    STDMETHOD_(ULONG, AddRef)() override;
    STDMETHOD_(ULONG, Release)() override;

    // ITfTextInputProcessor
    STDMETHOD(Activate)(ITfThreadMgr* pThreadMgr, TfClientId clientId) override;
    STDMETHOD(Deactivate)() override;

    /// ITfKeyEventSink
    STDMETHOD(OnKeyDown)
    (ITfContext* pContext, WPARAM wParam, LPARAM lParam, BOOL* pbEaten) override;
    STDMETHOD(OnKeyUp)(ITfContext* pContext, WPARAM wParam, LPARAM lParam, BOOL* pbEaten) override;
    STDMETHOD(OnTestKeyDown)
    (ITfContext* pContext, WPARAM wParam, LPARAM lParam, BOOL* pbEaten) override;
    STDMETHOD(OnTestKeyUp)
    (ITfContext* pContext, WPARAM wParam, LPARAM lParam, BOOL* pbEaten) override;
    STDMETHOD(OnSetFocus)(BOOL fForeground) override;
    STDMETHOD(OnPreservedKey)(ITfContext* pContext, REFGUID rguid, BOOL* pbEaten) override;

    // ITfThreadMgrEventSink
    STDMETHOD(OnInitDocumentMgr)(ITfDocumentMgr* pDocMgr) override;
    STDMETHOD(OnUninitDocumentMgr)(ITfDocumentMgr* pDocMgr) override;
    STDMETHOD(OnSetFocus)(ITfDocumentMgr* pDocMgrFocus, ITfDocumentMgr* pDocMgrPrevFocus) override;
    STDMETHOD(OnPushContext)(ITfContext* pContext) override;
    STDMETHOD(OnPopContext)(ITfContext* pContext) override;
    STDMETHOD(OnCompositionTerminated)
    (TfEditCookie editCookie, ITfComposition* composition) override;

    /// ITfTextEditSink — legacy reference IME parity (infra/TextEditSink.cpp): when the
    /// selection escapes the composition range while composing, end the
    /// composition keeping the composed text on screen.
    STDMETHOD(OnEndEdit)
    (ITfContext* pContext, TfEditCookie ecReadOnly, ITfEditRecord* pEditRecord) override;

    // ITfThreadFocusSink — legacy reference IME parity (infra/ThreadFocusSink.cpp): the
    // sink is advised so the thread manager notifies the service; upstream's
    // handlers are empty and Suborno's match that behaviour.
    STDMETHOD(OnSetThreadFocus)() override;
    STDMETHOD(OnKillThreadFocus)() override;

    /// ITfDisplayAttributeProvider — legacy reference IME parity
    /// (display/DisplayAttributeProvider.cpp): exposes the Input/Converted
    /// display attributes host applications use to render composition text.
    STDMETHOD(EnumDisplayAttributeInfo)(_COM_Outptr_ IEnumTfDisplayAttributeInfo** ppEnum) override;
    STDMETHOD(GetDisplayAttributeInfo)
    (__RPC__in REFGUID guidInfo, _COM_Outptr_ ITfDisplayAttributeInfo** ppInfo) override;

  public:
    static HRESULT CreateInstance(REFIID riid, void** ppv);

  private:
    SubornoTextService();
    ~SubornoTextService();

    /// COM reference counting
    volatile LONG m_refCount = 1;

    /// TSF interfaces
    wil::com_ptr<ITfThreadMgr> m_threadMgr;
    DWORD m_clientId = TF_CLIENTID_NULL;

    /// Per-context composition state
    struct ContextState {
        wil::com_ptr<ITfContext> context;
        std::unique_ptr<layout::InputSession> session;
        /// The layout this context's session was created for (engine layout id).
        std::string session_layout_id;
        /// Per-application-window switching (general.switching_mode): the
        /// layout the user last chose while typing in THIS context. Empty in
        /// global mode — every context then follows the engine's layout.
        std::string layout_override;
        bool session_needs_refresh = false;
        wil::com_ptr<ITfComposition> composition;
        wil::com_ptr<CandidateListUIElement> candidate_ui_element;
        DWORD candidate_ui_element_id = 0;
        bool candidate_ui_owned = false;
        /// ITfTextEditSink advice for this context (legacy reference IME parity:
        /// _InitTextEditSink advised on the focused context; Suborno keeps one
        /// advice per context because its composition state is per-context).
        wil::com_ptr<ITfContext> text_edit_sink_context;
        DWORD text_edit_sink_cookie = TF_INVALID_COOKIE;
    };
    HRESULT _CommitString(ITfContext* pContext, const std::wstring& text);
    HRESULT _UpdateCandidateUI(ITfContext* context,
                               ContextState& state,
                               const layout::CompositionSnapshot& snapshot);
    HRESULT _EndCandidateUI(ContextState& state);
    HRESULT _FinalizeCandidate(ITfContext* context, UINT index);
    HRESULT _AbortCandidate(ITfContext* context);
    HRESULT _UpdateComposition(ITfContext* context, ContextState& state, const std::wstring& text);
    HRESULT _FinishComposition(ITfContext* context, ContextState& state, const std::wstring& text);

    // Resolves the layout a context should type with: the context override
    // in per-application-window mode, otherwise the engine's active layout.
    std::string effective_layout_id(const ContextState& state) const;
    // Creates (or refreshes) the context session for its effective layout.
    HRESULT _EnsureSession(ITfContext* context, ContextState& state);
    // Applies a settings-driven layout change to the receiving context:
    // per-application-window mode records it as the context's override;
    // global mode clears every override.
    void _ApplyLayoutChange(ITfContext* context, const std::string& layout_id);
    // Converts a committed Bangla string (UTF-8) to the configured text
    // encoding (Unicode passthrough or Bijoy ANSI when encodable), then to
    // UTF-16 for the edit session.
    std::wstring _CommittedTextToUtf16(const std::string& text_utf8) const;
    // Learning + next-word prediction bookkeeping for a committed word.
    void _RecordCommit(const std::string& text_utf8);
    // Reflects the active layout into the Windows input indicator: the
    // keyboard open/close compartment (open = Bangla composing, closed =
    // English direct typing) and, when the bn-BD locale option is enabled,
    // a best-effort reactivation of the bn-BD language profile on this thread.
    void _UpdateInputIndicators();

    // ---- legacy reference IME parity: the four V16-audit-gap behaviours ----
    // Registers the Input/Converted display attribute GUIDs with the TSF
    // category manager (legacy reference IME _InitDisplayAttributeGuidAtom).
    HRESULT _InitDisplayAttributeGuidAtom();
    /// Sets/clears GUID_PROP_ATTRIBUTE over the composition range
    /// (legacy reference IME _SetCompositionDisplayAttributes /
    /// _ClearCompositionDisplayAttributes). The edit-session helpers apply
    /// them inside the composition edit session; these wrappers exist for the
    /// OnCompositionTerminated path where a write cookie is already held.
    HRESULT _ClearCompositionDisplayAttributes(TfEditCookie edit_cookie,
                                               ITfContext* context,
                                               ITfComposition* composition);
    // Advise/unadvise ITfTextEditSink on one context
    // (legacy reference IME _InitTextEditSink, adapted per-context).
    BOOL _InitTextEditSink(ITfContext* context, ContextState& state);
    void _ShutdownTextEditSink(ContextState& state);
    /// TRUE when the selection range lies fully inside the composition range
    /// (legacy reference IME _IsRangeCovered).
    BOOL _IsRangeCovered(TfEditCookie edit_cookie,
                         ITfRange* range_test,
                         ITfRange* range_cover) const;
    // Advise/unadvise ITfThreadFocusSink on the thread manager
    // (legacy reference IME _InitThreadFocusSink / _UninitThreadFocusSink).
    HRESULT _InitThreadFocusSink();
    void _UninitThreadFocusSink();
    // Compartment-change entry point: re-syncs the input indicator when the
    // keyboard open/close compartment changes externally (legacy reference IME advises
    // the same compartments; its callback is an empty stub, Suborno's routes
    // into _UpdateInputIndicators). Static because ITfCompartmentEventSink's
    // callback contract carries a void* context.
    static HRESULT CompartmentChangeCallback(void* context, REFGUID guid_compartment);

    std::unordered_map<ITfContext*, ContextState> m_contexts;
    std::unique_ptr<core::Engine> m_engine;
    wil::com_ptr<ITfKeystrokeMgr> m_keystrokeMgr;
    wil::com_ptr<ITfUIElementMgr> m_uiElementMgr;
    DWORD m_threadMgrSinkCookie = TF_INVALID_COOKIE;
    /// Tracks the layout the indicators were last applied for, so the
    /// compartment writes and profile activation run on actual changes only.
    std::string m_indicator_layout_id;

    /// legacy reference IME parity state
    /// Registered display-attribute guid atoms (0 until _InitDisplayAttributeGuidAtom).
    TfGuidAtom m_gaDisplayAttributeInput = 0;
    TfGuidAtom m_gaDisplayAttributeConverted = 0;
    /// ITfThreadFocusSink advice cookie on the thread manager.
    DWORD m_threadFocusSinkCookie = TF_INVALID_COOKIE;
    /// Compartment event sinks (legacy reference IME parity: keyboard open/close and
    /// input-mode conversion).
    std::unique_ptr<CompartmentEventSink> m_compartmentOpenCloseSink;
    std::unique_ptr<CompartmentEventSink> m_compartmentConversionSink;
};

}  // namespace suborno::platform::windows
