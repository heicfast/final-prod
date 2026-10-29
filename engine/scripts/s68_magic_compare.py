#!/usr/bin/env python3
"""S68: robust og-registry vs core-registry magic comparison.
Core rows: parse the kTable[] initializer structurally (balanced braces).
Og keys: the 66 extracted bindings from RAWSearchDLL_decompiled.c."""
import re

t = open("/home/z/my-project/audit/core_recovery/src/core/carve/cr_registry.cpp").read()

# Extract the kTable initializer body
i = t.index("const Signature kTable[] = {")
j = t.index("\n};", i)
body = t[i:j]

# Structural parse: each row starts with {FMT_
rows = []
for m in re.finditer(r"\{FMT_(\w+),\s*\"([^\"]*)\",\s*\{([^}]*)\},\s*(\d+),", body):
    fmt, ext, magic, mlen = m.group(1), m.group(2), m.group(3), int(m.group(4))
    # read forward to the validator + desc: find the tail of this row
    tail = body[m.end():m.end()+400]
    vm = re.search(r"(validate_\w+)", tail)
    dm = re.search(r'"([^"]*)"}', tail)
    magic_bytes = bytes(int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]{2})", magic))
    rows.append((fmt, ext, magic_bytes, mlen,
                 vm.group(1) if vm else "?", dm.group(1) if dm else "?"))
print(f"parsed {len(rows)} core rows")

og = [
 (0x4f524949, "ORF IIRO"), (0x53524949, "ORF IIRS"), (0x494a5546, "RAF FUJI"),
 (0x524f4d4d, "ORF MMOR"), (0x1a4949, "MRW II"), (0x1a4d4d, "MRW MM"),
 (0x31685a42, "DMG BZh1"), (0x62564f46, "X3F FOVb"),
 (0x2a004d4d, "TIFF MM"), (0x2a4949, "TIFF II"), (0x554949, "TIFF/EP IIU"),
 (0x46445025, "PDF"), (0xe0ffd8ff, "JPG E0"), (0xe1ffd8ff, "JPG E1"),
 (0xdbffd8ff, "JPG DB"), (0xeeffd8ff, "JPG EE"),
 (0x6d783f3c, "XML <?xm"), (0x3cbfbbef, "XML BOM"), (0x6776733c, "SVG <svg"),
 (0x1cd, "HTML 1cd"), (0xb3010000, "MPEGES B3"), (0xba010000, "MPEGPS BA"),
 (0x5367674f, "OGG"), (0x464d522e, "RM"), (0x342b0e06, "MXF"),
 (0x75b22630, "ASF"), (0x0d302020, "DXF"), (0x2043414d, "APE"),
 (0x11c, "FH11"), (0x694c5153, "SQLi/FCPEvt"), (0xc6d3d0c5, "AI/EPS"),
 (0x30314341, "DWG AC10"), (0x38464947, "GIF8"), (0x6468544d, "MID MThd"),
 (0x474e5089, "PNG"), (0x53504238, "PSD 8BPS"), (0x74725c7b, "RTF"),
 (0x6e696150, "PSP Pain"), (0x21726152, "RAR4 Rar!"), (0x66757453, "SIT Stuf"),
 (0x7b5c52e4, "ONE E4"), (0x677077, "WPG"), (0x36464d4a, "DBX JMF6"),
 (0xfe12adcf, "DBX CFAD"), (0x2c3489f7, "DBX 2C34"),
 (0x04034b50, "ZIP PK"), (0x435057ff, "WPD FF WPC"), (0x90001, "WMF std"),
 (0x9ac6cdd7, "WMF pl"), (0x100, "ICO"), (0x46464952, "RIFF"),
 (0x4e444221, "PST !BDN"), (0xe011cfd0, "OLE"), (0x2da5db, "LOWDOC DBA5"),
 (0x2ea694, "LOWDOC 94A6"), (0x10000, "CUR"), (0x35f3f, "HLP 3F5F"),
 (0x4d524f46, "AIF FORM"), (0x37545043, "CPT7"), (0x38545043, "CPT8"),
 (0x39545043, "CPT9"), (0x4643534d, "CAB MSCF"), (0x6663736d, "CAB mscf"),
 (0x646e732e, "AU .snd"), (0xa3df451a, "MKV EBML"), (0x46535449, "CHM ITSF"),
 (0x4d524d00, "MRW 0MRM"),
]
missing = []
for k, name in og:
    kb = k.to_bytes(4, "little")
    # core magic must be a PREFIX of the og key (core rows may key fewer bytes)
    hits = [(fmt, val, desc, mb) for fmt, ext, mb, mlen, val, desc in rows
            if mb and kb.startswith(mb)]
    if not hits:
        missing.append((name, kb))
        print(f"  *** NO CORE ROW: {name:>14} key={kb.hex()}")
    else:
        fmts = {h[0] for h in hits}
        print(f"  ok {name:>14} -> {','.join(sorted(fmts))}")
print(f"\nOG keys with no core row: {len(missing)}")
