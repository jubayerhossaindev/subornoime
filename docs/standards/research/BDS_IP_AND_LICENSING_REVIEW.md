# BDS IP and Licensing Review

**Mission**: COMPLETE BDS COMPLIANCE… — **PHASE 2 UPDATED** (2026-11-24)

> **PHASE 2 copyright handling**: the officially published full-text PDFs of
> BDS 1520 (obtained edition) and BDS 1834:2011 (SRC-20/21) were downloaded
> from BCC's public file store WITHOUT purchase/payment/account and are kept
> OUTSIDE the repository. Only allocation FACTS (code positions, printed
> names, Unicode scalars), clause numbers and short identification references
> are committed; no BDS prose, no full text, no table images enter the
> repository or any release package.
**Question**: what may SubornoIME (MIT-licensed) implement, distribute and
claim under BDS/Bijoy/Unicode/third-party rights?

## 1. Bijoy-related intellectual property

- BDS 1738:2018's foreword extract (mission brief) states the national layout
  is based on the Bijoy keyboard and that the rights owner's intellectual
  property rights are preserved ("Bijoy Keyboard-এর ভিত্তিতে তৈরি এবং
  মালিকের intellectual property rights সংরক্ষণ").
- Consequence: **implementing the national mapping from the standard's own
  table is a standards-application question, not a clean-room question** —
  but the mapping's IP status (whether the table itself or its exercise
  requires a licence from the Bijoy rights owner) is NOT resolvable from
  public sources. This is recorded as a formal open item and included in the
  official inquiry drafts.
- SubornoIME's `National` layout was implemented from this project's own
  clean-room national-layout conventions (explicit halant `X`, `Y` for য়,
  backtick nukta — documented in national_layout.cpp) — **no Bijoy driver,
  KLC or binary was copied**. Before any "BDS 1738 conformant" claim, both
  (a) the full standard text and (b) IP clearance must be obtained.

## 2. Clean-room status of the other layouts

| Layout | Source basis | IP posture |
|---|---|---|
| Suborno (phonetic) | Project's own design | Clean-room |
| Suborno Classic (phonetic) | Project's own design | Clean-room |
| Suborno Smart (phonetic) | Project's own rule set written from public behaviour; no upstream source copied | Clean-room implementation; naming may be nominative — trademark care applied (no branding claims beyond compatibility description) |
| Khipro | Project's own compositional engine; the new `qq` explicit-hasanta token follows the **documented public convention** of Khipro Classic (khipro.khiproteam.com quickstart) — convention referenced, no code copied | Clean-room |
| Probhat | Upstream MIT-licensed `probhat-layout-windows` KLC (princebillygk, commit debc8310, re-verified 2026-11-24, sha256 210a62fb…) | MIT licensed — compatible with MIT distribution; attribution in THIRD_PARTY_NOTICES.md |
| English | Trivial passthrough | Clean-room |

The single Probhat modification (AltGr+0x34 → U+09BC) is an original,
documented **"SubornoIME Probhat Compatibility Extension"** — labelled as
such in source comments and `docs/specifications/probhat-keymap.md`, never
claimed as part of the upstream or official specification.

## 3. Official documents copyright

- Full BDS texts (1738/1520/1935/1834) are BSTI-copyrighted paid publications
  (SRC-02/03/18). **None of their text, tables or PDFs enter this repository.**
  Reports cite only standard numbers, titles, and public scope identifications
  with clause-level statuses `BLOCKED`.
- The Bangladesh Standards and Testing Institution Act, 2018 authentic
  English text (SRC-08) is a public law document published by the Ministry of
  Law; verbatim section quotation is lawful and used (EV-A-01..05).
- Unicode chart extracts and `NormalizationTest.txt` are subject to the
  Unicode License/ToU which permit reproduction and derivative data;
  short identification references are used.

## 4. MIT license compatibility inventory

| Item | License | MIT distribution OK? |
|---|---|---|
| SubornoIME source | MIT | Yes (this project) |
| Probhat KLC-derived mapping | MIT (upstream project) | Yes, attribution retained |
| ICU (engine normalization dependency) | ICU License (MIT/X11-compatible) | Yes |
| SQLite (dictionary dependency) | Public domain | Yes |
| GTest | BSD-3 | Test-only; fine |
| Unicode data files | Unicode License v3 | Yes with notice |
| BSTI standard documents | BSTI copyright, paid | **No** — never committed; references only |
| Bijoy mapping/IP | Third-party rights | **No implementation** without clearance |

## 5. Test corpus ownership

All new BDS mission test vectors were derived from public Unicode data
(chart, NormalizationTest.txt) and the project's own documented behaviours.
No third-party proprietary corpus was copied.

## 6. Open items (legal)

1. Bijoy/IP clearance for any future BDS 1738 exact-mapping claim.
2. BSTI written interpretation of mandatory-list applicability to downloadable software (inquiry drafts ready).
3. Licensing/permission requirements if the national layout mapping table is ever re-implemented verbatim from the purchased standard.
