// Owns the single Suborno Classic engine instance. The engine is stateless
// per convert() call; one process-wide instance preserves the original
// one-instance-per-engine model while keeping the header's tables
// single-copy.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "compat_layouts.hpp"
#include "classic_phonetic_engine.hpp"

namespace compat {
namespace {

ClassicPhoneticEngine& classic_engine() {
    static ClassicPhoneticEngine engine;
    return engine;
}

}  // namespace

ClassicCompatEngine::ClassicCompatEngine() = default;

std::string ClassicCompatEngine::transliterate(const std::string& input) {
    return classic_engine().convert(input);
}

}  // namespace compat
