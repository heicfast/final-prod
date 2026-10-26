#!/usr/bin/env python3
"""Extract the Btrfs superblock mirror offset table from BtrfsSearchDLL.dll."""
import struct

PATH = "/home/z/my-project/easeus-drw-research/og-codebase/BtrfsSearchDLL.dll"
VA = 0x180018D60

data = open(PATH, "rb").read()

e_lfanew = struct.unpack_from("<I", data, 0x3C)[0]
assert data[e_lfanew:e_lfanew + 4] == b"PE\0\0"
coff = e_lfanew + 4
num_sections = struct.unpack_from("<H", data, coff + 2)[0]
opt_size = struct.unpack_from("<H", data, coff + 16)[0]
opt = coff + 20
magic = struct.unpack_from("<H", data, opt)[0]
image_base = struct.unpack_from("<Q" if magic == 0x20B else "<I", data,
                                opt + (24 if magic == 0x20B else 28))[0]
sections = opt + opt_size
print("image_base=%x sections=%d" % (image_base, num_sections))

rva = VA - image_base
for i in range(num_sections):
    sh = sections + i * 40
    name = data[sh:sh + 8].rstrip(b"\0").decode()
    vsize, vaddr, rawsize, rawptr = struct.unpack_from("<IIII", data, sh + 8)
    if vaddr <= rva < vaddr + max(vsize, rawsize):
        off = rawptr + (rva - vaddr)
        print("section %s rva=%x file_off=%x" % (name, rva, off))
        vals = struct.unpack_from("<12Q", data, off)
        for v in vals:
            if v == 0:
                print("  0 (terminator)")
                break
            print("  %x  (%s)" % (v, "%g GiB" % (v / 2**30)))
        break
