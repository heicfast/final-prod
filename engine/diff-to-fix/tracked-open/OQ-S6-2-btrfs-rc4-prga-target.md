# OQ-S6-2 — Btrfs RC4 PRGA: what does the og decrypt, with what key? (encrypted-dataset support)

**Filed by:** session-6 deep-continuation audit (converging agent A + agent C reads)
**Status:** OPEN — capa-verified primitive, undecoded target
**Blocking:** none active (no finding claims the behavior); would become a finding if the decode shows recoverable-content divergence

## The precise questions

1. **What content does the RC4 PRGA in BtrfsSearchDLL decrypt?** The capa-verified evidence says "RC4 PRGA (encrypted-dataset support)" (remaining-engines--ext4-hfs-refs-btrfs-zfs.md:68-69). Btrfs itself has no in-band encryption in the mainline on-disk format — so what is the target? Hypotheses: (a) a vendor-specific obfuscated metadata blob; (b) the engine's own BtrfsCache.txt obfuscation (see N-056 — the cache file could be RC4-obfuscated, which would explain a PRGA in a caching engine); (c) btrfs-progs-adjacent encrypted-subvolume extensions.
2. **Where does the key come from** (hardcoded, volume-derived, or file-adjacent)?
3. **Does the og recover content the core cannot** because of this path? (The core has no RC4 anywhere — grep-verified.)

## The corpus artifacts that must be examined next

- The BtrfsSearchDLL decompile tree under `re-workspace/reversed/` — xrefs from the capa-flagged PRGA function to its callers; follow the key argument and the output buffer consumers.
- `SearchDLL/remaining-engines--ext4-hfs-refs-btrfs-zfs.md` §BtrfsSearchDLL (the capa table) for the flagged function address.

## Why it was not resolved this session

The artifact lists the capa finding without the call graph; the PRGA's role (cache obfuscation vs content decryption) determines whether it is even a recovery-behavior divergence, and that requires the xref walk.

## Related findings

N-056 (BtrfsCache.txt — hypothesis (b) makes this OQ a dependency of that finding's cache design); S-028 (Btrfs xattr recovery — the active SEVERE in the same engine).
