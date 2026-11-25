#!/usr/bin/env bash
# Local Windows build helper (no CI usage). Forward slashes keep this usable
# from Git Bash; cmd.exe users should invoke cmake directly.
set -euo pipefail

cmake --build build/windows-msvc --config Debug --parallel 1
