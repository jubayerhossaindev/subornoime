# Borno Comment Migration Report

Scope: source-code comments and implementation notes in every C/C++ file,
CMake file, script and license text of the repository. Mandate reference:
engineering instruction §10 (Borno-related comment removal) and §10.4
(legal exception).

## Method

1. Case-insensitive scan of every code file (`borno_refined_audit.py` approach:
   comment-only extraction, excluding the project's own `suborno` namespace
   words).
2. Manual classification of each hit into:
   - **KEEP — Legal/provenance**: copyright, license and attribution lines
     (mandate §10.4 forbids removing required attribution).
   - **KEEP — Identifier**: comments that reference in-repo identifiers
     (`// namespace borno_compat`, header guard `SUBORNO_BORNO_PHONETIC_ENGINE_HPP`,
     layout profile ids like `borno-phonetic`). These name shipped features.
   - **REWRITE**: prose comments that explained the implementation by framing
     it around the reference project.
3. Each rewrite was written from the actual code, keeping the technical fact
   and dropping the old-project framing. No behaviour, invented fact or
   imagined design was introduced.

## Inventory (final audit state)

| Category | Count | Disposition |
|---|---|---|
| Total raw hits before migration | 49 (47 real + license path) | — |
| Legal/provenance attribution (kept verbatim) | 4 | `avro_phonetic_layout.hpp`, `borno_phonetic_layout.hpp`, `khipro_layout.cpp`, `borno_phonetic_engine.hpp` provenance lines |
| In-repo parity-suite references (kept, re-pointed) | 5 | now cite `tests/unit/borno_compat_parity_test.cpp` |
| Identifier references (kept) | 13 | namespace closers, header guards, layout ids |
| Rewritten prose comments | ~24 blocks across 19 files | see list below |

## Rewritten locations

| File | What changed |
|---|---|
| `include/suborno/layouts/phonetic/borno_phonetic_layout.hpp` | "exactly BornoTSF" framing → "replicates the published 'Borno Phonetic' reference" |
| `include/suborno/platform/windows/app/osk_window.hpp` | design reference re-pointed to `docs/design/SUBORNO_REFERENCE_DESIGN_SPEC.md` |
| `include/suborno/platform/windows/app/ui_theme.hpp` | "Borno-reference palette" → "reference palette" (spec doc reference kept) |
| `include/suborno/settings/settings.hpp` | two switch-documentation blocks rewritten around the settings store |
| `src/core/engine.cpp` | engine/settings sync comment rewritten Suborno-centric |
| `src/layouts/borno_compat/avro_phonetic_engine.hpp` | parity harness path fixed, framing Suborno-centric |
| `src/layouts/borno_compat/borno_phonetic_engine.hpp` | same + "written fresh for Suborno" statement kept |
| `src/layouts/phonetic/avro_phonetic_layout.cpp` | candidate-semantics comment de-framed |
| `src/layouts/phonetic/phonetic_tokenizer.cpp` | "rri" convention comment now names the Avro and Borno layouts |
| `src/platform/windows/app/topbar.cpp` | "Borno feature F-01" → "feature F-01 from the UI audit findings" |
| `src/platform/windows/standalone/standalone_main.cpp` | competitor tool list de-named (except factual Avro/Bijoy context) |
| `src/platform/windows/standalone/typing_controller.hpp` | "Avro/Borno model" → "hook-and-inject model used by popular standalone Bangla IMEs" |
| `src/platform/windows/standalone/unicode_injector.hpp` | same framing fix |
| `tests/unit/avro_phonetic_layout_test.cpp` | "verbatim BornoTSF engine" (inaccurate) → "clean-room cppAvroPhonetic-behaviour engine" |
| `tests/unit/borno_compat_parity_test.cpp` | stale `scripts/borno_parity` path removed; provenance (reference commit 2a4c96f) kept |
| `tests/unit/borno_phonetic_layout_test.cpp` | two blocks rewritten; "verbatim" claim corrected |
| `tests/unit/layout_typing_regression_test.cpp` | "rri" convention comment de-framed |
| `scripts/layout_verify/khipro_probhat_engine_verify.cpp` | layout name phrased in full |
| `LICENSE` | stale harness path → in-repo regression suite path (attribution unchanged) |

In addition, the full-file comment rewrites of `borno_compat_mim_engine.{hpp,cpp}`,
`borno_compat_dictionary_loader.{hpp,cpp}` and `borno_compat_autocorrect.hpp`
were done together with their naming refactor (Doxygen added, provenance lines
preserved).

## Stale reference repair

`scripts/borno_parity` (a harness directory that no longer exists) was
referenced in 4 files. All four now point to the in-repo parity regression
suite `tests/unit/borno_compat_parity_test.cpp`, which is what actually pins
the byte-identical outputs today.

## What was deliberately kept

1. **GPL attribution** — `SPDX-License-Identifier: GPL-3.0-or-later`,
   "Behaviour provenance: … (GPL-3, (c) Jayed Ahsan Saad and BornoTSF
   contributors)", the cppAvroPhonetic attribution (© 2014-2015 Muhammad
   Mominul Huque) and the Khipro keymap data attribution. Removing these would
   violate the license and mandate §10.4.
2. **Feature identifiers** — the `borno-phonetic` layout, the
   `src/layouts/borno_compat` module and its engines are shipped features
   (like `avro-phonetic`, `probhat`, `khipro`); deleting or renaming them would
   break public behaviour and tests (mandate §24.1/§24.5).
3. **Runtime display names** — regression tests lock the registered layout
   display names (e.g. `বাংলা (Borno)`) byte-exactly; these are product
   strings, not comments.

## Final audit result

A fresh case-insensitive scan finds 41 remaining hits, all in the three keep
categories above (4 legal + 5 parity-suite pointers + 32 identifier/feature
references). No old-project implementation explanation remains in any comment.
