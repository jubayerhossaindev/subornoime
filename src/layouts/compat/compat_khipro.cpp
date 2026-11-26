// Khipro layout engine adapter — Suborno's own implementation.
//
// Loads the Khipro .mim keymap lazily (first transliterate() call), probes
// the same search order as the dictionary loader (Suborno user path ->
// Suborno program-data -> legacy legacy reference IME paths -> bundled asset), and
// delegates to the SpecEngine state machine. Pass-through when no keymap is
// found.
//
// SPDX-License-Identifier: GPL-3.0-or-later
// Behaviour provenance: the legacy reference behaviour contract; see LICENSE and THIRD_PARTY_NOTICES.md.
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include "compat_layouts.hpp"
#include "compat_mim_engine.hpp"

namespace compat {
namespace {

std::filesystem::path env_folder(const char* name) {
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
    return std::filesystem::path(environment_path);
}

// Search order: engine/test-injected bundled asset first (mirrors the
// dictionary loader's override hook), then Suborno and legacy legacy reference IME
// install locations, then the asset tree next to the working directory.
std::vector<std::filesystem::path> spec_candidates() {
    std::vector<std::filesystem::path> candidate_paths;

    const std::wstring& injected = khipro_spec_override();
    if (!injected.empty()) {
        candidate_paths.push_back(std::filesystem::path(injected));
    }

    const std::filesystem::path app_data = env_folder("APPDATA");
    const std::filesystem::path program_data = env_folder("PROGRAMDATA");
    const char* vendors[] = {"SubornoIME", "legacy reference IME"};
    const char* names[] = {"bn-khipro.mim", "khipro.mim"};
    for (const char* vendor : vendors) {
        for (const char* file_name : names) {
            if (!app_data.empty()) {
                candidate_paths.push_back(app_data / vendor / "layouts" / file_name);
            }
            if (!program_data.empty()) {
                candidate_paths.push_back(program_data / vendor / "layouts" / file_name);
            }
        }
    }
    candidate_paths.push_back(std::filesystem::path("assets") / "layouts" / "bn-khipro.mim");
    return candidate_paths;
}

std::string read_spec() {
    for (const std::filesystem::path& path : spec_candidates()) {
        std::error_code probe_error;
        if (path.empty() || !std::filesystem::exists(path, probe_error)
            || std::filesystem::is_directory(path, probe_error)) {
            continue;
        }
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            continue;
        }
        std::stringstream spec_stream;
        spec_stream << file.rdbuf();
        const std::string spec_text = spec_stream.str();
        if (!spec_text.empty()) {
            return spec_text;
        }
    }
    return "";
}

std::unique_ptr<khipro::SpecEngine>& engine_slot() {
    static std::unique_ptr<khipro::SpecEngine> engine;
    return engine;
}

std::mutex& engine_mutex() {
    static std::mutex mutex;
    return mutex;
}

// Lazily constructs the engine; caller holds engine_mutex().
void ensure_engine_locked() {
    if (!engine_slot()) {
        const std::string spec_text = read_spec();
        if (!spec_text.empty()) {
            engine_slot() = std::make_unique<khipro::SpecEngine>(spec_text);
        }
    }
}

}  // namespace

KhiproCompatEngine::KhiproCompatEngine() {
    std::lock_guard<std::mutex> lock(engine_mutex());
    ensure_engine_locked();
}

std::string KhiproCompatEngine::transliterate(const std::string& input_text) {
    std::lock_guard<std::mutex> lock(engine_mutex());
    ensure_engine_locked();
    if (engine_slot()) {
        return engine_slot()->convert_keystroke_text(input_text);
    }
    return input_text;
}

}  // namespace compat
