# OCR-DI Test Fixtures

This directory contains synthetic, copyright-free educational test documents designed specifically for testing and benchmarking the EDMO C++ OCR Engine v0.1.

## Fixture Inventory

1. **`simple-text.png`**: High-contrast, clean single-page document with student metadata.
2. **`rotated-text.png`**: Document rotated 90 degrees clockwise to validate orientation detection and rotation correction.
3. **`skewed-text.png`**: Document with a 4.5-degree scan skew to validate Hough transform / contour deskew algorithms.
4. **`table-document.png`**: Complex tabular semester marksheet with course codes, credits, grades, and table borders.
5. **`transcript-like.pdf`**: Multi-page official university academic transcript containing student details, multiple semesters, GPA, and degree conferral.
6. **`multi-page.pdf`**: Multi-page PDF to test page ordering, page metadata, and independent page processing.

## Generating Fixtures

To regenerate synthetic fixtures at any time, run:

```bash
python3 generate_fixtures.py
```
