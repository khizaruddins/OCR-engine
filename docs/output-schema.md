# EDMO OCR Output Schema v0.1 Specification

This document defines the deterministic JSON schema returned by the EDMO DI-OCR engine.

## 1. Top-Level Schema Structure

```json
{
  "schema_version": "0.1",
  "id": "proc_1724063920123_a1b2c3",
  "engine": {
    "name": "edmo-ocr",
    "version": "0.1.0",
    "provider": "tesseract"
  },
  "document": {
    "id": "doc_1724063920000_f9e8d7",
    "source": "/path/to/transcript.pdf",
    "type": "pdf",
    "page_count": 2,
    "file_size_bytes": 142050
  },
  "confidence": 0.962,
  "pages": [ ... ],
  "processing": {
    "pdf_render_time_ms": 42.5,
    "preprocess_time_ms": 18.2,
    "ocr_time_ms": 380.1,
    "serialization_time_ms": 1.2,
    "total_time_ms": 442.0
  }
}
```

---

## 2. Page Structure (`pages[]`)

Each page object contains page metadata, full raw and normalized text, geometric rotation, skew details, and structural block/line/token hierarchies:

```json
{
  "page_number": 1,
  "width": 2480,
  "height": 3508,
  "rotation": 0,
  "skew_angle": 0.12,
  "confidence": 0.965,
  "text": "STATE UNIVERSITY OF DOCUMENT INTELLIGENCE\nOFFICIAL ACADEMIC TRANSCRIPT...",
  "raw_text": "STATE UNIVERSITY OF DOCUMENT INTELLIGENCE\nOFFICIAL ACADEMIC TRANSCRIPT...",
  "timing": {
    "render_time_ms": 21.2,
    "preprocess_time_ms": 9.1,
    "ocr_time_ms": 190.0
  },
  "blocks": [
    {
      "text": "STATE UNIVERSITY OF DOCUMENT INTELLIGENCE",
      "confidence": 0.985,
      "bbox": {
        "x": 100,
        "y": 80,
        "width": 1400,
        "height": 60
      },
      "lines": [
        {
          "text": "STATE UNIVERSITY OF DOCUMENT INTELLIGENCE",
          "confidence": 0.985,
          "bbox": {
            "x": 100,
            "y": 80,
            "width": 1400,
            "height": 60
          },
          "tokens": [
            {
              "text": "STATE",
              "confidence": 0.99,
              "bbox": { "x": 100, "y": 80, "width": 180, "height": 60 }
            },
            {
              "text": "UNIVERSITY",
              "confidence": 0.98,
              "bbox": { "x": 300, "y": 80, "width": 380, "height": 60 }
            }
          ]
        }
      ]
    }
  ]
}
```

---

## 3. Coordinate System Definition

- **Origin `(0, 0)`**: Top-left pixel of the page image.
- **`x`**: Horizontal distance from left edge (increases to the right).
- **`y`**: Vertical distance from top edge (increases downwards).
- **`width`**: Horizontal bounding span.
- **`height`**: Vertical bounding span.

All units are in integer pixels relative to the page dimensions (`width` x `height`).
