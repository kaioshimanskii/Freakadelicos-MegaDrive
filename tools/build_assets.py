from pathlib import Path
import base64, io, zipfile

raw = base64.b64decode(Path("assets.b64").read_text().strip())
with zipfile.ZipFile(io.BytesIO(raw), "r") as z:
    z.extractall(".")
print("Sprite assets restored:", len(z.namelist()))
