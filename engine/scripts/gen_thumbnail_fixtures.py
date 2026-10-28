#!/usr/bin/env python3
"""gen_thumbnail_fixtures.py - generate the cr_thumbnail unit-test fixtures.

Writes tests/data/thumbnails/*.bin image fixtures + SHA256SUMS. The quadrant
pattern (64x48, four solid color quadrants) is chosen so an exact-integer
box downscale to 16x12 maps each 4x4 source block onto exactly one output
pixel - the downscaled colors are then EXACT (no blending ambiguity), which
lets the unit tests assert pixel values without tolerance for lossless
formats (png/bmp/gif/tiff) and with a small tolerance only for jpeg.

Hand-spliced fixtures (no encoder exists for these shapes):
  bomb_dims.png            - IHDR declares 30000x30000 with a tiny IDAT
                             (decompression-bomb guard: dims-first refusal)
  huge_dims_exif_thumb.jpg - SOI + APP1/Exif carrying a REAL embedded
                             thumbnail + SOF0 declaring 30000x20000 + EOI
                             (embedded-thumbnail fast path for oversized
                             JPEGs; the main stream is intentionally not
                             decodable)
  exif_thumb_small.jpg     - a normal small JPEG that also carries an
                             embedded EXIF thumbnail (full decode must win
                             over the embedded path)
  webp_magic.bin           - RIFF/WEBP magic without a real payload
                             (named-unsupported path)
  exif_orient{1..8}.jpg    - quad64x48 JPEG + APP1/Exif whose IFD0 carries
                             EXIF Orientation 1..8 (S-043: the og applies
                             the matching RotateFlip before the 256-px pass;
                             the quadrant colors pin each transform)
  exif_orient9.jpg         - orientation byte 9 (invalid): no transform
  byteheavy12mp.jpg        - a 4.07 MiB 12-MP JPEG (M-080: the byte-heavy
                             pixel-benign middle class)
  tiff_raw_cr2.bin         - a CR2-shaped camera RAW (M-085/S-045: IFD0 with
                             the JPEGInterchangeFormat pair 513/514 pointing
                             at the full-size JPEG preview)
  exif_orient{1..8}.jpg    - quad64x48 JPEG + APP1/Exif whose IFD0 carries
                             EXIF Orientation 1..8 (S-043: the og applies
                             the matching RotateFlip before the 256-px pass;
                             the quadrant colors pin each transform)
  exif_orient9.jpg         - orientation byte 9 (invalid): no transform

Run: python3 scripts/gen_thumbnail_fixtures.py  (idempotent).
"""

import hashlib
import io
import os
import random
import struct
import sys
import zlib

from PIL import Image


def enc(im, **kw):
    buf = io.BytesIO()
    im.save(buf, **kw)
    return buf.getvalue()

OUT = os.path.join(os.path.dirname(__file__), "..", "tests", "data", "thumbnails")

W, H = 64, 48
RED, GREEN, BLUE, YELLOW = (255, 0, 0), (0, 255, 0), (0, 0, 255), (255, 255, 0)


def quadrant():
    im = Image.new("RGB", (W, H))
    px = im.load()
    for y in range(H):
        for x in range(W):
            if y < H // 2:
                px[x, y] = RED if x < W // 2 else GREEN
            else:
                px[x, y] = BLUE if x < W // 2 else YELLOW
    return im


def png_chunk(tag, payload):
    return (
        struct.pack(">I", len(payload))
        + tag
        + payload
        + struct.pack(">I", zlib.crc32(tag + payload) & 0xFFFFFFFF)
    )


def bomb_dims_png():
    ihdr = struct.pack(">IIBBBBB", 30000, 30000, 8, 2, 0, 0, 0)
    idat = zlib.compress(b"\x00" + b"\x11\x22\x33" * 8)  # one filtered row stub
    return (
        b"\x89PNG\r\n\x1a\n"
        + png_chunk(b"IHDR", ihdr)
        + png_chunk(b"IDAT", idat)
        + png_chunk(b"IEND", b"")
    )


def tiff_ifd_entry(tag, typ, values, bo="<"):
    """One 12-byte IFD entry; SHORT/LONG/BYTE inline when <= 4 bytes."""
    sizes = {1: 1, 3: 2, 4: 4}
    n = len(values)
    fmt = {1: "B", 3: "H", 4: "I"}[typ]
    raw = b"".join(struct.pack(bo + fmt, v) for v in values)
    if len(raw) <= 4:
        raw = raw.ljust(4, b"\x00")
        return struct.pack(bo + "HHI", tag, typ, n) + raw
    raise ValueError("out-of-line value not needed for these fixtures")


def exif_app1(thumb_jpeg: bytes) -> bytes:
    """APP1/Exif segment carrying IFD0 -> IFD1(0x201/0x202) -> thumbnail."""
    bo = "<"  # II
    tiff = struct.pack(bo + "2sHI", b"II", 42, 8)
    ifd0 = struct.pack(bo + "H", 1) + tiff_ifd_entry(0x0112, 3, [1]) + struct.pack(bo + "I", 26)
    ifd1_off = 8 + len(ifd0)  # 26
    thumb_off = ifd1_off + 2 + 12 * 2 + 4  # 56
    ifd1 = (
        struct.pack(bo + "H", 2)
        + tiff_ifd_entry(0x0201, 4, [thumb_off])
        + tiff_ifd_entry(0x0202, 4, [len(thumb_jpeg)])
        + struct.pack(bo + "I", 0)
    )
    body = tiff + ifd0 + ifd1 + thumb_jpeg
    return b"\xff\xe1" + struct.pack(">H", len(body) + 2 + 6)[:2] + b"Exif\x00\x00" + body


def exif_orient_app1(orient: int) -> bytes:
    """APP1/Exif with IFD0 carrying ONLY the Orientation tag (S-043)."""
    bo = "<"
    tiff = struct.pack(bo + "2sHI", b"II", 42, 8)
    ifd0 = struct.pack(bo + "H", 1) + tiff_ifd_entry(0x0112, 3, [orient]) + struct.pack(bo + "I", 0)
    body = tiff + ifd0
    return b"\xff\xe1" + struct.pack(">H", len(body) + 2 + 6)[:2] + b"Exif\x00\x00" + body


def exif_orient_app1(orient: int) -> bytes:
    """APP1/Exif with IFD0 carrying ONLY the Orientation tag (S-043)."""
    bo = "<"
    tiff = struct.pack(bo + "2sHI", b"II", 42, 8)
    ifd0 = struct.pack(bo + "H", 1) + tiff_ifd_entry(0x0112, 3, [orient]) + struct.pack(bo + "I", 0)
    body = tiff + ifd0
    return b"\xff\xe1" + struct.pack(">H", len(body) + 2 + 6)[:2] + b"Exif\x00\x00" + body


def tiff_raw_cr2(preview_jpeg: bytes) -> bytes:
    """S-045: a CR2-shaped camera RAW - II*\0 + IFD0 whose
    JPEGInterchangeFormat pair (513/514) points at the full-size JPEG
    preview (photometric 32803 = CFA, compression 1)."""
    bo = "<"
    entries = []

    def ent(tag, typ, vals):
        fmt = {1: "B", 3: "H", 4: "I"}[typ]
        raw = b"".join(struct.pack(bo + fmt, v) for v in vals)
        n = len(vals)
        if len(raw) <= 4:
            raw = raw.ljust(4, b"\x00")
            return struct.pack(bo + "HHI", tag, typ, n) + raw
        raise ValueError("inline only")

    entries.append(ent(256, 4, [64]))     # width
    entries.append(ent(257, 4, [48]))     # height
    entries.append(ent(258, 3, [8]))      # bits per sample
    entries.append(ent(259, 3, [1]))      # compression: none
    entries.append(ent(262, 4, [32803]))  # photometric: CFA
    entries.append(ent(277, 3, [1]))      # samples per pixel
    # 513/514 come last so their values can patch the real offsets below
    ifd_off = 8
    n_entries = len(entries) + 2
    ifd_size = 2 + 12 * n_entries + 4
    preview_off = ifd_off + ifd_size
    entries.append(ent(513, 4, [preview_off]))
    entries.append(ent(514, 4, [len(preview_jpeg)]))
    body = struct.pack(bo + "H", n_entries) + b"".join(entries) + struct.pack(bo + "I", 0)
    tiff = struct.pack(bo + "2sHI", b"II", 42, ifd_off) + body
    return tiff + preview_jpeg


def huge_dims_exif_jpeg(thumb_jpeg: bytes) -> bytes:
    app1 = exif_app1(thumb_jpeg)
    sof0 = (
        b"\xff\xc0"
        + struct.pack(">H", 11)  # 2 len + 1 prec + 2 h + 2 w + 1 Nf + 3*Nf
        + b"\x08"
        + struct.pack(">HH", 20000, 30000)  # height, width
        + b"\x01"  # 1 component
        + b"\x01\x11\x00"
    )
    return b"\xff\xd8" + app1 + sof0 + b"\xff\xd9"


def main():
    os.makedirs(OUT, exist_ok=True)
    files = {}

    q = quadrant()
    files["quad64x48.png"] = enc(q, format="PNG")
    files["quad64x48.jpg"] = enc(q, format="JPEG", quality=95)
    files["quad64x48.bmp"] = enc(q, format="BMP")
    files["quad64x48.gif"] = enc(q, format="GIF")

    files["tiff_rgb_le.tif"] = enc(q, format="TIFF")
    files["tiff_rgb_be.tif"] = enc(q, format="TIFF", big_endian=True)
    files["tiff_packbits.tif"] = enc(q, format="TIFF", compression="packbits")
    files["tiff_deflate.tif"] = enc(q, format="TIFF", compression="tiff_adobe_deflate")
    # M-085: the classic TIFF LZW (MSB-first, early change) - the scanner
    # and Photoshop "LZW" export variant GDI+ decodes
    files["tiff_lzw.tif"] = enc(q, format="TIFF", compression="tiff_lzw")
    # M-085: the classic TIFF LZW (MSB-first, early change) - the scanner
    # and Photoshop "LZW" export variant GDI+ decodes
    files["tiff_lzw.tif"] = enc(q, format="TIFF", compression="tiff_lzw")
    im16 = Image.new("I;16", (W, H))
    px16 = im16.load()
    for y in range(H):
        for x in range(W):
            base = [0x1F1F, 0xE0E0, 0x8080, 0xFFFF][
                (0 if y < H // 2 else 2) + (0 if x < W // 2 else 1)
            ]
            px16[x, y] = base
    files["tiff_gray16.tif"] = enc(im16, format="TIFF")
    pal = q.convert("P", palette=Image.ADAPTIVE)
    files["tiff_palette.tif"] = enc(pal, format="TIFF")

    portrait = Image.new("RGB", (32, 64))
    ppx = portrait.load()
    for y in range(64):
        for x in range(32):
            ppx[x, y] = RED if x < 16 else BLUE
    files["portrait32x64.png"] = enc(portrait, format="PNG")

    tiny = Image.new("RGB", (8, 6), (10, 200, 30))
    files["tiny8x6.png"] = enc(tiny, format="PNG")

    files["bomb_dims.png"] = bomb_dims_png()

    thumb_im = Image.new("RGB", (24, 18), (9, 99, 199))
    tpx = thumb_im.load()
    for y in range(18):
        for x in range(24):
            tpx[x, y] = (9, 99, 199) if x < 12 else (200, 100, 0)
    thumb_jpeg = enc(thumb_im, format="JPEG", quality=95)
    files["huge_dims_exif_thumb.jpg"] = huge_dims_exif_jpeg(thumb_jpeg)

    small_main = q.resize((48, 36))
    small_bytes = enc(small_main, format="JPEG", quality=95)
    files["exif_thumb_small.jpg"] = b"\xff\xd8" + exif_app1(thumb_jpeg) + small_bytes[2:]

    # S-045: the CR2-shaped RAW fixture - the JPEG preview is the quadrant
    raw_preview = enc(q, format="JPEG", quality=95)
    files["tiff_raw_cr2.bin"] = tiff_raw_cr2(raw_preview)

    # M-080: the byte-heavy pixel-benign middle class - a 4.07 MiB 12-MP
    # JPEG (seeded 8-px color runs, quality 80): the common modern-phone
    # photo shape that fell between the old 4 MiB read budget and the
    # 64 MP pixel guard
    rnd = random.Random(1042)
    noisy = Image.new("RGB", (4000, 3000))
    npx = noisy.load()
    for y in range(3000):
        for x in range(0, 4000, 8):
            v = (rnd.randrange(256), rnd.randrange(256), rnd.randrange(256))
            for dx in range(8):
                npx[x + dx, y] = v
    files["byteheavy12mp.jpg"] = enc(noisy, format="JPEG", quality=80)

    # S-043: the orientation battery - the quadrant JPEG + IFD0 Orientation
    full_jpg = files["quad64x48.jpg"]
    for orient in list(range(1, 9)) + [9]:
        files[f"exif_orient{orient}.jpg"] = b"\xff\xd8" + exif_orient_app1(orient) + full_jpg[2:]

    # S-043: the orientation battery - the quadrant JPEG + IFD0 Orientation
    full_jpg = files["quad64x48.jpg"]
    for orient in list(range(1, 9)) + [9]:
        files[f"exif_orient{orient}.jpg"] = b"\xff\xd8" + exif_orient_app1(orient) + full_jpg[2:]

    full_png = files["quad64x48.png"]
    files["truncated.png"] = full_png[: int(len(full_png) * 0.45)]
    full_jpg = files["quad64x48.jpg"]
    files["truncated.jpg"] = full_jpg[: int(len(full_jpg) * 0.45)]

    files["garbage.bin"] = b"This is obviously not an image file at all.\n" * 4
    files["webp_magic.bin"] = b"RIFF" + struct.pack("<I", 1024) + b"WEBPVP8 " + b"\x00" * 16

    sums = []
    for name, blob in sorted(files.items()):
        path = os.path.join(OUT, name)
        with open(path, "wb") as f:
            f.write(blob)
        sums.append(f"{hashlib.sha256(blob).hexdigest()}  {name}")
        print(f"{name}: {len(blob)} bytes")
    with open(os.path.join(OUT, "SHA256SUMS"), "w") as f:
        f.write("\n".join(sums) + "\n")
    print(f"-> {len(files)} fixtures + SHA256SUMS in {os.path.abspath(OUT)}")


if __name__ == "__main__":
    sys.exit(main())
