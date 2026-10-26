# OQ-S6-2 — Btrfs RC4 PRGA: what does the og decrypt, with what key? (encrypted-dataset support)

**Filed by:** session-6 deep-continuation audit (converging agent A + agent C reads)
**Status:** RESOLVED (DFX30, 2026-10-26) — **the capa flag is a FALSE
POSITIVE: there is NO RC4 in BtrfsSearchDLL.** The flagged function
(FUN_180012190) is the zlib-inflate cluster's window pump. All three
hypotheses REFUTED; no encrypted-dataset support exists in the engine;
the core's RC4-absence is MATCH by mutual absence.
**Blocking:** none active (no finding claims the behavior); would become a finding if the decode shows recoverable-content divergence

## The precise questions

1. **What content does the RC4 PRGA in BtrfsSearchDLL decrypt?** The capa-verified evidence says "RC4 PRGA (encrypted-dataset support)" (remaining-engines--ext4-hfs-refs-btrfs-zfs.md:68-69). Btrfs itself has no in-band encryption in the mainline on-disk format — so what is the target? Hypotheses: (a) a vendor-specific obfuscated metadata blob; (b) the engine's own BtrfsCache.txt obfuscation (see N-072 — the cache file could be RC4-obfuscated, which would explain a PRGA in a caching engine); (c) btrfs-progs-adjacent encrypted-subvolume extensions.
2. **Where does the key come from** (hardcoded, volume-derived, or file-adjacent)?
3. **Does the og recover content the core cannot** because of this path? (The core has no RC4 anywhere — grep-verified.)

## RESOLUTION (DFX30, 2026-10-26 — first-hand decode)

- The capa function-level match "encrypt data using RC4 PRGA" fired on
  **FUN_180012190** (:13797-13904). Read first-hand: it is a
  sliding-window stream pump — window memcpy + dual u16 offset arrays
  rebased by the consumed amount (clamped at 0) + a 2-byte header decode
  `(b0 << shift) ^ b1 & mask` (:13888-13893) + checksum dispatch on a
  mode field (+0x2c): mode 1 -> **FUN_180014c00 = adler32** (the
  mod-0xFFF1 dual sums, :15861+), mode 2 -> **FUN_180014ee0 =
  table-driven CRC32** (4 x 1 KiB tables at DAT_18001a680/1aa80/1ae80/
  1b280, :15997+).
- The sibling reader FUN_180012370 (min-lookahead 0x106 = 258+4 — the
  DEFLATE match bound) and the emit path FUN_1800143a0 (16-bit bit
  accumulator at +0x1710; the DAT_180019cc0 {code,extra-bits} length
  table indexed [len-1] or [(len>>7)+0x100] = **deflate's length-symbol
  mapping**, :15770-15795) complete the identification: the cluster
  190/370/3a0/c00/ee0 = **the zlib inflate implementation** — the same
  capability capa's OTHER hit names ("compress data via ZLIB inflate or
  deflate" at 0x180011550).
- The capa RC4 rule false-fired on the XOR-shaped header/bit-buffer
  expressions (its own XOR basic-block matches at 0x18000E0E0/
  0x180014F31/0x180015160 are the trigger family). **No RC4 KSA exists
  anywhere in the decompile** (no 256-iteration key-indexed swap loop; no
  S-box permutation state; no key schedule) — grep-verified.
- Consequences: hypothesis (a) vendor obfuscated metadata — REFUTED;
  (b) BtrfsCache.txt RC4 obfuscation — REFUTED (N-072's cache design
  proceeds without obfuscation parity); (c) encrypted-subvolume
  extensions — REFUTED (no such path). The og's Btrfs compression
  support = zlib + the CRC32 family, nothing else. The core's
  RC4-absence = MATCH by mutual absence; no PARITY-DELTAS row needed.
- Corpus erratum filed (E46): the artifact's "RC4 PRGA (encrypted-dataset
  support)" caption is a capa-rule-name over-read (the family doc's own
  "?" flag was correct to be suspicious).

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
