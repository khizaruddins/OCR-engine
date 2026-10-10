#!/usr/bin/env bash
# OCR-DI - Cleanup Legacy Output Artifacts
# This script safely cleans legacy flat outputs while preserving source documents.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_ROOT="$(dirname "$SCRIPT_DIR")"
RAW_JSON_DIR="${WORKSPACE_ROOT}/output/raw_json"

echo "================================================="
echo "OCR-DI Legacy Artifact Cleanup Utility"
echo "================================================="

if [ -d "${RAW_JSON_DIR}" ]; then
    echo "Found legacy flat output directory: ${RAW_JSON_DIR}"
    echo "Removing legacy generated JSON files with recursive suffixes..."
    
    # Remove files with double or recursive suffixes
    find "${RAW_JSON_DIR}" -type f \( \
        -name "*_raw_ocr_edr_full.json" -o \
        -name "*_raw_ocr_edr_llm.json" -o \
        -name "*_raw_ocr_llm_context.json" -o \
        -name "*_raw_ocr_llm_context_report.json" -o \
        -name "*_edr_full_edr_full.json" -o \
        -name "*_edr_full_edr_llm.json" \
    \) -print -delete || true

    echo "Cleanup of recursive suffixes complete."
else
    echo "No legacy flat output directory found."
fi

echo "================================================="
echo "Document artifacts are now neatly stored in:"
echo "output/documents/<document-name>/"
echo "================================================="
