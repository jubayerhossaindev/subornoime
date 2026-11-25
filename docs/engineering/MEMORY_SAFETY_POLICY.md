# Memory Safety Policy

**Status:** Active — enforced by build options, CI gates and code review.
**Applies to:** every module, function, class, API and resource in this repository.

## Purpose

Memory safety is the highest engineering priority of Suborno IME, ranked above
correctness fixes, readability and performance. This document is the project-wide
policy; the specialized companions (`MEMORY_OWNERSHIP.md`,
`OBJECT_LIFETIME_RULES.md`, `POINTER_AND_REFERENCE_SAFETY.md`,
`BUFFER_AND_BOUNDS_SAFETY.md`, `SMART_POINTER_POLICY.md`,
`RESOURCE_MANAGEMENT.md`, `EXCEPTION_SAFETY.md`, `CONCURRENCY_SAFETY.md`)
turn each section into concrete rules with evidence.

## Priority order for every engineering decision

1. Memory safety and object lifetime safety.
2. Data integrity and correctness.
3. Undefined behavior prevention.
4. Thread safety and concurrency correctness.
5. Security and input validation.
6. Architecture integrity.
7. Human-friendly code readability.
8. Consistent naming convention.
9. Error handling and reliability.
10. Performance and resource efficiency.
11. Documentation and developer experience.

Performance work must never weaken memory safety or data integrity, and no
clever technique may make code harder to verify.

## Mandatory safety questions

Every object must be able to answer all ten questions below. A module where an
answer is unclear is not considered memory-safe.

1. Who created the object?
2. Who owns the memory?
3. When does its lifetime start?
4. When does its lifetime end?
5. Who may hold a reference to it?
6. May another component destroy it?
7. May another thread access it?
8. Can a callback run after the object is destroyed?
9. Can any pointer or view become invalid before the owner dies?
10. Who releases the resource if an exception fires?

## Enforcement mechanisms (all active)

| Mechanism | Artifact | Gate |
| --- | --- | --- |
| Compiler hardening | `/W4 /permissive- /sdl /GS /guard:cf` on MSVC (`cmake/modules/SubornoBuildOptions.cmake`) | every build |
| AddressSanitizer + UBSan | `SUBORNO_ENABLE_SANITIZERS=ON`, presets `ubuntu-sanitizers-clang` | CI `ubuntu-sanitizers` job |
| Static analysis | MSVC `/analyze`, clang-tidy (`.clang-tidy`), cppcheck (`scripts/cppcheck-ubuntu.sh`), CodeQL (CI `codeql` job) | CI |
| Fuzz testing | libFuzzer targets (`tests/fuzz/`), portable fuzz-smoke driver | CI + nightly `fuzz.yml` |
| Bounds & input validation | rules in `BUFFER_AND_BOUNDS_SAFETY.md`, `INPUT_VALIDATION.md` | code review + tests |
| Ownership audit | `MEMORY_OWNERSHIP.md` + findings register in `REFACTORING_REPORT.md` | per change |

## Current verification evidence

- Release build: 104/104 targets compile clean.
- Test suite: 652 unit tests + integration tests + fuzz smoke (1105 runs) pass.
- ASan + UBSan instrumented run (leak detection, strict string checks,
  `halt_on_error` UBSan): 3/3 suites pass.
- Findings register: see `REFACTORING_REPORT.md` (Final Memory Safety Assessment).

## Limitations

No tool proves a program memory-safe. Sanitizer success means the tested paths
ran clean under that tool; it does not cover untested inputs or timing. Tool
coverage and limitations are reported per tool in `SANITIZER_CONFIGURATION.md`.
