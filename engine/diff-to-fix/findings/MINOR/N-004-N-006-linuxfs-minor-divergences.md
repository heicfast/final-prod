
## Resolution (N-005 portion)

**Status:** FIXED
**Fixed in commit:** this session's HFS+ batch (validate_volume_header bound 64 KiB → 16 MiB + scan-level product sanity; D-53)
**Verified by:** `HfsPlus.LargeBlockSizeAccepted` — fail-before on the pre-batch source (1 MiB-block VH rejected before, validates after); the zero-blockSize floor pin and the scan-level product-sanity case pass both sides as designed. Suite 392/392.
**Resolution notes:** Upper bound relaxed to (1 << 24) per the finding's fix plan, with the total-blocks product sanity (block × total ≤ 2× device, the NTFS P1-3 mirror the plan asked for) at the scan site where the device size is known; the ≥ 512 floor stays as the registered deliberate hardening (blockSize 0 is geometrically unusable — the og's mask-tolerance of 0 would divide by zero). N-004/N-006 (the other two findings in this group file) remain open — they are separate modules (ext4 transport layering / Btrfs nodesize) and are NOT part of this batch.
