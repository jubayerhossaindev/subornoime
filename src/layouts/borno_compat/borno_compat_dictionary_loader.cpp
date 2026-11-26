// Autocorrect dictionary loader — Suborno's own implementation.
// See borno_compat_dictionary_loader.hpp for the design contract.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "borno_compat_dictionary_loader.hpp"

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::wstring& pinned_override_path() {
    static std::wstring override_file_path;
    return override_file_path;
}

std::filesystem::path environment_folder(const char* name) {
    std::string environment_path;
#if defined(_MSC_VER)
    char* environment_value = nullptr;
    std::size_t environment_value_size = 0;
    const int environment_status = _dupenv_s(&environment_value, &environment_value_size, name);
    if (environment_status != 0 || environment_value == nullptr) {
        std::free(environment_value);
        return {};
    }
    environment_path.assign(environment_value, environment_value_size > 0 ? environment_value_size - 1 : 0);
    std::free(environment_value);
#else
    const char* environment_value = std::getenv(name);
    if (environment_value == nullptr) {
        return {};
    }
    environment_path.assign(environment_value);
#endif
    if (environment_path.empty()) {
        return {};
    }
    // APPDATA/PROGRAMDATA are ASCII in practice; byte-wise widening matches
    // the deployment layout.
    return std::filesystem::path(environment_path);
}

bool is_usable_dictionary_file(const std::filesystem::path& candidate_path) {
    std::error_code probe_error;
    return std::filesystem::exists(candidate_path, probe_error)
           && !std::filesystem::is_directory(candidate_path, probe_error);
}

// Search order: pinned override, Suborno user, Suborno program-data, legacy
// BornoTSF user, legacy BornoTSF program-data, bundled asset next to the
// working directory.
std::vector<std::filesystem::path> dictionary_search_paths() {
    std::vector<std::filesystem::path> candidate_paths;
    const std::filesystem::path app_data_folder = environment_folder("APPDATA");
    const std::filesystem::path program_data_folder = environment_folder("PROGRAMDATA");

    const char* vendors[] = {"SubornoIME", "BornoTSF"};
    for (const char* vendor : vendors) {
        if (!app_data_folder.empty()) {
            candidate_paths.push_back(app_data_folder / vendor / "dictionaries" / "ac.json");
        }
        if (!program_data_folder.empty()) {
            candidate_paths.push_back(program_data_folder / vendor / "dictionaries" / "ac.json");
        }
    }
    candidate_paths.push_back(std::filesystem::path("assets") / "dictionaries" / "ac.json");
    return candidate_paths;
}

void skip_blank(const std::string& json_text, std::size_t* byte_offset) {
    while (*byte_offset < json_text.size()) {
        const char current_char = json_text[*byte_offset];
        if (current_char == ' ' || current_char == '\t' || current_char == '\r'
            || current_char == '\n') {
            ++(*byte_offset);
            continue;
        }
        break;
    }
}

// Reads a JSON string literal starting at the opening quote. \uXXXX escapes
// are decoded to UTF-8 (BMP encoding: 1 byte < 0x80, 2 bytes < 0x800,
// 3 bytes otherwise — matching the BMP-only corrections data).
std::string read_json_string(const std::string& json_text, std::size_t* byte_offset) {
    std::string decoded_text;
    if (*byte_offset >= json_text.size() || json_text[*byte_offset] != '"') {
        return decoded_text;
    }
    ++(*byte_offset);
    while (*byte_offset < json_text.size()) {
        const char current_char = json_text[(*byte_offset)++];
        if (current_char == '"') {
            break;
        }
        if (current_char != '\\' || *byte_offset >= json_text.size()) {
            decoded_text.push_back(current_char);
            continue;
        }
        const char escape_char = json_text[(*byte_offset)++];
        switch (escape_char) {
        case '"':
            decoded_text.push_back('"');
            break;
        case '\\':
            decoded_text.push_back('\\');
            break;
        case '/':
            decoded_text.push_back('/');
            break;
        case 'b':
            decoded_text.push_back('\b');
            break;
        case 'f':
            decoded_text.push_back('\f');
            break;
        case 'n':
            decoded_text.push_back('\n');
            break;
        case 'r':
            decoded_text.push_back('\r');
            break;
        case 't':
            decoded_text.push_back('\t');
            break;
        case 'u': {
            if (*byte_offset + 4 <= json_text.size()) {
                const std::string hex_digits = json_text.substr(*byte_offset, 4);
                *byte_offset += 4;
                unsigned int code_point = 0;
                std::stringstream hex_stream;
                hex_stream << std::hex << hex_digits;
                hex_stream >> code_point;
                if (code_point < 0x80) {
                    decoded_text.push_back(static_cast<char>(code_point));
                } else if (code_point < 0x800) {
                    decoded_text.push_back(static_cast<char>(0xC0 | (code_point >> 6)));
                    decoded_text.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
                } else {
                    decoded_text.push_back(static_cast<char>(0xE0 | (code_point >> 12)));
                    decoded_text.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
                    decoded_text.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
                }
            }
            break;
        }
        default:
            decoded_text.push_back(escape_char);
            break;
        }
    }
    return decoded_text;
}

}  // namespace

void DictionaryLoader::set_override_path(const std::wstring& override_file_path) {
    pinned_override_path() = override_file_path;
}

void DictionaryLoader::reset_override_path() {
    pinned_override_path().clear();
}

DictionaryLoader& DictionaryLoader::instance() {
    static DictionaryLoader loader_instance;
    return loader_instance;
}

std::wstring DictionaryLoader::resolve_dictionary_file_path() const {
    const std::wstring& pinned_path = pinned_override_path();
    if (!pinned_path.empty() && is_usable_dictionary_file(std::filesystem::path(pinned_path))) {
        return pinned_path;
    }
    for (const std::filesystem::path& candidate_path : dictionary_search_paths()) {
        if (!candidate_path.empty() && is_usable_dictionary_file(candidate_path)) {
            return candidate_path.wstring();
        }
    }
    return L"";
}

bool DictionaryLoader::parse_json_corrections(const std::string& json_text) {
    std::unordered_map<std::string, std::string> parsed_corrections;

    const std::size_t json_size = json_text.size();
    std::size_t byte_offset = 0;
    while (byte_offset < json_size && json_text[byte_offset] != '{') {
        ++byte_offset;
    }
    if (byte_offset >= json_size) {
        return false;
    }
    ++byte_offset;

    while (byte_offset < json_size) {
        skip_blank(json_text, &byte_offset);
        if (byte_offset < json_size && json_text[byte_offset] == ',') {
            ++byte_offset;
            continue;
        }
        if (byte_offset >= json_size || json_text[byte_offset] == '}') {
            break;
        }
        if (json_text[byte_offset] != '"') {
            ++byte_offset;
            continue;
        }

        const std::string input_text = read_json_string(json_text, &byte_offset);
        skip_blank(json_text, &byte_offset);
        if (byte_offset < json_size && json_text[byte_offset] == ':') {
            ++byte_offset;
        }
        skip_blank(json_text, &byte_offset);
        if (byte_offset < json_size && json_text[byte_offset] == '"') {
            const std::string correction_text = read_json_string(json_text, &byte_offset);
            parsed_corrections[input_text] = correction_text;
        }
    }

    if (parsed_corrections.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> map_lock(corrections_mutex_);
    correction_by_input_text_ = std::move(parsed_corrections);
    is_loaded_ = true;
    return true;
}

bool DictionaryLoader::load_corrections(const std::wstring& corrections_file_path) {
    const std::wstring dictionary_file_path =
        corrections_file_path.empty() ? resolve_dictionary_file_path() : corrections_file_path;
    if (dictionary_file_path.empty()) {
        return false;
    }

    std::ifstream dictionary_stream(std::filesystem::path(dictionary_file_path), std::ios::binary);
    if (!dictionary_stream.is_open()) {
        return false;
    }
    const std::string file_content((std::istreambuf_iterator<char>(dictionary_stream)),
                                   std::istreambuf_iterator<char>());
    return parse_json_corrections(file_content);
}

bool DictionaryLoader::lookup_correction(const std::string& input_text,
                                         std::string& correction_text) {
    std::unique_lock<std::mutex> map_lock(corrections_mutex_);
    if (!is_loaded_) {
        // Lazy load on the first miss; release the mutex while reading the
        // file, then re-acquire for the lookup.
        map_lock.unlock();
        if (!load_corrections()) {
            return false;
        }
        map_lock.lock();
    }

    const auto correction_iterator = correction_by_input_text_.find(input_text);
    if (correction_iterator == correction_by_input_text_.end()) {
        return false;
    }
    correction_text = correction_iterator->second;
    return true;
}

const std::unordered_map<std::string, std::string>& DictionaryLoader::corrections_map() const {
    return correction_by_input_text_;
}
