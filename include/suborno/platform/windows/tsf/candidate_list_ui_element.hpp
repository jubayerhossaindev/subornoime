#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <cstddef>
#include <functional>
#include <msctf.h>
#include <string>
#include <string_view>
#include <vector>
#include <wil/com.h>

#include "suborno/layout-api/candidate.hpp"

namespace suborno::platform::windows {

inline constexpr GUID suborno_candidate_list_uielement_guid{
    0x0b8f6f43,
    0x8858,
    0x4e58,
    {0x9a, 0x2d, 0x4a, 0x6b, 0x31, 0x9d, 0x2c, 0x11},
};

// ITfCandidateListUIElementBehavior already derives from

// ITfCandidateListUIElement (C4584: listing both creates a duplicate base and

// makes interface casts ambiguous), so the behavior interface is the only

/// base this class needs — it carries every ITfCandidateListUIElement method.

class CandidateListUIElement final : public ITfCandidateListUIElementBehavior {
  public:
    using FinalizeCallback = std::function<HRESULT(UINT)>;
    using AbortCallback = std::function<HRESULT()>;

    // The Interface-tab candidate window options, resolved by the text

    // service from the settings file at composition time. Every field maps

    /// to a persisted settings key with a real runtime consumer in this class.

    struct Options {
        bool vertical_orientation = true;  ///< ui.candidate_orientation == vertical
        bool fixed_height = true;  ///< ui.candidate_fixed_height
        bool sliding_effect = true;  ///< ui.enable_sliding_effect
        bool follow_caret = true;  ///< ui.follow_caret
        bool follow_mouse_fallback = true;  ///< ui.follow_mouse_pointer_if_caret_tracking_fails
        std::wstring font_family;  ///< configured UI font; empty = "Nirmala UI"
    };

    CandidateListUIElement(ITfContext* context,
                           const std::vector<layout::Candidate>& candidates,
                           std::size_t selected_index,
                           const Options& options,
                           FinalizeCallback finalize_callback,
                           AbortCallback abort_callback);

    STDMETHOD(QueryInterface)(REFIID riid, void** object) override;
    STDMETHOD_(ULONG, AddRef)() override;
    STDMETHOD_(ULONG, Release)() override;

    STDMETHOD(GetDescription)(BSTR* description) override;
    STDMETHOD(GetGUID)(GUID* guid) override;
    STDMETHOD(Show)(BOOL show) override;
    STDMETHOD(IsShown)(BOOL* show) override;

    STDMETHOD(GetUpdatedFlags)(DWORD* flags) override;
    STDMETHOD(GetDocumentMgr)(ITfDocumentMgr** document_manager) override;
    STDMETHOD(GetCount)(UINT* count) override;
    STDMETHOD(GetSelection)(UINT* index) override;
    STDMETHOD(GetString)(UINT index, BSTR* text) override;
    STDMETHOD(GetPageIndex)(UINT* index, UINT size, UINT* page_count) override;
    STDMETHOD(SetPageIndex)(UINT* index, UINT page_count) override;
    STDMETHOD(GetCurrentPage)(UINT* page) override;

    STDMETHOD(SetSelection)(UINT index) override;
    STDMETHOD(Finalize)() override;
    STDMETHOD(Abort)() override;

    // True when the Interface tab selected the vertical list orientation.

    [[nodiscard]] bool is_vertical() const noexcept {
        return options_.vertical_orientation;
    }

    void Update(const std::vector<layout::Candidate>& candidates, std::size_t selected_index);
    static LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM w_param, LPARAM l_param);

  private:
    ~CandidateListUIElement();

    HRESULT show_native_window();
    void update_native_window() noexcept;
    void destroy_native_window() noexcept;
    static std::wstring to_wide(std::string_view utf8);
    void reset_page_index();
    void update_current_page_for_selection() noexcept;
    std::size_t current_page_start() const noexcept;
    std::size_t current_page_end() const noexcept;
    int scaled(int pixels) const noexcept;
    void refresh_dpi();
    void ensure_candidate_font();
    // Window metrics for the active orientation/fixed-height combination.

    std::size_t page_capacity() const noexcept;
    int window_width_for_page() const noexcept;
    int window_height_for_page() const noexcept;
    // Sliding-effect state (WM_TIMER driven; no timer exists when disabled).

    void start_slide_in();
    void advance_slide();
    void stop_slide_timer() noexcept;
    void apply_slide_offset() noexcept;
    // Hit testing shared by the click handler: page-local index or npos.

    std::size_t hit_candidate(POINT client_point) const noexcept;

    LONG reference_count_ = 1;
    wil::com_ptr<ITfContext> context_;
    std::vector<std::wstring> candidates_;
    std::vector<UINT> page_starts_;
    UINT selected_index_ = 0;
    UINT current_page_ = 0;
    DWORD updated_flags_ = TF_CLUIE_COUNT | TF_CLUIE_SELECTION | TF_CLUIE_STRING
                           | TF_CLUIE_PAGEINDEX | TF_CLUIE_CURRENTPAGE;
    bool shown_ = false;
    HWND window_ = nullptr;
    UINT_PTR slide_timer_id_ = 0;
    int slide_offset_ = 0;
    UINT dpi_ = 96;
    HFONT candidate_font_ = nullptr;
    Options options_;  ///< resolved Interface-tab options
    FinalizeCallback finalize_callback_;
    AbortCallback abort_callback_;
};

}  // namespace suborno::platform::windows
