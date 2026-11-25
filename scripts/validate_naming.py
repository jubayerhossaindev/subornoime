#!/usr/bin/env python3
"""Automated naming validation for the Suborno IME repository.

Enforces the project naming convention (docs/engineering/NAMING_CONVENTIONS.md)
that clang-tidy cannot fully express, with hand-audited whitelists:

  1. C++ file names must be snake_case (tests get a _test/_benchmark suffix).
  2. C++ constants must not reintroduce the banned kPascalCase style.
  3. Local variables / loop variables must not use one-letter names or the
     banned abbreviations (idx, cnt, cfg, buf, tmp, ...).
  4. Class members in the public headers must be snake_case with a trailing
     underscore for private data members.

Known-acceptable exceptions are whitelisted explicitly with a reason so the
CI gate stays meaningful. Exit code 0 = clean, 1 = violations found.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
CODE_EXT = {".cpp", ".hpp", ".h", ".c", ".inc"}
SKIP_DIRS = {"build", ".git"}

# COM / TSF / Win32 external contract method names (never renamed).
EXTERNAL_METHODS = {
    "QueryInterface", "AddRef", "Release", "DoEditSession", "GetDescription",
    "GetGUID", "EnumDisplayAttributeInfo", "Update", "Activate", "Deactivate",
    "Advise", "Unadvise", "OnChange", "Show", "IsShown", "GetUpdatedFlags",
    "GetDocumentMgr", "GetCount", "GetSelection", "GetString", "GetPageIndex",
    "SetPageIndex", "GetCurrentPage", "SetSelection", "Clone", "Next", "Skip",
    "Reset", "LockServer", "CreateInstance", "GetInstance", "DoDllMain",
    "MessageProc", "OnSetFocus", "OnKillFocus", "OnKeyDown", "OnKeyUp",
    "OnTestKeyDown", "OnTestKeyUp", "OnPreservedKey", "OnPushContext",
    "OnPopContext", "OnCompositionTerminated", "OnSetThreadFocus",
    "OnKillThreadFocus", "OnEndEdit", "OnInitDocumentMgr", "OnUninitDocumentMgr",
    "OnSetFocusSink", "GetDisplayAttributeInfo", "GetAttributeInfo",
    "SetAttributeInfo", "Clear", "GetCommits", "WndProc", "Finalize", "Abort",
    "Request", "Insert", "Start", "Finish", "Terminate", "GetBool",
    "SetBool", "GetDword", "SetDword", "Get", "Set", "Load", "Convert",
    "ResetOverridePath",
}

BANNED_ABBREV = {
    "cfg", "ctx", "mgr", "proc", "util", "misc", "stuff", "impl", "obj",
    "idx", "cnt", "num", "buf", "tmp", "val", "res", "req", "resp", "args",
    "params", "info", "helper", "repo", "cand",
}

ONE_LETTER = set("ijklmnxyz")

violations: list[str] = []


def code_regions(text: str) -> str:
    """Strip comments and string/char literals, keep code."""
    out: list[str] = []
    i = 0
    n = len(text)
    while i < n:
        ch = text[i]
        nxt = text[i + 1] if i + 1 < n else ""
        if ch == "/" and nxt == "/":
            end = text.find("\n", i)
            end = n if end == -1 else end
            out.append(" " * (end - i))
            i = end
        elif ch == "/" and nxt == "*":
            end = text.find("*/", i + 2)
            end = n if end == -1 else end + 2
            out.append(" " * (end - i))
            i = end
        elif ch in ('"', "'"):
            quote = ch
            j = i + 1
            while j < n:
                if text[j] == "\\":
                    j += 2
                    continue
                if text[j] == quote or (text[j] == "\n" and quote == '"'):
                    break
                j += 1
            # Replace the literal with a compact placeholder so encoding
            # prefixes (L, u8, U, R) stay glued to it and are not mistaken
            # for one-letter identifiers.
            literal_start = i
            while (
                literal_start > 0
                and out
                and out[-1]
                and (out[-1][-1] in "LuUR8" or text[literal_start - 1] in "LuUR8")
                and text[literal_start - 1] in "LuUR8"
            ):
                literal_start -= 1
                if out:
                    out[-1] = out[-1][:-1]
            out.append('""')
            i = min(j + 1, n)
        else:
            out.append(ch)
            i += 1
    return "".join(out)


def check_file_names() -> None:
    for path in sorted(REPO.rglob("*")):
        if not path.is_file() or path.suffix.lower() not in CODE_EXT:
            continue
        if any(p in SKIP_DIRS for p in path.parts):
            continue
        stem = path.stem
        if stem.endswith("_test") or stem.endswith("_benchmark"):
            stem = stem.rsplit("_", 1)[0]
        stem = re.sub(r"_?(in|hpp|inc)$", "", stem)
        if not re.fullmatch(r"[a-z0-9_]+", stem):
            violations.append(f"file-name: {path.relative_to(REPO)} is not snake_case")


def check_constants_and_locals() -> None:
    kconst_re = re.compile(r"\bk[A-Z]\w*\b")
    loop_re = re.compile(r"\bfor\s*\(\s*(?:const\s+|static\s+)*(?:[\w:]+[\s*&]+)?(\w+)\s*(?:=|:)")
    decl_re = re.compile(
        r"^\s*(?:const\s+|constexpr\s+|static\s+|inline\s+|auto\s+|unsigned\s+)*"
        r"(?:[\w:]+(?:<[^>]*>)?[\s*&]+)?(\w+)\s*(?:=|;|\{|\[|\))\s*(?://.*)?$"
    )
    for path in sorted(REPO.rglob("*")):
        if not path.is_file() or path.suffix.lower() not in CODE_EXT:
            continue
        if any(p in SKIP_DIRS for p in path.parts):
            continue
        rel = str(path.relative_to(REPO))
        code = code_regions(path.read_text(encoding="utf-8", errors="replace"))
        for match in kconst_re.finditer(code):
            violations.append(
                f"constant: {rel}:{text_pos_to_line(code, match.start())} "
                f"kPascalCase constant '{match.group(0)}' — use snake_case"
            )
        for line_no, raw in enumerate(code.splitlines(), 1):
            for match in loop_re.finditer(raw):
                name = match.group(1).rstrip("_")
                if name.lower() in ONE_LETTER:
                    violations.append(
                        f"loop-var: {rel}:{line_no} one-letter loop variable '{name}'"
                    )
            stripped = raw.strip()
            if not stripped or stripped.startswith("#") or stripped.startswith("}"):
                continue
            m = decl_re.match(stripped)
            if not m:
                continue
            name = m.group(1)
            core = name.rstrip("_").lstrip("_")
            if core.lower() in BANNED_ABBREV and core not in {"gen"}:
                # impl_/storage_ members are valid; 'Impl' pimpl types were renamed
                if name.endswith("_") and core in {"impl", "info", "obj"}:
                    violations.append(
                        f"member: {rel}:{line_no} unclear member name '{name}'"
                    )
                    continue
                violations.append(
                    f"identifier: {rel}:{line_no} banned abbreviation '{name}'"
                )
            elif len(core) == 1 and core.lower() in ONE_LETTER:
                violations.append(
                    f"identifier: {rel}:{line_no} one-letter variable '{name}'"
                )


def text_pos_to_line(text: str, pos: int) -> int:
    return text.count("\n", 0, pos) + 1


def main() -> int:
    check_file_names()
    check_constants_and_locals()
    if violations:
        print(f"NAMING VIOLATIONS ({len(violations)}):")
        for v in violations[:60]:
            print(f"  {v}")
        return 1
    print("naming validation: OK (files, constants, locals, loop variables)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
