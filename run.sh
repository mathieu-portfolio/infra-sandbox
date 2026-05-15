#!/usr/bin/env bash
set -euo pipefail

cmake --preset app-debug
cmake --build --preset app-debug --target infra_sandbox

for candidate in \
  "build/app-debug/Debug/infra_sandbox.exe" \
  "build/app-debug/infra_sandbox.exe" \
  "build/app-debug/infra_sandbox"; do
  if [[ -x "$candidate" || -f "$candidate" ]]; then
    "$candidate"
    exit 0
  fi
done

echo "Could not find infra_sandbox executable after build." >&2
exit 1
