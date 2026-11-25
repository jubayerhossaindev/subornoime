# Report 07 — Persistence & State Report (Save / Cancel / Reset / Restart)

## 1. Save path (traced end-to-end)

`save_settings_from_ui()` (settings footer Save, Enter, UIA Invoke):

1. Registry startup preference written first; on settings-save failure it is
   **rolled back** to the previous value and the error dialog shows the HRESULT —
   the two stores can never disagree.
2. `Engine::save_settings()` — schema v2 key/value file, strict `validate()` on every
   field (layout ids, theme charset, font charset/≤32 chars `LF_FACESIZE-1`, language
   enum, ui_mode enum, shortcut allow-list of 9 incl. Windows-reserved chords,
   opacity 0–100, seconds 1–3600, position ±32767, candidate_limit 1–100), written via
   the atomic-file layer (temp + rename).
3. Live apply: theme, UI language (locale reload + cache clear + retitles + OSK
   labels), layout & encoding hotkeys re-registered (with already-in-use diagnostics),
   app-wide fonts, topbar snapshot + label, status line.
4. `staged_` re-synced from the engine, pages rebuilt, window hidden; success toast.

**Executed evidence (PASS, logic layer):** settings round-trip incl. nested groups,
validation rejection of invalid values, atomic write behavior, first-launch defaults
persist, corrupt-file quarantine (`invalid_format`/`unsupported_version` → `.corrupt-`
preserving user data, restart from defaults) — all covered by the executed 542-test
suite (SettingsTest, PersistenceCoverageTest, AtomicFileTest, CoverageApiSettings…).

## 2. Cancel / Close / Esc (discard contract)

`cancel_settings()` resets `staged_` from the engine, rebuilds pages, resets press/
slider state, hides. Nothing is written: Cancel and Close(X) and Esc are
**verified non-persisting by construction** (no engine call exists on that path).
`show_window()` re-stages from the engine on every open, so a hide/reopen cycle also
discards. WM_CLOSE hides to tray (tray present) — same discard semantics.

## 3. Immediate (non-staged) actions

Clear Learnt Words (settings button and tray menu entry) bypasses staging entirely:
confirm → `engine_->clear_learnt_words()` → status refresh → toast. Save/Cancel never
re-run or undo it (documented in code). Font selections from the tray/topbar menus
persist immediately via `save_font()` (error dialog on failure) and re-stage the
settings window. Tray layout selections persist immediately via `save_layout()` which
also updates `last_bangla_layout_id` for the mode toggle.

## 4. Cross-surface state consistency

- Layout change from any surface (tray, topbar menu, mode icon, OSK, hotkey, settings
  dropdown) funnels into the same `save_layout()` → toast + status + OSK label refresh
  + topbar label refresh.
- The TSF text service re-checks the settings file stamp per keystroke
  (`reload_settings_if_changed`), so a shell-side layout switch takes effect on the
  next keystroke without focus loss (traced; typing-level runtime BLOCKED).
- `last_ui_mode` (topbar|tray|window) persisted by every visibility change and honored
  by the last-state start mode; `--startup` launch suppresses the settings window.
- Tray tooltip and General-page status line always reflect the real
  `ITfInputProcessorProfiles` state (`profile_enabled()`), not a cached flag.

## 5. Restart behavior

**Executed (PASS, logic layer):** full load path incl. legacy-version migration of the
format, unknown-key tolerance, quarantined-corrupt recovery, registry/`start_with_windows`
reconciliation at `initialize()` (registry wins, then persisted back so the two agree).
**BLOCKED (runtime):** observing a real relaunch (manual protocol §6 — modify+save,
close, relaunch, verify every control shows the persisted value).

## 6. Negative persistence cases (executed where possible)

| Case | Evidence |
|---|---|
| Hand-edited invalid values rejected | PASS — validation tests executed |
| Malformed/corrupt settings file | PASS — quarantine tests executed |
| Unknown keys in file | PASS — tolerated-by-design tests executed |
| Write failure (read-only dir) | Traced — atomic-file error paths unit-covered; live disk-failure BLOCKED |
| i18n file missing/unparsable/missing key | PASS — `UiStringsTest` malformed-input cases executed; UI cannot blank (English fallback per key) |
