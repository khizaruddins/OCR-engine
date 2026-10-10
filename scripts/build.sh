#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

BUILD_TYPE="${1:-Release}"
BUILD_DIR="${ROOT_DIR}/build"

echo "=== Building OCR-DI Engine v0.1 (${BUILD_TYPE}) ==="

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DBUILD_TESTING=ON \
    -DBUILD_BENCHMARKS=ON \
    -DBUILD_EXAMPLES=ON

cmake --build "${BUILD_DIR}" -j"$(nproc)"

echo "=== Build Complete! Artifacts located in: ${BUILD_DIR} ==="
