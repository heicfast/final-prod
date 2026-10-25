#!/usr/bin/env python3
"""S3-LOST: MSVC RTTI vtable extractor for the og SearchDLLs (read-only RE probe).

Walks the PE, finds the RTTI CompleteObjectLocator (COL) whose pTypeDescriptor
RVA matches the given TypeDescriptor VA, then finds the vftable (8-byte pointer
to the COL), and dumps vtable slots resolved against a Ghidra functions.txt
index (address -> name). Pure-stdlib, no writes outside stdout.
"""
import struct
import sys


class PE:
    def __init__(self, path):
        self.data = open(path, "rb").read()
        d = self.data
        assert d[:2] == b"MZ", "not a PE"
        e_lfanew = struct.unpack_from("<I", d, 0x3C)[0]
        assert d[e_lfanew:e_lfanew + 4] == b"PE\0\0"
        coff = e_lfanew + 4
        machine, nsec, _, _, _, opt_size, _ = struct.unpack_from("<HHIIIHH", d, coff)
        opt = coff + 20
        magic = struct.unpack_from("<H", d, opt)[0]
        assert magic == 0x20b, "not PE32+ (x64)"
        self.image_base = struct.unpack_from("<Q", d, opt + 24)[0]
        secs = []
        so = opt + opt_size
        for i in range(nsec):
            off = so + i * 40
            name = d[off:off + 8].rstrip(b"\0").decode()
            vsize, va, rsize, ro = struct.unpack_from("<IIII", d, off + 8)
            secs.append((name, va, vsize, ro, rsize))
        self.sections = secs

    def va2off(self, va):
        rva = va - self.image_base
        for name, sva, vsize, ro, rsize in self.sections:
            if sva <= rva < sva + max(vsize, rsize):
                delta = rva - sva
                if delta < rsize:
                    return ro + delta
                return None  # in BSS-ish region
        return None

    def off2va(self, off):
        for name, sva, vsize, ro, rsize in self.sections:
            if ro <= off < ro + rsize:
                return self.image_base + sva + (off - ro)
        return None

    def read(self, va, n):
        o = self.va2off(va)
        if o is None:
            return None
        return self.data[o:o + n]

    def u32(self, va):
        b = self.read(va, 4)
        return struct.unpack("<I", b)[0] if b else None

    def u64(self, va):
        b = self.read(va, 8)
        return struct.unpack("<Q", b)[0] if b else None

    def section_bytes(self, name):
        for n, sva, vsize, ro, rsize in self.sections:
            if n == name:
                return ro, rsize, self.image_base + sva
        return None


def find_col(pe, td_va):
    """Find the CompleteObjectLocator pointing at td_va's TypeDescriptor."""
    td_rva = td_va - pe.image_base
    hits = []
    for secname in (".rdata", ".data"):
        sb = pe.section_bytes(secname)
        if not sb:
            continue
        ro, rsize, sva = sb
        blob = pe.data[ro:ro + rsize]
        start = 0
        while True:
            i = blob.find(struct.pack("<I", td_rva), start)
            if i < 0:
                break
            # candidate COL base = i - 0x0c
            col_off = i - 0x0c
            if col_off >= 0:
                sig = struct.unpack_from("<I", blob, col_off)[0]
                pself = struct.unpack_from("<I", blob, col_off + 0x14)[0]
                col_rva = (sva - pe.image_base) + col_off
                if sig == 1 and pself == col_rva:
                    hits.append((secname, pe.image_base + col_rva))
            start = i + 1
    return hits


def find_vftable(pe, col_va):
    """Find the 8-byte pointer to col_va; vftable = that location + 8."""
    hits = []
    pat = struct.pack("<Q", col_va)
    for secname in (".rdata", ".data"):
        sb = pe.section_bytes(secname)
        if not sb:
            continue
        ro, rsize, sva = sb
        blob = pe.data[ro:ro + rsize]
        start = 0
        while True:
            i = blob.find(pat, start)
            if i < 0:
                break
            hits.append(sva + i + 8)  # vftable start (sva already VA)
            start = i + 1
    return hits


def load_functions(path):
    fns = {}
    for line in open(path, encoding="utf-8", errors="replace"):
        parts = line.split("\t")
        if not parts:
            continue
        try:
            addr = int(parts[0], 16)
        except ValueError:
            continue
        sig = parts[2].strip() if len(parts) > 2 else "?"
        fns[addr] = sig
    return fns


def dump_vtable(pe, vftable_va, fns, nslots, label):
    print(f"  {label}: vftable @ {vftable_va:#x}")
    for i in range(nslots):
        fn_va = pe.u64(vftable_va + i * 8)
        if fn_va is None:
            print(f"    slot {i}: <unreadable>")
            continue
        name = fns.get(fn_va, "<no Ghidra symbol>")
        print(f"    slot {i}: {fn_va:#x}  {name}")


def main():
    dll = sys.argv[1]
    ftxt = sys.argv[2]
    pe = PE(dll)
    fns = load_functions(ftxt)
    print(f"== {dll} (image base {pe.image_base:#x}) ==")
    for spec in sys.argv[3:]:
        label, tdva, nslots = spec.split(":")
        td_va = int(tdva, 16)
        n = int(nslots)
        cols = find_col(pe, td_va)
        if not cols:
            print(f"  {label}: no COL found for TD {td_va:#x}")
            continue
        for _, col_va in cols:
            vfts = find_vftable(pe, col_va)
            if not vfts:
                print(f"  {label}: COL @ {col_va:#x} but no vftable pointer")
                continue
            for v in vfts:
                dump_vtable(pe, v, fns, n, label)


if __name__ == "__main__":
    main()
