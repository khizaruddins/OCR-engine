# LLM Usage & Integration Guide for EDMO EDR

## 1. CLI Usage

### Default Processing Workflow (Stores Raw JSON -> Creates EDR JSONs)
```bash
./build/bin/edmo-ocr "/path/to/transcript.pdf" --output-dir "output/"
```
This automatically executes the full two-stage pipeline:
1. Performs OCR and **stores the Raw OCR JSON** to `output/<stem>_raw_ocr.json`.
2. Consumes that stored raw JSON to **generate Full and LLM EDR JSONs**:
   - `output/<stem>_edr_full.json`
   - `output/<stem>_edr_llm.json`

### Explicit Modes:
- `--representation both` (default): Store raw JSON and generate both Full & LLM EDR.
- `--representation none`: Output raw OCR only.
- `--representation edr`: Store raw JSON and output Full EDR.
- `--representation llm`: Store raw JSON and output LLM EDR.

### Analyze & Benchmark Existing Raw OCR JSON
```bash
./build/bin/edmo-ocr-analyze \
  --raw "output/raw_json/boston_transcript.json" \
  --edr-full "/tmp/boston_results/boston_transcript_edr_full.json" \
  --edr-llm "/tmp/boston_results/boston_transcript_edr_llm.json"
```

---

## 2. LLM Prompt Construction Example

```python
import json
import openai

# 1. Load the lightweight LLM EDR JSON
with open("/tmp/boston_results/boston_transcript_edr_llm.json") as f:
    edr_llm = json.load(f)

# 2. Inject directly into system prompt
response = openai.chat.completions.create(
    model="gpt-4o",
    messages=[
        {
            "role": "system",
            "content": "You are an academic evaluation assistant. Analyze the following verified transcript representation."
        },
        {
            "role": "user",
            "content": f"Here is the transcript data:\n{json.dumps(edr_llm, indent=2)}\n\nCalculate total major credits and list STEM courses taken."
        }
    ]
)
print(response.choices[0].message.content)
```

### Why use `edr_llm.json` instead of Raw OCR?
1. **Token Cost & Latency Reduction**: Reduces tokens by **90% - 95%** (~1,400 tokens vs ~27,000 tokens).
2. **Eliminates OCR Hallucination**: No bounding box distraction or OCR character noise.
3. **Deterministic Structure**: Standardized `student`, `institution`, `academic`, and `academic_records` fields.
