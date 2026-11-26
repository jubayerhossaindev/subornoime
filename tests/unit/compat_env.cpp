// Test-wide environment: point the legacy reference IME compatibility engines at the

// repository assets tree so the Khipro mim spec and the autocorrect

// dictionary load deterministically no matter which directory ctest runs

// from. Mirrors the EngineConfig assets_path wiring used in production.

#include <gtest/gtest.h>
#include <string>

#include "compat_dictionary_loader.hpp"
#include "compat_settings.hpp"

#ifndef SUBORNO_TEST_ASSETS_DIR
#define SUBORNO_TEST_ASSETS_DIR ""
#endif

namespace {

std::wstring ascii_to_wide(const std::string& text) {
    return std::wstring(text.begin(), text.end());
}

class CompatAssetsEnvironment : public ::testing::Environment {
  public:
    void SetUp() override {
        const std::string assets = SUBORNO_TEST_ASSETS_DIR;
        if (assets.empty()) {
            return;
        }
        compat::set_khipro_spec_override(ascii_to_wide(assets) + L"/layouts/bn-khipro.mim");
        DictionaryLoader::set_override_path(ascii_to_wide(assets) + L"/dictionaries/ac.json");
    }
};

[[maybe_unused]] const ::testing::Environment* const compat_assets_env =
    ::testing::AddGlobalTestEnvironment(new CompatAssetsEnvironment());

}  // namespace
