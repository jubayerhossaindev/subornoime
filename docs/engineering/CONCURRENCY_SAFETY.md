# Concurrency Safety

**Status:** Active. Complements `MEMORY_SAFETY_POLICY.md`.

## Threading model

Suborno IME is a UI-thread-centric application:

- The TSF text service, the app windows and the candidate pipeline run on the
  UI thread; TSF guarantees apartment affinity for its callouts.
- Cross-thread state is limited to a small, explicitly synchronized set:
  module-lifetime counters (`ModuleLifetime` with `std::atomic` +
  `std::shared_mutex`-style discipline), UIA liveness tokens
  (`std::atomic` in `ui_accessibility.cpp`), and settings persistence guarded
  by mutexes.

## Mandatory checks (per change)

- Data races and race conditions: shared mutable state must be listed below.
- Deadlock and lock inversion: nested locks are avoided; lock order is
  documented where two locks can meet.
- Use-after-free across threads: no object may be destroyed while another
  thread can reach it (TSF sinks are released before teardown).
- Callback safety: callbacks run on the calling thread's stack (no deferred
  captures); `[&]`/`[=]` default captures are banned except the synchronous
  COM boundary wrapper.
- Thread shutdown: no detached threads exist; the process owns no background
  threads to join.

## Shared mutable state register

| State | Location | Synchronization |
| --- | --- | --- |
| Module object/lock counters | `com/module_lifetime.hpp` | `std::atomic` with acquire/release ordering, documented at each use |
| UIA provider liveness token | `ui_accessibility.cpp` | `std::atomic<bool>` |
| Settings file writes | `persistence/atomic_file.cpp` | `std::mutex` around the write + rename sequence |
| Dictionary bulk writes | `dictionary.cpp` | SQLite transaction semantics + rollback guard |
| Spell/dictionary caches | `spell/`, `dictionary/` | `std::mutex` / `std::shared_mutex` where shared |

## Rules

1. Lock acquisition order is fixed; no callback (COM or UIA) is invoked while
   holding a project mutex (avoids reentrancy deadlocks through TSF).
2. `std::recursive_mutex` is not used anywhere; keep it that way.
3. Atomic memory ordering beyond seq_cst carries a justification comment at
   the use site (`ModuleLifetime` documents acquire/release pairs).
4. Unsynchronized concurrent writes to the same non-atomic memory are banned —
   TSF data flows are funneled through the single UI thread.
5. Lock-free techniques are permitted only with a proof of correctness and a
   benchmark need; performance never buys unsafety (`MEMORY_SAFETY_POLICY.md`).

## Verification evidence

- clang-tidy `concurrency-*` checks active (`.clang-tidy`).
- TSF contract tests (`tests/windows/`) exercise the callback/teardown order on
  the MSVC CI job.
- Sanitizer runs (TSan evaluated for Linux cores; ASan/UBSan mandatory) —
  `SANITIZER_CONFIGURATION.md` records coverage and limitations.
