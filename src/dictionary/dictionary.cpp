#include "suborno/dictionary/dictionary.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <filesystem>
#include <functional>
#include <limits>
#include <list>
#include <mutex>
#include <sqlite3.h>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

namespace suborno::dictionary {
namespace {

std::string escape_like_prefix(const std::string& prefix) {
    std::string escaped;
    escaped.reserve(prefix.size() + 1);
    for (const char character : prefix) {
        if (character == '\\' || character == '%' || character == '_') {
            escaped.push_back('\\');
        }
        escaped.push_back(character);
    }
    escaped.push_back('%');
    return escaped;
}

// Minimal UTF-8 walker: invokes fn for every decoded code point and returns

// false on malformed input or when fn rejects a code point.

bool for_each_utf8_codepoint(const std::string& text, const std::function<bool(char32_t)>& fn) {
    // Reviewed boundary cast: reinterprets char bytes as unsigned char purely
    // to inspect UTF-8 lead and continuation bits. The buffer is never
    // modified and never read beyond text.size().
    const auto* bytes = reinterpret_cast<const unsigned char*>(text.data());
    const std::size_t size = text.size();
    std::size_t index = 0;
    while (index < size) {
        const unsigned char lead = bytes[index];
        char32_t codepoint = 0;
        std::size_t continuation = 0;
        if (lead <= 0x7F) {
            codepoint = lead;
            continuation = 0;
        } else if ((lead & 0xE0) == 0xC0 && lead >= 0xC2) {
            codepoint = lead & 0x1F;
            continuation = 1;
        } else if ((lead & 0xF0) == 0xE0) {
            codepoint = lead & 0x0F;
            continuation = 2;
        } else if ((lead & 0xF8) == 0xF0 && lead <= 0xF4) {
            codepoint = lead & 0x07;
            continuation = 3;
        } else {
            return false;
        }
        if (size - index < continuation + 1) {
            return false;  // truncated multi-byte sequence
        }
        for (std::size_t offset = 1; offset <= continuation; ++offset) {
            const unsigned char next = bytes[index + offset];
            if ((next & 0xC0) != 0x80) {
                return false;
            }
            codepoint = (codepoint << 6) | (next & 0x3F);
        }
        index += continuation + 1;
        if (!fn(codepoint)) {
            return false;
        }
    }
    return true;
}

bool is_bengali_codepoint(char32_t codepoint) {
    return codepoint >= 0x0980 && codepoint <= 0x09FF;
}

bool is_combining_control_codepoint(char32_t codepoint) {
    return codepoint == 0x200C || codepoint == 0x200D;  // ZWNJ / ZWJ
}

bool is_bengali_digit(char32_t codepoint) {
    return codepoint >= 0x09E6 && codepoint <= 0x09EF;
}

}  // namespace

void execute_two_text_statement(sqlite3* database,
                                const char* sql,
                                const std::string& first,
                                const std::string& second,
                                const char* operation) {
    sqlite3_stmt* statement = nullptr;
    const int prepare_rc = sqlite3_prepare_v2(database, sql, -1, &statement, nullptr);
    if (prepare_rc != SQLITE_OK) {
        throw DictionaryError(
            DictionaryErrorCategory::prepare, prepare_rc, sqlite3_errmsg(database));
    }

    if (sqlite3_bind_text(statement, 1, first.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK
        || sqlite3_bind_text(statement, 2, second.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
        sqlite3_finalize(statement);
        throw DictionaryError(DictionaryErrorCategory::bind,
                              sqlite3_extended_errcode(database),
                              std::string("Unable to bind ") + operation);
    }

    const int step_rc = sqlite3_step(statement);
    sqlite3_finalize(statement);
    if (step_rc != SQLITE_DONE) {
        throw DictionaryError(DictionaryErrorCategory::step, step_rc, sqlite3_errmsg(database));
    }
}

void begin_dictionary_transaction(sqlite3* database) {
    char* error = nullptr;
    const int rc = sqlite3_exec(database, "BEGIN IMMEDIATE;", nullptr, nullptr, &error);
    if (rc != SQLITE_OK) {
        const std::string message = error == nullptr ? "unknown SQLite error" : error;
        sqlite3_free(error);
        throw DictionaryError(DictionaryErrorCategory::step, rc, message);
    }
}

void commit_dictionary_transaction(sqlite3* database) {
    char* error = nullptr;
    const int rc = sqlite3_exec(database, "COMMIT;", nullptr, nullptr, &error);
    if (rc != SQLITE_OK) {
        const std::string message = error == nullptr ? "unknown SQLite error" : error;
        sqlite3_free(error);
        throw DictionaryError(DictionaryErrorCategory::step, rc, message);
    }
}

bool Dictionary::is_valid_word(const std::string& word) {
    if (word.empty() || word.size() > 96) {
        return false;
    }
    bool has_bengali = false;
    const bool decoded = for_each_utf8_codepoint(word, [&](char32_t codepoint) {
        if (is_bengali_codepoint(codepoint)) {
            if (is_bengali_digit(codepoint)) {
                return false;  // Bangla numerals never occur inside a word
            }
            has_bengali = true;
            return true;
        }
        return is_combining_control_codepoint(codepoint);
    });
    return decoded && has_bengali;
}

bool Dictionary::is_valid_normalized(const std::string& normalized) {
    if (normalized.empty() || normalized.size() > 96) {
        return false;
    }
    return for_each_utf8_codepoint(normalized, [](char32_t codepoint) {
        return codepoint >= 0x20 && codepoint != 0x7F;  // printable, no DEL
    });
}

DictionaryError::DictionaryError(DictionaryErrorCategory category,
                                 int sqlite_code,
                                 const std::string& message)
    : std::runtime_error(message),
      category_(category),
      sqlite_code_(sqlite_code) {}

DictionaryErrorCategory DictionaryError::category() const noexcept {
    return category_;
}

int DictionaryError::sqlite_code() const noexcept {
    return sqlite_code_;
}

struct Dictionary::DictionaryStorage {
    // Hot read queries are prepared once and reused for the lifetime of the

    // connection (every IME keystroke hits find_matches / find_prefix_matches;

    // re-preparing per call was the dominant avoidable cost).

    enum class CachedId {
        FindMatches,
        FindPrefix,
        WordsForPrefix,
        UserFrequency,
        ListUserWords,
        RelatedWords,
        WordsByCategory,
        TransitionCandidates,
        Count
    };
    struct CachedQuery {
        const char* sql = nullptr;
        sqlite3_stmt* stmt = nullptr;
    };

    static constexpr std::size_t cache_capacity = 512;
    using CacheEntry = std::pair<std::string, std::vector<std::string>>;
    using CacheList = std::list<CacheEntry>;

    sqlite3* db = nullptr;
    // Single gate for every database access (reads included): sessions share

    // one connection, and serializing all access also keeps readers from

    // observing half of an in-flight write transaction.

    std::mutex write_mutex;
    CacheList cache_order_;
    std::unordered_map<std::string, CacheList::iterator> cache_index_;
    std::array<CachedQuery, static_cast<std::size_t>(CachedId::Count)> cached_queries_{};

    DictionaryStorage(const DictionaryStorage&) = delete;
    DictionaryStorage& operator=(const DictionaryStorage&) = delete;
    DictionaryStorage& operator=(DictionaryStorage&&) = delete;

    explicit DictionaryStorage(const std::string& path) {
        const std::filesystem::path database_path = path;
        std::error_code directory_error;
        if (!database_path.parent_path().empty()) {
            std::filesystem::create_directories(database_path.parent_path(), directory_error);
            if (directory_error) {
                throw DictionaryError(DictionaryErrorCategory::open,
                                      SQLITE_CANTOPEN,
                                      "Unable to create dictionary directory");
            }
        }

        if (sqlite3_open(database_path.string().c_str(), &db) != SQLITE_OK) {
            const int code = db == nullptr ? SQLITE_CANTOPEN : sqlite3_extended_errcode(db);
            const char* text = db == nullptr ? nullptr : sqlite3_errmsg(db);
            if (db != nullptr) {
                sqlite3_close(db);
                db = nullptr;
            }
            throw DictionaryError(
                DictionaryErrorCategory::open,
                code,
                text == nullptr ? "Unable to open dictionary database" : std::string(text));
        }

        sqlite3_busy_timeout(db, 5000);
        // Performance profile for an IME workload: WAL keeps readers free of

        // writer blocking and makes bulk imports commit fast; NORMAL is safe

        // with WAL (a crash may lose the last transaction, never corrupt);

        // a bigger page cache + mmap serve million-scale lookups.

        sqlite3_exec(db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
        sqlite3_exec(db, "PRAGMA synchronous=NORMAL;", nullptr, nullptr, nullptr);
        sqlite3_exec(db, "PRAGMA cache_size=-8000;", nullptr, nullptr, nullptr);
        sqlite3_exec(db, "PRAGMA temp_store=MEMORY;", nullptr, nullptr, nullptr);
        sqlite3_exec(db, "PRAGMA mmap_size=134217728;", nullptr, nullptr, nullptr);
        cached_queries_[static_cast<std::size_t>(CachedId::FindMatches)].sql =
            "WITH matches AS ("
            "SELECT word, frequency + user_frequency AS score "
            "FROM dictionary_entries WHERE normalized = ? "
            "UNION ALL "
            "SELECT entries.word, entries.frequency + entries.user_frequency "
            "+ aliases.frequency AS score "
            "FROM dictionary_user_aliases AS aliases "
            "JOIN dictionary_entries AS entries ON entries.word = aliases.word "
            "WHERE aliases.normalized = ?) "
            "SELECT word FROM matches GROUP BY word "
            "ORDER BY MAX(score) DESC, word ASC LIMIT ?";
        cached_queries_[static_cast<std::size_t>(CachedId::FindPrefix)].sql =
            "WITH matches AS ("
            "SELECT word, frequency + user_frequency AS score "
            "FROM dictionary_entries WHERE normalized LIKE ? ESCAPE '\\' "
            "UNION ALL "
            "SELECT entries.word, entries.frequency + entries.user_frequency "
            "+ aliases.frequency AS score "
            "FROM dictionary_user_aliases AS aliases "
            "JOIN dictionary_entries AS entries ON entries.word = aliases.word "
            "WHERE aliases.normalized LIKE ? ESCAPE '\\') "
            "SELECT word FROM matches GROUP BY word "
            "ORDER BY MAX(score) DESC, word ASC LIMIT ?";
        cached_queries_[static_cast<std::size_t>(CachedId::WordsForPrefix)].sql =
            "SELECT word FROM dictionary_entries WHERE word LIKE ? ESCAPE '\\' "
            "ORDER BY (frequency + COALESCE(user_frequency, 0)) DESC, word ASC LIMIT 10";
        cached_queries_[static_cast<std::size_t>(CachedId::UserFrequency)].sql =
            "SELECT user_frequency FROM dictionary_entries WHERE word = ?";
        cached_queries_[static_cast<std::size_t>(CachedId::ListUserWords)].sql =
            "SELECT word FROM dictionary_entries "
            "WHERE source = 'user' OR user_pinned = 1 OR user_frequency > 0 "
            "ORDER BY user_frequency DESC, word ASC";
        cached_queries_[static_cast<std::size_t>(CachedId::RelatedWords)].sql =
            "SELECT related FROM word_relations "
            "WHERE word = ? AND (? = '' OR kind = ?) ORDER BY rowid LIMIT ?";
        cached_queries_[static_cast<std::size_t>(CachedId::WordsByCategory)].sql =
            "SELECT word FROM dictionary_entries WHERE category = ? "
            "ORDER BY (frequency + user_frequency) DESC, word ASC LIMIT ?";
        cached_queries_[static_cast<std::size_t>(CachedId::TransitionCandidates)].sql =
            "SELECT next_normalized FROM word_transitions "
            "WHERE previous_normalized = ? AND (? = '' OR next_normalized LIKE ? ESCAPE '\\') "
            "ORDER BY frequency DESC, next_normalized ASC LIMIT ?";
    }

    ~DictionaryStorage() {
        for (CachedQuery& query : cached_queries_) {
            if (query.stmt != nullptr) {
                sqlite3_finalize(query.stmt);
                query.stmt = nullptr;
            }
        }
        if (db != nullptr) {
            sqlite3_close(db);
            db = nullptr;
        }
    }

    // Returns a reset, binding-free cached statement (still guarded by the

    // caller holding write_mutex).

    sqlite3_stmt* cached_stmt(CachedId id) {
        CachedQuery& query = cached_queries_[static_cast<std::size_t>(id)];
        if (query.stmt == nullptr) {
            const int prepare_rc = sqlite3_prepare_v2(db, query.sql, -1, &query.stmt, nullptr);
            if (prepare_rc != SQLITE_OK) {
                query.stmt = nullptr;
                throw DictionaryError(
                    DictionaryErrorCategory::prepare, prepare_rc, sqlite3_errmsg(db));
            }
        }
        sqlite3_reset(query.stmt);
        sqlite3_clear_bindings(query.stmt);
        return query.stmt;
    }

    // Re-arm a cached statement after an early exit so later reuse is clean.

    void reset_quietly(CachedId id) {
        CachedQuery& query = cached_queries_[static_cast<std::size_t>(id)];
        if (query.stmt != nullptr) {
            sqlite3_reset(query.stmt);
            sqlite3_clear_bindings(query.stmt);
        }
    }

    const std::vector<std::string>* cache_get(const std::string& key) {
        const auto found = cache_index_.find(key);
        if (found == cache_index_.end()) {
            return nullptr;
        }
        cache_order_.splice(cache_order_.begin(), cache_order_, found->second);
        return &found->second->second;
    }

    void cache_put(const std::string& key, std::vector<std::string> value) {
        auto entry = cache_order_.emplace(cache_order_.begin(), key, std::move(value));
        cache_index_[key] = entry;
        if (cache_order_.size() > cache_capacity) {
            const auto& oldest = cache_order_.back();
            cache_index_.erase(oldest.first);
            cache_order_.pop_back();
        }
    }

    void cache_clear() {
        cache_index_.clear();
        cache_order_.clear();
    }

    void log_event(const char* action, const std::string& word, const char* detail) {
        // Changelog is best-effort bookkeeping; never fail the real

        // operation because bookkeeping could not be recorded.

        sqlite3_stmt* statement = nullptr;
        const int prepare_rc = sqlite3_prepare_v2(
            db,
            "INSERT INTO dictionary_changelog(action, word, detail) VALUES(?, ?, ?);",
            -1,
            &statement,
            nullptr);
        if (prepare_rc != SQLITE_OK) {
            return;
        }
        if (sqlite3_bind_text(statement, 1, action, -1, SQLITE_STATIC) != SQLITE_OK
            || sqlite3_bind_text(statement, 2, word.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK
            || sqlite3_bind_text(statement, 3, detail == nullptr ? "" : detail, -1, SQLITE_STATIC)
                   != SQLITE_OK) {
            sqlite3_finalize(statement);
            return;
        }
        sqlite3_step(statement);
        sqlite3_finalize(statement);
    }
};

Dictionary::Dictionary(const std::string& database_path)
    : storage_(std::make_unique<DictionaryStorage>(database_path)) {
    initialize_schema();
}

Dictionary::~Dictionary() = default;

void Dictionary::initialize_schema() {
    const char* sql =
        "CREATE TABLE IF NOT EXISTS dictionary_metadata ("
        "  key TEXT PRIMARY KEY,"
        "  value TEXT NOT NULL);"
        "INSERT OR IGNORE INTO dictionary_metadata(key, value) VALUES('schema_version', '4');"
        "CREATE TABLE IF NOT EXISTS dictionary_entries ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  word TEXT NOT NULL UNIQUE,"
        "  normalized TEXT NOT NULL,"
        "  frequency INTEGER NOT NULL DEFAULT 1,"
        "  source TEXT NOT NULL DEFAULT 'system',"
        "  user_frequency INTEGER NOT NULL DEFAULT 0,"
        "  user_pinned INTEGER NOT NULL DEFAULT 0,"
        "  created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP);"
        "CREATE INDEX IF NOT EXISTS idx_dictionary_normalized ON dictionary_entries(normalized);"
        "CREATE INDEX IF NOT EXISTS idx_dictionary_normalized_prefix "
        "ON dictionary_entries(normalized COLLATE NOCASE);"
        "CREATE INDEX IF NOT EXISTS idx_dictionary_word_prefix "
        "ON dictionary_entries(word COLLATE NOCASE);"
        "CREATE TABLE IF NOT EXISTS dictionary_user_aliases ("
        "  word TEXT NOT NULL,"
        "  normalized TEXT NOT NULL,"
        "  frequency INTEGER NOT NULL DEFAULT 1,"
        "  PRIMARY KEY(word, normalized));"
        "CREATE INDEX IF NOT EXISTS idx_dictionary_user_alias_normalized "
        "ON dictionary_user_aliases(normalized COLLATE NOCASE);"
        "CREATE TABLE IF NOT EXISTS word_relations ("
        "  word TEXT NOT NULL,"
        "  related TEXT NOT NULL,"
        "  kind TEXT NOT NULL,"
        "  created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "  PRIMARY KEY(word, related, kind));"
        "CREATE INDEX IF NOT EXISTS idx_word_relations_related ON word_relations(related);"
        // Next-word transition history (bigram model for prediction). Added

        // additively with IF NOT EXISTS so existing schema-v4 databases gain

        // the table on first open without a schema_version bump — the v4

        // integrity check only rejects FUTURE versions, never unknown tables.

        "CREATE TABLE IF NOT EXISTS word_transitions ("
        "  previous_normalized TEXT NOT NULL,"
        "  next_normalized TEXT NOT NULL,"
        "  frequency INTEGER NOT NULL DEFAULT 0,"
        "  PRIMARY KEY (previous_normalized, next_normalized));"
        // The PK index only serves exact (previous, next) upserts; this index

        // serves the prediction lookup (previous = ? ORDER BY frequency DESC).

        "CREATE INDEX IF NOT EXISTS idx_word_transitions_previous "
        "ON word_transitions(previous_normalized, frequency);"
        "CREATE TABLE IF NOT EXISTS dictionary_changelog ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "  action TEXT NOT NULL,"
        "  word TEXT NOT NULL DEFAULT '',"
        "  detail TEXT NOT NULL DEFAULT '');";

    char* error = nullptr;
    const int rc = sqlite3_exec(storage_->db, sql, nullptr, nullptr, &error);
    if (rc != SQLITE_OK) {
        std::string message = error == nullptr ? "unknown SQLite error" : error;
        sqlite3_free(error);
        throw DictionaryError(DictionaryErrorCategory::schema,
                              rc,
                              "Unable to initialize dictionary schema: " + message);
    }

    sqlite3_stmt* column_stmt = nullptr;
    const int prepare_rc = sqlite3_prepare_v2(
        storage_->db, "PRAGMA table_info(dictionary_entries)", -1, &column_stmt, nullptr);
    if (prepare_rc != SQLITE_OK) {
        throw DictionaryError(
            DictionaryErrorCategory::schema, prepare_rc, "Unable to inspect dictionary schema");
    }

    bool has_source = false;
    bool has_user_frequency = false;
    bool has_user_pinned = false;
    bool has_category = false;
    bool has_register = false;
    bool has_origin = false;
    int step_rc = SQLITE_OK;
    while ((step_rc = sqlite3_step(column_stmt)) == SQLITE_ROW) {
        const unsigned char* name = sqlite3_column_text(column_stmt, 1);
        if (name == nullptr) {
            continue;
        }
        const std::string column(reinterpret_cast<const char*>(name));
        if (column == "source") {
            has_source = true;
        }
        if (column == "user_frequency") {
            has_user_frequency = true;
        }
        if (column == "user_pinned") {
            has_user_pinned = true;
        }
        if (column == "category") {
            has_category = true;
        }
        if (column == "register") {
            has_register = true;
        }
        if (column == "origin") {
            has_origin = true;
        }
    }
    sqlite3_finalize(column_stmt);
    if (step_rc != SQLITE_DONE) {
        throw DictionaryError(
            DictionaryErrorCategory::step, step_rc, "Unable to inspect dictionary columns");
    }

    const auto add_column = [this](const char* definition, const char* what) {
        char* alter_error = nullptr;
        const int alter_rc = sqlite3_exec(storage_->db, definition, nullptr, nullptr, &alter_error);
        if (alter_rc != SQLITE_OK) {
            std::string message = alter_error == nullptr ? "unknown SQLite error" : alter_error;
            sqlite3_free(alter_error);
            throw DictionaryError(
                DictionaryErrorCategory::schema,
                alter_rc,
                std::string("Unable to migrate dictionary schema (") + what + "): " + message);
        }
    };

    if (!has_source) {
        add_column(
            "ALTER TABLE dictionary_entries "
            "ADD COLUMN source TEXT NOT NULL DEFAULT 'system';",
            "source");
    }
    if (!has_user_frequency) {
        add_column(
            "ALTER TABLE dictionary_entries "
            "ADD COLUMN user_frequency INTEGER NOT NULL DEFAULT 0;",
            "user_frequency");
    }
    if (!has_user_pinned) {
        add_column(
            "ALTER TABLE dictionary_entries "
            "ADD COLUMN user_pinned INTEGER NOT NULL DEFAULT 0;",
            "user_pinned");
    }
    if (!has_category) {
        add_column(
            "ALTER TABLE dictionary_entries "
            "ADD COLUMN category TEXT NOT NULL DEFAULT '';",
            "category");
    }
    if (!has_register) {
        add_column(
            "ALTER TABLE dictionary_entries "
            "ADD COLUMN register TEXT NOT NULL DEFAULT '';",
            "register");
    }
    if (!has_origin) {
        add_column(
            "ALTER TABLE dictionary_entries "
            "ADD COLUMN origin TEXT NOT NULL DEFAULT 'seed';",
            "origin");
    }

    // Category/origin indexes can only exist after the columns are present

    // (fresh databases create the table without them, migrated databases get

    // them via ALTER TABLE above).

    char* late_index_error = nullptr;
    const int late_index_rc = sqlite3_exec(
        storage_->db,
        "CREATE INDEX IF NOT EXISTS idx_entries_category ON dictionary_entries(category);"
        "CREATE INDEX IF NOT EXISTS idx_entries_origin ON dictionary_entries(origin);",
        nullptr,
        nullptr,
        &late_index_error);
    if (late_index_rc != SQLITE_OK) {
        const std::string message =
            late_index_error == nullptr ? "unknown SQLite error" : late_index_error;
        sqlite3_free(late_index_error);
        throw DictionaryError(DictionaryErrorCategory::schema,
                              late_index_rc,
                              "Unable to create dictionary metadata indexes: " + message);
    }

    char* version_error = nullptr;
    const int version_update_rc = sqlite3_exec(storage_->db,
                                               "UPDATE dictionary_metadata SET value='2' "
                                               "WHERE key='schema_version' AND value='1';",
                                               nullptr,
                                               nullptr,
                                               &version_error);
    if (version_update_rc != SQLITE_OK) {
        std::string message = version_error == nullptr ? "unknown SQLite error" : version_error;
        sqlite3_free(version_error);
        throw DictionaryError(DictionaryErrorCategory::schema,
                              version_update_rc,
                              "Unable to update dictionary schema version: " + message);
    }
    char* alias_migration_error = nullptr;
    const int alias_migration_rc = sqlite3_exec(
        storage_->db,
        "INSERT OR IGNORE INTO dictionary_user_aliases(word, normalized, frequency) "
        "SELECT word, normalized, CASE WHEN user_frequency > 0 THEN user_frequency ELSE 1 END "
        "FROM dictionary_entries "
        "WHERE user_frequency > 0 OR source='user' OR user_pinned=1;",
        nullptr,
        nullptr,
        &alias_migration_error);
    if (alias_migration_rc != SQLITE_OK) {
        const std::string message =
            alias_migration_error == nullptr ? "unknown SQLite error" : alias_migration_error;
        sqlite3_free(alias_migration_error);
        throw DictionaryError(DictionaryErrorCategory::schema,
                              alias_migration_rc,
                              "Unable to migrate learned dictionary aliases: " + message);
    }

    char* version_three_error = nullptr;
    const int version_three_rc = sqlite3_exec(storage_->db,
                                              "UPDATE dictionary_metadata SET value='3' "
                                              "WHERE key='schema_version' AND value='2';",
                                              nullptr,
                                              nullptr,
                                              &version_three_error);
    if (version_three_rc != SQLITE_OK) {
        const std::string message =
            version_three_error == nullptr ? "unknown SQLite error" : version_three_error;
        sqlite3_free(version_three_error);
        throw DictionaryError(DictionaryErrorCategory::schema,
                              version_three_rc,
                              "Unable to update dictionary schema version: " + message);
    }
    char* version_four_error = nullptr;
    const int version_four_rc = sqlite3_exec(storage_->db,
                                             "UPDATE dictionary_metadata SET value='4' "
                                             "WHERE key='schema_version' AND value='3';",
                                             nullptr,
                                             nullptr,
                                             &version_four_error);
    if (version_four_rc != SQLITE_OK) {
        const std::string message =
            version_four_error == nullptr ? "unknown SQLite error" : version_four_error;
        sqlite3_free(version_four_error);
        throw DictionaryError(DictionaryErrorCategory::schema,
                              version_four_rc,
                              "Unable to update dictionary schema version: " + message);
    }

    int stored_version = 0;
    bool version_parsable = false;
    try {
        stored_version = schema_version();
        version_parsable = true;
    } catch (const DictionaryError&) {
        // Deliberately deferred: an unparsable version string lets construction

        // succeed and schema_version() surfaces the error on demand, matching

        // the DictionaryRejectsInvalidSchemaVersion contract.

        version_parsable = false;
    }
    if (version_parsable && stored_version > 4) {
        throw DictionaryError(DictionaryErrorCategory::schema,
                              SQLITE_SCHEMA,
                              "Dictionary database schema is newer than this engine");
    }
}

void Dictionary::add_entry(const std::string& word,
                           const std::string& normalized,
                           int frequency,
                           const std::string& category,
                           const std::string& word_register,
                           const std::string& origin) {
    if (!is_valid_word(word)) {
        throw DictionaryError(DictionaryErrorCategory::validation,
                              SQLITE_MISMATCH,
                              "Invalid dictionary word: " + word);
    }
    if (!is_valid_normalized(normalized)) {
        throw DictionaryError(DictionaryErrorCategory::validation,
                              SQLITE_MISMATCH,
                              "Invalid normalized key for word: " + word);
    }
    // Clamp user-supplied frequencies: bind_int stores int32 and an

    // out-of-range value would wrap negative and permanently sink the word.

    frequency = std::clamp(frequency, 0, std::numeric_limits<int>::max());
    std::lock_guard<std::mutex> lock(storage_->write_mutex);
    const std::string statement =
        "INSERT INTO dictionary_entries(word, normalized, frequency, source,"
        " category, register, origin) VALUES(?, ?, ?, 'system', ?, ?, ?) "
        "ON CONFLICT(word) DO UPDATE SET normalized=excluded.normalized,"
        " frequency=excluded.frequency, source='system', category=excluded.category,"
        " register=excluded.register, origin=excluded.origin";
    sqlite3_stmt* stmt = nullptr;
    const int prepare_rc = sqlite3_prepare_v2(storage_->db, statement.c_str(), -1, &stmt, nullptr);
    if (prepare_rc != SQLITE_OK) {
        throw DictionaryError(
            DictionaryErrorCategory::prepare, prepare_rc, sqlite3_errmsg(storage_->db));
    }

    if (sqlite3_bind_text(stmt, 1, word.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK
        || sqlite3_bind_text(stmt, 2, normalized.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK
        || sqlite3_bind_int(stmt, 3, frequency) != SQLITE_OK
        || sqlite3_bind_text(stmt, 4, category.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK
        || sqlite3_bind_text(stmt, 5, word_register.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK
        || sqlite3_bind_text(stmt, 6, origin.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        throw DictionaryError(DictionaryErrorCategory::bind,
                              sqlite3_extended_errcode(storage_->db),
                              "Unable to bind dictionary insert");
    }

    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        throw DictionaryError(DictionaryErrorCategory::step, rc, sqlite3_errmsg(storage_->db));
    }
    storage_->cache_clear();
}

void Dictionary::record_commit(const std::string& word, const std::string& normalized) {
    record_commit(word, normalized, normalized);
}

void Dictionary::record_commit(const std::string& word,
                               const std::string& canonical_normalized,
                               const std::string& user_alias) {
    std::lock_guard<std::mutex> lock(storage_->write_mutex);
    begin_dictionary_transaction(storage_->db);
    try {
        execute_two_text_statement(
            storage_->db,
            "INSERT INTO dictionary_entries("
            "word, normalized, frequency, source, user_frequency) "
            "VALUES(?, ?, 1, 'user', 1) "
            "ON CONFLICT(word) DO UPDATE SET "
            "user_frequency=MIN(dictionary_entries.user_frequency+1, 2147483647);",
            word,
            canonical_normalized,
            "committed dictionary entry");
        execute_two_text_statement(
            storage_->db,
            "INSERT INTO dictionary_user_aliases(word, normalized, frequency) "
            "VALUES(?, ?, 1) "
            "ON CONFLICT(word, normalized) DO UPDATE SET "
            "frequency=MIN(dictionary_user_aliases.frequency+1, 2147483647);",
            word,
            user_alias,
            "committed dictionary alias");
        commit_dictionary_transaction(storage_->db);
    } catch (...) {
        sqlite3_exec(storage_->db, "ROLLBACK;", nullptr, nullptr, nullptr);
        throw;
    }
    storage_->cache_clear();
    storage_->log_event("commit", word, "user_frequency incremented");
}

void Dictionary::record_transition(const std::string& previous_normalized,
                                   const std::string& next_normalized) {
    // Callers pass already-normalized NFC words (session/engine layers

    // normalize first); the only gate here is non-empty, because an empty

    // side would poison the bigram key and every prediction from it.

    if (previous_normalized.empty() || next_normalized.empty()) {
        return;
    }
    std::lock_guard<std::mutex> lock(storage_->write_mutex);
    execute_two_text_statement(
        storage_->db,
        "INSERT INTO word_transitions(previous_normalized, next_normalized, frequency) "
        "VALUES(?, ?, 1) "
        "ON CONFLICT(previous_normalized, next_normalized) DO UPDATE SET "
        "frequency=MIN(word_transitions.frequency+1, 2147483647);",
        previous_normalized,
        next_normalized,
        "recorded word transition");
    // No changelog entry: transitions update on every consecutive commit and

    // would flood the lifecycle log. Cached query results (including

    // transition_candidates) must drop though — they may embed this row.

    storage_->cache_clear();
}

std::vector<std::string> Dictionary::transition_candidates(const std::string& previous_normalized,
                                                           const std::string& prefix,
                                                           std::size_t limit) const {
    if (previous_normalized.empty() || limit == 0) {
        return {};
    }
    const std::string cache_key =
        "t\1" + previous_normalized + "\2" + prefix + "#" + std::to_string(limit);
    std::lock_guard<std::mutex> lock(storage_->write_mutex);
    if (const auto* cached = storage_->cache_get(cache_key)) {
        return *cached;
    }

    using Id = DictionaryStorage::CachedId;
    sqlite3_stmt* stmt = storage_->cached_stmt(Id::TransitionCandidates);
    const std::string pattern = escape_like_prefix(prefix);
    const std::size_t bounded_limit =
        std::min(limit, static_cast<std::size_t>(std::numeric_limits<sqlite3_int64>::max()));
    std::vector<std::string> results;
    try {
        if (sqlite3_bind_text(stmt, 1, previous_normalized.c_str(), -1, SQLITE_TRANSIENT)
                != SQLITE_OK
            || sqlite3_bind_text(stmt, 2, prefix.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK
            || sqlite3_bind_text(stmt, 3, pattern.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK
            || sqlite3_bind_int64(stmt, 4, static_cast<sqlite3_int64>(bounded_limit))
                   != SQLITE_OK) {
            throw DictionaryError(DictionaryErrorCategory::bind,
                                  sqlite3_extended_errcode(storage_->db),
                                  "Unable to bind transition candidates query");
        }
        int step_rc = SQLITE_OK;
        while ((step_rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            const unsigned char* value = sqlite3_column_text(stmt, 0);
            if (value != nullptr) {
                results.emplace_back(reinterpret_cast<const char*>(value));
            }
        }
        if (step_rc != SQLITE_DONE) {
            throw DictionaryError(
                DictionaryErrorCategory::step, step_rc, sqlite3_errmsg(storage_->db));
        }
    } catch (...) {
        storage_->reset_quietly(Id::TransitionCandidates);
        throw;
    }
    storage_->reset_quietly(Id::TransitionCandidates);
    storage_->cache_put(cache_key, results);
    return results;
}

void Dictionary::clear_user_entries() {
    std::lock_guard<std::mutex> lock(storage_->write_mutex);
    begin_dictionary_transaction(storage_->db);
    char* error = nullptr;
    const int rc = sqlite3_exec(storage_->db,
                                "DELETE FROM dictionary_user_aliases;"
                                "DELETE FROM dictionary_entries WHERE source='user';"
                                "DELETE FROM word_transitions;"
                                "UPDATE dictionary_entries "
                                "SET user_pinned=0, user_frequency=0;",
                                nullptr,
                                nullptr,
                                &error);
    if (rc != SQLITE_OK) {
        const std::string message = error == nullptr ? "unknown SQLite error" : error;
        sqlite3_free(error);
        sqlite3_exec(storage_->db, "ROLLBACK;", nullptr, nullptr, nullptr);
        throw DictionaryError(DictionaryErrorCategory::step, rc, message);
    }
    try {
        commit_dictionary_transaction(storage_->db);
    } catch (...) {
        sqlite3_exec(storage_->db, "ROLLBACK;", nullptr, nullptr, nullptr);
        throw;
    }
    storage_->cache_clear();
    storage_->log_event("clear-user", "", "all user entries removed");
}

int Dictionary::schema_version() const {
    sqlite3_stmt* stmt = nullptr;
    const int prepare_rc =
        sqlite3_prepare_v2(storage_->db,
                           "SELECT value FROM dictionary_metadata WHERE key='schema_version'",
                           -1,
                           &stmt,
                           nullptr);
    if (prepare_rc != SQLITE_OK) {
        throw DictionaryError(
            DictionaryErrorCategory::prepare, prepare_rc, sqlite3_errmsg(storage_->db));
    }
    int version = 0;
    const int step_rc = sqlite3_step(stmt);
    if (step_rc == SQLITE_ROW) {
        const unsigned char* value = sqlite3_column_text(stmt, 0);
        if (value != nullptr) {
            const char* text = reinterpret_cast<const char*>(value);
            const char* text_end = text + sqlite3_column_bytes(stmt, 0);
            const auto [parsed_end, parse_error] = std::from_chars(text, text_end, version);
            if (parse_error != std::errc{} || parsed_end != text_end || version < 0) {
                sqlite3_finalize(stmt);
                throw DictionaryError(DictionaryErrorCategory::schema,
                                      SQLITE_MISMATCH,
                                      "Invalid dictionary schema version");
            }
        }
    } else if (step_rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw DictionaryError(DictionaryErrorCategory::step, step_rc, sqlite3_errmsg(storage_->db));
    }
    sqlite3_finalize(stmt);
    return version;
}

void Dictionary::begin_bulk_write() {
    std::lock_guard<std::mutex> lock(storage_->write_mutex);
    begin_dictionary_transaction(storage_->db);
}

void Dictionary::commit_bulk_write() {
    std::lock_guard<std::mutex> lock(storage_->write_mutex);
    commit_dictionary_transaction(storage_->db);
    storage_->cache_clear();
}

void Dictionary::rollback_bulk_write() noexcept {
    std::lock_guard<std::mutex> lock(storage_->write_mutex);
    sqlite3_exec(storage_->db, "ROLLBACK;", nullptr, nullptr, nullptr);
    storage_->cache_clear();
}

// Incremental-update upsert: keeps the higher frequency, fills empty metadata.

void Dictionary::add_entry_merge(const std::string& word,
                                 const std::string& normalized,
                                 int frequency,
                                 const std::string& category,
                                 const std::string& word_register,
                                 const std::string& origin) {
    if (!is_valid_word(word)) {
        throw DictionaryError(DictionaryErrorCategory::validation,
                              SQLITE_MISMATCH,
                              "Invalid dictionary word: " + word);
    }
    if (!is_valid_normalized(normalized)) {
        throw DictionaryError(DictionaryErrorCategory::validation,
                              SQLITE_MISMATCH,
                              "Invalid normalized key for word: " + word);
    }
    frequency = std::clamp(frequency, 0, std::numeric_limits<int>::max());
    std::lock_guard<std::mutex> lock(storage_->write_mutex);
    // Incremental-update semantics: never lose ranking information — keep the

    // higher of the existing/imported frequency and only fill empty metadata.

    const std::string statement =
        "INSERT INTO dictionary_entries(word, normalized, frequency, source,"
        " category, register, origin) VALUES(?, ?, ?, 'system', ?, ?, ?) "
        "ON CONFLICT(word) DO UPDATE SET normalized=excluded.normalized,"
        " frequency=MAX(dictionary_entries.frequency, excluded.frequency),"
        " source='system',"
        " category=CASE WHEN dictionary_entries.category='' THEN excluded.category"
        " ELSE dictionary_entries.category END,"
        " register=CASE WHEN dictionary_entries.register='' THEN excluded.register"
        " ELSE dictionary_entries.register END,"
        " origin=CASE WHEN dictionary_entries.origin='' THEN excluded.origin"
        " ELSE dictionary_entries.origin END";
    sqlite3_stmt* stmt = nullptr;
    const int prepare_rc = sqlite3_prepare_v2(storage_->db, statement.c_str(), -1, &stmt, nullptr);
    if (prepare_rc != SQLITE_OK) {
        throw DictionaryError(
            DictionaryErrorCategory::prepare, prepare_rc, sqlite3_errmsg(storage_->db));
    }

    if (sqlite3_bind_text(stmt, 1, word.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK
        || sqlite3_bind_text(stmt, 2, normalized.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK
        || sqlite3_bind_int(stmt, 3, frequency) != SQLITE_OK
        || sqlite3_bind_text(stmt, 4, category.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK
        || sqlite3_bind_text(stmt, 5, word_register.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK
        || sqlite3_bind_text(stmt, 6, origin.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        throw DictionaryError(DictionaryErrorCategory::bind,
                              sqlite3_extended_errcode(storage_->db),
                              "Unable to bind dictionary merge insert");
    }

    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        throw DictionaryError(DictionaryErrorCategory::step, rc, sqlite3_errmsg(storage_->db));
    }
    storage_->cache_clear();
}

std::vector<std::string> Dictionary::find_matches(const std::string& normalized,
                                                  std::size_t limit) const {
    if (limit == 0 || !is_valid_normalized(normalized)) {
        return {};
    }
    const std::string cache_key = "m\1" + normalized + "#" + std::to_string(limit);
    std::lock_guard<std::mutex> lock(storage_->write_mutex);
    if (const auto* cached = storage_->cache_get(cache_key)) {
        return *cached;
    }

    using Id = DictionaryStorage::CachedId;
    sqlite3_stmt* stmt = storage_->cached_stmt(Id::FindMatches);
    const std::size_t bounded_limit =
        std::min(limit, static_cast<std::size_t>(std::numeric_limits<sqlite3_int64>::max()));
    std::vector<std::string> results;
    try {
        if (sqlite3_bind_text(stmt, 1, normalized.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK
            || sqlite3_bind_text(stmt, 2, normalized.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK
            || sqlite3_bind_int64(stmt, 3, static_cast<sqlite3_int64>(bounded_limit))
                   != SQLITE_OK) {
            throw DictionaryError(DictionaryErrorCategory::bind,
                                  sqlite3_extended_errcode(storage_->db),
                                  "Unable to bind dictionary lookup");
        }
        int step_rc = SQLITE_OK;
        while ((step_rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            const unsigned char* value = sqlite3_column_text(stmt, 0);
            if (value != nullptr) {
                results.emplace_back(reinterpret_cast<const char*>(value));
            }
        }
        if (step_rc != SQLITE_DONE) {
            throw DictionaryError(
                DictionaryErrorCategory::step, step_rc, sqlite3_errmsg(storage_->db));
        }
    } catch (...) {
        storage_->reset_quietly(Id::FindMatches);
        throw;
    }
    storage_->reset_quietly(Id::FindMatches);
    storage_->cache_put(cache_key, results);
    return results;
}

std::vector<std::string> Dictionary::find_prefix_matches(const std::string& normalized_prefix,
                                                         std::size_t limit) const {
    if (normalized_prefix.empty() || limit == 0 || !is_valid_normalized(normalized_prefix)) {
        return {};
    }
    const std::string cache_key = "p\1" + normalized_prefix + "#" + std::to_string(limit);
    std::lock_guard<std::mutex> lock(storage_->write_mutex);
    if (const auto* cached = storage_->cache_get(cache_key)) {
        return *cached;
    }

    using Id = DictionaryStorage::CachedId;
    sqlite3_stmt* stmt = storage_->cached_stmt(Id::FindPrefix);
    const std::string pattern = escape_like_prefix(normalized_prefix);
    const std::size_t bounded_limit =
        std::min(limit, static_cast<std::size_t>(std::numeric_limits<sqlite3_int64>::max()));
    std::vector<std::string> results;
    try {
        if (sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK
            || sqlite3_bind_text(stmt, 2, pattern.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK
            || sqlite3_bind_int64(stmt, 3, static_cast<sqlite3_int64>(bounded_limit))
                   != SQLITE_OK) {
            throw DictionaryError(DictionaryErrorCategory::bind,
                                  sqlite3_extended_errcode(storage_->db),
                                  "Unable to bind dictionary normalized prefix lookup");
        }
        int step_rc = SQLITE_OK;
        while ((step_rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            const unsigned char* value = sqlite3_column_text(stmt, 0);
            if (value != nullptr) {
                results.emplace_back(reinterpret_cast<const char*>(value));
            }
        }
        if (step_rc != SQLITE_DONE) {
            throw DictionaryError(
                DictionaryErrorCategory::step, step_rc, sqlite3_errmsg(storage_->db));
        }
    } catch (...) {
        storage_->reset_quietly(Id::FindPrefix);
        throw;
    }
    storage_->reset_quietly(Id::FindPrefix);
    storage_->cache_put(cache_key, results);
    return results;
}

std::vector<std::string> Dictionary::words_for_prefix(const std::string& prefix) const {
    if (prefix.empty() || !is_valid_normalized(prefix)) {
        return {};
    }
    const std::string cache_key = "w\1" + prefix;
    std::lock_guard<std::mutex> lock(storage_->write_mutex);
    if (const auto* cached = storage_->cache_get(cache_key)) {
        return *cached;
    }

    using Id = DictionaryStorage::CachedId;
    sqlite3_stmt* stmt = storage_->cached_stmt(Id::WordsForPrefix);
    const std::string pattern = escape_like_prefix(prefix);
    std::vector<std::string> results;
    try {
        if (sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
            throw DictionaryError(DictionaryErrorCategory::bind,
                                  sqlite3_extended_errcode(storage_->db),
                                  "Unable to bind dictionary prefix lookup");
        }
        int step_rc = SQLITE_OK;
        while ((step_rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            const unsigned char* value = sqlite3_column_text(stmt, 0);
            if (value != nullptr) {
                results.emplace_back(reinterpret_cast<const char*>(value));
            }
        }
        if (step_rc != SQLITE_DONE) {
            throw DictionaryError(
                DictionaryErrorCategory::step, step_rc, sqlite3_errmsg(storage_->db));
        }
    } catch (...) {
        storage_->reset_quietly(Id::WordsForPrefix);
        throw;
    }
    storage_->reset_quietly(Id::WordsForPrefix);
    storage_->cache_put(cache_key, results);
    return results;
}

void Dictionary::add_user_word(const std::string& word, const std::string& normalized) {
    std::lock_guard<std::mutex> lock(storage_->write_mutex);
    begin_dictionary_transaction(storage_->db);
    try {
        execute_two_text_statement(
            storage_->db,
            "INSERT INTO dictionary_entries("
            "word, normalized, frequency, source, user_frequency, user_pinned) "
            "VALUES(?, ?, 1, 'user', 1, 1) "
            "ON CONFLICT(word) DO UPDATE SET "
            "user_frequency=MIN(dictionary_entries.user_frequency+1, 2147483647),"
            "user_pinned=1;",
            word,
            normalized,
            "user dictionary entry");
        execute_two_text_statement(
            storage_->db,
            "INSERT INTO dictionary_user_aliases(word, normalized, frequency) "
            "VALUES(?, ?, 1) "
            "ON CONFLICT(word, normalized) DO UPDATE SET "
            "frequency=MIN(dictionary_user_aliases.frequency+1, 2147483647);",
            word,
            normalized,
            "user dictionary alias");
        commit_dictionary_transaction(storage_->db);
    } catch (...) {
        sqlite3_exec(storage_->db, "ROLLBACK;", nullptr, nullptr, nullptr);
        throw;
    }
    storage_->cache_clear();
    storage_->log_event("user-add", word, "pinned user word");
}

void Dictionary::remove_user_word(const std::string& word) {
    std::lock_guard<std::mutex> lock(storage_->write_mutex);
    begin_dictionary_transaction(storage_->db);
    const auto execute_word_statement = [this, &word](const char* sql, const char* operation) {
        sqlite3_stmt* stmt = nullptr;
        const int prepare_rc = sqlite3_prepare_v2(storage_->db, sql, -1, &stmt, nullptr);
        if (prepare_rc != SQLITE_OK) {
            throw DictionaryError(
                DictionaryErrorCategory::prepare, prepare_rc, sqlite3_errmsg(storage_->db));
        }
        if (sqlite3_bind_text(stmt, 1, word.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
            sqlite3_finalize(stmt);
            throw DictionaryError(DictionaryErrorCategory::bind,
                                  sqlite3_extended_errcode(storage_->db),
                                  std::string("Unable to bind user word ") + operation);
        }
        const int step_rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        if (step_rc != SQLITE_DONE) {
            throw DictionaryError(
                DictionaryErrorCategory::step, step_rc, sqlite3_errmsg(storage_->db));
        }
    };

    try {
        execute_word_statement("DELETE FROM dictionary_user_aliases WHERE word=?;",
                               "remove aliases");
        execute_word_statement(
            "UPDATE dictionary_entries SET user_pinned=0, user_frequency=0 "
            "WHERE word=? AND source='system';",
            "unpin");
        execute_word_statement("DELETE FROM dictionary_entries WHERE word=? AND source='user';",
                               "delete");
        commit_dictionary_transaction(storage_->db);
    } catch (...) {
        sqlite3_exec(storage_->db, "ROLLBACK;", nullptr, nullptr, nullptr);
        throw;
    }
    storage_->cache_clear();
    storage_->log_event("user-remove", word, "user word removed");
}

std::vector<std::string> Dictionary::list_user_words() const {
    std::lock_guard<std::mutex> lock(storage_->write_mutex);
    using Id = DictionaryStorage::CachedId;
    sqlite3_stmt* stmt = storage_->cached_stmt(Id::ListUserWords);
    std::vector<std::string> results;
    try {
        int step_rc = SQLITE_OK;
        while ((step_rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            const unsigned char* value = sqlite3_column_text(stmt, 0);
            if (value != nullptr) {
                results.emplace_back(reinterpret_cast<const char*>(value));
            }
        }
        if (step_rc != SQLITE_DONE) {
            throw DictionaryError(
                DictionaryErrorCategory::step, step_rc, sqlite3_errmsg(storage_->db));
        }
    } catch (...) {
        storage_->reset_quietly(Id::ListUserWords);
        throw;
    }
    storage_->reset_quietly(Id::ListUserWords);
    return results;
}

int Dictionary::get_user_frequency(const std::string& word) const {
    std::lock_guard<std::mutex> lock(storage_->write_mutex);
    using Id = DictionaryStorage::CachedId;
    sqlite3_stmt* stmt = storage_->cached_stmt(Id::UserFrequency);
    int frequency = 0;
    try {
        if (sqlite3_bind_text(stmt, 1, word.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
            throw DictionaryError(DictionaryErrorCategory::bind,
                                  sqlite3_extended_errcode(storage_->db),
                                  "Unable to bind user frequency query");
        }
        const int step_rc = sqlite3_step(stmt);
        if (step_rc == SQLITE_ROW) {
            frequency = sqlite3_column_int(stmt, 0);
        } else if (step_rc != SQLITE_DONE) {
            throw DictionaryError(
                DictionaryErrorCategory::step, step_rc, sqlite3_errmsg(storage_->db));
        }
    } catch (...) {
        storage_->reset_quietly(Id::UserFrequency);
        throw;
    }
    storage_->reset_quietly(Id::UserFrequency);
    return frequency;
}

void Dictionary::set_word_category(const std::string& word, const std::string& category) {
    std::lock_guard<std::mutex> lock(storage_->write_mutex);
    sqlite3_stmt* stmt = nullptr;
    const int prepare_rc = sqlite3_prepare_v2(storage_->db,
                                              "UPDATE dictionary_entries SET category=? "
                                              "WHERE word=?;",
                                              -1,
                                              &stmt,
                                              nullptr);
    if (prepare_rc != SQLITE_OK) {
        throw DictionaryError(
            DictionaryErrorCategory::prepare, prepare_rc, sqlite3_errmsg(storage_->db));
    }
    if (sqlite3_bind_text(stmt, 1, category.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK
        || sqlite3_bind_text(stmt, 2, word.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        throw DictionaryError(DictionaryErrorCategory::bind,
                              sqlite3_extended_errcode(storage_->db),
                              "Unable to bind category update");
    }
    const int rc = sqlite3_step(stmt);
    const int changed = sqlite3_changes(storage_->db);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        throw DictionaryError(DictionaryErrorCategory::step, rc, sqlite3_errmsg(storage_->db));
    }
    if (changed == 0) {
        throw DictionaryError(DictionaryErrorCategory::validation,
                              SQLITE_MISMATCH,
                              "Unknown word for category update: " + word);
    }
    storage_->cache_clear();
    storage_->log_event("category", word, category.c_str());
}

void Dictionary::add_word_relation(const std::string& word,
                                   const std::string& related,
                                   const std::string& kind) {
    if (!is_valid_word(word) || !is_valid_word(related)) {
        throw DictionaryError(DictionaryErrorCategory::validation,
                              SQLITE_MISMATCH,
                              "Invalid word in relation: " + word + " / " + related);
    }
    if (kind != "synonym" && kind != "variant" && kind != "antonym") {
        throw DictionaryError(
            DictionaryErrorCategory::validation, SQLITE_MISMATCH, "Unknown relation kind: " + kind);
    }
    std::lock_guard<std::mutex> lock(storage_->write_mutex);
    sqlite3_stmt* stmt = nullptr;
    const int prepare_rc = sqlite3_prepare_v2(storage_->db,
                                              "INSERT OR IGNORE INTO word_relations"
                                              "(word, related, kind) VALUES(?, ?, ?);",
                                              -1,
                                              &stmt,
                                              nullptr);
    if (prepare_rc != SQLITE_OK) {
        throw DictionaryError(
            DictionaryErrorCategory::prepare, prepare_rc, sqlite3_errmsg(storage_->db));
    }
    if (sqlite3_bind_text(stmt, 1, word.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK
        || sqlite3_bind_text(stmt, 2, related.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK
        || sqlite3_bind_text(stmt, 3, kind.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        throw DictionaryError(DictionaryErrorCategory::bind,
                              sqlite3_extended_errcode(storage_->db),
                              "Unable to bind word relation");
    }
    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        throw DictionaryError(DictionaryErrorCategory::step, rc, sqlite3_errmsg(storage_->db));
    }
    storage_->log_event("relation", word, (kind + " -> " + related).c_str());
}

std::vector<std::string> Dictionary::related_words(const std::string& word,
                                                   const std::string& kind,
                                                   std::size_t limit) const {
    if (limit == 0) {
        return {};
    }
    std::lock_guard<std::mutex> lock(storage_->write_mutex);
    // Management-path query: prepared per call (statement reuse across

    // interleaved queries showed stale-snapshot results on some sqlite

    // builds; these paths are not keystroke-hot, so the cost is negligible).

    const char* sql =
        "SELECT related FROM word_relations "
        "WHERE word = ? AND (? = '' OR kind = ?) ORDER BY rowid LIMIT ?;";
    const std::size_t bounded_limit =
        std::min(limit, static_cast<std::size_t>(std::numeric_limits<sqlite3_int64>::max()));
    std::vector<std::string> results;
    sqlite3_stmt* stmt = nullptr;
    const int prepare_rc = sqlite3_prepare_v2(storage_->db, sql, -1, &stmt, nullptr);
    if (prepare_rc != SQLITE_OK) {
        throw DictionaryError(
            DictionaryErrorCategory::prepare, prepare_rc, sqlite3_errmsg(storage_->db));
    }
    try {
        if (sqlite3_bind_text(stmt, 1, word.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK
            || sqlite3_bind_text(stmt, 2, kind.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK
            || sqlite3_bind_text(stmt, 3, kind.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK
            || sqlite3_bind_int64(stmt, 4, static_cast<sqlite3_int64>(bounded_limit))
                   != SQLITE_OK) {
            throw DictionaryError(DictionaryErrorCategory::bind,
                                  sqlite3_extended_errcode(storage_->db),
                                  "Unable to bind related words query");
        }
        int step_rc = SQLITE_OK;
        while ((step_rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            const unsigned char* value = sqlite3_column_text(stmt, 0);
            if (value != nullptr) {
                results.emplace_back(reinterpret_cast<const char*>(value));
            }
        }
        if (step_rc != SQLITE_DONE) {
            throw DictionaryError(
                DictionaryErrorCategory::step, step_rc, sqlite3_errmsg(storage_->db));
        }
    } catch (...) {
        sqlite3_finalize(stmt);
        throw;
    }
    sqlite3_finalize(stmt);
    return results;
}

std::vector<std::string> Dictionary::words_by_category(const std::string& category,
                                                       std::size_t limit) const {
    if (limit == 0) {
        return {};
    }
    std::lock_guard<std::mutex> lock(storage_->write_mutex);
    // Management-path query: prepared per call (see related_words).

    const char* sql =
        "SELECT word FROM dictionary_entries WHERE category = ? "
        "ORDER BY (frequency + user_frequency) DESC, word ASC LIMIT ?;";
    const std::size_t bounded_limit =
        std::min(limit, static_cast<std::size_t>(std::numeric_limits<sqlite3_int64>::max()));
    std::vector<std::string> results;
    sqlite3_stmt* stmt = nullptr;
    const int prepare_rc = sqlite3_prepare_v2(storage_->db, sql, -1, &stmt, nullptr);
    if (prepare_rc != SQLITE_OK) {
        throw DictionaryError(
            DictionaryErrorCategory::prepare, prepare_rc, sqlite3_errmsg(storage_->db));
    }
    try {
        if (sqlite3_bind_text(stmt, 1, category.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK
            || sqlite3_bind_int64(stmt, 2, static_cast<sqlite3_int64>(bounded_limit))
                   != SQLITE_OK) {
            throw DictionaryError(DictionaryErrorCategory::bind,
                                  sqlite3_extended_errcode(storage_->db),
                                  "Unable to bind category query");
        }
        int step_rc = SQLITE_OK;
        while ((step_rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            const unsigned char* value = sqlite3_column_text(stmt, 0);
            if (value != nullptr) {
                results.emplace_back(reinterpret_cast<const char*>(value));
            }
        }
        if (step_rc != SQLITE_DONE) {
            throw DictionaryError(
                DictionaryErrorCategory::step, step_rc, sqlite3_errmsg(storage_->db));
        }
    } catch (...) {
        sqlite3_finalize(stmt);
        throw;
    }
    sqlite3_finalize(stmt);
    return results;
}

std::vector<DuplicateGroup> Dictionary::find_duplicate_normalized() const {
    std::lock_guard<std::mutex> lock(storage_->write_mutex);
    const char* query =
        "SELECT normalized, COUNT(*) AS copies FROM dictionary_entries "
        "GROUP BY normalized HAVING copies > 1 ORDER BY copies DESC LIMIT 500";
    std::vector<DuplicateGroup> groups;
    sqlite3_stmt* stmt = nullptr;
    const int prepare_rc = sqlite3_prepare_v2(storage_->db, query, -1, &stmt, nullptr);
    if (prepare_rc != SQLITE_OK) {
        throw DictionaryError(
            DictionaryErrorCategory::prepare, prepare_rc, sqlite3_errmsg(storage_->db));
    }
    int step_rc = SQLITE_OK;
    while ((step_rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        DuplicateGroup group;
        const unsigned char* key = sqlite3_column_text(stmt, 0);
        if (key != nullptr) {
            group.normalized = reinterpret_cast<const char*>(key);
        }
        groups.push_back(std::move(group));
    }
    sqlite3_finalize(stmt);
    if (step_rc != SQLITE_DONE) {
        throw DictionaryError(DictionaryErrorCategory::step, step_rc, sqlite3_errmsg(storage_->db));
    }
    // Second pass: fetch the words behind each duplicated normalized key.

    for (DuplicateGroup& group : groups) {
        sqlite3_stmt* words_stmt = nullptr;
        const int words_prepare = sqlite3_prepare_v2(storage_->db,
                                                     "SELECT word FROM dictionary_entries "
                                                     "WHERE normalized = ? "
                                                     "ORDER BY (frequency + user_frequency) DESC;",
                                                     -1,
                                                     &words_stmt,
                                                     nullptr);
        if (words_prepare != SQLITE_OK) {
            throw DictionaryError(
                DictionaryErrorCategory::prepare, words_prepare, sqlite3_errmsg(storage_->db));
        }
        sqlite3_bind_text(words_stmt, 1, group.normalized.c_str(), -1, SQLITE_TRANSIENT);
        int words_rc = SQLITE_OK;
        while ((words_rc = sqlite3_step(words_stmt)) == SQLITE_ROW) {
            const unsigned char* value = sqlite3_column_text(words_stmt, 0);
            if (value != nullptr) {
                group.words.emplace_back(reinterpret_cast<const char*>(value));
            }
        }
        sqlite3_finalize(words_stmt);
        if (words_rc != SQLITE_DONE) {
            throw DictionaryError(
                DictionaryErrorCategory::step, words_rc, sqlite3_errmsg(storage_->db));
        }
    }
    return groups;
}

DictionaryStats Dictionary::stats() const {
    std::lock_guard<std::mutex> lock(storage_->write_mutex);
    DictionaryStats result;
    const auto scalar = [this](const char* sql) -> std::uint64_t {
        sqlite3_stmt* stmt = nullptr;
        const int prepare_rc = sqlite3_prepare_v2(storage_->db, sql, -1, &stmt, nullptr);
        if (prepare_rc != SQLITE_OK) {
            throw DictionaryError(
                DictionaryErrorCategory::prepare, prepare_rc, sqlite3_errmsg(storage_->db));
        }
        std::uint64_t value = 0;
        const int step_rc = sqlite3_step(stmt);
        if (step_rc == SQLITE_ROW) {
            value = static_cast<std::uint64_t>(sqlite3_column_int64(stmt, 0));
        } else if (step_rc != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            throw DictionaryError(
                DictionaryErrorCategory::step, step_rc, sqlite3_errmsg(storage_->db));
        }
        sqlite3_finalize(stmt);
        return value;
    };
    result.total_entries = scalar("SELECT COUNT(*) FROM dictionary_entries");
    result.system_entries = scalar("SELECT COUNT(*) FROM dictionary_entries WHERE source='system'");
    result.user_entries = scalar("SELECT COUNT(*) FROM dictionary_entries WHERE source='user'");
    result.user_aliases = scalar("SELECT COUNT(*) FROM dictionary_user_aliases");
    result.word_relations = scalar("SELECT COUNT(*) FROM word_relations");
    result.word_transitions = scalar("SELECT COUNT(*) FROM word_transitions");
    result.changelog_entries = scalar("SELECT COUNT(*) FROM dictionary_changelog");

    sqlite3_stmt* stmt = nullptr;
    const int categories_prepare =
        sqlite3_prepare_v2(storage_->db,
                           "SELECT COALESCE(NULLIF(category, ''), 'uncategorized') AS bucket,"
                           " COUNT(*) FROM dictionary_entries GROUP BY bucket"
                           " ORDER BY COUNT(*) DESC LIMIT 12;",
                           -1,
                           &stmt,
                           nullptr);
    if (categories_prepare != SQLITE_OK) {
        throw DictionaryError(
            DictionaryErrorCategory::prepare, categories_prepare, sqlite3_errmsg(storage_->db));
    }
    int step_rc = SQLITE_OK;
    while ((step_rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const unsigned char* name = sqlite3_column_text(stmt, 0);
        result.top_categories.emplace_back(
            name == nullptr ? "uncategorized" : reinterpret_cast<const char*>(name),
            static_cast<std::uint64_t>(sqlite3_column_int64(stmt, 1)));
    }
    sqlite3_finalize(stmt);
    if (step_rc != SQLITE_DONE) {
        throw DictionaryError(DictionaryErrorCategory::step, step_rc, sqlite3_errmsg(storage_->db));
    }

    const char* tiers =
        "SELECT "
        "SUM(CASE WHEN frequency + user_frequency >= 1000 THEN 1 ELSE 0 END),"
        "SUM(CASE WHEN frequency + user_frequency >= 500 AND frequency + user_frequency < 1000 THEN 1 ELSE 0 END),"
        "SUM(CASE WHEN frequency + user_frequency >= 100 AND frequency + user_frequency < 500 THEN 1 ELSE 0 END),"
        "SUM(CASE WHEN frequency + user_frequency >= 50 AND frequency + user_frequency < 100 THEN 1 ELSE 0 END),"
        "SUM(CASE WHEN frequency + user_frequency < 50 THEN 1 ELSE 0 END)"
        " FROM dictionary_entries";
    sqlite3_stmt* tier_stmt = nullptr;
    const int tier_prepare = sqlite3_prepare_v2(storage_->db, tiers, -1, &tier_stmt, nullptr);
    if (tier_prepare != SQLITE_OK) {
        throw DictionaryError(
            DictionaryErrorCategory::prepare, tier_prepare, sqlite3_errmsg(storage_->db));
    }
    const int tier_rc = sqlite3_step(tier_stmt);
    if (tier_rc == SQLITE_ROW) {
        const char* labels[] = {">=1000", "500-999", "100-499", "50-99", "<50"};
        for (int index = 0; index < 5; ++index) {
            result.frequency_tiers.emplace_back(
                labels[index], static_cast<std::uint64_t>(sqlite3_column_int64(tier_stmt, index)));
        }
    } else if (tier_rc != SQLITE_DONE) {
        sqlite3_finalize(tier_stmt);
        throw DictionaryError(DictionaryErrorCategory::step, tier_rc, sqlite3_errmsg(storage_->db));
    }
    sqlite3_finalize(tier_stmt);
    return result;
}

std::vector<WordRecord> Dictionary::all_entries() const {
    std::lock_guard<std::mutex> lock(storage_->write_mutex);
    const char* query =
        "SELECT word, normalized, frequency, user_frequency, user_pinned, source,"
        " category, register, origin FROM dictionary_entries ORDER BY word;";
    std::vector<WordRecord> records;
    sqlite3_stmt* stmt = nullptr;
    const int prepare_rc = sqlite3_prepare_v2(storage_->db, query, -1, &stmt, nullptr);
    if (prepare_rc != SQLITE_OK) {
        throw DictionaryError(
            DictionaryErrorCategory::prepare, prepare_rc, sqlite3_errmsg(storage_->db));
    }
    const auto text_or_empty = [&stmt](int column) {
        const unsigned char* value = sqlite3_column_text(stmt, column);
        return value == nullptr ? std::string() : std::string(reinterpret_cast<const char*>(value));
    };
    int step_rc = SQLITE_OK;
    while ((step_rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        WordRecord record;
        record.word = text_or_empty(0);
        record.normalized = text_or_empty(1);
        record.frequency = sqlite3_column_int(stmt, 2);
        record.user_frequency = sqlite3_column_int(stmt, 3);
        record.user_pinned = sqlite3_column_int(stmt, 4) != 0;
        record.source = text_or_empty(5);
        record.category = text_or_empty(6);
        record.word_register = text_or_empty(7);
        record.origin = text_or_empty(8);
        records.push_back(std::move(record));
    }
    sqlite3_finalize(stmt);
    if (step_rc != SQLITE_DONE) {
        throw DictionaryError(DictionaryErrorCategory::step, step_rc, sqlite3_errmsg(storage_->db));
    }
    return records;
}

std::vector<std::pair<std::string, std::string>> Dictionary::all_relations() const {
    std::lock_guard<std::mutex> lock(storage_->write_mutex);
    const char* query =
        "SELECT word, kind || ':' || related FROM word_relations ORDER BY word, kind, related;";
    std::vector<std::pair<std::string, std::string>> relations;
    sqlite3_stmt* stmt = nullptr;
    const int prepare_rc = sqlite3_prepare_v2(storage_->db, query, -1, &stmt, nullptr);
    if (prepare_rc != SQLITE_OK) {
        throw DictionaryError(
            DictionaryErrorCategory::prepare, prepare_rc, sqlite3_errmsg(storage_->db));
    }
    int step_rc = SQLITE_OK;
    while ((step_rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const unsigned char* first = sqlite3_column_text(stmt, 0);
        const unsigned char* second = sqlite3_column_text(stmt, 1);
        relations.emplace_back(
            first == nullptr ? std::string() : std::string(reinterpret_cast<const char*>(first)),
            second == nullptr ? std::string() : std::string(reinterpret_cast<const char*>(second)));
    }
    sqlite3_finalize(stmt);
    if (step_rc != SQLITE_DONE) {
        throw DictionaryError(DictionaryErrorCategory::step, step_rc, sqlite3_errmsg(storage_->db));
    }
    return relations;
}

void Dictionary::backup_to(const std::string& destination_path) const {
    std::lock_guard<std::mutex> lock(storage_->write_mutex);
    sqlite3* destination = nullptr;
    if (sqlite3_open_v2(destination_path.c_str(),
                        &destination,
                        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                        nullptr)
        != SQLITE_OK) {
        const std::string message =
            destination == nullptr ? "unable to open backup target" : sqlite3_errmsg(destination);
        if (destination != nullptr) {
            sqlite3_close(destination);
        }
        throw DictionaryError(DictionaryErrorCategory::open, SQLITE_CANTOPEN, message);
    }
    sqlite3_backup* backup = sqlite3_backup_init(destination, "main", storage_->db, "main");
    if (backup == nullptr) {
        const std::string message = sqlite3_errmsg(destination);
        sqlite3_close(destination);
        throw DictionaryError(DictionaryErrorCategory::open, SQLITE_CANTOPEN, message);
    }
    int rc = sqlite3_backup_step(backup, -1);
    sqlite3_backup_finish(backup);
    sqlite3_close(destination);
    if (rc != SQLITE_DONE) {
        throw DictionaryError(
            DictionaryErrorCategory::step, rc, "Dictionary backup did not complete");
    }
}

void Dictionary::vacuum() {
    std::lock_guard<std::mutex> lock(storage_->write_mutex);
    char* error = nullptr;
    const int rc = sqlite3_exec(storage_->db, "VACUUM;", nullptr, nullptr, &error);
    if (rc != SQLITE_OK) {
        const std::string message = error == nullptr ? "unknown SQLite error" : error;
        sqlite3_free(error);
        throw DictionaryError(DictionaryErrorCategory::step, rc, message);
    }
    storage_->cache_clear();
}

}  // namespace suborno::dictionary
