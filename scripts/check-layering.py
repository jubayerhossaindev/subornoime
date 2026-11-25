#!/usr/bin/env python3
"""Layering checker for SubornoIME module dependencies (audit #2, gap 4).

Verifies that `#include "suborno/..."` edges across product code (src/ and
include/) follow the documented layering contract in
docs/architecture/layering.md. Test, example and benchmark trees may include
any product module and are out of scope by design.

Usage:
    python3 scripts/check-layering.py            # enforce (exit 1 on violation)
    python3 scripts/check-layering.py --report   # print the edge summary only

Layer model (bottom -> top):
    leaves      : unicode, persistence, dictionary, settings
    contract    : layout-api            (may use unicode)
    services    : spell, candidates, layouts, tools
    composition : core                 (may compose everything below it)
    host        : platform             (thin shim over core + layout-api)

The allowed-edge matrix is a superset of the dependency graph measured by the
audit (21 edges), chosen to forbid upward/host leakage while keeping room for
near-term growth without a checker edit.
"""
import argparse
import os
import re
import sys
from collections import defaultdict

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

MODULES = [
    "candidates", "core", "dictionary", "layout-api", "layouts",
    "persistence", "platform", "settings", "spell", "tools", "unicode",
]

# target module -> modules it may include (the layering contract)
ALLOWED = {
    "unicode": set(),
    "persistence": set(),
    "dictionary": set(),
    "settings": set(),
    "layout-api": {"unicode"},
    "spell": {"dictionary", "layout-api", "unicode"},
    "candidates": {"dictionary", "layout-api", "spell", "unicode"},
    "layouts": {"candidates", "dictionary", "layout-api", "unicode"},
    "core": {"candidates", "dictionary", "layout-api", "layouts",
             "settings", "spell", "unicode"},
    "tools": {"dictionary"},
    "platform": {"core", "layout-api"},
}

INCLUDE_RE = re.compile(r'#include\s+[<"]suborno(?:/([A-Za-z0-9._/-]+))?[">]')


def module_of(rel_path):
    """Map a repo-relative file path to its product module (or None)."""
    parts = rel_path.split(os.sep)
    if len(parts) >= 3 and parts[0] == "include" and parts[1] == "suborno":
        name = parts[2]
        return name if name in MODULES else None  # suborno.h -> umbrella
    if len(parts) >= 2 and parts[0] == "src":
        return parts[1] if parts[1] in MODULES else None
    return None


def scan():
    edges = defaultdict(list)  # (from, to) -> [(file, line, text)]
    for base in ("src", "include"):
        full = os.path.join(ROOT, base)
        for dirpath, _, files in os.walk(full):
            for fname in files:
                if not fname.endswith((".cpp", ".hpp", ".h", ".cc")):
                    continue
                fpath = os.path.join(dirpath, fname)
                rel = os.path.relpath(fpath, ROOT)
                src_mod = module_of(rel)
                if src_mod is None:
                    continue
                with open(fpath, encoding="utf-8", errors="replace") as fh:
                    for lineno, line in enumerate(fh, 1):
                        m = INCLUDE_RE.search(line)
                        if not m:
                            continue
                        sub = m.group(1)  # None = bare suborno.h umbrella
                        if sub is None:
                            continue  # public C-API umbrella: allowed for all
                        top = sub.split("/")[0]
                        if top == src_mod:
                            continue  # same-module includes are free
                        if top not in MODULES and os.path.exists(
                                os.path.join(ROOT, "include", "suborno", sub)):
                            continue  # root-level public header: allowed
                        edges[(src_mod, top)].append((rel, lineno, line.strip()))
    return edges


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--report", action="store_true",
                    help="print the dependency summary and exit 0")
    args = ap.parse_args()

    edges = scan()
    if args.report:
        for (a, b), sites in sorted(edges.items()):
            print(f"{a:>10} -> {b:<12} ({len(sites)} include sites)")
        print(f"total cross-module edges: {len(edges)}")
        return 0

    violations = []
    for (src_mod, dst), sites in sorted(edges.items()):
        if dst not in ALLOWED.get(src_mod, set()):
            for rel, lineno, text in sites:
                violations.append(f"{rel}:{lineno}: {text} "
                                  f"[{src_mod} may not include {dst}]")

    if violations:
        print(f"LAYERING VIOLATIONS ({len(violations)}):", file=sys.stderr)
        for v in violations:
            print("  " + v, file=sys.stderr)
        print("See docs/architecture/layering.md for the contract.",
              file=sys.stderr)
        return 1

    print(f"layering OK: {len(edges)} cross-module edges, "
          f"all inside the allowed matrix")
    return 0


if __name__ == "__main__":
    sys.exit(main())
