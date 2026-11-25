#!/usr/bin/env python3
"""Merge a Windows UI-audit results JSON into the traceability matrix.

Consumes the envelope JSON emitted by
`scripts/windows-ui-audit/Invoke-SubornoUiAudit.ps1`
(`{environment, screenshots, results:[{check_id, expected, actual, status, screenshot}]}`)
plus `scripts/windows-ui-audit/functional/checkmap.json`
(orchestrator check-id -> matrix test-id), and updates
`docs/reports/UI_FUNCTIONAL_AUDIT_TRACEABILITY_MATRIX.csv`.

Contract (matches scripts/windows-ui-audit/README.md):
  * appends `[<label> <date>] <PASS|FAIL>: <actual>` to the matched row's
    `evidence_or_blocker` column; the previous status is preserved in the note
    for auditability;
  * a FAIL result flips the row status to FAIL (visible regression signal);
  * a PASS result only upgrades the row status when the result carries
    `verification_level` in {runtime-executed, engine-executed} — an
    unevidenced PASS is recorded in the note but never upgrades the status;
  * rows whose status is `NOT APPLICABLE` are never touched;
  * results that match no matrix row are reported (and written to the counts
    file) — evidence is never silently dropped.

Usage:
  python3 scripts/layout_verify/merge_audit_results.py \
      --results suborno-ui-audit-results.json \
      --map scripts/windows-ui-audit/functional/checkmap.json \
      --label "Windows run" \
      --counts matrix_counts_after_windows_run.json \
      [--matrix docs/reports/UI_FUNCTIONAL_AUDIT_TRACEABILITY_MATRIX.csv] \
      [--dry-run]
"""
from __future__ import annotations

import argparse
import csv
import json
import sys
from datetime import date
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
DEFAULT_MATRIX = REPO / "docs/reports/UI_FUNCTIONAL_AUDIT_TRACEABILITY_MATRIX.csv"
DEFAULT_MAP = REPO / "scripts/windows-ui-audit/functional/checkmap.json"
ALLOWED_UPGRADE_LEVELS = {"runtime-executed", "engine-executed"}
UNTOUCHABLE = {"NOT APPLICABLE"}


def load_results(path: Path) -> list[dict]:
    envelope = json.loads(path.read_text(encoding="utf-8-sig"))
    results = envelope.get("results") if isinstance(envelope, dict) else envelope
    if not isinstance(results, list):
        raise SystemExit(f"error: {path} carries no results list")
    return results


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--results", required=True, type=Path)
    parser.add_argument("--map", dest="map_path", default=DEFAULT_MAP, type=Path)
    parser.add_argument("--label", default="Windows run")
    parser.add_argument("--counts", type=Path, default=None)
    parser.add_argument("--matrix", type=Path, default=DEFAULT_MATRIX)
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    results = load_results(args.results)
    mapping = json.loads(args.map_path.read_text(encoding="utf-8"))["map"]

    with args.matrix.open(encoding="utf-8-sig", newline="") as fh:
        reader = csv.DictReader(fh)
        fieldnames = list(reader.fieldnames or [])
        rows = list(reader)
    by_id = {row["test_id"]: row for row in rows}

    unknown_targets = sorted(
        {tid for targets in mapping.values() for tid in targets if tid not in by_id}
    )
    if unknown_targets:
        print(f"error: checkmap targets unknown matrix rows: {unknown_targets}")
        return 2

    today = date.today().isoformat()
    touched: dict[str, dict] = {}
    unmatched: list[str] = []
    rejected_pass: list[str] = []
    flipped: list[str] = []

    for result in results:
        check_id = str(result.get("check_id", ""))
        status = str(result.get("status", "")).upper()
        actual = str(result.get("actual", "")).replace("\n", " ").strip()
        level = str(result.get("verification_level", "")).strip().lower()
        targets = mapping.get(check_id)
        if not targets:
            unmatched.append(check_id)
            continue
        if status == "PASS" and level not in ALLOWED_UPGRADE_LEVELS:
            # Record the note, refuse the status upgrade.
            rejected_pass.append(check_id)
            upgrade = False
        else:
            upgrade = True
        for tid in targets:
            row = by_id[tid]
            if row["status"] in UNTOUCHABLE:
                continue
            previous = row["status"]
            note = f"[{args.label} {today}] {status}: {actual or '(no actual recorded)'}"
            if status == "FAIL" and previous != "FAIL":
                note += f" (was {previous})"
                row["status"] = "FAIL"
                flipped.append(tid)
            elif status == "PASS" and upgrade and previous != "PASS":
                note += f" (upgraded from {previous}, verification_level={level})"
                row["status"] = "PASS"
                flipped.append(tid)
            elif status == "PASS" and not upgrade:
                note += " (PASS recorded; status NOT upgraded - no verification_level)"
            entry = touched.setdefault(tid, {"row": row, "notes": []})
            entry["notes"].append(note)
            row["evidence_or_blocker"] = (
                row["evidence_or_blocker"].rstrip() + " | " + note
            ).strip(" |")

    print(f"results: {len(results)}  mapped rows touched: {len(touched)}")
    print(f"status flips: {len(flipped)} {sorted(set(flipped))}")
    if rejected_pass:
        print(f"PASS without verification_level (status NOT upgraded): {sorted(set(rejected_pass))}")
    if unmatched:
        print(f"UNMATCHED check ids (not in checkmap; investigate): {sorted(set(unmatched))}")

    counts: dict[str, int] = {}
    for row in rows:
        counts[row["status"]] = counts.get(row["status"], 0) + 1
    print(f"matrix status counts after merge: {counts}")

    if args.counts:
        args.counts.write_text(
            json.dumps(
                {
                    "date": today,
                    "label": args.label,
                    "results_file": str(args.results),
                    "results_total": len(results),
                    "rows_touched": len(touched),
                    "status_flips": sorted(set(flipped)),
                    "unmatched_check_ids": sorted(set(unmatched)),
                    "rejected_pass_without_level": sorted(set(rejected_pass)),
                    "counts": counts,
                },
                indent=2,
            )
            + "\n",
            encoding="utf-8",
        )

    if args.dry_run:
        print("dry run: matrix NOT written")
        return 0
    with args.matrix.open("w", encoding="utf-8", newline="") as fh:
        writer = csv.DictWriter(fh, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)
    print(f"matrix written: {args.matrix}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
