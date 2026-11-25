# Khipro compositional keymap

## Mapping concept

Khipro is a compositional (auto-virama) phonetic-style layout, implemented as a
clean-room subset of the Khipro Classic compositional typing convention
(khipro.khiproteam.com). Consecutive consonants inside a run automatically
receive a virama joiner (U+09CD): `kt` → ক্ত, `Ngk` → ঙ্ক, `ksf` → ক্ষ.

## Special tokens

| Token | Output | Notes |
|---|---|---|
| `qq` | explicit hasanta (U+09CD) | After a consonant: appends the virama immediately, so the trailing/displaced virama ক্ (U+0995 U+09CD) is typeable at end of input (`kqq` → ক্). The next consonant completes the same join without a second virama (`kqqt` → ক্ত, `kqqtd` → ক্ত্দ). Standalone or after a vowel it is a silent state break, mirroring the National layout's bare-halant rule. Repeated `qq` is idempotent (`kqqqq` → ক্). The `/` slicer can undo an explicit hasanta exactly like an automatic one (`kqq/` → ক, `kqq/t` → ক্ত). Unlike real Khipro Classic's trailing variant (which appends ZWNJ), the output here is exactly U+0995 U+09CD with no U+200C. |
| `;` | partition / vowel break | Silent break key: `k;t` → কত, `m;i` → মই. `;;` → literal `;`. |
| `/` | slicer / chandrabindu | After a consonant with a pending virama: removes it (`ks/` → কস, `bakr/uddh` → বাকরুদ্ধ). After a consonant with no pending virama: silent ender (`k/` → ক). Otherwise: ঁ (`/` → ঁ, `ka/` → কাঁ). |
| `t/` | ৎ (khanda ta, U+09CE) | Dedicated mapping; longest match wins over the bare `/` handling. |
| `x` | ং (anusvara, U+0982) | |
| `oo` | ঃ (visarga, U+0983) | |
| `,,` | nukta (U+09BC) | Completes a consonant (য + ় = য়); run stays alive so the next consonant still auto-joins (`y,,z` → য়্য). Never doubled when the run tail already carries a nukta. |
| `.` | । (danda, U+0964) | `..` → literal `.`, `...` → …, `.f` → ॥. |
| `$` | ৳ (U+09F3) | `$$` → literal `$`, `$f` → ₹ (U+20B9). |
| `+f` / `-f` / `=f` | × / ÷ / ≠ | |
| `0`–`9` | ০–৯ (U+09E6–U+09EF) | Latin digits no longer pass through literally. |
| `Ng` | ঙ (U+0999) | |
| `rf` / `rff` | ড় / ঢ় | NFC-stable decomposed forms (U+09A1 U+09BC / U+09A2 U+09BC). |
| `y` | য় (U+09AF U+09BC) | Composition exclusion — stays decomposed under NFC. |
| `q` | ঋ (U+098B) / ৃ dependent | |
| `ae` / `wa` / `wi` / `wu` / `ii` / `uu` | compound vowels | Trailing `f` forces the split forms (`amiif` → আমিই). |
| Uppercase letters | literal passthrough | The layout is case-sensitive; uppercase Latin is never a token. |

## Explicit hasanta vs automatic virama

| Input | Output | Code points |
|---|---|---|
| `kt` | ক্ত | U+0995 U+09CD U+09A4 (automatic) |
| `kqqt` | ক্ত | U+0995 U+09CD U+09A4 (explicit, same result) |
| `kqq` | ক্ | U+0995 U+09CD (trailing virama) |
| `k;t` | কত | U+0995 U+09A4 (partition) |
| `ksqq` | ক্স্ | U+0995 U+09CD U+09B8 U+09CD (explicit virama after a realised pair) |

## Editing behaviour

- Backspace edits the raw Latin buffer one keystroke at a time
  (`kqq` → BS → `kq` → কৃ → BS → ক), identical to every other compositional
  layout editing operation.
- Commit transliterates the whole raw buffer and NFC-normalises; ক্ is
  NFC-stable, so the virama survives commit, copy/paste and serialization.
- A vowel typed right after `qq` appends its dependent form, exactly like the
  National layout's `X`-then-vowel edge case (deterministic, documented here).

## Token collision audit (2026-11-24 mission)

`qq` does not occur in any corpus word, inventory derivation, brief sequence
or existing test input (`q` appears only as `q` → ঋ and `kq` → কৃ). Existing
outputs are unchanged: corpus 49/49, brief sequences 14/14, inventory 74/74
after the fix. Source: `src/layouts/khipro/khipro_layout.cpp`.
