# BDS Applicability Analysis — SubornoIME

**Mission**: COMPLETE BDS COMPLIANCE… — **PHASE 2 UPDATED** (2026-11-24)

> **PHASE 2 changes**: (1) the BCC IT-standards page body now renders
> (SRC-22) — the 23 ISO/IEC adoptions were reviewed and none applies to a
> Bengali IME's normative scope (keyboard layout / character encoding /
> input methodology); the advisory is explicitly non-mandatory. (2) The BDS
> 1520 obtained edition (2011 Second Revision) allocates ৰ/ৱ at Table 2
> positions F0/F1 — the repertoire-level question recorded in §3 is now
> answered from the primary text; layouts still do not emit them.
**Product**: SubornoIME — native Windows 10/11 x64 Bengali IME (C++20, Win32,
TSF, COM, MSVC, CMake). Unicode-based; UTF-8 internal, UTF-16 at the TSF
boundary, NFC normalization at commit.
**Layouts**: 7 — Suborno (phonetic), English, Suborno Classic (phonetic), Suborno Smart
(phonetic), Khipro (compositional), Probhat (fixed physical), National.

## 1. Applicability decision table

| Standard | Scope | Applicable to SubornoIME? | Decision basis | Requirement type for this project |
|---|---|---|---|---|
| **BDS 1738:2018** — Specification for Computer Bangla Keyboard (First Revision) | Computer Bangla keyboard layout: character distribution, physical key positions, modifier layers, Bengali alphabet/digit/sign mapping | **Applicable — for the `National` layout only** | A desktop IME that ships a layout it *claims* is the national layout must meet the standard exactly. The other six layouts are alternative keyboards and MUST NOT be forced into the national mapping (mission rule; preserves layout identity) | Project-level conformance target: implement and verify the national mapping once an authorized copy is obtained. Today: mapping-level work done, clause-level verification BLOCKED |
| **BDS 1520:2018** — Specification for Bangla Coded Character Set for Information Interchange | Bangla character repertoire, coded character set, code allocation, interchange | **Applicable — at character/encoding level** | Any software that produces Bangla text for interchange must emit only valid, correctly encoded repertoire characters. Verified against the Unicode Bengali block (U+0980–U+09FF) with full executable evidence (EV-U-01..05). Clause-level comparison BLOCKED pending full document | Direct technical requirement; repertoire-level verified |
| **BDS 1935:2018** — BSCII (Bangladesh Standard Code for Information Interchange) | Legacy Bangla information-interchange code (8-bit-era character codes per catalogue scope sentence) | **Applicability audited — no mandatory effect on the Unicode pipeline** | SubornoIME is a Unicode IME. BSCII is an interchange code standard; it does not obligate a Unicode IME to emit legacy codes, and injecting BSCII into the Unicode pipeline would violate Unicode correctness. If BSCII interoperability is ever needed, it must be a separate, explicit conversion module (design constraint recorded; not built — no requirement source read) | Design constraint + separate-module rule; clause-level applicability BLOCKED pending full document |
| **BDS 1834:2011** — Bangla Keypad for Mobile Phone | Mobile phone keypad | **N/A** (with justification) | SubornoIME is a Windows 10/11 desktop IME; the standard's product scope is mobile phone keypads. Title/scope identification from the BCC list (SRC-07). Full text not obtained, so the N/A rests on the official title+scope identification, and a BSTI written confirmation is queued (see inquiry drafts) | N/A — justification recorded, not silently dropped |
| Other BCC-listed IT standards (23 ISO/IEC adoptions, SRC-06) | Various IT standards | Evaluated per item | The BCC list body is not retrievable (SRC-06); the mission brief's candidate areas (software maintenance, evaluation, security, documentation) map to ISO/IEC families adopted by BSTI, but the list itself cannot be read on the access date → **UNRESOLVED**; no item may be declared mandatory without reading it | Pending: obtain list via re-render/purchase/clarification |

## 2. Standards Applicability Matrix (mission-required format)

| Standard | Scope | Khipro | Probhat | National | Suborno/Classic/Smart phonetic | English |
|---|---|---|---|---|---|---|
| BDS 1520:2018 | Character repertoire/encoding | Applicable — character level (virama U+09CD, nukta U+09BC verified) | Applicable — character level (nukta U+09BC verified) | Applicable | Applicable | Applicable (ASCII passthrough only) |
| BDS 1738:2018 | National keyboard layout | Reference only — MUST NOT absorb national mapping | Reference only | **Conformance target** (mapping implemented; clause verification BLOCKED) | Not applicable (alternative phonetic layouts, own identity) | Not applicable |
| BDS 1935:2018 | Legacy interchange code | No legacy path (verified) | No legacy path (verified) | No legacy path | No legacy path | No legacy path |
| BDS 1834:2011 | Mobile keypad | N/A | N/A | N/A | N/A | N/A |

## 3. Assamese-specific character policy (BDS 1520 investigation)

Per the mission: Assamese `ৰ` (U+09F0) and `ৱ` (U+09F1) must not be added
gratuitously. Findings:

- Both code points are **assigned** in the Unicode Bengali block (BENGALI
  LETTER RA WITH MIDDLE DIAGONAL / RA WITH LOWER DIAGONAL; BENGALI LETTER WA)
  and are part of the repertoire sweep evidence (EV-U-02, rows REPL-9F0,
  REPL-9F1) — the engine handles their encodings correctly.
- No publicly retrievable BDS 1520 requirement was found that mandates them
  for a Bangla (as opposed to Assamese) repertoire, and no clause can be
  claimed without the full document. Therefore: **no engine or layout change
  was made**; the question is recorded as an open clause-level item, to be
  answered when the authorized BDS 1520 copy is read. Nothing was hidden.

## 4. Unicode version policy

Unicode current version is 18.0.0 (SRC-13). The Bengali block is unchanged
between the current chart and the 18.0.0 chart (SRC-11/SRC-12 byte-identical
sizes; identical extracted text). BDS 1738's foreword extract references
Unicode 10.0. Per the mission rule, the engine does NOT claim "Unicode 18
conformant BDS mapping" — the engine targets the Bengali block as published
in the current Unicode Standard, which for every character BDS 1520's
publicly described repertoire covers is identical content; a BDS clause that
fixes a specific Unicode version cannot be cited or altered without the full
document.

## 5. What would change applicability

- A BSTI/BCC written clarification (inquiry drafts ready: BSTI_OFFICIAL_INQUIRY_BN.md/EN.md) declaring mandatory CM applicability to downloadable software, or a published software-specific scheme.
- Purchase and reading of the full BDS texts → clause matrices upgrade from BLOCKED to PASS/FAIL.
- Publication of the BCC list body again (SRC-06) → other-standards applicability resolved.
