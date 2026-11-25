#pragma once
//

// Bundled-font choice policy for the standalone typing host.

//

// WHY THIS IS A SEPARATE, WINDOWS-FREE HEADER

// The choice logic (which family/weight the floating preview should use) is a

// pure function over (bundled-font state, user preference). Keeping it free of

// Windows.h lets the portable unit suite (tests/unit) exercise the policy on

// every CI platform, while the Windows-only loader (font_loader.hpp) and the

// preview window (preview_window.hpp) consume its results.

//

// FONT DATA SOURCES

// The family names below must match the name tables of the bundled TTFs

// (verified with fonttools; see assets/fonts/FONT-CREDITS.md):

//   "Hind Siliguri" / "Hind Siliguri Medium" / "Hind Siliguri SemiBold" ...

//   "Noto Sans Bengali" / "Noto Sans Bengali Med"

//   "Baloo Da 2" / "Baloo Da 2 SemiBold"

// Non-RIBBI styles ship their weight inside the family name, so GDI matching

// needs the exact string, not just "Hind Siliguri" + a weight.

#include <cstddef>
#include <string>

namespace suborno::platform::windows::standalone {

// Win32 FW_* weight values, restated here so the pure header stays

// independent of Windows.h (values are fixed by the Win32 API).

constexpr int font_weight_normal = 400;
constexpr int font_weight_medium = 500;
constexpr int font_weight_semibold = 600;
constexpr int font_weight_bold = 700;

// Which bundled families the loader actually registered in this process.

// Files are detected by their file-name prefix (the vendored set has stable

// names); see font_loader.cpp for the mapping.

struct BundledFontState {
    bool hind_siliguri_regular = false;
    bool hind_siliguri_medium = false;
    bool noto_sans_bengali = false;
    int files_loaded = 0;
};

// The resolved GDI face for the floating preview.

struct PreviewFontChoice {
    std::wstring family{L"Nirmala UI"};
    int weight = font_weight_normal;
};

inline bool operator==(const PreviewFontChoice& left, const PreviewFontChoice& right) {
    return left.family == right.family && left.weight == right.weight;
}
inline bool operator!=(const PreviewFontChoice& left, const PreviewFontChoice& right) {
    return !(left == right);
}

// Preview font policy (documented in docs/zero-dependency/FONT_BUNDLING_BANGLA.md):

//   1. An explicit user choice wins — the caller passes the wide-string

//      conversion of settings.ui.font_family only when the user actually

//      picked a family (i.e. !use_os_default_bangla_font && !family.empty()).

//   2. Otherwise the bundled Hind Siliguri is Suborno's brand look for its own

//      floating preview chrome — available on ANY machine, no language pack,

//      no font install, no Win+Space. Medium first (best clarity at 22 px on

//      the dark preview token), Regular as the safe fallback.

//   3. If nothing bundled loaded (fonts folder missing / corrupted), fall back

//      to Nirmala UI, which ships with Windows 8+ so Bengali never renders as

//      broken boxes.

//

// Inline header-only implementation: the portable unit suite links this logic

// directly, so the Windows-only translation unit (font_loader.cpp) must not

// be needed for the tests to run.

//

// GDI face names are limited to LF_FACESIZE (32 wchar_t, fixed by the Win32

// API); restated here so this header stays free of Windows.h.

constexpr std::size_t max_gdi_face_name_length = 32;

inline PreviewFontChoice resolve_preview_font(const BundledFontState& state,
                                              const std::wstring& user_family) noexcept {
    // 1. Explicit user pick wins. GDI cannot match a face name longer than

    //    LF_FACESIZE anyway, so an oversized settings value is ignored here

    //    instead of reaching CreateFontW.

    if (!user_family.empty() && user_family.size() < max_gdi_face_name_length) {
        return PreviewFontChoice{user_family, font_weight_normal};
    }
    // 2. Bundled brand faces for Suborno's own preview chrome. Medium is the

    //    clearest weight for 22 px ClearType text on the dark token

    //    (#1B1B1F); Regular covers the case where the Medium file is absent.

    if (state.hind_siliguri_medium) {
        return PreviewFontChoice{L"Hind Siliguri Medium", font_weight_medium};
    }
    if (state.hind_siliguri_regular) {
        return PreviewFontChoice{L"Hind Siliguri", font_weight_normal};
    }
    // 3. Nirmala UI ships with Windows 8+ (12 scripts incl. Bengali), so even

    //    a machine without the bundled files renders Bengali — just in the

    //    generic system face instead of the brand face.

    return PreviewFontChoice{L"Nirmala UI", font_weight_normal};
}

}  // namespace suborno::platform::windows::standalone
