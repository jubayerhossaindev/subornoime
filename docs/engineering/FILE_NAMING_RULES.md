# File & Directory Naming Rules

All internal files use consistent `snake_case`.

## Extensions

| Kind | Extension |
|---|---|
| C++ header | `.hpp` |
| C++ source | `.cpp` |
| C header | `.h` |
| C source | `.c` |
| Test | `<unit>_test.cpp` |
| Benchmark | `<unit>_benchmark.cpp` |
| Fuzz target | `<unit>_fuzz.cpp` |

## Header/source pairs

Each module keeps matching pairs:

```text
phonetic_parser.hpp        phonetic_parser.cpp
composition_engine.hpp     composition_engine.cpp
candidate_generator.hpp    candidate_generator.cpp
dictionary_repository.hpp  dictionary_repository.cpp
layout_registry.hpp        layout_registry.cpp
unicode_converter.hpp      unicode_converter.cpp
```

## Directory layout

```text
include/suborno/<module>/   public headers, one directory per module
src/<module>/               module implementations (own CMakeLists.txt)
src/platform/windows/       Windows-only adapters (TSF, COM, app shell)
tests/unit/                 unit tests (GoogleTest)
tests/integration/          cross-target integration tests
tests/fuzz/                 libFuzzer targets and seed corpus
benchmarks/                 Google Benchmark executables
scripts/                    developer and CI helper scripts
docs/engineering/           engineering standards and reports
```

## Rules

1. Every internal C++ file name must match `[a-z0-9_]+(\.h|\.hpp|\.cpp)`
   (plus `_test` / `_benchmark` suffixes). Enforced by
   `scripts/validate_naming.py`.
2. A header's name matches the primary type it declares
   (`candidate_engine.hpp` declares `CandidateEngine`).
3. Test files mirror the unit under test (`borno_phonetic_layout_test.cpp`).
4. Public headers under `include/` may only be renamed together with their
   include sites — they are part of the public API surface.
5. Generated files (build outputs, Doxygen HTML) never land in source
   directories; they live under the configured build tree.
