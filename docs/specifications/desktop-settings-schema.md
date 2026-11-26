# Desktop Settings Schema

## Format and compatibility

Settings are stored as UTF-8, newline-delimited key/value pairs and written
atomically. The current schema version is **2**.

- Version 1 contained only engine settings: active layout, candidate limit, and
  learning enablement.
- Version 2 adds General, Interface, Topbar, and Input groups.
- A version 1 file loads with version 2 defaults for the new groups.
- The next successful save writes the complete version 2 representation.
- Unknown keys, duplicate keys, malformed values, unsupported enum values,
  integer overflow, and values outside supported ranges are rejected.
- A missing file returns default settings. An unreadable existing file is an
  I/O error.

## General

| Key | Type | Default | Allowed values |
|---|---|---|---|
| remember_last_input_layout | boolean | true | true / false |
| start_with_windows | boolean | false | true / false |
| start_mode | enum | last-state | last-state, topbar, system-tray, main-window |
| switching_mode | enum | per-application-window | per-application-window, global |
| last_ui_mode | string | topbar | topbar, tray, window |
| last_bangla_layout_id | string | suborno-phonetic | value-string; the layout the English↔বাংলা toggle restores (unknown ids heal to the active layout at load) |

The Windows Run registry value is the effective OS startup mechanism. The
desktop application mirrors the observed registry state into
start_with_windows and uses rollback if settings persistence fails.

`last_ui_mode` records the surface that was last visible (floating topbar,
system tray only, or the settings window). The desktop shell writes it
whenever visibility changes and honors it when start_mode is last-state.
`last_bangla_layout_id` is written by the shell whenever a Bangla layout
becomes active and consumed by the English↔বাংলা quick toggle (topbar mode
icon, tray, on-screen keyboard mode buttons).

## Interface

| Key | Type | Default | Validation |
|---|---|---|---|
| theme_id | string | suborno-dark | lowercase ASCII letters, digits, hyphen, underscore; recognized values: `suborno-dark`, `suborno-light`, `system` (follow the Windows app-mode preference; any other valid-format value renders with the dark palette). When a Windows High Contrast scheme is active it overrides the saved theme for custom-drawn surfaces until HC is turned off |
| font_family | string | (empty) | ASCII letters, digits, space, `-`, `_`, `.`, `(`, `)`; max 32 chars; empty = OS default |
| show_candidate_window | boolean | true | true / false |
| next_word_prediction_enabled | boolean | false | true / false |
| candidate_orientation | enum | vertical | vertical, horizontal |
| candidate_fixed_height | boolean | true | true / false |
| use_os_default_bangla_font | boolean | true | true / false |
| enable_sliding_effect | boolean | true | true / false |
| follow_caret | boolean | true | true / false |
| follow_mouse_pointer_if_caret_tracking_fails | boolean | true | true / false |

## Topbar

| Key | Type | Default | Range |
|---|---|---|---|
| idle_dimming_enabled | boolean | true | true / false |
| idle_opacity_percent | integer | 50 | 0–100 |
| idle_time_seconds | integer | 10 | 1–3600 |
| remember_topbar_position | boolean | true | true / false |
| topbar_position_x | integer | -1 | -32768–32767; -1 = no saved position |
| topbar_position_y | integer | -1 | -32768–32767; -1 = no saved position |

The desktop shell writes topbar_position_x/y after each drag (when
remember_topbar_position is true) and restores the saved position on launch.

## Input

| Key | Type | Default | Allowed values |
|---|---|---|---|
| layout_switch_shortcut | string | F10 | F1, F2, F3, F4, F10, F12, Ctrl+Space, Alt+Shift, Win+Space |
| disable_encoding_shortcut | boolean | false | true / false |
| map_numpad_to_bangla_numbers | boolean | true | true / false |
| change_locale_to_bn_bd_when_layout_not_english | boolean | true | true / false |
| ignore_caps_lock_state_in_bangla | boolean | false | true / false |
| select_candidate_using_number_keys | boolean | true | true / false |
| preferred_encoding | enum | unicode | unicode, ansi |

The desktop shell registers a global hotkey for F1–F4/F10/F12/Ctrl+Space that
cycles through the keyboard layouts. Alt+Shift and Win+Space remain valid
storage values (Windows reserves them for its own language switching), so the
settings UI offers only the shortcuts it can actually register.

`disable_encoding_shortcut=false` additionally registers Ctrl+Alt+E, which
toggles `preferred_encoding` (Unicode ↔ Bijoy ANSI) live and persists the
choice; `true` keeps the chord free for other applications.

`preferred_encoding=ansi` re-encodes every committed Bangla string into the
legacy Bijoy ASCII encoding through `unicode::encode_bijoy` (with a Unicode
fallback when a grapheme has no legacy byte); `unicode` passes text through
unchanged. `change_locale_to_bn_bd_when_layout_not_english=true` makes the
TSF text service re-activate the Suborno bn-BD language profile on the typing
thread whenever a Bangla layout becomes active.

## Keyboard layouts

The visible layout list is `english`, `suborno-phonetic`, `suborno-classic`,
`suborno-smart`, `khipro`, `probhat`, `national`, shown as English,
বাংলা (Suborno), বাংলা (Suborno Classic), Suborno Smart, Khipro, Probhat, National — user-facing
names never contain the implementation term "Phonetic". The `english` layout
is a passthrough mode: the text service does not eat keys, so typing produces
plain Latin text.

## Implementation boundary

Every key above now has an implemented runtime consumer and is exposed in the
desktop UI (the earlier "schema-first, consumer-pending" caveat no longer
applies to any listed key):

| Key group | Runtime consumer |
|---|---|
| theme_id | `ui::set_active_theme` — settings window, topbar, menus, About, OSK and the TSF candidate window all read the active palette |
| font_family / use_os_default_bangla_font | shell UI + topbar + candidate window fonts |
| show_candidate_window / candidate_orientation / candidate_fixed_height / enable_sliding_effect / follow_caret / follow_mouse_pointer_if_caret_tracking_fails | `CandidateListUIElement` (vertical list ↔ horizontal row, fixed five-row paging, slide-in animation, caret/mouse/pinned anchoring) |
| next_word_prediction_enabled | bigram transition model in `CandidateEngine` (dictionary-backed, learning-gated) |
| switching_mode | TSF text service per-context layout overrides (per-application-window) vs engine-follow (global) |
| layout_switch_shortcut | registered global layout-cycling hotkey |
| disable_encoding_shortcut / preferred_encoding | Ctrl+Alt+E hotkey + Bijoy ANSI commit-path encoding |
| map_numpad_to_bangla_numbers / ignore_caps_lock_state_in_bangla / select_candidate_using_number_keys | key handling in the text service |
| change_locale_to_bn_bd_when_layout_not_english | bn-BD profile re-activation on the typing thread |
| idle_dimming_enabled / idle_opacity_percent / idle_time_seconds / remember_topbar_position / topbar_position_x/y | floating topbar |
| start_mode / last_ui_mode / remember_last_input_layout / start_with_windows | launch behavior and the Run registry entry |
| last_bangla_layout_id | English↔বাংলা quick toggle (topbar mode icon, OSK mode buttons, tray) |

A persisted option must not be represented in the UI as operational until its
runtime consumer is implemented and tested — this contract stays in force for
future keys.
