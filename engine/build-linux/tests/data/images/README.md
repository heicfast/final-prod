# tests/data/images — real-world image fixtures (S-030)

Generated with qemu-img / qemu-io 10.0.13 (Debian) — the compatibility
proof layer for `tests/unit/test_image.cpp` (Image.Qemu*Compat). The
in-memory builders in that test cover structure semantics; these files
prove the parsers read what real tools actually write.

```
qemu-img create -f vmdk disk1.vmdk 4M
qemu-img create -f vpc  disk1.vhd  4M
qemu-img create -f vhdx disk1.vhdx 4M
qemu-io -c "write -P 0xAB 0 64k" -c "write -P 0xCD 1M 64k" \
        -c "write -P 0xEF 3M 8k" -f <fmt> disk1.<ext>
```

Written regions: 0xAB @0..64K, 0xCD @1MiB..+64K, 0xEF @3MiB..+8K; all
other bytes read as zeros (sparse). The VHDX carries qemu's post-close
log entry (applied data, valid sequence) - the read-only overlay parse
exercises that path on every run.
