# Comment Standards

Comments explain **why**, never narrate **what**. Before writing one, answer:
why is this code needed, which invariant does it keep, which edge case or
platform limitation matters, who owns the resource, which Unicode constraint
applies?

## Rules

1. A comment must be true, current, and synchronized with the code. If the
   code changes, the comment changes with it — stale comments are bugs.
2. Do not narrate the obvious:

   ```cpp
   // Bad:  ++candidate_index;                 // Increment candidate index.
   // Good: // Skip candidates that do not match the current phonetic prefix.
   ```

3. Do not cover an unclear name with a comment — fix the name first, then
   comment only what still needs explaining.
4. No obsolete project references. Comments describe **this** implementation;
   history and provenance live in dedicated provenance lines and in
   `COMMENT_MIGRATION_REPORT.md`.
5. Language: simple, precise, professional English.
6. Style: implementation notes use `//`; public API documentation uses the
   Doxygen forms defined in `DOXYGEN_STANDARDS.md`.
7. Provenance lines (`SPDX-License-Identifier:`, `Behaviour provenance: …`)
   are legal records: they must not be removed or paraphrased away.
8. No ending markers ("End of file"), no completion notes, no unrequested
   meta commentary.
9. Density: no comment-free complex algorithm; no comment-only trivial code.
   The `.mim` parser state machine and the Unicode conversion helpers are the
   reference examples for the right level.

## Provenance and legal notes

Third-party-derived behaviour keeps an explicit provenance header, for example
in `src/layouts/compat/*`:

```text
// SPDX-License-Identifier: GPL-3.0-or-later
// Behaviour provenance: the legacy reference behaviour contract; see LICENSE and THIRD_PARTY_NOTICES.md..
```

These lines are attribution obligations, not implementation comments — they
survive any refactoring (see §10.4 of the engineering mandate and
`COMMENT_MIGRATION_REPORT.md`).
