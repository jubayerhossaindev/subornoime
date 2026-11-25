# Mouse and Keyboard Event Tests

Status: **BLOCKED (runtime for GUI events)**; **event-model level executed
on Linux** (KeyEventMatrix, editing/navigation/action classification,
auto-repeat — `part_iii/input_reliability.md`,
`part_iii/navigation_editing.md`).

## Event-coverage matrix (mission §11)

| Event class | GUI surface | Harness coverage | Status |
|---|---|---|---|
| Mouse left click | every button | UIA Invoke + real `SendInput`-class click as separate checks (Save asserted both ways) | BLOCKED |
| Mouse right click / context menu | tray, topbar | manual protocol (`Manual-Test-Protocol.md`) | BLOCKED |
| Hover / enter / leave | topbar zones, OSK keys | manual protocol | BLOCKED |
| Drag | slider thumbs, window moves | manual protocol | BLOCKED |
| Scroll wheel | candidate window, lists | manual protocol | BLOCKED |
| Keyboard: Enter/Space/Escape/Tab/arrows/Home/End/Delete | settings navigation, menus | `NAV-*`, menu keyboard walks (partly manual) | BLOCKED |
| Focus acquisition/loss, repeated focus changes | app windows | manual protocol | BLOCKED |
| Window open/close/minimize/restore | settings/OSK/topbar/toast | harness open/close counts (`ABT-open`, `OSK-open`, `TOP-*`) | BLOCKED |
| Monitor change / DPI change | candidate window positioning | manual protocol | BLOCKED |

## Already executed (engine/event-model level, Linux)

- Every `PhysicalKey` 0–200 through a live session without corruption.
- Auto-repeat semantics (all 4 paths), key-up and modifier suppression.
- Classification disjointness (editing/navigation/action).
- 50,000-keystroke burst soak + 2,000 focus-change recovery cycles
  (`StabilitySoak.FocusChangeChurn…` — focus loss/restore at the session
  level).

Per the mission, engine-level coverage is **not** claimed as GUI-event
coverage; the table above keeps them separate.
