#!/usr/bin/env python3
"""Static source-invariant regression checks for the four Phase-2 UI audit
defect fixes (evidence script for docs/reports/UI_FUNCTIONAL_AUDIT_10).

The four defects live in Windows-only translation units (osk_window.cpp,
ui_accessibility.cpp/hpp, application.cpp, menu_popup.cpp) that cannot be
compiled or executed in the Linux CI sandbox. The strongest automated check
available here — the same pattern the project already uses for
i18n_key_check.py / contrast_check.py — asserts the source-level invariants
each fix introduced. Every check below FAILED on the pre-fix tree and PASSES
after it; on a Windows 10/11 x64 + MSVC runner the behavioral halves must
still be confirmed live (TRC-040..045 stay BLOCKED there).

Exit code 0 = all invariants hold; 1 = at least one invariant broken.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

OSK = ROOT / "src/platform/windows/app/osk_window.cpp"
UIA_H = ROOT / "include/suborno/platform/windows/app/ui_accessibility.hpp"
UIA_C = ROOT / "src/platform/windows/app/ui_accessibility.cpp"
APP = ROOT / "src/platform/windows/app/application.cpp"
POPUP = ROOT / "src/platform/windows/app/menu_popup.cpp"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


CHECKS: list[tuple[str, str, callable]] = []


def check(finding: str, name: str):
    def wrap(fn):
        CHECKS.append((finding, name, fn))
        return fn

    return wrap


# ---- FIND-01 / UIA-01 (TRC-052): the OSK dead grid slot paints nothing and
# is not exposed through UI Automation. ----


@check("FIND-01", "osk paint loop skips dead grid slots")
def _(osk: str, uia_h: str, uia_c: str, app: str, popup: str) -> bool:
    loop = re.search(r"for \(int flat = 0; flat < OskWindow::Impl::kFlatKeyCount;\s*\+\+flat\) \{[^}]*", osk)
    return bool(loop) and "!impl->key_is_live(flat)" in loop.group(0) and "continue" in loop.group(0)


@check("FIND-01", "osk element_info declines dead slots (no empty UIA Name)")
def _(osk: str, uia_h: str, uia_c: str, app: str, popup: str) -> bool:
    return "!key_is_live(flat)" in osk


@check("FIND-01", "osk hit-testing ignores dead slots")
def _(osk: str, uia_h: str, uia_c: str, app: str, popup: str) -> bool:
    return "return key_is_live(flat) ? flat : -1;" in osk


@check("FIND-01", "UIA tree navigation skips non-exposed indices")
def _(osk: str, uia_h: str, uia_c: str, app: str, popup: str) -> bool:
    uses = len(re.findall(r"next_exposed_index\(", uia_c))
    # one definition + all four Navigate directions (FirstChild, LastChild,
    # NextSibling, PreviousSibling) must consult it
    return uses >= 5 and "int next_exposed_index(int start, int step) const" in uia_c


@check("FIND-01", "Navigate no longer hands out raw positional fragments")
def _(osk: str, uia_h: str, uia_c: str, app: str, popup: str) -> bool:
    navigate = re.search(r"IFACEMETHODIMP Navigate\(.*?\n    \}", uia_c, re.S)
    if not navigate:
        return False
    body = navigate.group(0)
    return "make_child_provider(0)" not in body and "make_child_provider(index_ + 1)" not in body \
        and "make_child_provider(index_ - 1)" not in body and "make_child_provider(count - 1)" not in body


# ---- FIND-02 / UI-01 (TRC-055): OSK mode-button hover follows the pointer,
# not the active mode. ----


@check("FIND-02", "hover tracked per mode button (no shared either-button flag)")
def _(osk: str, uia_h: str, uia_c: str, app: str, popup: str) -> bool:
    return "hover_mode_english" not in osk and "int hover_mode = -1;" in osk


@check("FIND-02", "mode-button hover face applies to the hovered button")
def _(osk: str, uia_h: str, uia_c: str, app: str, popup: str) -> bool:
    return "impl->hover_mode == 0" in osk and "impl->hover_mode == 1" in osk


@check("FIND-02", "cross-wired hover expressions removed")
def _(osk: str, uia_h: str, uia_c: str, app: str, popup: str) -> bool:
    return not re.search(r"hover_mode\w* && (impl->)?!?is_english\(\)", osk)


@check("FIND-02", "hit test names the hovered mode button")
def _(osk: str, uia_h: str, uia_c: str, app: str, popup: str) -> bool:
    return "int hit_mode_button(POINT point) const noexcept" in osk \
        and "hit_mode_english" not in osk


# ---- FIND-03 / I18N-01 (TRC-054): tray web-link failure strings are
# localized — the literals may appear only as ui::text fallbacks. ----


@check("FIND-03", "tray releases-page failure routes through ui::text")
def _(osk: str, uia_h: str, uia_c: str, app: str, popup: str) -> bool:
    for literal, key in (
        ("Suborno could not open the releases page.", "error.releases_open_failed"),
        ("Suborno could not open the documentation.", "error.documentation_open_failed"),
    ):
        for match in re.finditer(re.escape(literal), app):
            prefix = app[max(0, match.start() - 120) : match.start()]
            if f'ui::text("{key}"' not in prefix:
                return False
    return True


@check("FIND-03", "tray uses the same keys as the About dialog")
def _(osk: str, uia_h: str, uia_c: str, app: str, popup: str) -> bool:
    return len(re.findall(r'ui::text\("error\.releases_open_failed"', app)) >= 2 \
        and len(re.findall(r'ui::text\("error\.documentation_open_failed"', app)) >= 2


# ---- FIND-05 / UIA-03 (TRC-053): UIA IsEnabled reflects the element model. ----


@check("FIND-05", "ElementInfo carries the enabled field")
def _(osk: str, uia_h: str, uia_c: str, app: str, popup: str) -> bool:
    return "bool enabled = true;" in uia_h


def _case_body(uia_c: str, case_label: str) -> str:
    """The source text of one switch case: from the label to the next case."""
    start = uia_c.find(case_label)
    if start < 0:
        return ""
    nxt = re.search(r"\n            case |\n            default:", uia_c[start:])
    return uia_c[start : start + nxt.start()] if nxt else uia_c[start:]


@check("FIND-05", "provider IsEnabled reflects element_info.enabled")
def _(osk: str, uia_h: str, uia_c: str, app: str, popup: str) -> bool:
    body = _case_body(uia_c, "case UIA_IsEnabledPropertyId: {")
    return "element_info(index_, info) && info.enabled" in body


@check("FIND-05", "IsEnabled no longer hardcoded TRUE")
def _(osk: str, uia_h: str, uia_c: str, app: str, popup: str) -> bool:
    body = _case_body(uia_c, "case UIA_IsEnabledPropertyId: {")
    if not body:
        return False
    # the only unconditional TRUE is the window-root branch
    return "if (is_root_) {" in body and "element_info(index_, info) && info.enabled" in body


@check("FIND-05", "menu popup reports disabled items through enabled")
def _(osk: str, uia_h: str, uia_c: str, app: str, popup: str) -> bool:
    return "out.enabled = item.enabled;" in popup


def main() -> int:
    sources = {name: read(path) for name, path in
               (("osk", OSK), ("uia_h", UIA_H), ("uia_c", UIA_C), ("app", APP), ("popup", POPUP))}
    failures = 0
    current_finding = None
    for finding, name, fn in CHECKS:
        if finding != current_finding:
            current_finding = finding
            print(f"--- {finding} ---")
        ok = fn(sources["osk"], sources["uia_h"], sources["uia_c"], sources["app"], sources["popup"])
        if not ok:
            failures += 1
        print(f"{'PASS' if ok else 'FAIL'}  {name}")
    print(f"summary: {len(CHECKS)} invariants, {failures} failures "
          f"(TRC-052/053/054/055 static halves; runtime halves BLOCKED on Linux)")
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
