// Owns the single Avro Phonetic engine instance. The engine carries parse
// state across calls internally but resets it at the top of every convert()
// call, so one process-wide instance preserves the original
// one-instance-per-engine model.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "avro_phonetic_engine.hpp"
#include "borno_compat_layouts.hpp"

namespace borno_compat {
namespace {

AvroPhoneticEngine& avro_engine() {
    static AvroPhoneticEngine engine;
    return engine;
}

}  // namespace

AvroCompatEngine::AvroCompatEngine() = default;

std::string AvroCompatEngine::transliterate(const std::string& input) {
    return avro_engine().convert(input);
}

}  // namespace borno_compat
