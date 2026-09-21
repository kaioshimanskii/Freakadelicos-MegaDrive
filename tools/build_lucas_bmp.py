from pathlib import Path
import base64, zlib, struct

W, H = 512, 32
PAL_B64 = "eNr7z/D/88Vl948ceraudceiIytaJjbEJPnYOjvra5jo2BopK0gKC7MyszAxMDKA0CgYBcMKAACODhDH"
RAW_B64 = "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"

pal = zlib.decompress(base64.b64decode(PAL_B64))
pix = zlib.decompress(base64.b64decode(RAW_B64))
assert len(pal) == 768
assert len(pix) == W*H

palette = bytearray()
for i in range(256):
    r,g,b = pal[i*3:i*3+3]
    palette += bytes((b,g,r,0))

row = W
pixel_bytes = row * H
offset = 14 + 40 + 1024
size = offset + pixel_bytes

hdr = struct.pack("<2sIHHI", b"BM", size, 0, 0, offset)
dib = struct.pack("<IIIHHIIIIII", 40, W, H, 1, 8, 0, pixel_bytes, 2835, 2835, 256, 16)

out = bytearray(hdr + dib + palette)
for y in range(H-1, -1, -1):
    out += pix[y*W:(y+1)*W]

Path("res").mkdir(exist_ok=True)
Path("res/lucas.bmp").write_bytes(out)
print("Wrote", len(out), "bytes to res/lucas.bmp")
