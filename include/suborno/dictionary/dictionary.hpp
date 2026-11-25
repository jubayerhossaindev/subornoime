#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace suborno::dictionary {

enum class DictionaryErrorCategory : std::uint8_t {
    open,
    schema,
    prepare,
    bind,
    step,
    integrity,
    validation,
};

class DictionaryError final : public std::runtime_error {
  public:
    DictionaryError(DictionaryErrorCategory category, int sqlite_code, const std::string& message);

    [[nodiscard]] DictionaryErrorCategory category() const noexcept;
    [[nodiscard]] int sqlite_code() const noexcept;

  private:
    DictionaryErrorCategory category_;
    int sqlite_code_;
};

/// A normalized key that maps to more than one stored word (for example the
/// same romanization behind two different Bangla spellings). Reported by
/// find_duplicate_normalized(); duplicates on the word column itself are
/// impossible (UNIQUE constraint).
struct DuplicateGroup {
    std::string normalized;
    std::vector<std::string> words;
};

struct DictionaryStats {
    std::uint64_t total_entries = 0;
    std::uint64_t system_entries = 0;
    std::uint64_t user_entries = 0;
    std::uint64_t user_aliases = 0;
    std::uint64_t word_relations = 0;
    std::uint64_t word_transitions = 0;
    std::uint64_t changelog_entries = 0;
    std::vector<std::pair<std::string, std::uint64_t>> top_categories;
    std::vector<std::pair<std::string, std::uint64_t>> frequency_tiers;
};

/// Complete stored row (word-list management and export).
struct WordRecord {
    std::string word;
    std::string normalized;
    int frequency = 0;
    int user_frequency = 0;
    bool user_pinned = false;
    std::string source;  ///< system | user
    std::string category;  ///< free-text group, e.g. common / literary / regional / conjunct
    std::string word_register;  ///< sadhu / cholito / colloquial / literary / regional
    std::string origin;  ///< seed / corpus / manual / import
};

class Dictionary {
  public:
    explicit Dictionary(const std::string& database_path);
    ~Dictionary();
    Dictionary(const Dictionary&) = delete;
    Dictionary& operator=(const Dictionary&) = delete;
    Dictionary(Dictionary&&) = delete;
    Dictionary& operator=(Dictionary&&) = delete;

    // Word validation (no database access; safe to call anywhere).
    // A valid dictionary word: non-empty UTF-8, at most 96 bytes, contains at
    // least one Bengali-block character, no ASCII letters/digits/punctuation,
    // no spaces or control characters (ZWNJ/ZWJ combinators are allowed).
    [[nodiscard]] static bool is_valid_word(const std::string& word);
    [[nodiscard]] static bool is_valid_normalized(const std::string& normalized);

    void add_entry(const std::string& word,
                   const std::string& normalized,
                   int frequency = 1,
                   const std::string& category = "",
                   const std::string& word_register = "",
                   const std::string& origin = "seed");
    /// Incremental-update variant: on conflict keeps the higher frequency and
    /// only fills empty category/register/origin (never downgrades metadata).
    void add_entry_merge(const std::string& word,
                         const std::string& normalized,
                         int frequency = 1,
                         const std::string& category = "",
                         const std::string& word_register = "",
                         const std::string& origin = "seed");
    void record_commit(const std::string& word, const std::string& normalized);
    void record_commit(const std::string& word,
                       const std::string& canonical_normalized,
                       const std::string& user_alias);

    /// Next-word transition model (bigram history for prediction). Both keys
    /// must already be normalized NFC Bengali — callers normalize before
    /// calling, so the only gate here is non-empty (empty inputs are ignored
    /// because they would poison the bigram key).
    void record_transition(const std::string& previous_normalized,
                           const std::string& next_normalized);
    // Words that historically followed `previous_normalized`, most frequent
    // first (ties broken alphabetically for a stable order), filtered to those
    // starting with `prefix` (empty prefix = no filter), capped at `limit`.
    // Returns the stored normalized next words; empty previous returns {}.
    [[nodiscard]] std::vector<std::string> transition_candidates(
        const std::string& previous_normalized, const std::string& prefix, std::size_t limit) const;

    // Bulk import helpers for tools that insert many rows in one transaction
    // (thousands of add_entry calls otherwise cost one implicit commit each).
    // Between begin_bulk_write() and commit/rollback_bulk_write() the same
    // thread must not call other Dictionary methods that write.
    void begin_bulk_write();
    void commit_bulk_write();
    void rollback_bulk_write() noexcept;

    // Clears all user learning: user words, user frequencies AND the
    // next-word transition history (system entries are untouched).
    void clear_user_entries();
    [[nodiscard]] int schema_version() const;
    [[nodiscard]] std::vector<std::string> find_matches(const std::string& normalized,
                                                        std::size_t limit = 16) const;
    [[nodiscard]] std::vector<std::string> find_prefix_matches(const std::string& normalized_prefix,
                                                               std::size_t limit = 10) const;
    [[nodiscard]] std::vector<std::string> words_for_prefix(const std::string& prefix) const;
    void initialize_schema();

    // Personal dictionary management
    void add_user_word(const std::string& word, const std::string& normalized);
    void remove_user_word(const std::string& word);
    [[nodiscard]] std::vector<std::string> list_user_words() const;
    [[nodiscard]] int get_user_frequency(const std::string& word) const;

    // Word-list management (schema v4)
    void set_word_category(const std::string& word, const std::string& category);
    void add_word_relation(const std::string& word,
                           const std::string& related,
                           const std::string& kind);
    [[nodiscard]] std::vector<std::string> related_words(const std::string& word,
                                                         const std::string& kind = "",
                                                         std::size_t limit = 16) const;
    [[nodiscard]] std::vector<std::string> words_by_category(const std::string& category,
                                                             std::size_t limit = 100) const;
    [[nodiscard]] std::vector<DuplicateGroup> find_duplicate_normalized() const;
    [[nodiscard]] DictionaryStats stats() const;
    [[nodiscard]] std::vector<WordRecord> all_entries() const;
    [[nodiscard]] std::vector<std::pair<std::string, std::string>> all_relations() const;

    // Database maintenance
    void backup_to(const std::string& destination_path) const;
    void vacuum();

  private:
    struct DictionaryStorage;
    std::unique_ptr<DictionaryStorage> storage_;
};

}  // namespace suborno::dictionary
