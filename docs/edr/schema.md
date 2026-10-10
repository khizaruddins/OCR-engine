# OCR Document Representation (EDR) Schemas

## 1. Full EDR Schema (`<stem>_edr_full.json`)

The Full EDR schema contains full document structure, hierarchical confidence summaries, and geometry tracking.

```json
{
  "schema_version": "edr-0.1",
  "document": {
    "id": "boston_transcript",
    "type": "transcript",
    "page_count": 2,
    "timing": {
      "total_processing_ms": 1423.5,
      "page_rendering_ms": 110.2,
      "preprocessing_ms": 320.1,
      "ocr_ms": 993.2
    }
  },
  "identity": {
    "student_name": "Sophia Jones",
    "student_id": "2291829"
  },
  "institution": {
    "name": "Boston College"
  },
  "academic": {
    "degree": "BACHELOR OF SCIENCE - 05/19/2025",
    "major": "BIOLOGY",
    "school": "MORRISSEY COLLEGE OF ARTS AND SCIENCES"
  },
  "pages": [
    {
      "page_number": 1,
      "width": 1700,
      "height": 2200,
      "reading_order": ["block_p1_1", "block_p1_2"],
      "confidence": {
        "mean": 0.941,
        "minimum": 0.65,
        "maximum": 0.99,
        "token_count": 450,
        "low_confidence_token_count": 3
      }
    }
  ],
  "sections": [
    {
      "id": "section_student_info",
      "title": "Student Information",
      "type": "student_information",
      "blocks": [
        {
          "id": "block_p1_1",
          "type": "label_value",
          "label": "NAME",
          "value": "Sophia Jones",
          "source": {
            "page_number": 1,
            "bbox": {"x": 100, "y": 200, "width": 400, "height": 30}
          }
        }
      ]
    }
  ],
  "tables": [
    {
      "id": "table_1",
      "type": "course_records",
      "columns": [
        {"name": "course_code", "type": "course_code", "index": 0},
        {"name": "course_title", "type": "course_title", "index": 1},
        {"name": "credits", "type": "credits", "index": 2},
        {"name": "grade", "type": "grade", "index": 3}
      ],
      "rows": [
        {
          "fields": {
            "course_code": "CS101",
            "course_title": "Introduction to Computer Science",
            "credits": "3",
            "grade": "A"
          },
          "source": {
            "page_number": 1,
            "bbox": {"x": 100, "y": 500, "width": 600, "height": 30}
          }
        }
      ]
    }
  ],
  "compression_metrics": {
    "raw_bytes": 107950,
    "edr_full_bytes": 99314,
    "edr_llm_bytes": 5620,
    "raw_to_full_ratio": 1.09,
    "raw_to_llm_ratio": 19.21,
    "text_preservation_percentage": 100.0,
    "estimated_raw_tokens": 26987,
    "estimated_full_tokens": 24828,
    "estimated_llm_tokens": 1405
  },
  "low_confidence_warnings": []
}
```

---

## 2. LLM EDR Schema (`<stem>_edr_llm.json`)

The LLM EDR schema is engineered specifically for direct consumption by LLMs in prompt payloads.

```json
{
  "document_type": "transcript",
  "page_count": 2,
  "student": {
    "student_name": "Sophia Jones",
    "student_id": "2291829"
  },
  "institution": {
    "name": "Boston College"
  },
  "academic": {
    "degree": "BACHELOR OF SCIENCE - 05/19/2025",
    "major": "BIOLOGY",
    "school": "MORRISSEY COLLEGE OF ARTS AND SCIENCES",
    "gpa": "3.437"
  },
  "academic_records": [
    {
      "course_code": "BIOL4570",
      "course_title": "PRINCIPLES OF IMMUNOLOGY",
      "attempted_credits": "03",
      "earned_credits": "03",
      "credits": "03",
      "grade": "C+"
    },
    {
      "course_code": "BIOL4960",
      "course_title": "UNDERGRADUATE RESEARCH",
      "attempted_credits": "03",
      "earned_credits": "03",
      "credits": "03",
      "grade": "A"
    }
  ]
}
```
