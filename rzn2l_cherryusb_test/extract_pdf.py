import pypdf

src_path = "E:/Users/jerry.chen/Documents/Renesas/MCU&MPU/MCU/RZ/RZTN/RZN2L/r01uh0955ej0140-rzn2l.pdf"
dst_path = "E:/RS_workspace/rzn2l_cherryusb_test/rzn2l_usb_chapter.pdf"

reader = pypdf.PdfReader(src_path)
writer = pypdf.PdfWriter()

total_pages = len(reader.pages)
print(f"Total pages: {total_pages}")

start_page = 1406 - 1
end_page = 1519

if start_page < 0:
    start_page = 0
if end_page > total_pages:
    end_page = total_pages

print(f"Extracting pages {start_page+1} to {end_page}")

for i in range(start_page, end_page):
    writer.add_page(reader.pages[i])

with open(dst_path, "wb") as output:
    writer.write(output)

print(f"Extracted {end_page - start_page} pages to {dst_path}")
