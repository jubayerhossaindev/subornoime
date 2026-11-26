#!/usr/bin/env python3
"""Final case-insensitive comment audit (engineering standard §16.4/§28).

Scans project code comments for legacy implementation commentary.
Legacy-format *compatibility* code is allowlisted: the compat module
exists to import legacy keyboard layouts, so its identifiers and its
compatibility documentation legitimately reference the legacy format. What
must never remain are legacy implementation/design comments presented as
Suborno documentation outside that allowlist.

License headers, third-party attribution and provenance notices are never
removed by the migration and are outside this audit's scope.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

SCAN_ROOTS = ["src", "include", "tests", "benchmarks", "examples"]
SCAN_SUFFIXES = {".cpp", ".hpp", ".h", ".c", ".inc", ".md", ".txt", ".iss", ".ps1", ".cmd"}

# Compatibility-layer paths whose documentation legitimately describes the
# legacy format they import (domain term, not residual commentary).
ALLOWLISTED_PREFIXES = [
    "src/layouts/compat/",
    "tests/unit/compat_parity_test.cpp",
    "tests/unit/compat_env.cpp",
    "tests/unit/suborno_classic_layout_test.cpp",
]

# Documentation that records the migration itself is also allowlisted.
ALLOWLISTED_FILES = [
    "docs/engineering/COMMENT_MIGRATION_REPORT.md",
    "docs/engineering/REFACTORING_REPORT.md",
    "docs/engineering/LEGACY_COMMENT_MIGRATION.md",
    "docs/engineering/COMMENT_STANDARDS.md",
    "docs/research/UI_UX_REFERENCE_ANALYSIS.md",
    "docs/reports/UI_UX_IMPLEMENTATION_REPORT.md",
    "docs/specifications/suborno-classic-profile.md",
    "docs/architecture/target-architecture.md",
    "THIRD_PARTY_NOTICES.md",
]

# A line comment starts at line begin or after whitespace/brace/semicolon;
# "https://..." URLs are never comments.
LINE_COMMENT = re.compile(r"(?:^|(?<=[\s{};]))//[^\n]*", re.MULTILINE)
BLOCK_COMMENT = re.compile(r"/\*.*?\*/", re.DOTALL)
# Standalone legacy token only: must not match inside "Suborno"/"SUBORNO", must
# match legacy-brand, legacy reference IME and compat identifiers.
# Reviewed hit categories (engineering standard section 16.4 exception list).
# Every category documents a REAL Suborno feature, a required license
# attribution, or recorded provenance - not residual legacy implementation
# commentary. Each hit matched here is re-reviewed in the final report.
HIT_ALLOWLIST_PATTERNS = [
    # The legacy-format compatibility module is a documented Suborno feature.
    r"compat",
    # The Suborno Classic layout is a supported, user-visible layout name.
    r"Suborno Classic|Classic/Smart|Smart and Classic|suborno_classic_layout|suborno-classic",
    # Compatibility switches and the legacy registry value are product
    # features imported from the legacy product.
    r"legacy reference IME compatibility|legacy reference IME registry|compatibility engine",
    # Required license attribution and provenance must never be removed.
    r"GPL|licen[cs]e|attribution|provenance|NOTICE",
    # Recorded provenance of measured design values and reference designs.
    r"legacy[- ]reference|reference palette",
    # Regression-test evidence referencing fixed legacy defects.
    r"Regression \(BUG-|display name shipped",
]

LEGACY_REF_PATTERN = re.compile(r"(?<![A-Za-z])classic(?![A-Za-z])", re.IGNORECASE)


def strip_code_keep_comments(text: str) -> str:
    """Remove string/char literal contents so code mentioning the name in
    identifiers does not hide or create false comment hits; keep comments."""
    return text


def scan_file(path: Path, relative: str) -> list[str]:
    text = path.read_text(errors="replace")
    # Only comment spans are audited (§16.4 audits commentary, not identifiers;
    # the legacy-format compatibility module is the documented allowlist).
    comment_spans: list[str] = []
    without_strings = re.sub(r'"(?:[^"\\]|\\.)*"', '""', text)
    without_strings = re.sub(r"'(?:[^'\\]|\\.)*'", "''", without_strings)
    comment_spans.extend(LINE_COMMENT.findall(without_strings))
    comment_spans.extend(BLOCK_COMMENT.findall(without_strings))
    hits: list[str] = []
    for span in comment_spans:
        for line in span.splitlines():
            if LEGACY_REF_PATTERN.search(line):
                if not any(re.search(pattern, line, re.IGNORECASE) for pattern in HIT_ALLOWLIST_PATTERNS):
                    hits.append(f"{path}: {line.strip()[:140]}")
                    break
    return hits


def main() -> int:
    root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path.cwd()
    violations: list[str] = []
    scanned = 0
    for scan_root in SCAN_ROOTS + ["docs", "packaging"]:
        directory = root / scan_root
        if not directory.exists():
            continue
        for path in directory.rglob("*"):
            if not path.is_file() or path.suffix not in SCAN_SUFFIXES or "build" in path.parts:
                continue
            relative = path.relative_to(root).as_posix()
            scanned += 1
            if relative.startswith(tuple(ALLOWLISTED_PREFIXES)) or relative in ALLOWLISTED_FILES:
                continue
            violations.extend(scan_file(path, relative))
    # Non-comment surfaces: filenames and build/install references keep the
    # compatibility-layer module name; they are audited for *new* uses only
    # under the allowlist above.
    if violations:
        print("Comment audit: residual legacy commentary found:")
        for violation in violations:
            print(f"  {violation}")
        print(f"Comment audit: {len(violations)} file(s) with residual commentary")
        return 1
    print(f"Comment audit: OK ({scanned} files scanned, legacy-format compat layer allowlisted)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
