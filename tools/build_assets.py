from pathlib import Path
import base64, io, zipfile, struct

raw = base64.b64decode(Path("assets.b64").read_text().strip())
with zipfile.ZipFile(io.BytesIO(raw), "r") as z:
    z.extractall(".")

def enlarge_sheet_32_to_40(path):
    p = Path(path)
    data = bytearray(p.read_bytes())
    if data[:2] != b"BM":
        return
    off = struct.unpack_from("<I", data, 10)[0]
    width = struct.unpack_from("<i", data, 18)[0]
    height = struct.unpack_from("<i", data, 22)[0]
    bpp = struct.unpack_from("<H", data, 28)[0]
    if bpp != 8 or width % 32 != 0 or abs(height) != 32:
        return

    h = abs(height)
    old_stride = ((width + 3) // 4) * 4
    rows = []
    pix = data[off:]
    for y in range(h):
        start = y * old_stride
        rows.append(bytes(pix[start:start+width]))

    # BMP is bottom-up when height > 0. Keep that order while scaling.
    cells = width // 32
    new_w = cells * 40
    new_h = 40
    new_stride = ((new_w + 3) // 4) * 4
    out_rows = []

    for ny in range(new_h):
        sy = min(31, (ny * 32) // 40)
        src = rows[sy]
        dst = bytearray()
        for cell in range(cells):
            base = cell * 32
            for nx in range(40):
                sx = min(31, (nx * 32) // 40)
                dst.append(src[base + sx])
        dst.extend(b"\x00" * (new_stride - new_w))
        out_rows.append(bytes(dst))

    header = bytearray(data[:off])
    struct.pack_into("<i", header, 18, new_w)
    struct.pack_into("<i", header, 22, 40 if height > 0 else -40)
    size_image = new_stride * new_h
    struct.pack_into("<I", header, 34, size_image)
    struct.pack_into("<I", header, 2, off + size_image)
    p.write_bytes(header + b"".join(out_rows))

for name in ["kaio.bmp","nico.bmp","rod.bmp","lucas.bmp","mila.bmp"]:
    enlarge_sheet_32_to_40(Path("res") / name)

print("Sprite assets restored and main characters enlarged")
