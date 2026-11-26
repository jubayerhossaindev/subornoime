// Owns the single Suborno Smart engine instance. The engine carries parse
// state across calls internally but resets it at the top of every convert()
// call, so one process-wide instance preserves the original
// one-instance-per-engine model.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "smart_phonetic_engine.hpp"
#include "compat_layouts.hpp"

namespace compat {
namespace {

SmartPhoneticEngine& smart_engine() {
    static SmartPhoneticEngine engine;
    return engine;
}

}  // namespace

SmartCompatEngine::SmartCompatEngine() = default;

std::string SmartCompatEngine::transliterate(const std::string& input) {
    return smart_engine().convert(input);
}

}  // namespace compat
