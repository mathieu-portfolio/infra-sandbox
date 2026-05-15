#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PRESET="${PRESET:-tests-vcpkg-debug}"

cmake --preset "$PRESET" "$@"
cmake --build --preset "$PRESET"
ctest --preset "$PRESET"
