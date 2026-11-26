#!/usr/bin/env bash
# Local Windows build + installer compile helper (no CI usage). Forward
# slashes keep this usable from Git Bash; cmd.exe users should invoke cmake
# and ISCC directly.
set -euo pipefail

cmake --build build/windows-msvc --config Release --parallel 1
cmake --build build/windows-msvc --config Release --target suborno_app --parallel 1

ISCC=""
for candidate in "/c/Program Files (x86)/Inno Setup 6/ISCC.exe" "/c/Program Files/Inno Setup 6/ISCC.exe"; do
  if [ -x "$candidate" ]; then
    ISCC="$candidate"
    break
  fi
done
if [ -z "$ISCC" ]; then
  ISCC="ISCC"
fi

"$ISCC" installer/windows/suborno_installer.iss
