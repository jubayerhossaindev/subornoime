#!/usr/bin/env python3
"""WCAG 2.x contrast recompute for the Suborno UI theme tokens (evidence script).

Parses the kSubornoDark / kSubornoLight palette literals straight out of
include/suborno/platform/windows/app/ui_theme.hpp and recomputes the
documented text/boundary pairings against the WCAG AA floors (text >= 4.5:1,
non-text boundaries >= 3:1).  Exit code 0 = every REQUIRED pairing passes;
pairings below the floor that have zero current consumers are printed with
the NOT-APPLICABLE pairing rule (ui_theme.hpp header comment) so a reviewer
can see both facts.

This is the script cited by docs/reports/UI_FUNCTIONAL_AUDIT_05/06.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

THEME = (
    Path(__file__).resolve().parents[1]
    / "include/suborno/platform/windows/app/ui_theme.hpp"
)

# (theme, fg token, bg token, floor, required?) — required=False pairs are
# documented combination checks whose current consumers live on `surface`
# (see the pairing-rule comment next to .text_inactive in ui_theme.hpp).
PAIRS = [
    ("dark", "text", "surface", 4.5, True),
    ("dark", "text_inactive", "surface", 4.5, True),
    ("dark", "text", "surface_raised", 4.5, True),
    ("dark", "text_inactive", "surface_raised", 4.5, False),
    ("dark", "text", "surface_hover", 4.5, True),
    ("dark", "text_inactive", "surface_hover", 4.5, False),
    ("dark", "text", "surface_pressed", 4.5, True),
    ("dark", "text", "surface_topbar", 4.5, True),
    ("dark", "on_accent", "accent_fill", 4.5, True),
    ("dark", "accent", "surface", 3.0, True),
    ("dark", "control_border", "surface", 3.0, True),
    ("dark", "focus", "surface", 3.0, True),
    ("light", "text", "surface", 4.5, True),
    ("light", "text_inactive", "surface", 4.5, True),
    ("light", "text", "surface_raised", 4.5, True),
    ("light", "text_inactive", "surface_raised", 4.5, True),
    ("light", "text", "surface_hover", 4.5, True),
    ("light", "text", "surface_pressed", 4.5, True),
    ("light", "text", "surface_topbar", 4.5, True),
    ("light", "on_accent", "accent_fill", 4.5, True),
    ("light", "accent", "surface", 3.0, True),
    ("light", "control_border", "surface", 3.0, True),
]


def relative_luminance(rgb: tuple[int, int, int]) -> float:
    def channel(value: float) -> float:
        value /= 255.0
        return value / 12.92 if value <= 0.04045 else ((value + 0.055) / 1.055) ** 2.4

    r, g, b = rgb
    return 0.2126 * channel(r) + 0.7152 * channel(g) + 0.0722 * channel(b)


def contrast(a: tuple[int, int, int], b: tuple[int, int, int]) -> float:
    la, lb = relative_luminance(a), relative_luminance(b)
    hi, lo = max(la, lb), min(la, lb)
    return (hi + 0.05) / (lo + 0.05)


def parse_palette(name: str) -> dict[str, tuple[int, int, int]]:
    text = THEME.read_text(encoding="utf-8")
    block = text.split(f"constexpr ThemePalette {name}{{")[1].split("};")[0]
    return {
        match[0]: (int(match[1], 16), int(match[2], 16), int(match[3], 16))
        for match in re.findall(
            r"\.(\w+)\s*=\s*RGB\(0x([0-9A-Fa-f]{2}),\s*0x([0-9A-Fa-f]{2}),\s*0x([0-9A-Fa-f]{2})\)",
            block,
        )
    }


def main() -> int:
    palettes = {"dark": parse_palette("kSubornoDark"), "light": parse_palette("kSubornoLight")}
    failures = 0
    for theme, fg, bg, floor, required in PAIRS:
        ratio = contrast(palettes[theme][fg], palettes[theme][bg])
        if ratio >= floor:
            status = "PASS"
        elif required:
            status = "FAIL"
            failures += 1
        else:
            status = "NOT APPLICABLE (documented pairing rule; no current consumer)"
        print(f"{theme:5s} {fg:15s} on {bg:15s} {ratio:6.2f}:1  floor {floor:<4} {status}")
    print(f"summary: {len(PAIRS)} pairs, {failures} required-pair failures")
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
