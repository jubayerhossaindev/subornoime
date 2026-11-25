# Bangladesh Bengali Standards Compliance — SubornoIME Khipro & Probhat Character Support

**Scope**: BDS standards applicability audit for the 2026-11-24 mission
(Khipro trailing virama `ক্` = U+0995 U+09CD, Probhat standalone nukta `়` =
U+09BC). This document separates (a) what was verified against official
standard identities and public technical descriptions, (b) what was verified
directly against the Unicode Standard (fully verifiable), and (c) what remains
blocked because the full BSTI standard texts are copyright-protected and not
part of this repository.

## 1. Standards Applicability Matrix

| Standard | Official title (public reference) | Scope | Khipro | Probhat | Requirement in this mission |
|---|---|---|---|---|---|
| BDS 1520:2018 | Specification for Bangla Coded Character Set for Information Interchange (BSTI) | Bangla character repertoire / coded character set | Applicable — character level | Applicable — character level | Verify that U+09CD (virama/hasanta) and U+09BC (nukta) and their carrier consonants are part of the Bangla character repertoire |
| BDS 1738:2018 | Specification for Computer Bangla Keyboard (First Revision) (BSTI) | National computer keyboard layout | Reference only | Reference only | Do NOT overwrite either layout's own identity with the national mapping; preserve original key conventions |
| BDS 1935:2018 | Bangladesh Standard Code for Information Interchange — BSCII (BSTI) | Information interchange code | Applicability audited | Applicability audited | No legacy 8-bit mapping may be introduced into the Unicode pipeline |

## 2. What was verified (with evidence)

### 2.1 BDS 1520:2018 — character repertoire

Verified facts:

- Public technical sources describe BDS 1520 (2000 revision onward) as based on
  the ISO/IEC 10646 / Unicode character encoding model ("Bangla computing:
  follies all the way", 2021; BSTI catalogue 2018 lists BDS 1520:2018). The
  2018 revision continues that alignment.
- U+09CD BENGALI SIGN VIRAMA (hasanta) — general category Mn, canonical
  combining class 9, no decomposition mapping, no composition — is part of the
  Bengali block repertoire (U+0980–U+09FF) that BDS 1520's Unicode-aligned
  model covers.
- U+09BC BENGALI SIGN NUKTA — Mn, ccc=7, no decomposition — same.
- Carrier consonants used by the mission cases (ক U+0995, গ U+0997, ত U+09A4,
  ম U+09AE, ন U+09A8, র U+09B0, ড U+09A1, ঢ U+09A2, য U+09AF) are all in the
  Bengali block.
- Engine evidence: every mission case commits the exact scalar sequence — see
  `reports/layout-fixes/UNICODE_VERIFICATION.csv` (UNI-001..UNI-010,
  UNI-UTF8-01/02) and `reports/layout-fixes/KHIPRO_PROBHAT_REGRESSION_RESULTS.csv`
  (579 rows, 0 FAIL).

Compliance statement: **VERIFIED at character-repertoire level** (both
characters and their sequences are valid Bangla repertoire characters with
correct encoding). **Clause-level verification BLOCKED** — the full BSTI
document is copyright-protected; no clause numbers are claimed anywhere in
this repository. No compliance claim in this document relies on an inferred
clause.

### 2.2 BDS 1738:2018 — national keyboard mapping

Verified facts:

- BDS 1738:2018 defines the National (Jatiya) computer Bangla keyboard; public
  sources describe it as Bijoy-inspired with Unicode additions, four shift
  levels (BCC/BSTI public references; Ekushey community review notes glyph
  errata inside the official document itself).
- The mission deliberately did **not** inject the national mapping into Khipro
  or Probhat:
  - Khipro keeps its own compositional convention; the new explicit hasanta
    token `qq` follows the **upstream Khipro Classic convention**
    (khipro.khiproteam.com quickstart), not the BDS national halant key.
  - Probhat keeps the upstream MIT Probhat KLC table; the single added key
    (AltGr+0x34 → U+09BC) fills an upstream-empty (-1) column and is documented
    as a SubornoIME Compatibility Extension — it is not claimed to be part of
    the Probhat specification, nor of BDS 1738's national layout.
- Cross-reference inside this repository: the `national` layout already
  implements the national-style conventions (explicit halant `X`, য় via `Y`,
  nukta via backtick) and is untouched by this mission.

Compliance statement: **COMPLIANT by non-interference** — original layout
identity preserved; no national-mapping conflation. Clause-level mapping
comparison BLOCKED (copyright-protected text).

### 2.3 BDS 1935:2018 — BSCII applicability audit

- BSCII is an information-interchange code standard (BSTI catalogue; BCC
  pages). The Bangladesh Computer Council publicly describes the Bangla
  interchange direction as Unicode-based coding ("বাংলা ভাষার জন্য ইউনিকোড
  ভিত্তিক কোডিং পদ্ধতি").
- SubornoIME's pipeline is fully Unicode (UTF-8 internal, UTF-16 at the TSF
  boundary; NFC normalisation at commit). No 8-bit BSCII mapping was applied
  to the current pipeline; no legacy encoding path was introduced by this
  mission.

Compliance statement: **APPLICABILITY AUDIT COMPLETE — no legacy behaviour
introduced**. The mission's interchange correctness is carried by the Unicode
verification in §3.

## 3. Unicode correctness (fully verifiable)

| Requirement | Result | Evidence |
|---|---|---|
| Correct Unicode scalar for virama | U+09CD, UTF-8 `E0 A7 8D` | UNICODE_VERIFICATION.csv UNI-001, UNI-UTF8-01 |
| Correct Unicode scalar for nukta | U+09BC, UTF-8 `E0 A6 BC` | UNI-002, UNI-UTF8-02 |
| Exact sequence ক্ | U+0995 U+09CD (committed, NFC-stable) | UNI-003; regression harness INV-265 + 26 khipro cases |
| Exact sequence ক় | U+0995 U+09BC | UNI-004; FP-160 |
| Standalone ় | U+09BC | UNI-005; FP-159 |
| ড় / ঢ় / য় composition exclusions | U+09A1 U+09BC / U+09A2 U+09BC / U+09AF U+09BC — decomposed forms, never precomposed look-alikes | UNI-006..008 |
| No character dropping | committed text byte-equal to expected in all 579 rows | KHIPRO_PROBHAT_REGRESSION_RESULTS.csv |
| No duplicate mark | `kqqqq` → single থে (idempotent); repeated nukta concatenates only when physically repeated | FP-164, kqqqq cases |
| No accidental reordering | the only reorder is the standard-mandated canonical reordering (nukta ccc=7 before virama ccc=9) — documented, not accidental | UNI-010, FP-166 |
| NFC consistency | commit output == normalize_nfc(output) for every case | harness NFC-parity checks; gtest session-commit-parity tests |
| Copy/paste compatibility | committed strings are plain NFC-stable UTF-8; no private-use or surrogate content | UNICODE_VERIFICATION.csv |

## 4. Remaining gaps (honest register)

1. **Clause-level BDS verification BLOCKED**: the full texts of BDS 1520:2018,
   BDS 1738:2018 and BDS 1935:2018 are copyright-protected BSTI publications
   and are not included in this repository (also per the mission's own rule
   against redistributing the standard). All BDS statements above rest on the
   standards' official identities, scope descriptions and publicly documented
   technical alignment with ISO/IEC 10646 / Unicode.
2. **Windows runtime verification BLOCKED**: no Windows 10 x64 + MSVC v143
   environment exists in this sandbox. All engine-level PASS values are engine
   evidence only; no "Windows Verified" claim is made anywhere. Re-verify on
   Windows with `scripts/windows-ui-audit/Run-BanglaTypingTests.ps1` and the
   `suborno.windows.*` ctest targets.
3. **Known open notes carried forward** (unchanged by this mission): Borno and
   Avro phonetic layouts still have no standalone-nukta token (minor, outside
   this mission's protected scope); Probhat dead-key composition remains
   literal (documented in probhat-keymap.md).

## 5. Citation register

- BDS 1520:2018 — Specification for Bangla Coded Character Set for Information
  Interchange. Bangladesh Standards and Testing Institution (BSTI), Dhaka.
- BDS 1738:2018 — Specification for Computer Bangla Keyboard (First Revision).
  BSTI, Dhaka.
- BDS 1935:2018 — Bangladesh Standard Code for Information Interchange (BSCII).
  BSTI, Dhaka.
- Public technical references consulted: BSTI Standards Catalogue 2018;
  Bangladesh Computer Council standards pages (bcc.gov.bd); "Bangla computing:
  follies all the way" (2021) on BDS 1520's ISO/IEC 10646 alignment; Ekushey
  community review of the national layout. The Unicode Standard (Bengali block
  U+0980–U+09FF, composition exclusions U+09DC/U+09DD/U+09DF, canonical
  combining classes) was verified programmatically with ICU 76.1 and Python
  `unicodedata` during the mission.
- Upstream layout sources: Khipro Classic quickstart (khipro.khiproteam.com);
  MIT-licensed princebillygk/probhat-layout-windows layout.klc @ debc8310
  (re-verified 2026-11-24, sha256 210a62fb…).
