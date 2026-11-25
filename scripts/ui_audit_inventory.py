#!/usr/bin/env python3
"""UI/UX Functional Audit Phase 2 — inventory + traceability matrix builder.

Emits two CSVs into the repo's docs/reports/ directory:
  1. UI_FUNCTIONAL_AUDIT_INVENTORY_V2.csv      (Phase 02: every interactive
     element of every SubornoIME Windows surface, with handler/settings-key/
     UIA trace and the portable verification that already executed)
  2. UI_FUNCTIONAL_AUDIT_TRACEABILITY_MATRIX_V2.csv (Phase 10: element ->
     expected behavior -> test case -> execution kind -> evidence -> result)

Status vocabulary (mission-mandated):
  PASS        — actual behavior verified with executed evidence
  FAIL        — actual behavior differs from expected (defect, with finding id)
  BLOCKED     — could not execute in this environment (Linux sandbox; the
                Windows/MSVC runtime is unavailable), reason recorded
  NOT TESTED  — no execution performed
Row field `verification` distinguishes linux-unit-test / static-source-trace /
windows-runtime (the last is always BLOCKED here).
"""
from __future__ import annotations

import csv
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
OUT = REPO / "docs" / "reports"

APP = "src/platform/windows/app/application.cpp"
TOP = "src/platform/windows/app/topbar.cpp"
OSK = "src/platform/windows/app/osk_window.cpp"
TST = "src/platform/windows/app/toast_window.cpp"
MP = "src/platform/windows/app/menu_popup.cpp"
UIA = "src/platform/windows/app/ui_accessibility.cpp"
ABOUT = "src/platform/windows/app/application.cpp"
CAND = "src/platform/windows/tsf/candidate_list_ui_element.cpp"
TSF = "src/platform/windows/tsf/text_service.cpp"
SET = "src/settings/settings.cpp"

PASS = "PASS"
FAIL = "FAIL"
BLOCKED = "BLOCKED"
NOT_TESTED = "NOT TESTED"

INV_COLS = [
    "test_id", "surface", "page_or_section", "element_name", "element_type",
    "visible_label_or_i18n_key", "automation_id", "uia_control_type",
    "interaction_patterns", "default_state", "expected_behavior",
    "event_handler", "source_file", "source_line", "settings_key_or_state",
    "persistence_behavior", "keyboard_accessible", "dependencies",
    "portable_verification_test", "runtime_verification_status",
]

TRC_COLS = [
    "trace_id", "element_test_id", "expected_behavior", "test_case",
    "test_kind", "evidence", "result",
]

# ---------------------------------------------------------------------------
# Inventory rows.
# field order mirrors INV_COLS.
# ---------------------------------------------------------------------------
I: list[list[str]] = []


def inv(*cols: str) -> None:
    assert len(cols) == len(INV_COLS), f"{cols[0]}: {len(cols)} != {len(INV_COLS)}"
    I.append(list(cols))


# -- Settings window: chrome ------------------------------------------------
inv("SET-SIDEBAR-GENERAL", "Settings window", "sidebar", "General nav pill", "nav item",
    "settings.sidebar.general", "sidebar.general", "ListItem", "Invoke",
    "General active", "Switches content area to the General page",
    "handle_settings_mouse_down (sidebar branch) / Ctrl+Tab cycle", APP, "1926",
    "in-memory page id", "not persisted (page resets to General)", "yes (Tab; Ctrl+Tab cycles)",
    "page content rebuild",
    "UIAuditSettingsPages.GeneralPageEveryControlKeyRoundTripsThroughSave", BLOCKED)
inv("SET-SIDEBAR-INTERFACE", "Settings window", "sidebar", "Interface nav pill", "nav item",
    "settings.sidebar.interface", "sidebar.interface", "ListItem", "Invoke",
    "inactive", "Switches content area to the Interface page",
    "handle_settings_mouse_down (sidebar branch) / Ctrl+Tab cycle", APP, "1926",
    "in-memory page id", "not persisted", "yes (Tab; Ctrl+Tab cycles)",
    "page content rebuild",
    "UIAuditSettingsPages.InterfacePageEveryControlKeyRoundTripsThroughSave", BLOCKED)
inv("SET-SIDEBAR-TOPBAR", "Settings window", "sidebar", "Topbar nav pill", "nav item",
    "settings.sidebar.topbar", "sidebar.topbar", "ListItem", "Invoke",
    "inactive", "Switches content area to the Topbar page",
    "handle_settings_mouse_down (sidebar branch) / Ctrl+Tab cycle", APP, "1926",
    "in-memory page id", "not persisted", "yes (Tab; Ctrl+Tab cycles)",
    "page content rebuild",
    "UIAuditSettingsPages.TopbarPageSlidersCoverMeaningfulRange", BLOCKED)
inv("SET-SIDEBAR-INPUT", "Settings window", "sidebar", "Input nav pill", "nav item",
    "settings.sidebar.input", "sidebar.input", "ListItem", "Invoke",
    "inactive", "Switches content area to the Input page",
    "handle_settings_mouse_down (sidebar branch) / Ctrl+Tab cycle", APP, "1926",
    "in-memory page id", "not persisted", "yes (Tab; Ctrl+Tab cycles)",
    "page content rebuild",
    "UIAuditSettingsPages.InputPageEveryControlKeyRoundTripsThroughSave", BLOCKED)
inv("SET-FOOT-SAVE", "Settings window", "footer", "Save button", "button",
    "settings.action.save", "action.save", "Button", "Invoke",
    "enabled", "Commits staged settings: startup registry pref, engine save, TSF profile refresh, theme, i18n reload, hotkey re-registration, font refresh, topbar apply, toast, then hides window",
    "save_settings_from_ui", APP, "2989",
    "all staged keys", "full file atomically rewritten", "yes (Tab reaches it; Enter)",
    "every control on every page",
    "UIAuditSaveCancel.SaveIsIdempotentByteForByteWhenNothingChanged; UIAuditE2E.RestartPersistenceEverySavedSettingRestored", BLOCKED)
inv("SET-FOOT-CANCEL", "Settings window", "footer", "Cancel button", "button",
    "settings.action.cancel", "action.cancel", "Button", "Invoke",
    "enabled", "Discards staged changes (re-stage from engine), hides window to tray; must not persist",
    "cancel_settings", APP, "2975",
    "none (staged copy dropped)", "no write", "yes (Tab reaches it; Enter; Esc)",
    "every control on every page",
    "UIAuditSaveCancel.CancelLeavesPersistedStateUntouched", BLOCKED)
inv("SET-CLOSE-X", "Settings window", "title row", "Close (X)", "button",
    "common.close", "action.close", "Button", "Invoke",
    "enabled", "Cancel + hide (equivalent to Cancel when tray present)",
    "handle_settings_mouse_down close_hit -> cancel_settings", APP, "2623",
    "none", "no write", "no (mouse/UIA only)",
    "same as Cancel", "UIAuditSaveCancel.CancelLeavesPersistedStateUntouched (contract)", BLOCKED)

# -- Settings window: General page ------------------------------------------
inv("SET-G-REMEMBER-LAYOUT", "Settings window", "General", "Remember Last Input Layout",
    "checkbox", "settings.general.remember_layout", "action.remember-layout", "CheckBox",
    "Toggle", "checked (default true)", "Toggles staged remember_last_input_layout; Save commits; false forces suborno-phonetic at next start",
    "toggle_staged_checkbox kActionRememberLayout", APP, "2095",
    "remember_last_input_layout", "persisted on Save", "yes (Space)",
    "engine restart behavior",
    "UIAuditSettingsPages.GeneralPageEveryControlKeyRoundTripsThroughSave; UIAuditE2E.FirstLaunchPersistsDefaultsAndEngineStartsOnDefaultLayout", BLOCKED)
inv("SET-G-START-WITH-WINDOWS", "Settings window", "General", "Start with Windows",
    "checkbox", "settings.general.start_with_windows", "action.start-with-windows",
    "CheckBox", "Toggle", "unchecked (default false)",
    "Toggles staged flag; Save writes HKCU Run key 'Suborno' and syncs settings file",
    "toggle_staged_checkbox kActionStartWithWindows; save_settings_from_ui -> write_startup_preference",
    APP, "2101", "start_with_windows", "settings file + HKCU registry Run key on Save",
    "yes (Space)", "registry write on Save",
    "UIAuditSettingsPages.GeneralPageEveryControlKeyRoundTripsThroughSave (file half)", BLOCKED)
inv("SET-G-START-MODE", "Settings window", "General", "When Windows starts, show",
    "dropdown", "settings.start_mode.* (4 options)", "action.start-mode", "ComboBox",
    "ExpandCollapse", "last-state",
    "Each of 4 options (Remember last state/Topbar/System tray/Settings window) selects staged start_mode; Save commits; honored at next launch",
    "open_dropdown kActionStartMode", APP, "2113",
    "start_mode", "persisted on Save", "yes (Space opens; arrows navigate)",
    "none", "UIAuditSettingsPages.GeneralPageEveryControlKeyRoundTripsThroughSave (all 4 options walked)", BLOCKED)
inv("SET-G-SWITCHING-MODE", "Settings window", "General", "Layout switching mode",
    "dropdown", "settings.switching_mode.* (2 options)", "action.switching-mode",
    "ComboBox", "ExpandCollapse", "per-application-window",
    "Per window vs Global selects staged switching_mode; Save commits",
    "open_dropdown kActionSwitchingMode", APP, "2129",
    "switching_mode", "persisted on Save", "yes (Space opens; arrows navigate)",
    "none",
    "UIAuditSettingsPages.GeneralPageEveryControlKeyRoundTripsThroughSave (both options walked)", BLOCKED)
inv("SET-G-DEFAULT-LAYOUT", "Settings window", "General", "Default keyboard layout",
    "dropdown", "kLayouts names (7 options)", "action.default-layout", "ComboBox",
    "ExpandCollapse", "suborno-phonetic",
    "Each of the 7 registered layouts selects staged active_layout_id; Save commits; typing honors it at restart",
    "open_dropdown kActionLayoutDropdown", APP, "2142",
    "active_layout", "persisted on Save", "yes (Space opens; arrows navigate)",
    "engine layout registry",
    "UIAuditSettingsPages.DefaultLayoutDropdownCoversAllSevenRegisteredLayouts (all 7 walked)", BLOCKED)
inv("SET-G-STATUS-LINE", "Settings window", "General", "Windows input status line",
    "status text", "settings.status.*", "(none — painted, not a UiControl)", "Text",
    "(none)", "reflects profile_enabled()", "Read-only indicator of Windows language-profile state",
    "refresh_status (painted directly)", APP, "3408",
    "TSF profile state (runtime)", "not persisted", "no",
    "TSF runtime",
    "(none — Windows-runtime indicator)", BLOCKED)

# -- Settings window: Interface page ----------------------------------------
inv("SET-I-SHOW-CANDIDATE", "Settings window", "Interface", "Show Candidate Window",
    "checkbox", "settings.interface.show_candidate_window", "action.show-candidate-window",
    "CheckBox", "Toggle", "checked (default true)",
    "Toggles staged flag; Save commits; gates TSF candidate window display",
    "toggle_staged_checkbox kActionShowCandidateWindow", APP, "2155",
    "show_candidate_window", "persisted on Save", "yes (Space)",
    "TSF candidate window",
    "UIAuditSettingsPages.InterfacePageEveryControlKeyRoundTripsThroughSave (both states walked)", BLOCKED)
inv("SET-I-NEXT-WORD", "Settings window", "Interface", "Next Word Prediction",
    "checkbox", "settings.interface.next_word_prediction", "action.next-word-prediction",
    "CheckBox", "Toggle", "unchecked (default false)",
    "Toggles staged flag; Save commits; feeds candidate engine prediction",
    "toggle_staged_checkbox kActionNextWordPrediction", APP, "2161",
    "next_word_prediction_enabled", "persisted on Save", "yes (Space)",
    "candidate engine",
    "UIAuditLearning.PredictionTogglePersistsThroughEngine; UIAuditSettingsPages.InterfacePageEveryControlKeyRoundTripsThroughSave", BLOCKED)
inv("SET-I-ORIENTATION", "Settings window", "Interface", "Orientation",
    "dropdown", "settings.candidate_orientation.* (2 options)", "action.candidate-orientation",
    "ComboBox", "ExpandCollapse", "vertical",
    "Vertical list vs Horizontal row selects staged orientation; Save commits; candidate window renders accordingly",
    "open_dropdown kActionOrientation", APP, "2172",
    "candidate_orientation", "persisted on Save", "yes (Space opens)",
    "candidate window layout",
    "UIAuditSettingsPages.InterfacePageEveryControlKeyRoundTripsThroughSave (both options walked)", BLOCKED)
inv("SET-I-FIXED-HEIGHT", "Settings window", "Interface", "Fixed Height (page five at a time)",
    "checkbox", "settings.interface.candidate_fixed_height", "action.fixed-height",
    "CheckBox", "Toggle", "checked (default true)",
    "Toggles staged flag; Save commits; candidate window uses 5-row fixed pages",
    "toggle_staged_checkbox kActionFixedHeight", APP, "2182",
    "candidate_fixed_height", "persisted on Save", "yes (Space)",
    "candidate window paging",
    "UIAuditSettingsPages.InterfacePageEveryControlKeyRoundTripsThroughSave", BLOCKED)
inv("SET-I-SLIDING", "Settings window", "Interface", "Sliding Effect",
    "checkbox", "settings.interface.enable_sliding_effect", "action.sliding-effect",
    "CheckBox", "Toggle", "checked (default true)",
    "Toggles staged flag; Save commits; candidate window slide-in animation on/off",
    "toggle_staged_checkbox kActionSlidingEffect", APP, "2188",
    "enable_sliding_effect", "persisted on Save", "yes (Space)",
    "candidate window animation",
    "UIAuditSettingsPages.InterfacePageEveryControlKeyRoundTripsThroughSave", BLOCKED)
inv("SET-I-FOLLOW-CARET", "Settings window", "Interface", "Follow the Caret",
    "checkbox", "settings.interface.follow_caret", "action.follow-caret", "CheckBox",
    "Toggle", "checked (default true)",
    "Toggles staged flag; Save commits; candidate window placement source",
    "toggle_staged_checkbox kActionFollowCaret", APP, "2198",
    "follow_caret", "persisted on Save", "yes (Space)",
    "candidate window placement",
    "UIAuditSettingsPages.InterfacePageEveryControlKeyRoundTripsThroughSave", BLOCKED)
inv("SET-I-FOLLOW-MOUSE", "Settings window", "Interface",
    "Follow Mouse Pointer if Caret Tracking Fails", "checkbox",
    "settings.interface.follow_mouse_pointer_if_caret_tracking_fails",
    "action.follow-mouse-pointer", "CheckBox", "Toggle", "checked (default true)",
    "Toggles staged flag; Save commits; placement fallback",
    "toggle_staged_checkbox kActionFollowMouse", APP, "2204",
    "follow_mouse_pointer_if_caret_tracking_fails", "persisted on Save", "yes (Space)",
    "candidate window placement", 
    "UIAuditSettingsPages.InterfacePageEveryControlKeyRoundTripsThroughSave", BLOCKED)
inv("SET-I-THEME", "Settings window", "Interface", "Theme",
    "dropdown", "settings.ui_theme.* (3 options)", "action.theme", "ComboBox",
    "ExpandCollapse", "suborno-dark",
    "Suborno Dark / Suborno Light / System (Windows) selects staged theme_id; Save applies palette to every surface; System resolves via registry + WM_THEMECHANGED live",
    "open_dropdown kActionThemeDropdown; save_settings_from_ui -> apply_theme", APP, "2217",
    "theme_id", "persisted on Save", "yes (Space opens)",
    "all surfaces' palettes",
    "UIAuditSettingsPages.InterfacePageEveryControlKeyRoundTripsThroughSave (all 3 walked); scripts/contrast_check.py (22 WCAG pairs, 0 failures)", BLOCKED)
inv("SET-I-LANGUAGE", "Settings window", "Interface", "UI Language",
    "dropdown", "settings.interface.language.* (3 options)", "action.ui-language",
    "ComboBox", "ExpandCollapse", "system",
    "Follow system / English / বাংলা selects staged language; Save reloads i18n tables, refreshes window title + OSK labels, raises language_changed toast",
    "open_dropdown kActionUiLanguage; save_settings_from_ui -> apply_ui_language", APP, "2228",
    "language", "persisted on Save", "yes (Space opens)",
    "every surface's strings",
    "UIAuditI18n.ShippedLocaleFilesCarryEveryInventoryKeyInBothLanguages; UIAuditI18n.LanguageSelectionFollowsPortableLocaleMapping", BLOCKED)
inv("SET-I-USE-OS-FONT", "Settings window", "Interface", "Use OS Default Bangla Font",
    "checkbox", "settings.interface.use_os_default_font", "action.use-os-default-font",
    "CheckBox", "Toggle", "checked (default true)",
    "Checking clears staged font_family (dependency); Save commits; selects OS default font",
    "toggle_staged_checkbox kActionUseOsDefaultFont (clears staged_.ui.font_family)", APP, "2238",
    "use_os_default_bangla_font + font_family", "persisted on Save", "yes (Space)",
    "font dropdown sync",
    "UIAuditSettingsPages.InterfacePageEveryControlKeyRoundTripsThroughSave", BLOCKED)
inv("SET-I-FONT", "Settings window", "Interface", "Font",
    "dropdown", "settings.interface.font_os_default_item + bangla_font_options() (1+16)",
    "action.font", "ComboBox", "ExpandCollapse", "OS Default (Recommended)",
    "Selecting a family sets staged font_family and unchecks OS-default (dependency); pages rebuilt; Save commits and refreshes process font",
    "open_dropdown kActionFontDropdown -> apply_font_choice", APP, "2245",
    "font_family", "persisted on Save", "yes (Space opens)",
    "use_os_default_bangla_font checkbox; topbar/OSK font refresh",
    "UIAuditFontFamily.BengaliScriptFontNamesRoundTrip; UIAuditFontFamily.FontNameSafetyRulesStillHold", BLOCKED)

# -- Settings window: Topbar page -------------------------------------------
inv("SET-T-IDLE-DIMMING", "Settings window", "Topbar", "Enable Idle Dimming",
    "checkbox", "settings.topbar.idle_dimming_enabled", "action.idle-dimming",
    "CheckBox", "Toggle", "checked (default true)",
    "Toggles staged flag; Save commits; topbar dims after idle_time seconds to idle_opacity percent",
    "toggle_staged_checkbox kActionIdleDimming", APP, "2261",
    "idle_dimming_enabled", "persisted on Save", "yes (Space)",
    "topbar idle timer",
    "UIAuditSettingsPages.TopbarPageSlidersCoverMeaningfulRange", BLOCKED)
inv("SET-T-IDLE-OPACITY", "Settings window", "Topbar", "Idle Opacity (%)",
    "slider", "settings.topbar.idle_opacity_percent", "action.idle-opacity", "Slider",
    "RangeValue", "50 (0-100)",
    "Drag/arrows/PgUp-PgDn/Home-End stage 0..100; Save commits; value label updates live",
    "handle_settings_mouse_move slider drag / arrow keys / UIA set_element_value", APP, "2268",
    "idle_opacity_percent", "persisted on Save", "yes (arrows, PgUp/PgDn, Home/End)",
    "idle dimming checkbox",
    "UIAuditSettingsPages.TopbarPageSlidersCoverMeaningfulRange (0/1/50/99/100 walked; 101 rejected)", BLOCKED)
inv("SET-T-IDLE-TIME", "Settings window", "Topbar", "Idle Time (Seconds)",
    "slider", "settings.topbar.idle_time_seconds", "action.idle-time", "Slider",
    "RangeValue", "10 (1-60 on UI)",
    "Drag/keys stage 1..60 (UI cap); model accepts 1..3600; Save commits",
    "handle_settings_mouse_move slider drag / arrow keys / UIA set_element_value", APP, "2279",
    "idle_time_seconds", "persisted on Save", "yes (arrows, PgUp/PgDn, Home/End)",
    "idle dimming checkbox",
    "UIAuditSettingsPages.TopbarPageSlidersCoverMeaningfulRange (1/10/60/3599/3600 walked; 0 and 3601 rejected)", BLOCKED)
inv("SET-T-REMEMBER-POS", "Settings window", "Topbar", "Remember Topbar Position",
    "checkbox", "settings.topbar.remember_position", "action.remember-topbar-position",
    "CheckBox", "Toggle", "checked (default true)",
    "Toggles staged flag; Save commits; topbar drags persist position_x/y when enabled",
    "toggle_staged_checkbox kActionRememberTopbarPosition", APP, "2296",
    "remember_topbar_position + topbar_position_x/y", "persisted on Save", "yes (Space)",
    "topbar drag path",
    "UIAuditSettingsPages.TopbarPageSlidersCoverMeaningfulRange", BLOCKED)

# -- Settings window: Input page --------------------------------------------
inv("SET-IN-NUMPAD", "Settings window", "Input", "Map Numpad to Bangla Numbers",
    "checkbox", "settings.input.map_numpad_to_bangla_numbers", "action.map-numpad",
    "CheckBox", "Toggle", "checked (default true)",
    "Toggles staged flag; Save commits; typing maps numpad digits",
    "toggle_staged_checkbox kActionMapNumpad", APP, "2309",
    "map_numpad_to_bangla_numbers", "persisted on Save", "yes (Space)",
    "typing path", 
    "UIAuditSettingsPages.InputPageEveryControlKeyRoundTripsThroughSave", BLOCKED)
inv("SET-IN-CAPS", "Settings window", "Input", "Ignore Caps Lock state in Bangla layouts",
    "checkbox", "settings.input.ignore_caps_lock_state_in_bangla", "action.ignore-caps-lock",
    "CheckBox", "Toggle", "unchecked (default false)",
    "Toggles staged flag; Save commits",
    "toggle_staged_checkbox kActionIgnoreCapsLock", APP, "2315",
    "ignore_caps_lock_state_in_bangla", "persisted on Save", "yes (Space)",
    "typing path",
    "UIAuditSettingsPages.InputPageEveryControlKeyRoundTripsThroughSave", BLOCKED)
inv("SET-IN-LEARNING", "Settings window", "Input", "Suggestions & Learning",
    "dropdown", "settings.learning.* (2 options)", "action.learning", "ComboBox",
    "ExpandCollapse", "on (learn new words)",
    "Learn new words vs Off selects staged learning_enabled; Save commits; feeds candidate learning",
    "open_dropdown kActionLearning", APP, "2326",
    "learning_enabled", "persisted on Save", "yes (Space opens)",
    "personal dictionary",
    "UIAuditSettingsPages.InputPageEveryControlKeyRoundTripsThroughSave (both options walked); UIAuditLearning.PredictionTogglePersistsThroughEngine", BLOCKED)
inv("SET-IN-CLEAR-LEARNT", "Settings window", "Input", "Clear Learnt Words…",
    "button (immediate, destructive)", "settings.input.clear_learnt", "action.clear-learnt",
    "Button", "Invoke", "enabled",
    "MessageBoxW Yes/No confirm, then engine clear_learnt_words immediately; NOT staged — Cancel cannot undo; success toast",
    "run_button_action kActionClearLearnt -> clear_learnt_words_from_ui", APP, "2333",
    "personal dictionary (SQLite)", "immediate engine operation", "yes (Tab; Space/Enter)",
    "dictionary store",
    "UIAuditLearning.ClearLearntWordsLeavesEngineConsistent", BLOCKED)
inv("SET-IN-NUMBER-KEYS", "Settings window", "Input", "Select candidate using number keys",
    "checkbox", "settings.input.select_candidate_using_number_keys",
    "action.select-candidate-number-keys", "CheckBox", "Toggle", "checked (default true)",
    "Toggles staged flag; Save commits; gates TSF digit selection during composition",
    "toggle_staged_checkbox kActionNumberKeys", APP, "2343",
    "select_candidate_using_number_keys", "persisted on Save", "yes (Space)",
    "TSF candidate keys",
    "UIAuditSettingsPages.InputPageEveryControlKeyRoundTripsThroughSave", BLOCKED)
inv("SET-IN-SHORTCUT", "Settings window", "Input", "Layout Switching Shortcut",
    "dropdown", "kRegisterableShortcuts (F1 F2 F3 F4 F10 F12 Ctrl+Space) + saved reserved",
    "action.layout-shortcut", "ComboBox", "ExpandCollapse", "F10",
    "Selects staged layout_switch_shortcut; Save re-registers hotkey; pages rebuilt",
    "open_dropdown kActionLayoutShortcut; save_settings_from_ui -> register_layout_hotkey",
    APP, "2354", "layout_switch_shortcut", "persisted on Save", "yes (Space opens)",
    "RegisterHotKey (Windows runtime)",
    "UIAuditSettingsPages.ShortcutDropdownEveryRegisterableOptionAccepted (all 9 values + 4 rejects)", BLOCKED)
inv("SET-IN-ENCODING", "Settings window", "Input", "Encoding",
    "dropdown", "settings.preferred_encoding.* (2 options)", "action.preferred-encoding",
    "ComboBox", "ExpandCollapse", "unicode",
    "Unicode (recommended) vs Bijoy ANSI (legacy) selects staged preferred_encoding; Save commits; output encoding flips",
    "open_dropdown kActionEncoding", APP, "2382",
    "preferred_encoding", "persisted on Save", "yes (Space opens)",
    "engine encode path",
    "UIAuditSettingsPages.InputPageEveryControlKeyRoundTripsThroughSave (both walked); UIAuditEncoding.ToggleProducesUnicodeAndBijoyOutput", BLOCKED)
inv("SET-IN-LOCALE-BNBD", "Settings window", "Input",
    "Switch input language to bn-BD for Bangla layouts", "checkbox",
    "settings.input.change_locale_to_bn_bd_when_layout_not_english", "action.change-locale-bn-bd",
    "CheckBox", "Toggle", "checked (default true)",
    "Toggles staged flag; Save commits; TSF switches input locale",
    "toggle_staged_checkbox kActionChangeLocale", APP, "2390",
    "change_locale_to_bn_bd_when_layout_not_english", "persisted on Save", "yes (Space)",
    "TSF locale runtime",
    "UIAuditSettingsPages.InputPageEveryControlKeyRoundTripsThroughSave", BLOCKED)
inv("SET-IN-DISABLE-ENC-SHORTCUT", "Settings window", "Input",
    "Disable the encoding toggle shortcut (Ctrl+Alt+E)", "checkbox",
    "settings.input.disable_encoding_shortcut", "action.disable-encoding-shortcut",
    "CheckBox", "Toggle", "unchecked (default false)",
    "Toggles staged flag; Save re-registers or skips the Ctrl+Alt+E hotkey",
    "toggle_staged_checkbox kActionDisableEncodingShortcut", APP, "2397",
    "disable_encoding_shortcut", "persisted on Save", "yes (Space)",
    "Ctrl+Alt+E hotkey registration",
    "UIAuditSettingsPages.InputPageEveryControlKeyRoundTripsThroughSave", BLOCKED)

# -- Topbar ------------------------------------------------------------------
inv("TB-ZONE-LABEL", "Floating topbar", "pill", "Layout label pill (zone 0)", "button",
    "active layout name", "zone.layout", "Button", "Invoke", "active layout name",
    "Opens the 7-layout radio popup; selecting calls save_layout (engine + profile refresh + toast)",
    "handle_click zone 0 -> show_menu_popup -> host.select_layout", TOP, "534",
    "active_layout", "immediate save (bypasses Settings Save)", "no (mouse only; right-click menu is keyboard-reachable)",
    "tray/topbar visibility", "UIAuditSettingsPages.DefaultLayoutDropdownCoversAllSevenRegisteredLayouts (same ids)", BLOCKED)
inv("TB-ZONE-MODE", "Floating topbar", "pill", "Mode icon A/ব (zone 3)", "button",
    "A or ব", "zone.mode", "Button", "Invoke", "matches active layout",
    "Immediate English<->Bangla toggle; Bangla side returns to last_bangla_layout_id; toast",
    "handle_click zone 3 -> host toggle -> application toggle_english_bangla", APP, "3687",
    "active_layout + last_bangla_layout_id", "immediate save", "no",
    "layout dropdowns",
    "(Windows-runtime toggle; layout ids portable-verified)", BLOCKED)
inv("TB-ZONE-WRENCH", "Floating topbar", "pill", "Wrench menu (zone 1)", "menu button",
    "wrench glyph", "zone.wrench", "Button", "Invoke", "visible",
    "Opens 4-item menu: Settings…/On-Screen Keyboard…/Change Font…/About Suborno…",
    "handle_click zone 1 (button-down) -> show_menu_popup -> host callbacks", TOP, "559",
    "none", "n/a", "no (mouse only)",
    "settings window/OSK/About/font menu",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)
inv("TB-ZONE-CLOSE", "Floating topbar", "pill", "Close menu (zone 2)", "menu button",
    "X glyph", "zone.close", "Button", "Invoke", "visible",
    "Opens 2-item menu: System Tray (hide topbar, persist last_ui_mode=tray) / Exit (DestroyWindow)",
    "handle_click zone 2 -> host.switch_to_tray / host.exit_application", TOP, "574",
    "last_ui_mode", "tray choice persisted immediately", "no (mouse only)",
    "tray icon presence",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)
inv("TB-FONT-MENU", "Floating topbar", "wrench menu", "Change Font… submenu", "menu popup",
    "System Default + bangla_font_options()", "(per-item)", "ListItem", "Invoke/Toggle",
    "checked = active family", "Radio list; selection calls save_font (immediate + toast Font: {font})",
    "show_font_menu -> host.select_font -> save_font", TOP, "497",
    "font_family", "immediate save", "no (mouse only)",
    "settings font dropdown (same option list)",
    "UIAuditFontFamily.BengaliScriptFontNamesRoundTrip (model accepts unicode names)", BLOCKED)
inv("TB-DRAG", "Floating topbar", "pill", "Title drag", "gesture",
    "(none)", "(none)", "(none)", "(none)", "draggable",
    ">4px movement enters WM_NCLBUTTONDOWN drag; on release clamps into work area and persists position when remember_position",
    "WM_NCLBUTTONDOWN HTCAPTION + WM_EXITSIZEMOVE -> host.save_topbar_position", TOP, "719",
    "topbar_position_x/y", "persisted when remember_position", "no",
    "remember_position setting",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)
inv("TB-TOOLTIP", "Floating topbar", "pill", "Native tooltips (4)", "tooltip",
    "topbar.tooltip.layout/settings/tray/toggle", "(via HelpText)", "ToolTip", "(none)",
    "shown on hover", "Localized tooltip text per zone; doubles as UIA HelpText; refreshed on language change",
    "tooltip_text + refresh_layout_label", TOP, "163",
    "language", "not persisted", "no (hover)",
    "i18n tables",
    "UIAuditI18n.ShippedLocaleFilesCarryEveryInventoryKeyInBothLanguages (keys verified)", BLOCKED)
inv("TB-IDLE-DIM", "Floating topbar", "pill", "Idle dimming behavior", "state behavior",
    "(none)", "(none)", "(none)", "(none)", "full opacity",
    "After idle_time_seconds with dimming enabled, alpha = idle_opacity_percent*255/100; recovers on input",
    "kIdleTimerId 500ms -> update_idle_state -> SetLayeredWindowAttributes", TOP, "589",
    "idle_dimming_enabled, idle_time_seconds, idle_opacity_percent", "read from saved settings", "no",
    "Topbar page controls",
    "UIAuditSettingsPages.TopbarPageSlidersCoverMeaningfulRange (parameter range)", BLOCKED)
inv("TB-CONTEXT-MENU", "Floating topbar", "pill", "Right-click context menu", "context menu",
    "full tray menu", "(per-item)", "MenuItem", "Invoke", "hidden",
    "WM_CONTEXTMENU shows the full desktop context menu; keyboard-invoked anchors under pill",
    "WM_CONTEXTMENU -> host.show_context_menu -> show_desktop_context_menu", TOP, "775",
    "none", "n/a", "yes (keyboard-invoked WM_CONTEXTMENU)",
    "tray menu items",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)

# -- Tray --------------------------------------------------------------------
inv("TRAY-ICON", "System tray", "icon", "Tray icon", "icon",
    "Suborno", "(none)", "(none)", "(none)", "added at startup",
    "Left click/dblclick/NIN_SELECT shows topbar; right-click opens menu; restored on TaskbarCreated",
    "handle_tray_message", APP, "3780",
    "topbar visibility", "not persisted", "keyboard via NIN_KEYSELECT",
    "shell", "(static wiring verified; runtime BLOCKED)", BLOCKED)
inv("TRAY-CTX-SHOW-TOPBAR", "System tray", "context menu", "Show Topbar", "menu item",
    "topbar.menu.show_topbar", "(native menu id 2006)", "MenuItem", "Invoke",
    "checkmark = topbar_visible_", "Toggles topbar visibility",
    "handle_menu_command kMenuShowTopbar -> toggle_topbar", APP, "3825",
    "last_ui_mode (via toggle)", "persisted on toggle", "yes (keyboard menu nav)",
    "topbar window",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)
inv("TRAY-CTX-LAYOUT", "System tray", "context menu", "Keyboard Layout submenu", "menu popup",
    "topbar.menu.layout + 7 layout names", "(ids 2100-2106)", "MenuItem", "Invoke",
    "checked = active layout", "Radio selection calls save_layout (immediate + toast)",
    "handle_menu_command kMenuLayoutFirst+i -> save_layout", APP, "3829",
    "active_layout", "immediate save", "yes (keyboard menu nav)",
    "layout registry",
    "UIAuditSettingsPages.DefaultLayoutDropdownCoversAllSevenRegisteredLayouts (same ids)", BLOCKED)
inv("TRAY-CTX-FONT", "System tray", "context menu", "Font submenu", "menu popup",
    "topbar.menu.font + font_system_default + families", "(ids 2200+)", "MenuItem", "Invoke",
    "checked = active family", "Selection calls save_font (immediate + toast)",
    "handle_menu_command kMenuFontDefault/kMenuFontFirst+i -> save_font", APP, "3843",
    "font_family", "immediate save", "yes (keyboard menu nav)",
    "font list",
    "UIAuditFontFamily.* (model)", BLOCKED)
inv("TRAY-CTX-OSK", "System tray", "context menu", "On-Screen Keyboard…", "menu item",
    "topbar.menu.osk", "(id 2007)", "MenuItem", "Invoke", "—",
    "Toggles OSK window",
    "handle_menu_command kMenuOSK -> toggle_osk", APP, "3862",
    "none (window state)", "not persisted", "yes (keyboard menu nav)",
    "OSK window",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)
inv("TRAY-CTX-SETTINGS", "System tray", "context menu", "Settings…", "menu item",
    "topbar.menu.settings", "(id 2003)", "MenuItem", "Invoke", "—",
    "Opens/restores the Settings window",
    "handle_menu_command kMenuOpenSettings -> show_window", APP, "3864",
    "last_ui_mode", "persisted via show_window", "yes (keyboard menu nav)",
    "settings window",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)
inv("TRAY-CTX-TOGGLE-INPUT", "System tray", "context menu",
    "Enable/Disable Suborno in Windows (dynamic)", "menu item",
    "topbar.menu.enable / topbar.menu.disable", "(id 2002)", "MenuItem", "Invoke",
    "label follows profile_enabled()",
    "Flips TSF language profile; success/warning toast; tray tooltip updates",
    "handle_menu_command kMenuToggleInput -> toggle_profile", APP, "3865",
    "TSF profile state", "not persisted in settings file", "yes (keyboard menu nav)",
    "TSF runtime",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)
inv("TRAY-CTX-UPDATES", "System tray", "context menu", "Check for Updates…", "menu item",
    "topbar.menu.updates", "(id 2008)", "MenuItem", "Invoke", "—",
    "Opens the releases page in the default browser",
    "handle_menu_command kMenuUpdates -> open_web_link(kReleasesUrl)", APP, "3867",
    "none", "n/a", "yes (keyboard menu nav)",
    "shell/browser",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)
inv("TRAY-CTX-DOCS", "System tray", "context menu", "Documentation", "menu item",
    "topbar.menu.documentation", "(id 2009)", "MenuItem", "Invoke", "—",
    "Opens the documentation page",
    "handle_menu_command kMenuDocumentation -> open_web_link(kDocumentationUrl)", APP, "3868",
    "none", "n/a", "yes (keyboard menu nav)",
    "shell/browser",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)
inv("TRAY-CTX-CLEAR-LEARNT", "System tray", "context menu", "Clear Learnt Words…",
    "menu item", "topbar.menu.clear_learnt", "(id 2010)", "MenuItem", "Invoke", "—",
    "Same confirm + clear as the Input page button; success toast",
    "handle_menu_command kMenuClearLearnt -> clear_learnt_words_from_ui", APP, "3874",
    "personal dictionary", "immediate engine operation", "yes (keyboard menu nav)",
    "dictionary store",
    "UIAuditLearning.ClearLearntWordsLeavesEngineConsistent", BLOCKED)
inv("TRAY-CTX-UNINSTALL", "System tray", "context menu", "Uninstall Suborno", "menu item",
    "topbar.menu.uninstall", "(id 2004)", "MenuItem", "Invoke", "—",
    "Opens appwiz.cpl uninstall options",
    "handle_menu_command kMenuApps -> open_uninstall_options", APP, "3877",
    "none", "n/a", "yes (keyboard menu nav)",
    "shell",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)
inv("TRAY-CTX-EXIT", "System tray", "context menu", "Exit", "menu item",
    "topbar.menu.exit", "(id 2005)", "MenuItem", "Invoke", "—",
    "Destroys the main window (application exits)",
    "handle_menu_command kMenuExit -> DestroyWindow", APP, "3879",
    "none", "n/a", "yes (keyboard menu nav)",
    "application lifetime",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)
inv("TRAY-TOOLTIP", "System tray", "icon", "Tray tooltip", "tooltip",
    "Suborno - {enabled|disabled} - {layout}", "(none)", "ToolTip", "(none)",
    "Suborno", "Reflects profile state + active layout; updates on toggle/layout change",
    "update_tray_tooltip", APP, "3766",
    "profile state + active layout", "not persisted", "no (hover)",
    "settings.status.word_enabled/word_disabled i18n",
    "UIAuditI18n.* (keys verified)", BLOCKED)

# -- Toasts -------------------------------------------------------------------
inv("TOAST-SAVED", "Toast window", "notification", "Settings saved", "toast",
    "toast.settings_saved", "toast.message", "Text", "(none)", "hidden",
    "Shown after successful Save (success dot); 2600ms auto-dismiss; click dismisses",
    "save_settings_from_ui -> show_toast", APP, "3049",
    "none", "n/a", "no (WS_EX_NOACTIVATE)",
    "Save flow",
    "UIAuditE2E.RestartPersistenceEverySavedSettingRestored (save path)", BLOCKED)
inv("TOAST-LANGUAGE", "Toast window", "notification", "Interface language changed",
    "toast", "toast.language_changed", "toast.message", "Text", "(none)", "hidden",
    "Shown instead of settings_saved when the language changed on Save (information dot)",
    "save_settings_from_ui -> show_toast", APP, "3045",
    "none", "n/a", "no",
    "language dropdown",
    "UIAuditI18n.LanguageSelectionFollowsPortableLocaleMapping", BLOCKED)
inv("TOAST-FONT", "Toast window", "notification", "Font: {font}", "toast",
    "toast.font_changed", "toast.message", "Text", "(none)", "hidden",
    "Shown on font change from topbar/tray (information dot)",
    "save_font -> show_toast", APP, "3249",
    "none", "n/a", "no",
    "font menus",
    "UIAuditFontFamily.* (model)", BLOCKED)
inv("TOAST-LAYOUT", "Toast window", "notification", "Layout: {layout}", "toast",
    "toast.layout_switched", "toast.message", "Text", "(none)", "hidden",
    "Shown on every layout change (topbar/tray/hotkey/OSK mode)",
    "save_layout -> show_toast", APP, "3672",
    "none", "n/a", "no",
    "layout change paths",
    "UIAuditSettingsPages.DefaultLayoutDropdownCoversAllSevenRegisteredLayouts (ids)", BLOCKED)
inv("TOAST-ENC-U", "Toast window", "notification", "Encoding: Unicode", "toast",
    "toast.encoding_unicode", "toast.message", "Text", "(none)", "hidden",
    "Shown when Ctrl+Alt+E flips to Unicode",
    "toggle_encoding -> show_toast", APP, "3319",
    "none", "n/a", "no",
    "encoding hotkey",
    "UIAuditEncoding.ToggleProducesUnicodeAndBijoyOutput (engine path)", BLOCKED)
inv("TOAST-ENC-B", "Toast window", "notification", "Encoding: Bijoy ANSI", "toast",
    "toast.encoding_bijoy", "toast.message", "Text", "(none)", "hidden",
    "Shown when Ctrl+Alt+E flips to Bijoy ANSI",
    "toggle_encoding -> show_toast", APP, "3319",
    "none", "n/a", "no",
    "encoding hotkey",
    "UIAuditEncoding.ToggleProducesUnicodeAndBijoyOutput (engine path)", BLOCKED)
inv("TOAST-PROFILE-ON", "Toast window", "notification", "Suborno is enabled in Windows",
    "toast", "toast.profile_enabled", "toast.message", "Text", "(none)", "hidden",
    "Shown when the tray toggle enables the TSF profile (success dot)",
    "toggle_profile -> show_toast", APP, "3724",
    "none", "n/a", "no",
    "tray toggle item",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)
inv("TOAST-PROFILE-OFF", "Toast window", "notification", "Suborno is disabled in Windows",
    "toast", "toast.profile_disabled", "toast.message", "Text", "(none)", "hidden",
    "Shown when the tray toggle disables the TSF profile (warning dot)",
    "toggle_profile -> show_toast", APP, "3724",
    "none", "n/a", "no",
    "tray toggle item",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)
inv("TOAST-LEARNT-CLEARED", "Toast window", "notification", "Learnt words cleared", "toast",
    "toast.learnt_cleared", "toast.message", "Text", "(none)", "hidden",
    "Shown after Clear Learnt Words succeeds (success dot)",
    "clear_learnt_words_from_ui -> show_toast", APP, "2824",
    "none", "n/a", "no",
    "clear-learnt paths",
    "UIAuditLearning.ClearLearntWordsLeavesEngineConsistent", BLOCKED)
inv("TOAST-DISMISS", "Toast window", "notification", "Toast dismissal", "behavior",
    "(none)", "toast.message", "Button", "Invoke", "visible while timer runs",
    "Auto-dismiss 2600ms; mouse click dismisses; UIA Invoke dismisses; replacement replaces timer",
    "kDismissTimerId / WM_LBUTTONUP / UIA invoke", TST, "228",
    "none", "n/a", "no (never activates)",
    "toast lifetime",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)

# -- About dialog --------------------------------------------------------------
inv("ABOUT-CLOSE", "About dialog", "dialog", "Close button", "button",
    "common.close", "element-4", "Button", "Invoke", "enabled",
    "Destroys the About window",
    "activate_about_hotspot 4 -> DestroyWindow", APP, "760",
    "none", "n/a", "yes (Tab focus; Esc)",
    "about singleton",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)
inv("ABOUT-UPDATES", "About dialog", "dialog", "Check for Updates", "button",
    "about.check_for_updates", "element-0", "Button", "Invoke", "focused on open",
    "Opens the releases page",
    "activate_about_hotspot 0 -> open_web_link(kReleasesUrl)", APP, "760",
    "none", "n/a", "yes (Tab; Space/Enter)",
    "shell/browser",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)
inv("ABOUT-DOCS", "About dialog", "dialog", "Documentation", "button",
    "about.documentation", "element-1", "Button", "Invoke", "unfocused",
    "Opens the documentation page",
    "activate_about_hotspot 1 -> open_web_link(kDocumentationUrl)", APP, "760",
    "none", "n/a", "yes (Tab; Space/Enter)",
    "shell/browser",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)
inv("ABOUT-LICENSE", "About dialog", "dialog", "License (MIT)", "button",
    "about.license", "element-2", "Button", "Invoke", "unfocused",
    "Opens LICENSE beside the exe; localized error dialogs on failure",
    "activate_about_hotspot 2 -> open_license_document(L\"LICENSE\")", APP, "760",
    "none", "n/a", "yes (Tab; Space/Enter)",
    "installed files",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)
inv("ABOUT-NOTICES", "About dialog", "dialog", "Open Source Notices", "button",
    "about.open_source_notices", "element-3", "Button", "Invoke", "unfocused",
    "Opens THIRD_PARTY_NOTICES.md beside the exe",
    "activate_about_hotspot 3 -> open_license_document(L\"THIRD_PARTY_NOTICES.md\")", APP, "760",
    "none", "n/a", "yes (Tab; Space/Enter)",
    "installed files",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)
inv("ABOUT-KEYS", "About dialog", "dialog", "Keyboard model (Esc/Tab/Enter/Space)",
    "behavior", "(none)", "(none)", "(none)", "(none)", "(none)",
    "Esc closes; Tab/Shift+Tab cycle the 5 hotspots; Space/Enter activate focused; focus starts at hotspot 0",
    "about_window_proc WM_KEYDOWN", APP, "1027",
    "none", "n/a", "yes",
    "hotspot focus ring",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)
inv("ABOUT-SINGLETON", "About dialog", "dialog", "Single instance", "behavior",
    "(none)", "(none)", "(none)", "(none)",
    "one About at a time",
    "Second invocation focuses the existing window instead of creating another",
    "show_about_dialog FindWindowW guard", APP, "1138",
    "none", "n/a", "n/a",
    "window lifetime",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)

# -- Menu popup ----------------------------------------------------------------
inv("MENU-NAV-KEYS", "Menu popup", "popup", "Keyboard navigation", "behavior",
    "(none)", "menu.item.<n>", "ListItem", "Invoke/Toggle", "no highlight",
    "Up/Down move (skipping separators/disabled), Enter chooses, Esc dismisses",
    "menu_popup_proc WM_KEYDOWN", MP, "322",
    "none", "n/a", "yes",
    "all popup consumers",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)
inv("MENU-HOVER", "Menu popup", "popup", "Hover highlight", "behavior",
    "(none)", "(hit-tested)", "(none)", "(none)", "none",
    "Hovering highlights the item; mouse-up selects",
    "set_highlight / WM_MOUSEMOVE", MP, "236",
    "none", "n/a", "no",
    "hit testing",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)
inv("MENU-DISABLED", "Menu popup", "popup", "Disabled items", "behavior",
    "(none)", "menu.item.<n>", "ListItem", "(Invoke refused)", "dimmed",
    "Disabled items render dimmed and cannot be chosen by mouse or keyboard",
    "MenuItem.enabled flag checks", MP, "316",
    "none", "n/a", "yes (skipped by arrows)",
    "item producers",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)
inv("MENU-KILLFOCUS", "Menu popup", "popup", "Focus-loss dismissal", "behavior",
    "(none)", "(none)", "(none)", "(none)", "open",
    "WM_KILLFOCUS dismisses the popup; modal loop routes keys while open",
    "menu_popup_proc WM_KILLFOCUS", MP, "336",
    "none", "n/a", "n/a",
    "owner window",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)

# -- OSK ------------------------------------------------------------------------
inv("OSK-MODE-EN", "On-screen keyboard", "title row", "[English] mode button", "button",
    "osk.mode.english", "element-0", "Button", "Invoke", "accent when active",
    "Switches typing mode to English (save_layout(\"english\") immediate)",
    "handle_mode_click -> host.select_mode", OSK, "344",
    "active_layout", "immediate save", "yes (Tab-less: UIA Invoke)",
    "mode display name, key labels",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)
inv("OSK-MODE-BN", "On-screen keyboard", "title row", "[বাংলা] mode button", "button",
    "osk.mode.bangla", "element-1", "Button", "Invoke", "accent when active",
    "Switches typing mode to the remembered Bangla layout",
    "handle_mode_click -> host.select_mode", OSK, "344",
    "last_bangla_layout_id", "immediate save", "no",
    "mode display name, key labels",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)
inv("OSK-CLOSE", "On-screen keyboard", "title row", "Close ✕", "button",
    "common.close", "element-2", "Button", "Invoke", "enabled",
    "Hides the OSK window",
    "WM_LBUTTONDOWN / UIA invoke", OSK, "657",
    "none", "not persisted", "no",
    "OSK visibility",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)
inv("OSK-KEYS", "On-screen keyboard", "key grid", "30 letter/digit keys", "button group",
    "letters/digits on caps", "element-3..32", "Button", "Invoke", "visible",
    "Click injects the real virtual key (SendInput); output flows through TSF like a physical keystroke; Shift latches for one key",
    "handle_key_click -> send_virtual_key", OSK, "295",
    "typing path", "n/a", "no (mouse/UIA; not Tab-focusable)",
    "active layout + shift latch",
    "UIAuditOsk.PreviewLabelsForEveryRegisteredLayout (labels for all 7 layouts)", BLOCKED)
inv("OSK-SPECIALS", "On-screen keyboard", "key grid",
    "Shift / Space / Backspace / Enter", "button group",
    "osk.key.shift/space/backspace/enter", "element-33..36", "Button", "Invoke",
    "visible", "Shift latches; Space/Backspace/Enter send their VKs unwrapped",
    "handle_key_click -> send_virtual_key", OSK, "295",
    "typing path", "n/a", "no",
    "TSF text service",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)
inv("OSK-ESC", "On-screen keyboard", "window", "Escape hides", "behavior",
    "(none)", "(none)", "(none)", "(none)", "visible",
    "Esc hides the OSK",
    "osk_window_proc WM_KEYDOWN", OSK, "717",
    "none", "n/a", "n/a",
    "OSK visibility",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)
inv("OSK-BN-LABELS", "On-screen keyboard", "key grid",
    "Bangla key labels (host.bangla_preview)", "indicator",
    "layout_preview(letter)", "(via key Name)", "Button", "(names)",
    "letters or Bangla glyphs",
    "Each key shows what the active layout produces for that letter; honors font setting",
    "host.bangla_preview -> engine layout_preview", APP, "4092",
    "active layout", "follows saved layout", "n/a (labels)",
    "engine layout_preview",
    "UIAuditOsk.PreviewLabelsForEveryRegisteredLayout (all 7 layouts, base+shift)", PASS)

# -- TSF candidate window -------------------------------------------------------
inv("CAND-SHOW-GATE", "Candidate window", "window", "Show gate", "behavior",
    "(none)", "(none)", "(none)", "(none)", "hidden",
    "Shown only when ui.show_candidate_window and >= 2 candidates",
    "SubornoTextService::_UpdateCandidateUI", TSF, "997",
    "show_candidate_window", "persisted setting gates it", "n/a",
    "composition state",
    "UIAuditSettingsPages.InterfacePageEveryControlKeyRoundTripsThroughSave (flag)", BLOCKED)
inv("CAND-CLICK-SELECT", "Candidate window", "list", "Click to select", "behavior",
    "N. <candidate> rows", "(none — no UIA)", "ListItem (implicit)", "(none)",
    "none selected", "Click commits the clicked candidate (SetSelection+Finalize)",
    "WM_LBUTTONUP -> hit_candidate -> Finalize", CAND, "730",
    "none", "n/a", "no (window never activates)",
    "composition state",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)
inv("CAND-NUMBER-KEYS", "Candidate window", "list", "Digit selection 1-9", "behavior",
    "(none)", "(none)", "(none)", "(none)", "—",
    "Digits commit candidate n-1 only when select_candidate_using_number_keys",
    "text_service WM_KEYDOWN digit branch", TSF, "774",
    "select_candidate_using_number_keys", "persisted setting gates it", "n/a (types into app)",
    "Input page checkbox",
    "UIAuditSettingsPages.InputPageEveryControlKeyRoundTripsThroughSave (flag)", BLOCKED)
inv("CAND-ARROWS", "Candidate window", "list", "Arrow selection", "behavior",
    "(none)", "(none)", "(none)", "(none)", "first candidate",
    "Up/Down move selection; selection row highlighted",
    "text_service WM_KEYDOWN arrows", TSF, "764",
    "none", "n/a", "n/a",
    "composition state",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)
inv("CAND-PAGE-KEYS", "Candidate window", "list", "Paging", "behavior",
    "(none)", "(none)", "(none)", "(none)", "page 0",
    "PgUp/PgDn change page; page capacity 5 (fixed) or 8 (auto vertical) / 5 columns horizontal",
    "text_service paging + reset_page_index", TSF, "734",
    "candidate_fixed_height", "persisted setting gates capacity", "n/a",
    "orientation setting",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)
inv("CAND-SPACE-ESC", "Candidate window", "list", "Space commit / Esc abort", "behavior",
    "(none)", "(none)", "(none)", "(none)", "—",
    "Space commits the raw (unconverted) input; Esc aborts composition",
    "text_service WM_KEYDOWN space/esc", TSF, "726",
    "none", "n/a", "n/a",
    "composition state",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)
inv("CAND-PLACEMENT", "Candidate window", "window", "Placement chain", "behavior",
    "(none)", "(none)", "(none)", "(none)", "caret-follow",
    "Caret rect via GetGUIThreadInfo; falls back to mouse pointer when follow_mouse_pointer set; clamped on-screen",
    "candidate window placement code", CAND, "431",
    "follow_caret, follow_mouse_pointer_if_caret_tracking_fails", "persisted settings gate it", "n/a",
    "Interface page controls",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)
inv("CAND-SLIDE-ANIM", "Candidate window", "window", "Slide-in animation", "behavior",
    "(none)", "(none)", "(none)", "(none)", "—",
    "12px travel at 4px/16ms timer when enable_sliding_effect",
    "WM_TIMER animation", CAND, "618",
    "enable_sliding_effect", "persisted setting gates it", "n/a",
    "Interface page control",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)
inv("CAND-NOACTIVATE", "Candidate window", "window", "Never takes focus", "behavior",
    "(none)", "(none)", "(none)", "(none)", "—",
    "WM_MOUSEACTIVATE returns MA_NOACTIVATE; typing focus stays in the target app",
    "candidate window proc", CAND, "709",
    "none", "n/a", "n/a",
    "target application focus",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)

# -- Global shortcuts ------------------------------------------------------------
inv("SC-LAYOUT-HOTKEY", "Application", "global", "Layout switching hotkey", "hotkey",
    "layout_switch_shortcut value", "(none)", "(none)", "(none)", "F10",
    "Registered hotkey (F1/F2/F3/F4/F10/F12/Ctrl+Space) cycles to the next layout; save + toast",
    "RegisterHotKey kLayoutHotkeyId -> WM_HOTKEY -> cycle_layout", APP, "3257",
    "layout_switch_shortcut + active_layout", "immediate save", "yes (global hotkey)",
    "keyboard focus anywhere",
    "UIAuditSettingsPages.ShortcutDropdownEveryRegisterableOptionAccepted (allowlist)", BLOCKED)
inv("SC-ENC-HOTKEY", "Application", "global", "Encoding toggle hotkey Ctrl+Alt+E", "hotkey",
    "(hardcoded)", "(none)", "(none)", "(none)", "registered unless disabled",
    "Flips Unicode<->Bijoy ANSI; save + toast; skipped when disable_encoding_shortcut",
    "RegisterHotKey kEncodingHotkeyId -> WM_HOTKEY -> toggle_encoding", APP, "3293",
    "preferred_encoding + disable_encoding_shortcut", "immediate save", "yes (global hotkey)",
    "keyboard focus anywhere",
    "UIAuditEncoding.ToggleProducesUnicodeAndBijoyOutput (engine path)", BLOCKED)

# -- UIA providers ----------------------------------------------------------------
inv("UIA-SETTINGS-TREE", "Settings window", "UIA", "Settings UIA tree", "provider",
    "(window name)", "root 'window' + action.* + sidebar.* + text.*", "Window",
    "Invoke/Toggle/ExpandCollapse/RangeValue/SelectionItem", "exposed",
    "WM_GETOBJECT returns ElementProvider root; per-control patterns per control type",
    "handle_get_object + element_info/invoke/toggle/expand/set_element_value overrides",
    UIA, "795", "n/a", "n/a", "n/a",
    "element provider lifetime",
    "(static audit below; runtime BLOCKED)", BLOCKED)
inv("UIA-TOPBAR", "Floating topbar", "UIA", "Topbar UIA elements", "provider",
    "(4 buttons)", "zone.layout/wrench/close/mode", "Button", "Invoke",
    "exposed", "HelpText = tooltip text",
    "Topbar::Impl ElementSource", UIA, "71", "n/a", "n/a", "n/a",
    "element provider lifetime",
    "(static audit below; runtime BLOCKED)", BLOCKED)
inv("UIA-OSK", "On-screen keyboard", "UIA", "OSK UIA elements", "provider",
    "(37 buttons)", "element-0..36 (positional)", "Button", "Invoke",
    "exposed", "Key 29 (empty slot) exposes an EMPTY Name — FINDING UIA-01",
    "OskWindow::Impl ElementSource", UIA, "394", "n/a", "n/a", "n/a",
    "element provider lifetime",
    "(static audit below; runtime BLOCKED)", BLOCKED)
inv("UIA-TOAST", "Toast window", "UIA", "Toast UIA element", "provider",
    "toast text", "toast.message", "Text", "Invoke",
    "exposed", "Not keyboard-focusable; Invoke dismisses",
    "Toast Impl ElementSource", UIA, "48", "n/a", "n/a", "n/a",
    "element provider lifetime",
    "(static audit below; runtime BLOCKED)", BLOCKED)
inv("UIA-MENU", "Menu popup", "UIA", "Menu item UIA elements", "provider",
    "item labels", "menu.item.<n>", "ListItem", "Invoke/Toggle",
    "exposed", "Checked/radio items expose Toggle state",
    "MenuPopupState ElementSource", UIA, "60", "n/a", "n/a", "n/a",
    "element provider lifetime",
    "(static audit below; runtime BLOCKED)", BLOCKED)
inv("UIA-ABOUT", "About dialog", "UIA", "About UIA elements", "provider",
    "5 buttons", "element-0..4 (positional)", "Button", "Invoke",
    "exposed", "No explicit AutomationIds — FINDING UIA-02",
    "AboutState ElementSource", UIA, "632", "n/a", "n/a", "n/a",
    "element provider lifetime",
    "(static audit below; runtime BLOCKED)", BLOCKED)
inv("UIA-LIFETIME", "All surfaces", "UIA", "Provider lifetime safety", "provider",
    "(none)", "(none)", "(none)", "(none)", "—",
    "std::weak_ptr liveness token; destroyed windows answer UIA_E_ELEMENTNOTAVAILABLE",
    "ElementProvider liveness check", UIA, "693", "n/a", "n/a", "n/a",
    "window destruction", "(static audit; runtime BLOCKED)", BLOCKED)
inv("UIA-FOCUS-EVENTS", "All surfaces", "UIA", "Focus change events", "provider",
    "(none)", "(none)", "(none)", "(none)", "—",
    "notify_focus_changed raises UIA_AutomationFocusChangedEventId after mouse-down and Tab moves",
    "notify_focus_changed", UIA, "822", "n/a", "n/a", "n/a",
    "focus model", "(static audit; runtime BLOCKED)", BLOCKED)

# -- Persistence/state behaviors ---------------------------------------------------
inv("PERSIST-SAVE-FLOW", "Application", "persistence", "Save pipeline", "behavior",
    "(none)", "(none)", "(none)", "(none)", "—",
    "Staged -> startup registry -> engine save (rollback on failure) -> profile refresh -> theme -> i18n -> hotkeys -> fonts -> topbar -> toast -> hide",
    "save_settings_from_ui", APP, "2989",
    "all keys", "atomic whole-file write", "n/a",
    "every control",
    "UIAuditSaveCancel.*; UIAuditE2E.RestartPersistenceEverySavedSettingRestored; settings/atomic-file suites", BLOCKED)
inv("PERSIST-CANCEL-FLOW", "Application", "persistence", "Cancel pipeline", "behavior",
    "(none)", "(none)", "(none)", "(none)", "—",
    "Re-stage from engine, rebuild pages, hide; never writes",
    "cancel_settings", APP, "2975",
    "none", "no write", "n/a",
    "staged snapshot",
    "UIAuditSaveCancel.CancelLeavesPersistedStateUntouched", BLOCKED)
inv("PERSIST-RESTART", "Application", "persistence", "Restart restore + start mode", "behavior",
    "(none)", "(none)", "(none)", "(none)", "—",
    "initialize() loads settings, syncs startup pref, applies start_mode (topbar/tray/window/last-state via last_ui_mode)",
    "Application::initialize + apply_start_mode", APP, "1688",
    "all keys + last_ui_mode", "file read; defaults persisted on first run", "n/a",
    "settings store",
    "UIAuditE2E.RestartPersistenceEverySavedSettingRestored; UIAuditE2E.FirstLaunchPersistsDefaultsAndEngineStartsOnDefaultLayout", BLOCKED)
inv("PERSIST-QUARANTINE", "Application", "persistence", "Corrupt settings recovery", "behavior",
    "(none)", "(none)", "(none)", "(none)", "—",
    "invalid_format/unsupported_version -> quarantine_file (.corrupt-<pid>-<n>) -> defaults",
    "initialize -> quarantine_file", APP, "1737",
    "whole file", "original bytes preserved beside original", "n/a",
    "settings parser",
    "UIAuditNegative.CorruptSettingsQuarantinedAndDefaultsRestored", BLOCKED)
inv("PERSIST-LIVE-SAVE-PATHS", "Application", "persistence",
    "Immediate-save paths (bypass Save/Cancel)", "behavior",
    "(none)", "(none)", "(none)", "(none)", "—",
    "save_font / save_layout / persist_last_ui_mode / topbar position / toggle_encoding / startup registry write persist immediately",
    "save_font, save_layout, persist_last_ui_mode, save_topbar_position, toggle_encoding",
    APP, "3229", "font_family, active_layout, last_ui_mode, topbar position, preferred_encoding",
    "immediate whole-file saves", "n/a",
    "topbar/tray/hotkey paths",
    "UIAuditLearning.PredictionTogglePersistsThroughEngine (same engine path); UIAuditEncoding.ToggleProducesUnicodeAndBijoyOutput", BLOCKED)
inv("STATE-THEMECHANGED", "All surfaces", "state", "WM_THEMECHANGED handling", "behavior",
    "(none)", "(none)", "(none)", "(none)", "—",
    "Every surface refreshes the palette on Windows theme change; System mode re-probes the registry",
    "WM_THEMECHANGED handlers per window", APP, "1911",
    "theme_id=system", "not persisted", "n/a",
    "theme system",
    "scripts/contrast_check.py (palettes; 22 pairs 0 failures)", BLOCKED)
inv("STATE-DPICHANGED", "All surfaces", "state", "WM_DPICHANGED handling", "behavior",
    "(none)", "(none)", "(none)", "(none)", "—",
    "Per-Monitor v2: settings window refits into the new monitor's work area; other surfaces rescale",
    "WM_DPICHANGED handlers + dpi_fitting", APP, "1561",
    "none", "not persisted", "n/a",
    "DPI system",
    "(static wiring verified; runtime BLOCKED)", BLOCKED)

# ---------------------------------------------------------------------------
# Traceability rows: element -> test case -> result.
# test_kind: linux-unit-test (executed here) | static-source-trace (this audit)
#            | windows-runtime (BLOCKED in this sandbox)
# ---------------------------------------------------------------------------
T: list[list[str]] = []


def trc(tid: str, elem: str, expected: str, case: str, kind: str, evidence: str,
        result: str) -> None:
    T.append([tid, elem, expected, case, kind, evidence, result])


UNIT = "linux-unit-test"
STATIC = "static-source-trace"
RT = "windows-runtime"

trc("TRC-001", "SET-FOOT-SAVE",
    "Save commits staged values; repeated Save byte-identical",
    "UIAuditSaveCancel.SaveIsIdempotentByteForByteWhenNothingChanged", UNIT,
    "tests/unit/ui_functional_audit_test.cpp", PASS)
trc("TRC-002", "SET-FOOT-SAVE",
    "Saved settings restore on restart (fresh engine + fresh store)",
    "UIAuditE2E.RestartPersistenceEverySavedSettingRestored", UNIT,
    "tests/unit/ui_functional_audit_test.cpp", PASS)
trc("TRC-003", "SET-FOOT-CANCEL",
    "Cancel discards staged changes without touching the persisted file",
    "UIAuditSaveCancel.CancelLeavesPersistedStateUntouched", UNIT,
    "tests/unit/ui_functional_audit_test.cpp", PASS)
trc("TRC-004", "SET-CLOSE-X",
    "Close must not persist (same contract as Cancel)",
    "UIAuditSaveCancel.CancelLeavesPersistedStateUntouched (contract)", UNIT,
    "tests/unit/ui_functional_audit_test.cpp", PASS)
trc("TRC-005", "SET-FOOT-SAVE",
    "Saving one control leaves every other value intact",
    "UIAuditSaveCancel.SavingOneControlLeavesEveryOtherValueIntact", UNIT,
    "tests/unit/ui_functional_audit_test.cpp", PASS)
trc("TRC-010", "SET-G-* (page controls)",
    "All General-page staged keys round-trip through Save; all dropdown options walked",
    "UIAuditSettingsPages.GeneralPageEveryControlKeyRoundTripsThroughSave", UNIT,
    "tests/unit/ui_functional_audit_test.cpp", PASS)
trc("TRC-011", "SET-G-DEFAULT-LAYOUT",
    "All 7 registered layouts selectable and persisted",
    "UIAuditSettingsPages.DefaultLayoutDropdownCoversAllSevenRegisteredLayouts", UNIT,
    "tests/unit/ui_functional_audit_test.cpp", PASS)
trc("TRC-012", "SET-I-* (page controls)",
    "All Interface-page staged keys round-trip; orientation/theme/language dropdowns walked over every option; checkbox walked both states",
    "UIAuditSettingsPages.InterfacePageEveryControlKeyRoundTripsThroughSave", UNIT,
    "tests/unit/ui_functional_audit_test.cpp", PASS)
trc("TRC-013", "SET-T-IDLE-OPACITY",
    "Opacity slider 0/1/50/99/100 persist; 101 rejected",
    "UIAuditSettingsPages.TopbarPageSlidersCoverMeaningfulRange", UNIT,
    "tests/unit/ui_functional_audit_test.cpp", PASS)
trc("TRC-014", "SET-T-IDLE-TIME",
    "Idle-time slider 1/10/60/3599/3600 persist; 0 and 3601 rejected",
    "UIAuditSettingsPages.TopbarPageSlidersCoverMeaningfulRange", UNIT,
    "tests/unit/ui_functional_audit_test.cpp", PASS)
trc("TRC-015", "SET-IN-* (page controls)",
    "All Input-page staged keys round-trip through Save",
    "UIAuditSettingsPages.InputPageEveryControlKeyRoundTripsThroughSave", UNIT,
    "tests/unit/ui_functional_audit_test.cpp", PASS)
trc("TRC-016", "SET-IN-SHORTCUT",
    "All 9 shortcut values accepted; '', F5, Ctrl+Alt+F13, Shift+Space rejected",
    "UIAuditSettingsPages.ShortcutDropdownEveryRegisterableOptionAccepted", UNIT,
    "tests/unit/ui_functional_audit_test.cpp", PASS)
trc("TRC-017", "SET-I-FONT",
    "Unicode (Bangla-script) font family names persist; safety rules hold",
    "UIAuditFontFamily.BengaliScriptFontNamesRoundTrip; UIAuditFontFamily.FontNameSafetyRulesStillHold",
    UNIT, "tests/unit/ui_functional_audit_test.cpp + src/settings/settings.cpp fix", PASS)
trc("TRC-020", "TOAST-LANGUAGE",
    "Language setting maps bn/bangla->bangla, everything else->english",
    "UIAuditI18n.LanguageSelectionFollowsPortableLocaleMapping", UNIT,
    "tests/unit/ui_functional_audit_test.cpp", PASS)
trc("TRC-021", "SET-I-LANGUAGE / all surfaces",
    "All 157 render-time i18n keys resolve in BOTH locales; bn table localized (6 documented exceptions); en/bn key sets identical",
    "UIAuditI18n.ShippedLocaleFilesCarryEveryInventoryKeyInBothLanguages", UNIT,
    "tests/unit/ui_functional_audit_test.cpp; scripts/i18n_key_check.py (parity OK, 0 missing, 0 dead)",
    PASS)
trc("TRC-022", "OSK-BN-LABELS",
    "Every registered layout produces non-empty Bengali preview labels; shifted letters differ; anchors match per-layout suites",
    "UIAuditOsk.PreviewLabelsForEveryRegisteredLayout", UNIT,
    "tests/unit/ui_functional_audit_test.cpp", PASS)
trc("TRC-023", "SET-IN-ENCODING",
    "Unicode path preserves UTF-8; Bijoy path yields legacy bytes; broken conjunct drops only the joiner",
    "UIAuditEncoding.ToggleProducesUnicodeAndBijoyOutput", UNIT,
    "tests/unit/ui_functional_audit_test.cpp", PASS)
trc("TRC-024", "SET-I-NEXT-WORD",
    "Prediction toggle persists through the engine immediate-save path",
    "UIAuditLearning.PredictionTogglePersistsThroughEngine", UNIT,
    "tests/unit/ui_functional_audit_test.cpp", PASS)
trc("TRC-025", "SET-IN-CLEAR-LEARNT",
    "Clear learnt words leaves a consistent engine/dictionary",
    "UIAuditLearning.ClearLearntWordsLeavesEngineConsistent", UNIT,
    "tests/unit/ui_functional_audit_test.cpp", PASS)
trc("TRC-030", "PERSIST-RESTART",
    "First launch: defaults active, file absent; engine starts on suborno-phonetic",
    "UIAuditE2E.FirstLaunchPersistsDefaultsAndEngineStartsOnDefaultLayout", UNIT,
    "tests/unit/ui_functional_audit_test.cpp", PASS)
trc("TRC-031", "PERSIST-QUARANTINE",
    "Corrupt settings rejected; quarantine preserves bytes; next start = defaults",
    "UIAuditNegative.CorruptSettingsQuarantinedAndDefaultsRestored", UNIT,
    "tests/unit/ui_functional_audit_test.cpp", PASS)
trc("TRC-032", "PERSIST-RESTART",
    "Unknown saved layout id corrected to default; sessions fall back",
    "UIAuditNegative.UnknownSavedLayoutIdCorrectedOnEngineStart", UNIT,
    "tests/unit/ui_functional_audit_test.cpp", PASS)
trc("TRC-033", "SET-T-IDLE-OPACITY / SET-T-IDLE-TIME / candidate_limit",
    "Out-of-range values rejected at the model boundary",
    "UIAuditNegative.SliderAndLimitValuesOutsideRangeAreRejected", UNIT,
    "tests/unit/ui_functional_audit_test.cpp", PASS)
trc("TRC-034", "PERSIST-SAVE-FLOW",
    "50 rapid save/load cycles stay consistent",
    "UIAuditNegative.RapidRepeatedSaveLoadCyclesStayConsistent", UNIT,
    "tests/unit/ui_functional_audit_test.cpp", PASS)
trc("TRC-040", "Settings window (all interactive elements)",
    "Every mouse/keyboard interaction on the live window",
    "Windows runtime session (mouse clicks, key presses, screenshots, Narrator)", RT,
    "no Windows 10/11 x64 environment in this sandbox", BLOCKED)
trc("TRC-041", "Topbar / tray (all elements)",
    "Every click/menu item/toast/drag on the live shell", "Windows runtime session", RT,
    "no Windows environment", BLOCKED)
trc("TRC-042", "OSK (all keys/modes)",
    "Real SendInput key injection through TSF composition", "Windows runtime session", RT,
    "no Windows environment", BLOCKED)
trc("TRC-043", "Candidate window (all interactions)",
    "Live composition, paging, click-commit in a target app", "Windows runtime session", RT,
    "no Windows environment", BLOCKED)
trc("TRC-044", "UIA providers (all surfaces)",
    " FlaUI/Inspect/AccessibilityInsights tree walk + pattern invocation + Narrator",
    "Windows runtime UIA session", RT, "no Windows environment", BLOCKED)
trc("TRC-045", "STATE-DPICHANGED / STATE-THEMECHANGED / TB-DRAG",
    "DPI 100-200%, monitor moves, live theme/High-Contrast switches",
    "Windows runtime session", RT, "no Windows environment", BLOCKED)
trc("TRC-050", "SET-* handlers (wiring)",
    "Each control's staged mutation calls the right settings key",
    "Static source trace (this audit, application.cpp:2089-2401)", STATIC,
    "docs/reports/UI_FUNCTIONAL_AUDIT_01_INVENTORY_V2.csv handler column", PASS)
trc("TRC-051", "TB-*/TRAY-*/TOAST-*/ABOUT-*/CAND-*/SC-* handlers (wiring)",
    "Each element's handler chain matches the documented behavior",
    "Static source trace (this audit)", STATIC,
    "docs/reports/UI_FUNCTIONAL_AUDIT_01_INVENTORY_V2.csv handler column", PASS)
trc("TRC-052", "UIA-OSK dead key (element 29)",
    "Every exposed UIA element must carry a non-empty Name",
    "Static audit finding UIA-01 (osk_window.cpp:72 empty slot painted as a key)", STATIC,
    "osk_window.cpp:72; ui_accessibility.cpp:457", FAIL)
trc("TRC-053", "UIA-SETTINGS-TREE IsEnabled",
    "Disabled controls should report IsEnabled=false",
    "Static audit finding UIA-03 (ui_accessibility.cpp:271 always TRUE)", STATIC,
    "ui_accessibility.cpp:271", FAIL)
trc("TRC-054", "PERSIST-TRAY-ERROR-STRINGS",
    "Tray menu web-link failures should use localized strings like About does",
    "Static audit finding I18N-01 (application.cpp:3921-3924 raw literals)", STATIC,
    "application.cpp:3921-3924 vs application.cpp:766-776", FAIL)
trc("TRC-055", "TB-ZONE-MODE hover face",
    "Hovering a mode button must highlight THAT button",
    "Static audit finding UI-01 (osk hover face cross-wiring osk_window.cpp:578-587)", STATIC,
    "osk_window.cpp:578-587", FAIL)
trc("TRC-060", "Existing regression suites (settings, persistence, i18n, engine, layouts)",
    "All pre-existing behavior contracts keep passing after the audit fix",
    "Full unit + integration suite (632 + 2 tests)", UNIT,
    "build/linux-audit ctest run, all PASS", PASS)


def main() -> int:
    OUT.mkdir(parents=True, exist_ok=True)
    inv_path = OUT / "UI_FUNCTIONAL_AUDIT_INVENTORY_V2.csv"
    trc_path = OUT / "UI_FUNCTIONAL_AUDIT_TRACEABILITY_MATRIX_V2.csv"
    with inv_path.open("w", encoding="utf-8", newline="") as f:
        w = csv.writer(f)
        w.writerow(INV_COLS)
        w.writerows(I)
    with trc_path.open("w", encoding="utf-8", newline="") as f:
        w = csv.writer(f)
        w.writerow(TRC_COLS)
        w.writerows(T)

    surfaces = {}
    for row in I:
        surfaces[row[1]] = surfaces.get(row[1], 0) + 1
    print(f"inventory rows: {len(I)}  -> {inv_path}")
    for s, c in sorted(surfaces.items()):
        print(f"  {c:3d}  {s}")
    results = {}
    for row in T:
        results[row[6]] = results.get(row[6], 0) + 1
    print(f"traceability rows: {len(T)}  -> {trc_path}")
    for r, c in sorted(results.items()):
        print(f"  {c:3d}  {r}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
