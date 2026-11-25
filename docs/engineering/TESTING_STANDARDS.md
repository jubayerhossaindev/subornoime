# Testing Standards

GoogleTest 1.15 through CTest is the single test framework (existing choice,
kept). Tests are part of the definition of done: behaviour without a test is
unverified behaviour.

## Layout

```text
tests/unit/           unit tests per module (<unit>_test.cpp)
tests/integration/    cross-target integration tests
tests/fuzz/           libFuzzer targets + seed corpus + dictionary
tests/windows/        Windows-only build/runtime smoke tests
```

## Gates

1. **Naming gate** (CI `naming-and-format`): naming validation, format check,
   layering check — seconds before builds start.
2. **Build + test matrix**: `ctest --preset ubuntu-ci` (Clang),
   `ubuntu-gcc-ci` (GCC 13), `windows-msvc-release` (MSVC v143) — unit and
   integration suites on every push.
3. **Sanitizer runs**: ASan + UBSan builds (`SUBORNO_ENABLE_SANITIZERS=ON`).
4. **Fuzzing**: phonetic-pipeline fuzz target with the committed corpus;
   regression-triggered runs (`fuzz.yml`).
5. **Parity regression**: `tests/unit/borno_compat_parity_test.cpp` pins the
   compatibility engines byte-for-byte against the reference output goldens
   (836-input corpus basis) — renaming or refactoring engine internals must
   keep this suite green, and it did during the naming migration.
6. **Unicode/layout verification**: BDS conformance tests, layout typing
   regression tests, and the evidence CSVs under `reports/` stay the record of
   what was verified on which date.

## Writing tests

- One behaviour per `TEST`/`TEST_F`; name = `<Unit>Test, <Behaviour>`.
- Arrange–Act–Assert with meaningful variable names (naming standard applies
  to tests too — CI enforces it).
- Fixtures only when setup is genuinely shared; no global test state.
- Tests must not depend on execution order or external network.
- A bug fix arrives with the regression test that failed first (see BUG-05
  display-name test in `engine_test.cpp`).

## Running

```bash
cmake --preset ubuntu-dev
cmake --build --preset ubuntu-dev --target suborno_unit_tests suborno_integration_tests
ctest --preset ubuntu-dev --output-on-failure
```
