# OQ-S6-2 — Btrfs RC4 PRGA: what does the og decrypt, with what key? (encrypted-dataset support)

**Filed by:** session-6 deep-continuation audit (converging agent A + agent C reads)
**Status:** RESOLVED (session 25, DFX25) — **INVALID premise: the capa finding is a FALSE POSITIVE; no RC4 exists in BtrfsSearchDLL** (evidence: `../evidence/decompiled-quotes/s25-oq-s6-2-btrfs-rc4-false-positive.md`; corpus erratum E46)

## RESOLUTION (session 25, 2026-10-26)

The capa match (0x180012190, the only RC4-rule hit in the binary) was
disassembled first-hand for its full 469 bytes: it is a chunked
checksum-verified COPY PUMP — per-chunk dispatch at stream+0x2c: type 1 →
FUN_180014c00 = **adler-32** (0xFFF1 modulus, 5552-blocking), type 2 →
FUN_180014ee0 = **slicing-by-4 table CRC**; the rule-matched loops are u16
length-table maintenance. The RC4 essentials are all ABSENT: no `and
$0xff` anywhere in the function, no 256-entry state / no KSA anywhere in
the decompile (zero `% 0x100` expressions), no keystream XOR, no key
material, no rc4/arc4/crypt/cipher strings in the binary.

Answers to the three filed questions: (1) the og decrypts NOTHING with
RC4 — it does not exist; the artifact's "encrypted-dataset support" claim
is capa's false positive propagated (erratum E46). (2) Key source: N/A.
(3) NO content-recovery divergence — the core's no-RC4 posture is parity
by shared absence. Hypothesis (b) (BtrfsCache.txt RC4 obfuscation)
refuted at the RC4 level; N-072's cache design carries no RC4 dependence
(grep-verified). No code change; the erratum row + the evidence file are
the landing.
**Blocking:** none active (no finding claims the behavior); would become a finding if the decode shows recoverable-content divergence

## The precise questions

1. **What content does the RC4 PRGA in BtrfsSearchDLL decrypt?** The capa-verified evidence says "RC4 PRGA (encrypted-dataset support)" (remaining-engines--ext4-hfs-refs-btrfs-zfs.md:68-69). Btrfs itself has no in-band encryption in the mainline on-disk format — so what is the target? Hypotheses: (a) a vendor-specific obfuscated metadata blob; (b) the engine's own BtrfsCache.txt obfuscation (see N-072 — the cache file could be RC4-obfuscated, which would explain a PRGA in a caching engine); (c) btrfs-progs-adjacent encrypted-subvolume extensions.
2. **Where does the key come from** (hardcoded, volume-derived, or file-adjacent)?
3. **Does the og recover content the core cannot** because of this path? (The core has no RC4 anywhere — grep-verified.)

## The corpus artifacts that must be examined next

- The BtrfsSearchDLL decompile tree under `re-workspace/reversed/` — xrefs from the capa-flagged PRGA function to its callers; follow the key argument and the output buffer consumers.
- `SearchDLL/remaining-engines--ext4-hfs-refs-btrfs-zfs.md` §BtrfsSearchDLL (the capa table) for the flagged function address.

## Why it was not resolved this session

The artifact lists the capa finding without the call graph; the PRGA's role (cache obfuscation vs content decryption) determines whether it is even a recovery-behavior divergence, and that requires the xref walk.

## Related findings

N-072 (BtrfsCache.txt — hypothesis (b) makes this OQ a dependency of that finding's cache design); S-028 (Btrfs xattr recovery — the active SEVERE in the same engine).

---

## Session-14 addendum (DFX14 — pushed forward, unchanged)

Capa-verified primitive, target still undecoded; no new evidence this
session. The next concrete step stands (the Btrfs dataset-decrypt call
site in BtrfsSearchDLL).


### Convergence addendum (DFX30, 2026-10-26 — independent verification + cluster identification)

A second lane (DFX30, the tracked-open tail) decoded the same function
independently before discovering DFX25's landing. Per the convergence
protocol the LANDED resolution above stands; the converging decode is
recorded here because it carries ONE additional identification: the
pump's callers/peers complete a **zlib-inflate cluster** — the sibling
reader FUN_180012370 (min-lookahead 0x106 = 258+4, the DEFLATE max-match
bound) and the emit path FUN_1800143a0 (16-bit bit accumulator at
+0x1710; the DAT_180019cc0 {code, extra-bits} length table indexed
[len-1] / [(len>>7)+0x100] = deflate's length-symbol mapping,
:15770-15795) — i.e. the pump serves the engine's zlib inflate (the
capability capa's other hit names at 0x180011550), with adler32/CRC32 as
the verify modes. All other conclusions identical: no RC4, no KSA, no
encrypted-dataset support, hypotheses (a)/(b)/(c) refuted, core MATCH by
mutual absence. The FIXED/ filing completed by DFX30 in the same change.
