# Part III — Candidate Navigation and Selection Verification

Mission §3. Scope: `CandidateEngine` ↔ `InputSession` interaction for
navigation, selection, commit and reset, executed on Linux (Release binary,
commit `909dec1` + verification corrections).

## Source inspection

`src/layouts/phonetic/phonetic_input_session.cpp` (the reference
implementation; `LayoutInputSession` mirrors the same contract for fixed
layouts):

- `select_candidate(index)` — `false` when `index >= candidate_count`, no
  state change; otherwise sets the index.
- `select_next_candidate()` — `false` on empty list or at the last element;
  **no wraparound past the end**.
- `select_previous_candidate()` — `false` at index 0; **no wraparound below
  the start**.
- `commit_candidate(index)` — empty composition → `InputStatus::empty`; index
  out of range → `InputStatus::candidate_out_of_range` with the composition
  fully intact (atomic failure, retry possible); success commits
  `candidates[index].text`, records dictionary learning, and resets raw input
  and selection.
- `commit()` — commits the currently selected candidate.
- `reset()` — clears raw input and selection; the session is immediately
  reusable.

Stale-selection protection: `commit_candidate` and `select_candidate` always
operate on a freshly recomputed candidate list; `backspace()` resets the
selection to 0, so a selection can never outlive the composition it belongs
to.

## Executed tests (Release, exit code 0)

| Command (filter) | Result | Exit |
|---|---|---|
| `--gtest_filter=CandidateNavigation.*` | 6 passed | 0 |
| `--gtest_filter=PhoneticCandidateTest.*:CandidateEngineTest.*:PhoneticInputSessionTest.*:LayoutInputSessionTest.*` | 29 passed | 0 |
| full `suborno.unit` | 686 passed | 0 |

Executed under ASan+UBSan as well (see `logs/asan_ubsan.log`).

## Required scenario coverage (mission list 1–8)

1. **Moving forward selects the expected candidate** —
   `NextAndPreviousWalkTheListWithoutWrapping` (first tap → index 1; walk to
   the end lands on `size()-1`).
2. **Moving backward selects the expected candidate** — same test walks back
   to 0.
3. **Navigation stops at both boundaries** — same test asserts `false` at both
   ends (no wraparound either direction).
4. **Invalid selection does not corrupt the selection** —
   `OutOfRangeSelectionIsRejectedAtomically` (`select_candidate(size)` and
   `select_candidate(9999)` → false, index still 0, snapshot untouched).
5. **Commit inserts the selected candidate** —
   `CommitFollowsTheSelectedCandidate` (`commit()` after selecting index 1
   yields exactly `candidates[1].text`; `commit_candidate(0)` yields
   `candidates[0].text`).
6. **Reset restores a usable state** —
   `ResetClearsCompositionAndNavigationState` (empty snapshot, index 0, then
   `ami` → `আমি` commit succeeds immediately after reset).
7. **Rapid navigation never violates index bounds** —
   `RapidSelectionChurnKeepsIndexInsideBounds` (500 compose cycles × 10
   next/prev taps, invariant `index < candidates.size()` after every tap).
8. **Empty candidate state remains safe** —
   `NavigationOnEmptyCompositionIsRejected` (all navigation calls false,
   `commit_candidate` → `InputStatus::empty`, no crash).

Additional out-of-range commit behavior: `commit_candidate(size)` →
`candidate_out_of_range` with the composition preserved (covered inside test
4's file, `OutOfRangeSelectionIsRejectedAtomically`).

## Verdict

`PORTABLE CORE — VERIFIED`. Navigation, boundaries, atomic rejection, commit
selection, reset recovery and churn safety all execute green; no invalid
index, stale selection or unexpected wraparound exists in the tested contract.
