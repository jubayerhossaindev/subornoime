# Probhat fixed keymap

## Mapping source

The physical scan-code, base, Shift, and CapsLock output table is independently implemented
from the MIT-licensed princebillygk/probhat-layout-windows repository, layout.klc, upstream
revision debc8310c60200143c6c0420433cca8506771e09. Suborno does not copy the upstream driver or installer.
The upstream KLC was re-verified byte-level on 2026-11-24 (mission: standalone nukta).

Probhat is a fixed physical keyboard map, not a phonetic transliterator. Vowel signs stay
vowel signs on their mapped keys; they are not rewritten based on a preceding consonant.
For example, base A produces া, Shift+A produces অ, base W produces ূ, and Shift+W
produces ঊ.

## SubornoIME Probhat Compatibility Extension — standalone nukta

The upstream layout.klc contains **no U+09BC (Bengali sign nukta) anywhere**, and the
upstream AltGr column of scan 0x34 (OEM_PERIOD) is `-1` (empty). The upstream Probhat
layout therefore cannot type the standalone nukta ় at all — it only appears inside the
decomposed য় on key 0x2C, the decomposed ড় on Shift+0x13 and the precomposed ঢ় on
Shift+0x2D.

SubornoIME adds one documented extension (2026-11-24):

| Key | Output | Upstream KLC column |
|---|---|---|
| **AltGr (Ctrl+Alt) + 0x34** | **় (U+09BC, standalone nukta)** | `-1` (empty — extension slot, no existing key overwritten) |

Rationale for the slot choice (per the mission's priority order):

1. No unassigned physical key remains in the main block (every typeable key 0x02–0x35,
   0x39, 0x53, 0x56 carries an upstream mapping).
2. AltGr is a supported modifier layer (KLC value column 6; the implementation handles
   Ctrl+Alt as a text layer).
3. Scan 0x34 is the diacritic corner: its base layer is danda ।, its Shift layer is
   chandrabindu ঁ, and its neighbour 0x35 carries the virama ্ — the marks cluster stays
   together.

Behaviour of the extension output:

- ক + AltGr+0x34 → ক় = U+0995 U+09BC (decomposed; composition exclusions never compose
  under NFC, so this is byte-stable and distinct from any precomposed look-alike).
- ড + nukta → U+09A1 U+09BC; ঢ + nukta → U+09A2 U+09BC; য + nukta → U+09AF U+09BC —
  the dedicated Shift+0x13 / Shift+0x2D / 0x2C keys keep working unchanged.
- ক + ় is one grapheme cluster (Lo + Mn, ccc=7): one backspace removes the whole cluster.
- Typing virama before the nukta (ক + ্ + ়) is canonical-reordered at commit to
  ক ় ্ (U+0995 U+09BC U+09CD) — nukta ccc=7 sorts before virama ccc=9 per the Unicode
  Standard; the raw preedit keeps the typed order.
- Repeated presses concatenate (fixed-layout contract); Shift+AltGr+0x34 keeps the
  upstream empty column.

This extension is **not** part of the original Probhat specification and must not be
presented as upstream behaviour. Everything else in the table follows the upstream KLC
columns exactly.

## CapsLock semantics

The KLC CAP column is a bit field. Suborno preserves CAPLOK (0x01) and CAPLOKALTGR (0x04)
separately. Values 1 and 5 enable ordinary CapsLock-to-Shift behavior; value 4 applies only
with AltGr and must not change output when CapsLock is pressed alone. Holding Shift and
CapsLock together returns the base output for CAPLOK keys. AltGr and CAPLOKALTGR output
states are not implemented yet, so this is not complete KLC state parity.

## Physical-key policy

Windows fixed-layout input uses scan codes, not the active Windows logical character.
Ctrl/Alt/Windows shortcuts are not consumed. Unmapped numpad digits remain unmapped.
The KLC includes the numpad decimal key but does not define a numpad digit-to-Bengali layer.

The source KLC declares dead keys for certain Latin accent outputs. Suborno currently
uses literal visible output for those layers; full dead-key composition is not claimed.

## Test coverage

Unit tests cover base/Shift pairs, ordinary CAPLOK behavior, CAPLOKALTGR non-effect without
AltGr, literal vowel-sign concatenation, punctuation/currency, scan-code precedence,
unmapped numpad keys, and grapheme-safe backspace. AltGr/dead-key compatibility remains
incomplete. The 2026-11-24 mission adds the standalone-nukta extension tests
(`ProbhatLayoutTest.Probhat*Nukta*`, 8 tests) and the regression-campaign guard
(`LayoutTypingRegressionTest.ProbhatStandaloneNuktaIsTypeableViaScanCode`), all driven
through physical scan-code events — never through `transliterate()` alone.