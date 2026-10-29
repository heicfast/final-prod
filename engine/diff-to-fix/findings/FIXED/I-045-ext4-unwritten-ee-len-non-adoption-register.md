# INFORMATIONAL I-045 — Og-defect non-adoption register, batch #3: the ext4 unwritten-extent `ee_len` is multiplied raw in the og (overflow-class garbage run lengths); the core is kernel-correct — register the deliberate non-adoption

**Severity:** INFORMATIONAL
**Module:** ext4 (extent decode)
**Parameters affected:** Q68 (documentation accuracy of the deliberate-divergence register), B2 adjacency (the core's decode is the correct one)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/EXT4SearchDLL_decompiled.c` FUN_180005670 region (:4044-4048 — the extent-length multiplication `… * (longlong)*(int *)(param_1 + 0x58)` over the raw ee_len word with no unwritten-bit handling; the `*(ushort*)psVar5[-2] * block_size` form read first-hand this session)
**Core references:** audited `34d6cf10` — the core's extent decode applies the kernel's unwritten-extent semantics (ee_len > 32768 → unwritten, len −= 32768) — the kernel-correct form (`src/core/ext4/cr_ext4_engine.cpp` extent walk)
**Status:** READ-ONLY REGISTER — acknowledged deliberate divergence; no code change

## Summary

The og multiplies the raw ee_len word by the block size without masking the unwritten-extent high bit: an unwritten extent (ee_len with bit 15 set) yields an overflow-class garbage run length. The core decodes the kernel form (bit 15 = unwritten marker, length = value − 32768). This is the third batch of the og-defect non-adoption register (the I-026/I-040/D-123 family): the core's behavior is deliberately NOT the og's, and the register should say so.

## What the reference does

The multiplication sites (the :4044-4048 region; the direct-block and extent paths both multiply the raw word — the unwritten bit is never tested anywhere in the og's extent decoder, verified across the walk family this session).

## What our core does

The kernel-correct unwritten decode (documented beside the extent walker; the D-58/D-59/N-162 numeric-hygiene family precedent).

## Gap

Not a defect to fix — a stance to register: on an image containing unwritten extents (fallocate output — common on modern ext4), the og computes garbage lengths and the core computes correct ones. The register currently has no row for this class.

## Impact

Register completeness only; the recovery-quality direction favors the core.

## Reproduction / verification

Decompiled evidence: the raw multiply read first-hand; the absence of any 0x8000/32768 test in the og's extent decoder confirmed across the walk family. Core side: the unwritten decode located in the extent walker at `34d6cf10`.

## Fix plan (for the engineering team)

1. Add the D-row (the I-026/I-040 batch-3 companion): "ext4 unwritten-extent ee_len raw multiply — kernel-correct decode kept" with both cites.
2. Add an anti-down-fix pin test so a future parity pass cannot "restore" the og's garbage math.
3. Files: `docs/PARITY-DELTAS.md`; one test.

## Tests to add

- `Ext4Engine.UnwrittenExtentLenDecodedKernelCorrect` (ee_len = 0x8000 + n → n blocks, not the raw product) — the anti-down-fix pin.

## Related findings

- I-040/D-123 (batch #2 of the same register family).
- N-193 (the ext4 SB bands — the same engine's register-completeness batch; land the D-rows together).

## Open questions

None.

## Resolution

**Status:** FIXED (ACKNOWLEDGED — register applied; deliberate divergence preserved)
**Fixed in commit:** (this session's I-045 landing; SHA recorded in worklog.md)
**Verified by:** corpus re-read first-hand this session: EXT4SearchDLL_decompiled.c FUN_180005670 region :4044-4048 — BOTH raw products confirmed on the decompile lines (`CONCAT24(psVar5[-1],*(undefined4 *)psVar5) * (longlong)*(int *)(param_1 + 0x58)` at :4044-4045 and the `(uint)(ushort)psVar5[-2] * *(int *)(param_1 + 0x58)` twin at :4047 — no 0x8000/32768 test anywhere in the walk family). Core side: walk_extent_node :346-350 kernel decode re-verified at HEAD. Doc acceptance: `grep -c "I-045" docs/PARITY-DELTAS.md` ≥ 1 post-landing (D-141). Pin test: `Ext4Engine.UnwrittenExtentLenDecodedKernelCorrect` green (ee_len 0x8005 → one 5120-byte content extent at B_HELLO_DATA — never the og's raw 0x8005×1024 product).
**Resolution notes:** The register's fix plan executed in full (D-row + anti-down-fix pin; no code change — the finding's own verdict). D-141 lands as batch #3 of the I-026/I-040/D-123 og-defect non-adoption family: WHAT-OG-DOES carries the first-hand decompile quotes, WHAT-WE-DO carries the kernel decode cite, and the pin test locks the direction. The deliberate divergence is intentional and must be preserved in future work — a future parity pass must not "restore" the og's garbage math.
