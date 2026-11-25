// BDS mission (COMPLETE BDS COMPLIANCE...) repertoire + normalization

// verification harness.

//

// Part A — full Bengali block sweep U+0980..U+09FF through the engine's

//          unicode module: UTF-8 validity, code-point count, UTF-16 units,

//          is_bengali_codepoint, NFC stability + idempotence.

// Part B — NormalizationTest.txt (official UAX #15 conformance file) Bengali

//          block lines through the engine's normalize_nfc (UAX15-C3).

// Part C — BDS-critical sequences: virama, nukta, RRA/RHA/YYA composition

//          exclusions and singleton decompositions through normalize_nfc.

//

// Build (from the repository root):

//   g++ -std=c++20 -Iinclude scripts/layout_verify/bds_repertoire_verify.cpp \

//       -o build/ubuntu-dev/bds_repertoire_verify \

//       -Lbuild/ubuntu-dev -lsuborno_unicode -licui18n -licuuc -lpthread

//

// Usage:

//   bds_repertoire_verify <normalization-test-file> <inventory-csv> <normtest-csv> <keyseq-csv>

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "suborno/unicode/bengali.hpp"

namespace {

std::string utf8_hex(std::string_view s) {
    static const char* hex = "0123456789ABCDEF";
    std::string utf8_text;
    for (unsigned char c : s) {
        utf8_text.push_back(hex[(c >> 4) & 0xF]);
        utf8_text.push_back(hex[c & 0xF]);
    }
    return utf8_text;
}

std::string code_point_to_utf8(unsigned int code_point) {
    std::string utf8_text;
    if (cp < 0x80) {
        utf8_text.push_back(static_cast<char>(cp));
    } else if (cp < 0x800) {
        utf8_text.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        utf8_text.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp < 0x10000) {
        utf8_text.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        utf8_text.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        utf8_text.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        utf8_text.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        utf8_text.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        utf8_text.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        utf8_text.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    return utf8_text;
}

// UTF-16 code units (hex, space-separated) for the code point.

std::string utf16_units(unsigned int cp) {
    char formatted_hex[32];
    if (cp < 0x10000) {
        std::snprintf(formatted_hex, sizeof(formatted_hex), "%04X", cp);
    } else {
        unsigned int v = cp - 0x10000;
        std::snprintf(formatted_hex,
                      sizeof(formatted_hex),
                      "%04X %04X",
                      0xD800 + (v >> 10),
                      0xDC00 + (v & 0x3FF));
    }
    return formatted_hex;
}

std::string unescape_cp_list(const std::string& field) {
    // "09A1 09BC" -> UTF-8 string

    std::istringstream iss(field);
    std::string tok, out;
    while (iss >> tok) {
        out += cp_utf8(static_cast<unsigned int>(std::stoul(tok, nullptr, 16)));
    }
    return utf8_text;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 5) {
        std::cerr
            << "usage: bds_repertoire_verify <NormalizationTest.txt> <inv.csv> <norm.csv> <key.csv>\n";
        return 2;
    }
    const std::string normtest_path = argv[1];
    const std::string inv_csv = argv[2];
    const std::string norm_csv = argv[3];
    const std::string key_csv = argv[4];

    // ---------------- Part A ----------------

    {
        std::ofstream f(inv_csv);
        f << "test_id,codepoint,character,utf8_hex,utf16_units,is_bengali_cp,utf8_valid,cp_count,nfc_stable,nfc_idempotent,result\n";
        int pass = 0, fail = 0;
        for (unsigned int cp = 0x0980; cp <= 0x09FF; ++cp) {
            const std::string ch = cp_utf8(cp);
            const bool valid = suborno::unicode::is_valid_utf8(ch);
            const std::size_t count = suborno::unicode::count_utf8_code_points(ch);
            const bool is_beng = suborno::unicode::is_bengali_codepoint(cp);
            const std::string nfc = suborno::unicode::normalize_nfc(ch);
            const std::string nfc2 = suborno::unicode::normalize_nfc(nfc);
            const bool stable = (nfc == ch);
            const bool idem = (nfc2 == nfc);
            const bool ok = valid && (count == 1) && is_beng && idem;
            // NOTE: nfc_stable is informational; unassigned/reserved or

            // composition-participating code points may legitimately differ.

            if (ok) {
                ++pass;
            } else {
                ++fail;
            }
            f << "REPL-" << std::hex << std::uppercase << cp << std::dec << ","
              << "U+" << std::hex << std::uppercase << cp << std::dec << "," << (valid ? ch : "")
              << "," << utf8_hex(ch) << "," << utf16_units(cp) << ","
              << (is_beng ? "TRUE" : "FALSE") << "," << (valid ? "TRUE" : "FALSE") << "," << count
              << "," << (stable ? "TRUE" : "FALSE") << "," << (idem ? "TRUE" : "FALSE") << ","
              << (ok ? "PASS" : "FAIL") << "\n";
        }
        std::cout << "partA rows=128 pass=" << pass << " fail=" << fail << "\n";
    }

    // ---------------- Part B ----------------

    {
        std::ifstream in(normtest_path);
        std::ofstream f(norm_csv);
        f << "test_id,source_cp,nfc_expected,nfd_expected,nfc_of_nfd_expected,engine_nfc_source,engine_nfc_nfd,match_source,match_nfd,result\n";
        if (!in) {
            std::cout << "partB skipped (no NormalizationTest.txt)\n";
            f << "NT-BLOCKED,,,,,,,,,BLOCKED\n";
        } else {
            std::string line;
            int pass = 0, fail = 0, rows = 0;
            while (std::getline(in, line)) {
                if (line.empty() || line[0] == '#' || line[0] == '@')
                    continue;
                // fields separated by ';', comment after '#'

                std::vector<std::string> parts;
                std::string field;
                std::istringstream iss(line);
                while (std::getline(iss, field, ';'))
                    parts.push_back(field);
                if (parts.size() < 5)
                    continue;
                // Bengali block filter: first code point of column 1 in 0980..09FF

                unsigned first_cp = 0;
                {
                    std::istringstream fs(parts[0]);
                    std::string tok;
                    fs >> tok;
                    if (tok.size() >= 4)
                        first_cp = static_cast<unsigned>(std::stoul(tok, nullptr, 16));
                }
                if (first_cp < 0x0980 || first_cp > 0x09FF)
                    continue;
                ++rows;
                const std::string src = unescape_cp_list(parts[0]);
                const std::string nfc_exp = unescape_cp_list(parts[1]);
                const std::string nfd_exp = unescape_cp_list(parts[2]);
                const std::string nfc_nfd_exp = unescape_cp_list(parts[3]);
                const std::string engine_nfc_src = suborno::unicode::normalize_nfc(src);
                const std::string engine_nfc_nfd = suborno::unicode::normalize_nfc(nfd_exp);
                const bool m1 = (engine_nfc_src == nfc_exp);
                const bool m2 = (engine_nfc_nfd == nfc_nfd_exp);
                if (m1 && m2) {
                    ++pass;
                } else {
                    ++fail;
                }
                f << "NT-" << parts[0] << "," << parts[0] << "," << parts[1] << "," << parts[2]
                  << "," << parts[3] << "," << (m1 ? nfc_exp : "MISMATCH") << ","
                  << (m2 ? nfc_nfd_exp : "MISMATCH") << "," << (m1 ? "TRUE" : "FALSE") << ","
                  << (m2 ? "TRUE" : "FALSE") << "," << ((m1 && m2) ? "PASS" : "FAIL") << "\n";
            }
            std::cout << "partB rows=" << rows << " pass=" << pass << " fail=" << fail << "\n";
        }
    }

    // ---------------- Part C ----------------

    {
        struct Case {
            const char* id;
            const char* input_hex;  // space-separated code points

            const char* expected_hex;
            const char* note;
        };
        const Case cases[] = {
            {"KEY-01", "0995 09CD", "0995 09CD", "virama sequence stable under NFC"},
            {"KEY-02", "09BC", "09BC", "standalone nukta stable"},
            {"KEY-03",
             "0995 09BC",
             "0995 09BC",
             "KA+NUKTA stable (no precomposed KA-NUKTA exists)"},
            {"KEY-04",
             "09A1 09BC",
             "09A1 09BC",
             "DDA+NUKTA: composition exclusion keeps decomposed"},
            {"KEY-05", "09DC", "09A1 09BC", "RRA singleton decomposes to DDA+NUKTA"},
            {"KEY-06", "09A2 09BC", "09A2 09BC", "DDHA+NUKTA stable"},
            {"KEY-07", "09DD", "09A2 09BC", "RHA singleton decomposes to DDHA+NUKTA"},
            {"KEY-08", "09AF 09BC", "09AF 09BC", "YA+NUKTA stable"},
            {"KEY-09", "09DF", "09AF 09BC", "YYA singleton decomposes to YA+NUKTA"},
            {"KEY-10", "0995 09CD 0995", "0995 09CD 0995", "KA+virama+KA conjunct base stable"},
            {"KEY-11", "09C7 09BE", "09CB", "E+AA composes to O-sign (NFC composition works)"},
            {"KEY-12", "09CB", "09CB", "O-sign stays composed"},
        };
        std::ofstream f(key_csv);
        f << "test_id,input_cp,expected_nfc_cp,engine_nfc_cp,note,result\n";
        int pass = 0, fail = 0;
        for (const Case& c : cases) {
            const std::string in = unescape_cp_list(c.input_hex);
            const std::string expected = unescape_cp_list(c.expected_hex);
            const std::string got = suborno::unicode::normalize_nfc(in);
            const bool ok = (got == expected);
            if (ok) {
                ++pass;
            } else {
                ++fail;
            }
            f << c.id << "," << c.input_hex << "," << c.expected_hex << ","
              << (ok ? c.expected_hex : "MISMATCH") << "," << c.note << ","
              << (ok ? "PASS" : "FAIL") << "\n";
        }
        std::cout << "partC cases=" << (pass + fail) << " pass=" << pass << " fail=" << fail
                  << "\n";
    }

    return 0;
}
