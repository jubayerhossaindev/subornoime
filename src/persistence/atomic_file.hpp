#pragma once

#include <filesystem>
#include <string_view>
#include <system_error>

namespace suborno::persistence {

std::error_code write_file_atomically(const std::filesystem::path& destination,
                                      std::string_view contents);

// Renames an existing file beside itself under a unique, clearly marked name
// ("<name>.corrupt-<pid>-<n>") without altering or deleting its contents.
// Used to recover from settings files that cannot be parsed: the user's data
// stays on disk for inspection while the application starts from defaults.
// Returns an empty error_code on success or when the file does not exist;
// returns the rename failure otherwise (the original file stays untouched).
std::error_code quarantine_file(const std::filesystem::path& path);

}  // namespace suborno::persistence
