// SPDX-License-Identifier: MIT
// Suborno glue between the settings store and the BornoTSF compatibility
// engines. The GPL engines read the phonetic-correction switch through this
// bridge instead of the original BornoTSF registry-backed Settings class;
// behaviour (default: enabled) is identical.
#pragma once

#include <atomic>

namespace borno_compat {

inline std::atomic_bool& phonetic_correction_flag() {
    static std::atomic_bool phonetic_correction_enabled{true};
    return phonetic_correction_enabled;
}

inline void set_phonetic_correction(bool enabled) noexcept {
    phonetic_correction_flag().store(enabled, std::memory_order_relaxed);
}

inline bool get_phonetic_correction() noexcept {
    return phonetic_correction_flag().load(std::memory_order_relaxed);
}

// Bundled-asset override paths. The engine injects its known assets
// directory (shared install location); tests inject the repository assets
// tree. Empty string = fall back to the standard search order.
inline std::wstring& khipro_spec_override() {
    static std::wstring path;
    return path;
}

inline void set_khipro_spec_override(const std::wstring& path) {
    khipro_spec_override() = path;
}

}  // namespace borno_compat
