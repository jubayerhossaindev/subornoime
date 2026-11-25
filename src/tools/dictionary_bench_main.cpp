// Dictionary benchmark: measures million-scale word-list performance.

//

//   suborno_dictionary_bench <db> [word_count=1000000] [queries=10000]

//

// Generates deterministic synthetic Bangla words (stem + syllable suffixes),

// bulk-inserts them, then times exact lookups, prefix lookups and cache hits.

// The synthetic words are stress data only — the production pack lives in

// data/seed_dictionary.tsv.

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "suborno/dictionary/dictionary.hpp"

namespace {

using Clock = std::chrono::steady_clock;

double milliseconds_since(Clock::time_point start, Clock::time_point end) {
    return std::chrono::duration<double, std::milli>(end - start).count();
}

const char* stems[] = {
    "ব্রহ্ম", "শিশির", "মন্দা", "স্বপ্ন", "কল্যাণ", "বিদ্যুৎ", "উদ্যান", "সংস্কৃ", "চৈতন্য", "শ্রাবণ"};
const char* short_syllables[] = {"টা", "ড়া", "নি", "মা", "পা", "লা", "সা", "বা", "দা", "রা"};

// Six index slots (10 each) give 1,000,000 unique, script-valid synthetic

// words; two-character syllables keep every word inside the 96-byte limit.

std::string synthetic_word(std::uint64_t index) {
    std::string word = stems[index % 10];
    for (std::uint64_t divisor = 10; divisor <= 100000; divisor *= 10) {
        word += short_syllables[(index / divisor) % 10];
    }
    return word;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: suborno_dictionary_bench <db> [word_count=1000000] [queries=10000]\n";
        return 2;
    }
    const std::uint64_t word_count = argc > 2 ? std::strtoull(argv[2], nullptr, 10) : 1000000;
    const std::uint64_t query_count = argc > 3 ? std::strtoull(argv[3], nullptr, 10) : 10000;
    if (word_count == 0) {
        // Query generation takes `rng() % word_count`; zero would be a

        // division by zero, so reject it up front like every other CLI arg.

        std::cerr << "word_count must be greater than zero.\n";
        return 2;
    }

    std::filesystem::remove_all(std::filesystem::path(std::string(argv[1]) + "-wal"));
    std::filesystem::remove_all(std::filesystem::path(std::string(argv[1]) + "-shm"));
    std::filesystem::remove(argv[1]);
    std::filesystem::remove(std::string(argv[1]) + "-wal");
    std::filesystem::remove(std::string(argv[1]) + "-shm");

    const auto start = Clock::now();
    suborno::dictionary::Dictionary dictionary(argv[1]);
    const auto schema_ms = milliseconds_since(start, Clock::now());

    const auto insert_start = Clock::now();
    dictionary.begin_bulk_write();
    for (std::uint64_t index = 0; index < word_count; ++index) {
        const std::string word = synthetic_word(index);
        dictionary.add_entry(word, word, static_cast<int>(50 + (index % 900)));
    }
    dictionary.commit_bulk_write();
    const auto insert_ms = milliseconds_since(insert_start, Clock::now());

    std::vector<std::string> queries;
    queries.reserve(query_count);
    std::mt19937_64 rng(0xC0FFEE);
    for (std::uint64_t index = 0; index < query_count; ++index) {
        queries.push_back(synthetic_word(rng() % word_count));
    }

    std::uint64_t hits = 0;
    const auto exact_start = Clock::now();
    for (const std::string& query : queries) {
        if (!dictionary.find_matches(query, 8).empty()) {
            ++hits;
        }
    }
    const auto exact_ms = milliseconds_since(exact_start, Clock::now());

    std::uint64_t prefix_hits = 0;
    const auto prefix_start = Clock::now();
    for (const std::string& query : queries) {
        // Drop the final 6-byte two-character syllable to form a prefix.

        if (!dictionary.find_prefix_matches(query.substr(0, query.size() - 6), 8).empty()) {
            ++prefix_hits;
        }
    }
    const auto prefix_ms = milliseconds_since(prefix_start, Clock::now());

    const auto cache_start = Clock::now();
    const auto cached = dictionary.find_matches(queries.front(), 8);
    const auto cache_ms = milliseconds_since(cache_start, Clock::now());
    (void)cached;

    const suborno::dictionary::DictionaryStats stats = dictionary.stats();
    std::cout << "db=" << argv[1] << "\n"
              << "schema_open_ms=" << schema_ms << "\n"
              << "words=" << stats.total_entries << "\n"
              << "bulk_insert_ms=" << insert_ms << " ("
              << (insert_ms > 0 ? static_cast<double>(word_count) / insert_ms * 1000.0 : 0)
              << " words/s)\n"
              << "exact_lookup_avg_us=" << exact_ms * 1000.0 / static_cast<double>(query_count)
              << " (hits=" << hits << "/" << query_count << ")\n"
              << "prefix_lookup_avg_us=" << prefix_ms * 1000.0 / static_cast<double>(query_count)
              << " (hits=" << prefix_hits << "/" << query_count << ")\n"
              << "cached_repeat_us=" << cache_ms * 1000.0 << "\n";
    return 0;
}
