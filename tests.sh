#!/usr/bin/env bash
set -euo pipefail

cmake --preset tests-debug
cmake --build --preset tests-debug
ctest --preset tests-debug
