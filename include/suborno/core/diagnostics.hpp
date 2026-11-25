#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace suborno::core {

enum class DiagnosticSeverity : std::uint8_t {
    debug = 0,
    info = 1,
    warning = 2,
    error = 3,
};

struct DiagnosticEntry {
    std::uint64_t sequence = 0;
    DiagnosticSeverity severity = DiagnosticSeverity::info;
    std::string category;
    std::string message;
};

/// Process-wide diagnostic log for lifecycle and error events.
///
/// PRIVACY CONTRACT: user-typed text must never reach this logger. The API
/// has no method that accepts composition or committed text at all, so
/// accidentally logging keystrokes cannot compile. When a diagnostic genuinely
/// needs to describe text volume (for example "dropped a 12-codepoint paste"),
/// wrap the value in scrub_user_text() first — that produces a marker with a
/// count and none of the characters.
///
/// The log is a fixed-capacity ring buffer guarded by a mutex, safe to call
/// from any thread (TSF keyboards run per-thread; the settings UI and the
/// standalone tray may log concurrently). Entries are numbered by a global
/// sequence so a drained snapshot can be ordered even when producers race.
class DiagnosticLog {
  public:
    static constexpr std::size_t ring_capacity = 512;

    static DiagnosticLog& instance() noexcept;

    /// Entries below the threshold are dropped before allocation of the ring
    /// slot. The default threshold keeps debug noise out of the ring.
    void set_min_severity(DiagnosticSeverity severity) noexcept;
    [[nodiscard]] DiagnosticSeverity min_severity() const noexcept;

    void log(DiagnosticSeverity severity, std::string_view category, std::string_view message);

    /// Replaces user text with a "[redacted N codepoints]" marker. N counts
    /// Unicode codepoints, not bytes, so the marker stays truthful for
    /// multi-byte Bengali input.
    [[nodiscard]] static std::string scrub_user_text(std::string_view user_text_utf8);

    /// Copy of the ring contents, oldest first. Bounded by ring_capacity.
    [[nodiscard]] std::vector<DiagnosticEntry> snapshot() const;

    /// The next sequence number that will be handed out (0 when nothing has
    /// been logged; monotonic). Sequence numbers survive clear(): consumers
    /// that drain the log to a file can rely on global monotonicity across
    /// resets, and callers can derive the first future sequence from this.
    [[nodiscard]] std::uint64_t last_sequence() const noexcept;

    void clear() noexcept;

    /// Appends future entries as "severity|category|message" lines. Returns
    /// false and keeps the current sink when the file cannot be opened. The
    /// sink is disabled on destruction and by clear_file_sink().
    bool set_file_sink(const std::filesystem::path& path);
    void clear_file_sink() noexcept;
    [[nodiscard]] bool file_sink_active() const noexcept;

  private:
    void write_to_sink(const DiagnosticEntry& entry);

    mutable std::mutex mutex_;
    std::vector<DiagnosticEntry> ring_;
    std::size_t next_slot_ = 0;
    std::size_t total_logged_ = 0;
    DiagnosticSeverity min_severity_ = DiagnosticSeverity::debug;
    std::uint64_t sequence_ = 0;
    std::ofstream sink_;
};

/// Convenience wrapper: instance().log(severity, category, message).
void log_diagnostic(DiagnosticSeverity severity,
                    std::string_view category,
                    std::string_view message);

}  // namespace suborno::core
