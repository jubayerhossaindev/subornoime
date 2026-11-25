# Third-Party Dependency Inventory

The project consumes dependencies through the pinned vcpkg baseline
`0df93a05b9de080129e653ec4776d7e14a1b2e4f` (2026-11-16). The versions below
are the port versions at that baseline, not a claim that those packages have
been built on every supported platform. `vcpkg.json` constraints and features
define what each platform resolves.

| Package | Baseline version | Purpose | License | Platform / classification | Official source and maintenance |
|---|---:|---|---|---|---|
| ICU4C (`icu`) | 79.1 | Unicode support in the portable production core | ICU License | Ubuntu and Windows; production runtime dependency | [Unicode ICU](https://icu.unicode.org/), [releases](https://github.com/unicode-org/icu/releases); actively released. Upstream stable 79.1 was released 2026-10-22 and matches the vcpkg port. |
| SQLite (`sqlite3`) | 3.54.1 | Storage support in the portable production core | Public Domain / SQLite blessing | Ubuntu and Windows; production runtime dependency | [SQLite](https://sqlite.org/), [copyright](https://www.sqlite.org/copyright.html); maintained with regular releases. |
| Microsoft WIL (`wil`) | 1.0.260126.7 | Windows resource and COM helper utilities | MIT | Windows only; production-side Windows dependency | [Microsoft WIL](https://github.com/microsoft/wil); maintained by Microsoft. Not linked into Ubuntu targets. |
| GoogleTest (`gtest`) | 1.18.0 | Unit and infrastructure tests | BSD-3-Clause | Ubuntu and Windows; development-only | [GoogleTest](https://github.com/google/googletest); actively maintained by Google. |
| Google Benchmark (`benchmark`) | 1.10.0 | Optional performance benchmark executable | Apache-2.0 | Ubuntu and Windows; development-only and opt-in | [Google Benchmark](https://github.com/google/benchmark); actively maintained by Google. |

The vcpkg baseline pins the registry snapshot, while manifest minimum-version
constraints prevent silently selecting a port older than the reviewed
inventory. The resolved package graph is recorded by vcpkg under the ignored
build directory; verify it after configuring a platform.

vcpkg, CMake, Ninja, LLVM tools, GitHub Actions, and Grype are build/security
tools rather than application runtime dependencies. The repository's
third-party GitHub Actions are referenced by version tag (for example
`actions/checkout@v7.0.1`) and kept up to date via Dependabot
(`.github/dependabot.yml`); SHA-pinning them to immutable commit SHAs is the
recommended next step and is still pending.

## Bundled Bangla fonts (SIL Open Font License 1.1)

`assets/fonts` ships three OFL 1.1 font families used for Suborno's own UI
faces (see `assets/fonts/FONT-CREDITS.md` for byte-exact provenance and the
per-family `OFL-*.txt` files for the full license text):

| Family | Foundry | Files | Use |
|---|---|---|---|
| Hind Siliguri | Indian Type Foundry | 5 weights (Light…Bold) | Standalone floating preview brand face (FR_PRIVATE, per-process) |
| Baloo Da 2 | Ek Type | 3 weights (Regular/SemiBold/Bold) | Brand/display surfaces |
| Noto Sans Bengali | Google / Noto project | 2 weights (Regular/Medium) | Reference-coverage fallback |

Distribution terms (SIL OFL 1.1): the fonts are unmodified, each license text
ships beside them, they are not sold separately, and the Reserved Font Names
of the original families are not applied to any derivative (none exists).

## Probhat keymap reference

The Probhat fixed physical-key output table is independently implemented from the MIT-licensed
princebillygk/probhat-layout-windows repository, layout.klc, upstream revision debc8310c60200143c6c0420433cca8506771e09.
Only the key-position, base, Shift, CapsLock (CAPLOK/SGCAPS), and AltGr (Ctrl+Alt)
output data are referenced; Suborno does not copy
the upstream installer or keyboard driver.

MIT License

Copyright (c) 2023 Prince Billy Graham Karmoker

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## License review

The listed licenses are permissive or public-domain-style. The ICU License
must be included when redistributing ICU, SQLite's source carries its public
domain dedication, and the MIT/BSD/Apache notices must be retained for their
respective dependencies. Preserve the license files/notices supplied by vcpkg
when packaging. This table is an inventory, not legal advice; re-check the
exact resolved port metadata before shipping binaries.
