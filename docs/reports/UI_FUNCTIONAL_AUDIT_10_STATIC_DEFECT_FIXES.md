# UI_FUNCTIONAL_AUDIT_10 — Static Defect Fixes: Implementation & Verification (2026-11-24)

Mission: implement the four remaining open static defects from Report 08
(`docs/reports/UI_FUNCTIONAL_AUDIT_08_BUGS_V2.md`) and the traceability matrix
(`UI_FUNCTIONAL_AUDIT_TRACEABILITY_MATRIX_V2.csv`), each with a targeted
regression check, then re-run the full available regression suite.

---

## A. Executive summary

| Metric | Value |
|---|---|
| Original open static defects (Phase 2) | **4** |
| Defects actually fixed here | **4** (FIND-01, FIND-02, FIND-03, FIND-05) |
| Regression tests added | **1 executable portable unit test** + **15 automated source-invariant checks** (1 new evidence script) |
| Total tests executed this phase | **635** (633 unit + 2 integration) — all **PASS** |
| Regression vs Phase 2 baseline (632 unit + 2 integration = 634) | baseline tests unchanged and passing; **+1** new unit test |
| Remaining open static defects | **0** |
| Windows runtime verification | unchanged **BLOCKED** (TRC-040..045, no Windows 10/11 x64 + MSVC host in this sandbox) — never converted to PASS |

### Finding-ID reconciliation (per mission §1)

Report 08 names FIND-01..05 under "Open findings", but the traceability matrix
records exactly **four FAIL rows**. The authoritative mapping (from the CSV,
not the prose):

| Trace row | Finding ID | Report 08 tag | Defect | Disposition |
|---|---|---|---|---|
| TRC-052 | FIND-01 | UIA-01 | OSK paints an empty dead grid key; UIA exposes it with an empty Name | **fixed here** |
| TRC-055 | FIND-02 | UI-01 | OSK mode-button hover highlights the *active* button instead of the hovered one | **fixed here** |
| TRC-054 | FIND-03 | I18N-01 | Tray-menu web-link failure strings hardcoded English | **fixed here** |
| TRC-053 | FIND-05 | UIA-03 | UIA `IsEnabled` always TRUE | **fixed here** |
| — | FIND-04 | UIA-02 | Missing explicit AutomationIds (OSK/About) | **not a FAIL row** — documented recommendation, intentionally NOT implemented (out of scope) |
| — | FIND-06 | — | `font_family` Unicode names | already fixed in Phase 2; its regression test still passes (untouched) |
| — | FIND-07..13 | — | Documented design notes | not defects, not implemented |
| TRC-040..045 | — | — | Windows runtime groups | BLOCKED, unchanged |

One Report 08 pointer is corrected here: FIND-01 cites the empty-Name exposure
as "ui_accessibility.cpp:457-459"; that code is actually the OSK's own
`ElementSource::element_info` default branch at **osk_window.cpp:457-459**
(pre-fix numbering). The CSV row has been annotated accordingly.

---

## B. Defect-by-defect results

### FIND-01 / UIA-01 (TRC-052) — OSK dead grid key painted and exposed with empty UIA Name — **PASS (static fix verified)**

- **Original root cause**: the OSK letter-row grid is 3 × 10 slots; the third
  row has only 9 physical keys, so slot 10 was `{0, L'\0'}` (osk_window.cpp:72
  pre-fix). The paint loop drew all 34 flat keys — an empty button box — and
  the OSK `ElementSource::element_info` default branch exposed every flat key,
  so the dead slot carried an empty UIA Name. Row-3 trailing slot = flat index
  29, UIA element index 32.
- **Original evidence**: osk_window.cpp:72 (table), :590-613 (paint loop,
  pre-fix numbering), :457-459 (empty-Name default branch). Preserved in the
  TRC-052 row and in Report 08 (untouched).
- **Fix implemented** (smallest, local):
  - `osk_window.cpp` — new `Impl::key_is_live(int flat)` predicate (zero-VK
    letter slots are dead; Shift/Space/Backspace/Enter always live):
    - paint loop `continue`s on dead slots (no button box, no label) —
      osk_window.cpp:616-619;
    - `hit_flat_key` returns -1 for dead slots, so hover/press/click can never
      land there — osk_window.cpp:355-364;
    - `element_info` declines dead slots (`return false`), so the OSK source
      no longer exposes them — osk_window.cpp:461-464.
  - `ui_accessibility.cpp` — `Navigate` (FirstChild/LastChild/NextSibling/
    PreviousSibling) now walks through a new `next_exposed_index()` helper and
    skips indices the source declines, instead of handing out fragments for
    non-exposed elements — ui_accessibility.cpp:388-431, 718-731.
    (`ElementProviderFromPoint` already skipped declined indices.)
- **Regression test**: `scripts/ui_audit_static_fix_check.py` — 5 FIND-01
  invariants (paint guard, element_info guard, hit-test guard, Navigate uses
  `next_exposed_index` for all four directions, no raw positional fragment
  creation remains in `Navigate`). Pre-fix run: **5/5 FAIL**; post-fix:
  **5/5 PASS** (full transcript in §D).
- **Behavior-compat check**: every other `ElementSource` in the codebase
  returns false from `element_info` only for out-of-range indices
  (verified by grep over application.cpp / menu_popup.cpp / topbar.cpp /
  toast_window.cpp), so the Navigate change is behavior-neutral for all
  surfaces except the OSK dead slot it fixes.
- **Final status**: **PASS** at the static/source level; live visual + Narrator
  confirmation of the now-invisible slot stays **BLOCKED (ENV-01)**.

### FIND-02 / UI-01 (TRC-055) — OSK mode-button hover cross-wired — **PASS (static fix verified)**

- **Original root cause**: one flag `hover_mode_english` was set when the
  pointer was over *either* mode button (`hit_mode_english` union-tested both
  rects), but the hover face was applied as
  `hover_mode_english && is_english()` for the English button and
  `hover_mode_english && !is_english()` for the Bangla button
  (osk_window.cpp:578-587 pre-fix) — hovering the inactive button highlighted
  the *active* one.
- **Original evidence**: osk_window.cpp:578-587 (pre-fix). Preserved in
  TRC-055 and Report 08.
- **Fix implemented**: hover is now tracked **per button** —
  `int hover_mode = -1` (0 = English, 1 = Bangla, -1 = none,
  osk_window.cpp:93-98); `hit_mode_english` replaced by
  `hit_mode_button(POINT)` returning 0/1/-1 (osk_window.cpp:366-379); paint
  applies the hover face to the hovered button (`hover_mode == 0` /
  `hover_mode == 1`, osk_window.cpp:604-613); WM_MOUSEMOVE / WM_MOUSELEAVE /
  WM_LBUTTONDOWN updated (osk_window.cpp:655-666, 670-677, 693-697). The
  click path previously re-derived the button from
  `point.x < english_rect.right`; it now uses the same hit-test result —
  identical routing, one source of truth.
- **Regression test**: 4 FIND-02 invariants in
  `scripts/ui_audit_static_fix_check.py` (shared flag removed, per-button
  faces, cross-wired expressions absent, per-button hit-test present).
  Pre-fix **4/4 FAIL**, post-fix **4/4 PASS**.
- **Final status**: **PASS** static; live hover behavior **BLOCKED (ENV-01)**.

### FIND-03 / I18N-01 (TRC-054) — tray web-link failure strings hardcoded English — **PASS (static fix verified)**

- **Original root cause**: `handle_menu_command`'s `kMenuUpdates` /
  `kMenuDocumentation` cases passed raw English literals to `open_web_link`
  (application.cpp:3921-3924 pre-fix) while the About dialog localized the
  exact same messages through `ui::text("error.releases_open_failed", …)` /
  `ui::text("error.documentation_open_failed", …)` (application.cpp:766-776).
  Both keys already existed in both locale tables.
- **Original evidence**: application.cpp:3921-3924 vs :766-776. Preserved in
  TRC-054 and Report 08.
- **Fix implemented**: the two tray cases now route through the same
  `ui::text(key, fallback)` calls the About dialog uses — unchanged English
  fallbacks, localized Bangla strings when `ui.language=bn`
  (application.cpp:3920-3931). `ui::text` returns a process-lifetime cached
  `const wchar_t*` (ui_theme.hpp:613), so the call sites stay
  `open_web_link`-compatible with zero API change.
- **Regression tests**:
  - Executable: `UIAuditI18n.TrayWebLinkFailureKeysResolveAndMatchCompiledFallbacks`
    (tests/unit/ui_functional_audit_test.cpp:761) — the keys resolve in the
    English table byte-equal to the compiled fallback literals the tray call
    sites pin, and carry Bengali text in the bn table. **PASS**.
  - Source wiring: 2 FIND-03 invariants in `ui_audit_static_fix_check.py`
    (every occurrence of the failure literals in application.cpp sits inside a
    `ui::text("error.…")` call; the tray uses the same keys as About).
    Pre-fix **2/2 FAIL**, post-fix **2/2 PASS**.
- **Final status**: **PASS** (this fix is fully covered by executed tests —
  the localized strings themselves run portably; the toast rendering on the
  live tray is BLOCKED ENV-01, unchanged from Phase 2).

### FIND-05 / UIA-03 (TRC-053) — UIA IsEnabled always TRUE — **PASS (static fix verified)**

- **Original root cause**: `GetPropertyValues` answered
  `UIA_IsEnabledPropertyId` with `VARIANT_TRUE` unconditionally
  (ui_accessibility.cpp:271-274 pre-fix); disabled popup items only signaled
  through `focusable=false` and Invoke refusal. Note: Report 08's proposal
  said "reflect `ElementSource::element_info.enabled`", but `ElementInfo` had
  **no `enabled` field** — the fix therefore adds it.
- **Original evidence**: ui_accessibility.cpp:271-274. Preserved in TRC-053
  and Report 08.
- **Fix implemented** (additive, backward compatible):
  - `ui_accessibility.hpp` — `ElementInfo::enabled` added, default `true`, so
    every existing `ElementSource` keeps its behavior without modification
    (ui_accessibility.hpp:69-74);
  - `ui_accessibility.cpp` — the IsEnabled case returns TRUE only for the
    window root and otherwise reflects
    `source_->element_info(index_, info) && info.enabled`
    (ui_accessibility.cpp:271-282);
  - `menu_popup.cpp` — the known disabled-item producer sets
    `out.enabled = item.enabled` (menu_popup.cpp:82).
- **Regression test**: 3 FIND-05 invariants in `ui_audit_static_fix_check.py`
  (field exists, provider reflects `info.enabled`, IsEnabled no longer
  hardcoded TRUE; plus menu_popup producer check = 4 checks in the FIND-05
  block). Pre-fix **4/4 FAIL**, post-fix **4/4 PASS**.
- **Final status**: **PASS** static; live Narrator announcement of disabled
  items **BLOCKED (ENV-01)**.

---

## C. Modified files

| File | Change | Why |
|---|---|---|
| `src/platform/windows/app/osk_window.cpp` | modified | FIND-01 (`key_is_live` guard in paint / hit-test / `element_info`) and FIND-02 (per-button hover: `hover_mode`, `hit_mode_button`, paint/mouse handlers) |
| `src/platform/windows/app/ui_accessibility.cpp` | modified | FIND-05 (`IsEnabled` reflects `ElementInfo::enabled`) and FIND-01's provider half (`next_exposed_index` skip in `Navigate`) |
| `include/suborno/platform/windows/app/ui_accessibility.hpp` | modified | FIND-05: additive `ElementInfo::enabled = true` field (documented in-place) |
| `src/platform/windows/app/application.cpp` | modified | FIND-03: tray web-link failure messages routed through `ui::text` (2 call sites) |
| `src/platform/windows/app/menu_popup.cpp` | modified | FIND-05 producer: `out.enabled = item.enabled` (1 line) |
| `tests/unit/ui_functional_audit_test.cpp` | modified | 1 added portable regression test (`UIAuditI18n.TrayWebLinkFailureKeysResolveAndMatchCompiledFallbacks`); no existing test touched |
| `scripts/ui_audit_static_fix_check.py` | **new** | 15 source-invariant regression checks for the Windows-only fixes (the strongest automated check available on Linux) |
| `docs/reports/UI_FUNCTIONAL_AUDIT_TRACEABILITY_MATRIX_V2.csv` | modified | TRC-052..055 rows annotated with fix references + verification evidence; original FAIL evidence preserved in-row; result = PASS (static) with runtime BLOCKED note |
| `docs/reports/UI_FUNCTIONAL_AUDIT_10_STATIC_DEFECT_FIXES.md` | **new** | this report |
| `CHANGELOG.md` | modified | short Phase-12 entry per project convention |

No file was renamed, moved, or deleted. No existing test was weakened, skipped
or removed. Historical reports `UI_FUNCTIONAL_AUDIT_01..09_*` were not edited.

---

## D. Testing results (actual commands and outputs)

Environment (unchanged from Phase 2 honesty statement): Linux sandbox, Debian
13, g++ 14.2.0 (Debian 14.2.0-19), CMake/Ninja under
`build/linux-audit`, clang-format (project style), Python 3 evidence scripts.
No Windows 10/11 x64 + MSVC host is available; no mingw cross-compiler is
installed — Windows-only TUs are verified by source-level checks, honestly
labeled.

### D.1 Pre-fix demonstration (regression tests fail before the fix)

```
$ git stash push -- <five production files>          # back to pre-fix tree
$ python3 scripts/ui_audit_static_fix_check.py
--- FIND-01 --- FAIL x5  --- FIND-02 --- FAIL x4  --- FIND-03 --- FAIL x2  --- FIND-05 --- FAIL x4
summary: 15 invariants, 15 failures
PRE_FIX_EXIT=1
$ git stash pop
```

### D.2 Post-fix static invariants

```
$ python3 scripts/ui_audit_static_fix_check.py
--- FIND-01 ---  PASS x5
--- FIND-02 ---  PASS x4
--- FIND-03 ---  PASS x2
--- FIND-05 ---  PASS x4
summary: 15 invariants, 0 failures (TRC-052/053/054/055 static halves; runtime halves BLOCKED on Linux)
exit 0
```

### D.3 Build and full regression suite

```
$ cmake --build build/linux-audit --parallel 2
[2/2] Linking CXX executable suborno_unit_tests          # no warnings, no errors
$ cd build/linux-audit && ./suborno_unit_tests
[==========] 633 tests from 73 test suites ran. (565 ms total)
[  PASSED  ] 633 tests.                                   # 632 baseline + 1 new
$ ctest
100% tests passed out of 2                                # suborno.integration + unit driver
$ ./suborno_unit_tests --gtest_filter="UIAudit*"
[==========] 24 tests from 9 test suites ran.             # 23 Phase-2 audit tests + 1 new — all PASS
$ ./suborno_unit_tests --gtest_filter="UIAuditI18n.TrayWebLinkFailureKeysResolveAndMatchCompiledFallbacks"
[  PASSED  ] 1 test.
$ ./suborno_unit_tests --gtest_filter="UIAuditFontFamily.*"
PASS                                                      # FIND-06 regression test untouched and still green
```

Total executed: **633 unit + 2 integration = 635, 0 failures** (Phase 2
baseline 632 + 2 = 634 reconciled: same 634 all passing + 1 added test).

### D.4 Static checks / lint

```
$ clang-format --dry-run --Werror src/platform/windows/app/*.cpp \
    include/suborno/platform/windows/app/ui_accessibility.hpp tests/unit/ui_functional_audit_test.cpp
FORMAT_OK                                                 # exit 0 (after formatting the new test code)
$ python3 scripts/i18n_key_check.py
summary: locales=['bn','en'] parity=OK code-referenced=156 missing=0 dead=0
$ python3 scripts/contrast_check.py
summary: 22 pairs, 0 required-pair failures
$ python3 scripts/check-layering.py
layering OK: 20 cross-module edges, all inside the allowed matrix
```

Warning scan: rebuilding the touched portable TU emits **no compiler
warnings** (warnings/error grep over the build output is empty). The four
Windows-only TUs are not compiled by this toolchain; their correctness here is
covered by D.2 plus the clang-format parse pass — a Windows/MSVC build remains
a runner task (§E).

---

## E. Remaining limitations

**Fixed static defects**: FIND-01, FIND-02, FIND-03, FIND-05 (all four, with
regression evidence above).

**Unresolved static defects**: none. FIND-04 (AutomationIds) remains what it
was before this phase — a documented recommendation, not a FAIL row; it is
deliberately not implemented (scope protection).

**Windows runtime BLOCKED items (unchanged from Phase 2)** — require a
Windows 10/11 x64 + MSVC runner:

- TRC-040 live control interaction (clicks, popup behavior on every window);
- TRC-041 tray / topbar live behavior;
- TRC-042 OSK live key clicks (including the now-skipped dead slot — visual
  check that the empty box is gone);
- TRC-043 candidate window live composition UI;
- TRC-044 UIA / Narrator live session (including `IsEnabled=false`
  announcements on disabled popup items and clean tree walking over the OSK);
- TRC-045 DPI / monitor / theme switching.

**Verification that could not be performed here**: MSVC compilation of the
four modified Windows TUs; live behavioral confirmation of each fixed path.
These are extensions of TRC-040..045, not new gaps. No BLOCKED item was
converted into PASS; the four fixes' PASS status covers the static/source
level only, exactly as recorded in the traceability rows.

---

## F. Final acceptance checklist

- [x] All four FAIL-row defects (FIND-01, FIND-02, FIND-03, FIND-05) individually investigated; FIND-04 correctly excluded per the CSV reconciliation.
- [x] Every fix is a production-code change — no workaround, mock, TODO, commented-out code, or documentation-only substitute.
- [x] Smallest safe localized changes: 4 production files + 1 header; no redesign, renames, reordering, or unrelated behavior changes.
- [x] FIND-06 implementation untouched; its regression tests pass unchanged.
- [x] Targeted regression tests added: 15 source invariants (fail 15/15 pre-fix, pass 15/15 post-fix) + 1 executable portable test.
- [x] Full available regression suite re-executed: 633 unit + 2 integration, 0 failures; i18n / contrast / layering scripts green; clang-format clean; no new compiler warnings.
- [x] No existing test disabled, weakened, or removed; no CI rule changed.
- [x] Windows runtime statuses remain honestly BLOCKED (TRC-040..045); static vs runtime verification kept distinct everywhere.
- [x] Traceability matrix updated in place with fix evidence; original FAIL evidence preserved; historical Reports 01..09 not rewritten.
- [x] Every PASS/FAIL/BLOCKED claim in this report carries an actual command, test name, or file:line pointer.
