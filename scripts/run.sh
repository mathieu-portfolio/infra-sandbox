#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PRESET="${PRESET:-app-vcpkg-debug}"
CONFIG="${CONFIG:-Debug}"
BUILD_DIR="$ROOT_DIR/build/$PRESET"

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    cat <<EOF
Usage: PRESET=<preset> CONFIG=<config> ./scripts/run.sh [app args]

Builds the selected app preset, then runs infra_sandbox.

Environment:
  PRESET  CMake preset to build. Defaults to app-vcpkg-debug.
  CONFIG  Multi-config build configuration. Defaults to Debug.
EOF
    exit 0
fi

PRESET="$PRESET" "$ROOT_DIR/scripts/build.sh"

if [[ -x "$BUILD_DIR/$CONFIG/infra_sandbox.exe" ]]; then
    exec "$BUILD_DIR/$CONFIG/infra_sandbox.exe" "$@"
elif [[ -x "$BUILD_DIR/infra_sandbox" ]]; then
    exec "$BUILD_DIR/infra_sandbox" "$@"
elif [[ -x "$BUILD_DIR/infra_sandbox.exe" ]]; then
    exec "$BUILD_DIR/infra_sandbox.exe" "$@"
fi

echo "infra_sandbox executable was not found in $BUILD_DIR." >&2
echo "If you use vcpkg, run with: PRESET=app-vcpkg-debug ./scripts/run.sh" >&2
exit 1
