#include "suborno/core/diagnostics.hpp"

#include <algorithm>
#include <string>
#include <utility>

namespace suborno::core {
namespace {

std::string_view severity_name(DiagnosticSeverity severity) noexcept {
    switch (severity) {
    case DiagnosticSeverity::debug:
        return "debug";
    case DiagnosticSeverity::info:
        return "info";
    case DiagnosticSeverity::warning:
        return "warning";
    case DiagnosticSeverity::error:
        return "error";
    }
    return "unknown";
}

}  // namespace

DiagnosticLog& DiagnosticLog::instance() noexcept {
    // Function-local static: thread-safe initialisation is guaranteed by the
    // language since C++11, and the logger outlives every TSF thread that
    // reaches it during module shutdown.
    static DiagnosticLog logger;
    return logger;
}

void DiagnosticLog::set_min_severity(DiagnosticSeverity severity) noexcept {
    const std::lock_guard<std::mutex> guard(mutex_);
    min_severity_ = severity;
}

DiagnosticSeverity DiagnosticLog::min_severity() const noexcept {
    const std::lock_guard<std::mutex> guard(mutex_);
    return min_severity_;
}

void DiagnosticLog::log(DiagnosticSeverity severity,
                        std::string_view category,
                        std::string_view message) {
    {
        const std::lock_guard<std::mutex> guard(mutex_);
        if (severity < min_severity_) {
            return;
        }
        DiagnosticEntry entry;
        entry.sequence = sequence_++;
        entry.severity = severity;
        entry.category = category;
        entry.message = message;
        if (ring_.size() < ring_capacity) {
            ring_.push_back(std::move(entry));
            // While the ring is filling, the next overwrite position (once
            // full) is slot 0; size() modulo capacity reaches exactly 0 the
            // moment the ring becomes full.
            next_slot_ = ring_.size() % ring_capacity;
            write_to_sink(ring_.back());
        } else {
            DiagnosticEntry& slot = ring_[next_slot_];
            slot = std::move(entry);
            next_slot_ = (next_slot_ + 1U) % ring_capacity;
            write_to_sink(slot);
        }
        ++total_logged_;
    }
}

std::string DiagnosticLog::scrub_user_text(std::string_view user_text_utf8) {
    // Count Unicode codepoints: lead bytes (0xxxxxxx or 11xxxxxx) start one.
    std::size_t codepoints = 0;
    for (const char character : user_text_utf8) {
        const auto byte = static_cast<unsigned char>(character);
        if ((byte & 0xC0U) != 0x80U) {
            ++codepoints;
        }
    }
    return "[redacted " + std::to_string(codepoints) + " codepoints]";
}

std::vector<DiagnosticEntry> DiagnosticLog::snapshot() const {
    const std::lock_guard<std::mutex> guard(mutex_);
    std::vector<DiagnosticEntry> entries = ring_;
    std::sort(entries.begin(),
              entries.end(),
              [](const DiagnosticEntry& left, const DiagnosticEntry& right) {
                  return left.sequence < right.sequence;
              });
    return entries;
}

void DiagnosticLog::clear() noexcept {
    const std::lock_guard<std::mutex> guard(mutex_);
    ring_.clear();
    next_slot_ = 0;
    total_logged_ = 0;
}

std::uint64_t DiagnosticLog::last_sequence() const noexcept {
    const std::lock_guard<std::mutex> guard(mutex_);
    return sequence_;
}

bool DiagnosticLog::set_file_sink(const std::filesystem::path& path) {
    std::ofstream candidate(path, std::ios::app);
    if (!candidate.is_open()) {
        return false;
    }
    const std::lock_guard<std::mutex> guard(mutex_);
    // Swap under the same lock that serialises write_to_sink, so no entry is
    // written to the closing sink after this call returns.
    sink_ = std::move(candidate);
    return true;
}

void DiagnosticLog::clear_file_sink() noexcept {
    const std::lock_guard<std::mutex> guard(mutex_);
    if (sink_.is_open()) {
        sink_.flush();
        sink_.close();
    }
}

bool DiagnosticLog::file_sink_active() const noexcept {
    const std::lock_guard<std::mutex> guard(mutex_);
    return sink_.is_open();
}

void DiagnosticLog::write_to_sink(const DiagnosticEntry& entry) {
    // Caller holds mutex_. A failing sink is swallowed on purpose: the file
    // log is best-effort diagnostics, never a reason to disturb typing.
    if (!sink_.is_open()) {
        return;
    }
    sink_ << severity_name(entry.severity) << '|' << entry.category << '|' << entry.message << '\n';
}

void log_diagnostic(DiagnosticSeverity severity,
                    std::string_view category,
                    std::string_view message) {
    DiagnosticLog::instance().log(severity, category, message);
}

}  // namespace suborno::core
