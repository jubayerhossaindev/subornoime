# Windows Verification — Environment and Blocker Report

Date: 2026-11-25 (UTC). Sandbox: Linux 5.10.134, Debian, g++ 14.2.0,
2 cores, 3.9 GiB RAM.

## The GitHub execution blocker (exact evidence)

Log: `logs/github_blocker_evidence.log`. Verbatim facts:

1. **gh CLI is installed and working** (user-space tarball, no root needed):
   `gh version 2.105.0 (2026-11-17)` at
   `/home/z/my-project/tools/gh-cli/gh_2.105.0_linux_amd64/bin/gh`.
2. **No git remote is configured**:
   `git remote -v` → empty output.
3. **No GitHub authentication exists**:
   `gh auth status` →
   `You are not logged into any GitHub hosts. To log in, run: gh auth login`
   (`AUTH_EXIT=1`). `GH_TOKEN` and `GITHUB_TOKEN` are absent.
4. **The API is reachable but rate-limited for this shared IP** (the
   rate-limit response itself proves connectivity; unauthenticated limit is
   exhausted for 8.212.10.159 — authenticated requests would have a separate
   quota, but none exists here).
5. **Push cannot even resolve**: `git push --dry-run origin
   ci/suborno-v19-windows-verification` →
   `error: src refspec ... does not match any` / `failed to push some refs to
   'origin'` (`PUSH_EXIT=1`) — no remote, and (at probe time) no branch.

Consequences per mission §2 ("report the exact blocker rather than pretending
a push occurred"): no branch can be pushed, no workflow can be triggered, no
GitHub-hosted Windows runner can be reached. **Every GitHub-dependent result
in this tree is classified BLOCKED, never PASS.** The branch, workflow and
evidence are fully prepared locally (see `workflow_definition.md`).

Also unavailable in this sandbox: `pwsh` (PowerShell — the installer/UIA
scripts cannot even be syntax-validated locally), `doxygen`, `clang`
(gcc-only), root (`sudo` requires a password). Recorded where relevant.

## What this sandbox IS (executed)

- Full portable Linux verification pipeline (see Part III evidence:
  `reports/linux_verification/part_iii/`): Release/Debug/ASan+UBSan/TSan
  builds, 686-test unit suite, integration, fuzz smoke, soak, sanitizer runs.
- Source-level audits of every Windows artifact the mission lists: TSF COM
  service, boundary guard, installer `.iss` + 300-line PS lifecycle test,
  `.def`/exports, UIA accessibility provider, UI audit harness.
- The dedicated verification workflow, YAML-validated
  (`.github/workflows/suborno-windows-verification.yml`).
