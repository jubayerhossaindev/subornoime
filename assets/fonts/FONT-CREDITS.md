# Bundled Bangla Fonts — Credits & Provenance

Vendor date: 2026-11-25. Fetched and QA-verified by the SUBORNO project build
process (scripts in the delivery toolchain; see fonts_manifest.csv for the
byte-exact SHA-256 of every file in this directory).

## What is bundled and why

The standalone host (SubornoStandalone.exe) renders its floating preview in a
bundled OFL face instead of relying on whatever the host machine has, so the
"install -> run -> type Bangla anywhere" promise also covers how Suborno
looks. Nothing here is required for typing to work: if every bundled file
went missing, the preview falls back to Nirmala UI (ships with Windows 8+)
and Bengali still renders.

| Family | Files | Version | Foundry | License |
| --- | --- | --- | --- | --- |
| Hind Siliguri | HindSiliguri-Light/Regular/Medium/SemiBold/Bold.ttf | v14 (Google Fonts served build) | Indian Type Foundry | SIL Open Font License 1.1 (OFL-HindSiliguri.txt) |
| Baloo Da 2 | BalooDa2-Regular/SemiBold/Bold.ttf | v26 (Google Fonts served build) | Ek Type | SIL Open Font License 1.1 (OFL-BalooDa2.txt) |
| Noto Sans Bengali | NotoSansBengali-Regular/Medium.ttf | notofonts/noto-sans-bengali @ main, 2026-11-25 | Google / Noto project | SIL Open Font License 1.1 (OFL-NotoSansBengali.txt) |

Download sources (reproducible):

- Hind Siliguri, Baloo Da 2: `https://fonts.googleapis.com/css2?family=...`
  (plain-UA request returns the static TTFs served from
  `fonts.gstatic.com/s/hindsiliguri/v14/...` and
  `fonts.gstatic.com/s/balooda2/v26/...`).
- Noto Sans Bengali:
  `https://cdn.jsdelivr.net/gh/notofonts/noto-sans-bengali@main/unhinted/ttf/...`
  (CDN mirror of the official notofonts GitHub repository).

## Role of each family

- **Hind Siliguri** — primary UI face. The standalone floating preview uses
  `Hind Siliguri Medium` (22 px ClearType on the dark token), Regular as the
  safe fallback. After `install_fonts.cmd` (per-user font install) it is also
  selectable for the TSF candidate list via `settings.ui.font_family`.
- **Baloo Da 2** — brand/display face (About, future headings). Not wired
  into the preview policy.
- **Noto Sans Bengali** — reference-coverage fallback; candidate for the TSF
  candidate list options in a future iteration.

## QA performed at vendor time (2026-11-25, fonttools)

- TTF magic + full parse: all 10 files OK.
- Bengali block cmap coverage: Hind Siliguri 92/128, Noto Sans Bengali 93/128,
  Baloo Da 2 88/128 codepoints of U+0980-U+09FF.
- Required set (vowels, consonants, matras, digits 0-9, nukta, danda,
  chandrabindu, hasanta): present in Hind Siliguri and Noto Sans Bengali.
- U+09E7 (Bengali digit one, the 2023 `google/fonts` issue #6037 malformed
  glyph): verified FIXED in the served v14 build (`bnOne` glyph present with
  contours in all five weights).
- Known gap: Baloo Da 2 lacks U+0980 (Bengali Anji) — a rare mark; acceptable
  for a display face. No other required codepoint is missing.
- Family name tables confirmed for GDI matching: `Hind Siliguri`,
  `Hind Siliguri Light/Medium/SemiBold` (non-RIBBI), `Baloo Da 2`,
  `Baloo Da 2 SemiBold` (non-RIBBI), `Noto Sans Bengali`,
  `Noto Sans Bengali Med`.

## License obligations (SIL OFL 1.1)

- The license text of every family ships beside the fonts (OFL-*.txt).
- The fonts are unmodified; no subsetting, renaming, or conversion.
- The fonts are distributed free as part of the application; they are not
  sold separately and no separate fee is charged for them.
- Reserved Font Names are not used to name any modified derivative (none
  exists).

## Integrity

`fonts_manifest.csv` lists `file,bytes,sha256` for all 10 TTFs. The zero-dep
acceptance gate (`verify_zero_dep.cmd`, GATE 8) checks that this directory
contains the 10 TTFs, the three OFL texts, this credits file and the
manifest — so a partially copied font set cannot slip into a release.
