# Sanitizer Configuration

**Status:** Active. Records what is wired, how to run it, and what each tool
cannot see (§22.4 honesty rule).

## AddressSanitizer + UBSan (mandatory for CI test runs)

### MSVC (Windows)

```bash
cmake --preset windows-msvc -D SUBORNO_ENABLE_SANITIZERS=ON
cmake --build --preset windows-msvc-debug
ctest --preset windows-msvc-debug --output-on-failure
```

- `/fsanitize=address` per target (`cmake/modules/SubornoBuildOptions.cmake`);
  `/INCREMENTAL:NO` is forced at link time as MSVC ASan requires.
- Verifies heap/stack overflow, use-after-free, double-free, alloc/dealloc
  mismatch on the real platform binaries.

### GCC / Clang (Linux, used by CI and local verification)

```bash
cmake -S . -B build/sanitizers -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTING=ON -DSUBORNO_ENABLE_SANITIZERS=ON \
  -DSUBORNO_ENABLE_MSVC_ANALYSIS=OFF \
  -DCMAKE_PREFIX_PATH=<gtest-install>
cmake --build build/sanitizers
cd build/sanitizers
ASAN_OPTIONS=detect_leaks=1:strict_string_checks=1 \
UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 ctest --output-on-failure
```

- Flags: `-fsanitize=address,undefined -fno-omit-frame-pointer`.
- **Recorded run (this refactoring):** 3/3 suites pass — 652 unit tests,
  integration tests, portable fuzz smoke (1105 runs) — with leak detection and
  strict string checks enabled.
- Clang preset: `ubuntu-sanitizers-clang` (clang++-23) in CI.

## Windows-specific diagnostics (§22.2, evaluated)

| Tool | Use | Status |
| --- | --- | --- |
| Application Verifier | full-page heap + handle checks on the installer/TSF DLL | recommended manual run before each release |
| PageHeap (full) | targeted at `suborno_windows_tsf.dll` load paths | documented; not automatable in CI |
| Debug Diagnostics | leak/crash analysis on minidumps | on-demand |
| MSVC debug heap | `_CrtDbgReport`-class checks in Debug builds | active via `/sdl` + CRT defaults |

These require an interactive Windows environment; CI covers the automated
subset. They are evaluated per release, not silently claimed as running.

## Additional tools (§22.3)

- **Cppcheck** — CI stage `cppcheck` (`scripts/cppcheck-ubuntu.sh`), local
  install: `sudo apt-get install cppcheck`.
- **Clang Static Analyzer** — covered through clang-tidy's
  `clang-analyzer-*` checks (`.clang-tidy`) in the Clang CI job.
- **Dr. Memory / memory profilers** — evaluated on demand; not part of CI.

## Limitations (explicit, per §22.4)

- A passing sanitizer run proves only that the executed paths were clean under
  that tool. Untested inputs, timing-dependent paths and non-instrumented
  third-party code remain outside the proof.
- ASan does not detect logic errors, uninitialized *stack* reads on all
  platforms, or data races (ThreadSanitizer is evaluated for the Linux core;
  the UI/TSF layer is single-threaded by design — `CONCURRENCY_SAFETY.md`).
- MSVC ASan requires `/INCREMENTAL:NO` and does not instrument every
  third-party binary; coverage per binary is checked when the option is
  enabled.
- Full PageHeap cannot run in headless CI; it is a documented manual step.

## New findings policy

A new sanitizer finding blocks merge (`ci.yml` rules). Suppressions need a
written justification at the site plus an entry in the findings register.
