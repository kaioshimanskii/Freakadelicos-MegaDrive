from pathlib import Path
import base64
import zlib
import struct

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "assets_v4"
RES = ROOT / "res"
RES.mkdir(exist_ok=True)

BG_NAMES = {"stage", "encounter", "cave"}
WANTED = {"stage", "encounter", "cave", "kaio", "nico", "rod", "lucas", "mila", "guguwhite", "guguyellow", "turco"}

def write_bmp(path, w, h, palette_rgb, pixels):
    pal = bytearray()
    for i in range(256):
        if i < 16:
            r, g, b = palette_rgb[i*3:i*3+3]
        else:
            r = g = b = 0
        pal += bytes((b, g, r, 0))

    row_stride = (w + 3) & ~3
    pixel_bytes = row_stride * h
    offset = 14 + 40 + 1024
    size = offset + pixel_bytes

    out = bytearray()
    out += struct.pack("<2sIHHI", b"BM", size, 0, 0, offset)
    out += struct.pack("<IIIHHIIIIII", 40, w, h, 1, 8, 0, pixel_bytes, 2835, 2835, 256, 16)
    out += pal

    for y in range(h - 1, -1, -1):
        row = pixels[y*w:(y+1)*w]
        out += row
        out += bytes(row_stride - w)

    path.write_bytes(out)

def expand2(raw, w, h):
    out_w = w * 2
    out_h = h * 2
    out = bytearray(out_w * out_h)

    for y in range(h):
        for x in range(w):
            v = raw[y*w + x]
            ox = x * 2
            oy = y * 2
            out[oy*out_w + ox] = v
            out[oy*out_w + ox + 1] = v
            out[(oy+1)*out_w + ox] = v
            out[(oy+1)*out_w + ox + 1] = v

    return bytes(out), out_w, out_h

for dat in sorted(DATA.glob("*.dat")):
    for line in dat.read_text().splitlines():
        if not line.strip():
            continue

        name, sw, sh, payload = line.split("|", 3)

        if name not in WANTED:
            continue

        w = int(sw)
        h = int(sh)
        blob = zlib.decompress(base64.b64decode(payload))
        palette = blob[:48]
        raw = blob[48:]

        if len(raw) != w * h:
            raise RuntimeError(f"{name}: expected {w*h} pixels, got {len(raw)}")

        if name in BG_NAMES:
            raw, w, h = expand2(raw, w, h)

        write_bmp(RES / f"{name}.bmp", w, h, palette, raw)
        print(f"generated {name}.bmp {w}x{h}")
