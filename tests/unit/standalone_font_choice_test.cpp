// Unit tests for the bundled-font choice policy of the standalone host.

//

// The preview font decision is deliberately OS-independent: font_loader.cpp

// (Windows-only) fills a BundledFontState from AddFontResourceExW results and

// then delegates to resolve_preview_font(), a pure function. These tests pin

// the whole precedence policy so the brand-font/fallback behaviour of the

// floating preview is verified on every platform (Ubuntu CI and the MSVC v143

// build) without any Windows dependency. The family-name strings must stay in

// sync with the name tables of the vendored TTFs (assets/fonts/FONT-CREDITS.md).

#include <gtest/gtest.h>
#include <string>

#include "platform/windows/standalone/preview_font_choice.hpp"

namespace {

using suborno::platform::windows::standalone::BundledFontState;
using suborno::platform::windows::standalone::font_weight_medium;
using suborno::platform::windows::standalone::font_weight_normal;
using suborno::platform::windows::standalone::PreviewFontChoice;
using suborno::platform::windows::standalone::resolve_preview_font;

PreviewFontChoice resolve(const BundledFontState& state, const std::wstring& user = L"") {
    return resolve_preview_font(state, user);
}

TEST(StandalonePreviewFont, UserPickBeatsEverything) {
    BundledFontState state;
    state.hind_siliguri_medium = true;
    const PreviewFontChoice choice = resolve(state, L"Kalpurush");
    EXPECT_EQ(choice.family, std::wstring(L"Kalpurush"));
    EXPECT_EQ(choice.weight, font_weight_normal);
}

TEST(StandalonePreviewFont, UserPickEvenWhenFontsMissing) {
    BundledFontState state;  // nothing loaded

    const PreviewFontChoice choice = resolve(state, L"Nirmala UI");
    EXPECT_EQ(choice.family, std::wstring(L"Nirmala UI"));
    EXPECT_EQ(choice.weight, font_weight_normal);
}

TEST(StandalonePreviewFont, BundledMediumWinsOverRegular) {
    BundledFontState state;
    state.hind_siliguri_regular = true;
    state.hind_siliguri_medium = true;
    const PreviewFontChoice choice = resolve(state);
    EXPECT_EQ(choice.family, std::wstring(L"Hind Siliguri Medium"));
    EXPECT_EQ(choice.weight, font_weight_medium);
}

TEST(StandalonePreviewFont, RegularOnlyFallsBackToRegularFace) {
    BundledFontState state;
    state.hind_siliguri_regular = true;
    const PreviewFontChoice choice = resolve(state);
    EXPECT_EQ(choice.family, std::wstring(L"Hind Siliguri"));
    EXPECT_EQ(choice.weight, font_weight_normal);
}

TEST(StandalonePreviewFont, NothingLoadedFallsBackToNirmalaUi) {
    BundledFontState state;  // fonts folder absent or every AddFontResourceExW failed

    const PreviewFontChoice choice = resolve(state);
    EXPECT_EQ(choice.family, std::wstring(L"Nirmala UI"));
    EXPECT_EQ(choice.weight, font_weight_normal);
}

TEST(StandalonePreviewFont, NotoAndBalooDoNotDriveThePreviewPolicy) {
    // The fallback + display faces are vendored for the candidate list

    // options and branding; the preview policy must not resolve to them.

    BundledFontState state;
    state.noto_sans_bengali = true;
    state.files_loaded = 3;
    const PreviewFontChoice choice = resolve(state);
    EXPECT_EQ(choice.family, std::wstring(L"Nirmala UI"));
    EXPECT_EQ(choice.weight, font_weight_normal);
}

TEST(StandalonePreviewFont, OversizedUserFamilyIsIgnored) {
    // LF_FACESIZE guard: a settings value longer than a GDI face name can

    // never reach CreateFontW; the policy falls through to the brand font.

    BundledFontState state;
    state.hind_siliguri_medium = true;
    const std::wstring oversized(64, L'A');
    const PreviewFontChoice choice = resolve(state, oversized);
    EXPECT_EQ(choice.family, std::wstring(L"Hind Siliguri Medium"));
    EXPECT_EQ(choice.weight, font_weight_medium);
}

}  // namespace
