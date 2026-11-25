#!/usr/bin/env python3
"""Brace/paren/bracket balance scan for Windows-only sources.

The Windows presentation files cannot compile in this sandbox (the brief
forbids substituting the MSVC toolset), so this scan is the static check
that a large edit did not leave an unbalanced delimiter. It is literal- and
comment-aware: // and /* */ comments, "wide/narrow" string literals and
char literals are skipped before counting delimiters.

Usage: python3 scripts/brace_scan.py FILE...
Exit code 0 = all balanced.
"""

from __future__ import annotations

import sys


def scan(text: str) -> tuple[bool, str | None]:
    pairs = {"}": "{", ")": "(", "]": "["}
    stack: list[tuple[str, int]] = []
    index = 0
    length = len(text)
    while index < length:
        character = text[index]
        if character == "/" and index + 1 < length and text[index + 1] == "/":
            index = text.find("\n", index)
            index = length if index < 0 else index
            continue
        if character == "/" and index + 1 < length and text[index + 1] == "*":
            index = text.find("*/", index + 2)
            if index < 0:
                return False, "unterminated /* comment"
            index += 2
            continue
        if character in ('"', "'"):
            quote = character
            index += 1
            while index < length:
                if text[index] == "\\":
                    index += 2
                    continue
                if text[index] == quote:
                    break
                index += 1
            if index >= length:
                return False, f"unterminated {quote} literal"
            index += 1
            continue
        if character in "{([":
            stack.append((character, index))
        elif character in "})]":
            if not stack:
                return False, f"unmatched {character!r} at offset {index}"
            opened, _ = stack.pop()
            if opened != pairs[character]:
                return False, f"mismatched {character!r} at offset {index}"
        index += 1
    if stack:
        opened, offset = stack[-1]
        return False, f"unclosed {opened!r} at offset {offset}"
    return True, None


def main(argv: list[str]) -> int:
    failures = 0
    for path in argv:
        with open(path, "r", encoding="utf-8") as handle:
            ok, reason = scan(handle.read())
        if ok:
            print(f"brace-scan: OK   {path}")
        else:
            print(f"brace-scan: FAIL {path}: {reason}")
            failures += 1
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
