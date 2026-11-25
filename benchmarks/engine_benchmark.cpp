#include <benchmark/benchmark.h>
#include <memory>
#include <string>
#include <utility>

#include "suborno/core/engine.hpp"
#include "suborno/layouts/phonetic/suborno_phonetic_layout.hpp"

namespace {

// The engine drives the whole pipeline (settings + dictionary + layout), so
// its benchmarks must run against throwaway state files only.
suborno::core::EngineConfig temporary_engine_config() {
    static int run_id = 0;
    const int id = ++run_id;
    suborno::core::EngineConfig config;
    config.settings_path = "/tmp/suborno_bench_settings_" + std::to_string(id) + ".ini";
    config.dictionary_path = "/tmp/suborno_bench_dictionary_" + std::to_string(id) + ".sqlite";
    return config;
}

std::unique_ptr<suborno::core::Engine> make_engine() {
    auto engine = std::make_unique<suborno::core::Engine>(temporary_engine_config());
    engine->register_layout("suborno-phonetic",
                            std::make_unique<suborno::layouts::SubornoPhoneticLayout>());
    static_cast<void>(engine->set_active_layout("suborno-phonetic"));
    return engine;
}

void BenchmarkEngineCreateSession(benchmark::State& state) {
    const auto engine = make_engine();
    for (auto _ : state) {
        auto session = engine->create_session();
        benchmark::DoNotOptimize(session);
    }
}

void BenchmarkEngineSessionPipeline(benchmark::State& state) {
    const auto engine = make_engine();
    const auto session = engine->create_session();
    for (auto _ : state) {
        static_cast<void>(session->append_utf8("suborno"));
        auto composition = session->snapshot();
        benchmark::DoNotOptimize(composition);
        if (!composition.candidates.empty()) {
            auto commit = session->commit_candidate(0);
            benchmark::DoNotOptimize(commit);
        }
        static_cast<void>(session->backspace());
    }
    state.SetItemsProcessed(state.iterations());
}

void BenchmarkEngineSessionTransliterateOnly(benchmark::State& state) {
    const auto engine = make_engine();
    const auto session = engine->create_session();
    for (auto _ : state) {
        static_cast<void>(session->append_utf8("bangla"));
        auto composition = session->snapshot();
        benchmark::DoNotOptimize(composition);
        static_cast<void>(session->backspace());
    }
    state.SetItemsProcessed(state.iterations());
}

}  // namespace

BENCHMARK(BenchmarkEngineCreateSession);
BENCHMARK(BenchmarkEngineSessionPipeline);
BENCHMARK(BenchmarkEngineSessionTransliterateOnly);
BENCHMARK_MAIN();
