# reports/layout-verification — Seven Layout Typing Verification Evidence Pack

**Execution**: 2026-11-23 17:51Z run (Asia/Dhaka 2026-11-23 23:51) · fresh run from HEAD `a241477` (branch `main`) · GCC 14.2 engine-layer (MSVC/Windows absent — engine evidence only).

## Reproduce

```bash
# 1. build (must be a no-op against HEAD)
ninja -C build/ubuntu-dev
# 2. full gtest suite (557 unit + 2 integration)
ctest --preset ubuntu-dev
# 3. corpus/inventory/probe harness
bash /home/z/my-project/scripts/rerun_layout_verify.sh
# 4. brief + convention + digits + nukta probes
#    sources: /home/z/my-project/scripts/layout_verify/{brief,convention,digits_punct,nukta}_probe.cpp
```

## Contents

- `MASTER_LAYOUT_STATUS_REPORT.md` — বাংলা master report (executive summary, master table, verdicts, Q&A, executive statement)
- `MASTER_LAYOUT_STATUS_MATRIX.csv` — brief §16 master table (CSV)
- `CHARACTER_CAPABILITY_MATRIX.csv` — 67-item inventory + extra capability rows × 6 layouts (engine-verified keys)
- `EVIDENCE_INDEX.csv` — evidence file index with SHA-256 (first 16 hex)
- `layouts/<id>/` — per-layout REPORT.md + engine-results.csv + inventory-results.csv + regression-results.csv + brief-results.csv + windows-results.csv + findings.md
- `summary/` — execution-summary.json, limitations.json, verification-status.json
- `evidence-raw/` — fresh-run artifacts (verbatim)

## Rules honored

- Five states: PASS / FAIL / BLOCKED / NOT TESTED / NOT APPLICABLE — BLOCKED never counts as PASS.
- Engine PASS never claims Windows PASS; all Windows items are BLOCKED with blocker reasons and ready harnesses.
- English passthrough is verified separately (identity), Bangla corpus = NOT APPLICABLE for it.
- Khipro trailing-virama limitation is disclosed, classified as intentional design limitation (not a bug).
