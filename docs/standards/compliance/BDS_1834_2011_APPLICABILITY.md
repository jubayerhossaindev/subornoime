# BDS 1834:2011 — Applicability Statement (Bangla Keypad for Mobile Phone)

**Standard**: BDS 1834:2011 — *Bangla Keypad for Mobile Phone* (BSTI)
**Listed in**: BCC "Information Technology (IT) related Bangladesh Standards"
item 26 (SRC-22); **PHASE 2: full official text OBTAINED via BCC official file
store (SRC-21, 6 pages, SHA256 `0e9f2364edf7ade14480f5d8e233322e74d62ae3458d1cf42a0fb91aba46e942`) and READ**.
**Product assessed**: SubornoIME — native Windows 10/11 x64 desktop IME.

## Applicability decision: **N/A — grounded in the obtained primary text**

### Justification (PHASE 2 — now from the official document itself)

1. **Product-scope mismatch, verified from the primary text.** The obtained
   full text reads: clause 1.1 — "The standard defines layouts for input of
   Bangla characters, signs, symbols and currency marks using industry
   standard 3 X 4 Mobile Keypad"; clause 2 (NUMBER OF KEYS) — "This standard
   defines a layout with 12 keys (3 X 4 Keypad)". SubornoIME is a desktop
   operating system input-method editor for Windows PCs (full keyboards,
   TSF text services); it ships no mobile keypad and no mobile product
   surface. The scope mismatch is no longer an inference from the title — it
   is the standard's own scope clause.
2. **Not silently exempted.** The full text was obtained and read before the
   N/A was finalized. Foreword context (mass mobile-phone localization) is
   mobile-sector-specific throughout the obtained text.
3. **BSTI confirmation still queued.** The official inquiry drafts
   (`BSTI_OFFICIAL_INQUIRY_BN.md` / `EN.md`) include a question asking BSTI
   to confirm that BDS 1834:2011 does not apply to a Windows desktop IME.
   Until a written reply arrives, the master matrix carries this item as
   `N/A (justified, grounded in official text) / PENDING OFFICIAL CONFIRMATION`.

### Cross-checks performed

- No SubornoIME component (engine, TSF layer, installer, layouts) implements
  a mobile keypad model (12-key multitap etc.).
- No BDS 1834-derived mapping table exists anywhere in the repository
  (verified by design review; the seven layouts derive from their own
  documented sources — see BDS_IP_AND_LICENSING_REVIEW.md).

### What would flip this decision

- A BSTI/BCC clarification that BDS 1834 covers soft keyboards generally.
- A future mobile/soft-keypad product derived from SubornoIME → would then
  apply the obtained full text directly (already held) and run a dedicated
  conformance programme.

**Status in master matrix**: `N/A (justified, grounded in obtained official
text) — written confirmation pending`.
