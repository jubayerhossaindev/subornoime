// Khipro .mim layout-spec runtime — Suborno's own implementation.
// See borno_compat_mim_engine.hpp for the design contract.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "borno_compat_mim_engine.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <utility>

namespace khipro {

// Accepts whatever std::stoi() accepts, but only when the whole token is
// consumed; anything else falls back. This mirrors the keymap convention of
// treating non-numeric arguments as "absent" rather than as a hard error.
int SpecEngine::parse_integer_with_fallback(const std::string& text, int fallback) {
    try {
        std::size_t consumed_byte_count = 0;
        const int parsed = std::stoi(text, &consumed_byte_count);
        return consumed_byte_count == text.size() ? parsed : fallback;
    } catch (...) {
        return fallback;
    }
}

// ---------------------------------------------------------------------------
// s-expression reader: atoms, raw strings (no escape sequences), nested lists;
// ";;" to end-of-line acts as a comment, a lone ';' is an ordinary atom byte.
// ---------------------------------------------------------------------------

SpecEngine::TreeReader::TreeReader(std::string spec_text)
    : spec_text_(std::move(spec_text)) {}

bool SpecEngine::TreeReader::has_remaining_content() {
    while (byte_offset_ < spec_text_.size()) {
        const unsigned char lead_byte = static_cast<unsigned char>(spec_text_[byte_offset_]);
        if (std::isspace(lead_byte)) {
            ++byte_offset_;
            continue;
        }
        if (spec_text_[byte_offset_] == ';' && byte_offset_ + 1 < spec_text_.size()
            && spec_text_[byte_offset_ + 1] == ';') {
            while (byte_offset_ < spec_text_.size() && spec_text_[byte_offset_] != '\n') {
                ++byte_offset_;
            }
            continue;
        }
        return true;
    }
    return false;
}

std::vector<SpecEngine::Node> SpecEngine::TreeReader::read_all_nodes() {
    std::vector<Node> root_nodes;
    while (has_remaining_content()) {
        root_nodes.push_back(read_node());
    }
    return root_nodes;
}

SpecEngine::Node SpecEngine::TreeReader::read_node() {
    if (!has_remaining_content()) {
        throw std::runtime_error("keymap: unexpected end of input");
    }
    if (spec_text_[byte_offset_] == '(') {
        ++byte_offset_;
        Node group;
        group.atom = false;
        while (has_remaining_content() && spec_text_[byte_offset_] != ')') {
            group.items.push_back(read_node());
        }
        if (byte_offset_ >= spec_text_.size() || spec_text_[byte_offset_] != ')') {
            throw std::runtime_error("keymap: unterminated list");
        }
        ++byte_offset_;
        return group;
    }
    Node single;
    single.atom = true;
    single.text = (spec_text_[byte_offset_] == '"') ? read_quoted_string() : read_atom();
    return single;
}

std::string SpecEngine::TreeReader::read_quoted_string() {
    ++byte_offset_;  // skip the opening quote; payload bytes are kept verbatim
    std::string decoded_value;
    while (byte_offset_ < spec_text_.size() && spec_text_[byte_offset_] != '"') {
        decoded_value.push_back(spec_text_[byte_offset_]);
        ++byte_offset_;
    }
    if (byte_offset_ >= spec_text_.size()) {
        throw std::runtime_error("keymap: unterminated string");
    }
    ++byte_offset_;
    return decoded_value;
}

std::string SpecEngine::TreeReader::read_atom() {
    const std::size_t atom_start_offset = byte_offset_;
    while (byte_offset_ < spec_text_.size()) {
        const unsigned char lead_byte = static_cast<unsigned char>(spec_text_[byte_offset_]);
        if (std::isspace(lead_byte) || spec_text_[byte_offset_] == '('
            || spec_text_[byte_offset_] == ')') {
            break;
        }
        ++byte_offset_;
    }
    return spec_text_.substr(atom_start_offset, byte_offset_ - atom_start_offset);
}

// ---------------------------------------------------------------------------
// Table construction
// ---------------------------------------------------------------------------

SpecEngine::SpecEngine(std::string spec_text) {
    TreeReader tree_reader(std::move(spec_text));
    load_specification_tables(tree_reader.read_all_nodes());
    reset_to_initial_state();
}

void SpecEngine::load_specification_tables(const std::vector<Node>& root) {
    for (const Node& node : root) {
        if (node.atom || node.items.empty() || !node.items[0].atom) {
            continue;
        }
        const std::string& head = node.items[0].text;
        if (head == "map") {
            load_map_rules(std::vector<Node>(node.items.begin() + 1, node.items.end()));
        } else if (head == "state") {
            load_state_rules(std::vector<Node>(node.items.begin() + 1, node.items.end()));
        }
    }
}

void SpecEngine::load_map_rules(const std::vector<Node>& body) {
    for (const Node& group : body) {
        if (group.atom || group.items.empty() || !group.items[0].atom) {
            continue;
        }
        const std::string map_name = group.items[0].text;
        // Repeated maps append to the existing rule list.
        std::vector<MapKey>& entries = map_rules_by_name_[map_name];

        for (std::size_t entry_index = 1; entry_index < group.items.size(); ++entry_index) {
            const Node& item = group.items[entry_index];
            if (item.atom || item.items.empty()) {
                continue;
            }

            std::string key;
            const Node& head = item.items[0];
            if (head.atom) {
                key = head.text;
            } else if (!head.items.empty() && head.items[0].atom) {
                key = head.items[0].text == "BackSpace" ? std::string("\b") : head.items[0].text;
            }
            if (key.empty()) {
                continue;
            }

            MapKey entry;
            entry.key = key;
            std::string output_literal;
            for (std::size_t piece_index = 1; piece_index < item.items.size(); ++piece_index) {
                const Node& piece = item.items[piece_index];
                if (piece.atom) {
                    output_literal = piece.text;  // last bare atom wins
                    continue;
                }
                Step step;
                if (make_step_from_node(piece, &step)) {
                    entry.steps.push_back(std::move(step));
                }
            }
            if (!output_literal.empty()) {
                Step tail_step;
                tail_step.kind = StepKind::insert_text;
                tail_step.target = output_literal;
                entry.steps.push_back(std::move(tail_step));
            }
            entries.push_back(std::move(entry));
        }

        // Longest keys must be tried first.
        std::sort(entries.begin(),
                  entries.end(),
                  [](const MapKey& first_entry, const MapKey& second_entry) {
                      return first_entry.key.size() > second_entry.key.size();
                  });
    }
}

void SpecEngine::load_state_rules(const std::vector<Node>& body) {
    for (const Node& group : body) {
        if (group.atom || group.items.empty() || !group.items[0].atom) {
            continue;
        }

        StateNode state_node;
        state_node.name = group.items[0].text;
        for (std::size_t item_index = 1; item_index < group.items.size(); ++item_index) {
            const Node& item = group.items[item_index];
            if (item.atom || item.items.empty() || !item.items[0].atom) {
                continue;
            }
            const std::string head = item.items[0].text;
            if (head == "t") {
                for (std::size_t step_index = 1; step_index < item.items.size(); ++step_index) {
                    Step step;
                    if (make_step_from_node(item.items[step_index], &step)) {
                        state_node.entry_steps.push_back(std::move(step));
                    }
                }
            } else {
                MapRef rule;
                rule.map = head;
                for (std::size_t step_index = 1; step_index < item.items.size(); ++step_index) {
                    Step step;
                    if (make_step_from_node(item.items[step_index], &step)) {
                        rule.steps.push_back(std::move(step));
                    }
                }
                state_node.rules.push_back(std::move(rule));
            }
        }
        state_nodes_by_name_[state_node.name] = std::move(state_node);
    }
}

bool SpecEngine::make_step_from_node(const Node& node, Step* built_step) const {
    if (node.atom || node.items.empty() || !node.items[0].atom) {
        return false;
    }
    const std::string& head = node.items[0].text;

    if (head == "set") {
        if (node.items.size() >= 3 && node.items[1].atom && node.items[2].atom) {
            built_step->kind = StepKind::store;
            built_step->target = node.items[1].text;
            built_step->raw_argument = node.items[2].text;
            return true;
        }
        return false;
    }
    if (head == "insert") {
        if (node.items.size() >= 2 && node.items[1].atom) {
            built_step->kind = StepKind::insert_text;
            built_step->target = node.items[1].text;
            if (!built_step->target.empty() && built_step->target.front() == '?') {
                built_step->target.erase(0, 1);
            }
            return true;
        }
        return false;
    }
    if (head == "delete") {
        if (node.items.size() >= 2 && node.items[1].atom) {
            const std::string token = node.items[1].text;
            if (token.rfind("@-", 0) == 0) {
                built_step->kind = StepKind::erase_text;
                const std::string count_token = token.substr(2);
                built_step->code_point_count =
                    count_token.empty() ? 1 : parse_integer_with_fallback(count_token, 1);
                return true;
            }
        }
        return false;
    }
    if (head == "move") {
        if (node.items.size() >= 2 && node.items[1].atom) {
            const std::string token = node.items[1].text;
            if (token == "@>") {
                built_step->kind = StepKind::seek_cursor;
                built_step->seek_to_end = true;
                built_step->code_point_count = 0;
                return true;
            }
            if (token == "@-") {
                built_step->kind = StepKind::seek_cursor;
                built_step->code_point_count = -1;
                return true;
            }
            if (token.rfind("@-", 0) == 0) {
                built_step->kind = StepKind::seek_cursor;
                built_step->code_point_count = -parse_integer_with_fallback(token.substr(2), 1);
                return true;
            }
        }
        return false;
    }
    if (head == "shift") {
        if (node.items.size() >= 2 && node.items[1].atom) {
            built_step->kind = StepKind::switch_state;
            built_step->target = node.items[1].text;
            return true;
        }
        return false;
    }
    if (head == "commit") {
        built_step->kind = StepKind::finish;
        return true;
    }
    if (head == "undo") {
        built_step->kind = StepKind::undo_text;
        return true;
    }
    if (head == "cond") {
        Step choice;
        choice.kind = StepKind::choose_branch;
        for (std::size_t branch_index = 1; branch_index < node.items.size(); ++branch_index) {
            const Node& branch = node.items[branch_index];
            if (branch.atom || branch.items.empty()) {
                continue;
            }
            ChoiceBranch arm;
            if (branch.items[0].atom && branch.items[0].text == "1") {
                arm.guard.kind = GuardKind::always_true;
            } else {
                std::shared_ptr<Guard> guard = make_guard_from_node(branch.items[0]);
                if (!guard) {
                    continue;
                }
                arm.guard = *guard;
            }
            for (std::size_t step_index = 1; step_index < branch.items.size(); ++step_index) {
                Step step;
                if (make_step_from_node(branch.items[step_index], &step)) {
                    arm.steps.push_back(std::move(step));
                }
            }
            choice.branches.push_back(std::move(arm));
        }
        if (!choice.branches.empty()) {
            *built_step = std::move(choice);
            return true;
        }
        return false;
    }
    return false;
}

std::shared_ptr<Guard> SpecEngine::make_guard_from_node(const Node& node) const {
    if (node.atom || node.items.empty() || !node.items[0].atom) {
        return nullptr;
    }
    auto guard = std::make_shared<Guard>();
    const std::string& head = node.items[0].text;

    if (head == "1") {
        guard->kind = GuardKind::always_true;
        return guard;
    }
    if (head == "=") {
        if (node.items.size() >= 3 && node.items[1].atom && node.items[2].atom) {
            guard->kind = GuardKind::equals_value;
            guard->variable = node.items[1].text;
            guard->expected_value = parse_integer_with_fallback(node.items[2].text, 0);
            return guard;
        }
        return nullptr;
    }
    if (head == "&" || head == "|") {
        if (node.items.size() >= 3) {
            auto lhs_guard = make_guard_from_node(node.items[1]);
            auto rhs_guard = make_guard_from_node(node.items[2]);
            if (!lhs_guard || !rhs_guard) {
                return nullptr;
            }
            guard->kind = (head == "&") ? GuardKind::both_true : GuardKind::either_true;
            guard->lhs = std::move(lhs_guard);
            guard->rhs = std::move(rhs_guard);
            return guard;
        }
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// Runtime
// ---------------------------------------------------------------------------

void SpecEngine::reset_to_initial_state() {
    state_name_ = "init";
    variables_by_name_.clear();
    run_state_entry_steps();
}

std::pair<const MapKey*, const MapRef*> SpecEngine::find_longest_match(
    const StateNode& state, const std::string& input, std::size_t input_offset) const {
    const MapKey* matched_key = nullptr;
    const MapRef* matched_rule = nullptr;
    std::size_t longest_key_length = 0;

    for (const MapRef& rule : state.rules) {
        const auto map_rule_iterator = map_rules_by_name_.find(rule.map);
        if (map_rule_iterator == map_rules_by_name_.end()) {
            continue;
        }
        for (const MapKey& entry : map_rule_iterator->second) {
            if (entry.key.size() <= longest_key_length) {
                continue;
            }
            if (input_offset + entry.key.size() <= input.size()
                && input.compare(input_offset, entry.key.size(), entry.key) == 0) {
                matched_key = &entry;
                matched_rule = &rule;
                longest_key_length = entry.key.size();
            }
        }
    }
    return {matched_key, matched_rule};
}

std::string SpecEngine::convert_keystroke_text(const std::string& input) {
    reset_to_initial_state();

    std::u32string composed_text;
    std::size_t code_point_cursor = 0;
    std::size_t input_offset = 0;

    while (input_offset < input.size()) {
        const auto state_iterator = state_nodes_by_name_.find(state_name_);
        if (state_iterator == state_nodes_by_name_.end()) {
            break;
        }

        const std::pair<const MapKey*, const MapRef*> longest_match =
            find_longest_match(state_iterator->second, input, input_offset);
        if (!longest_match.first || !longest_match.second) {
            // Unmapped keystroke: pass the keystroke's first code point
            // through and fall back to the initial state.
            const unsigned char lead_byte = static_cast<unsigned char>(input[input_offset]);
            std::size_t sequence_length = 1;
            if ((lead_byte & 0xE0) == 0xC0 && input_offset + 1 < input.size()) {
                sequence_length = 2;
            } else if ((lead_byte & 0xF0) == 0xE0 && input_offset + 2 < input.size()) {
                sequence_length = 3;
            } else if ((lead_byte & 0xF8) == 0xF0 && input_offset + 3 < input.size()) {
                sequence_length = 4;
            }
            const std::u32string decoded_text =
                decode_utf8_to_utf32(input.substr(input_offset, sequence_length));
            if (!decoded_text.empty()) {
                composed_text.insert(
                    composed_text.begin() + static_cast<std::ptrdiff_t>(code_point_cursor),
                    decoded_text[0]);
                ++code_point_cursor;
            }
            input_offset += sequence_length;
            if (state_name_ != "init") {
                state_name_ = "init";
                run_state_entry_steps();
            }
            continue;
        }

        run_rule_steps(longest_match.first->steps, &composed_text, &code_point_cursor);
        run_rule_steps(longest_match.second->steps, &composed_text, &code_point_cursor);
        code_point_cursor = std::min(code_point_cursor, composed_text.size());
        input_offset += longest_match.first->key.size();
    }

    return encode_utf32_to_utf8(composed_text);
}

void SpecEngine::run_state_entry_steps() {
    const auto state_iterator = state_nodes_by_name_.find(state_name_);
    if (state_iterator == state_nodes_by_name_.end()) {
        return;
    }
    // Entry steps may only change state/variables meaningfully: text effects
    // land in a scratch buffer and are discarded by design.
    std::u32string scratch_text;
    std::size_t scratch_code_point_cursor = 0;
    run_rule_steps(state_iterator->second.entry_steps, &scratch_text, &scratch_code_point_cursor);
}

void SpecEngine::run_rule_steps(const std::vector<Step>& steps,
                                std::u32string* composed_text,
                                std::size_t* code_point_cursor) {
    for (const Step& step : steps) {
        switch (step.kind) {
        case StepKind::store: {
            // A variable reference resolves before an integer literal does.
            int resolved_value = parse_integer_with_fallback(step.raw_argument, 0);
            const auto variable_iterator = variables_by_name_.find(step.raw_argument);
            if (variable_iterator != variables_by_name_.end()) {
                resolved_value = variable_iterator->second;
            }
            variables_by_name_[step.target] = resolved_value;
            break;
        }
        case StepKind::insert_text: {
            const std::u32string payload = decode_utf8_to_utf32(step.target);
            composed_text->insert(
                composed_text->begin() + static_cast<std::ptrdiff_t>(*code_point_cursor),
                payload.begin(),
                payload.end());
            *code_point_cursor += payload.size();
            break;
        }
        case StepKind::erase_text: {
            if (*code_point_cursor == 0 || step.code_point_count <= 0) {
                break;
            }
            const std::size_t erase_count = static_cast<std::size_t>(step.code_point_count);
            const std::size_t erase_start =
                (*code_point_cursor > erase_count) ? (*code_point_cursor - erase_count) : 0;
            if (erase_start < *code_point_cursor && erase_start < composed_text->size()) {
                const std::size_t erase_end = std::min(*code_point_cursor, composed_text->size());
                composed_text->erase(
                    composed_text->begin() + static_cast<std::ptrdiff_t>(erase_start),
                    composed_text->begin() + static_cast<std::ptrdiff_t>(erase_end));
                *code_point_cursor = erase_start;
            }
            break;
        }
        case StepKind::seek_cursor: {
            if (step.seek_to_end) {
                *code_point_cursor = composed_text->size();
                break;
            }
            const long moved_cursor = static_cast<long>(*code_point_cursor) + step.code_point_count;
            *code_point_cursor = static_cast<std::size_t>(
                std::clamp<long>(moved_cursor, 0, static_cast<long>(composed_text->size())));
            break;
        }
        case StepKind::switch_state: {
            state_name_ = step.target;
            run_state_entry_steps();
            break;
        }
        case StepKind::finish:
            break;
        case StepKind::undo_text: {
            if (*code_point_cursor > 0 && !composed_text->empty()) {
                const std::size_t undo_offset = *code_point_cursor - 1;
                composed_text->erase(composed_text->begin()
                                     + static_cast<std::ptrdiff_t>(undo_offset));
                *code_point_cursor = undo_offset;
            }
            break;
        }
        case StepKind::choose_branch: {
            for (const ChoiceBranch& branch : step.branches) {
                if (is_guard_satisfied(branch.guard)) {
                    run_rule_steps(branch.steps, composed_text, code_point_cursor);
                    break;
                }
            }
            break;
        }
        }
    }
}

bool SpecEngine::is_guard_satisfied(const Guard& guard) const {
    switch (guard.kind) {
    case GuardKind::always_true:
        return true;
    case GuardKind::equals_value: {
        const auto variable_iterator = variables_by_name_.find(guard.variable);
        return (variable_iterator != variables_by_name_.end() ? variable_iterator->second : 0)
               == guard.expected_value;
    }
    case GuardKind::both_true:
        return guard.lhs && guard.rhs && is_guard_satisfied(*guard.lhs)
               && is_guard_satisfied(*guard.rhs);
    case GuardKind::either_true:
        return guard.lhs && guard.rhs
               && (is_guard_satisfied(*guard.lhs) || is_guard_satisfied(*guard.rhs));
    }
    return false;
}

// ---------------------------------------------------------------------------
// UTF-8 <-> UTF-32 helpers (lenient: malformed lead bytes are skipped).
// ---------------------------------------------------------------------------

std::u32string SpecEngine::decode_utf8_to_utf32(const std::string& text) {
    std::u32string decoded_text;
    std::size_t byte_offset = 0;
    while (byte_offset < text.size()) {
        char32_t code_point = 0;
        const unsigned char lead_byte = static_cast<unsigned char>(text[byte_offset]);
        if (lead_byte < 0x80) {
            code_point = lead_byte;
            ++byte_offset;
        } else if ((lead_byte & 0xE0) == 0xC0 && byte_offset + 1 < text.size()) {
            code_point = (static_cast<char32_t>(lead_byte & 0x1F) << 6)
                         | (static_cast<unsigned char>(text[byte_offset + 1]) & 0x3F);
            byte_offset += 2;
        } else if ((lead_byte & 0xF0) == 0xE0 && byte_offset + 2 < text.size()) {
            code_point = (static_cast<char32_t>(lead_byte & 0x0F) << 12)
                         | (static_cast<unsigned char>(text[byte_offset + 1]) & 0x3F) << 6
                         | (static_cast<unsigned char>(text[byte_offset + 2]) & 0x3F);
            byte_offset += 3;
        } else if ((lead_byte & 0xF8) == 0xF0 && byte_offset + 3 < text.size()) {
            code_point = (static_cast<char32_t>(lead_byte & 0x07) << 18)
                         | (static_cast<unsigned char>(text[byte_offset + 1]) & 0x3F) << 12
                         | (static_cast<unsigned char>(text[byte_offset + 2]) & 0x3F) << 6
                         | (static_cast<unsigned char>(text[byte_offset + 3]) & 0x3F);
            byte_offset += 4;
        } else {
            ++byte_offset;
            continue;
        }
        decoded_text.push_back(code_point);
    }
    return decoded_text;
}

std::string SpecEngine::encode_utf32_to_utf8(const std::u32string& text) {
    std::string encoded_text;
    for (const char32_t code_point : text) {
        if (code_point < 0x80) {
            encoded_text.push_back(static_cast<char>(code_point));
        } else if (code_point < 0x800) {
            encoded_text.push_back(static_cast<char>(0xC0 | (code_point >> 6)));
            encoded_text.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
        } else if (code_point < 0x10000) {
            encoded_text.push_back(static_cast<char>(0xE0 | (code_point >> 12)));
            encoded_text.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
            encoded_text.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
        } else {
            encoded_text.push_back(static_cast<char>(0xF0 | (code_point >> 18)));
            encoded_text.push_back(static_cast<char>(0x80 | ((code_point >> 12) & 0x3F)));
            encoded_text.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
            encoded_text.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
        }
    }
    return encoded_text;
}

}  // namespace khipro
