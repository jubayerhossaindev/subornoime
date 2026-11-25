# Part III — Benchmark Build and Execution

Mission §7. Scope: benchmark dependency discovery, target wiring, CTest
integration, and the on/off behavior of `SUBORNO_BUILD_BENCHMARKS`.

Log: `logs/benchmarks.log`.

## Dependency discovery — exact blocker

`google/benchmark` is **not present** in this sandbox:

- `find /usr /usr/local -name "benchmark-config.cmake"` → nothing
- `ldconfig -p | grep benchmark` → nothing
- No pip/conda equivalent exists for the C++ library; the sandbox has no
  apt/pypi network policy permitting a build-from-source download of the
  dependency.

## Defect found and fixed during this verification

The first configure attempt with `-DSUBORNO_BUILD_BENCHMARKS=ON` **failed
hard** instead of degrading gracefully:

```
CMake Error at CMakeLists.txt:90 (add_subdirectory):
  The binary directory .../build/bench-attempt/benchmarks
  is already used to build a source directory.
```

Root cause: root `CMakeLists.txt` contained `add_subdirectory(benchmarks)`
**twice** (the v20 wave added a guarded block at line 63 while an older
guarded block at line 90 remained). The first (graceful) call returned early
without the dependency; the second call then collided on the same binary
directory. Default builds were unaffected (both blocks guarded by the same
option), but the option was unusable — contrary to the documented contract.

Fix applied in this mission: the duplicate block at the old line 90 was
removed; one guarded block remains. Re-verified:

```
$ cmake -S . -B build/bench-attempt -DSUBORNO_BUILD_BENCHMARKS=ON ...
-- google/benchmark not found — SUBORNO_BUILD_BENCHMARKS disabled
   (CI latency fences live in tests/unit/performance_budget_test.cpp)
-- Generating done (0.3s)        # exit 0 — graceful degradation
$ cmake -S . -B build/bench-off-check ...          # option OFF
-- Generating done                # normal builds unaffected
```

## Verification results

| Check | Result |
|---|---|
| Option ON without dependency → graceful status + exit 0 | **PASS** (after fix) |
| Option OFF default build unaffected | **PASS** (all Release/Debug builds in this mission used OFF) |
| Benchmark targets built & executed | **NOT POSSIBLE** — dependency absent (blocker above) |
| CTest integration of `suborno.benchmarks.engine` | **VERIFIED AT SOURCE LEVEL** — wired in `benchmarks/CMakeLists.txt` behind `if(BUILD_TESTING)`; only active when the target exists |

## Honest status

No benchmark numbers can be reported from this sandbox, and none are
fabricated. The latency evidence that IS available from execution lives in
`performance_budgets.md` (unit-test fences, 5 repeated runs). The
google/benchmark suites must be executed on the CI runner or a machine with
the dependency installed; `cmake -DSUBORNO_BUILD_BENCHMARKS=ON` now works
there and registers `suborno.benchmarks.engine` with ctest automatically.

`windows runtime` relevance: none — this option is platform-independent.
