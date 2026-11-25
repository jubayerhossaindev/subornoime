#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>

namespace suborno::settings {

enum class StartMode : std::uint8_t {
    last_state,
    topbar,
    system_tray,
    main_window,
};

enum class SwitchingMode : std::uint8_t {
    per_application_window,
    global,
};

enum class CandidateOrientation : std::uint8_t {
    vertical,
    horizontal,
};

enum class TextEncoding : std::uint8_t {
    unicode,
    ansi,
};

struct GeneralSettings {
    bool remember_last_input_layout = true;
    bool start_with_windows = false;
    StartMode start_mode = StartMode::last_state;
    SwitchingMode switching_mode = SwitchingMode::per_application_window;
    /// The UI surface that was active last (topbar | tray | window). Written
    /// by the desktop shell whenever visibility changes and honored by the
    /// last-state start mode so restarts feel uninterrupted.
    std::string last_ui_mode = "topbar";
    /// The Bangla layout to return to when the user toggles English mode off
    /// (topbar mode icon / tray / OSK). Kept in sync by the shell whenever a
    /// Bangla layout becomes active; survives restarts.
    std::string last_bangla_layout_id = "suborno-phonetic";
};

struct InterfaceSettings {
    std::string theme_id = "suborno-dark";
    /// UI language for the desktop shell: "system" follows the Windows
    /// display language (Bangla UI when Windows itself is in Bangla), or an
    /// explicit "en"/"bn" selecting a data/i18n locale. Empty/unknown values
    /// never occur — load() validates them — and the English literals at
    /// every call site remain the permanent fallback for missing strings.
    std::string language = "system";
    /// Preferred UI/candidate font family; empty string = OS default. The
    /// desktop shell and the candidate window both apply it (when
    /// use_os_default_bangla_font is false).
    std::string font_family;
    bool show_candidate_window = true;
    bool next_word_prediction_enabled = false;
    CandidateOrientation candidate_orientation = CandidateOrientation::vertical;
    bool candidate_fixed_height = true;
    bool use_os_default_bangla_font = true;
    bool enable_sliding_effect = true;
    bool follow_caret = true;
    bool follow_mouse_pointer_if_caret_tracking_fails = true;
};

struct TopbarSettings {
    bool idle_dimming_enabled = true;
    std::uint32_t idle_opacity_percent = 50;
    std::uint32_t idle_time_seconds = 10;
    bool remember_position = true;
    /// Last dragged topbar position in pixels; -1 means "no saved position".
    std::int32_t position_x = -1;
    std::int32_t position_y = -1;
};

struct InputSettings {
    std::string layout_switch_shortcut = "F10";
    bool disable_encoding_shortcut = false;
    bool map_numpad_to_bangla_numbers = true;
    bool change_locale_to_bn_bd_when_layout_not_english = true;
    bool ignore_caps_lock_state_in_bangla = false;
    bool select_candidate_using_number_keys = true;
    TextEncoding preferred_encoding = TextEncoding::unicode;
};

struct EngineSettings {
    std::string active_layout_id = "suborno-phonetic";
    std::size_t candidate_limit = 16;
    bool learning_enabled = true;
    /// BornoTSF compatibility switches (src/layouts/borno_compat):
    /// phonetic_correction toggles the ac.json autocorrect/pass-through layer
    /// for the Borno/Avro phonetic layouts (BornoTSF registry value
    /// PhoneticCorrection; default enabled, exactly like BornoTSF).
    bool phonetic_correction = true;
    /// BornoTSF registry value TypingStyle ("normal" | "traditional"). The
    /// BornoTSF regex layouts used it to pick the matra-before/matra-after
    /// rule branch; Suborno's keystroke-level National/Probhat engines accept
    /// both matra orders natively, so the value is stored for configuration
    /// parity and surfaced in the settings UI.
    std::string typing_style = "normal";
    GeneralSettings general;
    /// Named `ui`, not `interface`: Windows COM headers `#define interface
    /// struct`, which corrupts this declaration the moment objbase.h has been
    /// included (TSF/COM translation units). The settings file format is
    /// unaffected — the `theme_id=`/`show_candidate_window=` keys below the
    /// `interface` group are unchanged.
    InterfaceSettings ui;
    TopbarSettings topbar;
    InputSettings input;
};

enum class SettingsStatus : std::uint8_t {
    ok,
    io_error,
    invalid_format,
    unsupported_version,
};

struct SettingsLoadResult {
    EngineSettings settings;
    SettingsStatus status = SettingsStatus::ok;
    std::string error;
    /// False when load() produced defaults because the settings file did not
    /// exist yet (first launch). Lets callers persist the defaults once so the
    /// file exists for subsequent launches, without treating a first run as an
    /// error. True when the file existed — including the io_error case where it
    /// exists but could not be opened.
    bool file_existed = false;
};

class SettingsStore {
  public:
    explicit SettingsStore(std::filesystem::path path);

    [[nodiscard]] SettingsLoadResult load() const;
    [[nodiscard]] SettingsStatus save(const EngineSettings& settings) const;

    /// Cheap change detection for callers that poll (e.g. focus changes):
    /// identity of the underlying file as (size, modification time).
    struct FileStamp {
        bool exists = false;
        std::uintmax_t size = 0;
        std::int64_t mtime_ticks = 0;
        bool operator==(const FileStamp&) const = default;
    };
    [[nodiscard]] FileStamp file_stamp() const;

  private:
    std::filesystem::path path_;
};

}  // namespace suborno::settings
