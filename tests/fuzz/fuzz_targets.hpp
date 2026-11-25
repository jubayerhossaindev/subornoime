// Shared fuzz-target bodies for the Suborno IME fuzz harnesses.
//
// The same targets run under libFuzzer (tests/fuzz/*_fuzz.cpp, Clang only)
// and under the portable fuzz-smoke driver (tests/fuzz/fuzz_smoke_main.cpp,
// any compiler). Keeping the target bodies in one place guarantees that the
// smoke test exercises exactly the code the libFuzzer campaign exercises.
//
// Every target enforces the project input limits itself: an IME never sees
// unbounded keyboard input, so the fuzz targets reject inputs above the
// documented maximum instead of allocating from unbounded data.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "suborno/layouts/phonetic/phonetic_input_session.hpp"
#include "suborno/suborno.h"

namespace suborno::fuzz {

// The composition buffer limit enforced by the engine (see
// docs/engineering/INPUT_VALIDATION.md). Fuzz inputs above this size are
// truncated before they reach the engine so the target models the real
// service boundary.
constexpr std::size_t maximum_fuzz_input_size = 4096;

inline std::string truncated_utf8_input(const uint8_t* data, std::size_t size) {
    if (data == nullptr || size == 0) {
        return {};
    }
    const std::size_t bounded_size =
        size < maximum_fuzz_input_size ? size : maximum_fuzz_input_size;
    return std::string(reinterpret_cast<const char*>(data), bounded_size);
}

// Phonetic pipeline target: append -> snapshot -> backspace -> commit ->
// reset. Exercises the phonetic parser, the Unicode conversion path and the
// candidate generator with arbitrary (possibly invalid) UTF-8.
inline void run_phonetic_target(const uint8_t* data, std::size_t size) {
    layouts::PhoneticInputSession session;
    session.append_utf8(truncated_utf8_input(data, size));
    session.snapshot();
    session.backspace();
    const auto committed = session.commit();
    (void)committed;
    session.reset();
}

// Public C API target: full engine/session lifecycle. The fuzzed byte string
// drives both the composition text and a bounded candidate index, so the
// out-of-range and buffer-too-small return paths run on every campaign.
inline void run_c_api_target(const uint8_t* data, std::size_t size) {
    SUBORNO_Engine* engine = suborno_engine_create();
    if (engine == nullptr) {
        return;
    }
    SUBORNO_Session* session = suborno_session_create(engine);
    if (session != nullptr) {
        const std::string input = truncated_utf8_input(data, size);
        (void)suborno_session_append_utf8(session, input.c_str());

        char snapshot_buffer[256];
        (void)suborno_session_snapshot(session, snapshot_buffer, sizeof(snapshot_buffer));

        std::size_t candidate_count = 0;
        (void)suborno_session_candidate_count(session, &candidate_count);
        if (candidate_count != 0) {
            // Derive a candidate index from the input so both the in-range
            // and the out-of-range branch run during a campaign.
            const std::size_t index =
                static_cast<std::size_t>(input.empty() ? 0 : input[0]) % candidate_count;
            char candidate_buffer[128];
            (void)suborno_session_candidate_snapshot(
                session, index, candidate_buffer, sizeof(candidate_buffer));
            (void)suborno_session_candidate_snapshot(
                session, candidate_count, candidate_buffer, sizeof(candidate_buffer));
            (void)suborno_session_select_candidate(session, index);
        }
        (void)suborno_session_backspace(session);

        char* committed = nullptr;
        std::size_t committed_size = 0;
        (void)suborno_session_commit(session, &committed, &committed_size);
        suborno_string_free(committed);
        suborno_session_destroy(session);
    }
    suborno_engine_destroy(engine);
}

}  // namespace suborno::fuzz
