import pypdf
reader = pypdf.PdfReader("rzn2l_usb_chapter.pdf")
print(f"Total pages: {len(reader.pages)}")
text = ""
for i, page in enumerate(reader.pages[:30]):
    t = page.extract_text()
    if t:
        text += f"\n=== Page {i+1} ===\n" + t
with open("rzn2l_usb_chapter.txt", "w", encoding="utf-8") as f:
    f.write(text)
print(f"Extracted text length: {len(text)}")
