#pragma once

// Localized UI strings for the Suborno desktop shell.
//
// The user-visible text lives in `data/i18n/<locale>/ui.json` as nested
// objects with dotted leaf keys ("topbar.menu.settings"). This loader
// flattens the file into that dotted form and answers lookups; the English
// literals compiled at every call site remain the permanent fallback, so a
// missing file, an unparsable file, or a single missing key can never
// remove a string from the UI — the surface just shows the English text.
//
// The loader is portable (no Windows headers) so the parser, the fallback
// rules and the shipped locale files themselves are unit-tested on every
// platform, not only where the GUI runs.
//
// Threading: the desktop shell calls load()/clear()/get() from its UI
// thread only. The table is deliberately a process-wide singleton shared by
// every surface (settings window, topbar, OSK, About dialog).

#include <filesystem>
#include <string>
#include <vector>

namespace suborno::core::ui_strings {

/// Locales the shell can switch to. "system" is resolved by the caller (the
/// OS UI language is a platform question) into one of these before loading.
enum class Locale {
    english,
    bangla
};

// Maps the settings value onto a locale; unknown or empty values are
// English (the language every call site can render without any file).
Locale locale_from_setting(const std::string& value);

// The JSON file for a locale inside an i18n root directory
// (`<root>/<locale>/ui.json`).
std::filesystem::path locale_file(const std::filesystem::path& i18n_root, Locale locale);

// Parses one ui.json and installs it as the active table. On any failure
// the previously active table is left untouched and `error` carries a short
// reason (empty string on success). Clearing first is never implicit.
bool load(const std::filesystem::path& json_path, std::string& error);

// Drops every override so get() answers empty again (used by tests and by
// a locale reset).
void clear();

// The number of leaf string entries currently loaded.
std::size_t size();

// Value for `key`, or an empty string when the active table does not carry
// it — callers render their compiled English fallback in that case. The
// reference stays valid until the next load()/clear().
const std::string& get(const char* key);

// The dotted keys currently loaded, sorted (diagnostics and tests).
std::vector<std::string> keys();

}  // namespace suborno::core::ui_strings
