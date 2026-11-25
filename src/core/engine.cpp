#include "suborno/core/engine.hpp"

#include <string>

#include "borno_compat_dictionary_loader.hpp"
#include "borno_compat_settings.hpp"
#include "suborno/dictionary/dictionary.hpp"
#include "suborno/layouts/english/english_layout.hpp"
#include "suborno/layouts/khipro/khipro_layout.hpp"
#include "suborno/layouts/national/national_layout.hpp"
#include "suborno/layouts/phonetic/avro_phonetic_layout.hpp"
#include "suborno/layouts/phonetic/borno_phonetic_layout.hpp"
#include "suborno/layouts/phonetic/suborno_phonetic_layout.hpp"
#include "suborno/layouts/probhat/probhat_layout.hpp"
#include "suborno/settings/settings.hpp"
#include "suborno/unicode/bijoy.hpp"

namespace suborno::core {
namespace {

// Keeps the legacy-layout compatibility engines (src/layouts/borno_compat)

// in sync with the settings store: the phonetic-correction switch is shared

// between the settings snapshot and the engines' process-global flag.

void sync_borno_compat_settings(const settings::EngineSettings& settings) {
    borno_compat::set_phonetic_correction(settings.phonetic_correction);
}

}  // namespace

Engine::Engine() {
    // The first registration becomes the default active layout: keep

    // "suborno-phonetic" first so a fresh install starts in Bangla.

    layouts_.register_layout("suborno-phonetic",
                             std::make_unique<layouts::SubornoPhoneticLayout>(),
                             layout::LayoutMetadata{"suborno-phonetic", "বাংলা (Suborno)", "1"});
    // English passthrough: switching to it disables transliteration entirely,

    // so users can always fall back to plain Latin typing from any menu.

    layouts_.register_layout("english",
                             std::make_unique<layouts::EnglishLayout>(),
                             layout::LayoutMetadata{"english", "English", "1"});
    // Display names shown to users never contain the implementation term

    // "Phonetic" — the topbar, tray, and settings menus all render these.

    layouts_.register_layout("borno-phonetic",
                             std::make_unique<layouts::BornoPhoneticLayout>(),
                             layout::LayoutMetadata{"borno-phonetic", "বাংলা (Borno)", "1"});
    layouts_.register_layout("avro-phonetic",
                             std::make_unique<layouts::AvroPhoneticLayout>(),
                             layout::LayoutMetadata{"avro-phonetic", "Avro", "1"});
    layouts_.register_layout("khipro",
                             std::make_unique<layouts::KhiproLayout>(),
                             layout::LayoutMetadata{"khipro", "Khipro", "1"});
    layouts_.register_layout("probhat",
                             std::make_unique<layouts::ProbhatLayout>(),
                             layout::LayoutMetadata{"probhat", "Probhat", "1"});
    layouts_.register_layout("national",
                             std::make_unique<layouts::NationalLayout>(),
                             layout::LayoutMetadata{"national", "National", "1"});
}

Engine::Engine(const EngineConfig& config)
    : Engine() {
    if (!config.settings_path.empty()) {
        settings_store_ = std::make_unique<settings::SettingsStore>(config.settings_path);
        const settings::SettingsLoadResult result = settings_store_->load();
        if (result.status == settings::SettingsStatus::ok) {
            current_settings_ = result.settings;
            sync_borno_compat_settings(current_settings_);
            const std::string_view restored_layout =
                current_settings_.general.remember_last_input_layout
                    ? std::string_view(current_settings_.active_layout_id)
                    : std::string_view("suborno-phonetic");
            if (!layouts_.set_active(restored_layout)) {
                current_settings_.active_layout_id = std::string(layouts_.active_id());
            } else {
                current_settings_.active_layout_id = std::string(restored_layout);
            }
        }
    }
    if (!config.dictionary_path.empty()) {
        const std::u8string dictionary_path = config.dictionary_path.u8string();
        dictionary_ = std::make_unique<dictionary::Dictionary>(std::string(
            reinterpret_cast<const char*>(dictionary_path.data()), dictionary_path.size()));
    }
    if (!config.assets_path.empty()) {
        // Point the BornoTSF compatibility engines at the bundled asset

        // copies when they exist. The overrides are process-global and

        // idempotent (every engine instance resolves the same install tree),

        // so per-thread TSF engines never race each other here.

        const std::filesystem::path ac_json = config.assets_path / "dictionaries" / "ac.json";
        const std::filesystem::path khipro_mim = config.assets_path / "layouts" / "bn-khipro.mim";
        std::error_code ec;
        if (std::filesystem::exists(ac_json, ec)) {
            DictionaryLoader::set_override_path(ac_json.wstring());
        }
        if (std::filesystem::exists(khipro_mim, ec)) {
            borno_compat::set_khipro_spec_override(khipro_mim.wstring());
        }
    }
}

Engine::~Engine() = default;

bool Engine::register_layout(std::string_view id, std::unique_ptr<layout::InputLayout> layout) {
    return layouts_.register_layout(id, std::move(layout));
}

bool Engine::set_active_layout(std::string_view id) {
    return layouts_.set_active(id);
}

std::string Engine::active_layout_id() const {
    return std::string(layouts_.active_id());
}

layout::LayoutKind Engine::active_layout_kind() const noexcept {
    const layout::InputLayout* active_layout = layouts_.active();
    return active_layout == nullptr ? layout::LayoutKind::phonetic : active_layout->kind();
}

layout::LayoutKind Engine::layout_kind_of(std::string_view layout_id) const noexcept {
    const layout::InputLayout* found = layouts_.find(layout_id);
    return found == nullptr ? layout::LayoutKind::phonetic : found->kind();
}

const layout::LayoutMetadata* Engine::layout_metadata(std::string_view id) const {
    return layouts_.find_metadata(id);
}

std::unique_ptr<layout::InputSession> Engine::create_session() {
    const layout::InputLayout* active_layout = layouts_.active();
    if (active_layout == nullptr) {
        return nullptr;
    }

    const layout::InputSessionOptions options{
        .candidate_limit = current_settings_.candidate_limit,
        .learning_enabled = current_settings_.learning_enabled,
        .previous_word = last_committed_word_,
        .next_word_prediction_enabled = current_settings_.ui.next_word_prediction_enabled};
    return active_layout->create_session(dictionary_.get(), options);
}

std::unique_ptr<layout::InputSession> Engine::create_session_for_layout(
    std::string_view layout_id) {
    const layout::InputLayout* requested = layouts_.find(layout_id);
    if (requested == nullptr) {
        // Unknown id (settings written by another build): keep typing on the

        // active layout instead of dropping the session.

        return create_session();
    }
    const layout::InputSessionOptions options{
        .candidate_limit = current_settings_.candidate_limit,
        .learning_enabled = current_settings_.learning_enabled,
        .previous_word = last_committed_word_,
        .next_word_prediction_enabled = current_settings_.ui.next_word_prediction_enabled};
    return requested->create_session(dictionary_.get(), options);
}

std::string Engine::encode_text_for_output(const std::string& text_utf8) const {
    if (current_settings_.input.preferred_encoding != settings::TextEncoding::ansi) {
        return text_utf8;
    }
    if (!unicode::is_bijoy_encodable(text_utf8)) {
        // Not every grapheme has a legacy byte (or the text is invalid):

        // falling back to Unicode loses nothing, converting would.

        return text_utf8;
    }
    return unicode::encode_bijoy(text_utf8);
}

std::string Engine::layout_preview(std::string_view letter) const {
    const layout::InputLayout* active_layout = layouts_.active();
    if (active_layout == nullptr || letter.empty()) {
        return {};
    }
    return active_layout->transliterate(std::string(letter));
}

void Engine::set_next_word_prediction_enabled(bool enabled) {
    current_settings_.ui.next_word_prediction_enabled = enabled;
    if (settings_store_ == nullptr) {
        return;
    }
    // Persist the whole settings struct the same way save_settings() does, so

    // every other field on disk survives the toggle (the in-memory

    // active_layout_id is always valid, so the save cannot fail validation).

    static_cast<void>(settings_store_->save(current_settings_));
}

void Engine::record_committed_word(const std::string& committed_word_normalized) {
    // Empty words carry no learnable context; ignore them instead of breaking

    // the previous -> current chain.

    if (committed_word_normalized.empty()) {
        return;
    }
    // Repeating the same word is still a real bigram ("হ্যাঁ হ্যাঁ"), so any

    // non-empty call after the first one records a transition. Transition

    // recording honors the learning switch; tracking the last word does not,

    // because prediction and learning are separate user settings.

    if (dictionary_ != nullptr && current_settings_.learning_enabled
        && !last_committed_word_.empty()) {
        dictionary_->record_transition(last_committed_word_, committed_word_normalized);
    }
    last_committed_word_ = committed_word_normalized;
}

bool Engine::clear_learnt_words() {
    // Forget the in-memory context first: even if the dictionary clear throws,

    // the engine must not keep predicting from a context the user wiped.

    last_committed_word_.clear();
    if (dictionary_ == nullptr) {
        return false;
    }
    dictionary_->clear_user_entries();
    return true;
}

std::string Engine::last_committed_word() const {
    return last_committed_word_;
}

settings::SettingsStatus Engine::save_settings(const settings::EngineSettings& settings) {
    if (settings_store_ == nullptr) {
        return settings::SettingsStatus::io_error;
    }
    if (layouts_.find(settings.active_layout_id) == nullptr) {
        return settings::SettingsStatus::invalid_format;
    }
    const settings::SettingsStatus status = settings_store_->save(settings);
    if (status == settings::SettingsStatus::ok) {
        current_settings_ = settings;
        sync_borno_compat_settings(current_settings_);
        layouts_.set_active(settings.active_layout_id);
    }
    return status;
}

settings::SettingsLoadResult Engine::load_settings() const {
    if (settings_store_ == nullptr) {
        settings::SettingsLoadResult result;
        result.settings = current_settings_;
        result.status = settings::SettingsStatus::ok;
        return result;
    }
    settings::SettingsLoadResult result = settings_store_->load();
    if (result.status == settings::SettingsStatus::ok
        && layouts_.find(result.settings.active_layout_id) == nullptr) {
        result.settings.active_layout_id = std::string(layouts_.active_id());
    }
    return result;
}

settings::SettingsLoadResult Engine::reload_settings() {
    settings::SettingsLoadResult result = load_settings();
    if (result.status != settings::SettingsStatus::ok) {
        return result;
    }

    if (!layouts_.set_active(result.settings.active_layout_id)) {
        result.settings.active_layout_id = std::string(layouts_.active_id());
    }
    current_settings_ = result.settings;
    sync_borno_compat_settings(current_settings_);
    if (settings_store_ != nullptr) {
        settings_file_stamp_ = settings_store_->file_stamp();
    }
    return result;
}

settings::SettingsLoadResult Engine::reload_settings_if_changed() {
    if (settings_store_ == nullptr) {
        settings::SettingsLoadResult result;
        result.settings = current_settings_;
        result.status = settings::SettingsStatus::ok;
        return result;
    }
    const settings::SettingsStore::FileStamp stamp = settings_store_->file_stamp();
    if (stamp == settings_file_stamp_) {
        settings::SettingsLoadResult result;
        result.settings = current_settings_;
        result.status = settings::SettingsStatus::ok;
        return result;
    }
    return reload_settings();
}

const settings::EngineSettings& Engine::settings() const noexcept {
    return current_settings_;
}

dictionary::Dictionary* Engine::dictionary() noexcept {
    return dictionary_.get();
}

const dictionary::Dictionary* Engine::dictionary() const noexcept {
    return dictionary_.get();
}

}  // namespace suborno::core
