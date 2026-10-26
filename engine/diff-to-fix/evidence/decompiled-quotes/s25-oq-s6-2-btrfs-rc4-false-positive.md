# s25 — OQ-S6-2 decode: the "RC4 PRGA" in BtrfsSearchDLL is a capa FALSE POSITIVE

Session 25 (DFX25), 2026-10-26. Corpus `03ad46c`. The capa finding
(`ci-multitool/searchdll/BtrfsSearchDLL/capa_capabilities.txt`:
"encrypt data using RC4 PRGA / data-manipulation/encryption/rc4 / matches
0x180012190") was re-verified first-hand against the disassembly and the
decompile. It does not hold.

## 1. What 0x180012190 actually is (FUN_180012190, 469 bytes)

A chunked, checksum-verified copy pump over a block stream:

- Per chunk, the checksum type at `+0x2c` of the stream object dispatches:
  - **type 1 → FUN_180014c00** = **adler-32** (the 0xFFF1/65521 modulus,
    two 16-bit accumulators packed `hi<<16|lo`, the 5552/0x15B0 NMAX
    block unrolling — read at :15861-15992);
  - **type 2 → FUN_180014ee0** = **table-driven CRC, slicing-by-4**
    (tables at DAT_18001a680/1aa80/1ae80/1b280, byte-indexed XOR at
    shifts 24/16/8/0 — read at :15997-16024).
- The two backward-walking `movzwl -0x2(%rdx); sub %bx,%ax; cmovb
  %r12w,%ax; mov %ax,(%rdx)` loops (0x180012210 / 0x180012240) maintain
  **u16 length tables** (subtract a constant, clamp at zero) — compressor/
  decompressor bookkeeping.
- The function tail is a stream-header decode: `value = ((b0 << shift) ^
  b1) & mask` (0x180012314-0x180012329).
- The chunk bytes are memcpy'd to the output ring (0x1800122d6-0x1800122e9).

## 2. Why the capa rule misfired

The RC4 PRGA rule's byte heuristic matched the compact
load-modify-store loop shape (`movzwl -0x2(%rdx) … mov %ax,(%rdx)` — a
2-byte swap-looking window walk). The RC4 essentials are all absent,
verified in the raw disassembly of the whole 469-byte function:

- **No `and $0xff` anywhere** (RC4's index arithmetic is impossible
  without it; the only zero-extends are `movzbl`/`movzwl` loads);
- **No 256-entry state / no KSA**: a pattern search over the ENTIRE
  BtrfsSearchDLL decompile finds **zero** `% 0x100` index expressions
  and no 256-iteration permutation initializer;
- **No keystream XOR of output** (data flows through memcpy and the
  checksum accumulators only);
- **No key material** and **zero** rc4/arc4/crypt/cipher strings in the
  binary (strings scan).

## 3. Consequences

1. **What does the og decrypt with RC4? Nothing — no RC4 exists in
   BtrfsSearchDLL.** The artifact's capa digest line ("RC4 PRGA
   (encrypted-dataset support)") propagated capa's false positive —
   corrected as corpus erratum E46.
2. **Key source: N/A.**
3. **Content-recovery divergence: none.** The og has no
   encrypted-dataset path; the core's no-RC4 posture is parity by
   shared absence. Hypothesis (b) from the OQ (BtrfsCache.txt RC4
   obfuscation) is refuted at the RC4 level; N-072's cache finding
   carries no RC4 dependence (grep-verified — its fix design is
   unaffected).

**OQ-S6-2 RESOLVED — INVALID premise (capa false positive); no code
change; the corpus erratum row + this evidence file are the landing.**
