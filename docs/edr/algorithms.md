# OCR Engine EDR Algorithms & Heuristics Reference

## 1. Reading Order & Spatial Sorting
- **Header / Body / Footer Segmentation**:
  - Header band: `y < pageHeight * 0.12`
  - Footer band: `y > pageHeight * 0.90`
- **Two-Column Gutter Detection**:
  - Documents with blocks split across the vertical median (`x < midX` vs `x > midX`) are detected and segmented into column 1 and column 2.
  - Sorted topologically: `Header -> Column 1 (top to bottom) -> Column 2 (top to bottom) -> Footer`.

## 2. Line & Block Reconstruction
- **Adaptive Line Baseline Grouping**:
  - Tokens sorted by `y` and `x`.
  - Tokens with vertical overlap (`|y1 - y2| <= max(height) * 0.5`) grouped into lines.
- **Label-Value Extraction**:
  - Heuristic parser searches for delimiters (`:`, `>`, `=`).
  - Validates key length (`<= 40 chars`) and uppercase alphabetic characteristics.

## 3. Academic Table Detection & Cell Reconstruction
- **Course Code Pattern**:
  - Regex pattern `[A-Z]{2,5}[ -]?[0-9]{3,4}[A-Z]?`.
  - Excludes header words (`ATT`, `EARN`, `UNITS`, `COURSE`, `GRADE`, `SPRING`, `FALL`, etc.).
- **Grade & Credit Classification**:
  - Standard grade dictionary: `A+, A, A-, B+, B, B-, C+, C, C-, D+, D, D-, F, P, NP, S, U, W, I, CR, NC`.
  - OCR error tolerance: `Ww` normalized to `W`.
  - Credit normalization: `o1` -> `01`.
- **Right-to-Left Reverse Scanning**:
  - Scans from line end for Grade and Credits (attempted vs earned).
  - Left tokens assigned to Course Code and Course Title.

## 4. Multi-Page Continuity
- Tables from consecutive pages with matching column schemas are automatically concatenated into a unified academic record table.
