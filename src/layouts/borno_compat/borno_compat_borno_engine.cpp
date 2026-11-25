// Owns the single Borno Phonetic engine instance. The engine is stateless
// per convert() call; one process-wide instance preserves the original
// one-instance-per-engine model while keeping the header's tables
// single-copy.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "borno_compat_layouts.hpp"
#include "borno_phonetic_engine.hpp"

namespace borno_compat {
namespace {

BornoPhoneticEngine& borno_engine() {
    static BornoPhoneticEngine engine;
    return engine;
}

}  // namespace

BornoPhoneticCompatEngine::BornoPhoneticCompatEngine() = default;

std::string BornoPhoneticCompatEngine::transliterate(const std::string& input) {
    return borno_engine().convert(input);
}

}  // namespace borno_compat
