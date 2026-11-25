#!/usr/bin/env python3
"""Locale integrity check for the Suborno UI strings (evidence script).

Verifies, for data/i18n/*/ui.json:
  1. key parity across locales (every key in every locale);
  2. {placeholder} integrity (same placeholder set per key in all locales);
  3. code coverage: every `ui::text("key", ...)` / `ui_strings::get("key")`
     literal in src/ + include/ resolves to a shipped key, and every shipped
     key is either referenced literally, referenced through the documented
     dynamic paths (toast.* via show_toast/format_text, option-label keys
     built from settings enums, meta.* metadata) or explicitly allow-listed.

Exit code 0 = all checks pass; nonzero = findings. This is the script cited
by docs/reports/UI_FUNCTIONAL_AUDIT_05/08 and FINAL_UI_FUNCTIONAL_VERIFICATION.
"""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]

# Keys referenced dynamically from settings enum values (dropdown option
# labels) rather than string literals, plus file metadata.
DYNAMIC_PREFIXES = (
    "start_mode.",
    "switching_mode.",
    "candidate_orientation.",
    "ui_theme.",
    "learning.",
    "preferred_encoding.",
    "interface.language.",
    "osk.key.",
)
ALLOWED_UNREFERENCED = {
    "meta.locale",
    "meta.schema",
    "meta.note",
    "settings.interface.language",  # caption key; see UI/UX audit finding N-04
}


def flatten(obj: dict, prefix: str = "") -> dict[str, str]:
    out: dict[str, str] = {}
    for key, value in obj.items():
        dotted = f"{prefix}.{key}" if prefix else key
        if isinstance(value, dict):
            out.update(flatten(value, dotted))
        else:
            out[dotted] = str(value)
    return out


def referenced_keys() -> set[str]:
    # `ui::text("key", ...)` / `ui_strings::get("key")` literals, plus the
    # `show_toast("toast.key", fallback, ...)` form whose first argument is
    # routed through ui::text by show_toast() (application.cpp).  `toast.*`
    # keys also appear inside show_toast argument ternaries
    # (`cond ? "toast.a" : "toast.b", ...`), so any string literal with the
    # toast.* prefix counts as a reference.
    pattern = re.compile(
        r'(?:ui::text|ui_strings::get|show_toast)\(\s*"([A-Za-z0-9_.]+)"'
    )
    # The toast AutomationId is a wide literal (L"toast.message") — exclude
    # L-prefixed strings so it is not mistaken for a locale key.
    toast_pattern = re.compile(r'(?<!L)"(toast\.[a-z_0-9.]+)"')
    found: set[str] = set()
    for base in ("src", "include"):
        for path in (REPO / base).rglob("*"):
            if path.suffix not in {".cpp", ".hpp"}:
                continue
            text = path.read_text(encoding="utf-8", errors="replace")
            for match in pattern.finditer(text):
                found.add(match.group(1))
            for match in toast_pattern.finditer(text):
                found.add(match.group(1))
    return found


def main() -> int:
    locales: dict[str, dict[str, str]] = {}
    for locale_dir in sorted((REPO / "data/i18n").iterdir()):
        table = locale_dir / "ui.json"
        if table.is_file():
            locales[locale_dir.name] = flatten(json.loads(table.read_text(encoding="utf-8")))

    failures = 0
    names = sorted(locales)
    sizes = {name: len(keys) for name, keys in locales.items()}
    print(f"locale key counts: {sizes}")
    if len(set(sizes.values())) != 1:
        print("FAIL: locale key counts differ")
        failures += 1

    base = locales[names[0]]
    for name in names[1:]:
        only_base = sorted(set(base) - set(locales[name]))
        only_other = sorted(set(locales[name]) - set(base))
        for key in only_base:
            print(f"FAIL: missing in {name}: {key}")
        for key in only_other:
            print(f"FAIL: missing in {names[0]}: {key}")
        failures += len(only_base) + len(only_other)

    placeholder_re = re.compile(r"\{([a-z_]+)\}")
    for key in sorted(set.intersection(*(set(v) for v in locales.values()))):
        sets = {
            name: sorted(placeholder_re.findall(locales[name][key])) for name in names
        }
        if len({tuple(s) for s in sets.values()}) > 1:
            print(f"FAIL: placeholder mismatch {key}: {sets}")
            failures += 1

    referenced = referenced_keys()
    missing = sorted(k for k in referenced if k not in base)
    for key in missing:
        print(f"FAIL: referenced by code but absent from locales: {key}")
    failures += len(missing)

    dead = sorted(
        k
        for k in base
        if k not in referenced
        and not k.startswith(DYNAMIC_PREFIXES)
        and k not in ALLOWED_UNREFERENCED
    )
    for key in dead:
        print(f"FAIL: shipped key referenced nowhere (dead): {key}")
    failures += len(dead)

    print(
        f"summary: locales={names} parity={'OK' if failures == 0 else 'FAILURES'} "
        f"code-referenced={len(referenced)} missing={len(missing)} dead={len(dead)}"
    )
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
