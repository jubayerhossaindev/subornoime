// Autocorrect gate for the phonetic engines — Suborno's own implementation.
//
// Contract (verified against upstream by the in-repo parity regression suite,
// tests/unit/compat_parity_test.cpp):
//   - correction switch off  : output = input, returns false (pure pipeline)
//   - switch on, entry found : output = correction, returns (input == output)
//     — an identity correction short-circuits the phonetic pipeline upstream,
//       which is why the equality result matters to the callers
//   - switch on, no entry    : output = input, returns false
//
// SPDX-License-Identifier: GPL-3.0-or-later
// Behaviour provenance: the legacy reference behaviour contract; see LICENSE and THIRD_PARTY_NOTICES.md.
#pragma once

#include <string>

#include "compat_dictionary_loader.hpp"
#include "compat_settings.hpp"

/**
 * @brief Applies the autocorrection gate used by the phonetic pipelines.
 *
 * Respects the phonetic-correction setting: when disabled the input passes
 * through unchanged and the function reports false. When a correction entry
 * is found the corrected text replaces the output and the return value
 * signals an identity correction (correction equals input), which callers
 * use to short-circuit the rest of the pipeline.
 *
 * @param input_text Phonetic text produced by the parser.
 * @param corrected_text Receives the text the pipeline should continue with.
 * @return True when a correction entry matched and equals the input text.
 */
inline bool apply_autocorrect_gate(const std::string& input_text, std::string& corrected_text) {
    if (!compat::get_phonetic_correction()) {
        corrected_text = input_text;
        return false;
    }
    if (DictionaryLoader::instance().lookup_correction(input_text, corrected_text)) {
        return input_text == corrected_text;
    }
    corrected_text = input_text;
    return false;
}

/**
 * @brief Pre-parser fixup: applies the dictionary correction when the switch
 *        is on, otherwise passes the input through untouched.
 *
 * @param input_text Raw phonetic input before parsing.
 * @return Corrected phonetic text ready for parsing.
 */
inline std::string fix_phonetic_input(const std::string& input_text) {
    if (!compat::get_phonetic_correction()) {
        return input_text;
    }
    std::string corrected_text;
    if (DictionaryLoader::instance().lookup_correction(input_text, corrected_text)) {
        return corrected_text;
    }
    return input_text;
}
