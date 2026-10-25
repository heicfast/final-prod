#!/usr/bin/env python3
"""Extract DAT_1800c4960 (GPT-entry LDM GUID in LdmManager.mo) and
DAT_180036630/0x36640 (Partition.mo) — decode both."""
import struct

def sections(data, pe_off):
    nsec = struct.unpack_from("<H", data, pe_off + 6)[0]
    opt_size = struct.unpack_from("<H", data, pe_off + 20)[0]
    sec0 = pe_off + 24 + opt_size
    image_base = struct.unpack_from("<Q", data, pe_off + 24 + 24)[0]
    secs = []
    for i in range(nsec):
        o = sec0 + i * 40
        name = data[o:o+8].rstrip(b"\0").decode()
        vsize, vaddr, rsize, roff = struct.unpack_from("<IIII", data, o + 8)
        secs.append((name, vaddr, vsize, roff, rsize))
    return image_base, secs

def rva2off(secs, rva):
    for name, vaddr, vsize, roff, rsize in secs:
        if vaddr <= rva < vaddr + max(vsize, rsize):
            return roff + (rva - vaddr)
    return None

def guid_mixed(b):
    d1, d2, d3 = struct.unpack_from("<IHH", b, 0)
    tail = b[8:]
    return f"{d1:08x}-{d2:04x}-{d3:04x}-{tail[:2].hex()}-{tail[2:].hex()}"

P_LDM = "/home/z/audit/easeus-drw-research/og-codebase/PartitionRecovery/LdmManager.mo"
P_PART = "/home/z/audit/easeus-drw-research/og-codebase/PartitionRecovery/Partition.mo"

for path, rvas in [(P_LDM, [0xC4960, 0xC4950, 0xC4970]), (P_PART, [0x36630, 0x36640])]:
    data = open(path, "rb").read()
    pe_off = struct.unpack_from("<I", data, 0x3C)[0]
    ib, secs = sections(data, pe_off)
    for rva in rvas:
        off = rva2off(secs, rva)
        if off is None:
            print(f"{path.split('/')[-1]} RVA {rva:#x}: NOT MAPPED")
            continue
        blob = data[off:off+16]
        print(f"{path.split('/')[-1]} DAT_{ib+rva:#x} (RVA {rva:#x}): {blob.hex()}")
        if len(blob) == 16:
            print(f"   mixed-endian GUID: {guid_mixed(blob)}")

# Also scan LdmManager.mo for ALL canonical + og-variant LDM GUID byte sequences
canon_meta = bytes.fromhex("aac808589f8e874caea2af0aa3c41687")
canon_data = bytes.fromhex("a0609baf4e387d4f8fa5b1c3d6a9e2b0")
og_meta = bytes.fromhex("aac808588f7ee04285d2e1e90434cfb3")
og_data = bytes.fromhex("a0609baf3114624fbc683311714a69ad")
for name, needle in [("canon-meta", canon_meta), ("canon-data", canon_data),
                     ("og-meta", og_meta), ("og-data", og_data)]:
    for path in [P_LDM, P_PART]:
        data = open(path, "rb").read()
        cnt = data.count(needle)
        print(f"{path.split('/')[-1]} contains {name}: {cnt} hits")
