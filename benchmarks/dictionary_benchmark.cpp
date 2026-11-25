#include <benchmark/benchmark.h>
#include <string>
#include <vector>

#include "suborno/dictionary/dictionary.hpp"

namespace {

// A throwaway database per benchmark run — dictionary I/O must never touch
// the developer's real user dictionary.
std::string temporary_database_path() {
    static int run_id = 0;
    return "/tmp/suborno_bench_dictionary_" + std::to_string(++run_id) + ".sqlite";
}

void BenchmarkDictionaryAddEntries(benchmark::State& state) {
    for (auto _ : state) {
        const std::string path = temporary_database_path();
        suborno::dictionary::Dictionary dictionary(path);
        dictionary.begin_bulk_write();
        for (int index = 0; index < static_cast<int>(state.range(0)); ++index) {
            const std::string word = "শব্দ" + std::to_string(index);
            dictionary.add_entry(word, word, 100 + index, "common", "cholito", "seed");
        }
        dictionary.commit_bulk_write();
        benchmark::DoNotOptimize(dictionary);
        std::remove(path.c_str());
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

void BenchmarkDictionaryFindMatches(benchmark::State& state) {
    const std::string path = temporary_database_path();
    suborno::dictionary::Dictionary dictionary(path);
    dictionary.begin_bulk_write();
    for (int index = 0; index < 1000; ++index) {
        const std::string word = "শব্দ" + std::to_string(index);
        dictionary.add_entry(word, word, 100 + index, "common", "cholito", "seed");
    }
    dictionary.commit_bulk_write();

    for (auto _ : state) {
        auto matches = dictionary.find_matches("শব্দ১", 10);
        benchmark::DoNotOptimize(matches);
    }
    std::remove(path.c_str());
}

void BenchmarkDictionaryIsValidWord(benchmark::State& state) {
    const std::vector<std::string> samples = {
        "আমি",
        "স্বপ্ন",
        "ami",
        "",
        "আমি1",
        "আমি তুমি",
    };
    for (auto _ : state) {
        for (const std::string& sample : samples) {
            benchmark::DoNotOptimize(suborno::dictionary::Dictionary::is_valid_word(sample));
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<long long>(samples.size()));
}

}  // namespace

BENCHMARK(BenchmarkDictionaryAddEntries)->Arg(1000)->Arg(10000);
BENCHMARK(BenchmarkDictionaryFindMatches);
BENCHMARK(BenchmarkDictionaryIsValidWord);
BENCHMARK_MAIN();
