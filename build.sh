#!/usr/bin/env bash
set -euo pipefail

cmake --preset app-debug
cmake --build --preset app-debug
