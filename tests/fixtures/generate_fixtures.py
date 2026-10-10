#!/usr/bin/env python3
"""
Synthetic Educational Document Fixture Generator for EDMO DI-OCR Engine.
Generates clean, rotated, skewed, tabular images and multi-page transcript PDFs.
"""

import os
from PIL import Image, ImageDraw, ImageFont

OUTPUT_DIR = os.path.dirname(os.path.abspath(__file__))

def get_font(size=20):
    try:
        # Standard DejaVu or FreeSans on Linux
        return ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", size)
    except Exception:
        try:
            return ImageFont.truetype("/usr/share/fonts/truetype/freefont/FreeSans.ttf", size)
        except Exception:
            return ImageFont.load_default()

def get_bold_font(size=24):
    try:
        return ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", size)
    except Exception:
        return get_font(size)

def create_simple_text_png():
    img = Image.new("RGB", (800, 400), color=(255, 255, 255))
    draw = ImageDraw.Draw(img)
    font = get_font(24)
    bold = get_bold_font(32)

    draw.text((50, 40), "EDMO EDUCATION INTELLIGENCE", fill=(0, 0, 0), font=bold)
    draw.text((50, 100), "Academic Record Verification Document", fill=(50, 50, 50), font=font)
    draw.text((50, 160), "Student Name: Alex Morgan", fill=(0, 0, 0), font=font)
    draw.text((50, 210), "Student ID: EDU-2026-9876", fill=(0, 0, 0), font=font)
    draw.text((50, 260), "Program: Bachelor of Computer Science", fill=(0, 0, 0), font=font)
    draw.text((50, 310), "Cumulative GPA: 3.85 / 4.00", fill=(0, 0, 0), font=font)

    out_path = os.path.join(OUTPUT_DIR, "simple-text.png")
    img.save(out_path)
    print(f"Generated {out_path}")

def create_rotated_text_png():
    img = Image.new("RGB", (800, 400), color=(255, 255, 255))
    draw = ImageDraw.Draw(img)
    bold = get_bold_font(28)
    font = get_font(20)

    draw.text((50, 60), "OFFICIAL TRANSCRIPT OF RECORDS", fill=(0, 0, 0), font=bold)
    draw.text((50, 130), "University of Document Intelligence", fill=(0, 0, 0), font=font)
    draw.text((50, 180), "Orientation Test: Rotated 90 Degrees Clockwise", fill=(0, 0, 0), font=font)
    draw.text((50, 230), "Status: Verified Authentic", fill=(0, 0, 0), font=font)

    # Rotate 90 degrees clockwise
    rotated = img.rotate(-90, expand=True, fillcolor=(255, 255, 255))
    out_path = os.path.join(OUTPUT_DIR, "rotated-text.png")
    rotated.save(out_path)
    print(f"Generated {out_path}")

def create_skewed_text_png():
    img = Image.new("RGB", (1000, 500), color=(255, 255, 255))
    draw = ImageDraw.Draw(img)
    bold = get_bold_font(30)
    font = get_font(22)

    draw.text((60, 50), "DEPARTMENT OF HIGHER EDUCATION", fill=(0, 0, 0), font=bold)
    draw.text((60, 120), "Course: CS101 Introduction to Algorithms", fill=(0, 0, 0), font=font)
    draw.text((60, 170), "Grade Awarded: A (4.0 Grade Points)", fill=(0, 0, 0), font=font)
    draw.text((60, 220), "Credits Earned: 4.0 Semester Units", fill=(0, 0, 0), font=font)
    draw.text((60, 270), "Instructor: Dr. Alan Turing", fill=(0, 0, 0), font=font)
    draw.text((60, 320), "Skew Correction Test Document", fill=(0, 0, 0), font=font)

    # Rotate by 4.5 degrees to simulate document scan skew
    skewed = img.rotate(4.5, expand=True, fillcolor=(255, 255, 255), resample=Image.BICUBIC)
    out_path = os.path.join(OUTPUT_DIR, "skewed-text.png")
    skewed.save(out_path)
    print(f"Generated {out_path}")

def create_table_document_png():
    img = Image.new("RGB", (1200, 800), color=(255, 255, 255))
    draw = ImageDraw.Draw(img)
    bold = get_bold_font(26)
    font = get_font(18)
    header_font = get_bold_font(18)

    draw.text((80, 50), "ACADEMIC MARKSHEET & SEMESTER REPORT", fill=(0, 0, 0), font=bold)
    draw.text((80, 100), "Term: Fall 2025 | Candidate: Jordan Smith | ID: JS-8842", fill=(50, 50, 50), font=font)

    # Draw Table
    top_y = 160
    row_h = 45
    cols = [80, 220, 650, 800, 950, 1120]
    headers = ["Course Code", "Course Title", "Credits", "Grade", "Status"]

    # Outer border & header background
    draw.rectangle([cols[0], top_y, cols[-1], top_y + row_h], fill=(240, 240, 240), outline=(0, 0, 0), width=2)
    for i, h in enumerate(headers):
        draw.text((cols[i] + 15, top_y + 12), h, fill=(0, 0, 0), font=header_font)
        if i > 0:
            draw.line([cols[i], top_y, cols[i], top_y + 6 * row_h], fill=(0, 0, 0), width=1)

    rows = [
        ("CS-101", "Introduction to Computer Science", "4.0", "A", "PASSED"),
        ("MATH-201", "Discrete Mathematics & Logic", "3.0", "A-", "PASSED"),
        ("ENG-105", "Academic Writing & Composition", "3.0", "B+", "PASSED"),
        ("PHYS-110", "General Physics: Mechanics", "4.0", "A", "PASSED"),
        ("DS-250", "Data Structures and Algorithms", "4.0", "A+", "PASSED"),
    ]

    for r_idx, row in enumerate(rows):
        curr_y = top_y + (r_idx + 1) * row_h
        draw.rectangle([cols[0], curr_y, cols[-1], curr_y + row_h], outline=(0, 0, 0), width=1)
        for c_idx, val in enumerate(row):
            draw.text((cols[c_idx] + 15, curr_y + 12), val, fill=(0, 0, 0), font=font)

    summary_y = top_y + 7 * row_h
    draw.text((80, summary_y), "Semester Total Credits: 18.0   |   Semester GPA: 3.92   |   Academic Standing: Dean's Honor List", fill=(0, 0, 0), font=bold)

    out_path = os.path.join(OUTPUT_DIR, "table-document.png")
    img.save(out_path)
    print(f"Generated {out_path}")

def create_transcript_pdf():
    # Page 1: Student Information and Freshman/Sophomore years
    p1 = Image.new("RGB", (1600, 2200), color=(255, 255, 255))
    d1 = ImageDraw.Draw(p1)
    title_font = get_bold_font(36)
    sub_font = get_font(22)
    header_font = get_bold_font(20)
    body_font = get_font(18)

    d1.text((100, 80), "STATE UNIVERSITY OF DOCUMENT INTELLIGENCE", fill=(0, 0, 0), font=title_font)
    d1.text((100, 140), "OFFICIAL ACADEMIC TRANSCRIPT", fill=(100, 100, 100), font=header_font)
    d1.line([100, 180, 1500, 180], fill=(0, 0, 0), width=3)

    # Student metadata box
    d1.rectangle([100, 210, 1500, 360], outline=(150, 150, 150), width=2)
    d1.text((130, 230), "Student: Emily Watson", fill=(0, 0, 0), font=sub_font)
    d1.text((130, 270), "Student ID: STU-88776655", fill=(0, 0, 0), font=sub_font)
    d1.text((130, 310), "Degree: Bachelor of Science in Artificial Intelligence", fill=(0, 0, 0), font=sub_font)
    d1.text((850, 230), "Date of Birth: 2003-05-14", fill=(0, 0, 0), font=sub_font)
    d1.text((850, 270), "Date Issued: 2026-06-15", fill=(0, 0, 0), font=sub_font)
    d1.text((850, 310), "Cumulative GPA: 3.91 / 4.00", fill=(0, 0, 0), font=sub_font)

    # Course records
    start_y = 420
    d1.text((100, start_y), "FIRST YEAR — FALL SEMESTER", fill=(0, 0, 0), font=header_font)
    y = start_y + 40
    courses_f1 = [
        ("CS-101", "Introduction to Programming", "4.0", "A", "4.00"),
        ("MATH-110", "Calculus I: Differential Equations", "4.0", "A", "4.00"),
        ("ENG-101", "College Writing and Research", "3.0", "A-", "3.70"),
        ("AI-100", "Foundations of Cognitive Science", "3.0", "A", "4.00"),
    ]
    for c in courses_f1:
        d1.text((120, y), f"{c[0]:<12} {c[1]:<45} Credits: {c[2]}   Grade: {c[3]}   Points: {c[4]}", fill=(0, 0, 0), font=body_font)
        y += 35

    y += 30
    d1.text((100, y), "FIRST YEAR — SPRING SEMESTER", fill=(0, 0, 0), font=header_font)
    y += 40
    courses_s1 = [
        ("CS-102", "Data Structures & Modern C++", "4.0", "A", "4.00"),
        ("MATH-120", "Linear Algebra and Matrices", "4.0", "A+", "4.00"),
        ("STAT-200", "Probability and Statistics for CS", "3.0", "A", "4.00"),
        ("PHYS-101", "Physics for Engineers", "4.0", "B+", "3.30"),
    ]
    for c in courses_s1:
        d1.text((120, y), f"{c[0]:<12} {c[1]:<45} Credits: {c[2]}   Grade: {c[3]}   Points: {c[4]}", fill=(0, 0, 0), font=body_font)
        y += 35

    d1.line([100, 2050, 1500, 2050], fill=(200, 200, 200), width=1)
    d1.text((100, 2070), "Page 1 of 2 — Official University Document — Registrar: Dr. R. Penrose", fill=(100, 100, 100), font=body_font)

    # Page 2: Junior/Senior years and graduation award
    p2 = Image.new("RGB", (1600, 2200), color=(255, 255, 255))
    d2 = ImageDraw.Draw(p2)

    d2.text((100, 80), "STATE UNIVERSITY OF DOCUMENT INTELLIGENCE", fill=(0, 0, 0), font=title_font)
    d2.text((100, 140), "OFFICIAL ACADEMIC TRANSCRIPT (CONTINUED)", fill=(100, 100, 100), font=header_font)
    d2.line([100, 180, 1500, 180], fill=(0, 0, 0), width=3)

    y = 220
    d2.text((100, y), "SECOND YEAR — ADVANCED STUDIES", fill=(0, 0, 0), font=header_font)
    y += 40
    courses_adv = [
        ("AI-301", "Computer Vision and Image Understanding", "4.0", "A+", "4.00"),
        ("AI-305", "Natural Language Processing and Transformers", "4.0", "A", "4.00"),
        ("CS-340", "Distributed Systems & Cloud Computing", "4.0", "A", "4.00"),
        ("AI-490", "Senior Capstone: Document Intelligence Core", "6.0", "A+", "4.00"),
    ]
    for c in courses_adv:
        d2.text((120, y), f"{c[0]:<12} {c[1]:<45} Credits: {c[2]}   Grade: {c[3]}   Points: {c[4]}", fill=(0, 0, 0), font=body_font)
        y += 35

    # Degree Award summary box
    y += 60
    d2.rectangle([100, y, 1500, y + 200], fill=(245, 248, 255), outline=(0, 80, 180), width=2)
    d2.text((140, y + 30), "DEGREE CONFERRED: BACHELOR OF SCIENCE IN ARTIFICIAL INTELLIGENCE", fill=(0, 50, 120), font=header_font)
    d2.text((140, y + 75), "Graduation Date: June 15, 2026   |   Honors: Summa Cum Laude", fill=(0, 0, 0), font=sub_font)
    d2.text((140, y + 120), "Total Degree Credits Earned: 128.0 Semester Units   |   Overall GPA: 3.94", fill=(0, 0, 0), font=sub_font)

    d2.line([100, 2050, 1500, 2050], fill=(200, 200, 200), width=1)
    d2.text((100, 2070), "Page 2 of 2 — End of Transcript — Verified by Registrar", fill=(100, 100, 100), font=body_font)

    out_pdf = os.path.join(OUTPUT_DIR, "transcript-like.pdf")
    p1.save(out_pdf, save_all=True, append_images=[p2])
    print(f"Generated {out_pdf}")

    # Also save simple multi-page.pdf
    multi_pdf = os.path.join(OUTPUT_DIR, "multi-page.pdf")
    p1.save(multi_pdf, save_all=True, append_images=[p2])
    print(f"Generated {multi_pdf}")

if __name__ == "__main__":
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    create_simple_text_png()
    create_rotated_text_png()
    create_skewed_text_png()
    create_table_document_png()
    create_transcript_pdf()
    print("All fixtures generated successfully!")
