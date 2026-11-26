# Fuzz Testing

**Status:** Active. Fuzz smoke runs in CI on every push; deep campaigns run
nightly (`fuzz.yml`).

## Fuzzed components (§23 scope and current coverage)

| Component | Target | Status |
| --- | --- | --- |
| Phonetic parser / pipeline | `tests/fuzz/phonetic_fuzz.cpp` (`PhoneticInputSession`) | active (libFuzzer + smoke) |
| C API input / lifecycle | `tests/fuzz/c_api_fuzz.cpp` (`run_c_api_target`) | active (libFuzzer + smoke) |
| Unicode converter | exercised inside both targets (append/commit paths drive it with arbitrary bytes) | covered via pipeline |
| Dictionary parser | import tool is an offline, trusted-CLI path with transactional rollback; fuzzing it is evaluated, not yet a standing target | documented decision |
| Layout/config parsers | layout tables are compiled code; legacy-import parsing is bounded by `compat` validation | documented decision |

Adding a target = shared body in `tests/fuzz/fuzz_targets.hpp` + a libFuzzer
driver + a smoke hook; both run everywhere automatically.

## Architecture

- **Shared targets** — `tests/fuzz/fuzz_targets.hpp` holds
  `run_phonetic_target` and `run_c_api_target` plus
  `maximum_fuzz_input_size = 4096` (input limit mirroring the engine's
  composition limit). Keeping one definition guarantees the smoke test and the
  libFuzzer campaigns exercise identical code.
- **libFuzzer drivers** (Clang-only, `-fsanitize=fuzzer,address,undefined`):
  built when `SUBORNO_BUILD_FUZZING=ON`; nightly workflow runs both with
  `-dict=tests/fuzz/dictionaries/phonetic.dict` and a capped session.
- **Portable smoke driver** — `tests/fuzz/fuzz_smoke_main.cpp`: runs every seed
  plus 64 deterministic mutations (truncate / flip / repeat / high-byte splice)
  through all targets on *any* compiler (GCC, MSVC, Clang), under a POSIX
  `alarm` timeout (60s; CI job timeout covers Windows). Registered as CTest
  test `suborno.fuzz_smoke` (TIMEOUT 180).

## Corpus and regression discipline

- Seeds live in `tests/fuzz/corpus` (versioned): phonetic seeds plus
  adversarial ones (`0008_invalid_utf8_sequences`,
  `0009_surrogate_and_emoji_mix`, `0010_boundary_repeats`).
- Session limits: maximum input size enforced inside the targets; libFuzzer
  `-max_total_time` caps nightly length; corpus is versioned so runs are
  reproducible.
- **Any crash becomes a regression test**: minimize the input, add it to the
  corpus and (for deterministic reproducers) to the unit suite, fix, then
  re-run the campaign.

## Running locally

```bash
# Portable smoke (any compiler, always built)
ctest -R fuzz_smoke --output-on-failure

# Deep libFuzzer campaign (Clang)
cmake -S . -B build/fuzz -G Ninja -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_BUILD_TYPE=Release -DSUBORNO_BUILD_FUZZING=ON
cmake --build build/fuzz --target suborno_phonetic_fuzz suborno_c_api_fuzz
./build/fuzz/suborno_phonetic_fuzz tests/fuzz/corpus /tmp/fuzz-artifacts \
  -dict=tests/fuzz/dictionaries/phonetic.dict -max_total_time=300
```

## Current evidence

- Portable smoke in the sanitizer build: 17 seeds → 1105 runs, no crashes.
- CI stages: `linux-system-deps` (30s libFuzzer + portable smoke), nightly
  `fuzz.yml` (both targets, capped sessions, crash artifacts uploaded on
  failure).
