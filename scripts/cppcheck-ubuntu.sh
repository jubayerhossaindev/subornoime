#!/usr/bin/env bash
# Cppcheck static analysis stage (engineering standard section 21).
#
# Runs cppcheck over project-owned sources only (src/ and include/). Third-
# party and generated trees are excluded; unused-function reports are kept
# informational because several small API surfaces exist for examples/tests.
set -euo pipefail

readonly PROJECT_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"

command -v cppcheck >/dev/null 2>&1 || {
    printf 'ERROR: cppcheck is required (sudo apt-get install cppcheck).\n' >&2
    exit 1
}

cd "$PROJECT_ROOT"

# --inline-suppr honors documented inline suppressions; every suppression in
# the tree carries a justification comment per the engineering standard.
cppcheck \
    --inline-suppr \
    --enable=warning,style,performance,portability \
    --error-exitcode=2 \
    --std=c++20 \
    --language=c++ \
    --platform=unix64 \
    -I include \
    --suppress='*:build/*' \
    --suppress=unusedFunction \
    --suppress='missingIncludeSystem' \
    # Cppcheck on Ubuntu has no Windows SDK/COM macro environment; Windows
    # sources are still scanned, while compiler/analyzer validation runs on
    # the native MSVC Windows verification job.
    --suppress=unknownMacro \
    src include tests/fuzz 2>&1 | tee /tmp/cppcheck-report.txt

printf 'cppcheck: clean (report saved to /tmp/cppcheck-report.txt)\n'
