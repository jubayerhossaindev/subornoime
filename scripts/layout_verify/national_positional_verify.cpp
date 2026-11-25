// National (Jatiyo) positional layout — engine evidence verifier.

//

// Re-declares the expected key->output facts independently (transcribed from

// docs/specifications/national-keymap.md, which itself was transcribed from

// the official National Keyboard diagram in Unicode L2/19-079 Figure 4 and

// cross-checked against the BornoTSF mapping facts) and drives the REAL

// engine (transliterate for base/shift keystrokes, real session KeyEvents

// with Ctrl+Alt for the Link/AltGr layers). Emits a CSV evidence file.

//

// Build (repo root):

//   g++ -std=c++20 -I include scripts/layout_verify/national_positional_verify.cpp \

//       build/ubuntu-ci/libsuborno_national.a build/ubuntu-ci/libsuborno_unicode.a \

//       -o build/ubuntu-ci/national_positional_verify

// Run:

//   ./build/ubuntu-ci/national_positional_verify > \

//       reports/standards/evidence-raw/bds_national_positional_engine_rows.csv

#include <cstdint>
#include <cstdio>
#include <string>

#include "suborno/layouts/national/national_layout.hpp"

namespace {

struct ExpectRow {
    const char* key;
    const char* layer;
    std::uint16_t scan_code;
    bool shift;
    bool altgr;
    const char* driver_keys;  // ASCII keystrokes for base/shift rows

    const char* expected_utf8;
};

// Full 4-layer table, independently transcribed for verification.

constexpr ExpectRow rows[] = {
    // Digit row

    {"1", "base", 0x02, false, false, "1", "\xE0\xA7\xA7"},
    {"2", "base", 0x03, false, false, "2", "\xE0\xA7\xA8"},
    {"3", "base", 0x04, false, false, "3", "\xE0\xA7\xA9"},
    {"4", "base", 0x05, false, false, "4", "\xE0\xA7\xAA"},
    {"5", "base", 0x06, false, false, "5", "\xE0\xA7\xAB"},
    {"6", "base", 0x07, false, false, "6", "\xE0\xA7\xAC"},
    {"7", "base", 0x08, false, false, "7", "\xE0\xA7\xAD"},
    {"8", "base", 0x09, false, false, "8", "\xE0\xA7\xAE"},
    {"9", "base", 0x0A, false, false, "9", "\xE0\xA7\xAF"},
    {"0", "base", 0x0B, false, false, "0", "\xE0\xA7\xA6"},
    {"Shift+1", "shift", 0x02, true, false, "!", "!"},
    {"Shift+2", "shift", 0x03, true, false, "@", "@"},
    {"Shift+3", "shift", 0x04, true, false, "#", "#"},
    {"Shift+4", "shift", 0x05, true, false, "$", "\xE0\xA7\xB3"},  // ৳ U+09F3

    {"Shift+5", "shift", 0x06, true, false, "%", "%"},
    {"Shift+6", "shift", 0x07, true, false, "^", "^"},
    {"Shift+7", "shift", 0x08, true, false, "&", "\xE0\xA6\x81"},  // ঁ U+0981

    {"Shift+8", "shift", 0x09, true, false, "*", "*"},
    {"Shift+9", "shift", 0x0A, true, false, "(", "("},
    {"Shift+0", "shift", 0x0B, true, false, ")", ")"},
    // Link digit row (bottom-left) and Link+Shift digit row (top-left)

    {"Link+1", "altgr", 0x02, false, true, "", "\xE0\xA7\xB4"},  // ৴ U+09F4

    {"Link+2", "altgr", 0x03, false, true, "", "\xE0\xA7\xB5"},  // U+09F5

    {"Link+3", "altgr", 0x04, false, true, "", "\xE0\xA7\xB6"},  // U+09F6

    {"Link+4", "altgr", 0x05, false, true, "", "\xE0\xA7\xB7"},  // U+09F7

    {"Link+5", "altgr", 0x06, false, true, "", "\xE0\xA7\xB8"},  // U+09F8

    {"Link+6", "altgr", 0x07, false, true, "", "\xE0\xA7\xA0"},  // ৠ U+09E0

    {"Link+7", "altgr", 0x08, false, true, "", "\xE0\xA7\xBA"},  // ৺ U+09FA

    {"Link+8", "altgr", 0x09, false, true, "", "\xE0\xA7\xA1"},  // ৡ U+09E1

    {"Link+9", "altgr", 0x0A, false, true, "", "\xE0\xA6\x8C"},  // ঌ U+098C

    {"Link+0", "altgr", 0x0B, false, true, "", "\xE0\xA7\xB9"},  // ৹ U+09F9

    {"Link+Shift+1", "shift_altgr", 0x02, true, true, "", "\xE0\xA6\x80"},  // ঀ U+0980

    {"Link+Shift+2", "shift_altgr", 0x03, true, true, "", ""},
    {"Link+Shift+3", "shift_altgr", 0x04, true, true, "", "\xE0\xA6\xBD"},  // ঽ U+09BD

    {"Link+Shift+4", "shift_altgr", 0x05, true, true, "", "\xE0\xA7\xB2"},  // ৲ U+09F2

    {"Link+Shift+5", "shift_altgr", 0x06, true, true, "", "\xE0\xA7\x84"},  // ৄ U+09C4

    {"Link+Shift+6", "shift_altgr", 0x07, true, true, "", "\xE0\xA7\xA0"},  // ৠ U+09E0

    {"Link+Shift+7", "shift_altgr", 0x08, true, true, "", "\xE0\xA7\xBC"},  // ৾ U+09FC

    {"Link+Shift+8", "shift_altgr", 0x09, true, true, "", "\xE0\xA7\xA3"},  // ৣ U+09E3

    {"Link+Shift+9", "shift_altgr", 0x0A, true, true, "", "\xE0\xA7\xA2"},  // ৢ U+09E2

    {"Link+Shift+0", "shift_altgr", 0x0B, true, true, "", "\xE0\xA7\xBD"},  // ৽ U+09FD

    // Q row

    {"q", "base", 0x10, false, false, "q", "ঙ"},
    {"w", "base", 0x11, false, false, "w", "\xE0\xA6\xAF"},
    {"e", "base", 0x12, false, false, "e", "\xE0\xA6\xA1"},
    {"r", "base", 0x13, false, false, "r", "\xE0\xA6\xAA"},
    {"t", "base", 0x14, false, false, "t", "\xE0\xA6\x9F"},
    {"y", "base", 0x15, false, false, "y", "\xE0\xA6\x9A"},
    {"u", "base", 0x16, false, false, "u", "\xE0\xA6\x9C"},
    {"i", "base", 0x17, false, false, "i", "\xE0\xA6\xB9"},
    {"o", "base", 0x18, false, false, "o", "\xE0\xA6\x97"},
    {"p", "base", 0x19, false, false, "p", "\xE0\xA6\xA1\xE0\xA6\xBC"},  // ড় decomposed

    {"Shift+Q", "shift", 0x10, true, false, "Q", "\xE0\xA6\x82"},  // ং U+0982

    {"Shift+W", "shift", 0x11, true, false, "W", "\xE0\xA6\xAF\xE0\xA6\xBC"},  // য় decomposed

    {"Shift+E", "shift", 0x12, true, false, "E", "\xE0\xA6\xA2"},
    {"Shift+R", "shift", 0x13, true, false, "R", "\xE0\xA6\xAB"},
    {"Shift+T", "shift", 0x14, true, false, "T", "\xE0\xA6\xA0"},
    {"Shift+Y", "shift", 0x15, true, false, "Y", "\xE0\xA6\x9B"},
    {"Shift+U", "shift", 0x16, true, false, "U", "\xE0\xA6\x9D"},
    {"Shift+I", "shift", 0x17, true, false, "I", "\xE0\xA6\x9E"},
    {"Shift+O", "shift", 0x18, true, false, "O", "\xE0\xA6\x98"},
    {"Shift+P", "shift", 0x19, true, false, "P", "\xE0\xA6\xA2\xE0\xA6\xBC"},  // ঢ় decomposed

    // A row

    {"a", "base", 0x1E, false, false, "a", "\xE0\xA7\x83"},  // ৃ U+09C3

    {"s", "base", 0x1F, false, false, "s", "\xE0\xA7\x81"},  // ু U+09C1

    {"d", "base", 0x20, false, false, "d", "\xE0\xA6\xBF"},  // ি U+09BF

    {"f", "base", 0x21, false, false, "f", "\xE0\xA6\xBE"},  // া U+09BE

    {"g", "base", 0x22, false, false, "g", "\xE0\xA7\x8D"},  // ্ U+09CD

    {"h", "base", 0x23, false, false, "h", "\xE0\xA6\xAC"},
    {"j", "base", 0x24, false, false, "j", "\xE0\xA6\x95"},
    {"k", "base", 0x25, false, false, "k", "\xE0\xA6\xA4"},
    {"l", "base", 0x26, false, false, "l", "\xE0\xA6\xA6"},
    {"Shift+A", "shift", 0x1E, true, false, "A", "\xE0\xA6\xB0\xE0\xA7\x8D"},  // র্ reph

    {"Shift+S", "shift", 0x1F, true, false, "S", "\xE0\xA7\x82"},  // ূ U+09C2

    {"Shift+D", "shift", 0x20, true, false, "D", "\xE0\xA7\x80"},  // ী U+09C0

    {"Shift+F", "shift", 0x21, true, false, "F", "\xE0\xA6\x85"},  // অ U+0985

    {"Shift+G", "shift", 0x22, true, false, "G", "\xE0\xA5\xA4"},  // । U+0964

    {"Shift+H", "shift", 0x23, true, false, "H", "\xE0\xA6\xAD"},
    {"Shift+J", "shift", 0x24, true, false, "J", "\xE0\xA6\x96"},
    {"Shift+K", "shift", 0x25, true, false, "K", "\xE0\xA6\xA5"},
    {"Shift+L", "shift", 0x26, true, false, "L", "\xE0\xA6\xA7"},
    // Z row

    {"z", "base", 0x2C, false, false, "z", "\xE0\xA7\x8D\xE0\xA6\xB0"},  // ্র

    {"x", "base", 0x2D, false, false, "x", "\xE0\xA6\x93"},
    {"c", "base", 0x2E, false, false, "c", "\xE0\xA7\x87"},  // ে U+09C7

    {"v", "base", 0x2F, false, false, "v", "\xE0\xA6\xB0"},
    {"b", "base", 0x30, false, false, "b", "\xE0\xA6\xA8"},
    {"n", "base", 0x31, false, false, "n", "\xE0\xA6\xB8"},
    {"m", "base", 0x32, false, false, "m", "\xE0\xA6\xAE"},
    {"Shift+Z", "shift", 0x2C, true, false, "Z", "\xE0\xA7\x8D\xE0\xA6\xAF"},  // ্য

    {"Shift+X", "shift", 0x2D, true, false, "X", "\xE0\xA7\x97"},  // ৗ U+09D7

    {"Shift+C", "shift", 0x2E, true, false, "C", "\xE0\xA7\x88"},  // ৈ U+09C8

    {"Shift+V", "shift", 0x2F, true, false, "V", "\xE0\xA6\xB2"},
    {"Shift+B", "shift", 0x30, true, false, "B", "\xE0\xA6\xA3"},
    {"Shift+N", "shift", 0x31, true, false, "N", "\xE0\xA6\xB7"},
    {"Shift+M", "shift", 0x32, true, false, "M", "\xE0\xA6\xB6"},
    // Punctuation

    {"\\", "base", 0x2B, false, false, "\\", "\xE0\xA7\x8E"},  // ৎ U+09CE

    {"Shift+\\", "shift", 0x2B, true, false, "|", "\xE0\xA6\x83"},  // ঃ U+0983

    {"Link+-", "altgr", 0x0C, false, true, "", "\xC3\x97"},  // × U+00D7

    {"Link+Shift+-", "shift_altgr", 0x0C, true, true, "", "\xC3\xB7"},  // ÷ U+00F7

    {"Link+Shift+G", "shift_altgr", 0x22, true, true, "", "\xE0\xA5\xA5"},  // ॥ U+0965

    // Link vowel row (independent vowels)

    {"Link+f", "altgr", 0x21, false, true, "", "\xE0\xA6\x86"},  // আ U+0986

    {"Link+d", "altgr", 0x20, false, true, "", "\xE0\xA6\x87"},  // ই U+0987

    {"Link+Shift+D", "shift_altgr", 0x20, true, true, "", "\xE0\xA6\x88"},  // ঈ U+0988

    {"Link+s", "altgr", 0x1F, false, true, "", "\xE0\xA6\x89"},  // উ U+0989

    {"Link+Shift+S", "shift_altgr", 0x1F, true, true, "", "\xE0\xA6\x8A"},  // ঊ U+098A

    {"Link+a", "altgr", 0x1E, false, true, "", "\xE0\xA6\x8B"},  // ঋ U+098B

    {"Link+c", "altgr", 0x2E, false, true, "", "\xE0\xA6\x8F"},  // এ U+098F

    {"Link+Shift+C", "shift_altgr", 0x2E, true, true, "", "\xE0\xA6\x90"},  // ঐ U+0990

    {"Link+x", "altgr", 0x2D, false, true, "", "\xE0\xA7\x8B"},  // ো U+09CB

    {"Link+Shift+X", "shift_altgr", 0x2D, true, true, "", "\xE0\xA6\x94"},  // ঔ U+0994

    // Deliberate exclusions (must stay empty)

    {"Link+,", "altgr", 0x33, false, true, "", ""},  // Assamese র (U+09F0) excluded

    {"Link+.", "altgr", 0x34, false, true, "", ""},  // Assamese ৱ (U+09F1) excluded

};

std::string codepoints(const std::string& text) {
    std::string out;
    std::size_t i = 0;
    while (i < text.size()) {
        const auto byte = static_cast<unsigned char>(text[i]);
        std::size_t width = 1;
        unsigned int cp = byte;
        if (byte >= 0xF0U && i + 3 < text.size()) {
            width = 4;
            cp = ((byte & 0x07U) << 18U) | (static_cast<unsigned char>(text[i + 1]) & 0x3FU) << 12U
                 | (static_cast<unsigned char>(text[i + 2]) & 0x3FU) << 6U
                 | (static_cast<unsigned char>(text[i + 3]) & 0x3FU);
        } else if (byte >= 0xE0U && i + 2 < text.size()) {
            width = 3;
            cp = ((byte & 0x0FU) << 12U) | (static_cast<unsigned char>(text[i + 1]) & 0x3FU) << 6U
                 | (static_cast<unsigned char>(text[i + 2]) & 0x3FU);
        } else if (byte >= 0xC0U && i + 1 < text.size()) {
            width = 2;
            cp = ((byte & 0x1FU) << 6U) | (static_cast<unsigned char>(text[i + 1]) & 0x3FU);
        }
        char buffer[16];
        std::snprintf(buffer, sizeof(buffer), "U+%04X ", cp);
        out += buffer;
        i += width;
    }
    if (!out.empty()) {
        out.pop_back();
    }
    return out;
}

// CSV quoting: wrap a field in double quotes when it contains a comma

// (key names like "Link+," — the comma key — would otherwise shift columns).

std::string csv_field(const char* raw) {
    const std::string value = raw;
    if (value.find(',') == std::string::npos) {
        return value;
    }
    return "\"" + value + "\"";
}

suborno::layout::KeyEvent make_event(std::uint16_t scan, bool shift, bool altgr) {
    suborno::layout::KeyEvent event{};
    event.modifiers.shift = shift;
    event.modifiers.control = altgr;
    event.modifiers.alt = altgr;
    event.scan_code = scan;
    event.key_down = true;
    return event;
}

}  // namespace

int main() {
    const suborno::layouts::NationalLayout layout;
    std::printf(
        "row_id,key,layer,driver,expected_utf8,actual_utf8,expected_codepoints,"
        "actual_codepoints,status\n");
    unsigned int pass = 0;
    unsigned int fail = 0;
    unsigned int row_id = 0;
    for (const ExpectRow& row : rows) {
        ++row_id;
        std::string actual;
        std::string driver;
        if (row.altgr) {
            driver =
                row.shift ? "session.append_key Ctrl+Alt+Shift" : "session.append_key Ctrl+Alt";
            auto session = layout.create_session(nullptr, {});
            session->append_key(make_event(row.scan_code, row.shift, true));
            actual = session->commit().text_utf8;
        } else {
            driver = row.shift ? "transliterate shift-keystroke" : "transliterate base-keystroke";
            actual = layout.transliterate(row.driver_keys);
        }
        const bool ok = actual == row.expected_utf8;
        if (ok) {
            ++pass;
        } else {
            ++fail;
        }
        std::printf("NAT-%03u,%s,%s,%s,%s,%s,%s,%s,%s\n",
                    row_id,
                    csv_field(row.key).c_str(),
                    row.layer,
                    driver.c_str(),
                    row.expected_utf8,
                    actual.c_str(),
                    codepoints(row.expected_utf8).c_str(),
                    codepoints(actual).c_str(),
                    ok ? "PASS" : "FAIL");
    }

    // Sequence behaviours through the string API.

    struct SeqRow {
        const char* name;
        const char* keys;
        const char* expected_utf8;
    };
    constexpr SeqRow seq_rows[] = {
        {"reph single consonant (kar-ma)",
         "jmA",
         "\xE0\xA6\x95\xE0\xA6\xB0\xE0\xA7\x8D\xE0\xA6\xAE"},  // ক র ্ ম

        {"reph on cluster",
         "jgjA",
         "\xE0\xA6\xB0\xE0\xA7\x8D\xE0\xA6\x95\xE0\xA7\x8D\xE0\xA6\x95"},  // র ্ ক ্ ক

        {"broken conjunct double hasanta",
         "jggj",
         "\xE0\xA6\x95\xE0\xA7\x8D\xE2\x80\x8C\xE0\xA6\x95"},  // ক ্ ZWNJ ক

        {"candrabindu aa-kar reorder (bam)",
         "h&f",
         "\xE0\xA6\xAC\xE0\xA6\xBE\xE0\xA6\x81"},  // ব া ঁ

        {"invalid A+aakar becomes AA letter", "Ff", "\xE0\xA6\x86"},  // আ

        {"ra-kar reorder (kru)",
         "jsz",
         "\xE0\xA6\x95\xE0\xA7\x8D\xE0\xA6\xB0\xE0\xA7\x81"},  // ক ্ র ু

        {"ya-phala reorder (kyu)",
         "jsZ",
         "\xE0\xA6\x95\xE0\xA7\x8D\xE0\xA6\xAF\xE0\xA7\x81"},  // ক ্ য ু

        {"ra ya-phala with ZWJ",
         "vZ",
         "\xE0\xA6\xB0\xE2\x80\x8D\xE0\xA7\x8D\xE0\xA6\xAF"},  // র ZWJ ্ য

        {"o-kar composition (ko)", "jcf", "\xE0\xA6\x95\xE0\xA7\x8B"},  // ক ো

        {"au-kar composition (kau)", "jcX", "\xE0\xA6\x95\xE0\xA7\x8C"},  // ক ৌ

        {"word bangla",
         "hfQVf",
         "\xE0\xA6\xAC\xE0\xA6\xBE\xE0\xA6\x82\xE0\xA6\xB2\xE0\xA6\xBE"},  // বাংলা

        {"word nomoshkar",
         "bmngjfv",
         "\xE0\xA6\xA8\xE0\xA6\xAE\xE0\xA6\xB8\xE0\xA7\x8D\xE0\xA6\x95\xE0\xA6\xBE"
         "\xE0\xA6\xB0"},  // নমস্কার

    };
    for (const SeqRow& row : seq_rows) {
        ++row_id;
        const std::string actual = layout.transliterate(row.keys);
        const bool ok = actual == row.expected_utf8;
        if (ok) {
            ++pass;
        } else {
            ++fail;
        }
        std::printf("NAT-%03u,%s,sequence,transliterate,%s,%s,%s,%s,%s\n",
                    row_id,
                    csv_field(row.name).c_str(),
                    row.expected_utf8,
                    actual.c_str(),
                    codepoints(row.expected_utf8).c_str(),
                    codepoints(actual).c_str(),
                    ok ? "PASS" : "FAIL");
    }

    std::fprintf(stderr, "SUMMARY rows=%u pass=%u fail=%u\n", row_id, pass, fail);
    return fail == 0 ? 0 : 1;
}
