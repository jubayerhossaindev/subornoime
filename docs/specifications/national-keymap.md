# National (Jatiyo) fixed keymap

## Mapping source

The National layout is a fixed positional keyboard map, not a phonetic
transliterator. Its four Bangla layers per key follow the official
**National Keyboard of Bangladesh (BDS 1738:2018)** diagram as published in
public Unicode Consortium document **L2/19-079** (Figure 4, page 4, fetched
from unicode.org on 2026-11-24), with the layer legend of that diagram:

- **স্বাভাবিক বাংলা** — base layer, keycap bottom-right
- **শিফট বাংলা** — Shift layer, keycap top-right
- **লিংক বাংলা** — Link (AltGr) layer, keycap bottom-left
- **লিংক শিফট বাংলা** — Link+Shift layer, keycap top-left
- **ইংরেজি** — the English glyph printed at key centre

The **legacy reference IME** project (`layoutParsers/Layouts/
National.cpp`, GPL-3.0-or-later) was used as an independent factual
cross-reference for the same diagram; its README marks the National layout
"Tested (WIP)", so its mapping was **not** treated as an independently
verified compliance claim. SubornoIME's implementation is clean-room MIT:
only the factual key→character assignments were taken; **no GPL code, table
structure, or regex rules were copied**. See
`reports/standards/BDS_NATIONAL_LAYOUT_DISCREPANCY_REPORT.md` for the full
three-way comparison and IP review.

The clause-level equivalence to the BDS 1738:2018 official table **cannot be
claimed**: the authorized standard text is not obtained (GAP-01). The diagram
evidence is the official layout as republished by the Unicode Consortium
document, not the purchased standard.

## Layout identity

- Layout ID: `national` (registry name unchanged; seven-layout inventory
  unchanged).
- `LayoutKind::fixed` (was `phonetic` in the pre-2026-11-24 prototype).
- One key press emits exactly one mapped output (digit, letter, vowel sign,
  sign, or a two-code-point cluster such as ্র). No phonetic tokens exist any
  more; the old prototype tokens (`ami`, `X`, `rri`, `..`, `tw`, `^`) are
  removed.

## Base and Shift layers (letter keys)

| Key | Base | Shift | | Key | Base | Shift |
|---|---|---|---|---|---|---|
| q | ঙ | ং | | a | ৃ | র্ (reph, see sequences) |
| w | য | য় | | s | ু | ূ |
| e | ড | ঢ | | d | ি | ী |
| r | প | ফ | | f | া | অ |
| t | ট | ঠ | | g | ্ | । |
| y | চ | ছ | | h | ব | ভ |
| u | জ | ঝ | | j | ক | খ |
| i | হ | ঞ | | k | ত | থ |
| o | গ | ঘ | | l | দ | ধ |
| p | ড় | ঢ় | | | | |
| z | ্র | ্য | | v | র | ল |
| x | ও | ৗ | | b | ন | ণ |
| c | ে | ৈ | | n | স | ষ |
| | | | | m | ম | শ |

ড়/ঢ়/য় are emitted decomposed (consonant + U+09BC); they are Unicode
composition exclusions, so NFC keeps them decomposed end to end.

## National hallmark Shift positions

- **Shift+4 → ৳ (U+09F3)** — Taka sign.
- **Shift+7 → ঁ (U+0981)** — candrabindu.
- **`\` → ৎ (U+09CE)**, **Shift+`\` (|) → ঃ (U+0983)**.
- **Shift+`` ` `` (~) → “ (U+201C)**; **Shift+`'` (") → `` ` ``**.
- Digit row base: `1..0` → `১..০` (U+09E7..U+09EF, `0` → U+09E6).
- Shift+1/2/3/5/6/8/9/0 keep the English glyphs `! @ # % ^ * ( )`.
- `=`/`+`, `[`/`{`, `]`/`}`, `;`/`:`, `,`/`<`, `.`/`>`, `/`/`?` stay English
  (pass-through).

## Link (AltGr) layer

AltGr is delivered to the engine as Ctrl+Alt (standard Windows AltGr), the
same pipeline the Probhat layout uses.

| Keys | Output | | Keys | Output |
|---|---|---|---|---|
| Link+1 | ৴ (U+09F4) | | Link+f | আ (U+0986) |
| Link+2 | ৵ (U+09F5) | | Link+d | ই (U+0987) |
| Link+3 | ৶ (U+09F6) | | Link+Shift+D | ঈ (U+0988) |
| Link+4 | ৷ (U+09F7) | | Link+s | উ (U+0989) |
| Link+5 | ৸ (U+09F8) | | Link+Shift+S | ঊ (U+098A) |
| Link+0 | ৹ (U+09F9) | | Link+a | ঋ (U+098B) |
| Link+6 | ৠ (U+09E0) | | Link+c | এ (U+098F) |
| Link+7 | ৺ (U+09FA) | | Link+Shift+C | ঐ (U+0990) |
| Link+8 | ৡ (U+09E1) | | Link+x | ো (U+09CB) |
| Link+9 | ঌ (U+098C) | | Link+Shift+X | ঔ (U+0994) |
| Link+Shift+1 | ঀ (U+0980) | | Link+Shift+G | ॥ (U+0965) |
| Link+Shift+2 | (empty) | | Link+- | × (U+00D7) |
| Link+Shift+3 | ঽ (U+09BD) | | Link+Shift+- | ÷ (U+00F7) |
| Link+Shift+4 | ৲ (U+09F2) | | | |
| Link+Shift+5 | ৄ (U+09C4) | | | |
| Link+Shift+6 | ৠ (U+09E0) | | | |
| Link+Shift+7 | ৾ (U+09FC) | | | |
| Link+Shift+8 | ৣ (U+09E3) | | | |
| Link+Shift+9 | ৢ (U+09E2) | | | |
| Link+Shift+0 | ৽ (U+09FD) | | | |

The Link digit row 1..5,0 carries the Bengali currency
numerator/denominator series **৴ (U+09F4), ৵ (U+09F5), ৶ (U+09F6),
৷ (U+09F7), ৸ (U+09F8), ৹ (U+09F9)** in key order.

## Deliberate exclusions (Assamese letters)

The reference mapping and the official Link layer carry the Assamese letters
**ৰ (U+09F0)** at Link+`,` and **ৱ (U+09F1)** at Link+`.`. SubornoIME
deliberately leaves both positions empty: they are outside the project's
Bengali character inventory decision (user directive, 2026-11-24). The
exclusion is asserted by
`NationalLayoutTest.AssameseLinkCharactersAreExcluded`.

## Sequence behaviours (modern standard "Regular" typing style)

These are composition-buffer behaviours implemented in
`NationalInputSession` and in the string API, mirroring the reference's
documented Regular-mode facts:

1. **Reph (র্) via Shift+A**: after a consonant (or consonant cluster), র্ is
   placed **before** the trailing cluster: `জA` → র্ক, `ক ম A` → কর্ম,
   `ক ্ ক A` → র্ক্ক. Standalone Shift+A emits র্ raw. The trailing-cluster
   scan accepts the decomposed ড়/ঢ়/য় forms (consonant + nukta).
2. **Broken conjunct**: a second consecutive press of the hasanta key
   produces ্ + ZWNJ (U+200C): `ক ্ ্ ক` keys → ক্‌ক.
3. **Candrabindu/aa-kar canonical order**: া typed after ঁ reorders to াঁ:
   `ব ঁ া` keys → বাঁ.
4. **Invalid অ + া-kar**: া typed after Shift+F (অ) yields the letter আ
   (reference fact `Ff` → আ).
5. **Ra-kar / ya-phala reorder**: ্র or ্য typed directly after a dependent
   vowel sign (া ি ী ু ূ ৃ ে ৈ ো ৌ) swaps to canonical order: ক + ু + ্র →
   ক্রু.
6. **র‍্য ya-phala**: ্য typed directly after র emits র + ZWJ + ্ + য
   (reference fact `vZ`).
7. **o-kar / au-kar composition**: া after ে and ৗ after ে are composed by
   the final NFC normalization: ক + ে + া → কো (U+09CB), ক + ে + ৗ → কৌ
   (U+09CC).

## CapsLock semantics

The National layout has no caps-sensitive keys: CapsLock alone never selects
the Shift layer (`caps_flags = 0` on every key, matching the reference which
has no caps concept). Shift alone selects the second layer; AltGr layers are
unaffected by CapsLock.

## Physical-key policy

- Scan codes are the standard AT set 1 codes (0x02..0x39) used by the
  Probhat implementation.
- Ctrl alone, Alt alone, and the Windows key never inject text; AltGr
  (Ctrl+Alt together) is a text layer.
- `repeat_count` > 1 repeats the mapped output (including reph handling per
  repetition).
- Backspace removes one extended grapheme cluster; commit normalizes the
  composition with NFC.
- Invalid UTF-8 input via `append_utf8` returns `invalid_utf8` and the string
  API returns an empty output.

## Test coverage

- `tests/unit/national_layout_test.cpp` — 20 tests: layout kind, full
  base/shift letter tables, vowel-sign keys, hallmark Shift positions,
  Bengali digit row, shift-digit English pass-through, English-only keys,
  full Link layer (session KeyEvents), Assamese exclusion, reph (single,
  cluster, decomposed-ড়, standalone), broken conjunct, াঁ reorder,
  ra-kar/ya-phala reorder, র‍্য, NFC composition, `Ff`→আ, words (বাংলা,
  নমস্কার, আমি), CapsLock neutrality, backspace, commit, modifier policy,
  candidates, invalid UTF-8.
- `tests/unit/layout_typing_regression_test.cpp` — positional regression
  equivalents (conjuncts, previously-missing characters, reph, words).
- `tests/unit/bds_conformance_test.cpp` — National digit row U+09E6..U+09EF
  exact (unchanged test, still passing).
- `tests/unit/coverage_final_gaps_test.cpp` — pass-through and Bengali
  decode coverage.
- `scripts/layout_verify/national_positional_verify.cpp` — 121-row
  independent engine evidence (base/shift/Link/Link+Shift + sequences), all
  PASS, output committed as
  `reports/standards/evidence-raw/bds_national_positional_engine_rows.csv`.
