// Coverage tests for src/core/engine.cpp (suite EngineCoverage).
// Each test names the previously uncovered gap lines it exercises; behavior
// already covered by engine_test.cpp is intentionally not duplicated here.

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

#include "suborno/core/engine.hpp"
#include "suborno/dictionary/dictionary.hpp"
#include "suborno/layout-api/phonetic_layout.hpp"
#include "suborno/layouts/english/english_layout.hpp"
#include "suborno/layouts/phonetic/suborno_phonetic_layout.hpp"
#include "suborno/settings/settings.hpp"
#include "suborno/unicode/bijoy.hpp"

namespace {

// Five distinct candidates per input so a configured candidate limit is
// observable through any engine-created session.
class FiveCandidateLayout final : public suborno::layout::PhoneticLayout {
  public:
    [[nodiscard]] std::string transliterate(const std::string& input) const override {
        return input;
    }

    [[nodiscard]] std::vector<suborno::layout::Candidate> generate_candidates(
        const std::string& input) const override {
        std::vector<suborno::layout::Candidate> candidates;
        for (int index = 1; index <= 5; ++index) {
            const std::string text = input + "-" + std::to_string(index);
            candidates.push_back({.text = text,
                                  .normalized_text = text,
                                  .stable_id = "layout:five:" + text,
                                  .source = suborno::layout::CandidateSource::layout,
                                  .score = 1.0 / static_cast<double>(index),
                                  .rationale = "five candidate layout"});
        }
        return candidates;
    }
};

std::filesystem::path coverage_path(const std::string& filename) {
    return std::filesystem::temp_directory_path() / filename;
}

void remove_file(const std::filesystem::path& path) {
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
}

void write_settings_file(const std::filesystem::path& path,
                         const suborno::settings::EngineSettings& settings) {
    ASSERT_EQ(suborno::settings::SettingsStore(path).save(settings),
              suborno::settings::SettingsStatus::ok);
}

suborno::core::EngineConfig config_with(const std::filesystem::path& settings_path,
                                        const std::filesystem::path& dictionary_path = {}) {
    suborno::core::EngineConfig config;
    config.settings_path = settings_path;
    config.dictionary_path = dictionary_path;
    return config;
}

}  // namespace

TEST(EngineCoverage, StorelessEngineSaveSettingsReportsIoError) {
    // Gap: engine.cpp:195-197 — save_settings without a store returns io_error.
    suborno::core::Engine engine;
    EXPECT_EQ(engine.save_settings(suborno::settings::EngineSettings{}),
              suborno::settings::SettingsStatus::io_error);
}

TEST(EngineCoverage, StorelessEngineLoadSettingsReturnsInMemoryDefaults) {
    // Gap: engine.cpp:209-215 — load_settings without a store mirrors the in-memory settings.
    suborno::core::Engine engine;
    const suborno::settings::SettingsLoadResult loaded = engine.load_settings();
    EXPECT_EQ(loaded.status, suborno::settings::SettingsStatus::ok);
    EXPECT_EQ(loaded.settings.active_layout_id, "suborno-phonetic");
    EXPECT_EQ(loaded.settings.candidate_limit, 16U);
    EXPECT_TRUE(loaded.settings.learning_enabled);
    EXPECT_TRUE(loaded.settings.general.remember_last_input_layout);
    EXPECT_FALSE(loaded.settings.ui.next_word_prediction_enabled);
    EXPECT_EQ(loaded.settings.input.preferred_encoding, suborno::settings::TextEncoding::unicode);
    // load_settings() is const: the engine's own view is untouched.
    EXPECT_EQ(engine.settings().candidate_limit, 16U);
}

TEST(EngineCoverage, StorelessEngineReloadSettingsKeepsInMemoryState) {
    // Gap: engine.cpp:224-238 — reload without a store keeps the in-memory state (234-236 skipped).
    suborno::core::Engine engine;
    engine.set_next_word_prediction_enabled(true);

    const suborno::settings::SettingsLoadResult reloaded = engine.reload_settings();
    EXPECT_EQ(reloaded.status, suborno::settings::SettingsStatus::ok);
    EXPECT_EQ(reloaded.settings.active_layout_id, "suborno-phonetic");
    EXPECT_TRUE(reloaded.settings.ui.next_word_prediction_enabled);
    EXPECT_EQ(engine.active_layout_id(), "suborno-phonetic");
    EXPECT_TRUE(engine.settings().ui.next_word_prediction_enabled);
}

TEST(EngineCoverage, StorelessEngineReloadIfChangedReturnsInMemoryState) {
    // Gap: engine.cpp:240-254 — the no-store branch of reload_settings_if_changed (241-246).
    suborno::core::Engine engine;
    engine.set_next_word_prediction_enabled(true);

    const suborno::settings::SettingsLoadResult result = engine.reload_settings_if_changed();
    EXPECT_EQ(result.status, suborno::settings::SettingsStatus::ok);
    EXPECT_EQ(result.settings.active_layout_id, "suborno-phonetic");
    EXPECT_EQ(result.settings.candidate_limit, 16U);
    EXPECT_TRUE(result.settings.ui.next_word_prediction_enabled);
}

TEST(EngineCoverage, StorelessEngineDictionaryAccessorsReturnNull) {
    // Gap: engine.cpp:261-267 — both dictionary() overloads without a configured dictionary.
    suborno::core::Engine engine;
    EXPECT_EQ(engine.dictionary(), nullptr);
    const suborno::core::Engine& const_view = engine;
    EXPECT_EQ(const_view.dictionary(), nullptr);
}

TEST(EngineCoverage, StorelessEngineClearLearntWordsReportsFalseAndForgetsContext) {
    // Gap: engine.cpp:183-185 — clear_learnt_words without a dictionary still forgets the context.
    suborno::core::Engine engine;
    engine.record_committed_word("bangla");
    EXPECT_EQ(engine.last_committed_word(), "bangla");

    EXPECT_FALSE(engine.clear_learnt_words());
    EXPECT_TRUE(engine.last_committed_word().empty());
}

TEST(EngineCoverage, StorelessEnginePredictionToggleStaysInMemory) {
    // Gap: engine.cpp:151-155 — the no-store early return of set_next_word_prediction_enabled.
    suborno::core::Engine engine;
    EXPECT_FALSE(engine.settings().ui.next_word_prediction_enabled);

    engine.set_next_word_prediction_enabled(true);
    EXPECT_TRUE(engine.settings().ui.next_word_prediction_enabled);
    engine.set_next_word_prediction_enabled(false);
    EXPECT_FALSE(engine.settings().ui.next_word_prediction_enabled);
}

TEST(EngineCoverage, ConstructorRestoresRememberedLayoutFromSettingsFile) {
    // Gap: engine.cpp:53-63 (true branches 57/62) — remembered layout restored from disk.
    const std::filesystem::path settings_path =
        coverage_path("suborno_engine_coverage_restore.conf");
    remove_file(settings_path);
    suborno::settings::EngineSettings stored;
    stored.active_layout_id = "khipro";
    stored.candidate_limit = 12;
    write_settings_file(settings_path, stored);

    suborno::core::Engine engine(config_with(settings_path));
    EXPECT_EQ(engine.active_layout_id(), "khipro");
    EXPECT_EQ(engine.settings().active_layout_id, "khipro");
    EXPECT_EQ(engine.settings().candidate_limit, 12U);
    EXPECT_TRUE(engine.settings().general.remember_last_input_layout);

    // The restored layout really drives new sessions (khipro: "kt" -> "ক্ত").
    auto session = engine.create_session();
    ASSERT_NE(session, nullptr);
    ASSERT_EQ(session->append_utf8("kt"), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "ক্ত");

    remove_file(settings_path);
}

TEST(EngineCoverage, ConstructorIgnoresStoredLayoutWhenRememberDisabled) {
    // Gap: engine.cpp:58 — remember_last_input_layout=false forces the default layout.
    const std::filesystem::path settings_path =
        coverage_path("suborno_engine_coverage_remember_off.conf");
    remove_file(settings_path);
    suborno::settings::EngineSettings stored;
    stored.active_layout_id = "probhat";
    stored.general.remember_last_input_layout = false;
    write_settings_file(settings_path, stored);

    suborno::core::Engine engine(config_with(settings_path));
    EXPECT_EQ(engine.active_layout_id(), "suborno-phonetic");
    EXPECT_EQ(engine.settings().active_layout_id, "suborno-phonetic");
    EXPECT_FALSE(engine.settings().general.remember_last_input_layout);

    remove_file(settings_path);
}

TEST(EngineCoverage, ConstructorCorrectsUnknownRememberedLayout) {
    // Gap: engine.cpp:59-61 — a failed set_active corrects the stored layout id to the default.
    const std::filesystem::path settings_path =
        coverage_path("suborno_engine_coverage_unknown_layout.conf");
    remove_file(settings_path);
    suborno::settings::EngineSettings stored;
    stored.active_layout_id = "no-such-layout";
    write_settings_file(settings_path, stored);

    suborno::core::Engine engine(config_with(settings_path));
    EXPECT_EQ(engine.active_layout_id(), "suborno-phonetic");
    EXPECT_EQ(engine.settings().active_layout_id, "suborno-phonetic");

    remove_file(settings_path);
}

TEST(EngineCoverage, ConstructorFallsBackToDefaultsOnCorruptSettingsFile) {
    // Gap: engine.cpp:52-54 (else branch) — a rejected file leaves factory defaults in place.
    const std::filesystem::path settings_path =
        coverage_path("suborno_engine_coverage_corrupt.conf");
    remove_file(settings_path);
    {
        std::ofstream corrupt(settings_path, std::ios::binary);
        corrupt << "garbage\n";
    }

    suborno::core::Engine engine(config_with(settings_path));
    EXPECT_EQ(engine.active_layout_id(), "suborno-phonetic");
    EXPECT_EQ(engine.settings().candidate_limit, 16U);
    EXPECT_TRUE(engine.settings().learning_enabled);
    // load_settings() surfaces the parse failure while the engine stays usable.
    const suborno::settings::SettingsLoadResult loaded = engine.load_settings();
    EXPECT_EQ(loaded.status, suborno::settings::SettingsStatus::invalid_format);
    EXPECT_TRUE(loaded.file_existed);

    remove_file(settings_path);
}

TEST(EngineCoverage, ConstructorOpensDictionaryAtPath) {
    // Gap: engine.cpp:66-70 — a dictionary_path installs a live dictionary.
    const std::filesystem::path dictionary_path =
        coverage_path("suborno_engine_coverage_dictionary_open.sqlite");
    remove_file(dictionary_path);

    suborno::core::Engine engine(config_with({}, dictionary_path));
    ASSERT_NE(engine.dictionary(), nullptr);
    const suborno::core::Engine& const_view = engine;
    EXPECT_EQ(const_view.dictionary(), engine.dictionary());

    remove_file(dictionary_path);
}

TEST(EngineCoverage, RecordCommittedWordChainsTransitionsAndIgnoresEmptyWords) {
    // Gap: engine.cpp:162-177 — the transition chain built by record_committed_word.
    const std::filesystem::path dictionary_path =
        coverage_path("suborno_engine_coverage_transitions.sqlite");
    remove_file(dictionary_path);

    suborno::core::Engine engine(config_with({}, dictionary_path));
    ASSERT_NE(engine.dictionary(), nullptr);

    // The first word has no predecessor: tracked, no transition row yet.
    engine.record_committed_word("bangla");
    EXPECT_EQ(engine.last_committed_word(), "bangla");
    EXPECT_EQ(engine.dictionary()->stats().word_transitions, 0U);

    // The second word chains the bigram bangla -> kriti.
    engine.record_committed_word("kriti");
    EXPECT_EQ(engine.last_committed_word(), "kriti");
    EXPECT_EQ(engine.dictionary()->stats().word_transitions, 1U);
    EXPECT_EQ(engine.dictionary()->transition_candidates("bangla", "", 10),
              std::vector<std::string>{"kriti"});

    // Empty words carry no context: ignored, the chain stays untouched.
    engine.record_committed_word("");
    EXPECT_EQ(engine.last_committed_word(), "kriti");
    EXPECT_EQ(engine.dictionary()->stats().word_transitions, 1U);

    remove_file(dictionary_path);
}

TEST(EngineCoverage, RecordCommittedWordRecordsRepeatedWordBigram) {
    // Gap: engine.cpp:168-171 — repeating the same word is a real bigram ("হ্যাঁ হ্যাঁ").
    const std::filesystem::path dictionary_path =
        coverage_path("suborno_engine_coverage_repeat_transition.sqlite");
    remove_file(dictionary_path);

    suborno::core::Engine engine(config_with({}, dictionary_path));
    ASSERT_NE(engine.dictionary(), nullptr);

    engine.record_committed_word("হ্যাঁ");
    EXPECT_EQ(engine.dictionary()->stats().word_transitions, 0U);

    engine.record_committed_word("হ্যাঁ");
    EXPECT_EQ(engine.last_committed_word(), "হ্যাঁ");
    EXPECT_EQ(engine.dictionary()->stats().word_transitions, 1U);
    EXPECT_EQ(engine.dictionary()->transition_candidates("হ্যাঁ", "", 10),
              std::vector<std::string>{"হ্যাঁ"});

    remove_file(dictionary_path);
}

TEST(EngineCoverage, LearningDisabledTracksLastWordWithoutTransitions) {
    // Gap: engine.cpp:172-173 — the learning switch gates transitions but not tracking.
    const std::filesystem::path settings_path =
        coverage_path("suborno_engine_coverage_learning_off.conf");
    const std::filesystem::path dictionary_path =
        coverage_path("suborno_engine_coverage_learning_off.sqlite");
    remove_file(settings_path);
    remove_file(dictionary_path);
    suborno::settings::EngineSettings stored;
    stored.learning_enabled = false;
    write_settings_file(settings_path, stored);

    suborno::core::Engine engine(config_with(settings_path, dictionary_path));
    ASSERT_NE(engine.dictionary(), nullptr);
    ASSERT_FALSE(engine.settings().learning_enabled);

    engine.record_committed_word("bangla");
    engine.record_committed_word("kriti");
    EXPECT_EQ(engine.last_committed_word(), "kriti");
    EXPECT_EQ(engine.dictionary()->stats().word_transitions, 0U);
    EXPECT_TRUE(engine.dictionary()->transition_candidates("bangla", "", 10).empty());

    remove_file(settings_path);
    remove_file(dictionary_path);
}

TEST(EngineCoverage, ClearLearntWordsClearsDictionaryAndContext) {
    // Gap: engine.cpp:179-188 — full clear with a dictionary: transitions wiped, system kept.
    const std::filesystem::path dictionary_path =
        coverage_path("suborno_engine_coverage_clear_learnt.sqlite");
    remove_file(dictionary_path);

    suborno::core::Engine engine(config_with({}, dictionary_path));
    ASSERT_NE(engine.dictionary(), nullptr);
    engine.dictionary()->add_entry("ক্ষতি", "kriti", 5);
    engine.record_committed_word("bangla");
    engine.record_committed_word("kriti");
    ASSERT_EQ(engine.dictionary()->stats().word_transitions, 1U);

    EXPECT_TRUE(engine.clear_learnt_words());
    EXPECT_TRUE(engine.last_committed_word().empty());
    EXPECT_EQ(engine.dictionary()->stats().word_transitions, 0U);
    EXPECT_TRUE(engine.dictionary()->transition_candidates("bangla", "", 10).empty());
    // System entries and settings survive the clear.
    EXPECT_EQ(engine.dictionary()->find_matches("kriti"), std::vector<std::string>{"ক্ষতি"});
    EXPECT_TRUE(engine.settings().learning_enabled);

    remove_file(dictionary_path);
}

TEST(EngineCoverage, CreateSessionCarriesSettingsOptionsForBothPredictionStates) {
    // Gap: engine.cpp:107-112 — InputSessionOptions construction with the flag off and on.
    suborno::core::Engine engine;

    auto session = engine.create_session();
    ASSERT_NE(session, nullptr);
    ASSERT_EQ(session->append_utf8("ami"), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "আমি");

    engine.set_next_word_prediction_enabled(true);
    auto prediction_session = engine.create_session();
    ASSERT_NE(prediction_session, nullptr);
    ASSERT_EQ(prediction_session->append_utf8("ami"), suborno::layout::InputStatus::updated);
    EXPECT_EQ(prediction_session->snapshot().preedit_utf8, "আমি");
}

TEST(EngineCoverage, CreateSessionForLayoutUsesRequestedLayoutAndFallsBackForUnknown) {
    // Gap: engine.cpp:117-128 — known ids type with the requested layout; unknown ids fall back.
    suborno::core::Engine engine;
    ASSERT_TRUE(engine.set_active_layout("probhat"));

    // Known id: the khipro session is compositional ("kt" -> "ক্ত"), unlike the
    // active fixed probhat layout.
    auto khipro_session = engine.create_session_for_layout("khipro");
    ASSERT_NE(khipro_session, nullptr);
    ASSERT_EQ(khipro_session->append_utf8("kt"), suborno::layout::InputStatus::updated);
    EXPECT_EQ(khipro_session->snapshot().preedit_utf8, "ক্ত");

    // Unknown id (settings written by another build): falls back to the active
    // layout — identical behavior to a session for "probhat" itself.
    auto fallback_session = engine.create_session_for_layout("no-such-layout");
    auto direct_session = engine.create_session_for_layout("probhat");
    ASSERT_NE(fallback_session, nullptr);
    ASSERT_NE(direct_session, nullptr);
    ASSERT_EQ(fallback_session->append_utf8("ami"), suborno::layout::InputStatus::updated);
    ASSERT_EQ(direct_session->append_utf8("ami"), suborno::layout::InputStatus::updated);
    EXPECT_EQ(fallback_session->snapshot().preedit_utf8, direct_session->snapshot().preedit_utf8);
    EXPECT_EQ(fallback_session->commit().text_utf8, direct_session->commit().text_utf8);
}

TEST(EngineCoverage, CreateSessionForLayoutHonorsConfiguredCandidateLimit) {
    // Gap: engine.cpp:123-128 — the session options carry the configured candidate limit.
    const std::filesystem::path settings_path =
        coverage_path("suborno_engine_coverage_candidate_limit.conf");
    remove_file(settings_path);
    suborno::settings::EngineSettings stored;
    stored.candidate_limit = 3;
    write_settings_file(settings_path, stored);

    suborno::core::Engine engine(config_with(settings_path));
    ASSERT_TRUE(engine.register_layout("five-candidates", std::make_unique<FiveCandidateLayout>()));
    ASSERT_TRUE(engine.set_active_layout("five-candidates"));

    auto session = engine.create_session_for_layout("five-candidates");
    ASSERT_NE(session, nullptr);
    ASSERT_EQ(session->append_utf8("x"), suborno::layout::InputStatus::updated);

    const suborno::layout::CompositionSnapshot snapshot = session->snapshot();
    ASSERT_EQ(snapshot.candidates.size(), 3U);
    EXPECT_EQ(snapshot.candidates[0].text, "x-1");
    EXPECT_EQ(snapshot.candidates[1].text, "x-2");
    EXPECT_EQ(snapshot.candidates[2].text, "x-3");

    remove_file(settings_path);
}

TEST(EngineCoverage, EncodeTextForOutputFollowsPreferredEncoding) {
    // Gap: engine.cpp:131-141 — unicode passthrough, Bijoy conversion, unencodable fallback.
    suborno::core::Engine engine;
    EXPECT_EQ(engine.encode_text_for_output("আমি"), "আমি");
    EXPECT_EQ(engine.encode_text_for_output("plain ASCII"), "plain ASCII");

    const std::filesystem::path settings_path =
        coverage_path("suborno_engine_coverage_encoding.conf");
    remove_file(settings_path);
    suborno::core::Engine ansi_engine(config_with(settings_path));
    suborno::settings::EngineSettings ansi;
    ansi.input.preferred_encoding = suborno::settings::TextEncoding::ansi;
    ASSERT_EQ(ansi_engine.save_settings(ansi), suborno::settings::SettingsStatus::ok);

    // Bijoy-encodable Bangla comes back in legacy ASCII bytes.
    EXPECT_EQ(ansi_engine.encode_text_for_output("আমি"), suborno::unicode::encode_bijoy("আমি"));
    EXPECT_EQ(ansi_engine.encode_text_for_output("আমি"), "Ami");
    EXPECT_EQ(ansi_engine.encode_text_for_output("বাংলা"), "cv'lv");
    // Non-mappable code points keep the input unchanged (nothing is dropped).
    EXPECT_EQ(ansi_engine.encode_text_for_output("caf\xC3\xA9"), "caf\xC3\xA9");

    remove_file(settings_path);
}

TEST(EngineCoverage, LayoutPreviewReflectsTheActiveLayout) {
    // Gap: engine.cpp:143-149 — preview flows through the active layout; empty letter is empty.
    suborno::core::Engine engine;
    const suborno::layouts::SubornoPhoneticLayout layout;
    EXPECT_EQ(engine.layout_preview("k"), layout.transliterate("k"));
    EXPECT_FALSE(engine.layout_preview("k").empty());
    EXPECT_TRUE(engine.layout_preview("").empty());

    // Switching the active layout switches the preview source.
    ASSERT_TRUE(engine.set_active_layout("english"));
    EXPECT_EQ(engine.layout_preview("k"), "k");
}

TEST(EngineCoverage, SaveSettingsRoundTripPersistsAndValidates) {
    // Gap: engine.cpp:194-207 — the ok path (201-206), unknown layout (198-200) and
    // store-level validation failure (201-206) all leave consistent state.
    const std::filesystem::path settings_path =
        coverage_path("suborno_engine_coverage_save_roundtrip.conf");
    remove_file(settings_path);
    suborno::core::Engine engine(config_with(settings_path));

    suborno::settings::EngineSettings settings;
    settings.active_layout_id = "english";
    settings.candidate_limit = 21;
    settings.ui.theme_id = "suborno-light";
    settings.ui.next_word_prediction_enabled = true;
    EXPECT_EQ(engine.save_settings(settings), suborno::settings::SettingsStatus::ok);
    EXPECT_EQ(engine.active_layout_id(), "english");
    EXPECT_EQ(engine.settings().candidate_limit, 21U);
    EXPECT_TRUE(engine.settings().ui.next_word_prediction_enabled);

    // A fresh engine reads the whole persisted struct back.
    suborno::core::Engine reopened(config_with(settings_path));
    EXPECT_EQ(reopened.active_layout_id(), "english");
    EXPECT_EQ(reopened.settings().candidate_limit, 21U);
    EXPECT_TRUE(reopened.settings().ui.next_word_prediction_enabled);
    EXPECT_EQ(reopened.settings().ui.theme_id, "suborno-light");

    // Unknown layout id: engine-level invalid_format, nothing applied.
    suborno::settings::EngineSettings unknown = settings;
    unknown.active_layout_id = "no-such-layout";
    EXPECT_EQ(engine.save_settings(unknown), suborno::settings::SettingsStatus::invalid_format);
    EXPECT_EQ(engine.active_layout_id(), "english");
    EXPECT_EQ(engine.settings().candidate_limit, 21U);

    // Store-level validation failure (limit 0) behaves the same.
    suborno::settings::EngineSettings zero_limit = settings;
    zero_limit.candidate_limit = 0;
    EXPECT_EQ(engine.save_settings(zero_limit), suborno::settings::SettingsStatus::invalid_format);
    EXPECT_EQ(engine.settings().candidate_limit, 21U);
    EXPECT_EQ(engine.active_layout_id(), "english");

    remove_file(settings_path);
}

TEST(EngineCoverage, LoadSettingsFromStoreReturnsPersistedValuesAndCorrectsUnknownLayout) {
    // Gap: engine.cpp:216-221 — store-backed load; an unknown disk layout is corrected
    // to the engine's active layout without touching engine state.
    const std::filesystem::path settings_path =
        coverage_path("suborno_engine_coverage_load_correct.conf");
    remove_file(settings_path);
    suborno::core::Engine engine(config_with(settings_path));

    suborno::settings::EngineSettings settings;
    settings.active_layout_id = "probhat";
    settings.candidate_limit = 33;
    ASSERT_EQ(engine.save_settings(settings), suborno::settings::SettingsStatus::ok);

    const suborno::settings::SettingsLoadResult loaded = engine.load_settings();
    EXPECT_EQ(loaded.status, suborno::settings::SettingsStatus::ok);
    EXPECT_TRUE(loaded.file_existed);
    EXPECT_EQ(loaded.settings.active_layout_id, "probhat");
    EXPECT_EQ(loaded.settings.candidate_limit, 33U);

    // A stale file naming an unknown layout loads fine but reports the engine's
    // active layout instead, while every other field is preserved.
    suborno::settings::EngineSettings stale = settings;
    stale.active_layout_id = "no-such-layout";
    ASSERT_EQ(suborno::settings::SettingsStore(settings_path).save(stale),
              suborno::settings::SettingsStatus::ok);
    const suborno::settings::SettingsLoadResult corrected = engine.load_settings();
    EXPECT_EQ(corrected.status, suborno::settings::SettingsStatus::ok);
    EXPECT_EQ(corrected.settings.active_layout_id, "probhat");
    EXPECT_EQ(corrected.settings.candidate_limit, 33U);
    EXPECT_EQ(engine.settings().active_layout_id, "probhat");

    remove_file(settings_path);
}

TEST(EngineCoverage, ReloadSettingsPicksUpExternalChangesAndCorrectsUnknownLayout) {
    // Gap: engine.cpp:224-238 — external reload applies values (230-237); an unknown
    // id on disk is corrected to the engine's active layout (230-232) and the file
    // stamp is refreshed (234-235).
    const std::filesystem::path settings_path =
        coverage_path("suborno_engine_coverage_reload_external.conf");
    remove_file(settings_path);
    suborno::core::Engine engine(config_with(settings_path));
    ASSERT_TRUE(engine.set_active_layout("probhat"));

    suborno::settings::EngineSettings external;
    external.active_layout_id = "khipro";
    external.candidate_limit = 44;
    ASSERT_EQ(suborno::settings::SettingsStore(settings_path).save(external),
              suborno::settings::SettingsStatus::ok);

    const suborno::settings::SettingsLoadResult reloaded = engine.reload_settings();
    EXPECT_EQ(reloaded.status, suborno::settings::SettingsStatus::ok);
    EXPECT_EQ(engine.active_layout_id(), "khipro");
    EXPECT_EQ(engine.settings().active_layout_id, "khipro");
    EXPECT_EQ(engine.settings().candidate_limit, 44U);

    // Unknown id on disk: reload succeeds but reports the engine's active layout.
    suborno::settings::EngineSettings stale;
    stale.active_layout_id = "no-such-layout";
    ASSERT_EQ(suborno::settings::SettingsStore(settings_path).save(stale),
              suborno::settings::SettingsStatus::ok);
    const suborno::settings::SettingsLoadResult corrected = engine.reload_settings();
    EXPECT_EQ(corrected.status, suborno::settings::SettingsStatus::ok);
    EXPECT_EQ(corrected.settings.active_layout_id, "khipro");
    EXPECT_EQ(engine.settings().active_layout_id, "khipro");
    EXPECT_EQ(engine.active_layout_id(), "khipro");

    remove_file(settings_path);
}

TEST(EngineCoverage, ReloadSettingsKeepsStateWhenFileTurnsInvalid) {
    // Gap: engine.cpp:226-228 — a failed reload leaves settings and active layout untouched.
    const std::filesystem::path settings_path =
        coverage_path("suborno_engine_coverage_reload_invalid.conf");
    remove_file(settings_path);
    suborno::core::Engine engine(config_with(settings_path));

    suborno::settings::EngineSettings settings;
    settings.active_layout_id = "probhat";
    settings.candidate_limit = 9;
    ASSERT_EQ(engine.save_settings(settings), suborno::settings::SettingsStatus::ok);

    {
        std::ofstream broken(settings_path, std::ios::binary | std::ios::trunc);
        broken << "version=99\n";
    }
    const suborno::settings::SettingsLoadResult reloaded = engine.reload_settings();
    EXPECT_EQ(reloaded.status, suborno::settings::SettingsStatus::unsupported_version);
    EXPECT_EQ(engine.active_layout_id(), "probhat");
    EXPECT_EQ(engine.settings().active_layout_id, "probhat");
    EXPECT_EQ(engine.settings().candidate_limit, 9U);

    remove_file(settings_path);
}

TEST(EngineCoverage, ReloadSettingsIfChangedReloadsOnlyWhenFileChanged) {
    // Gap: engine.cpp:240-254 — the unchanged fast path (247-253) and the changed
    // reload path (254) both return the current settings with ok.
    const std::filesystem::path settings_path =
        coverage_path("suborno_engine_coverage_reload_if_changed.conf");
    remove_file(settings_path);
    suborno::core::Engine engine(config_with(settings_path));

    // No file on disk yet: the missing-file stamp matches the constructor state,
    // so nothing is reloaded.
    const suborno::settings::SettingsLoadResult first = engine.reload_settings_if_changed();
    EXPECT_EQ(first.status, suborno::settings::SettingsStatus::ok);
    EXPECT_EQ(first.settings.candidate_limit, 16U);
    EXPECT_EQ(engine.active_layout_id(), "suborno-phonetic");

    suborno::settings::EngineSettings external;
    external.active_layout_id = "probhat";
    external.candidate_limit = 5;
    ASSERT_EQ(suborno::settings::SettingsStore(settings_path).save(external),
              suborno::settings::SettingsStatus::ok);

    // The file changed (did not exist before): the change is picked up.
    const suborno::settings::SettingsLoadResult second = engine.reload_settings_if_changed();
    EXPECT_EQ(second.status, suborno::settings::SettingsStatus::ok);
    EXPECT_EQ(engine.active_layout_id(), "probhat");
    EXPECT_EQ(engine.settings().candidate_limit, 5U);

    // The reload refreshed the engine's stamp: calling again is a fast path that
    // reports the current settings without another reload.
    const suborno::settings::SettingsLoadResult third = engine.reload_settings_if_changed();
    EXPECT_EQ(third.status, suborno::settings::SettingsStatus::ok);
    EXPECT_EQ(third.settings.active_layout_id, "probhat");
    EXPECT_EQ(third.settings.candidate_limit, 5U);
    EXPECT_EQ(engine.settings().candidate_limit, 5U);

    remove_file(settings_path);
}

TEST(EngineCoverage, RegisterLayoutAcceptsNewIdsAndRejectsDuplicatesAndInvalid) {
    // Gap: engine.cpp:75-77 — register_layout passthrough with the registry's
    // duplicate/empty/null guards.
    suborno::core::Engine engine;
    EXPECT_TRUE(
        engine.register_layout("my-english", std::make_unique<suborno::layouts::EnglishLayout>()));
    EXPECT_TRUE(engine.set_active_layout("my-english"));
    EXPECT_EQ(engine.active_layout_id(), "my-english");

    auto session = engine.create_session();
    ASSERT_NE(session, nullptr);
    ASSERT_EQ(session->append_utf8("abc"), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->commit().text_utf8, "abc");

    EXPECT_FALSE(
        engine.register_layout("my-english", std::make_unique<suborno::layouts::EnglishLayout>()));
    EXPECT_FALSE(engine.register_layout("", std::make_unique<suborno::layouts::EnglishLayout>()));
    EXPECT_FALSE(engine.register_layout("null-layout", nullptr));
    EXPECT_EQ(engine.active_layout_id(), "my-english");
}
