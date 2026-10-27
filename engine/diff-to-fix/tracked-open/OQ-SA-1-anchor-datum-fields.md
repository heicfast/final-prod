# OQ-SA-1 — the part1 anchor-datum fields: FAT +0x3e (first-cluster), exFAT +0x50, NTFS +0x64 — semantics undecoded; core has no carrier

**Status:** UNVERIFIED (moved from the session's wave-2 comparison — the corpus field census records the slots, but two of the three semantics are inferred, not decoded)
**Module / Surface:** session / per-FS persisted fields (modules/session.md)
**Parameters affected:** D11 (node fields)
**Corpus references:** `easeus-drw-research/re-workspace/artifacts/SaveScan/ifileproperties-part1--ntfs-fat-exfat.md` — FAT: `:125-127` (`u32 +0x3e` "first-cluster/dir-entry anchor [INFERRED: cluster-size scale]", `u32 +0x42` parent-directory anchor); exFAT `+0x50` (secondary anchor, semantics undocumented beyond the slot); NTFS `+0x64` (secondary id, undecoded)
**Core references:** audited commit `dd82db9` — the session record carriers (`src/core/session/cr_records.cpp`, `cr_crsf.cpp` + the per-FS payload fields): no field carries a FAT first-cluster anchor for chainless files (the first cluster is derivable only when a chain resolved — a deleted file with a broken chain loses the datum the og keeps); no exFAT +0x50 / NTFS +0x64 equivalents exist
**Prior audit check:** N-050 (s5)/N-059 trail flagged the type-registry-ID residual (+0x60 NTFS / +0x54 exFAT) but never landed a carrier; the anchor fields are newly surfaced by this session's first full read of part1

## The precise question

1. **FAT +0x3e**: does the og's value survive for files whose chain did NOT resolve (the census sample is all-chain-resolved, 39/39 hasRun)? If yes, the og retains a deleted file's origin cluster where the core reconstructs nothing — a recall/identity datum the core drops.
2. **exFAT +0x50**: what is the field's semantics (part1 records the slot and the observed value band only)?
3. **NTFS +0x64**: same question (part1 leaves it undecoded; the +0x60 type-registry id sibling is the known N-050/N-059 residual).

## The probe that resolves it

- For +0x64: run the existing corpus probe script `rsf_field_census.py` (core repo, `scripts/`) with a correlation pass against MFT record numbers and $I30-derived parent ids — the value's correlation pattern identifies the semantics (record-ref vs parent vs flags).
- For +0x50: run the og on a controlled exFAT volume (the bench harness `bench/og/run-og.ps1` supports staged volumes) with known file/cluster placements, and read the field's value from the produced .rsf; alternatively decompile the exFAT IFileProperties writer at `FUN_14058c870` (the part1 evidence base) and read the field's source expression.
- For +0x3e: the same og-run on a FAT volume with a deliberately chain-broken deleted file (the fixture the FAT tests already build for chain truncation) — the field's value for that record decides whether the og keeps the origin cluster.

## Why it was not resolved this session

The corpus artifacts record the slots and value bands but mark the semantics INFERRED/undecoded; resolving them requires either an og execution on controlled fixtures (Windows + the og binary — available via the bench harness but out of this session's remaining scope) or a fresh decompile read of the named writer function. Both are specified above and executable by the next session without further investigation.

## Related findings

- **N-087** (this session) — the same artifact's name-field finding (filed, evidence complete).
- **N-050/N-059** (FIXED) — the adjacent type-registry-ID residual (registered; distinct fields).
