#!/usr/bin/env python3
"""Extract DAT_180036630 / DAT_180036640 (LDM GUID constants) from Partition.mo
and compare against canonical LDM GUIDs (both byte orders)."""
import struct

P = "/home/z/audit/easeus-drw-research/og-codebase/PartitionRecovery/Partition.mo"
data = open(P, "rb").read()

# PE: find section headers to map RVA -> file offset
pe_off = struct.unpack_from("<I", data, 0x3C)[0]
assert data[pe_off:pe_off+4] == b"PE\0\0"
nsec = struct.unpack_from("<H", data, pe_off + 6)[0]
opt_size = struct.unpack_from("<H", data, pe_off + 20)[0]
sec0 = pe_off + 24 + opt_size
image_base = struct.unpack_from("<Q", data, pe_off + 24 + 24)[0]
print(f"PE at {pe_off:#x}, {nsec} sections, image_base {image_base:#x}")
secs = []
for i in range(nsec):
    o = sec0 + i * 40
    name = data[o:o+8].rstrip(b"\0").decode()
    vsize, vaddr, rsize, roff = struct.unpack_from("<IIII", data, o + 8)
    secs.append((name, vaddr, vsize, roff, rsize))
    print(f"  {name}: vaddr {vaddr:#x} vsize {vsize:#x} rawoff {roff:#x}")

def rva2off(rva):
    for name, vaddr, vsize, roff, rsize in secs:
        if vaddr <= rva < vaddr + max(vsize, rsize):
            return roff + (rva - vaddr)
    return None

for rva in (0x36630, 0x36640, 0x36620, 0x36650):
    off = rva2off(rva)
    if off is None:
        print(f"RVA {rva:#x}: NOT MAPPED")
        continue
    blob = data[off:off+16]
    print(f"DAT_{image_base+rva:#x} (RVA {rva:#x}, file {off:#x}): {blob.hex()}")
    # decode as mixed-endian GUID
    if len(blob) == 16:
        d1, d2, d3 = struct.unpack_from("<IHH", blob, 0)
        tail = blob[8:]
        guid = f"{d1:08x}-{d2:04x}-{d3:04x}-{tail[:2].hex()}-{tail[2:].hex()}"
        print(f"   as mixed-endian GUID: {guid}")

# canonical LDM GUIDs for comparison
canon = {
    "LDM metadata": "5808c8aa-8e9f-4c87-aea2-af0aa3c41687",
    "LDM data":     "af9b60a0-384e-4f7d-8fa5-b1c3d6a9e2b0",
}
print("\ncanonical:", canon)
