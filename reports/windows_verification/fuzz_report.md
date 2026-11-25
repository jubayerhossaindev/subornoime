# Fuzz and Sanitizer Report (Windows + Linux separation)

## Linux (executed in this session — Part III evidence)

- `suborno.fuzz_smoke` ctest: 17 seeds, corpus replay — PASS, exit 0
  (`reports/linux_verification/part_iii/logs/release_ctest.log`).
- ASan+UBSan instrumented Part III suites — see
  `part_iii/logs/asan_ubsan.log` and `regression_matrix.md`.
- TSan run of the concurrency-sensitive `DiagnosticsTest` — see
  `part_iii/logs/tsan.log`.
- Historical libFuzzer 30s smoke (CI `linux-system-deps` job) — not re-run
  here; Linux fuzz evidence cited from the repository's own CI config.

## Windows (BLOCKED — wired into the verification workflow)

- The portable fuzz driver `suborno_fuzz_smoke` (libFuzzer-independent:
  `tests/CMakeLists.txt:199`, links `suborno_core` only) builds with MSVC —
  no MSVC guard, no clang dependency. The workflow builds and executes it
  against `tests/fuzz/corpus` and captures
  `verify-evidence/fuzz_smoke_windows.log`; a nonzero exit fails the job.
- Corpus seeds: the same 17 versioned seeds replayed on Windows — results
  stay **separate** from the Linux numbers (mission §10).
- MSVC AddressSanitizer: no project target currently enables
  `/fsanitize=address`; the workflow does not claim ASan-on-Windows. Any
  future MSVC ASan configuration would be a separate, explicit build (not
  mixed into the production pipeline).
- TSan on Windows: not applicable (no MSVC TSan); concurrency coverage comes
  from the Linux TSan run + the OS-level tests on Windows.

## Claims made

None beyond the Linux evidence above. Windows fuzz results are BLOCKED until
the workflow executes.
