#!/usr/bin/env python3
"""compare_og_rsf.py - extract the found-file tree from an EaseUS .rsf
scan session into a CSV comparable with `rrecov tree --csv` output.

Grammar (reverse-engineered record 4, validated in the easeus-drw-research
corpus; see artifacts/SaveScan/rsf-format--scan-session.md and
scripts/rsf_record4_grammar.py):
  container: magic[16] + header, records @0x8C chained by +0x24+chain;
  each record payload = chunked stream (u16 header: bit15=zlib,
  bits0-11 = size-1). Record 4 (GUID 6cf208d2...) = the BFS node tree:
  node: 34-byte base + (marker 1) type_flags, 40-byte stat blob, name;
  content objects per FS family (0=NTFS, 1=FAT, 3=RAW/carved, 4=exFAT,
  8=EXT4, 9=generic, 0xC=ZFS, 0xA=ReFS) with name/size/flags/dataruns.

Output CSV: path,size,origin,deleted,confidence,extent
  path        reconstructed from BFS depth + names ('/'-separated)
  size        logical size from the content object (0 when unknown)
  origin      structured|carved (content type 3 = carved)
  deleted     heuristics: NTFS/FAT content flags (deleted markers live in
              the node flags/state area; the corpus reader grades at
              recoverability time) - 'unknown' when not decodable
  confidence  '' (og grades in a separate pass)

Licensed under the MIT license. See LICENSE.
"""

import csv
import struct
import sys
import zlib

MAGIC = bytes.fromhex("eaf4b31b0543b64ab4b8244a33d913bb")
REC4 = bytes.fromhex("6cf208d28c07344e9d71d91f535f7b57")  # record-4 tree


class R:
    __slots__ = ("b", "o", "names", "rows", "stats")

    def __init__(self, buf):
        self.b = buf
        self.o = 0
        self.names = []  # (depth, name)
        self.rows = []
        self.stats = {}

    def take(self, n):
        if self.o + n > len(self.b):
            raise EOFError(f"take({n}) past end at {self.o:#x}")
        d = self.b[self.o : self.o + n]
        self.o += n
        return d

    def u8(self):
        return self.take(1)[0]

    def u16(self):
        return struct.unpack("<H", self.take(2))[0]

    def u32(self):
        return struct.unpack("<I", self.take(4))[0]

    def u64(self):
        return struct.unpack("<Q", self.take(8))[0]


def load_record4(path):
    data = open(path, "rb").read()
    if data[:16] != MAGIC:
        raise ValueError("not an .rsf file (bad magic)")
    off = 0x8C
    while off + 0x24 <= len(data):
        hdr = data[off : off + 0x24]
        if hdr[:16] != MAGIC:
            break
        sid = hdr[16:32]
        chain = struct.unpack("<I", hdr[32:36])[0]
        if sid == REC4:
            out = bytearray()
            p = off + 0x24
            end = p + chain
            while p + 2 <= end:
                h = struct.unpack_from("<H", data, p)[0]
                total = (h & 0xFFF) + 3
                dsize = total - 2
                chunk = data[p + 2 : p + 2 + dsize]
                if h & 0x8000:
                    try:
                        out += zlib.decompress(chunk)
                    except zlib.error:
                        out += chunk
                else:
                    out += chunk
                p += total
            return bytes(out)
        off = off + 0x24 + chain
    return None


def datarun(r):
    n = r.u32()
    r.take(n)
    r.u16()
    r.stats["datarun"] = r.stats.get("datarun", 0) + 1


def content(r, node_name, depth, path_stack):
    """Parse one IFileProperties content object; emits a row."""
    t = r.u32()
    r.stats[f"content_type_{t:#x}"] = r.stats.get(f"content_type_{t:#x}", 0) + 1
    name = node_name
    size = 0
    deleted = "unknown"
    if t == 0:  # CNTFSFileProperties
        n = r.u16()
        if n:
            name = r.take(n).decode("utf-8", errors="replace")
        size = r.u64()
        flags = r.u16()
        r.u64(); r.u64(); r.u64(); r.u8(); r.u64(); r.u32(); r.u32()
        if r.u32():
            datarun(r)
        cnt = r.u64()
        if cnt:
            if cnt > 1_000_000:
                raise ValueError("absurd runlist")
            for _ in range(cnt):
                r.u64(); r.u64()
        deleted = "deleted" if flags & 0x8000 else "active"
    elif t == 1:  # CFATFileProperties
        n = r.u16()
        if n:
            name = r.take(n).decode("utf-8", errors="replace")
        size = r.u64()
        flags = r.u16()
        r.u64(); r.u64(); r.u64(); r.u8(); r.u32(); r.u32()
        if r.u32():
            datarun(r)
        deleted = "deleted" if flags & 0x8000 else "active"
    elif t == 3:  # CRAWFileProperties (carved)
        n = r.u8()
        if n:
            name = r.take(n).decode("utf-8", errors="replace")
        m2 = r.u8()
        if m2:
            r.take(m2)
        r.u64()
        flags = r.u16()
        r.u8()
        r.u32()
        m3 = r.u8()
        if m3:
            r.take(m3)
        r.u64()
        size = r.u64()
        r.u32()
        r.u32()
        datarun(r)
        deleted = "deleted"
    elif t == 4:  # CExFatFileProperties
        r.u16()
        size = r.u64()
        flags = r.u16()
        r.u64(); r.u64(); r.u64(); r.u8(); r.u32(); r.u32()
        if r.u32():
            datarun(r)
        deleted = "deleted" if flags & 0x8000 else "active"
    elif t == 8:  # CEXT4FileProperties
        n = r.u16()
        if n:
            name = r.take(n).decode("utf-8", errors="replace")
        size = r.u64()
        flags = r.u16()
        r.u64(); r.u64(); r.u64(); r.u8(); r.u64()
        if r.u32():
            datarun(r)
        deleted = "deleted" if flags & 0x8000 else "active"
    elif t == 9:  # CGenericFileProperties
        n = r.u32()
        if n:
            name = r.take(n).decode("utf-8", errors="replace")
        flags = r.u16()
        r.u16()
    elif t == 5:  # HFSPlusFileProperties (u32 name; count x 16B extents)
        n = r.u32()
        if n:
            if n > 65536:
                raise ValueError("absurd hfs+ name length")
            name = r.take(n).decode("utf-8", errors="replace")
        size = r.u64()
        flags = r.u16()
        r.u64(); r.u64(); r.u64()  # create/write/access
        r.u16()                    # secondary flags
        r.u32()                    # type id
        r.u8()                     # deleted/section marker
        cnt = r.u32()
        if cnt:
            if cnt > 0x1000:
                raise ValueError("absurd hfs+ extent count")
            r.take(cnt * 16)
        deleted = "deleted" if flags & 0x20 else "active"
    elif t == 6:  # RefsFileProperties (u16 name; anchor between size+times)
        n = r.u16()
        if n:
            if n > 256:
                raise ValueError("absurd refs name length")
            name = r.take(n).decode("utf-8", errors="replace")
        size = r.u64()
        r.u32()                    # objectId / parent anchor
        r.u64(); r.u64(); r.u64()  # create/write/access
        r.u8()                     # deleted/section marker
        if r.u32():
            datarun(r)
        deleted = "deleted" if flags & 0x20 else "active"
    elif t == 0x0C:  # ZfsFileProperties
        r.u64()
        n = r.u64()
        if n:
            r.take(n)
        r.u64()
        flags = r.u16()
        r.u64(); r.u64(); r.u64(); r.u16()
        r.u32()
    else:
        raise ValueError(f"unhandled content type {t:#x} at {r.o:#x}")

    if name:
        path = "/".join(path_stack[:depth] + [name]) if depth else name
        r.rows.append(
            {
                "path": path,
                "size": size,
                "origin": "carved" if t == 3 else "structured",
                "deleted": deleted,
                "confidence": "",
                "extent": "",
            }
        )
    return flags


def node(r, marker, depth, path_stack):
    # 34-byte base
    r.u64(); r.u64()
    r.u8(); r.u8(); r.u8(); r.u32(); r.u8()
    r.u16(); r.u16(); r.u16(); r.u8(); r.u8(); r.u8(); r.u8()
    r.stats["nodes"] = r.stats.get("nodes", 0) + 1
    name = ""
    if marker == 1:
        r.u64()  # type flags
        r.take(40)  # stat blob
        r.u64(); r.u32(); r.u32()
        namelen = r.u32()
        r.u32(); r.u32(); r.u32()
        r.u32(); r.u64(); r.u64(); r.u8(); r.u8()
        if namelen:
            if namelen > 4096:
                raise ValueError("absurd name length")
            name = r.take(namelen).decode("utf-8", errors="replace")
            if len(path_stack) <= depth:
                path_stack.append(name)
            else:
                path_stack[depth] = name
            r.names.append((depth, name))
        vc = r.u32()
        if vc:
            if vc > 100_000:
                raise ValueError("absurd vector count")
            r.stats["vector_children"] = r.stats.get("vector_children", 0) + vc
            for _ in range(vc):
                node(r, 1, depth + 1, path_stack)
    return name


def parse_record4(buf):
    r = R(buf)
    flag = r.u8()
    if flag == 0:
        r.u32(); r.u32(); r.u32()
    else:
        r.u32()
    rootn = r.u32()
    r.stats["roots"] = rootn
    for i in range(rootn):
        m = r.u32()
        if m not in (0, 1):
            raise ValueError(f"root {i}: marker {m:#x} not 0/1")
        path_stack = []
        nm = node(r, m, 0, path_stack)
        cf = content(r, nm, 0, path_stack)
        # roots are queued unconditionally by the writer
        q = [1]
        while q:
            d = q.pop(0)
            while True:
                cm = r.u32()
                if cm == 0xFFFFFFFF:
                    break
                if cm not in (0, 1):
                    raise ValueError(f"bad child marker {cm:#x}")
                nm = node(r, cm, d, path_stack)
                cf = content(r, nm, d, path_stack)
                if cf & 0x10:
                    q.append(d + 1)
    return r


def main():
    if len(sys.argv) < 3:
        print("usage: compare_og_rsf.py <session.rsf> <out.csv>", file=sys.stderr)
        return 2
    buf = load_record4(sys.argv[1])
    if buf is None:
        print("no record-4 in session", file=sys.stderr)
        return 3
    print(f"record-4 decompressed: {len(buf):,} bytes", file=sys.stderr)
    try:
        r = parse_record4(buf)
    except (ValueError, EOFError, struct.error) as e:
        print(f"parse stopped: {e}", file=sys.stderr)
        return 4
    with open(sys.argv[2], "w", encoding="utf-8", newline="") as fh:
        w = csv.DictWriter(
            fh, fieldnames=["path", "size", "origin", "deleted", "confidence", "extent"]
        )
        w.writeheader()
        for row in r.rows:
            w.writerow(row)
    print(
        f"rows={len(r.rows)} names={len(r.names)} stats={r.stats}",
        file=sys.stderr,
    )
    print(f"consumed {r.o:,} / {len(buf):,} bytes", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
