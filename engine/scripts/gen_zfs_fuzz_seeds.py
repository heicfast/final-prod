#!/usr/bin/env python3
"""gen_zfs_fuzz_seeds.py - structured seeds for the fuzz_zfs smoke (S-034).

Builds minimal OpenZFS-shaped pool images exercising the NEW ring-walk
paths: multiple valid uberblocks at distinct txgs (the per-state walk),
duplicate txgs across labels (the distinct-txg dedup), zeroed slots (the
skip), torn rootbps (the unopenable-state fallback), and BE-magic slots
(the refusal). Feeds them to the standalone smoke driver.
"""
import os
import struct
import sys

IMG = 16 * 1024 * 1024
LABEL = 256 * 1024
RING = 128 * 1024  # label + 128K


def ub(off, txg, root_offset, version=5000, magic=0x00BAB10C):
    b = bytearray(1024)
    struct.pack_into("<Q", b, 0, magic)
    struct.pack_into("<Q", b, 8, version)
    struct.pack_into("<Q", b, 16, txg)
    struct.pack_into("<Q", b, 32, 1700000000)
    # rootbp at +40: DVA0 {w0: asize>>9, w1: offset>>9}, prop at +88 rel bp
    bp = bytearray(128)
    struct.pack_into("<Q", bp, 0, (512 >> 9))          # asize 512
    struct.pack_into("<Q", bp, 8, root_offset >> 9)    # offset
    prop = ((1 - 1) & 0xFFFF) | (((1 - 1) & 0xFFFF) << 16)  # 512B l/psize
    struct.pack_into("<Q", bp, 48, prop)
    struct.pack_into("<Q", bp, 72, txg)                # birth
    b[40:168] = bp
    return bytes(b)


def base_pool(ring_writes):
    d = bytearray(IMG)
    for li in range(4):
        base = li * LABEL if li < 2 else IMG - (4 - li) * LABEL
        # a plausible MOS block at 0x80000-ish so rootbps can land somewhere
        for i, (slot, blob) in enumerate(ring_writes):
            d[base + RING + slot * 1024: base + RING + (slot + 1) * 1024] = blob
    # garbage MOS-ish content at 0x80000 (reads succeed, parse fails)
    d[0x80000:0x80200] = b"\xde\xad\xbe\xef" * 128
    return bytes(d)


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    seeds = {}
    # 1. single state (the classic)
    seeds["single"] = base_pool([(0, ub(0, 77, 0x80000))])
    # 2. two distinct txgs (the per-state walk)
    seeds["two-txg"] = base_pool([(0, ub(0, 77, 0x80000)), (1, ub(0, 78, 0x80000))])
    # 3. same txg in slots 0/1 (consecutive dedup)
    seeds["dup-txg"] = base_pool([(0, ub(0, 77, 0x80000)), (5, ub(0, 77, 0x80000))])
    # 4. torn newest (rootbp out of range) + healthy older
    seeds["torn-newest"] = base_pool([(0, ub(0, 77, 0x80000)),
                                      (3, ub(0, 90, IMG * 4))])
    # 5. big-endian magic slot (the refusal) beside valid ones
    be = bytearray(ub(0, 77, 0x80000))
    struct.pack_into(">Q", be, 0, 0x00BAB10C)
    seeds["be-mixed"] = base_pool([(0, ub(0, 77, 0x80000)), (2, bytes(be))])
    # 6. many states (ring saturation)
    seeds["many"] = base_pool([(i, ub(0, 60 + i, 0x80000)) for i in range(16)])
    # 7. zeroed slot between valid ones (the skip)
    seeds["gap"] = base_pool([(0, ub(0, 77, 0x80000)), (2, ub(0, 79, 0x80000))])
    # 8. txg 0 slot (guarded as invalid)
    seeds["txg-zero"] = base_pool([(0, ub(0, 0, 0x80000)), (1, ub(0, 77, 0x80000))])
    # 9-16. mutations of two-txg (flips in uberblock/label bytes)
    src = bytearray(seeds["two-txg"])
    import random
    rng = random.Random(20261026)
    for k in range(8):
        m = bytearray(src)
        for _ in range(24):
            pos = rng.randrange(len(m))
            m[pos] ^= 1 << rng.randrange(8)
        seeds[f"mut{k}"] = bytes(m)
    for name, blob in seeds.items():
        with open(os.path.join(outdir, name), "wb") as f:
            f.write(blob)
        print(f"{name}: {len(blob)} bytes")


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "/tmp/zfs-seeds")
