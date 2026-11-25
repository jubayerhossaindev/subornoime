#!/usr/bin/env python3
"""C++20-only compliance scan (engineering standard §1).

Fails when a post-C++20 language or library feature appears in project
sources. The scan is intentionally cheap and deterministic so CI can run it
on every push without a compiler.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

# Post-C++20 library surface. Anything matched in project code fails CI.
BANNED_PATTERNS = [
    (r"\bstd::expected\b", "C++23 std::expected is forbidden (use std::optional/variant or a project result type)"),
    (r"\bstd::print\b", "C++23 std::print is forbidden"),
    (r"\bstd::println\b", "C++23 std::println is forbidden"),
    (r"\bstd::flat_map\b", "C++23 std::flat_map is forbidden"),
    (r"\bstd::flat_set\b", "C++23 std::flat_set is forbidden"),
    (r"\bstd::mdspan\b", "C++23 std::mdspan is forbidden"),
    (r"\bstd::stacktrace\b", "C++23 std::stacktrace is forbidden"),
    (r"\bstd::byteswap\b", "C++23 std::byteswap is forbidden"),
    (r"\bstd::to_underlying\b", "C++23 std::to_underlying is forbidden"),
    (r"\bstd::unreachable\b", "C++23 std::unreachable is forbidden"),
    (r"\bstd::basic_string_view(?=.*construct_at)", "C++23-only view constructor pattern"),
    (r"\bimport\s+std\s*;", "C++23 module import of std is forbidden"),
]

SCAN_ROOTS = ["src", "include", "tests", "benchmarks", "examples"]
SCAN_SUFFIXES = {".cpp", ".hpp", ".h", ".cc", ".cxx", ".inc"}
# Only post-C++20 standard settings are forbidden; CMAKE_CXX_STANDARD 20 is the policy itself.
COMPILE_LINE = re.compile(r"cxx_std_(23|26)|/std:c\+\+(23|26)latest|-std=c\+\+(23|26)\b|CMAKE_CXX_STANDARD\s+(23|26)")


def scan_file(path: Path) -> list[str]:
    violations: list[str] = []
    text = path.read_text(errors="replace")
    for line_number, line in enumerate(text.splitlines(), start=1):
        for pattern, message in BANNED_PATTERNS:
            if re.search(pattern, line):
                violations.append(f"{path}:{line_number}: {message}: {line.strip()[:120]}")
        if path.suffix == ".txt" or path.name == "CMakeLists.txt":
            if COMPILE_LINE.search(line):
                violations.append(f"{path}:{line_number}: post-C++20 standard setting forbidden: {line.strip()[:120]}")
    return violations


def main() -> int:
    root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path.cwd()
    violations: list[str] = []
    for scan_root in SCAN_ROOTS:
        directory = root / scan_root
        if not directory.exists():
            continue
        for path in directory.rglob("*"):
            if path.suffix in SCAN_SUFFIXES and "build" not in path.parts:
                violations.extend(scan_file(path))
    for cmake_file in [root / "CMakeLists.txt", *(root / "cmake" / "modules").glob("*.cmake")]:
        if cmake_file.exists():
            violations.extend(scan_file(cmake_file))
    for violation in violations:
        print(violation)
    if violations:
        print(f"C++20 compliance scan: {len(violations)} violation(s)")
        return 1
    print("C++20 compliance scan: OK (no post-C++20 features)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
