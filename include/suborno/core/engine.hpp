#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

#include "suborno/layout-api/input_session.hpp"
#include "suborno/layout-api/layout_registry.hpp"
#include "suborno/settings/settings.hpp"

namespace suborno::dictionary {
class Dictionary;
}

namespace suborno::core {

struct EngineConfig {
    std::filesystem::path settings_path;
    std::filesystem::path dictionary_path;
    /// Bundled assets directory (contains dictionaries/ac.json and
    /// layouts/bn-khipro.mim). When set and the files exist, the BornoTSF
    /// compatibility engines load their data from here before falling back
    /// to the per-user / program-data search order. Empty = standard search.
    std::filesystem::path assets_path;
};

class Engine {
  public:
    Engine();
    explicit Engine(const EngineConfig& config);
    ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(Engine&&) = delete;

    bool register_layout(std::string_view id, std::unique_ptr<layout::InputLayout> layout);
    bool set_active_layout(std::string_view id);
    // Returns by value on purpose: a std::string_view into the registry would
    // dangle once set_active_layout() reassigns the underlying std::string.
    [[nodiscard]] std::string active_layout_id() const;
    layout::LayoutKind active_layout_kind() const noexcept;
    // Kind of any registered layout (passthrough for the English mode, etc.);
    // phonetic when the id is unknown.
    layout::LayoutKind layout_kind_of(std::string_view layout_id) const noexcept;
    const layout::LayoutMetadata* layout_metadata(std::string_view id) const;

    std::unique_ptr<layout::InputSession> create_session();
    // Session for a specific registered layout — per-application-window
    // switching creates sessions for a context's remembered layout, not the
    // engine's active one. Falls back to the active layout when the id is
    // unknown (e.g. a settings file written by a build with other layouts).
    std::unique_ptr<layout::InputSession> create_session_for_layout(std::string_view layout_id);

    // Applies the configured output encoding (input.preferred_encoding) to
    // committed text: Unicode passes through unchanged; Bijoy ANSI converts
    // to the legacy ASCII encoding when every grapheme is encodable (unknown
    // or invalid input falls back to Unicode so nothing is ever dropped).
    [[nodiscard]] std::string encode_text_for_output(const std::string& text_utf8) const;

    // What the ACTIVE layout produces for a typed letter (e.g. "k" -> "ক").
    // Drives the On-Screen Keyboard's Bangla key labels. Empty for keys the
    // active layout does not map.
    [[nodiscard]] std::string layout_preview(std::string_view letter) const;

    // Next-word prediction controls (bigram transition model). TSF callers
    // cannot write the settings file themselves, so the toggle updates the
    // in-memory settings and persists them through the settings store when
    // one exists.
    void set_next_word_prediction_enabled(bool enabled);
    // Tracks the word the user just committed so the NEXT composition can be
    // predicted from it. Consecutive calls record a transition
    // (previous -> current) in the dictionary while learning is enabled. No
    // locking on purpose: TSF keyboards run one Engine instance per thread,
    // so the only mutating caller is the owning thread.
    void record_committed_word(const std::string& committed_word_normalized);
    // Clears all user learning (dictionary user words, user frequencies and
    // the transition history) and forgets the last committed word. Settings
    // are untouched; returns false when no dictionary is configured.
    bool clear_learnt_words();
    [[nodiscard]] std::string last_committed_word() const;

    settings::SettingsStatus save_settings(const settings::EngineSettings& settings);
    settings::SettingsLoadResult load_settings() const;
    settings::SettingsLoadResult reload_settings();
    // Reloads only when the settings file actually changed on disk. Callers
    // that poll on focus changes should prefer this over reload_settings(),
    // which re-reads and re-validates the file every time.
    settings::SettingsLoadResult reload_settings_if_changed();
    const settings::EngineSettings& settings() const noexcept;
    dictionary::Dictionary* dictionary() noexcept;
    const dictionary::Dictionary* dictionary() const noexcept;

  private:
    layout::LayoutRegistry layouts_;
    settings::EngineSettings current_settings_;
    std::unique_ptr<settings::SettingsStore> settings_store_;
    settings::SettingsStore::FileStamp settings_file_stamp_{};
    std::unique_ptr<dictionary::Dictionary> dictionary_;
    /// Normalized last word fed through record_committed_word(); drives both
    /// the transition chain and the prediction context of new sessions.
    std::string last_committed_word_;
};

}  // namespace suborno::core
