// Autocorrect dictionary loader — Suborno's own implementation.
//
// Owns the lazy, mutex-guarded corrections map shared by the phonetic
// engines. The JSON scanning, the search order for the dictionary file and
// the lazy-load-on-miss contract were written fresh for Suborno; the
// observable behaviour (what parses, what wins on lookup) is pinned by the
// unit tests and the in-repo parity regression suite
// (tests/unit/borno_compat_parity_test.cpp).
//
// SPDX-License-Identifier: GPL-3.0-or-later
// Behaviour provenance: BornoTSF autocorrect contract (GPL-3).
#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

/**
 * @brief Loads the shared autocorrection map and resolves lookups for the
 *        phonetic engines.
 *
 * The loader is a process-wide singleton guarded by an internal mutex: the
 * first lookup miss lazily reads the corrections file, later lookups hit the
 * in-memory map. The JSON content of the file is scanned with a small
 * purpose-built parser (no external JSON dependency). Thread safety: all
 * public members may be called from any thread; internal synchronization
 * serializes map access.
 */
class DictionaryLoader {
  public:
    // Process-wide loader instance.
    static DictionaryLoader& instance();

    // Reads the corrections file (explicit path, or the search order below)
    // into the map. Re-reading replaces the whole map.
    bool load_corrections(const std::wstring& corrections_file_path = L"");
    // True when `input` has an entry; `correction_text` receives the fix.
    bool lookup_correction(const std::string& input_text, std::string& correction_text);
    const std::unordered_map<std::string, std::string>& corrections_map() const;
    bool is_loaded() const {
        return is_loaded_;
    }

    // Engine/test hook: pin the dictionary location (wins over the search
    // order). Empty string clears the pin.
    static void set_override_path(const std::wstring& override_file_path);
    static void reset_override_path();

  private:
    DictionaryLoader() = default;
    ~DictionaryLoader() = default;

    std::wstring resolve_dictionary_file_path() const;
    bool parse_json_corrections(const std::string& json_text);

    std::unordered_map<std::string, std::string> correction_by_input_text_;
    bool is_loaded_ = false;
    std::mutex corrections_mutex_;
};
