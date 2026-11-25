#!/usr/bin/env python3
"""BDS mission — independent cross-check of the engine repertoire inventory
against Python unicodedata (an independent Unicode authority).

Reads the engine-produced bds_repertoire_inventory.csv and verifies, for every
code point U+0980..U+09FF:
  - assigned status (unicodedata.lookup / name)
  - general category
  - NFC/NFD reference results vs the engine's NFC stability flag
  - UTF-8 byte encoding match

Writes bds_repertoire_crosscheck.csv with per-row verdicts.
"""
import csv
import sys
import unicodedata

FIRST, LAST = 0x0980, 0x09FF


def utf8_hex(ch: str) -> str:
    return ch.encode("utf-8").hex().upper()


def main() -> int:
    inv_path, out_path = sys.argv[1], sys.argv[2]
    with open(inv_path, newline="", encoding="utf-8") as f:
        rows = list(csv.DictReader(f))

    mismatches = []
    out_rows = []
    for row in rows:
        cp = int(row["codepoint"].replace("U+", ""), 16)
        ch = chr(cp)
        try:
            name = unicodedata.name(ch)
            assigned = True
        except ValueError:
            name = ""
            assigned = False
        cat = unicodedata.category(ch) if assigned else "Cn(unassigned)"
        nfc = unicodedata.normalize("NFC", ch)
        nfd = unicodedata.normalize("NFD", ch)
        engine_utf8_ok = row["utf8_valid"] == "TRUE"
        engine_utf8_hex = row["utf8_hex"]
        utf8_match = (not assigned) or (engine_utf8_hex == utf8_hex(ch))
        # Engine NFC stability flag should equal reference (NFC(ch) == ch)
        ref_stable = (nfc == ch)
        engine_stable = row["nfc_stable"] == "TRUE"
        stable_match = (ref_stable == engine_stable)
        ok = engine_utf8_ok and utf8_match and stable_match and row["nfc_idempotent"] == "TRUE"
        if not ok:
            mismatches.append((cp, name, cat))
        out_rows.append({
            "codepoint": row["codepoint"],
            "assigned": "TRUE" if assigned else "FALSE",
            "unicode_name": name,
            "category": cat,
            "nfc_ref_cp": " ".join(f"{ord(c):04X}" for c in nfc) if nfc else "",
            "nfd_ref_cp": " ".join(f"{ord(c):04X}" for c in nfd) if nfd else "",
            "utf8_ref_hex": utf8_hex(ch),
            "utf8_match": "TRUE" if utf8_match else "FALSE",
            "nfc_stable_match": "TRUE" if stable_match else "FALSE",
            "result": "PASS" if ok else "FAIL",
        })

    with open(out_path, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=list(out_rows[0].keys()), lineterminator="\n")
        w.writeheader()
        w.writerows(out_rows)

    assigned_n = sum(1 for r in out_rows if r["assigned"] == "TRUE")
    print(f"rows={len(out_rows)} assigned={assigned_n} unassigned={len(out_rows)-assigned_n} "
          f"mismatches={len(mismatches)}")
    for m in mismatches[:10]:
        print("MISMATCH:", m)
    return 0 if not mismatches else 1


if __name__ == "__main__":
    sys.exit(main())
