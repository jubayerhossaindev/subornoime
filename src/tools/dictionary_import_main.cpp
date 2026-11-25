// Dictionary word-list management tool.
//
// Usage:
//   suborno_dictionary_import <db> <words.tsv>                 legacy import (replace)
//   suborno_dictionary_import <db> import <words.tsv> [--mode=replace|merge] [--origin=X]
//   suborno_dictionary_import <db> export <out.tsv>            word|frequency|category|register
//   suborno_dictionary_import <db> relations <relations.tsv>   word|kind|related
//   suborno_dictionary_import <db> validate                    integrity report (exit 1 on problems)
//   suborno_dictionary_import <db> stats                       counts, categories, tiers
//   suborno_dictionary_import <db> dedupe [--apply]            duplicate-normalized report / fix
//   suborno_dictionary_import <db> backup <dest.sqlite>        consistent online backup
//   suborno_dictionary_import <db> vacuum                      rebuild + compact the database
//
// Words TSV: word [TAB] frequency [TAB] category [TAB] register
// (2-column legacy files are accepted; '#' lines are comments).

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "suborno/dictionary/dictionary.hpp"

namespace {

std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::vector<std::string> split_tsv(const std::string& line) {
    std::vector<std::string> fields;
    std::size_t start = 0;
    while (true) {
        const auto tab = line.find('\t', start);
        if (tab == std::string::npos) {
            fields.push_back(line.substr(start));
            break;
        }
        fields.push_back(line.substr(start, tab - start));
        start = tab + 1;
    }
    return fields;
}

std::uint32_t parse_frequency(const std::string& text, bool* ok) {
    try {
        const unsigned long long parsed = std::stoull(text);
        *ok = true;
        return static_cast<std::uint32_t>(parsed > 0x7FFFFFFFLL ? 0x7FFFFFFFLL : parsed);
    } catch (...) {
        *ok = false;
        return 1;
    }
}

int run_import(suborno::dictionary::Dictionary& dictionary,
               const std::filesystem::path& wordlist_path,
               bool merge,
               const std::string& origin) {
    std::ifstream wordlist(wordlist_path);
    if (!wordlist) {
        std::cerr << "Cannot open wordlist: " << wordlist_path.string() << "\n";
        return 2;
    }
    dictionary.begin_bulk_write();
    std::uint64_t imported = 0;
    std::uint64_t invalid = 0;
    std::string line;
    try {
        while (std::getline(wordlist, line)) {
            if (line.empty() || line.front() == '#') {
                continue;
            }
            const std::vector<std::string> fields = split_tsv(line);
            const std::string word = trim(fields[0]);
            std::uint32_t frequency = 1;
            bool frequency_ok = true;
            std::string category;
            std::string word_register;
            if (fields.size() >= 2) {
                frequency = parse_frequency(trim(fields[1]), &frequency_ok);
            }
            if (fields.size() >= 3) {
                category = trim(fields[2]);
            }
            if (fields.size() >= 4) {
                word_register = trim(fields[3]);
            }
            if (word.empty() || !frequency_ok
                || !suborno::dictionary::Dictionary::is_valid_word(word)) {
                ++invalid;
                if (invalid <= 20) {
                    std::cerr << "Invalid word rejected at line: " << line << "\n";
                }
                continue;
            }
            // The normalized key of a Bangla word list entry is the Bangla
            // word itself; transliteration keys come from the engine at
            // typing time, not from the pack.
            const std::string normalized = word;
            if (merge) {
                dictionary.add_entry_merge(
                    word, normalized, static_cast<int>(frequency), category, word_register, origin);
            } else {
                dictionary.add_entry(
                    word, normalized, static_cast<int>(frequency), category, word_register, origin);
            }
            ++imported;
        }
        dictionary.commit_bulk_write();
    } catch (...) {
        dictionary.rollback_bulk_write();
        throw;
    }
    std::cout << "Imported " << imported << " words (mode=" << (merge ? "merge" : "replace")
              << ", origin=" << origin << "); rejected " << invalid << " invalid lines.\n";
    return imported == 0 ? 2 : 0;
}

int run_export(suborno::dictionary::Dictionary& dictionary,
               const std::filesystem::path& output_path) {
    std::ofstream output(output_path);
    if (!output) {
        std::cerr << "Cannot open output: " << output_path.string() << "\n";
        return 2;
    }
    output << "# word\tfrequency\tcategory\tregister\n";
    std::uint64_t exported = 0;
    for (const suborno::dictionary::WordRecord& record : dictionary.all_entries()) {
        if (record.source == "user" && record.user_frequency == 0) {
            continue;  // transient learned rows are not part of the word pack
        }
        output << record.word << "\t" << record.frequency << "\t" << record.category << "\t"
               << record.word_register << "\n";
        ++exported;
    }
    std::cout << "Exported " << exported << " words to " << output_path.string() << "\n";
    return exported == 0 ? 2 : 0;
}

int run_relations(suborno::dictionary::Dictionary& dictionary,
                  const std::filesystem::path& relations_path) {
    std::ifstream relations(relations_path);
    if (!relations) {
        std::cerr << "Cannot open relations file: " << relations_path.string() << "\n";
        return 2;
    }
    std::uint64_t added = 0;
    std::uint64_t rejected = 0;
    std::string line;
    while (std::getline(relations, line)) {
        if (line.empty() || line.front() == '#') {
            continue;
        }
        const std::vector<std::string> fields = split_tsv(line);
        if (fields.size() < 3) {
            ++rejected;
            continue;
        }
        const std::string word = trim(fields[0]);
        const std::string kind = trim(fields[1]);
        const std::string related = trim(fields[2]);
        try {
            dictionary.add_word_relation(word, related, kind);
            ++added;
        } catch (const std::exception& error) {
            ++rejected;
            if (rejected <= 20) {
                std::cerr << "Rejected relation: " << line << " (" << error.what() << ")\n";
            }
        }
    }
    std::cout << "Added " << added << " word relations; rejected " << rejected << ".\n";
    return added == 0 ? 2 : 0;
}

int run_validate(suborno::dictionary::Dictionary& dictionary) {
    std::uint64_t problems = 0;
    const auto entries = dictionary.all_entries();
    for (const suborno::dictionary::WordRecord& record : entries) {
        if (!suborno::dictionary::Dictionary::is_valid_word(record.word)) {
            ++problems;
            std::cout << "INVALID-WORD\t" << record.word << "\n";
        }
        if (!suborno::dictionary::Dictionary::is_valid_normalized(record.normalized)) {
            ++problems;
            std::cout << "INVALID-NORMALIZED\t" << record.word << "\t" << record.normalized << "\n";
        }
    }
    for (const auto& group : dictionary.find_duplicate_normalized()) {
        ++problems;
        std::cout << "DUPLICATE-NORMALIZED\t" << group.normalized;
        for (const std::string& word : group.words) {
            std::cout << "\t" << word;
        }
        std::cout << "\n";
    }
    if (problems == 0) {
        std::cout << "Validation clean (" << entries.size() << " entries checked).\n";
        return 0;
    }
    std::cout << "Validation problems: " << problems << "\n";
    return 1;
}

int run_stats(suborno::dictionary::Dictionary& dictionary) {
    const suborno::dictionary::DictionaryStats stats = dictionary.stats();
    std::cout << "schema_version=" << dictionary.schema_version() << "\n"
              << "total_entries=" << stats.total_entries << "\n"
              << "system_entries=" << stats.system_entries << "\n"
              << "user_entries=" << stats.user_entries << "\n"
              << "user_aliases=" << stats.user_aliases << "\n"
              << "word_relations=" << stats.word_relations << "\n"
              << "changelog_entries=" << stats.changelog_entries << "\n";
    for (const auto& [category, count] : stats.top_categories) {
        std::cout << "category\t" << category << "\t" << count << "\n";
    }
    for (const auto& [tier, count] : stats.frequency_tiers) {
        std::cout << "tier\t" << tier << "\t" << count << "\n";
    }
    return 0;
}

int run_dedupe(suborno::dictionary::Dictionary& dictionary, bool apply) {
    const auto groups = dictionary.find_duplicate_normalized();
    std::uint64_t removed = 0;
    for (const auto& group : groups) {
        if (group.words.size() < 2) {
            continue;
        }
        std::cout << "DUPLICATE\t" << group.normalized;
        for (const std::string& word : group.words) {
            std::cout << "\t" << word;
        }
        std::cout << "\n";
        if (apply) {
            // Keep the highest-frequency form; remove the rest, but never
            // touch user-sourced or user-learned words.
            for (std::size_t index = 1; index < group.words.size(); ++index) {
                if (dictionary.get_user_frequency(group.words[index]) != 0) {
                    std::cout << "KEEP-USER\t" << group.words[index] << "\n";
                    continue;
                }
                dictionary.remove_user_word(group.words[index]);
                ++removed;
            }
        }
    }
    std::cout << "Duplicate groups: " << groups.size();
    if (apply) {
        std::cout << "; removed " << removed;
    } else {
        std::cout << " (pass --apply to remove the lower-frequency forms)";
    }
    std::cout << "\n";
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: suborno_dictionary_import <db> <words.tsv | mode> [args...]\n"
                     "Modes: import|export|relations|validate|stats|dedupe|backup|vacuum\n";
        return 2;
    }
    const std::filesystem::path database_path = argv[1];
    const std::string mode = argv[2];

    suborno::dictionary::Dictionary dictionary(database_path.string());

    if (mode == "validate") {
        return run_validate(dictionary);
    }
    if (mode == "stats") {
        return run_stats(dictionary);
    }
    if (mode == "vacuum") {
        dictionary.vacuum();
        std::cout << "Vacuum complete.\n";
        return 0;
    }
    // Legacy form: the second positional argument is a words TSV (2 args total).
    if (mode.find(".tsv") != std::string::npos || mode.find(".txt") != std::string::npos) {
        return run_import(dictionary, std::filesystem::path(mode), false, "seed");
    }
    // Only these modes take a required positional argument; validate/stats/
    // vacuum/dedupe run bare (dedupe's --apply is optional, see usage above).
    const bool mode_requires_argument =
        (mode == "import" || mode == "export" || mode == "relations" || mode == "backup");
    if (argc < 4 && mode_requires_argument) {
        std::cerr << "Mode '" << mode << "' requires an argument.\n";
        return 2;
    }
    if (argc < 4 && mode != "dedupe") {
        // Unknown short-argument mode: fall through to the unknown-mode error
        // so the CLI never misreports a typo as a missing argument.
        std::cerr << "Unknown mode: " << mode << "\n";
        return 2;
    }
    if (mode == "import") {
        bool merge = false;
        std::string origin = "seed";
        for (int index = 4; index < argc; ++index) {
            const std::string flag = argv[index];
            if (flag.rfind("--mode=", 0) == 0) {
                merge = flag.substr(7) == "merge";
            } else if (flag.rfind("--origin=", 0) == 0) {
                origin = flag.substr(9);
            } else {
                std::cerr << "Unknown flag: " << flag << "\n";
                return 2;
            }
        }
        return run_import(dictionary, std::filesystem::path(argv[3]), merge, origin);
    }
    if (mode == "export") {
        return run_export(dictionary, std::filesystem::path(argv[3]));
    }
    if (mode == "relations") {
        return run_relations(dictionary, std::filesystem::path(argv[3]));
    }
    if (mode == "backup") {
        dictionary.backup_to(argv[3]);
        std::cout << "Backup written to " << argv[3] << "\n";
        return 0;
    }
    if (mode == "dedupe") {
        // --apply sits at argv[3] (right after the mode word); the optional
        // flag used to be read from argv[4], which made bare `dedupe --apply`
        // silently behave as a report-only run.
        return run_dedupe(dictionary, argc > 3 && std::string(argv[3]) == "--apply");
    }
    std::cerr << "Unknown mode: " << mode << "\n";
    return 2;
}
