#include "suborno/candidates/candidate_engine.hpp"

#include <string>
#include <unordered_set>
#include <utility>

#include "suborno/dictionary/dictionary.hpp"
#include "suborno/spell/spell_checker.hpp"
#include "suborno/unicode/bengali.hpp"

namespace suborno::candidates {
namespace {

// Layout variants describe what the user literally typed, so they must never

// be pushed out of the visible list by dictionary prefix completions.

constexpr std::size_t minimum_layout_slots = 2;
constexpr std::size_t maximum_spell_suggestions = 3;

}  // namespace

CandidateEngine::CandidateEngine(const suborno::layout::InputLayout& layout,
                                 const suborno::dictionary::Dictionary* dictionary)
    : layout_(layout),
      dictionary_(dictionary) {
    if (dictionary_ != nullptr) {
        // Bounded-cost spelling suggestions for complete-but-unknown words.

        spell_checker_ = std::make_unique<spell::SpellChecker>(*dictionary_);
    }
}

CandidateEngine::CandidateEngine(const suborno::layout::InputLayout& layout,
                                 const suborno::dictionary::Dictionary& dictionary)
    : CandidateEngine(layout, &dictionary) {}

CandidateEngine::~CandidateEngine() = default;

std::vector<suborno::layout::Candidate> CandidateEngine::make_candidates(
    std::string_view input,
    std::size_t candidate_limit,
    const std::string& previous_word_normalized) const {
    std::vector<suborno::layout::Candidate> result;
    if (candidate_limit == 0) {
        return result;
    }

    std::unordered_set<std::string> seen;
    const std::string input_text(input);
    const std::string normalized_input =
        suborno::unicode::normalize_nfc(layout_.transliterate(input_text));

    const auto push_candidate = [&](suborno::layout::Candidate candidate) {
        std::string normalized_text =
            candidate.normalized_text.empty()
                ? suborno::unicode::normalize_nfc(candidate.text)
                : suborno::unicode::normalize_nfc(candidate.normalized_text);
        if (!seen.insert(normalized_text).second) {
            return false;
        }
        candidate.normalized_text = std::move(normalized_text);
        result.push_back(std::move(candidate));
        return true;
    };

    std::size_t exact_match_count = 0;

    // Stage 1: dictionary exact matches.

    if (dictionary_ != nullptr) {
        const auto append_dictionary_matches =
            [&](const std::string& query, double score, const char* rationale) {
                const auto matches = dictionary_->find_matches(query, candidate_limit);
                for (const std::string& word : matches) {
                    if (result.size() >= candidate_limit) {
                        break;
                    }
                    const std::string normalized_word = suborno::unicode::normalize_nfc(word);
                    if (push_candidate({
                            .text = word,
                            .normalized_text = normalized_word,
                            .stable_id = "system-dictionary:" + normalized_word,
                            .source = suborno::layout::CandidateSource::system_dictionary,
                            .score = score,
                            .rationale = rationale,
                        })) {
                        ++exact_match_count;
                    }
                }
            };

        append_dictionary_matches(normalized_input, 1.0, "exact dictionary match");
        if (input_text != normalized_input) {
            append_dictionary_matches(input_text, 0.9, "input dictionary match");
        }
    }

    // Stage 2: next-word prediction. Words that historically followed the

    // previously committed word sit between exact matches (1.0) and prefix

    // completions (0.75): they are strong guesses, not literal echoes of the

    // input. An empty previous word (or no transition history at all) leaves

    // this stage empty, so the list is identical to the pre-prediction one.

    if (dictionary_ != nullptr && !previous_word_normalized.empty()) {
        const auto predictions = dictionary_->transition_candidates(
            previous_word_normalized, normalized_input, candidate_limit);
        for (const std::string& word : predictions) {
            if (result.size() >= candidate_limit) {
                break;
            }
            push_candidate({
                .text = word,
                .normalized_text = {},
                .stable_id = "next-word:" + word,
                .source = suborno::layout::CandidateSource::system_dictionary,
                .score = 0.85,
                .rationale = "next-word prediction",
            });
        }
    }

    // Stage 3: prefix completions. They may occupy at most

    // (candidate_limit - minimum_layout_slots) slots so the layout variants

    // below always stay visible (the pre-fix starvation bug).

    if (dictionary_ != nullptr && result.size() < candidate_limit) {
        const std::size_t prefix_capacity =
            candidate_limit > minimum_layout_slots + result.size()
                ? candidate_limit - minimum_layout_slots - result.size()
                : 0;
        const auto append_prefix_matches = [&](const std::string& query,
                                               double score,
                                               const char* rationale) {
            if (prefix_capacity == 0) {
                return;
            }
            std::size_t accepted_from_stage = 0;
            const auto matches = dictionary_->find_prefix_matches(query, prefix_capacity);
            for (const std::string& word : matches) {
                if (accepted_from_stage >= prefix_capacity || result.size() >= candidate_limit) {
                    break;
                }
                if (push_candidate({
                        .text = word,
                        .normalized_text = {},
                        .stable_id = "dictionary-prefix:" + word,
                        .source = suborno::layout::CandidateSource::system_dictionary,
                        .score = score,
                        .rationale = rationale,
                    })) {
                    ++accepted_from_stage;
                }
            }
        };

        append_prefix_matches(input_text, 0.75, "normalized input prefix match");
        if (normalized_input != input_text) {
            append_prefix_matches(normalized_input, 0.7, "transliterated prefix match");
        }
    }

    // Stage 4: spelling suggestions. Only fire when the typed word is

    // complete-but-unknown: there is no exact match and no prefix completion

    // available (mid-typing prefixes mean the word is simply unfinished).

    if (spell_checker_ != nullptr && exact_match_count == 0 && result.size() < candidate_limit) {
        const bool prefix_completions_available =
            dictionary_ != nullptr && !dictionary_->find_prefix_matches(input_text, 1).empty();
        if (!prefix_completions_available) {
            try {
                const spell::SpellingResult spelling = spell_checker_->check(normalized_input);
                if (!spelling.is_correct) {
                    for (const suborno::layout::Candidate& suggestion : spelling.suggestions) {
                        if (result.size() >= candidate_limit
                            || result.size() >= maximum_spell_suggestions) {
                            break;
                        }
                        suborno::layout::Candidate corrected = suggestion;
                        corrected.source = suborno::layout::CandidateSource::system_dictionary;
                        corrected.rationale = "spelling suggestion";
                        corrected.score = 0.95 / static_cast<double>(result.size() + 1);
                        push_candidate(std::move(corrected));
                    }
                }
            } catch (...) {
                // Spelling is a best-effort stage; a failure (e.g. an

                // over-limit grapheme count) must never break candidates.
            }
        }
    }

    // Stage 5: layout alternatives. Always appended while room remains, and

    // the prefix cap above guarantees at least minimum_layout_slots of them.

    for (const auto& candidate : layout_.generate_candidates(input_text)) {
        if (result.size() >= candidate_limit) {
            break;
        }
        push_candidate(candidate);
    }

    // Position-decay scores keep the visible order monotonically decreasing

    // across sources; dictionary exact/input matches keep their own scores.

    for (std::size_t index = 0; index < result.size(); ++index) {
        if (result[index].source == suborno::layout::CandidateSource::layout) {
            result[index].score = 1.0 / static_cast<double>(index + 1);
        }
    }

    return result;
}

}  // namespace suborno::candidates
