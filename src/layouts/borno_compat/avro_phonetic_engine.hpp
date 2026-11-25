// Avro Phonetic transliteration engine — Suborno's own implementation.

//

// Rule-driven rewrite: every consonant lead character owns an ordered rule

// list tried top-to-bottom (longest cluster first), with a handful of

// context-sensitive handlers (leading 'a', 'e', 'o', 'O', r-family, y, Z, w,

// x, dot) implemented as explicit functions. The rule data and the attempt

// order follow the cppAvroPhonetic behaviour contract and are pinned by the

// in-repo parity regression suite (tests/unit/borno_compat_parity_test.cpp,

// 836 inputs x 3 engines, byte-identical output).

//

// SPDX-License-Identifier: GPL-3.0-or-later

// Behaviour provenance: cppAvroPhonetic (GPL-3, (c) 2014-2015 Muhammad

// Mominul Huque) as shipped in BornoTSF.

#ifndef SUBORNO_AVRO_PHONETIC_ENGINE_HPP
#define SUBORNO_AVRO_PHONETIC_ENGINE_HPP

#include <cctype>
#include <string>
#include <vector>

#include "borno_compat_autocorrect.hpp"

namespace borno_compat {

namespace avro_glyph {
// Bengali glyph constants (one code point each unless commented).

inline const std::string candrabindu = "\u0981";
inline const std::string anushar = "\u0982";
inline const std::string bisharga = "\u0983";
inline const std::string vowel_a = "\u0985";
inline const std::string vowel_aa = "\u0986";
inline const std::string vowel_i = "\u0987";
inline const std::string vowel_ii = "\u0988";
inline const std::string vowel_u = "\u0989";
inline const std::string vowel_uu = "\u098A";
inline const std::string vocalic_ri = "\u098B";
inline const std::string vowel_e = "\u098F";
inline const std::string vowel_o = "\u0993";
inline const std::string vowel_oi = "\u0990";
inline const std::string vowel_ou = "\u0994";
inline const std::string ka = "\u0995";
inline const std::string kha = "\u0996";
inline const std::string ga = "\u0997";
inline const std::string gha = "\u0998";
inline const std::string nga = "\u0999";
inline const std::string cha = "\u099A";
inline const std::string chha = "\u099B";
inline const std::string ja = "\u099C";
inline const std::string jha = "\u099D";
inline const std::string nya = "\u099E";
inline const std::string tta = "\u099F";
inline const std::string ttha = "\u09A0";
inline const std::string dda = "\u09A1";
inline const std::string ddha = "\u09A2";
inline const std::string na = "\u09A8";
inline const std::string ta = "\u09A4";
inline const std::string tha = "\u09A5";
inline const std::string da = "\u09A6";
inline const std::string dha = "\u09A7";
inline const std::string nna = "\u09A3";
inline const std::string pa = "\u09AA";
inline const std::string pha = "\u09AB";
inline const std::string ba = "\u09AC";
inline const std::string bha = "\u09AD";
inline const std::string ma = "\u09AE";
inline const std::string za = "\u09AF";
inline const std::string ra = "\u09B0";
inline const std::string rra = "\u09DC";
inline const std::string rrha = "\u09DD";
inline const std::string la = "\u09B2";
inline const std::string sha = "\u09B6";
inline const std::string ssa = "\u09B7";
inline const std::string sa = "\u09B8";
inline const std::string ha = "\u09B9";
inline const std::string nukta = "\u09BC";
inline const std::string khanda = "\u09CE";
inline const std::string ya = "\u09DF";
inline const std::string dari = "\u0964";
inline const std::string hasanta = "\u09CD";
inline const std::string taka = "\u09F3";
inline const std::string zwj = "\u200D";
inline const std::string zwnj = "\u200C";
inline const std::string vowel_sign_aa = "\u09BE";
inline const std::string vowel_sign_i = "\u09BF";
inline const std::string vowel_sign_ii = "\u09C0";
inline const std::string vowel_sign_u = "\u09C1";
inline const std::string vowel_sign_uu = "\u09C2";
inline const std::string vowel_sign_ri = "\u09C3";
inline const std::string vowel_sign_e = "\u09C7";
inline const std::string vowel_sign_oi = "\u09C8";
inline const std::string vowel_sign_o = "\u09CB";
inline const std::string vowel_sign_ou = "\u09CC";
}  // namespace avro_glyph

class AvroPhoneticEngine {
  public:
    AvroPhoneticEngine() {
        build_rule_tables();
    }

    std::string convert(const std::string& input_text) {
        std::string corrected_text;
        if (apply_autocorrect_gate(input_text, corrected_text)) {
            return corrected_text;
        }
        normalized_input_ = fold_case(corrected_text);
        composed_output_.clear();
        parse_byte_offset_ = 0;

        while (parse_byte_offset_ < normalized_input_.size()) {
            const std::string current_character = normalized_input_.substr(parse_byte_offset_, 1);
            if (current_character >= "0" && current_character <= "9") {
                emit_glyph(bengali_digit(current_character));
                continue;
            }
            if (current_character == "o") {
                handle_small_o();
                continue;
            }
            if (current_character == "A") {
                emit_glyph(avro_glyph::vowel_aa);  // forced AA

                continue;
            }
            if (current_character == "a") {
                handle_leading_a();
                continue;
            }
            if (current_character == "i") {
                handle_kar_vowel(avro_glyph::vowel_i, avro_glyph::vowel_sign_i);
                continue;
            }
            if (current_character == "I") {
                handle_kar_vowel(avro_glyph::vowel_ii, avro_glyph::vowel_sign_ii);
                continue;
            }
            if (current_character == "u") {
                handle_kar_vowel(avro_glyph::vowel_u, avro_glyph::vowel_sign_u);
                continue;
            }
            if (current_character == "U") {
                handle_kar_vowel(avro_glyph::vowel_uu, avro_glyph::vowel_sign_uu);
                continue;
            }
            if (current_character == "e" || current_character == "E") {
                handle_e();
                continue;
            }
            if (current_character == "O") {
                handle_capital_o();
                continue;
            }
            if (current_character == "k") {
                try_rule_list(k_rules_);
                continue;
            }
            if (current_character == "G" || current_character == "g") {
                try_rule_list(g_rules_);
                continue;
            }
            if (current_character == "N" || current_character == "n") {
                try_rule_list(n_rules_);
                continue;
            }
            if (current_character == "c") {
                try_rule_list(c_rules_);
                continue;
            }
            if (current_character == "J" || current_character == "j") {
                try_rule_list(j_rules_);
                continue;
            }
            if (current_character == "T" || current_character == "t") {
                try_rule_list(t_rules_);
                continue;
            }
            if (current_character == "D" || current_character == "d") {
                try_rule_list(d_rules_);
                continue;
            }
            if (current_character == "p" || current_character == "f") {
                try_rule_list(p_rules_);
                continue;
            }
            if (current_character == "b" || current_character == "v") {
                try_rule_list(b_rules_);
                continue;
            }
            if (current_character == "m") {
                try_rule_list(m_rules_);
                continue;
            }
            if (current_character == "z") {
                emit_glyph(avro_glyph::za);
                continue;
            }
            if (current_character == "Z") {
                handle_z_after_r();
                continue;
            }
            if (current_character == "R" || current_character == "r") {
                handle_r_family();
                continue;
            }
            if (current_character == "l") {
                try_rule_list(l_rules_);
                continue;
            }
            if (current_character == "S" || current_character == "s") {
                try_rule_list(s_rules_);
                continue;
            }
            if (current_character == "h") {
                try_rule_list(h_rules_);
                continue;
            }
            if (current_character == "y") {
                handle_y();
                continue;
            }
            if (current_character == "Y") {
                emit_glyph(avro_glyph::ya);  // forced Y

                continue;
            }
            if (current_character == "w") {
                handle_w();
                continue;
            }
            if (current_character == "q") {
                emit_glyph(avro_glyph::ka);
                continue;
            }
            if (current_character == "x") {
                handle_x();
                continue;
            }
            if (current_character == ".") {
                handle_dot();
                continue;
            }
            if (current_character == ":") {
                if (character_ahead(0) != "`") {
                    emit_glyph(avro_glyph::bisharga);
                } else {
                    emit_glyph_advanced(":", 2);
                }
                continue;
            }
            if (current_character == "^") {
                if (character_ahead(0) != "`") {
                    emit_glyph(avro_glyph::candrabindu);
                } else {
                    emit_glyph_advanced("^", 2);
                }
                continue;
            }
            if (current_character == ",") {
                if (character_ahead(0) == ",") {
                    emit_glyph_advanced(avro_glyph::hasanta + avro_glyph::zwnj, 2);
                } else {
                    emit_glyph(",");
                }
                continue;
            }
            if (current_character == "$") {
                emit_glyph(avro_glyph::taka);
                continue;
            }
            if (current_character == "`") {
                ++parse_byte_offset_;  // conjunct-break marker: consumed, no output

                continue;
            }
            emit_glyph(current_character);
        }
        return composed_output_;
    }

  private:
    struct Rule {
        std::string pattern;
        std::string output;
    };
    using RuleList = std::vector<Rule>;

    RuleList k_rules_, g_rules_, n_rules_, c_rules_, j_rules_, t_rules_;
    RuleList d_rules_, p_rules_, b_rules_, m_rules_, l_rules_, s_rules_, h_rules_;

    // ---- cursor machinery ----------------------------------------------------

    // cursor_ is the 0-based index of the character under process. Emit()

    // always advances exactly one character; EmitAdvance() advances n.

    void emit_glyph(const std::string& glyph) {
        composed_output_ += glyph;
        parse_byte_offset_ += 1;
    }

    void emit_glyph_advanced(const std::string& glyph, size_t n) {
        composed_output_ += glyph;
        parse_byte_offset_ += n;
    }

    bool try_rule(const std::string& pattern, const std::string& output) {
        if (normalized_input_.compare(parse_byte_offset_, pattern.size(), pattern) == 0) {
            composed_output_ += output;
            parse_byte_offset_ += pattern.size();
            return true;
        }
        return false;
    }

    void try_rule_list(const RuleList& rules) {
        for (const Rule& rule : rules) {
            if (try_rule(rule.pattern, rule.output)) {
                return;
            }
        }
    }

    // Character one step back; "" at the beginning.

    std::string character_behind(size_t steps) const {
        if (parse_byte_offset_ < steps) {
            return "";
        }
        return normalized_input_.substr(parse_byte_offset_ - steps, 1);
    }

    std::string previous_character() const {
        return character_behind(1);
    }

    // Character ahead of the current one by `skip` characters (skip 0 = the

    // next character); "" when past the end.

    std::string character_ahead(size_t skip) const {
        const size_t index = parse_byte_offset_ + 1 + skip;
        if (index >= normalized_input_.size()) {
            return "";
        }
        return normalized_input_.substr(index, 1);
    }

    // "Word start": the previous character is not an ASCII letter/digit

    // (or there is none) — the upstream boundary heuristic.

    bool is_word_start() const {
        const std::string previous_text = previous_character();
        if (previous_text.empty()) {
            return true;
        }
        const char previous_byte = previous_text[0];
        const bool letter_or_digit = (previous_byte >= 'a' && previous_byte <= 'z')
                                     || (previous_byte >= 'A' && previous_byte <= 'Z')
                                     || (previous_byte >= '0' && previous_byte <= '9');
        return !letter_or_digit;
    }

    static std::string to_lower_ascii(std::string text) {
        for (char& byte_char : text) {
            if (byte_char >= 'A' && byte_char <= 'Z') {
                byte_char = static_cast<char>(byte_char + 32);
            }
        }
        return text;
    }

    static bool is_vowel_character(std::string text) {
        text = to_lower_ascii(text);
        return text == "a" || text == "e" || text == "i" || text == "o" || text == "u";
    }

    static bool is_consonant_character(std::string text) {
        text = to_lower_ascii(text);
        const std::string consonant_set = "bcdfghjklmnpqrstvwxyz";
        return text.size() == 1 && consonant_set.find(text[0]) != std::string::npos;
    }

    // Digit check — upstream treats "8" as a non-digit (pre-decimal quirk),

    // preserved here.

    static bool is_digit_character(const std::string& text) {
        return (text >= "0" && text <= "7") || text == "9";
    }

    std::string bengali_digit(const std::string& current_character) const {
        static const char* const glyphs[] = {
            "\u09E6",
            "\u09E7",
            "\u09E8",
            "\u09E9",
            "\u09EA",
            "\u09EB",
            "\u09EC",
            "\u09ED",
            "\u09EE",
            "\u09EF",
        };
        return glyphs[current_character[0] - '0'];
    }

    // Case folding: letters whose case the rule tables distinguish keep it;

    // every other ASCII uppercase letter folds to lowercase. Bytes outside

    // ASCII pass through untouched.

    static std::string fold_case(const std::string& text) {
        std::string folded_text = text;
        for (char& byte_char : folded_text) {
            switch (byte_char) {
            case 'a':
            case 'A':
            case 'o':
            case 'O':
            case 'i':
            case 'I':
            case 'u':
            case 'U':
            case 'd':
            case 'D':
            case 'g':
            case 'G':
            case 'j':
            case 'J':
            case 'n':
            case 'N':
            case 'r':
            case 'R':
            case 's':
            case 'S':
            case 't':
            case 'T':
            case 'y':
            case 'Y':
            case 'z':
            case 'Z':
                continue;
            default:
                if (byte_char >= 'A' && byte_char <= 'Z') {
                    byte_char = static_cast<char>(byte_char + 32);
                }
                break;
            }
        }
        return folded_text;
    }

    // ---- context handlers -----------------------------------------------------

    void handle_leading_a() {
        if (character_ahead(0) == "Z") {
            emit_glyph_advanced(avro_glyph::vowel_a + avro_glyph::hasanta + avro_glyph::za
                                    + avro_glyph::vowel_sign_aa,
                                2);
        } else if (is_word_start() && character_ahead(0) != "`") {
            emit_glyph(avro_glyph::vowel_aa);
        } else if (!is_consonant_character(previous_character()) && previous_character() != "a"
                   && character_ahead(0) != "`") {
            emit_glyph(avro_glyph::ya + avro_glyph::vowel_sign_aa);
        } else if (character_ahead(0) == "`") {
            emit_glyph_advanced(avro_glyph::vowel_sign_aa, 2);
        } else if (previous_character() == "a" && character_ahead(0) != "`") {
            emit_glyph(avro_glyph::vowel_aa);
        } else {
            emit_glyph(avro_glyph::vowel_sign_aa);
        }
    }

    void handle_kar_vowel(const std::string& full, const std::string& kar) {
        if ((!is_consonant_character(previous_character()) || is_word_start())
            && character_ahead(0) != "`") {
            emit_glyph(full);
        } else if (character_ahead(0) == "`") {
            emit_glyph_advanced(kar, 2);
        } else {
            emit_glyph(kar);
        }
    }

    void handle_e() {
        if ((!is_consonant_character(previous_character()) || is_word_start())
            && character_ahead(0) != "`") {
            if (character_ahead(0) == "e") {
                emit_glyph_advanced(avro_glyph::vowel_ii, 2);
            } else {
                emit_glyph(avro_glyph::vowel_e);
            }
        } else if (character_ahead(0) == "`") {
            emit_glyph_advanced(avro_glyph::vowel_sign_e, 2);
        } else {
            if (character_ahead(0) == "e") {
                emit_glyph_advanced(avro_glyph::vowel_sign_ii, 2);
            } else {
                emit_glyph(avro_glyph::vowel_sign_e);
            }
        }
    }

    void handle_small_o() {
        if ((!is_consonant_character(previous_character()) || is_word_start())
            && character_ahead(0) != "`") {
            if (try_rule("oo", avro_glyph::vowel_u))
                return;
            if (try_rule("oZ", avro_glyph::vowel_a + avro_glyph::hasanta + avro_glyph::za))
                return;
            if (is_vowel_character(previous_character()) && previous_character() != "o") {
                if (try_rule("o", avro_glyph::vowel_o))
                    return;
            } else {
                if (try_rule("o", avro_glyph::vowel_a))
                    return;
            }
        }
        if (try_rule("oo", avro_glyph::vowel_sign_u))
            return;
        if (try_rule("o`", ""))
            return;
        try_rule("o", "");
    }

    void handle_capital_o() {
        if (try_rule("OI`", avro_glyph::vowel_sign_oi))
            return;
        if (try_rule("OU`", avro_glyph::vowel_sign_ou))
            return;
        if (try_rule("O`", avro_glyph::vowel_sign_o))
            return;

        if (!is_consonant_character(previous_character()) || is_word_start()) {
            if (try_rule("OI", avro_glyph::vowel_oi))
                return;
            if (try_rule("OU", avro_glyph::vowel_ou))
                return;
            try_rule("O", avro_glyph::vowel_o);
        } else {
            if (try_rule("OI", avro_glyph::vowel_sign_oi))
                return;
            if (try_rule("OU", avro_glyph::vowel_sign_ou))
                return;
            try_rule("O", avro_glyph::vowel_sign_o);
        }
    }

    // r-family: rri, reph, ra-phola, Rg/Rh, standalone r/R.

    void handle_r_family() {
        if (character_ahead(2) == "`") {
            if (try_rule("rri", avro_glyph::vowel_sign_ri))
                return;
        }
        if (!is_consonant_character(previous_character())) {
            if (try_rule("rri", avro_glyph::vocalic_ri))
                return;
        } else if (is_word_start()) {
            if (try_rule("rri", avro_glyph::vocalic_ri))
                return;
        } else {
            if (try_rule("rri", avro_glyph::vowel_sign_ri))
                return;
        }

        // Reph: "rr" before a non-vowel, non-r character.

        const std::string after_pair = character_ahead(1);
        if (!is_consonant_character(previous_character()) && !is_vowel_character(after_pair)
            && after_pair != "r" && !after_pair.empty()) {
            if (try_rule("rr", avro_glyph::ra + avro_glyph::hasanta))
                return;
        }

        if (try_rule("Rg", avro_glyph::rra + avro_glyph::hasanta + avro_glyph::ga))
            return;
        if (try_rule("Rh", avro_glyph::rrha))
            return;

        // Ra-phola after a cluster-capable consonant; standalone otherwise.

        if (is_consonant_character(previous_character()) && previous_character() != "r"
            && previous_character() != "y" && previous_character() != "w"
            && previous_character() != "x" && previous_character() != "Z") {
            if (try_rule("r", avro_glyph::hasanta + avro_glyph::ra))
                return;
        } else {
            if (try_rule("r", avro_glyph::ra))
                return;
        }

        try_rule("R", avro_glyph::rra);
    }

    void handle_y() {
        if (!is_consonant_character(previous_character()) && !is_word_start()) {
            emit_glyph(avro_glyph::ya);
        } else if (is_word_start()) {
            emit_glyph(avro_glyph::vowel_i + avro_glyph::ya);
        } else {
            if (previous_character() == "r") {
                if (is_consonant_character(character_behind(2)) && character_behind(2) != "r"
                    && character_behind(2) != "y" && character_behind(2) != "w"
                    && character_behind(2) != "x") {
                    emit_glyph(avro_glyph::hasanta + avro_glyph::za);
                } else {
                    emit_glyph(avro_glyph::zwj + avro_glyph::hasanta + avro_glyph::za);
                }
            } else {
                emit_glyph(avro_glyph::hasanta + avro_glyph::za);
            }
        }
    }

    void handle_z_after_r() {
        if (previous_character() == "r") {
            if (is_consonant_character(character_behind(2)) && character_behind(2) != "r"
                && character_behind(2) != "y" && character_behind(2) != "w"
                && character_behind(2) != "x") {
                emit_glyph(avro_glyph::hasanta + avro_glyph::za);
            } else {
                emit_glyph(avro_glyph::zwj + avro_glyph::hasanta + avro_glyph::za);
            }
        } else {
            emit_glyph(avro_glyph::hasanta + avro_glyph::za);
        }
    }

    void handle_w() {
        if (is_word_start() && is_vowel_character(character_ahead(0))) {
            emit_glyph(avro_glyph::vowel_o + avro_glyph::ya);
        } else if (is_consonant_character(previous_character())) {
            emit_glyph(avro_glyph::hasanta + avro_glyph::ba);
        } else {
            emit_glyph(avro_glyph::vowel_o);
        }
    }

    void handle_x() {
        if (is_word_start()) {
            emit_glyph(avro_glyph::vowel_e + avro_glyph::ka + avro_glyph::hasanta + avro_glyph::sa);
        } else {
            emit_glyph(avro_glyph::ka + avro_glyph::hasanta + avro_glyph::sa);
        }
    }

    void handle_dot() {
        if (try_rule("...", "..."))
            return;
        if (try_rule(".`", "."))
            return;
        if (try_rule("..", avro_glyph::dari + avro_glyph::dari))
            return;
        if (is_digit_character(character_ahead(0))) {
            if (try_rule(".", "."))
                return;  // decimal mark
        }
        try_rule(".", avro_glyph::dari);
    }

    // ---- rule tables ------------------------------------------------------------

    // Order inside each list is the upstream attempt order and matters.

    void build_rule_tables() {
        namespace glyph = avro_glyph;
        const std::string& hasanta_sign = glyph::hasanta;

        k_rules_ = {
            {"kkhN", glyph::ka + hasanta_sign + glyph::ssa + hasanta_sign + glyph::nna},
            {"kShN", glyph::ka + hasanta_sign + glyph::ssa + hasanta_sign + glyph::nna},
            {"kkhm", glyph::ka + hasanta_sign + glyph::ssa + hasanta_sign + glyph::ma},
            {"kShm", glyph::ka + hasanta_sign + glyph::ssa + hasanta_sign + glyph::ma},
            {"kxN", glyph::ka + hasanta_sign + glyph::ssa + hasanta_sign + glyph::nna},
            {"kxm", glyph::ka + hasanta_sign + glyph::ssa + hasanta_sign + glyph::ma},
            {"kkh", glyph::ka + hasanta_sign + glyph::ssa},
            {"kSh", glyph::ka + hasanta_sign + glyph::ssa},
            {"ksh", glyph::ka + glyph::sha},
            {"kx", glyph::ka + hasanta_sign + glyph::ssa},
            {"kk", glyph::ka + hasanta_sign + glyph::ka},
            {"kT", glyph::ka + hasanta_sign + glyph::tta},
            {"kt", glyph::ka + hasanta_sign + glyph::ta},
            {"km", glyph::ka + hasanta_sign + glyph::ma},
            {"kl", glyph::ka + hasanta_sign + glyph::la},
            {"ks", glyph::ka + hasanta_sign + glyph::sa},
            {"kh", glyph::kha},
            {"k", glyph::ka},
        };

        g_rules_ = {
            {"ghn", glyph::gha + hasanta_sign + glyph::na},
            {"Ghn", glyph::gha + hasanta_sign + glyph::na},
            {"gdh", glyph::ga + hasanta_sign + glyph::dha},
            {"Gdh", glyph::ga + hasanta_sign + glyph::dha},
            {"gN", glyph::ga + hasanta_sign + glyph::nna},
            {"GN", glyph::ga + hasanta_sign + glyph::nna},
            {"gn", glyph::ga + hasanta_sign + glyph::na},
            {"Gn", glyph::ga + hasanta_sign + glyph::na},
            {"gm", glyph::ga + hasanta_sign + glyph::ma},
            {"Gm", glyph::ga + hasanta_sign + glyph::ma},
            {"gl", glyph::ga + hasanta_sign + glyph::la},
            {"Gl", glyph::ga + hasanta_sign + glyph::la},
            {"gg", glyph::ja + hasanta_sign + glyph::nya},
            {"GG", glyph::ja + hasanta_sign + glyph::nya},
            {"Gg", glyph::ja + hasanta_sign + glyph::nya},
            {"gG", glyph::ja + hasanta_sign + glyph::nya},
            {"gh", glyph::gha},
            {"Gh", glyph::gha},
            {"g", glyph::ga},
            {"G", glyph::ga},
        };

        n_rules_ = {
            {"NgkSh", glyph::nga + hasanta_sign + glyph::ka + hasanta_sign + glyph::ssa},
            {"Ngkkh", glyph::nga + hasanta_sign + glyph::ka + hasanta_sign + glyph::ssa},
            {"NGch", glyph::nya + hasanta_sign + glyph::chha},
            {"Nggh", glyph::nga + hasanta_sign + glyph::gha},
            {"Ngkh", glyph::nga + hasanta_sign + glyph::kha},
            {"NGjh", glyph::nya + hasanta_sign + glyph::jha},
            {"ngOU", glyph::nga + hasanta_sign + glyph::ga + glyph::vowel_sign_ou},
            {"ngOI", glyph::nga + hasanta_sign + glyph::ga + glyph::vowel_sign_oi},
            {"Ngkx", glyph::nga + hasanta_sign + glyph::ka + hasanta_sign + glyph::ssa},
            {"NGc", glyph::nya + hasanta_sign + glyph::cha},
            {"nch", glyph::nya + hasanta_sign + glyph::chha},
            {"njh", glyph::nya + hasanta_sign + glyph::jha},
            {"nch", glyph::nya + hasanta_sign + glyph::cha},
            {"ngh", glyph::nga + hasanta_sign + glyph::gha},
            {"Ngk", glyph::nga + hasanta_sign + glyph::ka},
            {"Ngx", glyph::nga + hasanta_sign + glyph::ssa},
            {"Ngg", glyph::nga + hasanta_sign + glyph::ga},
            {"Ngm", glyph::nga + hasanta_sign + glyph::ma},
            {"NGj", glyph::nya + hasanta_sign + glyph::ja},
            {"ndh", glyph::na + hasanta_sign + glyph::dha},
            {"nTh", glyph::na + hasanta_sign + glyph::ttha},
            {"NTh", glyph::nna + hasanta_sign + glyph::ttha},
            {"nth", glyph::na + hasanta_sign + glyph::tha},
            {"nkh", glyph::nga + hasanta_sign + glyph::kha},
            {"ngo", glyph::nga + hasanta_sign + glyph::ga},
            {"nga", glyph::nga + hasanta_sign + glyph::ga + glyph::vowel_sign_aa},
            {"ngi", glyph::nga + hasanta_sign + glyph::ga + glyph::vowel_sign_i},
            {"ngI", glyph::nga + hasanta_sign + glyph::ga + glyph::vowel_sign_ii},
            {"ngu", glyph::nga + hasanta_sign + glyph::ga + glyph::vowel_sign_u},
            {"ngU", glyph::nga + hasanta_sign + glyph::ga + glyph::vowel_sign_uu},
            {"nge", glyph::nga + hasanta_sign + glyph::ga + glyph::vowel_sign_e},
            {"ngO", glyph::nga + hasanta_sign + glyph::ga + glyph::vowel_sign_o},
            {"NDh", glyph::nna + hasanta_sign + glyph::ddha},
            {"nsh", glyph::na + glyph::sha},
            {"Ngr", glyph::nga + glyph::ra},
            {"NGr", glyph::nya + glyph::ra},
            {"ngr", glyph::anushar + glyph::ra},
            {"nj", glyph::nya + hasanta_sign + glyph::ja},
            {"Ng", glyph::nga},
            {"NG", glyph::nya},
            {"nk", glyph::nga + hasanta_sign + glyph::ka},
            {"ng", glyph::anushar},
            {"nn", glyph::na + hasanta_sign + glyph::na},
            {"NN", glyph::nna + hasanta_sign + glyph::nna},
            {"Nn", glyph::nna + hasanta_sign + glyph::na},
            {"nm", glyph::na + hasanta_sign + glyph::ma},
            {"Nm", glyph::nna + hasanta_sign + glyph::ma},
            {"nd", glyph::na + hasanta_sign + glyph::da},
            {"nT", glyph::na + hasanta_sign + glyph::tta},
            {"NT", glyph::nna + hasanta_sign + glyph::tta},
            {"nD", glyph::na + hasanta_sign + glyph::dda},
            {"ND", glyph::nna + hasanta_sign + glyph::dda},
            {"nt", glyph::na + hasanta_sign + glyph::ta},
            {"ns", glyph::na + hasanta_sign + glyph::sa},
            {"nc", glyph::nya + hasanta_sign + glyph::cha},
            {"n", glyph::na},
            {"N", glyph::nna},
        };

        c_rules_ = {
            {"cNG", glyph::cha + hasanta_sign + glyph::nya},
            {"cch", glyph::cha + hasanta_sign + glyph::chha},
            {"cc", glyph::cha + hasanta_sign + glyph::cha},
            {"ch", glyph::chha},
            {"c", glyph::cha},
        };

        // (JoNukta build flag of upstream disabled by default: "J" + nukta.)

        j_rules_ = {
            {"jjh", glyph::ja + hasanta_sign + glyph::jha},
            {"jNG", glyph::ja + hasanta_sign + glyph::nya},
            {"jh", glyph::jha},
            {"jj", glyph::ja + hasanta_sign + glyph::ja},
            {"j", glyph::ja},
            {"J", glyph::ja},
        };

        t_rules_ = {
            {"tth", glyph::ta + hasanta_sign + glyph::tha},
            {"t``", glyph::khanda},
            {"TT", glyph::tta + hasanta_sign + glyph::tta},
            {"Tm", glyph::tta + hasanta_sign + glyph::ma},
            {"Th", glyph::ttha},
            {"tn", glyph::ta + hasanta_sign + glyph::na},
            {"tm", glyph::ta + hasanta_sign + glyph::ma},
            {"th", glyph::tha},
            {"tt", glyph::ta + hasanta_sign + glyph::ta},
            {"T", glyph::tta},
            {"t", glyph::ta},
        };

        d_rules_ = {
            {"dhn", glyph::dha + hasanta_sign + glyph::na},
            {"dhm", glyph::dha + hasanta_sign + glyph::ma},
            {"dgh", glyph::da + hasanta_sign + glyph::gha},
            {"ddh", glyph::da + hasanta_sign + glyph::dha},
            {"dbh", glyph::da + hasanta_sign + glyph::bha},
            {"dv", glyph::da + hasanta_sign + glyph::bha},
            {"dm", glyph::da + hasanta_sign + glyph::ma},
            {"DD", glyph::dda + hasanta_sign + glyph::dda},
            {"Dh", glyph::ddha},
            {"dh", glyph::dha},
            {"dg", glyph::da + hasanta_sign + glyph::ga},
            {"dd", glyph::da + hasanta_sign + glyph::da},
            {"D", glyph::dda},
            {"d", glyph::da},
        };

        p_rules_ = {
            {"phl", glyph::pha + hasanta_sign + glyph::la},
            {"pT", glyph::pa + hasanta_sign + glyph::tta},
            {"pt", glyph::pa + hasanta_sign + glyph::ta},
            {"pn", glyph::pa + hasanta_sign + glyph::na},
            {"pp", glyph::pa + hasanta_sign + glyph::pa},
            {"pl", glyph::pa + hasanta_sign + glyph::la},
            {"ps", glyph::pa + hasanta_sign + glyph::sa},
            {"ph", glyph::pha},
            {"fl", glyph::pha + hasanta_sign + glyph::la},
            {"f", glyph::pha},
            {"p", glyph::pa},
        };

        b_rules_ = {
            {"bdh", glyph::ba + hasanta_sign + glyph::dha},
            {"bhl", glyph::bha + hasanta_sign + glyph::la},
            {"bj", glyph::ba + hasanta_sign + glyph::ja},
            {"bd", glyph::ba + hasanta_sign + glyph::da},
            {"bb", glyph::ba + hasanta_sign + glyph::ba},
            {"bl", glyph::ba + hasanta_sign + glyph::la},
            {"bh", glyph::bha},
            {"vl", glyph::bha + hasanta_sign + glyph::la},
            {"b", glyph::ba},
            {"v", glyph::bha},
        };

        m_rules_ = {
            {"mth", glyph::ma + hasanta_sign + glyph::tha},
            {"mph", glyph::ma + hasanta_sign + glyph::pha},
            {"mbh", glyph::ma + hasanta_sign + glyph::bha},
            {"mpl", glyph::ma + glyph::pa + hasanta_sign + glyph::la},
            {"mn", glyph::ma + hasanta_sign + glyph::na},
            {"mp", glyph::ma + hasanta_sign + glyph::pa},
            {"mv", glyph::ma + hasanta_sign + glyph::bha},
            {"mm", glyph::ma + hasanta_sign + glyph::ma},
            {"ml", glyph::ma + hasanta_sign + glyph::la},
            {"mb", glyph::ma + hasanta_sign + glyph::ba},
            {"mf", glyph::ma + hasanta_sign + glyph::pha},
            {"m", glyph::ma},
        };

        l_rules_ = {
            {"lbh", glyph::la + hasanta_sign + glyph::bha},
            {"ldh", glyph::la + hasanta_sign + glyph::dha},
            {"lkh", glyph::la + glyph::kha},  // upstream renders L&Kh without hasanta

            {"lgh", glyph::la + glyph::gha},  // upstream renders L&Gh without hasanta

            {"lph", glyph::la + glyph::pha},  // upstream renders L&Ph without hasanta

            {"lk", glyph::la + hasanta_sign + glyph::ka},
            {"lg", glyph::la + hasanta_sign + glyph::ga},
            {"lT", glyph::la + hasanta_sign + glyph::tta},
            {"lD", glyph::la + hasanta_sign + glyph::dda},
            {"lp", glyph::la + hasanta_sign + glyph::pa},
            {"lv", glyph::la + hasanta_sign + glyph::bha},
            {"lm", glyph::la + hasanta_sign + glyph::ma},
            {"ll", glyph::la + hasanta_sign + glyph::la},
            {"lb", glyph::la + hasanta_sign + glyph::ba},
            {"l", glyph::la},
        };

        s_rules_ = {
            {"shch", glyph::sha + hasanta_sign + glyph::chha},
            {"ShTh", glyph::ssa + hasanta_sign + glyph::ttha},
            {"Shph", glyph::ssa + hasanta_sign + glyph::pha},
            {"Sch", glyph::sha + hasanta_sign + glyph::chha},
            {"skl", glyph::sa + hasanta_sign + glyph::ka + hasanta_sign + glyph::la},
            {"skh", glyph::sa + hasanta_sign + glyph::kha},
            {"sth", glyph::sa + hasanta_sign + glyph::tha},
            {"sph", glyph::sa + hasanta_sign + glyph::pha},
            {"shc", glyph::sha + hasanta_sign + glyph::cha},
            {"sht", glyph::sha + hasanta_sign + glyph::ta},
            {"shn", glyph::sha + hasanta_sign + glyph::na},
            {"shm", glyph::sha + hasanta_sign + glyph::ma},
            {"spl", glyph::sa + hasanta_sign + glyph::pa + hasanta_sign + glyph::la},
            {"shl", glyph::sha + hasanta_sign + glyph::la},
            {"Shk", glyph::ssa + hasanta_sign + glyph::ka},
            {"ShT", glyph::ssa + hasanta_sign + glyph::tta},
            {"ShN", glyph::ssa + hasanta_sign + glyph::nna},
            {"Shp", glyph::ssa + hasanta_sign + glyph::pa},
            {"Shf", glyph::ssa + hasanta_sign + glyph::pha},
            {"Shm", glyph::ssa + hasanta_sign + glyph::ma},
            {"sk", glyph::sa + hasanta_sign + glyph::ka},
            {"Sc", glyph::sha + hasanta_sign + glyph::cha},
            {"sT", glyph::sa + hasanta_sign + glyph::tta},
            {"st", glyph::sa + hasanta_sign + glyph::ta},
            {"sn", glyph::sa + hasanta_sign + glyph::na},
            {"sp", glyph::sa + hasanta_sign + glyph::pa},
            {"sf", glyph::sa + hasanta_sign + glyph::pha},
            {"sm", glyph::sa + hasanta_sign + glyph::ma},
            {"sl", glyph::sa + hasanta_sign + glyph::la},
            {"sh", glyph::sha},
            {"Sc", glyph::sha + hasanta_sign + glyph::cha},
            {"St", glyph::sha + hasanta_sign + glyph::ta},
            {"Sn", glyph::sha + hasanta_sign + glyph::na},
            {"Sm", glyph::sha + hasanta_sign + glyph::ma},
            {"Sl", glyph::sha + hasanta_sign + glyph::la},
            {"Sh", glyph::ssa},
            {"s", glyph::sa},
            {"S", glyph::sha},
        };

        h_rules_ = {
            {"hN", glyph::ha + hasanta_sign + glyph::nna},
            {"hn", glyph::ha + hasanta_sign + glyph::na},
            {"hm", glyph::ha + hasanta_sign + glyph::ma},
            {"hl", glyph::ha + hasanta_sign + glyph::la},
            {"h", glyph::ha},
        };
    }

    std::string normalized_input_;
    std::string composed_output_;
    size_t parse_byte_offset_ = 0;
};

}  // namespace borno_compat

#endif  // SUBORNO_AVRO_PHONETIC_ENGINE_HPP
