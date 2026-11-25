# Borno Comment Migration Record

**Status:** Complete for this refactoring wave; the audit is an automated CI
gate.

## What was migrated

All legacy "Borno"-brand implementation commentary was replaced with original
Suborno documentation: plain-English, technically accurate, Doxygen-compatible
comments describing the code as it is now — not renamed copies of the old text
(the migration report `COMMENT_MIGRATION_REPORT.md` records the per-file
inventory of that pass).

## What was deliberately preserved (§16.4 exception list)

Nothing below is residual implementation commentary; each category documents a
real Suborno feature, a required attribution, or recorded provenance. The
automated audit (`scripts/audit_comments.py`) enforces exactly these
categories:

1. **Legacy-format compatibility module** (`src/layouts/borno_compat/`) — the
   module imports legacy Borno keyboard layouts; its identifiers and
   documentation name the format it exists to support. This is the product
   feature, not leftover commentary.
2. **The Borno Phonetic layout** — a supported, user-visible layout name
   (`borno_phonetic_layout.*`, candidate/typing regression tests).
3. **Compatibility switches** — settings that control the compatibility
   engines (`BornoTSF compatibility switches`, legacy registry value
   compatibility).
4. **Required attribution** — GPL-3.0-or-later provenance notes tied to the
   compatibility engine's sources (`COMMENT_STANDARDS.md` behavior-provenance
   example). License headers, NOTICE and third-party attributions are never
   removed.
5. **Measured provenance** — comments recording where measured values came
   from (e.g. the dark-theme palette measured against the legacy reference).
6. **Regression evidence** — tests documenting fixed legacy defects
   (`Regression (BUG-05): the Borno display name ...`).

## The automated audit

- `python3 scripts/audit_comments.py` scans 300+ files (code, docs, packaging),
  extracting comment spans only, matching the standalone word
  "borno" (never the substring inside "Suborno"/"SUBORNO"), never treating
  `https://...` URLs as comments.
- Allowlists: the compatibility module files, the migration documentation
  itself (`COMMENT_MIGRATION_REPORT.md`, `BORNO_COMMENT_MIGRATION.md`,
  `REFACTORING_REPORT.md`), the legacy-reference research/spec documents, and
  the reviewed hit categories above.
- CI runs it on every push (`ci.yml` naming-and-format job). A new hit fails
  the build until it is either rewritten in Suborno terms or explicitly
  classified into a preserved category.

## Rule of thumb for future changes

If a comment explains *how the legacy product worked*, rewrite it to describe
what Suborno's code does now. If it records *where something came from* or *a
legal requirement*, keep it and make that role explicit.
