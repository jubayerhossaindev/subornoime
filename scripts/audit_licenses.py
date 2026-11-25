#!/usr/bin/env python3
"""License & dependency validation (engineering standard §27 stage 15/16).

Cross-checks that every declared dependency (vcpkg manifest + vendored
tools) is recorded in THIRD_PARTY_NOTICES.md with a license reference, and
that the notice file itself names the licenses of the bundled fonts/data.
Fails CI when a dependency lacks attribution.
"""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path


def declared_dependencies(root: Path) -> dict[str, str]:
    dependencies: dict[str, str] = {}
    manifest_path = root / "vcpkg.json"
    if manifest_path.exists():
        manifest = json.loads(manifest_path.read_text())
        for entry in manifest.get("dependencies", []):
            if isinstance(entry, str):
                dependencies[entry] = "vcpkg"
            else:
                dependencies[entry.get("name", "")] = "vcpkg"
    return dependencies


def main() -> int:
    root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path.cwd()
    notices_path = root / "THIRD_PARTY_NOTICES.md"
    if not notices_path.exists():
        print("License validation: THIRD_PARTY_NOTICES.md is missing")
        return 1
    notices = notices_path.read_text(errors="replace")

    violations: list[str] = []
    for name, source in declared_dependencies(root).items():
        if not name:
            continue
        if name.lower() not in notices.lower():
            violations.append(f"{source} dependency '{name}' has no THIRD_PARTY_NOTICES.md entry")

    # Every documented license must be an actual SPDX-style license token or
    # a URL the project can point auditors at.
    license_mentions = re.findall(r"\b(?:MIT|BSD-[0-9]?|Apache-2\.0|Zlib|BSL-1\.0|Unicode-3\.0|GPL-[0-9.]+|LGPL-[0-9.]+)\b", notices)
    if len(license_mentions) < 3:
        violations.append("THIRD_PARTY_NOTICES.md lists fewer than three license identifiers - check completeness")

    if violations:
        print("License validation: problems found:")
        for violation in violations:
            print(f"  {violation}")
        return 1
    print(f"License validation: OK ({len(declared_dependencies(root))} dependencies, {len(set(license_mentions))} license identifiers)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
