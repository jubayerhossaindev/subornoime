# PRODUCTION READY — Fix Pass 2026-11-21

This pass applies the fixes from the deep repository audit (see the audit
report: 19-page Bangla document, `SubornoIME_Audit_Report.docx`) to the
source tree. Every change is listed below with its verification status.

## What was fixed (audit recommendation → change)

| ID | Fix | Files |
|----|-----|-------|
| R1 | **Spell checker wired into the candidate pipeline.** `CandidateEngine` now owns a `SpellChecker` (constructed when a dictionary is present) and emits spelling suggestions for complete-but-unknown words (no exact match, no prefix completion — mid-typing prefixes skip spell to stay cheap). Suggestions are bounded (≤ 3, ≤ 256 dictionary lookups inside the checker) and exception-guarded. | `src/candidates/candidate_engine.{hpp,cpp}`, `CMakeLists.txt` (suborno_candidates → suborno_spell) |
| R2 | **Candidate starvation + ranking fixed.** Staged pipeline: dictionary exact → prefix (capped at `limit − 2` so layout variants can never be pushed out) → spelling → layout alternatives (always appended while room remains). Scores stay monotonically decreasing across sources. | `src/candidates/candidate_engine.cpp` |
| R3 | **`m_contexts` lifecycle leak closed.** `OnUninitDocumentMgr` now enumerates the dying document manager's contexts and erases their map entries (previously a no-op: leaked AddRef'd `ITfContext*` and stale composition state). | `src/platform/windows/tsf/text_service.cpp` |
| R4 | **`TF_ES_SYNC` fallback.** Composition Update/Finish sessions retry asynchronously when the synchronous request fails (host busy) — no more stranded preedit on deactivate/pop. Composition Start and plain insertion stay synchronous (they need the result immediately). | `src/platform/windows/tsf/text_edit_session.{hpp,cpp}` |
| R5 | **Exception guards at COM boundaries.** `Finalize()`/`Abort()` and the candidate window procedure (`WM_PAINT`, `WM_LBUTTONUP`, `WM_DPICHANGED`) can no longer let a C++ exception escape into the host application. The spell stage of `make_candidates` is guarded too. | `src/platform/windows/tsf/candidate_list_ui_element.cpp`, `src/spell/spell_checker.cpp`, `src/candidates/candidate_engine.cpp` |
| R6 | **Settings reload change-detected.** `SettingsStore::file_stamp()` (size + mtime) + `Engine::reload_settings_if_changed()`; focus changes now cost one stat instead of a full file re-read + validation. | `include/suborno/settings/settings.hpp`, `src/settings/settings.cpp`, `include/suborno/core/engine.hpp`, `src/core/engine.cpp`, `src/platform/windows/tsf/text_service.cpp` |
| R7 | **DPI-aware candidate window.** Window metrics (row height, width, padding) scale with the window's DPI (`GetDpiForWindow` resolved dynamically, GDI fallback), `WM_DPICHANGED` re-scales live, and the popup now renders Bengali in a DPI-scaled **Nirmala UI** font instead of the default GUI font. | `src/platform/windows/tsf/candidate_list_ui_element.{hpp,cpp}` |
| R9 | **Dictionary seeding infrastructure + starter corpus.** New portable tool `suborno_dictionary_import <db.sqlite> <words.tsv>` and `data/seed_dictionary.tsv` (800 common Bengali words, synthetic ranking weights — provenance documented in the file header). Import once after install; replace with a real corpus-derived wordlist as it becomes available. | `src/tools/dictionary_import_main.cpp`, `data/seed_dictionary.tsv`, `CMakeLists.txt` |
| R10 | **CI: sanitizer + fuzz + system-deps jobs.** New `ubuntu-sanitizers` job (ASan/UBSan preset, build + ctest) and `linux-system-deps` job (builds without vcpkg, imports the seed dictionary and asserts ≥ 700 rows, 30-second libFuzzer smoke run). | `.github/workflows/ci.yml` |
| — | **Two real link bugs found during the fix pass.** (1) `src/unicode/bengali.cpp` uses ICU `BreakIterator` (grapheme segmentation) which lives in the **i18n** library, but only `ICU::uc` was linked — now `ICU::uc ICU::i18n`. (2) `LanguageIntelligence` (inside `suborno_candidates`) references `SpellChecker` symbols that were only linked into the unit-test binary — now `suborno_candidates` links `suborno_spell`. Also added a system ICU/SQLite fallback so the project builds without vcpkg (used by the new CI job and verified on Linux). | `CMakeLists.txt` |
| — | **Orphaned assets removed.** `born-images/` (18 unreferenced screenshots) deleted. | — |

## Deferred (documented in the audit, not in this pass)

- **R8** `ITfDisplayAttributeProvider` (preedit underline) — new COM surface; implement with a real Windows machine + host-app matrix test.
- **R11** Phonetic engine unification + external profile format — large refactor; do it behind golden-file equivalence tests.
- **R12** Authenticode signing — requires a certificate; `release.yml` already ships SHA256 checksums and labels the installer unsigned (prerelease).
- **Corpus expansion** beyond the 800-word starter (target: thousands, with measured frequencies and clear licensing).
- Runtime profiling (R-perf) — needs Windows + WPR/ETW; the benchmark targets are in place.

## Verification status

Verified in this sandbox (Linux, GCC 14.2, system ICU 76 + SQLite):

- Full build of all portable targets — clean, no warnings from project sources.
- `suborno_unit_tests`: **161/161 passed** (includes the three existing
  `CandidateEngineTest` cases, unchanged — the new pipeline preserves their
  contract: exact > prefix > layout tail, prefix predictions below exact).
- `suborno_integration_tests`: **2/2 passed**.
- `suborno_dictionary_import` + `data/seed_dictionary.tsv`: **800 words imported,
  0 skipped**; SQLite rows verified.
- Functional driver (standalone, not shipped): prefix flood no longer starves
  layout variants; unknown complete word receives spelling suggestions;
  exact match ranks first.

**Not verifiable here (requires Windows + MSVC v143):** compilation and
interactive behavior of the TSF DLL changes (R3, R4, R5, R7 are in
`src/platform/windows/*`). These edits are surgical and the files are exercised
by the Windows CI job (`windows-msvc` builds + runs `suborno_windows_tests`,
installer lifecycle test) — make sure that job is green before shipping.

## Round 2 (audit pass 2, same day) — fixes from the second deep sweep

A second full audit (parallel deep sweeps of the Windows surface and the
portable core, plus re-review of round-1 changes) produced these additional
fixes — all portable ones compile- and test-verified on Linux:

- **[HIGH] Shared-dictionary write transactions serialized.** Per-window
  sessions share one `Dictionary` (one SQLite connection); concurrent
  `record_commit` calls could hit "cannot start a transaction within a
  transaction" and one thread's ROLLBACK would abort another thread's commit.
  `Dictionary::Impl` now serializes multi-statement writes with a mutex
  (`src/dictionary/dictionary.cpp`), covered by a new 4-thread × 100-commit
  test.
- **[MEDIUM] Frequency clamping.** `Dictionary::add_entry` now clamps
  frequency to `[0, INT32_MAX]` (an out-of-range uint32 wrapped negative and
  permanently sank the word); the import tool clamps too.
- **[MEDIUM] Atomic-write durability.** The Windows temp file's
  `FILE_ATTRIBUTE_TEMPORARY` (which defers cache-manager flushing) is now
  cleared after the rename; on POSIX the parent directory is fsynced after
  `rename` so the rename itself survives power loss
  (`src/persistence/atomic_file.cpp`).
- **[MEDIUM] `snapshot()` no longer mutates session state from a const
  method** (data race + "selection sticks to last candidate when the list
  shrinks"); the clamped selection is computed locally in both
  `LayoutInputSession` and `PhoneticInputSession`.
- **[LOW] Dangling `string_view` from `Engine::active_layout_id()`** fixed by
  returning `std::string` (all call sites already copied).
- **[LOW] Bulk import in one transaction:** `Dictionary::begin_bulk_write() /
  commit_bulk_write() / rollback_bulk_write()` + the import tool wraps its
  loop (100k-row imports no longer cost one implicit commit per row).
- **[LOW] `words_for_prefix` index:** added
  `idx_dictionary_word_prefix (word COLLATE NOCASE)` so prefix lookups stop
  forcing a full table scan on large dictionaries.
- **[LOW] Candidate `stable_id` uses the NFC-normalized word** (consistency
  with round-1 ranking metadata).
- **[LOW] Candidate-window font at exactly 96 DPI** now still gets the
  Bengali (Nirmala UI) font; previously only created when the DPI differed
  from the 96 default.
- **[DOC] Seeding instructions corrected:** the engine reads
  `%APPDATA%\Suborno\dictionaries\user_candidates.db` (not the parent
  directory); `docs/development/windows.md` now mentions the CI artifact
  upload; `scripts/debug-build.sh` / `release-build.sh` rewritten with
  forward slashes (they previously contained cmd-only syntax).
- **Tests:** +4 (shared-dictionary concurrency, C API null-arg rejection,
  buffer-too-small, out-of-range candidate + commit-candidate). Suite now
  **165 unit + 2 integration tests, all passing**.

**Investigated and intentionally NOT changed (audit false positive):** the
second `logical_text_utf8` branch in `InputSession::append_key` — the test
`LayoutInputSessionTest.UsesLogicalUnicodeTextFromActiveKeyboardLayout`
proves the behavior is the intended feature (a non-empty logical text wins
even without the availability flag). A clarifying comment was added instead.

**Deferred to Windows verification (from round 2):** Inno Setup
`[UninstallRun]` vs file-deletion ordering for the TSF COM unregistration
(the CI uninstall assertion is the arbiter — if it is green, the current
ordering is correct); x86 installer branch (registry view in the test
script + no x86 build preset backing); spell-suggestion boost uses row count
rather than frequency (needs `find_matches` to return scores).

## Building / shipping

```sh
# Windows production build (MSVC v143, from a VS 2022 developer shell)
cmake --preset windows-msvc
cmake --build --preset windows-msvc
ctest --preset windows-msvc-release --output-on-failure

# Dictionary seeding (run once per machine, after install, per-user DB —
# this is the exact path the engine reads; see src/platform/windows/app_paths.cpp)
suborno_dictionary_import "%APPDATA%\Suborno\dictionaries\user_candidates.db" data\seed_dictionary.tsv
```

## Word-list & dictionary database optimization (round 3)

A dedicated audit of the word list + dictionary database produced a new
schema (v4), a much larger verified word pack, management tooling and a
million-scale benchmark. Highlights:

- **Schema v4** (auto-migrated from v3/v2/v1 on open, idempotent): adds
  `category` (word-group), `register` (sadhu/cholito/literary/colloquial/
  regional), `origin` (seed/corpus/manual) columns to `dictionary_entries`,
  plus `word_relations` (synonym/variant/antonym links) and
  `dictionary_changelog` (best-effort audit trail), with new indexes
  (`idx_entries_category`, `idx_entries_origin`, relations reverse index).
- **Performance**: hot lookups (`find_matches`, `find_prefix_matches`,
  `words_for_prefix`) reuse prepared statements and serve repeats through an
  in-memory LRU cache (512 entries, invalidated on every write). PRAGMA
  profile: WAL, `synchronous=NORMAL`, 8 MB page cache, temp_store=MEMORY,
  128 MB mmap. Management paths use per-call statements deliberately (a
  statement-reuse stale-snapshot behavior was observed on sqlite 3.53.1 and
  avoided where it is not keystroke-hot).
- **Word validation**: `Dictionary::is_valid_word` / `is_valid_normalized`
  reject non-Bangla, digit-bearing, spaced, overlong (>96 bytes) or malformed
  UTF-8 entries; `add_entry` enforces them and clamps frequencies.
- **Management tool** (`suborno_dictionary_import`): `import [--mode=replace|
  merge]` (merge keeps the higher frequency and only fills empty metadata),
  `export`, `relations`, `validate` (integrity report, exit 1 on problems),
  `stats`, `dedupe [--apply]`, `backup` (consistent online backup via the
  sqlite backup API), `vacuum`.
- **Word pack**: data/wordlist/*.tsv modules (function words, common nouns,
  verbs + inflected forms, adjectives, food/household, nature/body, numerals
  and time, conjunct-heavy, literary/sadhu, regional/colloquial, rare
  variants, synonym groups) validated and merged by
  scripts/validate_wordlist.py into data/seed_dictionary.tsv. The pack is
  manually compiled public-domain vocabulary; ranking weights are synthetic
  tiers. scripts/generate_clitics.py adds the fully regular clitic forms
  (টা/টি/টায়/গুলো/গুলি/গুলোর and consonant-final এ/এর). Pack totals:
  **9,527 unique words + 222 relations, 0 duplicates, validation clean**.
- **Benchmark** (`suborno_dictionary_bench <db> [count] [queries]`, synthetic
  stress data): 1M-word bulk import in ~37 s (≈27k words/s); exact lookup
  ~15 µs avg; prefix lookup ~59 µs avg; cached repeat ~20 µs (10,000-query
  samples, Linux/GCC build).
- **Quality pass (human review)**: a dedicated manual review of the whole
  pack (scripts/clean_wordlist.py) removed 28 non-words/junk entries and one
  offensive entry, fixed 4 misspellings (ছায়াপথ, শঙ্খচিল, হিমশিখর, মুক্তোর),
  corrected 25 semantically wrong / inflected-junk relation pairs and added
  69 missing relation-target words (16_relation_targets.tsv) so no
  word_relations row dangles. Policy: every new module gets a mandatory
  human review pass in addition to the mechanical validator.
- **Tests**: +7 (schema v4, categories/records, merge semantics, relations +
  backup round-trip, word validation, duplicate detection, cache
  invalidation). Suite: **172 unit + 2 integration tests, all passing**.
