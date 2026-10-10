# OCR-DI Processing Pipeline

The OCR processing pipeline consists of 8 sequential stages designed for deterministic execution, high accuracy on educational documents, and low latency.

```text
Input Path / Bytes
       │
       ▼
 [1. Validation] ──────► (Existence, Size limits, Supported formats, Read permissions)
       │
       ▼
 [2. Ingestion]  ──────► PDF: Poppler rendering @ target DPI (150-600)
       │                 Image: OpenCV imread with dimension bounds
       ▼
 [3. Orientation] ─────► Detect orientation {0°, 90°, 180°, 270°} and rotate
       │
       ▼
 [4. Deskew] ──────────► Detect skew angle [-10°..+10°] via Hough Lines & affine warp
       │
       ▼
 [5. Preprocess] ──────► Grayscale -> Denoise -> Contrast (CLAHE) -> Binarize -> Sharpen
       │
       ▼
 [6. Inference] ───────► IOcrProvider (Tesseract LSTM engine)
       │
       ▼
 [7. Normalization] ───► Reconstruct Document -> Page -> Block -> Line -> Token tree
       │
       ▼
 [8. Serialization] ───► JSON Output (Compact / Full) or Plain Text
```

---

## Detailed Pipeline Stages

### Stage 1: Validation
- Inspects filesystem existence, ensures non-empty content (size > 0 bytes).
- Enforces configurable limits (`max_file_size_mb = 50`, `max_pages = 100`, `max_image_dimension = 20000`).
- Validates file extensions (`.pdf`, `.png`, `.jpg`, `.jpeg`, `.tiff`, `.bmp`).

### Stage 2: Document Ingestion & Rendering
- **PDF Documents**: Rendered page-by-page at configurable resolution (default `300 DPI`) using Poppler C++ with anti-aliasing hints.
- **Images**: Loaded with OpenCV `cv::imread` in native BGR format.

### Stage 3: Orientation Detection & Correction
- Uses Tesseract Orientation and Script Detection (OSD) to identify whether the document was scanned or uploaded sideways/upside-down.
- Applies lossless 90-degree step rotations (`cv::ROTATE_90_CLOCKWISE`, `cv::ROTATE_180`, `cv::ROTATE_90_COUNTERCLOCKWISE`).

### Stage 4: Skew Detection & Deskewing
- Applies adaptive thresholding and horizontal morphological closing to consolidate characters into continuous text baselines.
- Employs Probabilistic Hough Transform (`cv::HoughLinesP`) to find line angles within `[-10.0°, +10.0°]`.
- Calculates median angle and performs affine rotation with bicubic interpolation and white background padding.

### Stage 5: Image Preprocessing Pipeline
Configurable preprocessing transformations:
- **Grayscale**: Reduces color channels to single-channel luminance.
- **Denoising**: Gaussian blur, median filtering, or bilateral edge-preserving smoothing.
- **Contrast Enhancement**: Contrast Limited Adaptive Histogram Equalization (CLAHE) with configurable clip limit (default 2.0) and grid tiles.
- **Binarization**: Conservative by default. Supports Otsu's thresholding, Adaptive Gaussian, Adaptive Mean, and Global thresholding.
- **Sharpening**: Unsharp masking to enhance blurred character glyphs.

### Stage 6: OCR Provider Inference
- Executes `IOcrProvider::process()`.
- Reference backend leverages Tesseract LSTM neural network models with specified language and Page Segmentation Modes (PSM 3 default).

### Stage 7: Result Normalization & Hierarchy
- Parses `ResultIterator` from inference into an explicit hierarchical document graph:
  - `Block`: Paragraph / structural text block with bounding box.
  - `Line`: Single baseline of text with bounding box.
  - `Token`: Individual word/symbol with bounding box and confidence.
- Normalizes confidence values to a standardized `[0.0, 1.0]` floating-point range.
- Coordinate system: Origin `(0, 0)` is top-left, X increases right, Y increases down.

### Stage 8: Serialization & Reporting
- Emits deterministic JSON conforming to OCR schema v0.1.
- Tracks granular latency metrics: rendering time, preprocessing time, OCR time, serialization time, total end-to-end time.
