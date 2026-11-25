// Localized UI strings: minimal JSON reader + dotted-key table. See the
// header for the contract. The parser intentionally handles exactly the
// structure data/i18n/*/ui.json uses (objects, strings, escapes); anything
// else is a load failure that keeps the previous table, and every call site
// keeps its compiled English fallback.

#include "suborno/core/ui_strings.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <map>
#include <utility>

namespace suborno::core::ui_strings {
namespace {

std::map<std::string, std::string>& table() {
    static std::map<std::string, std::string> strings;
    return strings;
}

bool is_space(char character) {
    return character == ' ' || character == '\t' || character == '\r' || character == '\n';
}

// Appends the UTF-8 encoding of one Unicode scalar value.
void append_utf8(std::string& out, std::uint32_t code_point) {
    if (code_point <= 0x7F) {
        out.push_back(static_cast<char>(code_point));
    } else if (code_point <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | (code_point >> 6)));
        out.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
    } else if (code_point <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | (code_point >> 12)));
        out.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (code_point >> 18)));
        out.push_back(static_cast<char>(0x80 | ((code_point >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
    }
}

std::uint32_t hex4(const char* text) {
    std::uint32_t value = 0;
    for (int index = 0; index < 4; ++index) {
        const char character = text[index];
        value <<= 4;
        if (character >= '0' && character <= '9') {
            value |= static_cast<std::uint32_t>(character - '0');
        } else if (character >= 'a' && character <= 'f') {
            value |= static_cast<std::uint32_t>(character - 'a' + 10);
        } else if (character >= 'A' && character <= 'F') {
            value |= static_cast<std::uint32_t>(character - 'A' + 10);
        } else {
            return 0xFFFFFD;  // sentinel: not a hex digit
        }
    }
    return value;
}

// A minimal JSON string reader. `cursor` sits on the opening quote; on
// success it moves past the closing quote and `out` carries the decoded
// UTF-8 bytes.
bool parse_string(const char*& cursor, const char* end, std::string& out) {
    out.clear();
    if (cursor == end || *cursor != '"') {
        return false;
    }
    ++cursor;
    while (cursor != end) {
        const unsigned char character = static_cast<unsigned char>(*cursor);
        if (character == '"') {
            ++cursor;
            return true;
        }
        if (character == '\\') {
            ++cursor;
            if (cursor == end) {
                return false;
            }
            switch (*cursor) {
            case '"':
                out.push_back('"');
                break;
            case '\\':
                out.push_back('\\');
                break;
            case '/':
                out.push_back('/');
                break;
            case 'b':
                out.push_back('\b');
                break;
            case 'f':
                out.push_back('\f');
                break;
            case 'n':
                out.push_back('\n');
                break;
            case 'r':
                out.push_back('\r');
                break;
            case 't':
                out.push_back('\t');
                break;
            case 'u': {
                if (end - cursor < 5) {
                    return false;
                }
                std::uint32_t code_point = hex4(cursor + 1);
                if (code_point > 0xFFFF) {
                    return false;
                }
                cursor += 4;
                // Surrogate pair: high surrogate followed by a \u low one.
                if (code_point >= 0xD800 && code_point <= 0xDBFF) {
                    if (end - cursor < 7 || cursor[1] != '\\' || cursor[2] != 'u') {
                        return false;
                    }
                    const std::uint32_t low = hex4(cursor + 3);
                    if (low < 0xDC00 || low > 0xDFFF) {
                        return false;
                    }
                    code_point = 0x10000 + ((code_point - 0xD800) << 10) + (low - 0xDC00);
                    cursor += 6;
                } else if (code_point >= 0xDC00 && code_point <= 0xDFFF) {
                    return false;  // unpaired low surrogate
                }
                append_utf8(out, code_point);
                break;
            }
            default:
                return false;
            }
            ++cursor;
            continue;
        }
        // Control characters must be escaped in JSON; raw UTF-8 bytes pass
        // through unchanged.
        if (character < 0x20) {
            return false;
        }
        out.push_back(static_cast<char>(character));
        ++cursor;
    }
    return false;  // unterminated
}

// Strict value skipper: consumes one non-object, non-string value (number,
// true, false, null) up to the next structural byte. Structural validity is
// enforced by the caller (it must see ',' '}' ']' next).
bool skip_scalar(const char*& cursor, const char* end) {
    while (cursor != end && is_space(*cursor)) {
        ++cursor;
    }
    if (cursor == end) {
        return false;
    }
    if (*cursor == '"') {
        std::string ignored;
        return parse_string(cursor, end, ignored);
    }
    if (*cursor == '{') {
        // Strict object skip (values inside arrays or deeper nesting): same
        // member grammar as flatten, but nothing is stored.
        ++cursor;
        while (cursor != end && is_space(*cursor)) {
            ++cursor;
        }
        if (cursor != end && *cursor == '}') {
            ++cursor;
            return true;
        }
        while (true) {
            while (cursor != end && is_space(*cursor)) {
                ++cursor;
            }
            std::string ignored;
            if (!parse_string(cursor, end, ignored)) {
                return false;
            }
            while (cursor != end && is_space(*cursor)) {
                ++cursor;
            }
            if (cursor == end || *cursor != ':') {
                return false;
            }
            ++cursor;
            if (!skip_scalar(cursor, end)) {
                return false;
            }
            while (cursor != end && is_space(*cursor)) {
                ++cursor;
            }
            if (cursor != end && *cursor == ',') {
                ++cursor;
                continue;
            }
            if (cursor != end && *cursor == '}') {
                ++cursor;
                return true;
            }
            return false;
        }
    }
    if (*cursor == '[') {
        ++cursor;
        while (cursor != end && is_space(*cursor)) {
            ++cursor;
        }
        if (cursor != end && *cursor == ']') {
            ++cursor;
            return true;
        }
        while (true) {
            if (!skip_scalar(cursor, end)) {
                return false;
            }
            while (cursor != end && is_space(*cursor)) {
                ++cursor;
            }
            if (cursor != end && *cursor == ',') {
                ++cursor;
                continue;
            }
            if (cursor != end && *cursor == ']') {
                ++cursor;
                return true;
            }
            return false;
        }
    }
    // number / true / false / null: validate the first byte, then consume
    // the token up to the next structural byte. An empty or structurally
    // misplaced value ("{\"x\": }", "[1, ]", "{: 1}") is rejected here.
    {
        const char character = *cursor;
        const bool scalar_start = character == '-' || character == '+'
                                  || (character >= '0' && character <= '9') || character == 't'
                                  || character == 'f' || character == 'n';
        if (!scalar_start) {
            return false;
        }
        while (cursor != end && !is_space(*cursor) && *cursor != ',' && *cursor != '}'
               && *cursor != ']') {
            ++cursor;
        }
        return true;
    }
}

// Depth-first flatten: nested objects join with '.', leaf strings store.
// Strictly validates; on any malformed input it returns false and the
// caller discards the whole file.
bool flatten(const char*& cursor, const char* end, const std::string& prefix) {
    while (cursor != end && is_space(*cursor)) {
        ++cursor;
    }
    if (cursor == end || *cursor != '{') {
        return false;
    }
    ++cursor;
    while (cursor != end && is_space(*cursor)) {
        ++cursor;
    }
    if (cursor != end && *cursor == '}') {
        ++cursor;
        return true;
    }
    while (true) {
        while (cursor != end && is_space(*cursor)) {
            ++cursor;
        }
        std::string member;
        if (!parse_string(cursor, end, member)) {
            return false;
        }
        while (cursor != end && is_space(*cursor)) {
            ++cursor;
        }
        if (cursor == end || *cursor != ':') {
            return false;
        }
        ++cursor;
        while (cursor != end && is_space(*cursor)) {
            ++cursor;
        }
        std::string key = prefix.empty() ? member : prefix + '.' + member;
        if (cursor != end && *cursor == '"') {
            std::string value;
            if (!parse_string(cursor, end, value)) {
                return false;
            }
            table().emplace(std::move(key), std::move(value));
        } else if (cursor != end && *cursor == '{') {
            if (!flatten(cursor, end, key)) {
                return false;
            }
        } else if (!skip_scalar(cursor, end)) {
            return false;
        }
        while (cursor != end && is_space(*cursor)) {
            ++cursor;
        }
        if (cursor != end && *cursor == ',') {
            ++cursor;
            continue;
        }
        if (cursor != end && *cursor == '}') {
            ++cursor;
            return true;
        }
        return false;  // malformed: missing ',' or '}'
    }
}

const std::string& empty_string() {
    static const std::string empty;
    return empty;
}

}  // namespace

Locale locale_from_setting(const std::string& value) {
    if (value == "bn" || value == "bangla") {
        return Locale::bangla;
    }
    return Locale::english;
}

std::filesystem::path locale_file(const std::filesystem::path& i18n_root, Locale locale) {
    const char* name = locale == Locale::bangla ? "bn" : "en";
    return i18n_root / name / "ui.json";
}

bool load(const std::filesystem::path& json_path, std::string& error) {
    error.clear();
    std::FILE* file = nullptr;
#ifdef _WIN32
    _wfopen_s(&file, json_path.c_str(), L"rb");
#else
    file = std::fopen(json_path.c_str(), "rb");
#endif
    if (file == nullptr) {
        error = "ui.json not found: " + json_path.string();
        return false;
    }
    std::string contents;
    char buffer[4096];
    std::size_t read = 0;
    while ((read = std::fread(buffer, 1, sizeof(buffer), file)) > 0) {
        contents.append(buffer, read);
    }
    const bool read_error = std::ferror(file) != 0;
    std::fclose(file);
    if (read_error) {
        error = "ui.json read failed: " + json_path.string();
        return false;
    }

    const char* cursor = contents.c_str();
    const char* end = cursor + contents.size();
    while (cursor != end && is_space(*cursor)) {
        ++cursor;
    }
    if (cursor == end || *cursor != '{') {
        error = "ui.json is not a JSON object: " + json_path.string();
        return false;
    }
    // Flatten into a scratch table first, so an unparsable tail never
    // replaces the active strings with a half-read table.
    std::map<std::string, std::string> previous;
    previous.swap(table());
    if (!flatten(cursor, end, std::string())) {
        error = "ui.json is malformed: " + json_path.string();
        previous.swap(table());  // restore the previous strings untouched
        return false;
    }
    while (cursor != end && is_space(*cursor)) {
        ++cursor;
    }
    if (cursor != end) {
        error = "ui.json has trailing content after the root object: " + json_path.string();
        previous.swap(table());  // restore the previous strings untouched
        return false;
    }
    return true;
}

void clear() {
    table().clear();
}

std::size_t size() {
    return table().size();
}

const std::string& get(const char* key) {
    if (key == nullptr) {
        return empty_string();
    }
    const auto found = table().find(key);
    return found == table().end() ? empty_string() : found->second;
}

std::vector<std::string> keys() {
    std::vector<std::string> all;
    all.reserve(table().size());
    for (const auto& entry : table()) {
        all.push_back(entry.first);
    }
    return all;
}

}  // namespace suborno::core::ui_strings
