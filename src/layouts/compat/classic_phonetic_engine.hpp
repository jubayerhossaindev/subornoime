// Suborno Classic transliteration engine — Suborno's own implementation.

//

// The engine parses romanized Bangla through a longest-match tokenizer and a

// token-class state machine (word-start / consonant / vowel / mark / other).

// This file was written fresh for Suborno — own class, own table layout,

// own tokenizer — while the observable contract it implements follows the

// published "Suborno Classic" reference behaviour and is pinned by the

// in-repo parity regression suite (tests/unit/compat_parity_test.cpp,

// 836 inputs x 3 engines, byte-identical output).

//

// SPDX-License-Identifier: GPL-3.0-or-later

// Behaviour provenance: the legacy reference behaviour contract
// (attribution in LICENSE and THIRD_PARTY_NOTICES.md).

#ifndef SUBORNO_CLASSIC_PHONETIC_ENGINE_HPP
#define SUBORNO_CLASSIC_PHONETIC_ENGINE_HPP

#include <cstddef>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "compat_autocorrect.hpp"

namespace compat {

class ClassicPhoneticEngine {
  public:
    ClassicPhoneticEngine() {
        build_transliteration_tables();
    }

    // Full pipeline: autocorrect gate, backslash-run escaping, then the

    // token state machine (applied per segment between escape runs).

    std::string convert(const std::string& input_text) {
        if (input_text.empty()) {
            return "";
        }

        std::string corrected_text;
        if (apply_autocorrect_gate(input_text, corrected_text)) {
            return corrected_text;
        }

        const size_t first_slash_offset = corrected_text.find('\\');
        if (first_slash_offset == std::string::npos) {
            return convert_segment(corrected_text);
        }

        size_t slash_run_length = 0;
        while (first_slash_offset + slash_run_length < corrected_text.size()
               && corrected_text[first_slash_offset + slash_run_length] == '\\') {
            ++slash_run_length;
        }
        const std::string text_before_slashes = corrected_text.substr(0, first_slash_offset);
        const std::string text_after_slashes =
            corrected_text.substr(first_slash_offset + slash_run_length);

        // Escape contract (pairs of '\' collapse; odd runs release one '\' and

        // parse the left part, even runs parse the left part and drop half):

        if (text_after_slashes.empty()) {
            switch (slash_run_length) {
            case 1:
                return convert_segment(text_before_slashes) + "\\";
            case 2:
                return text_before_slashes;
            case 3:
                return text_before_slashes + "\\";
            case 4:
                return convert_segment(text_before_slashes) + "\\\\";
            default:
                return (slash_run_length % 2 == 1)
                           ? text_before_slashes + std::string((slash_run_length - 1) / 2, '\\')
                           : convert_segment(text_before_slashes)
                                 + std::string(slash_run_length / 2, '\\');
            }
        }
        switch (slash_run_length) {
        case 1:
            return convert_segment(text_before_slashes) + "\\" + convert(text_after_slashes);
        case 2:
            return text_before_slashes + convert(text_after_slashes);
        case 3:
            return text_before_slashes + "\\" + convert(text_after_slashes);
        case 4:
            return convert_segment(text_before_slashes) + "\\\\" + convert(text_after_slashes);
        default:
            return (slash_run_length % 2 == 1)
                       ? text_before_slashes + std::string((slash_run_length - 1) / 2, '\\')
                             + convert(text_after_slashes)
                       : convert_segment(text_before_slashes)
                             + std::string(slash_run_length / 2, '\\')
                             + convert(text_after_slashes);
        }
    }

  private:
    // Token classes driving the state machine.

    enum class TokenClass {
        word_start,
        consonant,
        vowel,
        mark,
        other
    };

    // Table entry helpers -----------------------------------------------------

    using Table = std::unordered_map<std::string, std::string>;

    Table symbols_;  // ng, TH, X, t``, xx, ::

    Table vowels_head_;  // standalone vowel forms at word start

    Table vowels_after_;  // standalone vowel forms after another vowel

    Table signs_;  // vowel signs (kar) attached after a consonant

    Table letters_;  // consonants (incl. the x/q digraph helpers)

    Table clusters_;  // conjuncts

    Table marks_;  // diacritics and hasanta combinators

    Table punctuations_;
    Table fola_;  // y/z/Z/r/w phola forms

    Table reph_;  // rr reph form

    Table numerals_;  // digits (incl. ".N" decimal-prefixed forms)

    std::unordered_set<std::string> lexicon_;  // every tokenizable key

    size_t longest_key_ = 0;

    static const std::string hasanta() {
        return "\u09CD";
    }
    static const std::string zwj() {
        return "\u200D";
    }
    static const std::string zwnj() {
        return "\u200C";
    }

    void build_transliteration_tables() {
        symbols_ = {
            {"ng", "\u0982"},
            {"TH", "\u09CE"},
            {"X", "\u09CE"},
            {"t``", "\u09CE"},
            {"xx", "\u0981"},
            {"::", "\u0983"},
        };

        vowels_head_ = {
            {"a", "\u0986"},
            {"A", "\u0986"},
            {"i", "\u0987"},
            {"I", "\u0988"},
            {"u", "\u0989"},
            {"U", "\u098A"},
            {"e", "\u098F"},
            {"E", "\u098F"},
            {"o", "\u0985"},
            {"O", "\u0993"},
            {"OI", "\u0990"},
            {"OU", "\u0994"},
        };
        vowels_after_ = vowels_head_;

        signs_ = {
            {"a", "\u09BE"},
            {"A", "\u09BE"},
            {"i", "\u09BF"},
            {"I", "\u09C0"},
            {"u", "\u09C1"},
            {"U", "\u09C2"},
            {"e", "\u09C7"},
            {"E", "\u09C7"},
            {"o", ""},
            {"O", "\u09CB"},
            {"OI", "\u09C8"},
            {"OU", "\u09CC"},
        };

        letters_ = {
            {"k", "\u0995"},  {"kh", "\u0996"},
            {"g", "\u0997"},  {"G", "\u0997"},
            {"gh", "\u0998"}, {"Gh", "\u0998"},
            {"Ng", "\u0999"}, {"NG", "\u099E"},
            {"c", "\u099A"},  {"ch", "\u099B"},
            {"j", "\u099C"},  {"J", "\u099C"},
            {"jh", "\u099D"}, {"t", "\u09A4"},
            {"th", "\u09A5"}, {"T", "\u099F"},
            {"Th", "\u09A0"}, {"d", "\u09A6"},
            {"dh", "\u09A7"}, {"D", "\u09A1"},
            {"Dh", "\u09A2"}, {"n", "\u09A8"},
            {"N", "\u09A3"},  {"p", "\u09AA"},
            {"ph", "\u09AB"}, {"f", "\u09AB"},
            {"b", "\u09AC"},  {"bh", "\u09AD"},
            {"v", "\u09AD"},  {"m", "\u09AE"},
            {"z", "\u09AF"},  {"y", "\u09DF"},
            {"Y", "\u09DF"},  {"r", "\u09B0"},
            {"R", "\u09DC"},  {"Rh", "\u09DD"},
            {"l", "\u09B2"},  {"s", "\u09B8"},
            {"S", "\u09B6"},  {"sh", "\u09B6"},
            {"Sh", "\u09B7"}, {"SH", "\u09B7"},
            {"h", "\u09B9"},  {"H", "\u09B9"},
            {"q", "\u0995"},  {"x", "\u0995\u09CD\u09B8"},
        };

        clusters_ = {
            {"kkhN", "\u0995\u09CD\u09B7\u09CD\u09A3"},
            {"kShN", "\u0995\u09CD\u09B7\u09CD\u09A3"},
            {"kkhm", "\u0995\u09CD\u09B7\u09CD\u09AE"},
            {"kShm", "\u0995\u09CD\u09B7\u09CD\u09AE"},
            {"kxN", "\u0995\u09CD\u09B7\u09CD\u09A3"},
            {"kxm", "\u0995\u09CD\u09B7\u09CD\u09AE"},
            {"kkh", "\u0995\u09CD\u09B7"},
            {"kSh", "\u0995\u09CD\u09B7"},
            {"ksh", "\u0995\u09CD\u09B6"},
            {"kx", "\u0995\u09CD\u09B7"},
            {"kk", "\u0995\u09CD\u0995"},
            {"kT", "\u0995\u09CD\u099F"},
            {"kt", "\u0995\u09CD\u09A4"},
            {"km", "\u0995\u09CD\u09AE"},
            {"kl", "\u0995\u09CD\u09B2"},
            {"ks", "\u0995\u09CD\u09B8"},
            {"ghn", "\u0998\u09CD\u09A8"},
            {"Ghn", "\u0998\u09CD\u09A8"},
            {"gdh", "\u0997\u09CD\u09A7"},
            {"Gdh", "\u0997\u09CD\u09A7"},
            {"gN", "\u0997\u09CD\u09A3"},
            {"GN", "\u0997\u09CD\u09A3"},
            {"gn", "\u0997\u09CD\u09A8"},
            {"Gn", "\u0997\u09CD\u09A8"},
            {"gm", "\u0997\u09CD\u09AE"},
            {"Gm", "\u0997\u09CD\u09AE"},
            {"gl", "\u0997\u09CD\u09B2"},
            {"Gl", "\u0997\u09CD\u09B2"},
            {"gg", "\u099C\u09CD\u099E"},
            {"GG", "\u099C\u09CD\u099E"},
            {"Gg", "\u099C\u09CD\u099E"},
            {"gG", "\u099C\u09CD\u099E"},
            {"nch", "\u099E\u09CD\u099B"},
            {"njh", "\u099E\u09CD\u099D"},
            {"ndh", "\u09A8\u09CD\u09A7"},
            {"nTh", "\u09A8\u09CD\u09A0"},
            {"nth", "\u09A8\u09CD\u09A5"},
            {"nj", "\u099E\u09CD\u099C"},
            {"nn", "\u09A8\u09CD\u09A8"},
            {"nm", "\u09A8\u09CD\u09AE"},
            {"nd", "\u09A8\u09CD\u09A6"},
            {"nT", "\u09A8\u09CD\u099F"},
            {"nD", "\u09A8\u09CD\u09A1"},
            {"nt", "\u09A8\u09CD\u09A4"},
            {"ns", "\u09A8\u09CD\u09B8"},
            {"nc", "\u099E\u09CD\u099A"},
            {"NgkSh", "\u0999\u09CD\u0995\u09CD\u09B7"},
            {"Ngkkh", "\u0999\u09CD\u0995\u09CD\u09B7"},
            {"NGch", "\u099E\u09CD\u099B"},
            {"Nggh", "\u0999\u09CD\u0998"},
            {"Ngkh", "\u0999\u09CD\u0996"},
            {"NGjh", "\u099E\u09CD\u099D"},
            {"ngOI", "\u0999\u09CD\u0997\u09C8"},
            {"ngOU", "\u0999\u09CD\u0997\u09CC"},
            {"Ngkx", "\u0999\u09CD\u0995\u09CD\u09B7"},
            {"NGc", "\u099E\u09CD\u099A"},
            {"ngh", "\u0999\u09CD\u0998"},
            {"ngH", "\u0982\u09B9"},
            {"Ngk", "\u0999\u09CD\u0995"},
            {"Ngx", "\u0999\u09CD\u0995\u09CD\u09B7"},
            {"Ngg", "\u0999\u09CD\u0997"},
            {"Ngm", "\u0999\u09CD\u09AE"},
            {"NGj", "\u099E\u09CD\u099C"},
            {"NTh", "\u09A3\u09CD\u09A0"},
            {"nkh", "\u0999\u09CD\u0996"},
            {"ngo", "\u0999\u09CD\u0997"},
            {"nga", "\u0999\u09CD\u0997\u09BE"},
            {"ngi", "\u0999\u09CD\u0997\u09BF"},
            {"ngI", "\u0999\u09CD\u0997\u09C0"},
            {"ngu", "\u0999\u09CD\u0997\u09C1"},
            {"ngU", "\u0999\u09CD\u0997\u09C2"},
            {"nge", "\u0999\u09CD\u0997\u09C7"},
            {"ngO", "\u0999\u09CD\u0997\u09CB"},
            {"NDh", "\u09A3\u09CD\u09A2"},
            {"nsh", "\u09A8\u09B6"},
            {"Ngr", "\u0999\u09CD\u09B0"},
            {"NGr", "\u099E\u09CD\u09B0"},
            {"Ng", "\u0999"},
            {"NG", "\u099E"},
            {"nk", "\u0999\u09CD\u0995"},
            {"NN", "\u09A3\u09CD\u09A3"},
            {"Nn", "\u09A3\u09CD\u09A3"},
            {"Nm", "\u09A3\u09CD\u09AE"},
            {"NT", "\u09A3\u09CD\u099F"},
            {"ND", "\u09A3\u09CD\u09A1"},
            {"cNG", "\u099A\u09CD\u099E"},
            {"cch", "\u099A\u09CD\u099B"},
            {"cc", "\u099A\u09CD\u099A"},
            {"jjh", "\u099C\u09CD\u099D"},
            {"jNG", "\u099C\u09CD\u099E"},
            {"jj", "\u099C\u09CD\u099C"},
            {"tth", "\u09A4\u09CD\u09A5"},
            {"tn", "\u09A4\u09CD\u09A8"},
            {"tm", "\u09A4\u09CD\u09AE"},
            {"tt", "\u09A4\u09CD\u09A4"},
            {"TT", "\u099F\u09CD\u099F"},
            {"Tm", "\u099F\u09CD\u09AE"},
            {"dhn", "\u09A7\u09CD\u09A8"},
            {"dhm", "\u09A7\u09CD\u09AE"},
            {"dgh", "\u09A6\u09CD\u0998"},
            {"ddh", "\u09A6\u09CD\u09A7"},
            {"dbh", "\u09A6\u09CD\u09AD"},
            {"dv", "\u09A6\u09CD\u09AD"},
            {"dm", "\u09A6\u09CD\u09AE"},
            {"dg", "\u09A6\u09CD\u0997"},
            {"dd", "\u09A6\u09CD\u09A6"},
            {"DD", "\u09A1\u09CD\u09A1"},
            {"phl", "\u09AB\u09CD\u09B2"},
            {"pT", "\u09AA\u09CD\u099F"},
            {"pt", "\u09AA\u09CD\u09A4"},
            {"pn", "\u09AA\u09CD\u09A8"},
            {"pp", "\u09AA\u09CD\u09AA"},
            {"pl", "\u09AA\u09CD\u09B2"},
            {"ps", "\u09AA\u09CD\u09B8"},
            {"fl", "\u09AB\u09CD\u09B2"},
            {"bdh", "\u09AC\u09CD\u09A7"},
            {"bhl", "\u09AD\u09CD\u09B2"},
            {"bj", "\u09AC\u09CD\u099C"},
            {"bd", "\u09AC\u09CD\u09A6"},
            {"bb", "\u09AC\u09CD\u09AC"},
            {"bl", "\u09AC\u09CD\u09B2"},
            {"vl", "\u09AD\u09CD\u09B2"},
            {"mth", "\u09AE\u09CD\u09A5"},
            {"mph", "\u09AE\u09CD\u09AB"},
            {"mbh", "\u09AE\u09CD\u09AD"},
            {"mpl", "\u09AE\u09AA\u09CD\u09B2"},
            {"mn", "\u09AE\u09CD\u09A8"},
            {"mp", "\u09AE\u09CD\u09AA"},
            {"mv", "\u09AE\u09CD\u09AD"},
            {"mm", "\u09AE\u09CD\u09AE"},
            {"ml", "\u09AE\u09CD\u09B2"},
            {"mb", "\u09AE\u09CD\u09AC"},
            {"mf", "\u09AE\u09CD\u09AB"},
            {"Rg", "\u09DC\u09CD\u0997"},
            {"oZ", "\u0985\u09CD\u09AF"},
            {"lbh", "\u09B2\u09CD\u09AD"},
            {"ldh", "\u09B2\u09CD\u09A7"},
            {"lkh", "\u09B2\u09CD\u0996"},
            {"lgh", "\u09B2\u09CD\u0998"},
            {"lph", "\u09B2\u09CD\u09AB"},
            {"lk", "\u09B2\u09CD\u0995"},
            {"lg", "\u09B2\u09CD\u0997"},
            {"lT", "\u09B2\u09CD\u099F"},
            {"lD", "\u09B2\u09CD\u09A1"},
            {"lp", "\u09B2\u09CD\u09AA"},
            {"lv", "\u09B2\u09CD\u09AD"},
            {"lm", "\u09B2\u09CD\u09AE"},
            {"ll", "\u09B2\u09CD\u09B2"},
            {"lb", "\u09B2\u09CD\u09AC"},
            {"shch", "\u09B6\u09CD\u099B"},
            {"Sch", "\u09B6\u09CD\u099B"},
            {"skl", "\u09B8\u09CD\u0995\u09CD\u09B2"},
            {"skh", "\u09B8\u09CD\u0996"},
            {"sth", "\u09B8\u09CD\u09A5"},
            {"sph", "\u09B8\u09CD\u09AB"},
            {"shc", "\u09B6\u09CD\u099A"},
            {"sht", "\u09B6\u09CD\u09A4"},
            {"shn", "\u09B6\u09CD\u09A8"},
            {"shm", "\u09B6\u09CD\u09AE"},
            {"spl", "\u09B8\u09CD\u09AA\u09CD\u09B2"},
            {"shl", "\u09B6\u09CD\u09B2"},
            {"sk", "\u09B8\u09CD\u0995"},
            {"Sc", "\u09B6\u09CD\u099A"},
            {"sT", "\u09B8\u09CD\u099F"},
            {"st", "\u09B8\u09CD\u09A4"},
            {"sn", "\u09B8\u09CD\u09A8"},
            {"sp", "\u09B8\u09CD\u09AA"},
            {"sf", "\u09B8\u09CD\u09AB"},
            {"sm", "\u09B8\u09CD\u09AE"},
            {"sl", "\u09B8\u09CD\u09B2"},
            {"ShTh", "\u09B7\u09CD\u09A0"},
            {"Shph", "\u09B7\u09CD\u09AB"},
            {"Shk", "\u09B7\u09CD\u0995"},
            {"ShT", "\u09B7\u09CD\u099F"},
            {"ShN", "\u09B7\u09CD\u09A3"},
            {"Shp", "\u09B7\u09CD\u09AA"},
            {"Shf", "\u09B7\u09CD\u09AB"},
            {"Shm", "\u09B7\u09CD\u09AE"},
            {"SHTh", "\u09B7\u09CD\u09A0"},
            {"SHph", "\u09B7\u09CD\u09AB"},
            {"SHk", "\u09B7\u09CD\u0995"},
            {"SHT", "\u09B7\u09CD\u099F"},
            {"SHN", "\u09B7\u09CD\u09A3"},
            {"SHp", "\u09B7\u09CD\u09AA"},
            {"SHf", "\u09B7\u09CD\u09AB"},
            {"SHm", "\u09B7\u09CD\u09AE"},
            {"Sn", "\u09B6\u09CD\u09A8"},
            {"Sm", "\u09B6\u09CD\u09AE"},
            {"Sl", "\u09B6\u09CD\u09B2"},
            {"hN", "\u09B9\u09CD\u09A3"},
            {"hn", "\u09B9\u09CD\u09A8"},
            {"hm", "\u09B9\u09CD\u09AE"},
            {"hl", "\u09B9\u09CD\u09B2"},
        };

        marks_ = {
            {"^`", "^"},
            {":`", ":"},
            {",,", hasanta()},
            {",.", hasanta() + zwnj()},
            {",..", hasanta() + zwj()},
            {"a`", "\u09BE"},
            {"i`", "\u09BF"},
            {"I`", "\u09C0"},
            {"u`", "\u09C1"},
            {"U`", "\u09C2"},
            {"e`", "\u09C7"},
            {"o`", ""},
            {"O`", "\u09CB"},
            {"OI`", "\u09C8"},
            {"OU`", "\u09CC"},
            {"rri`", "\u09C3"},
            {"./", "\u09BC"},
        };

        punctuations_ = {
            {"`", ""},
            {"``", "`"},
            {".", "\u0964"},
            {"..", "\u0964\u0964"},
            {"...", "..."},
            {"$$", "$"},
            {",", ","},
            {"$", "\u09F3"},
            {"$%", "\u20B9"},
        };

        fola_ = {
            {"y", "\u09AF"},
            {"z", "\u09AF"},
            {"Z", "\u09AF"},
            {"r", "\u09B0"},
            {"w", "\u09AC"},
        };

        reph_ = {{"rr", "\u09B0\u09CD"}};

        numerals_ = {
            {".1", ".\u09E7"}, {".2", ".\u09E8"}, {".3", ".\u09E9"}, {".4", ".\u09EA"},
            {".5", ".\u09EB"}, {".6", ".\u09EC"}, {".7", ".\u09ED"}, {".8", ".\u09EE"},
            {".9", ".\u09EF"}, {".0", ".\u09E6"}, {"1", "\u09E7"},   {"2", "\u09E8"},
            {"3", "\u09E9"},   {"4", "\u09EA"},   {"5", "\u09EB"},   {"6", "\u09EC"},
            {"7", "\u09ED"},   {"8", "\u09EE"},   {"9", "\u09EF"},   {"0", "\u09E6"},
        };

        const Table* const tables[] = {
            &symbols_,
            &vowels_head_,
            &vowels_after_,
            &signs_,
            &letters_,
            &clusters_,
            &marks_,
            &punctuations_,
            &fola_,
            &reph_,
            &numerals_,
        };
        for (const Table* table : tables) {
            for (const auto& entry : *table) {
                lexicon_.insert(entry.first);
                if (entry.first.size() > longest_key_) {
                    longest_key_ = entry.first.size();
                }
            }
        }
        lexicon_.insert("rri");
        if (longest_key_ < 3) {
            longest_key_ = 3;
        }
    }

    // Longest-match tokenizer with the two upstream split heuristics:

    // "…r|ri" and "…T|H" prefer the shorter lexicon entry when the tail

    // characters form their own known token.

    std::vector<std::string> tokenize(const std::string& text) const {
        std::vector<std::string> tokens;
        const char* text_data = text.data();
        const std::size_t text_size = text.size();
        size_t byte_offset = 0;

        while (byte_offset < text_size) {
            bool is_token_matched = false;
            for (size_t key_length = longest_key_; key_length > 0; --key_length) {
                if (byte_offset + key_length > text_size) {
                    continue;
                }
                const std::string candidate_key(text_data + byte_offset, key_length);
                if (!lexicon_.count(candidate_key)) {
                    continue;
                }

                if (key_length > 2 && candidate_key.back() == 'r'
                    && byte_offset + key_length + 2 <= text_size
                    && text_data[byte_offset + key_length] == 'r'
                    && text_data[byte_offset + key_length + 1] == 'i') {
                    const std::string shorter_key(text_data + byte_offset, key_length - 1);
                    if (lexicon_.count(shorter_key)) {
                        tokens.push_back(shorter_key);
                        byte_offset += key_length - 1;
                        is_token_matched = true;
                        break;
                    }
                }
                if (key_length > 1 && candidate_key.back() == 'T'
                    && byte_offset + key_length < text_size
                    && text_data[byte_offset + key_length] == 'H') {
                    const std::string shorter_key(text_data + byte_offset, key_length - 1);
                    if (lexicon_.count(shorter_key)) {
                        tokens.push_back(shorter_key);
                        byte_offset += key_length - 1;
                        is_token_matched = true;
                        break;
                    }
                }

                tokens.push_back(candidate_key);
                byte_offset += key_length;
                is_token_matched = true;
                break;
            }
            if (!is_token_matched) {
                tokens.emplace_back(text_data + byte_offset, 1);
                byte_offset += 1;
            }
        }
        return tokens;
    }

    // Case folding: letters whose case is meaningful to the rule tables keep

    // their case; every other ASCII uppercase letter folds to lowercase.

    std::string fold_case(const std::string& text) const {
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
            case 'h':
            case 'H':
            case 'x':
            case 'X':
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

    std::string convert_segment(const std::string& input_text) const {
        if (input_text.empty()) {
            return "";
        }

        const std::string folded_text = fold_case(input_text);
        const std::vector<std::string> tokens = tokenize(folded_text);
        std::string segment_output;
        segment_output.reserve(folded_text.length() * 3);

        TokenClass previous_token_class = TokenClass::word_start;

        for (size_t token_index = 0; token_index < tokens.size(); ++token_index) {
            const std::string& token = tokens[token_index];

            if (token == " ") {
                segment_output += " ";
                previous_token_class = TokenClass::word_start;
                continue;
            }

            // w: fola after a consonant, full letter after a mark, otherwise

            // the "wa"/"we"/"wo" standalone forms.

            if (token == "w") {
                if (previous_token_class == TokenClass::consonant) {
                    segment_output += hasanta() + fola_.at("w");
                    previous_token_class = TokenClass::consonant;
                } else if (previous_token_class == TokenClass::mark) {
                    segment_output += fola_.at("w");
                    previous_token_class = TokenClass::consonant;
                } else {
                    if (token_index + 1 < tokens.size() && tokens[token_index + 1] == "a") {
                        segment_output += "\u0993\u09DF\u09BE";
                        ++token_index;
                    } else if (token_index + 1 < tokens.size() && tokens[token_index + 1] == "e") {
                        segment_output += "\u0993\u09DF\u09C7";
                        ++token_index;
                    } else {
                        segment_output += "\u0993";
                    }
                    previous_token_class = TokenClass::vowel;
                }
                continue;
            }

            if (token == "x") {
                segment_output += (previous_token_class == TokenClass::word_start)
                                      ? "\u098F\u0995\u09CD\u09B8"
                                      : "\u0995\u09CD\u09B8";
                previous_token_class = TokenClass::consonant;
                continue;
            }

            if (token == "rri") {
                if (token_index + 1 < tokens.size() && tokens[token_index + 1] == "`") {
                    segment_output += "\u09C3";
                } else if (previous_token_class != TokenClass::consonant) {
                    segment_output += "\u098B";
                } else {
                    segment_output += "\u09C3";
                }
                previous_token_class = TokenClass::vowel;
                continue;
            }

            auto symbol_match_iterator = symbols_.find(token);
            if (symbol_match_iterator != symbols_.end()) {
                segment_output += symbol_match_iterator->second;
                previous_token_class = TokenClass::mark;
                continue;
            }

            auto numeral_match_iterator = numerals_.find(token);
            if (numeral_match_iterator != numerals_.end()) {
                segment_output += numeral_match_iterator->second;
                previous_token_class = TokenClass::other;
                continue;
            }

            auto punctuation_match_iterator = punctuations_.find(token);
            if (punctuation_match_iterator != punctuations_.end()) {
                segment_output += punctuation_match_iterator->second;
                previous_token_class = TokenClass::other;
                continue;
            }

            auto reph_match_iterator = reph_.find(token);
            if (reph_match_iterator != reph_.end()) {
                if (previous_token_class == TokenClass::consonant) {
                    segment_output += hasanta() + letters_.at("r") + letters_.at("r");
                    previous_token_class = TokenClass::consonant;
                } else {
                    // Reph lands only when the next token cannot start a

                    // vowel cluster of its own; end-of-token counts as "no".

                    bool is_standalone_reph = false;
                    if (token_index + 1 < tokens.size()) {
                        const std::string& next_token = tokens[token_index + 1];
                        const bool next_is_vowel = vowels_head_.count(next_token) != 0;
                        if (!next_is_vowel && next_token != "r" && next_token != "R") {
                            is_standalone_reph = true;
                        }
                    }
                    if (is_standalone_reph) {
                        segment_output += reph_match_iterator->second;
                        previous_token_class = TokenClass::other;
                    } else {
                        segment_output += "\u09B0\u09B0";
                        previous_token_class = TokenClass::consonant;
                    }
                }
                continue;
            }

            auto mark_match_iterator = marks_.find(token);
            if (mark_match_iterator != marks_.end()) {
                segment_output += mark_match_iterator->second;
                // The hasanta combinators feed another consonant next.

                previous_token_class = (token == ",," || token == ",." || token == ",..")
                                           ? TokenClass::consonant
                                           : TokenClass::vowel;
                continue;
            }

            auto cluster_match_iterator = clusters_.find(token);
            if (cluster_match_iterator != clusters_.end()) {
                segment_output += cluster_match_iterator->second;
                previous_token_class = TokenClass::consonant;
                continue;
            }

            auto fola_match_iterator = fola_.find(token);
            if (fola_match_iterator != fola_.end()) {
                if (previous_token_class == TokenClass::consonant) {
                    if (token == "y" || token == "z" || token == "Z") {
                        const bool prev_is_r = (token_index > 0 && tokens[token_index - 1] == "r");
                        if (prev_is_r) {
                            // "r" + ya-phola renders as ya-phala after a

                            // conjunct-capable consonant, otherwise the ZWJ

                            // form keeps the two visually separate.

                            bool prev2_is_cluster_base = false;
                            if (token_index >= 2) {
                                const std::string& second_previous_token = tokens[token_index - 2];
                                const bool known_consonant =
                                    letters_.count(second_previous_token) != 0
                                    || clusters_.count(second_previous_token) != 0;
                                if (known_consonant && second_previous_token != "r"
                                    && second_previous_token != "y" && second_previous_token != "w"
                                    && second_previous_token != "x") {
                                    prev2_is_cluster_base = true;
                                }
                            }
                            if (prev2_is_cluster_base) {
                                segment_output += hasanta() + fola_match_iterator->second;
                            } else {
                                segment_output += zwj() + hasanta() + fola_match_iterator->second;
                            }
                        } else {
                            segment_output += hasanta() + fola_match_iterator->second;
                        }
                    } else if (token == "r") {
                        // Doubled ra-phola spells the full letter instead.

                        const std::string ra_phola = hasanta() + fola_match_iterator->second;
                        if (segment_output.length() >= ra_phola.length()
                            && segment_output.compare(segment_output.length() - ra_phola.length(),
                                                      ra_phola.length(),
                                                      ra_phola)
                                   == 0) {
                            segment_output += letters_.at("r");
                        } else {
                            segment_output += hasanta() + fola_match_iterator->second;
                        }
                    } else {
                        segment_output += hasanta() + fola_match_iterator->second;
                    }
                } else {
                    // Not after a consonant: the standalone forms.

                    if (token == "r") {
                        segment_output += letters_.at("r");
                    } else if (token == "R") {
                        segment_output += letters_.at("R");
                    } else if (token == "y" || token == "Y") {
                        segment_output += letters_.at("y");
                    } else if (token == "z" || token == "Z") {
                        segment_output += letters_.at("z");
                    }
                }
                previous_token_class = TokenClass::consonant;
                continue;
            }

            auto consonant_match_iterator = letters_.find(token);
            if (consonant_match_iterator != letters_.end()) {
                segment_output += consonant_match_iterator->second;
                previous_token_class = TokenClass::consonant;
                continue;
            }

            auto vowel_match_iterator = vowels_head_.find(token);
            if (vowel_match_iterator != vowels_head_.end()) {
                if (previous_token_class == TokenClass::consonant) {
                    if (token != "o") {
                        auto sign_match_iterator = signs_.find(token);
                        if (sign_match_iterator != signs_.end()) {
                            segment_output += sign_match_iterator->second;
                        }
                    }
                    // "o" after a consonant is the inherent vowel: silent.

                } else if (previous_token_class == TokenClass::vowel) {
                    if (token == "o") {
                        segment_output += "\u0993";
                    } else {
                        auto standalone_vowel_iterator = vowels_after_.find(token);
                        if (standalone_vowel_iterator != vowels_after_.end()) {
                            segment_output += standalone_vowel_iterator->second;
                        }
                    }
                } else {
                    segment_output += vowel_match_iterator->second;
                }
                previous_token_class = TokenClass::vowel;
                continue;
            }

            segment_output += token;
            previous_token_class = TokenClass::other;
        }

        return tidy_vowel_signs(segment_output);
    }

    // Upstream kept a post-pass meant to swap a doubled vowel sign for the

    // standalone vowel form, but its predicate compares single bytes against

    // three-byte UTF-8 code-unit ranges (0x09BE..), which can never match on

    // any platform — the pass is effectively a no-op. The loop below

    // reproduces that behaviour exactly so outputs stay byte-identical.

    std::string tidy_vowel_signs(std::string text) const {
        const auto byte_value = [](char byte_char) {
            return static_cast<int>(static_cast<unsigned char>(byte_char));
        };
        const auto is_sign_byte = [byte_value](char byte_char) {
            const int byte_code = byte_value(byte_char);
            return (byte_code >= 0x09BE && byte_code <= 0x09C4)
                   || (byte_code >= 0x09C7 && byte_code <= 0x09C8)
                   || (byte_code >= 0x09CB && byte_code <= 0x09CC) || (byte_code == 0x09D7);
        };
        const auto corresponding_vowel = [byte_value](char sign_char) {
            const int byte_code = byte_value(sign_char);
            if (byte_code == 0x09BE)
                return 0x0986;
            if (byte_code == 0x09BF)
                return 0x0987;
            if (byte_code == 0x09C0)
                return 0x0988;
            if (byte_code == 0x09C1)
                return 0x0989;
            if (byte_code == 0x09C2)
                return 0x098A;
            if (byte_code == 0x09C3)
                return 0x098B;
            if (byte_code == 0x09C7)
                return 0x098F;
            if (byte_code == 0x09C8)
                return 0x0990;
            if (byte_code == 0x09CB)
                return 0x0993;
            if (byte_code == 0x09CC)
                return 0x0994;
            if (byte_code == 0x09D7)
                return 0x0994;
            return byte_code;
        };
        if (text.length() < 2) {
            return text;
        }
        for (std::size_t byte_index = 1; byte_index < text.length(); ++byte_index) {
            if (is_sign_byte(text[byte_index - 1]) && is_sign_byte(text[byte_index])) {
                text[byte_index] = static_cast<char>(corresponding_vowel(text[byte_index]));
            }
        }
        return text;
    }
};

}  // namespace compat

#endif  // SUBORNO_CLASSIC_PHONETIC_ENGINE_HPP
