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

## Probe resolution (2026-10-28, Agent 2 scan-engine lane — FAT +0x3e and exFAT +0x50 RESOLVED; NTFS +0x64 narrowed)

**FAT +0x3e — RESOLVED (emission-side decode, decisive).** The slot is the
first-cluster anchor behind a virtual contract (getter = vtable slot 6,
setter = vtable+0x28). The walk stamps it from the DENTRY's first-cluster
pair (dentry +0x14/+0x1A) for live entries
(FATSearchDLL_decompiled.c:11652-11660), and for deleted entries the
chain-reconstruction helper FUN_180012e60 (:13702-13738) ALWAYS closes with
`props->SetFirstCluster(start & 0xffffffff)` — where start = the
reconstructed chain head on success and **the DENTRY'S OWN first-cluster
pair when the reconstruction produced nothing** (the entry fallback
:13716-13719). So a chainless deleted file KEEPS its origin cluster: the
question's premise is confirmed, the census sample (39/39 hasRun) simply
never contained the chainless case. The emitted node carries the anchor
(puVar19[10] = props->+0x3e, :11688-11695) and the reader path uses it as
the contiguous read anchor (cluster-from-sector math, :11446-11470).

**exFAT +0x50 — RESOLVED (same contract, other object offset).** The
CExFatFileProperties 0xB8-B object's +0x50 slot (zeroed as `plVar13[10]` in
the ctor, exFATSearchDLL_decompiled.c:5198-5214) receives the same
vt+0x28 SetFirstCluster: the orphan/LostDir path stamps it
unconditionally from the entry's first cluster (source+0x18,
:5246-5252) and the emitted node carries the cluster's absolute byte
offset (node[10], :5290-5298 — `cluster_size*(c-2)+data_base`).

**NTFS +0x64 — NARROWED (partial).** The copy path FUN_180003020
(NTFSSearchDLL_decompiled.c:1972-2010) maps source+0x4f → the landed
type-registry id (+0x60, N-050) and source+0x53 → the +0x64 u32; the
caller is the $FILE_NAME parse (FUN_18002a340 :33930+), whose source
qwords are 48-bit-masked file-reference-family values (e.g.
`local_41 = *param_1 & 0xffffffffffff` — the parent-reference mask,
:33901-33915). The +0x64 datum is therefore a u32 drawn from the
$FILE_NAME reference region adjacent to the type id — most consistent
with the parent-directory/MFT-reference family — but the exact identity
(record number low word vs parent-reference low word) is NOT yet
instruction-decisive. The remaining probe: trace local_41's stack slot
into FUN_180003020's param_2+0x53 (one stack-offset mapping in the
caller's local layout). NOT implemented — zero-invention bar held.

**Core fix landed (the FAT/exFAT halves):** `tree::FileProperties` gained
`fat_origin_cluster` (the passive anchor datum, stamped from the
dentry/stream-dentry at emission independent of chain resolution —
cr_fat_quick.cpp / cr_exfat_quick.cpp), persisted as the session tree
record's nflags bit-7 block (4 bytes, fires only when nonzero —
pre-fix sessions stay shape-identical; cr_records.cpp). The og's
degraded contiguous-READ use of the anchor is NOT reproduced (not
decoded). Tests: FatEngine.FatOriginClusterCarriedForChainlessDeleted +
ExfatEngine.OriginClusterCarriedFromStreamDentry +
Session.FatOriginClusterRoundTrip — runtime fail-before PROVEN (stamps
neutralized: both engine tests RED with the carrier at 0 while the
session round-trip stays green; full suite 927 passed / 4 platform-skips
/ 0 failed).

**Status: OQ-SA-1 stays tracked-open for the NTFS +0x64 half only (the
narrowed question above); the FAT/exFAT anchor halves are CLOSED.**

## FINAL RESOLUTION (2026-10-28, Agent 2 session 3 — the NTFS +0x64 half decoded; OQ-SA-1 CLOSES)

**NTFS +0x64 — RESOLVED (instruction-decisive, every link read first-hand).**
The value is the **low 32 bits of the file's FIRST $DATA RUN'S STARTING LCN** —
the NTFS member of the vtable+0x30 location-anchor family (FAT +0x3e /
exFAT +0x50 / NTFS +0x64 / CRAW +0xec; HFS+/ReFS/Generic return −1):

- `FUN_1800336f0` (:40999-41128) — the runlist encoder: out1 (:41028) = the
  first run cursor entry's stored start LCN; out2 (:41043) = Σ run lengths
  (total clusters, zeroed when any run is sparse :41096-41098).
- `FUN_1800069e0` ($DATA parse, :5115-5127): raw+0x53 = (u32)out1; raw+0x4f =
  (u32)out2. The size-class formula at :5144 confirms out2 = total clusters.
- `FUN_180003020` (copier, :1986-1988): +0x58 ← raw+0x47 (own record number);
  **+0x64 ← raw+0x53**; +0x60 ← raw+0x4f.
- `FUN_18002a340` ($FILE_NAME deep path, :33912): the raw+0x4f qword is
  zeroed on this path (+0x60/+0x64 = 0; runs attach later via
  `FUN_180033250` :2253-2265). The prior session's "local_41 → +0x53" premise
  was the wrong stack slot: local_41 (@−0x41 = raw+0x47) feeds **+0x58**, and
  the parent reference is returned via `*param_4` (:33932-33934) — never
  stored in the object.
- Serializer `FUN_180003440` (:2362-2363) persists +0x64 then +0x60.

**The N-050/N-059 "+0x60 = type-registry id" reading is REFUTED** (writers
site the encoder outputs there; the +0x60 getter (vtable+0x20) is the shared
return-0 stub for 8 of 10 per-FS classes; the census's per-extension value
multiplicity is allocated-clusters behavior). No core code landed on the
refuted reading. Corpus errata rows filed (ifileproperties-part1, rsf-format
:385, ntfs-engine-algorithms-v2's vtable+0x28 label — it is the SECTOR size).

**Core fix landed (this session):** `tree::FileProperties::ntfs_origin_lcn`
(u32) — the passive anchor stamped from the first decoded $DATA run at BOTH
emission sites (main walk `cr_mftwalk.cpp` props block; GUID-dump remnant
path), sparse first run → 0xFFFFFFFF (the og's stored-start read), resident /
no-runs → 0 (the og's zeroed raw block). Persisted as the session tree
record's nflags bit-8 block (4 bytes, fires only when nonzero — pre-fix
sessions stay shape-identical). The og's +0x60 total-clusters twin is NOT
carried (derivable from allocated_size + extents; no decoded consumer — its
vtable getter is a 0-stub for 8/10 classes).

**Status: OQ-SA-1 CLOSED — all three anchor fields (FAT +0x3e, exFAT +0x50,
NTFS +0x64) decoded, carried, and persisted.**
