# Part III — Navigation and Editing Key Verification

Mission §2. Scope: `PhysicalKey` navigation/editing keys and their
classifiers, executed on Linux (Release binary, commit `909dec1` +
verification corrections).

## Source inspection

`include/suborno/layout-api/key_event.hpp` (added in the v20 wave) defines 14
non-text keys with stable enum values that cannot collide with the
ASCII-backed letters/digits/punctuation:

`tab (180), return_key (181), escape (182), backspace_key (183), delete_key
(184), insert_key (185), home (186), end (187), page_up (188), page_down
(189), arrow_left (190), arrow_up (191), arrow_right (192), arrow_down (193)`

Classifiers inspected for mutual consistency:

- `is_editing_key` → `backspace_key`, `delete_key` only.
- `is_navigation_key` → 4 arrows + home/end/page_up/page_down only.
- `is_action_key` → `return_key`, `escape`, `tab` only.
- The three sets are disjoint — asserted by
  `KeyEventMatrix.EditingNavigationAndActionKeysAreClassified` for every
  member of every set, so a routing switch in TSF/standalone layers cannot
  double-handle a key.
- `key_event_to_ascii` maps none of the 14 keys (with or without Shift) to
  ASCII — asserted by `KeyEventMatrix.NonTextKeysNeverMapToAscii` for all 14
  keys in both shift states.

Event-routing design check: the base `append_key` answers non-text keys with
`InputStatus::empty` and never touches the composition; `backspace()` is the
only editing entry point that mutates state. Insert/Delete/Home/End/Page keys
are caret-side operations owned by the host application (TSF `ITfRange`
edits), so the session reports "not text" and stays out of the way — this is
the documented routing contract, not an omission.

## Executed tests (Release, exit code 0)

| Command (filter) | Result | Exit |
|---|---|---|
| `--gtest_filter=KeyEventMatrix.NavigationAndActionKeysDuringCompositionPreserveState:KeyEventMatrix.EveryPhysicalKeyPassesThroughWithoutCorruption:KeyEventMatrix.EditingNavigationAndActionKeysAreClassified:KeyEventMatrix.NonTextKeysNeverMapToAscii` | 4 passed | 0 |
| `--gtest_filter=KeyEventMatrix.*` | 11 passed | 0 |
| full `suborno.unit` | 686 passed | 0 |

## Required scenario coverage (mission list)

- **Every supported physical key** —
  `EveryPhysicalKeyPassesThroughWithoutCorruption` sweeps every `PhysicalKey`
  value 0–200 through a live session: no crash, raw buffer stays printable
  ASCII, status ∈ {empty, updated, pending, passthrough, input_limit}, and the
  session remains committable afterwards.
- **Classification of each key** —
  `EditingNavigationAndActionKeysAreClassified` (membership + disjointness).
- **Text vs non-text behavior** — `NonTextKeysNeverMapToAscii` (both shift
  states) plus the sweep.
- **No accidental double handling** — disjoint classifiers asserted; base
  `append_key` routes everything non-text to a single `empty` return.
- **Non-text keys do not corrupt a live composition** —
  `NavigationAndActionKeysDuringCompositionPreserveState` composes `ami`
  (preedit `আমি`), injects each of the 12 non-backspace keys individually,
  asserts snapshot identity (raw + preedit + candidate count unchanged), then
  commits and requires exactly `আমি`.
- **Composition committable after navigation/editing events** — same test's
  final commit per key.
- **Invalid/unsupported keys** — `PhysicalKey::unknown` and every gap value in
  0–200 go through the exhaustive sweep with the same invariants.
- **Buffer limits under event processing** —
  `AutoRepeatAboveSessionLimitIsRejectedAtomically` (reject above 256) and the
  sweep's `input_limit` branch.

## Verdict

`PORTABLE CORE — VERIFIED`. All 14 keys exist in the event model, the
classifiers are mutually consistent, no key corrupts a live composition, and
invalid values are safe. (Windows-side routing of these keys inside the TSF
text service remains `WINDOWS SOURCE AUDIT — VERIFIED` at source level; see
`windows_runtime_limitations.md`.)
