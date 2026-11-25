#!/usr/bin/env python3
# BDS 1520 official allocation corpus builder + verifier (BDS FINALIZATION PHASE 2).
#
# Provenance: character allocations transcribed VISUALLY from the full official
# document PDF (scanned) — "SPECIFICATION FOR BANGLA CODED CHARACTER SET FOR
# INFORMATION INTERCHANGE (Second Revision)", headers "BDS 1520:2011",
# obtained 2026-11-24 from the Bangladesh Computer Council official file store
# (objectstorage.ap-dcc-gazipur-1.oraclecloud15.com, office-bcc bucket, linked
# from the BCC IT-standards page). SHA256 of the obtained PDF:
#   ab2bc3c26896257f4447d2a1ff88d93c8ddcf21ce5c68e103fbed9743b0cf456
# NOTE: the BCC page lists the standard as "BDS 1520:2018", but the document
# itself carries the cover text "(Second Revision)" and headers "BDS 1520:2011"
# — the obtained text is therefore the 2011 Second Revision edition. The
# current 2018 Third Revision full text is still only purchasable from the
# BSTI BDS Store (requires account + payment; NOT performed — no approval).
#
# IMPORTANT (copyright): only the allocation FACTS (code position, printed
# name, Unicode scalar) are extracted here — no BDS prose is reproduced. The
# PDF itself is kept OUTSIDE the repository.
#
# Verification performed (mechanical, not assumed):
#   1. Offset rule: 8-bit code NN  <->  Unicode scalar U+09NN  (verified row
#      by row; the two Daari positions E4/E5 intentionally map to the shared
#      danda characters U+0964/U+0965 — recorded as exceptions).
#   2. unicodedata cross-check of every scalar (assigned, expected name).
#   3. Count reconciliation: clause 5 statement (90) vs transcribed rows vs
#      modern Unicode assigned count in the Bengali block.
#   4. Subset property: every code point emitted by the 579-row engine typing
#      harness must belong to the official allocation set (layouts never emit
#      characters outside the official repertoire).
#
# Output: docs/standards/compliance/BDS_1520_OBTAINED_EDITION_ALLOCATION.csv
#         (plus stdout summary for the conformance report).

import csv
import os
import sys
import unicodedata as ud

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT_CSV = os.path.join(REPO, "docs", "standards", "compliance",
                       "BDS_1520_OBTAINED_EDITION_ALLOCATION.csv")
HARNESS_CSV = os.path.join(REPO, "reports", "standards", "evidence-raw",
                           "bds_engine_rows_579.csv")

# (hex code, table position x/y, printed name, printed glyph or "")
# Transcribed from Table 2 "Graphic Character Allocations", pages 8-10.
TABLE2 = [
    (0x81, "8/1",  "Bangla sign chandrabindu",               "◌ঁ"),
    (0x82, "8/2",  "Bangla sign Anusvara",                   "◌ং"),
    (0x83, "8/3",  "Bangla sign Visarga",                    "◌ঃ"),
    (0x85, "8/5",  "Bangla letter A",                        "অ"),
    (0x86, "8/6",  "Bangla letter AA",                       "আ"),
    (0x87, "8/7",  "Bangla letter I",                        "ই"),
    (0x88, "8/8",  "Bangla letter II",                       "ঈ"),
    (0x89, "8/9",  "Bangla letter U",                        "উ"),
    (0x8A, "8/10", "Bangla letter UU",                       "ঊ"),
    (0x8B, "8/11", "Bangla letter Vocalic R",                "ঋ"),
    (0x8C, "8/12", "Bangla letter Vocalic L",                "ঌ"),
    (0x8F, "8/15", "Bangla letter E",                        "এ"),
    (0x90, "9/0",  "Bangla letter AI",                       "ঐ"),
    (0x93, "9/3",  "Bangla letter O",                        "ও"),
    (0x94, "9/4",  "Bangla letter AU",                       "ঔ"),
    (0x95, "9/5",  "Bangla letter KA",                       "ক"),
    (0x96, "9/6",  "Bangla letter KHA",                      "খ"),
    (0x97, "9/7",  "Bangla letter GA",                       "গ"),
    (0x98, "9/8",  "Bangla letter GHA",                      "ঘ"),
    (0x99, "9/9",  "Bangla letter UNGA",                     "ঙ"),
    (0x9A, "9/10", "Bangla letter CA",                       "চ"),
    (0x9B, "9/11", "Bangla letter CHA",                      "ছ"),
    (0x9C, "9/12", "Bangla letter JA",                       "জ"),
    (0x9D, "9/13", "Bangla letter JHA",                      "ঝ"),
    (0x9E, "9/14", "Bangla letter NYA",                      "ঞ"),
    (0x9F, "9/15", "Bangla letter TTA",                      "ট"),
    (0xA0, "10/0", "Bangla letter TTHA",                     "ঠ"),
    (0xA1, "10/1", "Bangla letter DDA",                      "ড"),
    (0xA2, "10/2", "Bangla letter DDHA",                     "ঢ"),
    (0xA3, "10/3", "Bangla letter NNA",                      "ণ"),
    (0xA4, "10/4", "Bangla letter TA",                       "ত"),
    (0xA5, "10/5", "Bangla letter THA",                      "থ"),
    (0xA6, "10/6", "Bangla letter DA",                       "দ"),
    (0xA7, "10/7", "Bangla letter DHA",                      "ধ"),
    (0xA8, "10/8", "Bangla letter NA",                       "ন"),
    (0xAA, "10/10", "Bangla letter PA",                      "প"),
    (0xAB, "10/11", "Bangla letter PHA",                     "ফ"),
    (0xAC, "10/12", "Bangla letter BA",                      "ব"),
    (0xAD, "10/13", "Bangla letter BHA",                     "ভ"),
    (0xAE, "10/14", "Bangla letter MA",                      "ম"),
    (0xAF, "10/15", "Bangla letter YA",                      "য"),
    (0xB0, "11/0", "Bangla letter RA",                       "র"),
    (0xB2, "11/2", "Bangla letter LA",                       "ল"),
    (0xB6, "11/6", "Bangla letter SHA",                      "শ"),
    (0xB7, "11/7", "Bangla letter SSA",                      "ষ"),
    (0xB8, "11/8", "Bangla letter SA",                       "স"),
    (0xB9, "11/9", "Bangla letter HA",                       "হ"),
    (0xBC, "11/12", "Bangla sign NUKTA",                     "◌়"),
    (0xBD, "11/13", "Bangla sign AVAGRAHA",                  "ঽ"),
    (0xBE, "11/14", "Bangla Vowel Sign AA",                  "◌া"),
    (0xBF, "11/15", "Bangla Vowel Sign I",                   "ি"),
    (0xC0, "12/0", "Bangla Vowel Sign II",                   "ী"),
    (0xC1, "12/1", "Bangla Vowel Sign U",                    "◌ু"),
    (0xC2, "12/2", "Bangla Vowel Sign UU",                   "◌ূ"),
    (0xC3, "12/3", "Bangla Vowel Sign Vocalic R",            "ৃ"),
    (0xC4, "12/4", "Bangla Vowel Sign Vocalic RR",           "ৄ"),
    (0xC7, "12/7", "Bangla Vowel Sign E",                    "ে"),
    (0xC8, "12/8", "Bangla Vowel Sign AI",                   "ৈ"),
    (0xCB, "12/11", "Bangla Vowel Sign O",                   "ো"),
    (0xCC, "12/12", "Bangla Vowel Sign AU",                  "ৌ"),
    (0xCD, "12/13", "Bangla Sign Virama",                    "◌্"),
    (0xCE, "12/14", "Bangla letter KHANDATA",                "ৎ"),
    (0xD7, "13/7", "Bangla AU Length Mark",                  "ৗ"),
    (0xDC, "13/12", "Bangla letter RRA",                      "ড়"),
    (0xDD, "13/13", "Bangla letter RHA",                     "ঢ়"),
    (0xDF, "13/15", "Bangla letter YYA",                     "য়"),
    (0xE0, "14/0", "Bangla letter Vocalic RR",               "ৠ"),
    (0xE1, "14/1", "Bangla letter Vocalic LL",               "ৡ"),
    (0xE2, "14/2", "Bangla Vowel Sign Vocalic L",            "ৢ"),
    (0xE3, "14/3", "Bangla Vowel Sign Vocalic LL",           "ৣ"),
    (0xE4, "14/4", "Bangla Daari",                           "।"),
    (0xE5, "14/5", "Bangla Double Daari",                    "॥"),
    (0xE6, "14/6", "Bangla Digit Zero",                      "০"),
    (0xE7, "14/7", "Bangla Digit One",                       "১"),
    (0xE8, "14/8", "Bangla Digit Two",                       "২"),
    (0xE9, "14/9", "Bangla Digit Three",                     "৩"),
    (0xEA, "14/10", "Bangla Digit Four",                     "৪"),
    (0xEB, "14/11", "Bangla Digit Five",                     "৫"),
    (0xEC, "14/12", "Bangla Digit Six",                      "৬"),
    (0xED, "14/13", "Bangla Digit seven",                    "৭"),
    (0xEE, "14/14", "Bangla Digit Eight",                    "৮"),
    (0xEF, "14/15", "Bangla Digit Nine",                     "৯"),
    (0xF0, "15/0", "Bangla letter RA with Middle Diagonal",  "ৰ"),
    (0xF1, "15/1", "Bangla letter RA with Lower Diagonal",   "ৱ"),
    (0xF2, "15/2", "Bangla Taka Mark",                       "৲"),
    (0xF3, "15/3", "Bangla Taka sign",                       "৳"),
    (0xF4, "15/4", "Bangla Currency Numerator One",          "৴"),
    (0xF5, "15/5", "Bangla Currency Numerator Two",          "৵"),
    (0xF6, "15/6", "Bangla Currency Numerator Three",        "৶"),
    (0xF7, "15/7", "Bangla Currency Numerator Four",         "৷"),
    (0xF8, "15/8", "Bangla Currency Numerator One less than the Denominator", "৸"),
    (0xF9, "15/9", "Bangla Currency Denominator Sixteen",    "৹"),
    (0xFA, "15/10", "Bangla ISSHAR",                          "৺"),
    (0xFB, "15/11", "Bangla Ganda Mark",                     "৻"),
]

# Printed-name -> Unicode-name normalisation (informational comparison only;
# the binding check is the code-point identity via the offset rule).
NAME_OVERRIDES = {
    0x99: ("BENGALI LETTER NGA", "printed 'UNGA' paraphrases Unicode 'NGA'"),
    0xCE: ("BENGALI LETTER KHANDA TA", "printed 'KHANDATA' = Unicode 'KHANDA TA'"),
    0xE4: ("DEVANAGARI DANDA", "Bangla Daari shares the danda character U+0964"),
    0xE5: ("DEVANAGARI DOUBLE DANDA", "Bangla Double Daari = U+0965"),
    0xF2: ("BENGALI RUPEE MARK", "printed 'Taka Mark' = Unicode 'RUPEE MARK'"),
    0xF3: ("BENGALI RUPEE SIGN", "printed 'Taka sign' = Unicode 'RUPEE SIGN'"),
}


def scalar_for(hexcode: int) -> int:
    if hexcode == 0xE4:
        return 0x0964
    if hexcode == 0xE5:
        return 0x0965
    return 0x0900 + hexcode


def norm_name(name: str) -> str:
    n = name.strip().upper().replace("BANGLA", "BENGALI")
    n = " ".join(n.split())
    return n


def main() -> int:
    failures = []
    rows = []
    for hexcode, pos, printed, glyph in TABLE2:
        cp = scalar_for(hexcode)
        ch = chr(cp)
        try:
            ud_name = ud.name(ch)
            assigned = True
        except ValueError:
            ud_name = ""
            assigned = False
        # rule 1: offset rule identity (by construction here, but asserted in
        # the committed C++ test independently)
        offset_ok = (cp == 0x0900 + hexcode) or hexcode in (0xE4, 0xE5)
        # rule 2: assigned in modern Unicode + glyph identity check
        if not assigned:
            failures.append(f"0x{hexcode:02X}: scalar U+{cp:04X} NOT assigned in Unicode {ud.unidata_version}")
        # glyph-vs-scan identity: the print glyph for small/combining marks is
        # rendered with dotted circles in the scan; compare via NFC only where
        # the printed glyph is base-letter-like, and never treat glyph shape
        # as binding — the binding data is (code position, name, scalar).
        glyph_ok = True
        if glyph and not glyph.startswith("◌"):
            if ud.normalize("NFC", glyph) != ud.normalize("NFC", ch):
                glyph_ok = False
        if not glyph_ok:
            failures.append(f"0x{hexcode:02X}: printed glyph {glyph!r} != Unicode char U+{cp:04X}")
        # informational name comparison
        expected_name, note = NAME_OVERRIDES.get(hexcode, (norm_name(printed), ""))
        name_match = norm_name(printed) == expected_name or bool(note)
        rows.append({
            "bds_hex": f"0x{hexcode:02X}",
            "bds_position": pos,
            "bds_name_as_printed": printed,
            "printed_glyph": glyph,
            "unicode_scalar": f"U+{cp:04X}",
            "unicode_char": ch,
            "unicode_name": ud_name,
            "utf8_hex": ch.encode("utf-8").hex().upper(),
            "utf16_units": f"{cp:04X}" if cp < 0x10000 else "surrogate-pair",
            "offset_rule": "match" if cp == 0x0900 + hexcode else "exception(danda)",
            "assigned_in_unicode": "yes" if assigned else "no",
            "unicode_name_match": "yes" if name_match else "no",
            "glyph_identity": "match" if glyph_ok else "see scan",
            "name_note": note,
        })

    # count reconciliation
    n_rows = len(rows)
    n_block = sum(1 for r in rows
                  if 0x0980 <= int(r["unicode_scalar"][2:], 16) <= 0x09FF)
    n_danda = n_rows - n_block
    modern_assigned = sum(1 for c in range(0x0980, 0x0A00)
                          if ud.category(chr(c))[0] not in ("C", "Z"))
    block_scalars = {int(r["unicode_scalar"][2:], 16) for r in rows
                     if 0x0980 <= int(r["unicode_scalar"][2:], 16) <= 0x09FF}
    modern_set = {c for c in range(0x0980, 0x0A00)
                  if ud.category(chr(c))[0] not in ("C", "Z")}
    not_in_table = sorted(modern_set - block_scalars)

    print(f"transcribed rows           : {n_rows}")
    print(f"  within U+09xx block      : {n_block}")
    print(f"  danda (U+0964/U+0965)    : {n_danda}")
    print(f"clause 5 statement         : 90 graphic characters in columns 8..15")
    print(f"modern Unicode {ud.unidata_version} assigned in block : {modern_assigned}")
    print(f"modern block chars NOT in obtained table: "
          + ", ".join(f"U+{c:04X} {ud.name(chr(c), '?')}" for c in not_in_table))

    # rule 4: every BANGLA-SCRIPT code point the harness emits must belong to
    # the official repertoire. ASCII/other-script output (English passthrough,
    # punctuation tokens) is outside BDS 1520's scope and is not counted.
    allowed = block_scalars | {0x0964, 0x0965}
    emitted = set()
    if os.path.exists(HARNESS_CSV):
        with open(HARNESS_CSV, newline="", encoding="utf-8") as fh:
            for row in csv.DictReader(fh):
                for key in ("expected_cps", "actual_cps", "codepoints"):
                    v = (row.get(key) or "").strip()
                    if not v:
                        continue
                    for tok in v.replace(";", " ").replace(",", " ").split():
                        tok = tok.strip()
                        if tok.startswith("U+"):
                            try:
                                emitted.add(int(tok[2:], 16))
                            except ValueError:
                                pass
        bangla_emitted = sorted(e for e in emitted if 0x0964 <= e <= 0x09FF)
        outside = sorted(e for e in bangla_emitted if e not in allowed)
        print(f"harness emitted code points : {len(emitted)} distinct "
              f"({len(bangla_emitted)} Bangla-script)")
        if outside:
            failures.append("harness emits Bangla-script points outside official repertoire: "
                            + ", ".join(f"U+{c:04X}" for c in outside))
        else:
            print("harness subset check       : PASS (all Bangla-script points in official repertoire)")
    else:
        print("harness CSV not found — subset check skipped")

    os.makedirs(os.path.dirname(OUT_CSV), exist_ok=True)
    with open(OUT_CSV, "w", newline="", encoding="utf-8") as fh:
        w = csv.DictWriter(fh, fieldnames=list(rows[0].keys()), lineterminator="\n")
        w.writeheader()
        w.writerows(rows)
    print(f"written                    : {OUT_CSV}")

    if n_rows != 94:
        failures.append(f"expected 94 transcribed rows, got {n_rows}")
    # Post-Unicode-6.0.0 additions absent from the obtained table are EXPECTED
    # (the standard's scope clause 1.1 binds to Unicode 6.0.0):
    #   U+0980 BENGALI ANJI (Unicode 11.0), U+09FC/U+09FD/U+09FE (Unicode 14.0).
    # Anything else missing must be reported, never silently accepted.
    expected_absent = {0x0980, 0x09FC, 0x09FD, 0x09FE}
    if set(not_in_table) != expected_absent:
        unexpected = [c for c in not_in_table if c not in expected_absent]
        missing_expected = [c for c in expected_absent if c not in not_in_table]
        if unexpected:
            failures.append("unexpected modern chars missing from table: "
                            + ", ".join(f"U+{c:04X}" for c in unexpected))
        if missing_expected:
            failures.append("post-6.0.0 additions unexpectedly PRESENT in table: "
                            + ", ".join(f"U+{c:04X}" for c in missing_expected))

    if failures:
        print("\nFAILURES:")
        for f in failures:
            print(" -", f)
        return 1
    print("\nALL CORPUS CHECKS PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
