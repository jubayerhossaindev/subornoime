// Portable fuzz-smoke driver: runs the shared fuzz targets over the seed
// corpus plus deterministic mutations, on any compiler (GCC, MSVC, Clang).
//
// This driver gives every CI run - including MSVC-only environments where
// libFuzzer is unavailable - a real fuzz smoke stage: crash, hang (alarm
// timeout) and input-limit regressions fail the build. The deep campaigns
// keep running under libFuzzer in tests/fuzz/*_fuzz.cpp via fuzz.yml.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#if defined(_WIN32)
// MSVC has no POSIX alarm(); the CI job timeout covers hangs on Windows.
#define SUBORNO_FUZZ_SMOKE_HAS_ALARM 0
#else
#include <unistd.h>
#define SUBORNO_FUZZ_SMOKE_HAS_ALARM 1
#endif

#include "fuzz_targets.hpp"

namespace {

constexpr std::uint32_t mutation_count_per_seed = 64;
constexpr unsigned fuzz_smoke_timeout_seconds = 60;

std::vector<std::vector<std::uint8_t>> load_seed_corpus(const std::filesystem::path& corpus_dir) {
    std::vector<std::vector<std::uint8_t>> seeds;
    if (!std::filesystem::exists(corpus_dir)) {
        return seeds;
    }
    for (const auto& entry : std::filesystem::directory_iterator(corpus_dir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        FILE* seed_file = std::fopen(entry.path().string().c_str(), "rb");
        if (seed_file == nullptr) {
            continue;
        }
        std::vector<std::uint8_t> bytes(
            static_cast<std::size_t>(std::filesystem::file_size(entry.path())));
        const std::size_t read_count =
            bytes.empty() ? 0 : std::fread(bytes.data(), 1, bytes.size(), seed_file);
        std::fclose(seed_file);
        bytes.resize(read_count);
        seeds.push_back(std::move(bytes));
    }
    return seeds;
}

// Deterministic mutation set: truncation, byte flips, repetition and splice.
// Every mutation is reproducible from the seed alone, so a crash reproduces
// without storing the mutated input separately.
std::vector<std::uint8_t> mutate_seed(const std::vector<std::uint8_t>& seed,
                                      std::uint32_t mutation_index) {
    std::vector<std::uint8_t> mutated = seed;
    const std::uint32_t kind = mutation_index % 4;
    const std::size_t flip_position = mutation_index % (mutated.empty() ? 1 : mutated.size());
    switch (kind) {
    case 0:
        if (!mutated.empty()) {
            mutated.resize(flip_position);
        }
        break;
    case 1:
        if (!mutated.empty()) {
            mutated[flip_position] = static_cast<std::uint8_t>(mutated[flip_position] ^ 0xFF);
        }
        break;
    case 2:
        if (!mutated.empty()) {
            mutated.insert(mutated.end(), seed.begin(), seed.end());
            if (mutated.size() > suborno::fuzz::maximum_fuzz_input_size) {
                mutated.resize(suborno::fuzz::maximum_fuzz_input_size);
            }
        }
        break;
    default:
        if (!mutated.empty()) {
            mutated[flip_position] = static_cast<std::uint8_t>(0x80u + (mutation_index % 0x40u));
        }
        break;
    }
    return mutated;
}

void run_both_targets(const std::vector<std::uint8_t>& bytes) {
    suborno::fuzz::run_phonetic_target(bytes.data(), bytes.size());
    suborno::fuzz::run_c_api_target(bytes.data(), bytes.size());
}

}  // namespace

int main(int argc, char** argv) {
    std::filesystem::path corpus_dir = "fuzz_corpus";
    if (argc > 1) {
        corpus_dir = argv[1];
    }

#if SUBORNO_FUZZ_SMOKE_HAS_ALARM
    ::alarm(fuzz_smoke_timeout_seconds);
#endif
    const std::vector<std::vector<std::uint8_t>> seeds = load_seed_corpus(corpus_dir);
    std::size_t executed_runs = 0;

    for (const auto& seed : seeds) {
        run_both_targets(seed);
        ++executed_runs;
        for (std::uint32_t mutation_index = 0; mutation_index < mutation_count_per_seed;
             ++mutation_index) {
            run_both_targets(mutate_seed(seed, mutation_index));
            ++executed_runs;
        }
    }

#if SUBORNO_FUZZ_SMOKE_HAS_ALARM
    ::alarm(0);
#endif
    std::printf(
        "fuzz smoke: %zu seeds, %zu runs executed, no crashes\n", seeds.size(), executed_runs);
    return seeds.empty() ? 1 : 0;
}
