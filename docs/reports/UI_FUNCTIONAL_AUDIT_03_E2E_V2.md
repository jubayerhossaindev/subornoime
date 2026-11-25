# UI_FUNCTIONAL_AUDIT_03 — E2E Testing Report (Phase 2, 2026-11-24)

Nine mission scenarios, each decomposed into the steps that could execute here
(real engine/session/settings/i18n execution) and the steps that need the live
Windows shell (BLOCKED). Test code: `tests/unit/ui_functional_audit_test.cpp`
+ existing suites.

| # | Scenario | Executed here (PASS evidence) | BLOCKED (Windows runtime) |
|---|---|---|---|
| 01 | First launch | `FirstLaunchPersistsDefaultsAndEngineStartsOnDefaultLayout`: fresh engine over absent file starts on `suborno-phonetic`; store reports `file_existed=false` | Live window geometry, tray integration, status indicators on screen |
| 02 | Change interface language | `LanguageSelectionFollowsPortableLocaleMapping` (bn/bangla→bangla; system/en/other→english); `ShippedLocaleFilesCarryEveryInventoryKeyInBothLanguages` (all 157 keys in both locales); `RestartPersistenceEverySavedSettingRestored` (language=bn survives restart) | Live re-render of Settings/topbar/tray/OSK/About/toasts in Bangla |
| 03 | Change theme | theme_id suborno-dark/light/system round-trip; `scripts/contrast_check.py` recomputes both palettes: **22 pairs, 0 required-pair failures** | Live repaint of every window; High-Contrast on/off; System-follow |
| 04 | Change layout | `DefaultLayoutDropdownCoversAllSevenRegisteredLayouts` (all 7 ids persist); `PreviewLabelsForEveryRegisteredLayout` (OSK labels per layout, Probhat ক / National ত anchors); typing output per layout covered by 1,000+ layout-regression assertions | Typing into a real app through TSF; hotkey cycling |
| 05 | Font configuration | `UIAuditFontFamily.*`: Unicode Bangla font names persist; OS-default dependency; font list contract | Live rendering of the chosen font in topbar/OSK/candidates |
| 06 | Cancel unsaved changes | `CancelLeavesPersistedStateUntouched` + `SavingOneControlLeavesEveryOtherValueIntact` | Live staged-vs-committed UI check on screen |
| 07 | OSK interaction | `PreviewLabelsForEveryRegisteredLayout`; key model (34 keys incl. Shift latch, Space/Backspace/Enter) traced; `OSK-MODE-*` handlers verified static | Real `SendInput` clicks producing text in a target app; mode buttons on screen |
| 08 | Tray interaction | All 12 menu items traced to handlers; toggle/enable paths feed `toast.*` + tray tooltip strings (i18n keys verified) | Real tray menu open/click; profile toggle; toast appearance |
| 09 | Application restart | `RestartPersistenceEverySavedSettingRestored` (layout, language, theme, font, encoding, shortcut, start mode all restored via fresh Engine + fresh store); `RapidRepeatedSaveLoadCyclesStayConsistent` (50 cycles) | Real process exit/relaunch, start-mode surface choice on screen |

## Composition-level E2E (typing into an application)

The deepest executable slice: real `Engine` → `InputSession` → commit →
output encoding, i.e. exactly what the TSF text service feeds into apps:

- All 7 layouts commit representative words (`layout_typing_regression_test`,
  `national_layout_test` positional suite 121/121 engine rows).
- Encoding flip changes committed output: Unicode passthrough vs Bijoy bytes
  (`cv'lv` golden); broken-conjunct ZWNJ drops only the joiner
  (`ToggleProducesUnicodeAndBijoyOutput`).
- Candidate pipeline stages (exact/prediction/prefix/spelling/layout) and
  personal-dictionary learning covered by the candidate/dictionary suites.

BLOCKED: the final `ITfTextService` → target-application keystroke delivery,
candidate-window visuals, and caret tracking need the live shell.

## Verdict

- Executed E2E scenarios: **9/9** with at least their model-level core
  verified and passing.
- Verified E2E workflows end-to-end on the live Windows shell: **0** — every
  scenario's live-shell steps remain BLOCKED until a Windows 10/11 x64 +
  MSVC environment runs this audit's Windows-runtime session (TRC-040..045).
