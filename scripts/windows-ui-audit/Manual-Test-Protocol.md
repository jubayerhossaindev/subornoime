# Manual Test Protocol — SubornoIME (Windows runtime acceptance)

Companion to `Invoke-SubornoUiAudit.ps1` for everything UIA cannot observe. Check
each box only after performing it; attach screenshots where marked. Any failure goes
into `docs/reports/UI_FUNCTIONAL_AUDIT_08_BUGS.md` with steps + severity.

## 1. Narrator (15–20 min)

1. Start Narrator (`Win+Ctrl+Enter`), open Settings.
2. Tab through one full cycle: every checkbox must announce its label AND state
   ("checked"/"not checked"), every dropdown its section heading, both sliders their
   label AND current value, Save/Cancel/Close their names.
3. Ctrl+Tab across all four pages; Narrator must announce the new page's first control.
4. On a dropdown: press Space, arrow through options (each announced), Enter — the
   selected value must be announced.
5. On a slider: ←/→/PgUp/PgDn/Home/End — each new value announced.
6. Open the About dialog: five buttons in Tab order, Esc closes.
7. Open the OSK: invoke any 5 keys from the UIA element list / by clicking — the typed
   result must appear in an open Notepad window (Narrator must not steal focus).
8. Trigger a toast (switch layout from the tray): Narrator announces the message;
   typing focus never moves.

## 2. Tray menu (10 min)

- [ ] Right-click tray: full menu opens; Esc dismisses; focus returns to the previous app.
- [ ] Show Topbar — checkmark reflects visibility each time; topbar toggles.
- [ ] Keyboard Layout submenu — all 7 items; radio dot on the active one; selecting each
      updates topbar label + status line + toast; typing in Notepad immediately uses it.
- [ ] Font submenu — System Default + installed families; selecting applies to the
      topbar/OSK/menus/candidates immediately and persists after restart.
- [ ] Enable/Disable Suborno in Windows — tooltip text flips; toast success/warning.
- [ ] Check for Updates opens the releases page; Documentation opens the README.
- [ ] Clear Learnt Words… — confirmation dialog (default No); Yes clears; toast shows.
- [ ] Uninstall Suborno opens Programs & Features.
- [ ] Exit removes the tray icon and ends the process (Task Manager).

## 3. Themes (10 min)

- [ ] Settings → Interface → Theme: Suborno Dark / Suborno Light / System — the settings
      window, About, topbar, OSK and next toast all switch (screenshot each).
- [ ] With System: flip Windows Settings → Personalization → Colors between Light and
      Dark while all surfaces are open — every live surface repaints.
- [ ] Enable High Contrast (Left Alt+Left Shift+PrtScn): all live windows repaint with
      system colors; disable restores the saved theme.

## 4. DPI & monitors (15 min)

- [ ] 100% / 125% / 150% / 175% / 200%: every surface proportional, text crisp, no
      clipped corners, Save/Cancel always on screen (screenshot 125% and 200%).
- [ ] Move the Settings window, About, topbar and OSK to a second monitor with a
      different scale factor: geometry rescales live (no bitmap blur), hit targets
      still land on the controls.
- [ ] Change scale while a window is open: the window reflows (WM_DPICHANGED), corners
      remain rounded, tooltips follow the new zone rects.
- [ ] Popups near screen edges: settings dropdown near the bottom edge flips above;
      menus clamp inside the work area.

## 5. Negative / robustness (10 min)

- [ ] Rapid repeated clicks on Save (≥10): no crash, single logical commit, window hides once.
- [ ] Rapidly open/close dropdowns (≥10): no orphan popup, no focus loss.
- [ ] Switch pages mid-edit, then Cancel: everything discards; reopen shows persisted values.
- [ ] Save with a Just-in-time modified settings file on disk (hand-edit while open):
      next keystroke reloads per file-stamp (layout switch applies without refocus).
- [ ] Delete data/i18n/en/ui.json beside the exe and switch language: English compiled
      fallback shows; app never blanks or crashes.
- [ ] Hand-edit settings.ini with `theme_id=<garbage>` and relaunch: quarantine dialog/
      log path, defaults active, original file preserved as `.corrupt-*`.
- [ ] Long Bangla layout names and 17 font entries: labels ellipsize, menus clamp.
- [ ] Toggle the encoding hotkey (Ctrl+Alt+E) with a toast visible; exit the app while a
      toast is visible — no crash, icon gone.

## 6. Restart persistence (5 min)

- [ ] Modify + Save: language=বাংলা, theme=Light, layout=Probhat, opacity=35,
      seconds=25, shortcut=F4, remember-position on; drag the topbar; close via Exit.
- [ ] Relaunch: every value above restored; topbar at the dragged position; start
      surface per Start-mode; General status line matches the real Windows profile.
- [ ] Verify settings.ini contains the expected keys (compare with the schema doc).

## 7. Evidence

Save all screenshots under `docs/reports/evidence/<date>/` and reference them from
Report 06/09. Update the traceability matrix BLOCKED rows to PASS with the date +
screenshot/log reference — never without them.
