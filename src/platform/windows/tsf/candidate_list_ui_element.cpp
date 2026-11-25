#include "suborno/platform/windows/tsf/candidate_list_ui_element.hpp"

#include <algorithm>
#include <cwchar>
#include <iterator>
#include <limits>
#include <mutex>
#include <new>
#include <oleauto.h>
#include <string>
#include <string_view>
#include <windowsx.h>

#include "../com/boundary_guard.hpp"
#include "suborno/platform/windows/app/ui_theme.hpp"

namespace suborno::platform::windows {

// The candidate window draws with the shared theme tokens and DPI helpers,

// which live in the app shell's ui namespace — a sibling of this one (the

// ui namespace is suborno::platform::windows::app::ui), so alias it for the

// qualified ui::… calls below instead of re-qualifying every token.

namespace ui = suborno::platform::windows::app::ui;

namespace {

constexpr wchar_t candidate_window_class[] = L"SubornoCandidateListUIElement";
constexpr int candidate_row_height = 26;
constexpr int candidate_column_width = 108;
constexpr int candidate_window_width = 340;
constexpr std::size_t auto_candidates_per_page = 8;
// "Fixed height" keeps a constant five-row window and pages through

// candidates; with it off the window hugs the candidate count instead.

constexpr std::size_t fixed_candidates_per_page = 5;
constexpr int horizontal_candidates_per_page = 5;
// Slide-in animation: total travel and per-frame step (96-DPI design px).

constexpr int slide_travel = 12;
constexpr int slide_step = 4;
constexpr UINT_PTR slide_timer_id = 1;
constexpr UINT slide_timer_ms = 16;

// GetDpiForWindow is Windows 10 1607+; resolve it dynamically so older

// systems still fall back to the legacy GDI metric.

UINT dpi_for_window(HWND window) noexcept {
    using GetDpiForWindowFn = UINT(WINAPI*)(HWND);
    static const GetDpiForWindowFn get_dpi_for_window = []() -> GetDpiForWindowFn {
        HMODULE user32 = ::GetModuleHandleW(L"user32.dll");
        return user32 == nullptr ? nullptr
                                 : reinterpret_cast<GetDpiForWindowFn>(
                                       ::GetProcAddress(user32, "GetDpiForWindow"));
    }();
    if (get_dpi_for_window != nullptr && window != nullptr) {
        const UINT dpi = get_dpi_for_window(window);
        if (dpi != 0) {
            return dpi;
        }
    }
    if (HDC device_context = window == nullptr ? nullptr : ::GetDC(window);
        device_context != nullptr) {
        const UINT dpi = static_cast<UINT>(::GetDeviceCaps(device_context, LOGPIXELSY));
        ::ReleaseDC(window, device_context);
        if (dpi != 0) {
            return dpi;
        }
    }
    return 96;
}

HINSTANCE candidate_window_module_instance() noexcept {
    HMODULE module = nullptr;
    constexpr DWORD module_flags =
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;
    if (::GetModuleHandleExW(
            module_flags, reinterpret_cast<LPCWSTR>(&CandidateListUIElement::window_proc), &module)
        == FALSE) {
        return nullptr;
    }
    return module;
}

ATOM register_candidate_window_class() {
    static std::once_flag registration;
    static ATOM registered_class = 0;
    std::call_once(registration, [] {
        WNDCLASSEXW window_class{};
        window_class.cbSize = sizeof(window_class);
        window_class.lpfnWndProc = &CandidateListUIElement::window_proc;
        window_class.hInstance = candidate_window_module_instance();
        if (window_class.hInstance == nullptr) {
            return;
        }
        window_class.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
        window_class.hbrBackground = ::GetSysColorBrush(COLOR_INFOBK);
        window_class.lpszClassName = candidate_window_class;
        registered_class = ::RegisterClassExW(&window_class);
        if (registered_class == 0 && ::GetLastError() == ERROR_CLASS_ALREADY_EXISTS) {
            registered_class = 1;
        }
    });
    return registered_class;
}

}  // namespace

CandidateListUIElement::CandidateListUIElement(ITfContext* context,
                                               const std::vector<layout::Candidate>& candidates,
                                               std::size_t selected_index,
                                               const Options& options,
                                               FinalizeCallback finalize_callback,
                                               AbortCallback abort_callback)
    : context_(context),
      selected_index_(static_cast<UINT>(selected_index)),
      options_(options),
      finalize_callback_(std::move(finalize_callback)),
      abort_callback_(std::move(abort_callback)) {
    candidates_.reserve(candidates.size());
    for (const layout::Candidate& candidate : candidates) {
        candidates_.push_back(to_wide(candidate.text));
    }
    if (candidates_.empty()) {
        selected_index_ = 0;
    } else if (selected_index_ >= candidates_.size()) {
        selected_index_ = static_cast<UINT>(candidates_.size() - 1);
    }
    reset_page_index();
    update_current_page_for_selection();
}

CandidateListUIElement::~CandidateListUIElement() {
    stop_slide_timer();
    destroy_native_window();
    if (candidate_font_ != nullptr) {
        ::DeleteObject(candidate_font_);
        candidate_font_ = nullptr;
    }
}

std::wstring CandidateListUIElement::to_wide(std::string_view utf8) {
    if (utf8.empty() || utf8.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return {};
    }

    const int input_length = static_cast<int>(utf8.size());
    const int required =
        ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), input_length, nullptr, 0);
    if (required <= 0) {
        return {};
    }

    std::wstring result(static_cast<std::size_t>(required), L'\0');
    if (::MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), input_length, result.data(), required)
        != required) {
        return {};
    }
    return result;
}

STDMETHODIMP CandidateListUIElement::QueryInterface(REFIID riid, void** object) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (object == nullptr) {
            return E_POINTER;
        }
        *object = nullptr;

        if (riid == IID_IUnknown || riid == IID_ITfUIElement
            || riid == IID_ITfCandidateListUIElement) {
            *object = static_cast<ITfCandidateListUIElement*>(this);
        } else if (riid == IID_ITfCandidateListUIElementBehavior) {
            *object = static_cast<ITfCandidateListUIElementBehavior*>(this);
        } else {
            return E_NOINTERFACE;
        }
        AddRef();
        return S_OK;
    });
}

STDMETHODIMP_(ULONG) CandidateListUIElement::AddRef() {
    return static_cast<ULONG>(::InterlockedIncrement(&reference_count_));
}

STDMETHODIMP_(ULONG) CandidateListUIElement::Release() {
    const ULONG count = static_cast<ULONG>(::InterlockedDecrement(&reference_count_));
    if (count == 0) {
        delete this;
    }
    return count;
}

STDMETHODIMP CandidateListUIElement::GetDescription(BSTR* description) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (description == nullptr) {
            return E_INVALIDARG;
        }
        *description = ::SysAllocString(L"Suborno candidate list");
        return *description == nullptr ? E_OUTOFMEMORY : S_OK;
    });
}

STDMETHODIMP CandidateListUIElement::GetGUID(GUID* guid) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (guid == nullptr) {
            return E_INVALIDARG;
        }
        *guid = suborno_candidate_list_uielement_guid;
        return S_OK;
    });
}

STDMETHODIMP CandidateListUIElement::Show(BOOL show) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (show != FALSE) {
            const HRESULT result = show_native_window();
            if (SUCCEEDED(result)) {
                shown_ = true;
            }
            return result;
        }

        shown_ = false;
        destroy_native_window();
        return S_OK;
    });
}

STDMETHODIMP CandidateListUIElement::IsShown(BOOL* show) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (show == nullptr) {
            return E_INVALIDARG;
        }
        *show = shown_ ? TRUE : FALSE;
        return S_OK;
    });
}

STDMETHODIMP CandidateListUIElement::GetUpdatedFlags(DWORD* flags) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (flags == nullptr) {
            return E_INVALIDARG;
        }
        *flags = updated_flags_;
        updated_flags_ = 0;
        return S_OK;
    });
}

STDMETHODIMP CandidateListUIElement::GetDocumentMgr(ITfDocumentMgr** document_manager) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (document_manager == nullptr) {
            return E_INVALIDARG;
        }
        *document_manager = nullptr;
        if (context_ == nullptr) {
            return E_FAIL;
        }
        return context_->GetDocumentMgr(document_manager);
    });
}

STDMETHODIMP CandidateListUIElement::GetCount(UINT* count) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (count == nullptr) {
            return E_INVALIDARG;
        }
        *count = static_cast<UINT>(candidates_.size());
        return S_OK;
    });
}

STDMETHODIMP CandidateListUIElement::GetSelection(UINT* index) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (index == nullptr) {
            return E_INVALIDARG;
        }
        *index = selected_index_;
        return S_OK;
    });
}

STDMETHODIMP CandidateListUIElement::GetString(UINT index, BSTR* text) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (text == nullptr || index >= candidates_.size()) {
            return E_INVALIDARG;
        }
        const std::wstring& candidate = candidates_[index];
        *text = ::SysAllocStringLen(candidate.data(), static_cast<UINT>(candidate.size()));
        return *text == nullptr ? E_OUTOFMEMORY : S_OK;
    });
}

STDMETHODIMP CandidateListUIElement::GetPageIndex(UINT* index, UINT size, UINT* page_count) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (page_count == nullptr) {
            return E_INVALIDARG;
        }
        *page_count = static_cast<UINT>(page_starts_.size());
        if (index == nullptr) {
            return S_OK;
        }
        if (size < page_starts_.size()) {
            return E_INVALIDARG;
        }
        std::copy(page_starts_.begin(), page_starts_.end(), index);
        return S_OK;
    });
}

STDMETHODIMP CandidateListUIElement::SetPageIndex(UINT* index, UINT page_count) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (index == nullptr || page_count == 0 || candidates_.empty() || index[0] != 0) {
            return E_INVALIDARG;
        }

        std::vector<UINT> updated;
        updated.reserve(page_count);
        UINT previous = 0;
        for (UINT page = 0; page < page_count; ++page) {
            if (index[page] >= candidates_.size() || (page != 0 && index[page] <= previous)) {
                return E_INVALIDARG;
            }
            updated.push_back(index[page]);
            previous = index[page];
        }

        page_starts_ = std::move(updated);
        update_current_page_for_selection();
        updated_flags_ |= TF_CLUIE_PAGEINDEX | TF_CLUIE_CURRENTPAGE;
        update_native_window();
        return S_OK;
    });
}

STDMETHODIMP CandidateListUIElement::GetCurrentPage(UINT* page) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (page == nullptr) {
            return E_INVALIDARG;
        }
        *page = current_page_;
        return S_OK;
    });
}

STDMETHODIMP CandidateListUIElement::SetSelection(UINT index) {
    return run_inside_com_boundary([&]() -> HRESULT {
        if (index >= candidates_.size()) {
            return E_INVALIDARG;
        }
        if (selected_index_ != index) {
            selected_index_ = index;
            updated_flags_ |= TF_CLUIE_SELECTION;
            const UINT previous_page = current_page_;
            update_current_page_for_selection();
            if (previous_page != current_page_) {
                updated_flags_ |= TF_CLUIE_CURRENTPAGE;
            }
            update_native_window();
        }
        return S_OK;
    });
}

STDMETHODIMP CandidateListUIElement::Finalize() {
    return run_inside_com_boundary([&]() -> HRESULT {
        AddRef();
        // The callbacks cross back into the owning text service and can throw

        // (bad_alloc etc.); a C++ exception must never escape a COM boundary.

        HRESULT result = E_NOTIMPL;
        try {
            result = finalize_callback_ ? finalize_callback_(selected_index_) : E_NOTIMPL;
        } catch (const std::bad_alloc&) {
            result = E_OUTOFMEMORY;
        } catch (...) {
            result = E_FAIL;
        }
        if (SUCCEEDED(result)) {
            Show(FALSE);
        }
        Release();
        return result;
    });
}

STDMETHODIMP CandidateListUIElement::Abort() {
    return run_inside_com_boundary([&]() -> HRESULT {
        AddRef();
        HRESULT result = E_NOTIMPL;
        try {
            result = abort_callback_ ? abort_callback_() : E_NOTIMPL;
        } catch (const std::bad_alloc&) {
            result = E_OUTOFMEMORY;
        } catch (...) {
            result = E_FAIL;
        }
        if (SUCCEEDED(result)) {
            Show(FALSE);
        }
        Release();
        return result;
    });
}

void CandidateListUIElement::Update(const std::vector<layout::Candidate>& candidates,
                                    std::size_t selected_index) {
    candidates_.clear();
    candidates_.reserve(candidates.size());
    for (const layout::Candidate& candidate : candidates) {
        candidates_.push_back(to_wide(candidate.text));
    }

    const UINT new_selection =
        candidates_.empty() ? 0U
                            : static_cast<UINT>(std::min(selected_index, candidates_.size() - 1));
    updated_flags_ |= TF_CLUIE_COUNT | TF_CLUIE_STRING | TF_CLUIE_PAGEINDEX;
    const UINT previous_page = current_page_;
    reset_page_index();
    if (selected_index_ != new_selection) {
        selected_index_ = new_selection;
        updated_flags_ |= TF_CLUIE_SELECTION;
    }
    update_current_page_for_selection();
    if (previous_page != current_page_) {
        updated_flags_ |= TF_CLUIE_CURRENTPAGE;
    }
    update_native_window();
}

std::size_t CandidateListUIElement::hit_candidate(POINT client_point) const noexcept {
    const int row_height = scaled(candidate_row_height);
    if (!options_.vertical_orientation) {
        const int column_width = scaled(candidate_column_width);
        const int column = column_width == 0 ? -1 : (client_point.x - scaled(4)) / column_width;
        const std::size_t page_size = page_capacity();
        if (column < 0 || static_cast<std::size_t>(column) >= page_size) {
            return std::numeric_limits<std::size_t>::max();
        }
        const std::size_t index = current_page_start() + static_cast<std::size_t>(column);
        return index < current_page_end() ? index : std::numeric_limits<std::size_t>::max();
    }
    const int top_padding = scaled(4);
    const int row = row_height == 0 ? -1 : (client_point.y - top_padding) / row_height;
    if (row < 0) {
        return std::numeric_limits<std::size_t>::max();
    }
    const std::size_t index = current_page_start() + static_cast<std::size_t>(row);
    return index < current_page_end() ? index : std::numeric_limits<std::size_t>::max();
}

HRESULT CandidateListUIElement::show_native_window() {
    if (window_ != nullptr) {
        update_native_window();
        ::ShowWindow(window_, SW_SHOWNOACTIVATE);
        return S_OK;
    }
    if (register_candidate_window_class() == 0) {
        return HRESULT_FROM_WIN32(::GetLastError());
    }

    const int dpi_estimate = static_cast<int>(dpi_for_window(nullptr));
    const int row_height = candidate_row_height * dpi_estimate / 96;
    const int column_width = candidate_column_width * dpi_estimate / 96;
    const int width_estimate =
        options_.vertical_orientation
            ? candidate_window_width * dpi_estimate / 96
            : horizontal_candidates_per_page * column_width + 8 * dpi_estimate / 96;
    const int height_estimate =
        options_.vertical_orientation
            ? static_cast<int>(page_capacity()) * row_height + 8 * dpi_estimate / 96
            : row_height + 8 * dpi_estimate / 96;

    // Anchor resolution honors the Interface-tab positioning options:

    // follow_caret=false pins the window to the top-right work-area corner;

    // when caret tracking fails, follow_mouse_fallback chooses between the

    // mouse pointer and that same pinned spot.

    POINT anchor{};
    bool anchored = false;
    HWND owner = ::GetForegroundWindow();
    const DWORD thread_id =
        owner == nullptr ? ::GetCurrentThreadId() : ::GetWindowThreadProcessId(owner, nullptr);
    GUITHREADINFO gui_info{};
    gui_info.cbSize = sizeof(gui_info);
    if (options_.follow_caret && ::GetGUIThreadInfo(thread_id, &gui_info) != FALSE
        && gui_info.hwndCaret != nullptr) {
        anchor.x = gui_info.rcCaret.left;
        anchor.y = gui_info.rcCaret.bottom;
        anchored = ::ClientToScreen(gui_info.hwndCaret, &anchor) != FALSE;
        if (!anchored) {
            anchor = {};
        }
    }
    if (!anchored && options_.follow_mouse_fallback) {
        anchored = ::GetCursorPos(&anchor) != FALSE;
    }
    if (!anchored) {
        // No caret and no mouse fallback: pin to the top-right corner of the

        // nearest monitor so the list is never drawn off-screen mid-type.

        const HMONITOR pinned_monitor = ::MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTONEAREST);
        MONITORINFO pinned_info{};
        pinned_info.cbSize = sizeof(pinned_info);
        if (::GetMonitorInfoW(pinned_monitor, &pinned_info) != FALSE) {
            anchor.x = pinned_info.rcWork.right - width_estimate - 8 * dpi_estimate / 96;
            anchor.y = pinned_info.rcWork.top + 8 * dpi_estimate / 96;
        }
    }

    const HMONITOR monitor = ::MonitorFromPoint(anchor, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitor_info{};
    monitor_info.cbSize = sizeof(monitor_info);
    if (::GetMonitorInfoW(monitor, &monitor_info) != FALSE) {
        if (anchor.y + height_estimate > monitor_info.rcWork.bottom) {
            anchor.y = std::max(monitor_info.rcWork.top,
                                anchor.y - height_estimate - 24 * dpi_estimate / 96);
        }
        // Near the right screen edge, keep the window fully visible instead

        // of letting it clip off the display.

        anchor.x = std::max(monitor_info.rcWork.left,
                            std::min(anchor.x, monitor_info.rcWork.right - width_estimate));
    }

    // No WS_BORDER: the system-drawn frame ignored the theme palettes. The

    // paint loop draws a 1 px frame from theme_separator() instead.

    window_ = ::CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                                candidate_window_class,
                                L"Suborno candidates",
                                WS_POPUP,
                                anchor.x,
                                anchor.y,
                                width_estimate,
                                height_estimate,
                                owner,
                                nullptr,
                                candidate_window_module_instance(),
                                this);
    if (window_ == nullptr) {
        return HRESULT_FROM_WIN32(::GetLastError());
    }

    // Read the actual window DPI and scale the metrics + font for it; the

    // sizes computed above were 96-DPI estimates used for the initial size.

    refresh_dpi();
    if (candidate_font_ == nullptr) {
        // refresh_dpi only rebuilds the font when the DPI changed from the

        // 96 default; make sure a 96-DPI window still gets the Bengali font.

        ensure_candidate_font();
    }
    ::SetWindowPos(window_,
                   HWND_TOPMOST,
                   anchor.x,
                   anchor.y,
                   window_width_for_page(),
                   window_height_for_page(),
                   SWP_NOACTIVATE | SWP_SHOWWINDOW);
    ::ShowWindow(window_, SW_SHOWNOACTIVATE);
    start_slide_in();
    update_native_window();
    return S_OK;
}

int CandidateListUIElement::scaled(int pixels) const noexcept {
    return pixels * static_cast<int>(dpi_) / 96;
}

void CandidateListUIElement::refresh_dpi() {
    if (window_ != nullptr) {
        const UINT dpi = dpi_for_window(window_);
        if (dpi != dpi_) {
            dpi_ = dpi;
            ensure_candidate_font();
        }
    }
}

void CandidateListUIElement::ensure_candidate_font() {
    if (candidate_font_ != nullptr) {
        ::DeleteObject(candidate_font_);
        candidate_font_ = nullptr;
    }
    LOGFONTW font{};
    font.lfHeight = -scaled(17);
    font.lfWeight = FW_NORMAL;
    font.lfCharSet = DEFAULT_CHARSET;
    font.lfOutPrecision = OUT_TT_PRECIS;
    font.lfQuality = CLEARTYPE_QUALITY;
    // The configured app font wins; when none is set (or the configured name

    // is empty) fall back to the standard Windows Bangla UI font.

    wcscpy_s(font.lfFaceName,
             LF_FACESIZE,
             options_.font_family.empty() ? L"Nirmala UI" : options_.font_family.c_str());
    candidate_font_ = ::CreateFontIndirectW(&font);
}

void CandidateListUIElement::update_native_window() noexcept {
    if (window_ != nullptr) {
        // Orientation/fixed-height changes (and auto-height page growth) must

        // resize the live window, not just repaint it.

        ::SetWindowPos(window_,
                       nullptr,
                       0,
                       0,
                       window_width_for_page(),
                       window_height_for_page(),
                       SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
        ::InvalidateRect(window_, nullptr, TRUE);
        ::UpdateWindow(window_);
    }
}

void CandidateListUIElement::destroy_native_window() noexcept {
    if (window_ != nullptr && ::IsWindow(window_) != FALSE) {
        ::DestroyWindow(window_);
    }
}

void CandidateListUIElement::reset_page_index() {
    // Page capacity follows the Interface-tab options: fixed height pages

    // through a constant five-row/slot window; auto height shows more rows

    // (or slots, horizontally) when they exist.

    const std::size_t per_page = page_capacity();
    page_starts_.clear();
    for (std::size_t start = 0; start < candidates_.size(); start += per_page) {
        page_starts_.push_back(static_cast<UINT>(start));
    }
    if (page_starts_.empty()) {
        current_page_ = 0;
    } else if (current_page_ >= page_starts_.size()) {
        current_page_ = static_cast<UINT>(page_starts_.size() - 1);
    }
}

std::size_t CandidateListUIElement::page_capacity() const noexcept {
    if (!options_.vertical_orientation) {
        return static_cast<std::size_t>(horizontal_candidates_per_page);
    }
    return options_.fixed_height ? fixed_candidates_per_page : auto_candidates_per_page;
}

int CandidateListUIElement::window_width_for_page() const noexcept {
    if (options_.vertical_orientation) {
        return scaled(candidate_window_width);
    }
    // Horizontal: the row hugs the visible slots so it tracks the caret

    // without dwarfing short candidate sets.

    const std::size_t visible = current_page_end() - current_page_start();
    const int columns = static_cast<int>(std::max<std::size_t>(visible, 1));
    return scaled(columns * candidate_column_width + 8);
}

int CandidateListUIElement::window_height_for_page() const noexcept {
    if (!options_.vertical_orientation) {
        return scaled(candidate_row_height + 8);
    }
    if (options_.fixed_height) {
        // Fixed height: always five rows, even when fewer are on the page.

        return scaled(static_cast<int>(fixed_candidates_per_page) * candidate_row_height + 8);
    }
    const std::size_t visible = current_page_end() - current_page_start();
    return scaled(static_cast<int>(std::max<std::size_t>(visible, 1)) * candidate_row_height + 8);
}

void CandidateListUIElement::start_slide_in() {
    if (!options_.sliding_effect || window_ == nullptr) {
        slide_offset_ = 0;
        return;
    }
    slide_offset_ = slide_travel;
    apply_slide_offset();
    slide_timer_id_ = ::SetTimer(window_, slide_timer_id, slide_timer_ms, nullptr);
    if (slide_timer_id_ == 0) {
        // A timer-less fallback must not leave the window displaced.

        slide_offset_ = 0;
        apply_slide_offset();
    }
}

void CandidateListUIElement::advance_slide() {
    slide_offset_ -= slide_step;
    if (slide_offset_ <= 0) {
        slide_offset_ = 0;
        stop_slide_timer();
    }
    apply_slide_offset();
}

void CandidateListUIElement::stop_slide_timer() noexcept {
    if (window_ != nullptr && slide_timer_id_ != 0) {
        ::KillTimer(window_, slide_timer_id_);
    }
    slide_timer_id_ = 0;
}

void CandidateListUIElement::apply_slide_offset() noexcept {
    if (window_ == nullptr) {
        return;
    }
    RECT rect{};
    if (::GetWindowRect(window_, &rect) == FALSE) {
        return;
    }
    ::SetWindowPos(window_,
                   HWND_TOPMOST,
                   rect.left,
                   rect.top - ui::scaled(dpi_, slide_offset_),
                   0,
                   0,
                   SWP_NOACTIVATE | SWP_NOSIZE);
}

void CandidateListUIElement::update_current_page_for_selection() noexcept {
    if (page_starts_.empty()) {
        current_page_ = 0;
        return;
    }
    const auto found = std::upper_bound(page_starts_.begin(), page_starts_.end(), selected_index_);
    current_page_ = static_cast<UINT>(
        std::distance(page_starts_.begin(), found == page_starts_.begin() ? found : found - 1));
}

std::size_t CandidateListUIElement::current_page_start() const noexcept {
    return page_starts_.empty() ? 0 : page_starts_[current_page_];
}

std::size_t CandidateListUIElement::current_page_end() const noexcept {
    if (page_starts_.empty()) {
        return 0;
    }
    return current_page_ + 1 < page_starts_.size() ? page_starts_[current_page_ + 1]
                                                   : candidates_.size();
}

LRESULT CALLBACK CandidateListUIElement::window_proc(HWND window,
                                                     UINT message,
                                                     WPARAM w_param,
                                                     LPARAM l_param) {
    auto* element =
        reinterpret_cast<CandidateListUIElement*>(::GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(l_param);
        element = static_cast<CandidateListUIElement*>(create->lpCreateParams);
        if (element == nullptr) {
            return FALSE;
        }
        ::SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(element));
        element->window_ = window;
        element->AddRef();
    }
    if (element == nullptr) {
        return ::DefWindowProcW(window, message, w_param, l_param);
    }

    switch (message) {
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_DPICHANGED: {
        // Re-scale metrics + font for the new monitor DPI and apply the

        // suggested rectangle from the message.

        element->refresh_dpi();
        if (l_param != 0) {
            const auto* suggested = reinterpret_cast<const RECT*>(l_param);
            ::SetWindowPos(window,
                           nullptr,
                           suggested->left,
                           suggested->top,
                           suggested->right - suggested->left,
                           suggested->bottom - suggested->top,
                           SWP_NOACTIVATE | SWP_NOZORDER);
        }
        ::InvalidateRect(window, nullptr, TRUE);
        return 0;
    }
    case WM_LBUTTONUP: {
        try {
            const POINT point{GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
            const std::size_t index = element->hit_candidate(point);
            if (index < element->candidates_.size()) {
                element->SetSelection(static_cast<UINT>(index));
                element->Finalize();
            }
        } catch (...) {
            // Never let an exception escape the window procedure.

            return 0;
        }
        return 0;
    }
    case WM_TIMER:
        if (w_param == slide_timer_id) {
            element->advance_slide();
            return 0;
        }
        // A timer ID this window did not create: delegate to the default

        // window procedure, the same handling as the switch's default below.

        // The only WM_TIMER a window procedure can observe carries a NULL

        // callback: a timer registered with a TimerProc is intercepted by

        // DispatchMessage, which "calls [the function in lParam] instead of

        // the window procedure" (DispatchMessage / WM_TIMER / SetTimer

        // documentation) — DefWindowProcW never dispatches timer callbacks,

        // and for a callback-less foreign timer it simply returns 0.

        // Falling out of the switch here left this as the one control path

        // without a return value (C4715, undefined return to the caller).

        return ::DefWindowProcW(window, message, w_param, l_param);
    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC device_context = ::BeginPaint(window, &paint);
        if (device_context == nullptr) {
            return 0;
        }
        try {
            RECT client{};
            ::GetClientRect(window, &client);
            // Theme-aware surface: the candidate window follows the same

            // dark/light palette as the rest of the shell (it used to use

            // the system INFO colors, which ignored the theme setting).

            HBRUSH surface = ::CreateSolidBrush(ui::theme_surface());
            if (surface != nullptr) {
                ::FillRect(device_context, &client, surface);
                ::DeleteObject(surface);
            }
            ::SetBkMode(device_context, TRANSPARENT);
            // Theme-aware 1 px frame replacing the old system WS_BORDER.

            HBRUSH frame = ::CreateSolidBrush(ui::theme_separator());
            if (frame != nullptr) {
                ::FrameRect(device_context, &client, frame);
                ::DeleteObject(frame);
            }

            HGDIOBJ previous_font = nullptr;
            if (element->candidate_font_ != nullptr) {
                previous_font = ::SelectObject(device_context, element->candidate_font_);
            }

            const int padding = element->scaled(4);
            const int row_height = element->scaled(candidate_row_height);
            const int column_width = element->scaled(candidate_column_width);
            const std::size_t page_start = element->current_page_start();
            const std::size_t page_end = element->current_page_end();
            for (std::size_t index = page_start; index < page_end; ++index) {
                RECT slot;
                if (element->is_vertical()) {
                    const std::size_t row_index = index - page_start;
                    slot =
                        RECT{padding,
                             static_cast<LONG>(padding + static_cast<int>(row_index) * row_height),
                             client.right - padding,
                             static_cast<LONG>(padding
                                               + (static_cast<int>(row_index) + 1) * row_height)};
                } else {
                    const std::size_t column_index = index - page_start;
                    slot = RECT{
                        static_cast<LONG>(padding + static_cast<int>(column_index) * column_width),
                        padding,
                        static_cast<LONG>(padding
                                          + (static_cast<int>(column_index) + 1) * column_width),
                        client.bottom - padding};
                }
                if (index == element->selected_index_) {
                    HBRUSH highlight = ::CreateSolidBrush(ui::theme_accent_fill());
                    if (highlight != nullptr) {
                        ::FillRect(device_context, &slot, highlight);
                        ::DeleteObject(highlight);
                    }
                    ::SetTextColor(device_context, ui::theme_on_accent());
                } else {
                    ::SetTextColor(device_context, ui::theme_text());
                }

                const std::wstring label =
                    std::to_wstring(index + 1) + L". " + element->candidates_[index];
                ::DrawTextW(device_context,
                            label.c_str(),
                            static_cast<int>(label.size()),
                            &slot,
                            DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
            }
            if (previous_font != nullptr) {
                ::SelectObject(device_context, previous_font);
            }
        } catch (...) {
            // Never let an exception escape the window procedure; EndPaint

            // below still runs so the paint cycle stays balanced.
        }
        ::EndPaint(window, &paint);
        return 0;
    }
    case WM_NCDESTROY: {
        element->window_ = nullptr;
        ::SetWindowLongPtrW(window, GWLP_USERDATA, 0);
        const LRESULT result = ::DefWindowProcW(window, message, w_param, l_param);
        element->Release();
        return result;
    }
    default:
        return ::DefWindowProcW(window, message, w_param, l_param);
    }
}

}  // namespace suborno::platform::windows
