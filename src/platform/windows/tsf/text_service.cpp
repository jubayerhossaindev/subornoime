#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "suborno/platform/windows/tsf/text_service.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "../com/boundary_guard.hpp"
#include "../com/module_lifetime.hpp"
#include "suborno/core/engine.hpp"
#include "suborno/layout-api/key_event.hpp"
#include "suborno/platform/windows/app_paths.hpp"
#include "suborno/platform/windows/tsf/compartment.hpp"
#include "suborno/platform/windows/tsf/display_attribute_info.hpp"
#include "suborno/platform/windows/tsf/guids.hpp"
#include "suborno/platform/windows/tsf/text_edit_session.hpp"

namespace suborno::platform::windows {

namespace {

// Bengali (Bangladesh) — the language the Suborno TSF profile is registered

// under; mirrors the constant used by the desktop shell.

constexpr LANGID bengali_bangladesh = MAKELANGID(0x45, 0x02);

std::wstring utf8_to_utf16(const std::string& utf8) {
    if (utf8.empty()) {
        return L"";
    }
    if (utf8.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return {};
    }
    const int size_needed = ::MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, utf8.c_str(), static_cast<int>(utf8.size()), nullptr, 0);
    if (size_needed <= 0) {
        return L"";
    }
    std::wstring utf16(size_needed, 0);
    if (::MultiByteToWideChar(CP_UTF8,
                              MB_ERR_INVALID_CHARS,
                              utf8.c_str(),
                              static_cast<int>(utf8.size()),
                              utf16.data(),
                              size_needed)
        == 0) {
        return {};
    }
    return utf16;
}

bool is_modifier_key(WPARAM key) noexcept {
    return key == VK_SHIFT || key == VK_CONTROL || key == VK_MENU || key == VK_CAPITAL
           || key == VK_LSHIFT || key == VK_RSHIFT || key == VK_LCONTROL || key == VK_RCONTROL
           || key == VK_LMENU || key == VK_RMENU || key == VK_LWIN || key == VK_RWIN;
}

suborno::layout::KeyModifiers current_key_modifiers() noexcept {
    return {
        .shift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0,
        .caps_lock = (::GetKeyState(VK_CAPITAL) & 0x0001) != 0,
        .num_lock = (::GetKeyState(VK_NUMLOCK) & 0x0001) != 0,
        .control = (::GetKeyState(VK_CONTROL) & 0x8000) != 0,
        .alt = (::GetKeyState(VK_MENU) & 0x8000) != 0,
        .windows = (::GetKeyState(VK_LWIN) & 0x8000) != 0 || (::GetKeyState(VK_RWIN) & 0x8000) != 0,
    };
}

std::optional<suborno::layout::PhysicalKey> physical_key_from_virtual_key(
    WPARAM virtual_key) noexcept {
    if (virtual_key >= 'A' && virtual_key <= 'Z') {
        const auto key = static_cast<std::uint8_t>(
            static_cast<std::uint8_t>(suborno::layout::PhysicalKey::letter_a)
            + static_cast<std::uint8_t>(virtual_key - 'A'));
        return static_cast<suborno::layout::PhysicalKey>(key);
    }
    if (virtual_key >= '0' && virtual_key <= '9') {
        return static_cast<suborno::layout::PhysicalKey>(static_cast<std::uint8_t>(virtual_key));
    }
    if (virtual_key >= VK_NUMPAD0 && virtual_key <= VK_NUMPAD9) {
        const auto key = static_cast<std::uint8_t>(
            static_cast<std::uint8_t>(suborno::layout::PhysicalKey::numpad_0)
            + static_cast<std::uint8_t>(virtual_key - VK_NUMPAD0));
        return static_cast<suborno::layout::PhysicalKey>(key);
    }

    using suborno::layout::PhysicalKey;
    switch (virtual_key) {
    case VK_OEM_1:
        return PhysicalKey::semicolon;
    case VK_OEM_PLUS:
        return PhysicalKey::equal;
    case VK_OEM_COMMA:
        return PhysicalKey::comma;
    case VK_OEM_MINUS:
        return PhysicalKey::minus;
    case VK_OEM_PERIOD:
        return PhysicalKey::period;
    case VK_OEM_2:
        return PhysicalKey::slash;
    case VK_OEM_3:
        return PhysicalKey::grave;
    case VK_OEM_4:
        return PhysicalKey::left_bracket;
    case VK_OEM_5:
        return PhysicalKey::backslash;
    case VK_OEM_6:
        return PhysicalKey::right_bracket;
    case VK_OEM_7:
        return PhysicalKey::apostrophe;
    case VK_SPACE:
        return PhysicalKey::space;
    default:
        return std::nullopt;
    }
}

bool logical_text_from_keyboard_layout(UINT virtual_key,
                                       UINT scan_code,
                                       bool ignore_caps_lock,
                                       std::string& output) {
    std::array<BYTE, 256> keyboard_state{};
    if (::GetKeyboardState(keyboard_state.data()) == FALSE) {
        return false;
    }
    if (ignore_caps_lock) {
        keyboard_state[VK_CAPITAL] = static_cast<BYTE>(keyboard_state[VK_CAPITAL] & 0xFEU);
    }

    std::array<wchar_t, 8> translated{};
    const HKL keyboard_layout = ::GetKeyboardLayout(0);
    const int translated_count = ::ToUnicodeEx(virtual_key,
                                               scan_code,
                                               keyboard_state.data(),
                                               translated.data(),
                                               static_cast<int>(translated.size()),
                                               0x4,
                                               keyboard_layout);
    if (translated_count <= 0) {
        return true;
    }

    const int required = ::WideCharToMultiByte(CP_UTF8,
                                               WC_ERR_INVALID_CHARS,
                                               translated.data(),
                                               translated_count,
                                               nullptr,
                                               0,
                                               nullptr,
                                               nullptr);
    if (required <= 0) {
        return false;
    }

    std::string converted(static_cast<std::size_t>(required), '\0');
    if (::WideCharToMultiByte(CP_UTF8,
                              WC_ERR_INVALID_CHARS,
                              translated.data(),
                              translated_count,
                              converted.data(),
                              required,
                              nullptr,
                              nullptr)
        != required) {
        return false;
    }
    output = std::move(converted);
    return true;
}

std::optional<suborno::layout::KeyEvent> key_event_from_virtual_key(WPARAM virtual_key,
                                                                    LPARAM lparam,
                                                                    bool ignore_caps_lock,
                                                                    bool capture_logical_text) {
    const auto physical_key = physical_key_from_virtual_key(virtual_key);
    if (!physical_key.has_value()) {
        return std::nullopt;
    }

    const auto message_data = suborno::layout::decode_key_message_lparam(
        static_cast<std::uint32_t>(static_cast<ULONG_PTR>(lparam)));
    suborno::layout::KeyEvent event{
        .key = *physical_key,
        .modifiers = current_key_modifiers(),
        .scan_code = message_data.scan_code,
        .extended_key = message_data.extended_key,
        .repeat_count =
            message_data.repeat_count == 0 ? std::uint16_t{1} : message_data.repeat_count,
        .previous_key_state = message_data.previous_key_state,
        .key_down = message_data.key_down,
    };
    if (capture_logical_text && event.key_down && !event.modifiers.control && !event.modifiers.alt
        && !event.modifiers.windows) {
        event.logical_text_available =
            logical_text_from_keyboard_layout(static_cast<UINT>(virtual_key),
                                              message_data.scan_code,
                                              ignore_caps_lock,
                                              event.logical_text_utf8);
    }
    return event;
}

void apply_input_settings(std::optional<suborno::layout::KeyEvent>& event,
                          const suborno::settings::EngineSettings& settings) noexcept {
    if (!event.has_value()) {
        return;
    }

    if (settings.input.ignore_caps_lock_state_in_bangla) {
        event->modifiers.caps_lock = false;
    }
    if (suborno::layout::is_numpad_key(event->key)) {
        if (!settings.input.map_numpad_to_bangla_numbers
            || !suborno::layout::remap_numpad_to_top_row(*event)) {
            event.reset();
        }
    }
}

bool has_text_modifier_chord() noexcept {
    const auto modifiers = current_key_modifiers();
    return modifiers.control || modifiers.alt || modifiers.windows;
}

std::optional<std::size_t> candidate_index_from_virtual_key(WPARAM virtual_key) noexcept {
    const auto physical_key = physical_key_from_virtual_key(virtual_key);
    if (!physical_key.has_value()) {
        return std::nullopt;
    }
    const suborno::layout::KeyEvent key_event{*physical_key, current_key_modifiers()};
    const auto character = suborno::layout::key_event_to_ascii(key_event);
    if (!character.has_value() || *character < '1' || *character > '9') {
        return std::nullopt;
    }
    return static_cast<std::size_t>(*character - '1');
}

}  // namespace

SubornoTextService::SubornoTextService() {
    com::module_lifetime().object_created();
}

std::string SubornoTextService::effective_layout_id(const ContextState& state) const {
    if (m_engine->settings().general.switching_mode
            == suborno::settings::SwitchingMode::per_application_window
        && !state.layout_override.empty()) {
        return state.layout_override;
    }
    return std::string(m_engine->active_layout_id());
}

HRESULT SubornoTextService::_EnsureSession(ITfContext* context, ContextState& state) {
    const std::string layout_id = effective_layout_id(state);
    const bool session_is_idle =
        state.session == nullptr || state.session->snapshot().raw_input_utf8.empty();
    if (state.session == nullptr
        || (session_is_idle
            && (state.session_needs_refresh || state.session_layout_id != layout_id))) {
        state.session = m_engine->create_session_for_layout(layout_id);
        state.session_layout_id = layout_id;
        state.session_needs_refresh = false;
    }
    // BornoTSF parity: every context this service types into carries an

    // ITfTextEditSink advice (upstream advises the focused context's top

    // context; Suborno's composition state is per-context).

    if (context != nullptr
        && (state.text_edit_sink_context.get() != context
            || state.text_edit_sink_cookie == TF_INVALID_COOKIE)) {
        static_cast<void>(_InitTextEditSink(context, state));
    }
    return state.session == nullptr ? E_UNEXPECTED : S_OK;
}

void SubornoTextService::_ApplyLayoutChange(ITfContext* context, const std::string& layout_id) {
    const suborno::settings::SwitchingMode mode = m_engine->settings().general.switching_mode;
    if (mode == suborno::settings::SwitchingMode::global) {
        // Global mode: every context follows the engine; forget any stale

        // per-window override from a previous settings combination.

        for (auto& [context_ptr, state] : m_contexts) {
            static_cast<void>(context_ptr);
            state.layout_override.clear();
            state.session_needs_refresh = true;
        }
        return;
    }
    // Per-application-window mode: only the context the user is typing in

    // picks up the new layout; other windows keep their own.

    const auto found = m_contexts.find(context);
    if (found != m_contexts.end()) {
        found->second.layout_override = layout_id;
        found->second.session_needs_refresh = true;
    }
}

std::wstring SubornoTextService::_CommittedTextToUtf16(const std::string& text_utf8) const {
    // The committed string is Unicode Bangla from the layout; Bijoy ANSI

    // mode re-encodes it (with a Unicode fallback) before the edit session.

    return utf8_to_utf16(m_engine->encode_text_for_output(text_utf8));
}

void SubornoTextService::_RecordCommit(const std::string& text_utf8) {
    if (m_engine == nullptr || text_utf8.empty()) {
        return;
    }
    // The space-commit path appends a trailing space before this point; the

    // learning model wants the bare word.

    std::string word = text_utf8;
    while (!word.empty() && (word.back() == ' ' || word.back() == '\t')) {
        word.pop_back();
    }
    if (!word.empty()) {
        m_engine->record_committed_word(word);
    }
}

void SubornoTextService::_UpdateInputIndicators() {
    if (m_threadMgr == nullptr || m_engine == nullptr) {
        return;
    }
    const std::string active_layout(m_engine->active_layout_id());
    if (active_layout == m_indicator_layout_id) {
        return;
    }
    m_indicator_layout_id = active_layout;

    const bool bangla = m_engine->active_layout_kind() != layout::LayoutKind::passthrough;
    // The keyboard open/close compartment is what the Windows input

    // indicator (and the language bar) reflects: open = composing IME (the

    // Bangla state), closed = direct typing (the English state). This is the

    // same mechanism the Japanese IME uses for its Kana/Direct toggle.

    wil::com_ptr<ITfCompartmentMgr> compartments;
    if (SUCCEEDED(m_threadMgr->QueryInterface(IID_PPV_ARGS(compartments.put())))
        && compartments != nullptr) {
        wil::com_ptr<ITfCompartment> compartment;
        if (SUCCEEDED(compartments->GetCompartment(GUID_COMPARTMENT_KEYBOARD_OPENCLOSE,
                                                   compartment.put()))
            && compartment != nullptr) {
            VARIANT value{};
            value.vt = VT_I4;
            value.lVal = bangla ? 1 : 0;
            static_cast<void>(compartment->SetValue(0, &value));
        }
    }

    // Locale option: when a Bangla layout becomes active, make sure this

    // thread's input language is the Suborno bn-BD profile (the user may have

    // Alt+Shift'ed away); with the option disabled Suborno never re-grabs the

    // thread. Best effort: a failed activation never breaks typing.

    if (bangla && m_engine->settings().input.change_locale_to_bn_bd_when_layout_not_english) {
        wil::com_ptr<ITfInputProcessorProfiles> profiles;
        if (SUCCEEDED(::CoCreateInstance(CLSID_TF_InputProcessorProfiles,
                                         nullptr,
                                         CLSCTX_INPROC_SERVER,
                                         IID_PPV_ARGS(profiles.put())))
            && profiles != nullptr) {
            static_cast<void>(
                profiles->ActivateLanguageProfile(CLSID_SubornoTextService,
                                                  bengali_bangladesh,
                                                  GUID_LANGUAGE_PROFILE_SUBORNO_BENGALI));
        }
    }
}

SubornoTextService::~SubornoTextService() {
    com::module_lifetime().object_destroyed();
}

STDMETHODIMP SubornoTextService::QueryInterface(REFIID riid, void** ppvObj) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (ppvObj == nullptr) {
            return E_POINTER;
        }
        *ppvObj = nullptr;

        if (riid == IID_IUnknown || riid == IID_ITfTextInputProcessor) {
            *ppvObj = static_cast<ITfTextInputProcessor*>(this);
        } else if (riid == IID_ITfKeyEventSink) {
            *ppvObj = static_cast<ITfKeyEventSink*>(this);
        } else if (riid == IID_ITfThreadMgrEventSink) {
            *ppvObj = static_cast<ITfThreadMgrEventSink*>(this);
        } else if (riid == IID_ITfCompositionSink) {
            *ppvObj = static_cast<ITfCompositionSink*>(this);
        } else if (riid == IID_ITfTextEditSink) {
            *ppvObj = static_cast<ITfTextEditSink*>(this);
        } else if (riid == IID_ITfThreadFocusSink) {
            *ppvObj = static_cast<ITfThreadFocusSink*>(this);
        } else if (riid == IID_ITfDisplayAttributeProvider) {
            *ppvObj = static_cast<ITfDisplayAttributeProvider*>(this);
        } else {
            return E_NOINTERFACE;
        }

        AddRef();
        return S_OK;
    });
}

STDMETHODIMP_(ULONG) SubornoTextService::AddRef() {
    return static_cast<ULONG>(::InterlockedIncrement(&m_refCount));
}

STDMETHODIMP_(ULONG) SubornoTextService::Release() {
    const ULONG count = static_cast<ULONG>(::InterlockedDecrement(&m_refCount));
    if (count == 0) {
        delete this;
    }
    return count;
}

STDMETHODIMP SubornoTextService::Activate(ITfThreadMgr* pThreadMgr, TfClientId clientId) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (pThreadMgr == nullptr || clientId == TF_CLIENTID_NULL) {
            return E_INVALIDARG;
        }

        m_threadMgr = pThreadMgr;
        m_clientId = clientId;
        try {
            std::filesystem::path user_data;
            std::filesystem::path legacy_user_data;
            HRESULT path_result = get_user_data_directory(user_data);
            if (SUCCEEDED(path_result)) {
                path_result = get_legacy_user_data_directory(legacy_user_data);
            }
            if (SUCCEEDED(path_result)) {
                path_result = prepare_user_data_directory(user_data, legacy_user_data);
            }
            if (FAILED(path_result)) {
                m_threadMgr.reset();
                m_clientId = TF_CLIENTID_NULL;
                return path_result;
            }
            core::EngineConfig config;
            config.settings_path = user_data / L"settings.ini";
            config.dictionary_path = user_data / L"dictionaries" / L"user_candidates.db";
            std::filesystem::path assets_path;
            if (SUCCEEDED(get_shared_assets_directory(assets_path))) {
                config.assets_path = assets_path;
            }
            m_engine = std::make_unique<core::Engine>(config);
        } catch (const std::bad_alloc&) {
            m_threadMgr.reset();
            m_clientId = TF_CLIENTID_NULL;
            return E_OUTOFMEMORY;
        } catch (...) {
            m_threadMgr.reset();
            m_clientId = TF_CLIENTID_NULL;
            return E_FAIL;
        }

        wil::com_ptr<ITfSource> source;
        HRESULT hr = m_threadMgr->QueryInterface(IID_PPV_ARGS(source.put()));
        if (FAILED(hr)) {
            m_engine.reset();
            m_threadMgr.reset();
            m_clientId = TF_CLIENTID_NULL;
            return hr;
        }
        hr = source->AdviseSink(IID_ITfThreadMgrEventSink,
                                static_cast<ITfThreadMgrEventSink*>(this),
                                &m_threadMgrSinkCookie);
        if (FAILED(hr)) {
            m_engine.reset();
            m_threadMgr.reset();
            m_clientId = TF_CLIENTID_NULL;
            return hr;
        }

        hr = m_threadMgr->QueryInterface(IID_PPV_ARGS(m_keystrokeMgr.put()));
        if (SUCCEEDED(hr)) {
            hr = m_keystrokeMgr->AdviseKeyEventSink(m_clientId, this, TRUE);
        }
        if (FAILED(hr)) {
            source->UnadviseSink(m_threadMgrSinkCookie);
            m_threadMgrSinkCookie = TF_INVALID_COOKIE;
            m_keystrokeMgr.reset();
            m_engine.reset();
            m_threadMgr.reset();
            m_clientId = TF_CLIENTID_NULL;
            return hr;
        }

        hr = m_threadMgr->QueryInterface(IID_PPV_ARGS(m_uiElementMgr.put()));
        if (FAILED(hr)) {
            m_keystrokeMgr->UnadviseKeyEventSink(m_clientId);
            source->UnadviseSink(m_threadMgrSinkCookie);
            m_threadMgrSinkCookie = TF_INVALID_COOKIE;
            m_keystrokeMgr.reset();
            m_engine.reset();
            m_threadMgr.reset();
            m_clientId = TF_CLIENTID_NULL;
            return hr;
        }
        // Seed the indicator tracking with the restored layout so the first

        // focus/keystroke does not redundantly re-apply the startup state.

        m_indicator_layout_id = std::string(m_engine->active_layout_id());

        // BornoTSF parity: register the display-attribute GUIDs with the category

        // manager (upstream Activate fails when this fails — _InitDisplayAttributeGuidAtom

        // gates BornoTSF::Activate the same way).

        hr = _InitDisplayAttributeGuidAtom();
        if (FAILED(hr)) {
            m_uiElementMgr.reset();
            m_keystrokeMgr->UnadviseKeyEventSink(m_clientId);
            wil::com_ptr<ITfSource> source_cleanup;
            if (SUCCEEDED(m_threadMgr->QueryInterface(IID_PPV_ARGS(source_cleanup.put())))) {
                static_cast<void>(source_cleanup->UnadviseSink(m_threadMgrSinkCookie));
            }
            m_threadMgrSinkCookie = TF_INVALID_COOKIE;
            m_keystrokeMgr.reset();
            m_engine.reset();
            m_threadMgr.reset();
            m_clientId = TF_CLIENTID_NULL;
            return hr;
        }

        // BornoTSF parity: advise the thread focus sink (upstream Activate fails

        // when this fails).

        hr = _InitThreadFocusSink();
        if (FAILED(hr)) {
            m_uiElementMgr.reset();
            m_keystrokeMgr->UnadviseKeyEventSink(m_clientId);
            wil::com_ptr<ITfSource> source_cleanup;
            if (SUCCEEDED(m_threadMgr->QueryInterface(IID_PPV_ARGS(source_cleanup.put())))) {
                static_cast<void>(source_cleanup->UnadviseSink(m_threadMgrSinkCookie));
            }
            m_threadMgrSinkCookie = TF_INVALID_COOKIE;
            m_keystrokeMgr.reset();
            m_engine.reset();
            m_threadMgr.reset();
            m_clientId = TF_CLIENTID_NULL;
            return hr;
        }

        // BornoTSF parity: advise compartment event sinks on the same two

        // compartments as CCompositionProcessorEngine::SetupLanguageBar. Best

        // effort — upstream tolerates the sink being absent too.

        m_compartmentOpenCloseSink.reset(new (std::nothrow) CompartmentEventSink(
            &SubornoTextService::CompartmentChangeCallback, this));
        if (m_compartmentOpenCloseSink != nullptr) {
            static_cast<void>(m_compartmentOpenCloseSink->Advise(
                m_threadMgr.get(), GUID_COMPARTMENT_KEYBOARD_OPENCLOSE));
            // Upstream sets the open/close compartment to TRUE right after

            // advising; Suborno writes its bangla-aware value instead of a

            // hardcoded TRUE so the input indicator matches the active layout.

            const bool bangla = m_engine->active_layout_kind() != layout::LayoutKind::passthrough;
            const Compartment open_close(
                m_threadMgr.get(), m_clientId, GUID_COMPARTMENT_KEYBOARD_OPENCLOSE);
            static_cast<void>(open_close.set_bool_value(bangla ? TRUE : FALSE));
        }
        m_compartmentConversionSink.reset(new (std::nothrow) CompartmentEventSink(
            &SubornoTextService::CompartmentChangeCallback, this));
        if (m_compartmentConversionSink != nullptr) {
            static_cast<void>(m_compartmentConversionSink->Advise(
                m_threadMgr.get(), GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION));
        }

        _UpdateInputIndicators();
        return S_OK;
    });
}

STDMETHODIMP SubornoTextService::Deactivate() {
    return run_inside_com_boundary([&]() -> HRESULT {
        HRESULT first_error = S_OK;
        try {
            for (auto& [context, state] : m_contexts) {
                const HRESULT ui_result = _EndCandidateUI(state);
                if (FAILED(ui_result) && SUCCEEDED(first_error)) {
                    first_error = ui_result;
                }
                if (state.session != nullptr && state.composition != nullptr) {
                    std::string text_utf8;
                    text_utf8 = state.session->commit().text_utf8;
                    const std::wstring text = _CommittedTextToUtf16(text_utf8);
                    if (!text_utf8.empty() && text.empty() && SUCCEEDED(first_error)) {
                        first_error = E_INVALIDARG;
                    }
                    const HRESULT finish_result = _FinishComposition(context, state, text);
                    if (FAILED(finish_result) && SUCCEEDED(first_error)) {
                        first_error = finish_result;
                    }
                }
            }
        } catch (const std::bad_alloc&) {
            first_error = E_OUTOFMEMORY;
        } catch (...) {
            first_error = E_FAIL;
        }
        if (m_keystrokeMgr != nullptr && m_clientId != TF_CLIENTID_NULL) {
            const HRESULT hr = m_keystrokeMgr->UnadviseKeyEventSink(m_clientId);
            if (FAILED(hr) && SUCCEEDED(first_error)) {
                first_error = hr;
            }
        }
        m_keystrokeMgr.reset();
        m_uiElementMgr.reset();
        // BornoTSF parity: release the compartment event sinks (upstream deletes

        // them after _Unadvise in CCompositionProcessorEngine cleanup).

        if (m_compartmentOpenCloseSink != nullptr) {
            static_cast<void>(m_compartmentOpenCloseSink->Unadvise());
            m_compartmentOpenCloseSink.reset();
        }
        if (m_compartmentConversionSink != nullptr) {
            static_cast<void>(m_compartmentConversionSink->Unadvise());
            m_compartmentConversionSink.reset();
        }
        // BornoTSF parity: unadvise the thread focus sink.

        _UninitThreadFocusSink();
        if (m_threadMgr != nullptr && m_threadMgrSinkCookie != TF_INVALID_COOKIE) {
            wil::com_ptr<ITfSource> source;
            const HRESULT query_result = m_threadMgr->QueryInterface(IID_PPV_ARGS(source.put()));
            const HRESULT hr =
                FAILED(query_result) ? query_result : source->UnadviseSink(m_threadMgrSinkCookie);
            if (FAILED(hr) && SUCCEEDED(first_error)) {
                first_error = hr;
            }
        }
        m_threadMgrSinkCookie = TF_INVALID_COOKIE;
        m_threadMgr.reset();
        m_clientId = TF_CLIENTID_NULL;
        for (auto& [context, state] : m_contexts) {
            static_cast<void>(context);
            _ShutdownTextEditSink(state);
        }
        m_contexts.clear();
        m_indicator_layout_id.clear();
        m_gaDisplayAttributeInput = 0;
        m_gaDisplayAttributeConverted = 0;
        m_engine.reset();
        return first_error;
    });
}

STDMETHODIMP SubornoTextService::OnInitDocumentMgr(ITfDocumentMgr*) {
    return run_inside_com_boundary([&]() -> HRESULT {
        return S_OK;
    });
}

STDMETHODIMP SubornoTextService::OnUninitDocumentMgr(ITfDocumentMgr* pDocMgr) {
    return run_inside_com_boundary([&]() -> HRESULT {
        // The document manager (and every context in it) is going away. Without

        // this cleanup the m_contexts map keeps AddRef'd ITfContext pointers for

        // documents that no longer exist, so the entries leak and future lookups

        // could resurrect stale composition state.

        if (pDocMgr != nullptr) {
            wil::com_ptr<IEnumTfContexts> context_enum;
            if (SUCCEEDED(pDocMgr->EnumContexts(context_enum.put())) && context_enum != nullptr) {
                ULONG fetched = 0;
                ITfContext* contexts[16] = {};
                while (SUCCEEDED(context_enum->Next(16, contexts, &fetched)) && fetched > 0) {
                    for (ULONG index = 0; index < fetched; ++index) {
                        const auto found = m_contexts.find(contexts[index]);
                        if (found != m_contexts.end()) {
                            _ShutdownTextEditSink(found->second);
                            m_contexts.erase(found);
                        }
                        contexts[index]->Release();
                    }
                }
            }
        }
        return S_OK;
    });
}

STDMETHODIMP SubornoTextService::OnTestKeyDown(ITfContext* pContext,
                                               WPARAM wParam,
                                               LPARAM lParam,
                                               BOOL* pbEaten) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (pbEaten == nullptr) {
            return E_INVALIDARG;
        }
        *pbEaten = FALSE;
        if (pContext == nullptr || m_engine == nullptr || is_modifier_key(wParam)) {
            return S_OK;
        }
        try {
            // The desktop shell saves layout/settings changes to the shared

            // settings file. Without this change-detected reload the in-memory

            // engine stays stale and every new keystroke keeps using the previous

            // layout (the "English letters instead of Bangla" bug) until the app

            // is refocused.

            m_engine->reload_settings_if_changed();
            // A settings-driven layout change is applied to the receiving context

            // right here so the eat decision below already reflects the layout

            // the matching OnKeyDown will use (per-window overrides included).

            const std::string engine_layout(m_engine->active_layout_id());
            if (engine_layout != m_indicator_layout_id) {
                _ApplyLayoutChange(pContext, engine_layout);
                _UpdateInputIndicators();
            }
            // Per-application-window switching: this context may type with its

            // own layout even though the engine holds a different one.

            const auto found_for_kind = m_contexts.find(pContext);
            const std::string effective_layout = found_for_kind == m_contexts.end()
                                                     ? engine_layout
                                                     : effective_layout_id(found_for_kind->second);
            const layout::LayoutKind effective_kind = m_engine->layout_kind_of(effective_layout);
            if (effective_kind == layout::LayoutKind::passthrough) {
                // English mode: keys flow to the host application untouched.

                return S_OK;
            }
            if (has_text_modifier_chord()) {
                return S_OK;
            }
            const auto found = m_contexts.find(pContext);
            const bool composing = found != m_contexts.end() && found->second.session != nullptr
                                   && !found->second.session->snapshot().raw_input_utf8.empty();
            const auto& input_settings = m_engine->settings().input;
            auto key_event =
                key_event_from_virtual_key(wParam,
                                           lParam,
                                           input_settings.ignore_caps_lock_state_in_bangla,
                                           effective_kind != layout::LayoutKind::fixed);
            apply_input_settings(key_event, m_engine->settings());
            const bool printable =
                key_event.has_value()
                && (key_event->logical_text_available
                        ? !key_event->logical_text_utf8.empty()
                        : suborno::layout::key_event_to_ascii(*key_event).has_value());
            const bool starts_phonetic_input =
                key_event.has_value()
                && (key_event->logical_text_available
                        ? key_event->logical_text_utf8.size() == 1
                              && ((key_event->logical_text_utf8.front() >= 'A'
                                   && key_event->logical_text_utf8.front() <= 'Z')
                                  || (key_event->logical_text_utf8.front() >= 'a'
                                      && key_event->logical_text_utf8.front() <= 'z'))
                        : wParam >= 'A' && wParam <= 'Z');
            const bool starts_numpad_input = key_event.has_value()
                                             && input_settings.map_numpad_to_bangla_numbers
                                             && wParam >= VK_NUMPAD0 && wParam <= VK_NUMPAD9;
            const bool fixed_or_compositional =
                effective_kind != suborno::layout::LayoutKind::phonetic;
            const bool starts_composition =
                fixed_or_compositional ? printable : (starts_phonetic_input || starts_numpad_input);
            *pbEaten = (composing || starts_composition) && (composing || wParam != VK_SPACE);
            return S_OK;
        } catch (const std::bad_alloc&) {
            return E_OUTOFMEMORY;
        } catch (...) {
            return E_FAIL;
        }
    });
}

STDMETHODIMP SubornoTextService::OnTestKeyUp(ITfContext*, WPARAM, LPARAM, BOOL* pbEaten) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (pbEaten == nullptr) {
            return E_INVALIDARG;
        }
        *pbEaten = FALSE;
        return S_OK;
    });
}

STDMETHODIMP SubornoTextService::OnKeyDown(ITfContext* pContext,
                                           WPARAM wParam,
                                           LPARAM lParam,
                                           BOOL* pbEaten) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (pbEaten == nullptr) {
            return E_INVALIDARG;
        }
        *pbEaten = FALSE;
        if (pContext == nullptr || m_engine == nullptr) {
            return E_INVALIDARG;
        }
        if (is_modifier_key(wParam) || has_text_modifier_chord()) {
            return S_OK;
        }

        try {
            // Same change-detected reload as OnTestKeyDown: layout switches made

            // in the desktop shell take effect on the very next keystroke.

            m_engine->reload_settings_if_changed();
            const std::string engine_layout(m_engine->active_layout_id());
            if (engine_layout != m_indicator_layout_id) {
                _ApplyLayoutChange(pContext, engine_layout);
                _UpdateInputIndicators();
            }
            const auto found_for_kind = m_contexts.find(pContext);
            const std::string effective_layout = found_for_kind == m_contexts.end()
                                                     ? engine_layout
                                                     : effective_layout_id(found_for_kind->second);
            if (m_engine->layout_kind_of(effective_layout) == layout::LayoutKind::passthrough) {
                // Switching to English mid-composition commits what is on screen,

                // then hands the key to the host application.

                *pbEaten = FALSE;
                const auto found = m_contexts.find(pContext);
                if (found != m_contexts.end() && found->second.session != nullptr
                    && !found->second.session->snapshot().raw_input_utf8.empty()) {
                    const auto committed = found->second.session->commit();
                    const std::wstring text = _CommittedTextToUtf16(committed.text_utf8);
                    if (!committed.text_utf8.empty() && text.empty()) {
                        return E_INVALIDARG;
                    }
                    _RecordCommit(committed.text_utf8);
                    return _FinishComposition(pContext, found->second, text);
                }
                return S_OK;
            }

            auto [found, inserted] = m_contexts.try_emplace(pContext);
            ContextState& state = found->second;
            if (inserted) {
                state.context = pContext;
            }
            if (_EnsureSession(pContext, state) != S_OK) {
                return E_UNEXPECTED;
            }

            const auto snapshot = state.session->snapshot();
            const bool composing = !snapshot.raw_input_utf8.empty();
            if (!composing && wParam == VK_SPACE) {
                return S_OK;
            }

            if (composing && wParam == VK_BACK) {
                state.session->backspace();
                const auto updated = state.session->snapshot();
                const HRESULT hr =
                    updated.raw_input_utf8.empty()
                        ? _FinishComposition(pContext, state, {})
                        : _UpdateComposition(pContext, state, utf8_to_utf16(updated.preedit_utf8));
                if (SUCCEEDED(hr) && !updated.raw_input_utf8.empty()) {
                    _UpdateCandidateUI(pContext, state, updated);
                }
                if (SUCCEEDED(hr)) {
                    *pbEaten = TRUE;
                }
                return hr;
            }
            if (composing && wParam == VK_ESCAPE) {
                state.session->reset();
                const HRESULT hr = _FinishComposition(pContext, state, {});
                if (SUCCEEDED(hr)) {
                    *pbEaten = TRUE;
                }
                return hr;
            }
            if (composing && (wParam == VK_PRIOR || wParam == VK_NEXT)) {
                const auto current = state.session->snapshot();
                if (!current.candidates.empty() && state.candidate_ui_element != nullptr) {
                    UINT page_count = 0;
                    UINT current_page = 0;
                    HRESULT page_result =
                        state.candidate_ui_element->GetPageIndex(nullptr, 0, &page_count);
                    if (SUCCEEDED(page_result) && page_count > 0
                        && SUCCEEDED(state.candidate_ui_element->GetCurrentPage(&current_page))) {
                        std::vector<UINT> page_starts(page_count);
                        page_result = state.candidate_ui_element->GetPageIndex(
                            page_starts.data(), page_count, &page_count);
                        if (SUCCEEDED(page_result)) {
                            const bool has_adjacent_page = wParam == VK_NEXT
                                                               ? current_page + 1 < page_count
                                                               : current_page > 0;
                            if (has_adjacent_page) {
                                const UINT destination_page =
                                    wParam == VK_NEXT ? current_page + 1 : current_page - 1;
                                const std::size_t destination = page_starts[destination_page];
                                if (state.session->select_candidate(destination)) {
                                    const auto updated = state.session->snapshot();
                                    _UpdateCandidateUI(pContext, state, updated);
                                    *pbEaten = TRUE;
                                }
                            }
                        }
                    }
                }
                return S_OK;
            }
            if (composing && (wParam == VK_UP || wParam == VK_DOWN)) {
                const bool changed = wParam == VK_DOWN ? state.session->select_next_candidate()
                                                       : state.session->select_previous_candidate();
                if (changed) {
                    const auto updated = state.session->snapshot();
                    _UpdateCandidateUI(pContext, state, updated);
                    *pbEaten = TRUE;
                }
                return S_OK;
            }
            if (composing) {
                const auto candidate_index =
                    m_engine->settings().input.select_candidate_using_number_keys
                        ? candidate_index_from_virtual_key(wParam)
                        : std::nullopt;
                if (candidate_index.has_value() && *candidate_index < snapshot.candidates.size()) {
                    const auto committed = state.session->commit_candidate(*candidate_index);
                    const std::wstring text = _CommittedTextToUtf16(committed.text_utf8);
                    if (!committed.text_utf8.empty() && text.empty()) {
                        return E_INVALIDARG;
                    }
                    _RecordCommit(committed.text_utf8);
                    const HRESULT hr = _FinishComposition(pContext, state, text);
                    if (SUCCEEDED(hr)) {
                        *pbEaten = TRUE;
                    }
                    return hr;
                }
            }
            if (composing && wParam == VK_SPACE) {
                const auto committed = state.session->commit();
                const std::wstring text = _CommittedTextToUtf16(committed.text_utf8);
                if (!committed.text_utf8.empty() && text.empty()) {
                    return E_INVALIDARG;
                }
                _RecordCommit(committed.text_utf8);
                std::wstring with_space = text;
                with_space.push_back(L' ');
                const HRESULT hr = _FinishComposition(pContext, state, with_space);
                if (SUCCEEDED(hr)) {
                    *pbEaten = TRUE;
                }
                return hr;
            }

            const auto& input_settings = m_engine->settings().input;
            auto key_event = key_event_from_virtual_key(
                wParam,
                lParam,
                input_settings.ignore_caps_lock_state_in_bangla,
                m_engine->layout_kind_of(state.session_layout_id) != layout::LayoutKind::fixed);
            apply_input_settings(key_event, m_engine->settings());
            const bool starts_phonetic_input =
                key_event.has_value() && !key_event->logical_text_utf8.empty()
                && key_event->logical_text_utf8.size() == 1
                && ((key_event->logical_text_utf8.front() >= 'A'
                     && key_event->logical_text_utf8.front() <= 'Z')
                    || (key_event->logical_text_utf8.front() >= 'a'
                        && key_event->logical_text_utf8.front() <= 'z'));
            const bool starts_numpad_input = key_event.has_value()
                                             && input_settings.map_numpad_to_bangla_numbers
                                             && wParam >= VK_NUMPAD0 && wParam <= VK_NUMPAD9;
            const bool fixed_or_compositional =
                m_engine->layout_kind_of(state.session_layout_id) != layout::LayoutKind::phonetic;
            const bool should_append_key = key_event.has_value()
                                           && (composing || fixed_or_compositional
                                               || starts_phonetic_input || starts_numpad_input);
            if (should_append_key) {
                const layout::InputStatus input_status = state.session->append_key(*key_event);
                if (input_status == layout::InputStatus::invalid_utf8
                    || input_status == layout::InputStatus::input_limit
                    || input_status == layout::InputStatus::empty) {
                    return S_OK;
                }
                const auto updated = state.session->snapshot();
                const std::wstring preedit = utf8_to_utf16(updated.preedit_utf8);
                if (preedit.empty()) {
                    return E_FAIL;
                }
                const HRESULT hr = _UpdateComposition(pContext, state, preedit);
                if (SUCCEEDED(hr)) {
                    _UpdateCandidateUI(pContext, state, updated);
                    *pbEaten = TRUE;
                }
                return hr;
            }

            if (!composing) {
                return S_OK;
            }

            const auto committed = state.session->commit();
            const std::wstring text = _CommittedTextToUtf16(committed.text_utf8);
            if (!committed.text_utf8.empty() && text.empty()) {
                return E_INVALIDARG;
            }
            _RecordCommit(committed.text_utf8);
            const HRESULT hr = _FinishComposition(pContext, state, text);
            if (FAILED(hr)) {
                return hr;
            }
            return S_OK;
        } catch (const std::bad_alloc&) {
            return E_OUTOFMEMORY;
        } catch (...) {
            return E_FAIL;
        }
    });
}

STDMETHODIMP SubornoTextService::OnKeyUp(ITfContext*, WPARAM, LPARAM, BOOL* pbEaten) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (pbEaten == nullptr) {
            return E_INVALIDARG;
        }
        *pbEaten = FALSE;
        return S_OK;
    });
}

STDMETHODIMP SubornoTextService::OnSetFocus(BOOL foreground) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (foreground == FALSE || m_engine == nullptr) {
            return S_OK;
        }

        // A failed settings reload (e.g. a corrupt or unreadable settings file)

        // must not break focus handling: reload keeps the last good settings in

        // that case, so continue with the currently active layout. The reload is

        // change-detected: focus churn no longer re-reads and re-validates the

        // settings file on every focus change.

        m_engine->reload_settings_if_changed();
        _UpdateInputIndicators();

        try {
            for (auto& [context, state] : m_contexts) {
                const auto current = state.session == nullptr ? layout::CompositionSnapshot{}
                                                              : state.session->snapshot();
                if (state.session == nullptr || current.raw_input_utf8.empty()) {
                    // Per-application-window mode: a context with its own layout

                    // override gets a session for THAT layout, not the engine's.

                    if (_EnsureSession(context, state) != S_OK) {
                        return E_UNEXPECTED;
                    }
                } else {
                    state.session_needs_refresh = true;
                    const HRESULT ui_result = _UpdateCandidateUI(context, state, current);
                    if (FAILED(ui_result)) {
                        return ui_result;
                    }
                }
            }
        } catch (const std::bad_alloc&) {
            return E_OUTOFMEMORY;
        } catch (...) {
            return E_FAIL;
        }
        return S_OK;
    });
}

STDMETHODIMP SubornoTextService::OnPreservedKey(ITfContext*, REFGUID, BOOL* pbEaten) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (pbEaten == nullptr) {
            return E_INVALIDARG;
        }
        *pbEaten = FALSE;
        return S_OK;
    });
}

STDMETHODIMP SubornoTextService::OnSetFocus(ITfDocumentMgr*, ITfDocumentMgr*) {
    return run_inside_com_boundary([&]() -> HRESULT {
        return S_OK;
    });
}

STDMETHODIMP SubornoTextService::OnPushContext(ITfContext* pContext) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (pContext == nullptr || m_engine == nullptr) {
            return E_INVALIDARG;
        }
        try {
            auto [found, inserted] = m_contexts.try_emplace(pContext);
            if (inserted) {
                found->second.context = pContext;
            }
            if (_EnsureSession(pContext, found->second) != S_OK) {
                return E_UNEXPECTED;
            }
            return S_OK;
        } catch (const std::bad_alloc&) {
            return E_OUTOFMEMORY;
        } catch (...) {
            return E_FAIL;
        }
    });
}

STDMETHODIMP SubornoTextService::OnPopContext(ITfContext* pContext) {
    return run_inside_com_boundary([&]() -> HRESULT {
        const auto found = m_contexts.find(pContext);
        if (found == m_contexts.end()) {
            return S_OK;
        }
        _ShutdownTextEditSink(found->second);
        HRESULT result = _EndCandidateUI(found->second);
        try {
            if (found->second.session != nullptr && found->second.composition != nullptr) {
                const auto committed = found->second.session->commit();
                const std::wstring text = _CommittedTextToUtf16(committed.text_utf8);
                if (!committed.text_utf8.empty() && text.empty()) {
                    result = E_INVALIDARG;
                } else {
                    _RecordCommit(committed.text_utf8);
                    result = _FinishComposition(pContext, found->second, text);
                }
            }
        } catch (const std::bad_alloc&) {
            result = E_OUTOFMEMORY;
        } catch (...) {
            result = E_FAIL;
        }
        m_contexts.erase(found);
        return result;
    });
}

STDMETHODIMP SubornoTextService::OnCompositionTerminated(TfEditCookie edit_cookie,
                                                         ITfComposition* composition) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (composition == nullptr) {
            return E_INVALIDARG;
        }
        for (auto& [context, state] : m_contexts) {
            if (state.composition.get() == composition) {
                _EndCandidateUI(state);
                // BornoTSF parity: the host just force-terminated the

                // composition — clear the display attribute with the write

                // cookie the notification provides.

                static_cast<void>(
                    _ClearCompositionDisplayAttributes(edit_cookie, context, composition));
                state.composition.reset();
                if (state.session != nullptr) {
                    state.session->reset();
                }
                break;
            }
        }
        return S_OK;
    });
}

STDMETHODIMP SubornoTextService::OnEndEdit(ITfContext* pContext,
                                           TfEditCookie ecReadOnly,
                                           ITfEditRecord* pEditRecord) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (pEditRecord == nullptr) {
            return E_INVALIDARG;
        }
        BOOL selection_changed = FALSE;
        if (!SUCCEEDED(pEditRecord->GetSelectionStatus(&selection_changed)) || !selection_changed) {
            return S_OK;
        }

        const auto found = m_contexts.find(pContext);
        if (found == m_contexts.end() || found->second.composition == nullptr) {
            return S_OK;
        }
        if (pContext == nullptr) {
            return E_INVALIDARG;
        }

        TF_SELECTION selection{};
        ULONG fetched = 0;
        if (FAILED(
                pContext->GetSelection(ecReadOnly, TF_DEFAULT_SELECTION, 1, &selection, &fetched))
            || fetched != 1) {
            return S_FALSE;
        }

        ContextState& state = found->second;
        wil::com_ptr<ITfRange> composition_range;
        if (SUCCEEDED(state.composition->GetRange(composition_range.put()))
            && composition_range != nullptr) {
            if (!_IsRangeCovered(ecReadOnly, selection.range, composition_range.get())) {
                // BornoTSF parity (_EndComposition → _TerminateComposition):

                // keep the composed text exactly as it is on screen, clear the

                // display attribute and end the composition. The session resets

                // so the next keystroke starts a fresh composition.

                static_cast<void>(TextCompositionEditSession::terminate_composition(
                    pContext, m_clientId, state.composition.get()));
                state.composition.reset();
                if (state.session != nullptr) {
                    state.session->reset();
                }
            }
        }

        if (selection.range != nullptr) {
            selection.range->Release();
        }
        return S_OK;
    });
}

STDMETHODIMP SubornoTextService::OnSetThreadFocus() {
    return run_inside_com_boundary([&]() -> HRESULT {
        // BornoTSF parity: upstream's handlers are intentionally empty.

        return S_OK;
    });
}

STDMETHODIMP SubornoTextService::OnKillThreadFocus() {
    return run_inside_com_boundary([&]() -> HRESULT {
        // BornoTSF parity: upstream's handlers are intentionally empty.

        return S_OK;
    });
}

STDMETHODIMP SubornoTextService::EnumDisplayAttributeInfo(
    __RPC__deref_out_opt IEnumTfDisplayAttributeInfo** ppEnum) {
    if (ppEnum == nullptr) {
        return E_INVALIDARG;
    }
    *ppEnum = nullptr;

    return run_inside_com_boundary([&]() -> HRESULT {
        // The method name shadows the enum class type inside member scope, so

        // the type is named with its namespace qualification.

        auto* attribute_enum =
            new (std::nothrow) suborno::platform::windows::EnumDisplayAttributeInfo();
        if (attribute_enum == nullptr) {
            return E_OUTOFMEMORY;
        }
        *ppEnum = attribute_enum;
        return S_OK;
    });
}

STDMETHODIMP SubornoTextService::GetDisplayAttributeInfo(
    __RPC__in REFGUID guidInfo, __RPC__deref_out_opt ITfDisplayAttributeInfo** ppInfo) {
    if (ppInfo == nullptr) {
        return E_INVALIDARG;
    }
    *ppInfo = nullptr;

    return run_inside_com_boundary([&]() -> HRESULT {
        ITfDisplayAttributeInfo* info = nullptr;
        if (::IsEqualGUID(guidInfo, GUID_DISPLAY_ATTRIBUTE_INPUT)) {
            info = new (std::nothrow) DisplayAttributeInfoInput();
        } else if (::IsEqualGUID(guidInfo, GUID_DISPLAY_ATTRIBUTE_CONVERTED)) {
            info = new (std::nothrow) DisplayAttributeInfoConverted();
        } else {
            return E_INVALIDARG;
        }
        if (info == nullptr) {
            return E_OUTOFMEMORY;
        }
        *ppInfo = info;
        return S_OK;
    });
}

HRESULT SubornoTextService::_UpdateCandidateUI(ITfContext* context,
                                               ContextState& state,
                                               const layout::CompositionSnapshot& snapshot) {
    if (m_uiElementMgr == nullptr || !m_engine->settings().ui.show_candidate_window
        || snapshot.candidates.size() < 2) {
        return _EndCandidateUI(state);
    }

    if (state.candidate_ui_element == nullptr) {
        // Resolve the full Interface-tab candidate window contract: font,

        // orientation, fixed height, sliding effect and caret/mouse

        // positioning are all real runtime consumers here.

        const auto& ui_settings = m_engine->settings().ui;
        CandidateListUIElement::Options options;
        options.vertical_orientation =
            ui_settings.candidate_orientation == suborno::settings::CandidateOrientation::vertical;
        options.fixed_height = ui_settings.candidate_fixed_height;
        options.sliding_effect = ui_settings.enable_sliding_effect;
        options.follow_caret = ui_settings.follow_caret;
        options.follow_mouse_fallback = ui_settings.follow_mouse_pointer_if_caret_tracking_fails;
        options.font_family =
            ui_settings.use_os_default_bangla_font || ui_settings.font_family.empty()
                ? std::wstring()
                : utf8_to_utf16(ui_settings.font_family);
        auto* created = new (std::nothrow) CandidateListUIElement(
            context,
            snapshot.candidates,
            snapshot.selected_candidate_index,
            options,
            [this, context](UINT index) {
                return _FinalizeCandidate(context, index);
            },
            [this, context]() {
                return _AbortCandidate(context);
            });
        if (created == nullptr) {
            return E_OUTOFMEMORY;
        }

        wil::com_ptr<CandidateListUIElement> element;
        element.attach(created);
        wil::com_ptr<ITfUIElement> ui_element;
        HRESULT result = element->QueryInterface(IID_PPV_ARGS(ui_element.put()));
        if (FAILED(result)) {
            return result;
        }

        BOOL show = TRUE;
        DWORD element_id = 0;
        result = m_uiElementMgr->BeginUIElement(ui_element.get(), &show, &element_id);
        if (FAILED(result)) {
            return result;
        }

        state.candidate_ui_element = std::move(element);
        state.candidate_ui_element_id = element_id;
        state.candidate_ui_owned = show != FALSE;
        if (state.candidate_ui_owned) {
            result = state.candidate_ui_element->Show(TRUE);
            if (FAILED(result)) {
                _EndCandidateUI(state);
                return result;
            }
        } else {
            result = m_uiElementMgr->UpdateUIElement(state.candidate_ui_element_id);
            if (FAILED(result)) {
                _EndCandidateUI(state);
                return result;
            }
        }
        return S_OK;
    }

    state.candidate_ui_element->Update(snapshot.candidates, snapshot.selected_candidate_index);
    return m_uiElementMgr->UpdateUIElement(state.candidate_ui_element_id);
}

HRESULT SubornoTextService::_EndCandidateUI(ContextState& state) {
    HRESULT result = S_OK;
    if (state.candidate_ui_element != nullptr && state.candidate_ui_owned) {
        result = state.candidate_ui_element->Show(FALSE);
    }
    if (state.candidate_ui_element != nullptr && m_uiElementMgr != nullptr) {
        const HRESULT end_result = m_uiElementMgr->EndUIElement(state.candidate_ui_element_id);
        if (FAILED(end_result) && SUCCEEDED(result)) {
            result = end_result;
        }
    }
    state.candidate_ui_element.reset();
    state.candidate_ui_element_id = 0;
    state.candidate_ui_owned = false;
    return result;
}

HRESULT SubornoTextService::_FinalizeCandidate(ITfContext* context, UINT index) {
    const auto found = m_contexts.find(context);
    if (found == m_contexts.end() || found->second.session == nullptr) {
        return E_UNEXPECTED;
    }

    ContextState& state = found->second;
    const layout::CommitResult committed = state.session->commit_candidate(index);
    if (committed.status != layout::InputStatus::updated) {
        return E_INVALIDARG;
    }
    const std::wstring text = _CommittedTextToUtf16(committed.text_utf8);
    if (!committed.text_utf8.empty() && text.empty()) {
        return E_INVALIDARG;
    }
    _RecordCommit(committed.text_utf8);
    return _FinishComposition(context, state, text);
}

HRESULT SubornoTextService::_AbortCandidate(ITfContext* context) {
    const auto found = m_contexts.find(context);
    if (found == m_contexts.end() || found->second.session == nullptr) {
        return E_UNEXPECTED;
    }
    found->second.session->reset();
    return _FinishComposition(context, found->second, {});
}

HRESULT SubornoTextService::_UpdateComposition(ITfContext* context,
                                               ContextState& state,
                                               const std::wstring& text) {
    // BornoTSF parity: the composition text carries the Input display

    // attribute while composing (_SetCompositionDisplayAttributes in

    // display/DisplayAttribute.cpp, applied in the same edit session).

    const TfGuidAtom input_attribute =
        m_gaDisplayAttributeInput != 0 ? m_gaDisplayAttributeInput : no_display_attribute;
    if (state.composition == nullptr) {
        return TextCompositionEditSession::start_composition(
            context, m_clientId, this, text, state.composition, input_attribute);
    }
    return TextCompositionEditSession::update_composition(
        context, m_clientId, state.composition.get(), text, input_attribute);
}

HRESULT SubornoTextService::_FinishComposition(ITfContext* context,
                                               ContextState& state,
                                               const std::wstring& text) {
    const HRESULT ui_result = _EndCandidateUI(state);
    if (FAILED(ui_result)) {
        return ui_result;
    }
    HRESULT result = S_OK;
    if (state.composition != nullptr) {
        result = TextCompositionEditSession::finish_composition(
            context, m_clientId, state.composition.get(), text, /*clear_display_attribute=*/true);
        if (SUCCEEDED(result)) {
            state.composition.reset();
        }
    } else if (!text.empty()) {
        result = _CommitString(context, text);
    }
    if (SUCCEEDED(result) && state.session != nullptr) {
        state.session->reset();
    }
    return result;
}

HRESULT SubornoTextService::_CommitString(ITfContext* pContext, const std::wstring& text) {
    if (pContext == nullptr) {
        return E_INVALIDARG;
    }
    if (text.empty()) {
        return S_OK;
    }
    return TextInsertionEditSession::insert_text(pContext, m_clientId, text);
}

HRESULT SubornoTextService::CreateInstance(REFIID riid, void** ppv) {
    if (ppv == nullptr) {
        return E_POINTER;
    }
    *ppv = nullptr;

    SubornoTextService* service = new (std::nothrow) SubornoTextService();
    if (service == nullptr) {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = service->QueryInterface(riid, ppv);
    service->Release();
    return hr;
}

// ---- BornoTSF parity: the four V16-audit-gap behaviours ----

HRESULT SubornoTextService::_InitDisplayAttributeGuidAtom() {
    // BornoTSF _InitDisplayAttributeGuidAtom (display/DisplayAttribute.cpp:54-79):

    // CoCreateInstance(CLSID_TF_CategoryMgr) then RegisterGUID for both

    // attributes; any failure aborts activation.

    wil::com_ptr<ITfCategoryMgr> category_mgr;
    HRESULT hr = ::CoCreateInstance(
        CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(category_mgr.put()));
    if (FAILED(hr)) {
        return hr;
    }

    hr = category_mgr->RegisterGUID(GUID_DISPLAY_ATTRIBUTE_INPUT, &m_gaDisplayAttributeInput);
    if (SUCCEEDED(hr)) {
        hr = category_mgr->RegisterGUID(GUID_DISPLAY_ATTRIBUTE_CONVERTED,
                                        &m_gaDisplayAttributeConverted);
    }
    if (FAILED(hr)) {
        m_gaDisplayAttributeInput = 0;
        m_gaDisplayAttributeConverted = 0;
    }
    return SUCCEEDED(hr) ? S_OK : hr;
}

HRESULT SubornoTextService::_ClearCompositionDisplayAttributes(TfEditCookie edit_cookie,
                                                               ITfContext* context,
                                                               ITfComposition* composition) {
    // BornoTSF _ClearCompositionDisplayAttributes (display/DisplayAttribute.cpp:7-24).

    if (context == nullptr || composition == nullptr) {
        return E_INVALIDARG;
    }
    wil::com_ptr<ITfRange> range;
    HRESULT hr = composition->GetRange(range.put());
    if (FAILED(hr)) {
        return hr;
    }
    wil::com_ptr<ITfProperty> display_attribute_property;
    if (SUCCEEDED(context->GetProperty(GUID_PROP_ATTRIBUTE, display_attribute_property.put()))
        && display_attribute_property != nullptr) {
        static_cast<void>(display_attribute_property->Clear(edit_cookie, range.get()));
    }
    return S_OK;
}

BOOL SubornoTextService::_InitTextEditSink(ITfContext* context, ContextState& state) {
    // BornoTSF _InitTextEditSink (infra/TextEditSink.cpp:49-103), adapted to

    // the per-context state model: unadvise the previous advice first, then

    // advise ITfTextEditSink on the given context.

    if (state.text_edit_sink_cookie != TF_INVALID_COOKIE
        && state.text_edit_sink_context != nullptr) {
        wil::com_ptr<ITfSource> source;
        if (SUCCEEDED(state.text_edit_sink_context->QueryInterface(IID_PPV_ARGS(source.put())))
            && source != nullptr) {
            static_cast<void>(source->UnadviseSink(state.text_edit_sink_cookie));
        }
        state.text_edit_sink_context.reset();
        state.text_edit_sink_cookie = TF_INVALID_COOKIE;
    }

    if (context == nullptr) {
        return TRUE;
    }

    // AddRef the context for the lifetime of the advice (upstream holds

    // _pTextEditSinkContext with its own reference too).

    state.text_edit_sink_context = context;
    if (state.text_edit_sink_context == nullptr) {
        return TRUE;
    }

    wil::com_ptr<ITfSource> source;
    if (FAILED(state.text_edit_sink_context->QueryInterface(IID_PPV_ARGS(source.put())))
        || source == nullptr) {
        state.text_edit_sink_context.reset();
        return FALSE;
    }
    if (FAILED(source->AdviseSink(IID_ITfTextEditSink,
                                  static_cast<ITfTextEditSink*>(this),
                                  &state.text_edit_sink_cookie))) {
        state.text_edit_sink_cookie = TF_INVALID_COOKIE;
        state.text_edit_sink_context.reset();
        return FALSE;
    }
    return TRUE;
}

void SubornoTextService::_ShutdownTextEditSink(ContextState& state) {
    if (state.text_edit_sink_cookie == TF_INVALID_COOKIE) {
        state.text_edit_sink_context.reset();
        return;
    }
    wil::com_ptr<ITfSource> source;
    if (state.text_edit_sink_context != nullptr
        && SUCCEEDED(state.text_edit_sink_context->QueryInterface(IID_PPV_ARGS(source.put())))
        && source != nullptr) {
        static_cast<void>(source->UnadviseSink(state.text_edit_sink_cookie));
    }
    state.text_edit_sink_context.reset();
    state.text_edit_sink_cookie = TF_INVALID_COOKIE;
}

BOOL SubornoTextService::_IsRangeCovered(TfEditCookie edit_cookie,
                                         ITfRange* range_test,
                                         ITfRange* range_cover) const {
    // BornoTSF _IsRangeCovered (composition/KeyHandler.cpp:8-23).

    if (range_test == nullptr || range_cover == nullptr) {
        return FALSE;
    }
    LONG result = 0;

    if (FAILED(range_cover->CompareStart(edit_cookie, range_test, TF_ANCHOR_START, &result))
        || result > 0) {
        return FALSE;
    }
    if (FAILED(range_cover->CompareEnd(edit_cookie, range_test, TF_ANCHOR_END, &result))
        || result < 0) {
        return FALSE;
    }
    return TRUE;
}

HRESULT SubornoTextService::_InitThreadFocusSink() {
    // BornoTSF _InitThreadFocusSink (infra/ThreadFocusSink.cpp:16-33).

    if (m_threadMgr == nullptr) {
        return E_UNEXPECTED;
    }
    wil::com_ptr<ITfSource> source;
    const HRESULT hr = m_threadMgr->QueryInterface(IID_PPV_ARGS(source.put()));
    if (FAILED(hr)) {
        return hr;
    }
    const HRESULT advise = source->AdviseSink(
        IID_ITfThreadFocusSink, static_cast<ITfThreadFocusSink*>(this), &m_threadFocusSinkCookie);
    if (FAILED(advise)) {
        m_threadFocusSinkCookie = TF_INVALID_COOKIE;
        return advise;
    }
    return S_OK;
}

void SubornoTextService::_UninitThreadFocusSink() {
    // BornoTSF _UninitThreadFocusSink (infra/ThreadFocusSink.cpp:35-51).

    if (m_threadMgr == nullptr || m_threadFocusSinkCookie == TF_INVALID_COOKIE) {
        m_threadFocusSinkCookie = TF_INVALID_COOKIE;
        return;
    }
    wil::com_ptr<ITfSource> source;
    if (SUCCEEDED(m_threadMgr->QueryInterface(IID_PPV_ARGS(source.put()))) && source != nullptr) {
        static_cast<void>(source->UnadviseSink(m_threadFocusSinkCookie));
    }
    m_threadFocusSinkCookie = TF_INVALID_COOKIE;
}

HRESULT SubornoTextService::CompartmentChangeCallback(void* context, REFGUID guid_compartment) {
    (void)guid_compartment;
    // BornoTSF's CompartmentCallback body is empty; Suborno uses the same

    // notification to re-sync the input indicator, so an externally driven

    // compartment change (user toggling another keyboard) keeps the Windows

    // input indicator truthful. The refresh is change-gated: it writes the

    // compartment only when the active layout actually differs.

    auto* service = static_cast<SubornoTextService*>(context);
    if (service != nullptr) {
        service->_UpdateInputIndicators();
    }
    return S_OK;
}

}  // namespace suborno::platform::windows
