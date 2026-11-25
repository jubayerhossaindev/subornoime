// Khipro .mim layout-spec runtime — Suborno's own implementation.
//
// Reads the s-expression keymap format (.mim) into (map)/(state) tables and
// runs the resulting finite state machine over keystrokes. The code in this
// file was written fresh for Suborno (own parser, own runtime structures);
// the observable contract it implements follows the Khipro keymap behaviour
// shipped with BornoTSF and is pinned by the in-repo parity regression suite
// (tests/unit/borno_compat_parity_test.cpp).
//
// SPDX-License-Identifier: GPL-3.0-or-later
// Behaviour provenance: Khipro keymap semantics via BornoTSF (GPL-3).
#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace khipro {

// Guard expressions attached to cond() branches.
enum class GuardKind {
    always_true,  // (1) — always true
    equals_value,  // (= <var> <n>) — variable equals n (missing variable reads 0)
    both_true,  // (& a b) — both sub-guards true (both must parse)
    either_true,  // (| a b) — either sub-guard true (both must still parse)
};

// One parsed guard expression node. Guards form a small tree: equals_value
// leaves compare a state variable against an integer, while both_true /
// either_true combine the two sub-guard trees in lhs / rhs.
struct Guard {
    GuardKind kind = GuardKind::always_true;
    std::string variable;
    int expected_value = 0;
    std::shared_ptr<Guard> lhs;
    std::shared_ptr<Guard> rhs;
};

// Primitive operations a keymap rule can perform per hit.
enum class StepKind {
    store,  // (set <var> <token>): token resolves to a variable first,
    // then falls back to an integer literal (0 when neither).
    insert_text,  // (insert <text>): splice UTF-8 text at the cursor; a single
    // leading '?' on the payload is stripped.
    erase_text,  // (delete @-<n>): drop up to <n> code points before the cursor.
    seek_cursor,  // (move @> | @- | @-<n>): reposition the cursor.
    switch_state,  // (shift <state>): enter another state, running its entry steps.
    finish,  // (commit): no-op marker kept for keymap compatibility.
    choose_branch,  // (cond (guard step...)...): first branch whose guard holds runs.
    undo_text  // (undo): drop one code point before the cursor.
};

struct Step;
// One branch of a (cond ...) step: the guard to evaluate and the steps that
// run when the guard is satisfied.
struct ChoiceBranch {
    Guard guard;
    std::vector<Step> steps;
};

// A single parsed operation of a keymap rule.
struct Step {
    StepKind kind = StepKind::finish;
    std::string target;  // store: variable name; insert: payload; switch_state: state.
    std::string raw_argument;  // store: raw second token (variable name or integer).
    int code_point_count = 0;  // erase/seek: code point count (seek may be negative).
    bool seek_to_end = false;  // seek: (move @>).
    std::vector<ChoiceBranch> branches;  // choose_branch.
};

// One longest-match entry inside a keymap: a literal key sequence and the
// steps that run when the sequence matches the upcoming keystrokes.
struct MapKey {
    std::string key;
    std::vector<Step> steps;
};

// A state row referencing a map plus the steps appended after the map's own.
struct MapRef {
    std::string map;
    std::vector<Step> steps;
};

// One keymap state: a name, the steps to run when the state is entered, and
// the (map, steps) rules evaluated on every keystroke while the state is active.
struct StateNode {
    std::string name;
    std::vector<Step> entry_steps;
    std::vector<MapRef> rules;
};

/**
 * @brief Finite-state-machine interpreter for Khipro .mim keymap files.
 *
 * Parses the s-expression keymap format into (map)/(state) tables and then
 * replays keystroke text through the resulting state machine to produce the
 * composed output text. Used by the Khipro layout adapter; unmapped
 * keystrokes fall back to the initial state and pass their first code point
 * through unchanged.
 */
class SpecEngine {
  public:
    explicit SpecEngine(std::string spec_text);

    // Restores the initial state and clears the keymap variables.
    void reset_to_initial_state();

    // Replays the supplied keystroke text through the state machine and
    // returns the composed output text.
    std::string convert_keystroke_text(const std::string& input);

  private:
    // ---- s-expression layer -------------------------------------------------
    struct Node {
        bool atom = true;
        std::string text;
        std::vector<Node> items;
    };

    // Turns the raw keymap text into a tree of nodes, skipping whitespace
    // and ";;" line comments along the way.
    class TreeReader {
      public:
        explicit TreeReader(std::string spec_text);
        std::vector<Node> read_all_nodes();

      private:
        bool has_remaining_content();
        Node read_node();
        std::string read_quoted_string();
        std::string read_atom();

        std::string spec_text_;
        std::size_t byte_offset_ = 0;
    };

    // ---- table construction ---------------------------------------------------
    void load_specification_tables(const std::vector<Node>& root);
    void load_map_rules(const std::vector<Node>& body);
    void load_state_rules(const std::vector<Node>& body);
    bool make_step_from_node(const Node& node, Step* built_step) const;
    std::shared_ptr<Guard> make_guard_from_node(const Node& node) const;

    // ---- runtime ---------------------------------------------------------------
    void run_state_entry_steps();
    std::pair<const MapKey*, const MapRef*> find_longest_match(const StateNode& state,
                                                               const std::string& input,
                                                               std::size_t input_offset) const;
    void run_rule_steps(const std::vector<Step>& steps,
                        std::u32string* composed_text,
                        std::size_t* code_point_cursor);
    bool is_guard_satisfied(const Guard& guard) const;

    static std::u32string decode_utf8_to_utf32(const std::string& text);
    static std::string encode_utf32_to_utf8(const std::u32string& text);
    static int parse_integer_with_fallback(const std::string& text, int fallback);

    // Longest keys are tried first inside every map (sorted at load time).
    std::unordered_map<std::string, std::vector<MapKey>> map_rules_by_name_;
    std::unordered_map<std::string, StateNode> state_nodes_by_name_;
    std::string state_name_ = "init";
    std::unordered_map<std::string, int> variables_by_name_;
};

}  // namespace khipro
