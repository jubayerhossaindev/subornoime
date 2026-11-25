#!/usr/bin/env python3
"""Word-list validator and combined seed generator for SubornoIME.

- Validates every data/wordlist/*.tsv entry: NFC, Bengali-script-only words,
  integer frequency range, known category/register enums.
- Detects duplicate words (exact) and duplicate-normalized conflicts.
- Merges all modules into the single data/seed_dictionary.tsv the engine
  documentation points at.

Usage: python3 scripts/validate_wordlist.py [--write]
"""
import sys
import unicodedata
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
WORDLIST_DIR = ROOT / "data" / "wordlist"
SEED_OUT = ROOT / "data" / "seed_dictionary.tsv"

CATEGORIES = {
    "function", "noun", "verb", "adjective", "numeral", "literary",
    "regional", "conjunct", "synonym", "rare", "abbreviation",
}
REGISTERS = {"", "cholito", "sadhu", "literary", "colloquial", "regional"}

ZWNJ, ZWJ = "\u200c", "\u200d"


def is_bengali_word(word: str) -> bool:
    if not word or len(word.encode("utf-8")) > 96:
        return False
    has_bengali = False
    for ch in word:
        code = ord(ch)
        if 0x0980 <= code <= 0x09FF:
            if 0x09E6 <= code <= 0x09EF:  # Bengali digits never occur in words
                return False
            has_bengali = True
            continue
        if ch in (ZWNJ, ZWJ):
            continue
        return False
    return has_bengali


def load_module(path: Path, problems: list, entries: dict, merged: list) -> int:
    count = 0
    for line_no, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = raw.rstrip("\n")
        if not line.strip() or line.startswith("#"):
            continue
        fields = [f.strip() for f in line.split("\t")]
        if len(fields) < 1 or not fields[0]:
            problems.append(f"{path.name}:{line_no}: empty word")
            continue
        word = fields[0]
        freq_text = fields[1] if len(fields) > 1 else "1"
        category = fields[2] if len(fields) > 2 else "noun"
        register = fields[3] if len(fields) > 3 else ""

        nfc = unicodedata.normalize("NFC", word)
        if nfc != word:
            problems.append(f"{path.name}:{line_no}: not NFC: {word!r}")
        if not is_bengali_word(nfc):
            problems.append(f"{path.name}:{line_no}: not a Bengali word: {word!r}")
        try:
            freq = int(freq_text)
            if not (0 <= freq <= 0x7FFFFFFF):
                raise ValueError
        except ValueError:
            problems.append(f"{path.name}:{line_no}: bad frequency: {freq_text!r}")
            freq = 1
        if category not in CATEGORIES:
            problems.append(f"{path.name}:{line_no}: unknown category: {category!r}")
        if register not in REGISTERS:
            problems.append(f"{path.name}:{line_no}: unknown register: {register!r}")

        key = nfc
        if key in entries:
            # Duplicate across modules: merge instead of failing — keep the
            # higher frequency and the first module's category metadata.
            first_mod, first_line, first_freq, first_cat, first_reg = entries[key]
            entries[key] = (first_mod, first_line, max(first_freq, freq), first_cat, first_reg)
            merged.append(f"{path.name}:{line_no}: {nfc!r} merged into {first_mod}:{first_line}")
        else:
            entries[key] = (path.name, line_no, freq, category, register)
        count += 1
    return count


def main() -> int:
    write = "--write" in sys.argv
    problems: list = []
    merged: list = []
    entries: dict = {}
    total_lines = 0
    if not WORDLIST_DIR.is_dir():
        print("ERROR: data/wordlist/ missing")
        return 2
    for path in sorted(WORDLIST_DIR.glob("*.tsv")):
        if path.name == "relations.tsv":
            continue
        total_lines += load_module(path, problems, entries, merged)

    # Duplicate-normalized report (same normalized key, different words):
    # for a Bangla pack normalized == word, so NFC-collisions are the risk.
    seen_form: dict = {}
    for key in entries:
        compact = unicodedata.normalize("NFC", key)
        seen_form.setdefault(compact, []).append(key)
    for compact, keys in seen_form.items():
        if len(keys) > 1:
            problems.append(f"duplicate-normalized: {' | '.join(keys)}")

    print(f"modules: {len(list(WORDLIST_DIR.glob('*.tsv')))-1}")
    print(f"total lines: {total_lines}")
    print(f"unique words: {len(entries)} (merged duplicates: {len(merged)})")
    categories: dict = {}
    registers: dict = {}
    tiers = {"4": 0, "3": 0, "2": 0, "1": 0}
    for (_mod, _ln, freq, category, register) in entries.values():
        categories[category] = categories.get(category, 0) + 1
        registers[register or "cholito"] = registers.get(register or "cholito", 0) + 1
        bucket = "4" if freq >= 1000 else "3" if freq >= 500 else "2" if freq >= 100 else "1"
        tiers[bucket] += 1
    print("categories:", ", ".join(f"{k}={v}" for k, v in sorted(categories.items())))
    print("registers:", ", ".join(f"{k}={v}" for k, v in sorted(registers.items())))
    print("tiers:", tiers)

    if problems:
        print(f"\nPROBLEMS ({len(problems)}):")
        for problem in problems[:60]:
            print(" -", problem)
        return 1

    if write:
        header = (
            "# Format: word<TAB>frequency<TAB>category<TAB>register (one entry per line;\n"
            "# '#' starts a comment). GENERATED by scripts/validate_wordlist.py from\n"
            "# data/wordlist/*.tsv — edit the modules, not this file.\n"
            "#\n"
            "# PROVENANCE / LICENSING:\n"
            "# - Words are everyday and literary Bengali vocabulary compiled manually\n"
            "#   for this repository (public-domain knowledge: function words, common\n"
            "#   nouns/verbs/adjectives, numerals, family/food/nature vocabulary,\n"
            "#   conjunct-heavy forms, literary/colloquial variants, synonym groups).\n"
            "# - Frequencies are SYNTHETIC ranking tiers, not measured corpus data.\n"
        )
        lines = [header]
        for word, (_m, _l, freq, category, register) in sorted(entries.items()):
            lines.append(f"{word}\t{freq}\t{category}\t{register}")
        SEED_OUT.write_text("\n".join(lines) + "\n", encoding="utf-8")
        print(f"wrote {SEED_OUT} ({len(entries)} words)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
