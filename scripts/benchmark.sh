#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
BENCHMARK_BIN="${BUILD_DIR}/bin/edmo_ocr_benchmark"
FIXTURE_PATH="${1:-${ROOT_DIR}/tests/fixtures/transcript-like.pdf}"
ITERATIONS="${2:-5}"

if [ ! -f "${BENCHMARK_BIN}" ]; then
    echo "Benchmark binary not found. Building project..."
    "${SCRIPT_DIR}/build.sh"
fi

if [ ! -f "${FIXTURE_PATH}" ]; then
    echo "Generating test fixtures..."
    python3 "${ROOT_DIR}/tests/fixtures/generate_fixtures.py"
fi

echo "=== Launching Benchmark on ${FIXTURE_PATH} (${ITERATIONS} iterations) ==="
"${BENCHMARK_BIN}" "${FIXTURE_PATH}" "${ITERATIONS}"
