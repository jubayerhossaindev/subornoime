#include "suborno/spell/spell_checker.hpp"

#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "suborno/unicode/bengali.hpp"

namespace bengali {

// Independent vowels (স্বরবর্ণ)

inline constexpr std::u32string_view vowels = U"অআইঈউঊঋঌএঐওঔৠৡ";

// Consonants (ব্যঞ্জনবর্ণ)

inline constexpr std::u32string_view consonants = U"কখগঘঙচছজঝঞটঠডঢণতথদধনপফবভমযরলশষসহ";

// Additional Bengali consonants (অতিরিক্ত ব্যঞ্জনবর্ণ)

inline constexpr std::u32string_view additional_consonants = U"ৎড়ঢ়য়";

// All letters usable as base replacement candidates

inline constexpr std::u32string_view replacements =
    U"অআইঈউঊঋঌএঐওঔৠৡ"
    U"কখগঘঙচছজঝঞটঠডঢণতথদধনপফবভমযরলশষসহ"
    U"ৎড়ঢ়য়";

// Dependent vowel signs (স্বরচিহ্ন / কার)

inline constexpr std::u32string_view vowel_signs = U"ািীুূৃৄেৈোৌৢৣ";

// Other Bengali signs (অন্যান্য চিহ্ন)

inline constexpr std::u32string_view signs = U"ঁংঃ়্ঽৗ৾";

// Bengali digits (বাংলা অঙ্ক)

inline constexpr std::u32string_view digits = U"০১২৩৪৫৬৭৮৯";

// Bengali punctuation (বাংলা দাঁড়ি ও দ্বি-দাঁড়ি)

inline constexpr std::u32string_view punctuation = U"।॥";

// Historical and special Bengali-block characters

inline constexpr std::u32string_view extended_symbols = U"ঀ৲৳৴৵৶৷৸৹৺৻ৼ৽";

}  // namespace bengali

namespace suborno::spell {

namespace {

constexpr std::size_t maximum_word_graphemes = 32;
constexpr std::size_t maximum_generated_edits = 512;
constexpr std::size_t maximum_first_pass_lookups = 128;
constexpr std::size_t maximum_second_pass_lookups = 128;
constexpr std::size_t maximum_second_pass_seeds = 16;
constexpr std::size_t maximum_suggestions = 10;

std::string to_utf8(char32_t code_point) {
    std::string result;
    if (code_point < 0x80U) {
        result.push_back(static_cast<char>(code_point));
    } else if (code_point < 0x800U) {
        result.push_back(static_cast<char>(0xC0U | (static_cast<std::uint32_t>(code_point) >> 6U)));
        result.push_back(
            static_cast<char>(0x80U | (static_cast<std::uint32_t>(code_point) & 0x3FU)));
    } else if (code_point < 0x10000U) {
        result.push_back(
            static_cast<char>(0xE0U | (static_cast<std::uint32_t>(code_point) >> 12U)));
        result.push_back(
            static_cast<char>(0x80U | ((static_cast<std::uint32_t>(code_point) >> 6U) & 0x3FU)));
        result.push_back(
            static_cast<char>(0x80U | (static_cast<std::uint32_t>(code_point) & 0x3FU)));
    } else if (code_point < 0x110000U) {
        result.push_back(
            static_cast<char>(0xF0U | (static_cast<std::uint32_t>(code_point) >> 18U)));
        result.push_back(
            static_cast<char>(0x80U | ((static_cast<std::uint32_t>(code_point) >> 12U) & 0x3FU)));
        result.push_back(
            static_cast<char>(0x80U | ((static_cast<std::uint32_t>(code_point) >> 6U) & 0x3FU)));
        result.push_back(
            static_cast<char>(0x80U | (static_cast<std::uint32_t>(code_point) & 0x3FU)));
    }
    return result;
}

std::vector<std::string> bengali_replacement_graphemes() {
    std::vector<std::string> result;
    result.reserve(bengali::replacements.size());
    for (char32_t cp : bengali::replacements) {
        result.push_back(to_utf8(cp));
    }
    return result;
}

std::vector<std::string> replacement_graphemes_for(const std::vector<std::string>& graphemes) {
    const bool is_ascii_latin_word =
        !graphemes.empty()
        && std::all_of(graphemes.begin(), graphemes.end(), [](const std::string& grapheme) {
               return grapheme.size() == 1
                      && ((grapheme[0] >= 'a' && grapheme[0] <= 'z')
                          || (grapheme[0] >= 'A' && grapheme[0] <= 'Z'));
           });
    if (!is_ascii_latin_word) {
        return bengali_replacement_graphemes();
    }

    const bool uppercase_word =
        std::all_of(graphemes.begin(), graphemes.end(), [](const std::string& grapheme) {
            return grapheme[0] >= 'A' && grapheme[0] <= 'Z';
        });
    std::vector<std::string> result;
    result.reserve(26);
    for (char letter = 'a'; letter <= 'z'; ++letter) {
        const char replacement = uppercase_word ? static_cast<char>(letter - ('a' - 'A')) : letter;
        result.emplace_back(1, replacement);
    }
    return result;
}

}  // namespace

SpellChecker::SpellChecker(const dictionary::Dictionary& dictionary)
    : dictionary_(dictionary) {}

SpellingResult SpellChecker::check(const std::string& word) const {
    SpellingResult result;
    if (word.empty() || !suborno::unicode::is_valid_utf8(word)) {
        return result;
    }

    const auto word_graphemes = suborno::unicode::split_graphemes(word);
    if (word_graphemes.empty() || word_graphemes.size() > maximum_word_graphemes) {
        return result;
    }

    const auto exact_matches = dictionary_.find_matches(word);
    if (!exact_matches.empty()) {
        result.is_correct = true;
        return result;
    }

    struct ScoredCandidate {
        std::string word;
        double score;
    };

    std::vector<ScoredCandidate> scored;
    std::unordered_set<std::string> checked_candidates;
    std::size_t dictionary_lookups = 0;

    const auto consider_candidate = [&](const std::string& candidate) {
        if (dictionary_lookups >= maximum_first_pass_lookups + maximum_second_pass_lookups
            || candidate.empty() || candidate == word
            || !checked_candidates.insert(candidate).second) {
            return;
        }

        ++dictionary_lookups;
        const auto matches = dictionary_.find_matches(candidate);
        if (matches.empty()) {
            return;
        }

        double distance = 0.0;
        try {
            distance = edit_distance(word, candidate);
        } catch (...) {
            // Dictionary words beyond the grapheme limit cannot be scored;

            // skip them instead of letting the exception escape check().

            return;
        }
        const double frequency_boost = static_cast<double>(matches.size());
        scored.push_back({matches.front(), frequency_boost / (1.0 + distance)});
    };

    const auto edits1 = generate_edits(word);
    for (const auto& edit : edits1) {
        if (dictionary_lookups >= maximum_first_pass_lookups) {
            break;
        }
        consider_candidate(edit);
    }

    for (std::size_t index = 0;
         index < edits1.size() && index < maximum_second_pass_seeds
         && dictionary_lookups < maximum_first_pass_lookups + maximum_second_pass_lookups;
         ++index) {
        const auto edits2 = generate_edits(edits1[index]);
        for (const auto& candidate : edits2) {
            if (dictionary_lookups >= maximum_first_pass_lookups + maximum_second_pass_lookups) {
                break;
            }
            consider_candidate(candidate);
        }
    }

    std::sort(scored.begin(),
              scored.end(),
              [](const ScoredCandidate& left, const ScoredCandidate& right) {
                  if (left.score != right.score) {
                      return left.score > right.score;
                  }
                  return left.word < right.word;
              });

    for (std::size_t index = 0; index < scored.size() && index < maximum_suggestions; ++index) {
        result.suggestions.push_back({scored[index].word,
                                      scored[index].word,
                                      "spell:" + scored[index].word,
                                      suborno::layout::CandidateSource::user_dictionary,
                                      scored[index].score,
                                      "spelling suggestion"});
    }

    return result;
}

std::vector<std::string> SpellChecker::generate_edits(const std::string& word) const {
    const auto graphemes = suborno::unicode::split_graphemes(word);
    if (graphemes.size() > maximum_word_graphemes) {
        return {};
    }

    const auto replacements = replacement_graphemes_for(graphemes);
    std::vector<std::string> edits;
    edits.reserve(maximum_generated_edits);
    std::unordered_set<std::string> seen;

    const auto append_edit = [&](std::string candidate) {
        if (edits.size() >= maximum_generated_edits) {
            return;
        }
        if (seen.insert(candidate).second) {
            edits.push_back(std::move(candidate));
        }
    };

    // Deletions

    for (std::size_t deletion_index = 0;
         deletion_index < graphemes.size() && edits.size() < maximum_generated_edits;
         ++deletion_index) {
        std::string candidate;
        for (std::size_t grapheme_index = 0; grapheme_index < graphemes.size(); ++grapheme_index) {
            if (grapheme_index != deletion_index) {
                candidate += graphemes[grapheme_index];
            }
        }
        append_edit(std::move(candidate));
    }

    // Transpositions

    for (std::size_t transposition_index = 0;
         transposition_index + 1 < graphemes.size() && edits.size() < maximum_generated_edits;
         ++transposition_index) {
        std::string candidate;
        for (std::size_t grapheme_index = 0; grapheme_index < graphemes.size(); ++grapheme_index) {
            if (grapheme_index == transposition_index) {
                candidate += graphemes[transposition_index + 1];
            } else if (grapheme_index == transposition_index + 1) {
                candidate += graphemes[transposition_index];
            } else {
                candidate += graphemes[grapheme_index];
            }
        }
        append_edit(std::move(candidate));
    }

    // Interleave insertions and substitutions by position so both typo types

    // remain reachable within the first bounded dictionary lookup pass.

    for (std::size_t position_index = 0;
         position_index < graphemes.size() && edits.size() < maximum_generated_edits;
         ++position_index) {
        for (const auto& replacement : replacements) {
            if (edits.size() >= maximum_generated_edits) {
                break;
            }
            std::string candidate;
            for (std::size_t grapheme_index = 0; grapheme_index < graphemes.size();
                 ++grapheme_index) {
                if (grapheme_index == position_index) {
                    candidate += replacement;
                }
                candidate += graphemes[grapheme_index];
            }
            append_edit(std::move(candidate));
        }

        for (const auto& replacement : replacements) {
            if (edits.size() >= maximum_generated_edits) {
                break;
            }
            std::string candidate;
            for (std::size_t grapheme_index = 0; grapheme_index < graphemes.size();
                 ++grapheme_index) {
                candidate +=
                    grapheme_index == position_index ? replacement : graphemes[grapheme_index];
            }
            append_edit(std::move(candidate));
        }
    }

    // Insertions at the end of the word.

    for (const auto& replacement : replacements) {
        if (edits.size() >= maximum_generated_edits) {
            break;
        }
        std::string candidate;
        for (const auto& grapheme : graphemes) {
            candidate += grapheme;
        }
        candidate += replacement;
        append_edit(std::move(candidate));
    }

    return edits;
}

double SpellChecker::edit_distance(const std::string& a, const std::string& b) const {
    auto graphemes_a = suborno::unicode::split_graphemes(a);
    auto graphemes_b = suborno::unicode::split_graphemes(b);
    if (graphemes_a.size() > maximum_word_graphemes
        || graphemes_b.size() > maximum_word_graphemes) {
        throw std::length_error("Spell-check word exceeds the configured grapheme limit");
    }

    if (graphemes_a.size() < graphemes_b.size()) {
        graphemes_a.swap(graphemes_b);
    }

    std::vector<std::size_t> previous(graphemes_b.size() + 1);
    std::vector<std::size_t> current(graphemes_b.size() + 1);
    std::iota(previous.begin(), previous.end(), 0);

    for (std::size_t row_index = 1; row_index <= graphemes_a.size(); ++row_index) {
        current[0] = row_index;
        for (std::size_t column_index = 1; column_index <= graphemes_b.size(); ++column_index) {
            const std::size_t substitution_cost =
                graphemes_a[row_index - 1] == graphemes_b[column_index - 1] ? 0U : 1U;
            current[column_index] = std::min({previous[column_index] + 1,
                                              current[column_index - 1] + 1,
                                              previous[column_index - 1] + substitution_cost});
        }
        previous.swap(current);
    }

    return static_cast<double>(previous.back());
}

}  // namespace suborno::spell
