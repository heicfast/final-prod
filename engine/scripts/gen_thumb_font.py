#!/usr/bin/env python3
"""gen_thumb_font.py - generate the embedded ASCII bitmap font for the
text-snapshot thumbnail renderer (N-144, diff-to-fix).

Renders ASCII 32..126 as a 1-bpp 6x12 glyph table (one byte per row, the
low 6 bits used) for embedding in src/core/preview/cr_thumbnail_font.cpp.
The thumbnail renderer draws the og's text-snapshot content (the first
<=100 bytes of a TxtViewFiles-eligible file, DRWUI FUN_1403d88e0's
cap/BOM/empty-check sequence) with this font - the og draws Arial 13 via
GDI (FUN_1403d83f0(param_1,"Arial",0xd)); a fixed bitmap table is the
core's dependency-free equivalent (the exact Arial rasterization is a
Windows-side artifact the core cannot reproduce without the font file;
the CAPABILITY - a readable content-preview thumbnail - is the parity
surface, and the glyph metric is documented in the module header).

Base: DejaVu Sans Mono (Bitstream Vera license family - free, permissive
redistribution and derivative works; the name reservation does not apply
to this derived bitmap table, which is not distributed as a font). The
table is a build-time artifact of the rasterized ASCII range only.

Regenerate: python3 scripts/gen_thumb_font.py
"""

from PIL import Image, ImageDraw, ImageFont
import sys

W, H = 6, 12
FIRST, LAST = 32, 126  # space .. tilde

FONT_CANDIDATES = [
    "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
]


def load_font():
    for path in FONT_CANDIDATES:
        try:
            # the face at 8x the cell height: a 12px cell renders from a
            # 96px face, so the averaged-down ink lands at true cell scale
            return ImageFont.truetype(path, H * 8), path
        except OSError:
            continue
    return ImageFont.load_default(), "PIL default bitmap font"


def render_glyph(font, ch):
    """Render one glyph into a WxH 1-bpp cell, centered horizontally.

    8x supersampled GRAYSCALE render -> crop to ink -> average-down to
    the cell grid -> threshold at 40% ink. Grayscale averaging keeps the
    thin strokes that a 1-bit nearest-neighbor downscale would lose."""
    img = Image.new("L", (320, 320), 0)
    d = ImageDraw.Draw(img)
    d.text((32, 32), ch, font=font, fill=255)
    bbox = img.getbbox()
    if bbox is None:
        return [0] * H  # blank glyph (space renders empty)
    x0, y0, x1, y1 = bbox
    ink = img.crop((x0, y0, x1, y1))
    tw = max(1, (x1 - x0) // 8)
    th = max(1, (y1 - y0) // 8)
    small = ink.resize((tw, th), Image.LANCZOS)
    cell = Image.new("1", (W, H), 0)
    gx = max(0, min(W - tw, (W - tw) // 2))
    # baseline anchor: the ink's bottom sits at row H-2 (a 12px cell with
    # a 2px descent - descenders reach row H-1)
    gy = max(0, min(H - th, (H - 2) - th))
    cell.paste(small.point(lambda v: 255 if v > 96 else 0), (gx, gy))
    rows = []
    px = cell.load()
    for y in range(H):
        b = 0
        for x in range(W):
            if px[x, y]:
                b |= 1 << (W - 1 - x)
        rows.append(b)
    return rows


def main():
    font, used = load_font()
    glyphs = []
    for code in range(FIRST, LAST + 1):
        rows = render_glyph(font, chr(code))
        assert len(rows) == H
        glyphs.append(rows)
    out = sys.argv[1] if len(sys.argv) > 1 else "src/core/preview/cr_thumbnail_font.cpp"
    with open(out, "w", encoding="utf-8") as f:
        f.write("/* cr_thumbnail_font.cpp - the embedded ASCII bitmap font for the\n")
        f.write(" * text-snapshot thumbnail renderer (N-144, diff-to-fix).\n")
        f.write(" *\n")
        f.write(" * GENERATED FILE - do not edit by hand; regenerate with\n")
        f.write(" * scripts/gen_thumb_font.py (PIL rasterization of ASCII 32..126\n")
        f.write(f" * at the 6x12 cell, base font: {used} - the Bitstream Vera\n")
        f.write(" * license family permits redistribution and derivative works; this\n")
        f.write(" * derived 1-bpp glyph table is not distributed as a font and does\n")
        f.write(" * not carry the font's reserved name).\n")
        f.write(" *\n")
        f.write(" * The table: one glyph per ASCII code 32..126, each glyph = 12 row\n")
        f.write(" * bytes, the low 6 bits of each row = the pixel columns (bit 5 =\n")
        f.write(" * the leftmost column). The renderer (cr_thumbnail.cpp's text\n")
        f.write(" * snapshot) draws white-background/black-glyph cells at the 6x12\n")
        f.write(" * metric - the og draws Arial 13 through GDI (DRWUI\n")
        f.write(" * FUN_1403d83f0(param_1,\"Arial\",0xd), :652947-948); a fixed bitmap\n")
        f.write(" * table is the dependency-free equivalent (the exact Arial\n")
        f.write(" * rasterization is a Windows font artifact the core cannot embed).\n")
        f.write(" *\n")
        f.write(" * Licensed under the MIT license. See LICENSE.\n")
        f.write(" */\n\n")
        f.write("#include \"cr/preview/cr_thumbnail.h\"\n\n")
        f.write("namespace cr {\nnamespace preview {\n\n")
        f.write("namespace {\n\n")
        f.write("constexpr int kFontW = 6;\nconstexpr int kFontH = 12;\n")
        f.write("constexpr int kFontFirst = 32;\nconstexpr int kFontLast = 126;\n\n")
        f.write("/* [code - 32][row], low 6 bits per row (bit 5 = leftmost). */\n")
        f.write("constexpr unsigned char kGlyphs[95][12] = {\n")
        for i, rows in enumerate(glyphs):
            code = FIRST + i
            commented = chr(code) if 33 <= code <= 126 else "space"
            f.write("    {%s},  /* %s */\n" %
                    (", ".join("0x%02x" % r for r in rows), commented))
        f.write("};\n\n")
        f.write("} // namespace\n\n")
        f.write("/* exposed for the renderer + tests (the metric constants and the\n")
        f.write(" * glyph lookup; non-printable/out-of-range codes answer nullptr -\n")
        f.write(" * the renderer renders a blank cell for them) */\n")
        f.write("const unsigned char* thumbnail_font_glyph(unsigned int code) {\n")
        f.write("    if (code < unsigned(kFontFirst) || code > unsigned(kFontLast))\n")
        f.write("        return nullptr;\n")
        f.write("    return kGlyphs[code - unsigned(kFontFirst)];\n")
        f.write("}\n\n")
        f.write("unsigned thumbnail_font_cell_w() { return unsigned(kFontW); }\n")
        f.write("unsigned thumbnail_font_cell_h() { return unsigned(kFontH); }\n\n")
        f.write("} // namespace preview\n} // namespace cr\n")
    print(f"wrote {out}: 95 glyphs at {W}x{H} (base: {used})")


if __name__ == "__main__":
    main()
