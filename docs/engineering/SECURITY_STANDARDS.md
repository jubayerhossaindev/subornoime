# Security Standards

An IME is keyboard-adjacent security-sensitive software: it reads every
keystroke, injects input, and ships as a DLL loaded into arbitrary processes.

## Memory safety

- Buffer boundaries: every C-API output write checks the buffer size first
  (`suborno_session_snapshot` → `SUBORNO_ERROR_BUFFER_TOO_SMALL`); string
  copies always NUL-terminate.
- Integer safety: lengths/offsets are `std::size_t`; arithmetic that crosses
  signedness uses explicit casts; decode loops bound-check the remaining bytes
  (`at + 3 < text.size()` in the UTF-8 decoders).
- No use-after-free / double-free: ASan + UBSan CI runs, fuzzing with
  malformed-UTF-8 and long-repeat corpus seeds.
- No unchecked casts across unrelated types.

## Input handling

- All external text (keystrokes, .mim keymaps, ac.json, dictionaries) is
  treated as untrusted: the JSON/mim scanners are hand-rolled, bounds-checked
  and fuzz-covered; no `system()`, no dynamic shell commands.
- Invalid UTF-8 never crashes: `SUBORNO_STATUS_INVALID_UTF8` /
  lenient-decode fallbacks are specified behaviour.

## File access

- Settings/dictionary paths come from the pinned search order; override paths
  only from explicit engine/test hooks. No path is taken from network input.
- File writes go through the atomic-file module (`src/persistence/`) —
  no partially-written settings.

## DLL & COM safety

- `DllMain` does no complex work; module lifetime is a refcount lock
  (`module_lifetime.hpp`).
- Every COM object keeps a valid refcount model (class factory, TSF sinks);
  verified in the Windows tests and by the release build audit
  (`audit-windows-dependencies.ps1` checks DLL dependencies).
- No dynamic loads beyond the documented Windows SDK set.

## Supply chain

- vcpkg baseline is pinned to a specific commit; Inno Setup download is
  SHA256-pinned in `release.yml`.
- Dependabot watches the manifest; license/attribution obligations are tracked
  in `LICENSE` and `THIRD_PARTY_NOTICES.md`.

## Reported issues

Security-relevant findings (buffer overflow, integer overflow, race, resource
leak, invalid Unicode handling) are fixed only when proven and reproducible —
no speculative "hardening" that changes behaviour — and they arrive with a
regression test.
