#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

echo "=== Ensuring test fixtures are generated ==="
python3 "${ROOT_DIR}/tests/fixtures/generate_fixtures.py"

echo "=== Building targets ==="
cmake --build "${BUILD_DIR}" -j"$(nproc)"

echo "=== Running OCR-DI Test Suite ==="
ctest --test-dir "${BUILD_DIR}" --output-on-failure
