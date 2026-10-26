#!/usr/bin/env python3
"""DFX7 INFO tier: append Resolution sections to the 15 unacknowledged
INFORMATIONAL findings. Verified this session against corpus 03ad46c and
core main (4702ec8 lineage)."""
import os

BASE = "diff-to-fix/findings/INFORMATIONAL"

RESOLUTIONS = {
"I-006-ext4-nanoseconds-superset-core-preserves-og-drops.md": """
## Resolution

**Status:** ACKNOWLEDGED (deliberate superset — preserved)
**Verified by:** first-hand re-verification this session (session 7):
corpus `ghidra-searchdll/EXT4SearchDLL_decompiled.c:5494-5515` (i_ctime
sec-only read, `_gmtime32_s`, `wMilliseconds = 0` hardcoded) and `:6255-6285`
(3rd/4th FILETIME slots zero-initialized, never assigned) re-read directly;
core `cr_ext4_engine.cpp:96` (`decode_extra` — kernel ext4_decode_extra_time),
`:603-607`/`:702-704` (all four timestamps through `unix_to_filetime_ns`),
`cr_types.h:56` — all re-read directly.
**Resolution notes:** the rationale is correct and verified on both sides:
the og's ext4 engine is second-only (sub-second forced to zero at two
independent sites), the core preserves nsec at 100ns granularity per the
ext4 spec. No code change. Registered as PARITY-DELTAS **D-59a**. Bench
comparisons must expect mechanical divergence on the sub-second units —
expected, not a regression. Future work must PRESERVE this divergence.
""",
"I-007-ext4-hfsplus-btrfs-core-reads-more-timestamps-than-og.md": """
## Resolution

**Status:** ACKNOWLEDGED (deliberate superset — preserved)
**Verified by:** first-hand re-verification this session: corpus
`EXT4SearchDLL_decompiled.c:6255-6258` (slots +0x4b/+0x55 zeroed, never
assigned); `HFSPlusSearchDLL_decompiled.c:8005-8048` (exactly THREE
conversion functions — +0xc0/+0xc4/+0xc8(200); no 4th); Btrfs
`BtrfsSearchDLL_decompiled.c` FUN_180006ba0 (:4425-4450, three timespec
conversions; `+0x94` appears only as a zeroing write at :271 — otime never
converted). Core: ext4 `cr_ext4_engine.cpp:603-607,702-704`; HFS+
`cr_hfsplus_engine.cpp:853-856` AND `:1093-1096` (both record sources —
catalog walk and the S-021 journal emission carry all four); Btrfs
`cr_btrfs_engine.cpp:83-86,626-632` (INO_otime=0x94 read).
**Resolution notes:** rationale correct on both sides: og reads 2/4 (ext4),
3/4 (HFS+), 3/4 (Btrfs); core reads 4/4 on every engine. No code change.
Registered as PARITY-DELTAS **D-59b**. Future work must PRESERVE this.
""",
"I-008-ldm-raid5-four-rotations-dormant.md": """
## Resolution

**Status:** ACKNOWLEDGED (enabled-mode MATCH; dormant og dead-code not replicated — registered)
**Verified by:** first-hand re-verification this session: corpus
`ghidra-mo/LdmManager_decompiled.c` — grep over the module confirms
`0x2a8) = ` has exactly ONE write (:68942, constructor, value 0); the
rotation read at :68718/:68829 branches on all four values (:68777-68876
region re-read). Core: `src/core/table/cr_ldm.cpp:1352+` — the left-symmetric
mapping `P=(N-1)-(row%N); col=(P+1+i)%N` is algebraically identical to the
og's R=0 branch `((N-1-s%N)+1+f)%N`; reconstruct-on-failure XOR at
`:1372-1389` (xor_into, second-missing-column surfaced as failure, never
zeroed) matches ComputeDataWithOtherDisk :68393-68451.
**Resolution notes:** rationale correct: no og-reproducible volume can
exercise rotations 1/2/3 (selector hard-zeroed at construction; no VBLK
field, setter, or config path writes it). The core implements the only
constructible mode exactly. No code change. Registered as PARITY-DELTAS
**D-59d** (do not implement the dead rotations).
""",
"I-009-carve-probing-and-block-io-supersets.md": """
## Resolution

**Status:** ACKNOWLEDGED (deliberate superset — pinned this session)
**Verified by:** first-hand re-verification this session: corpus
`RAWSearchDLL_decompiled.c:9496` — candidate = `buffer + half +
sector_size × sector_index` (whole-sector stepping, re-read directly);
core `src/core/carve/cr_carve.cpp` `scan_one_window` — `for (u64 off = 0;
off + 12 <= want; ++off)` byte-stepped (re-read directly).
**Resolution notes:** rationale correct; the session-3 addendum's action
item is now DONE: pin test
`CarveEngine.CarvesMidSectorStart` added (a valid PNG planted at
sector+137 — invisible to any sector-aligned prober — must surface; asserts
exact offset, FMT_PNG, and the 65-byte grammar end). Suite 440/440 ASan
green. Registered as PARITY-DELTAS **D-59c**. The M-025 read-model rework
must not adopt sector stepping.
""",
"I-010-scope-exclusions-and-supersets-registered.md": """
## Resolution

**Status:** ACKNOWLEDGED (registration group — all rows verified or re-registered)
**Verified by:** spot-checks this session: ZFS label fields — core reads
name/ashift/asize/offset/guid (`cr_zfs_engine.cpp:1014-1040`) vs og
vdev_tree/id only; ext4 BIGALLOC refusal stands (`cr_ext4_engine.cpp:45,
:513`); ReFS single-record shape — current `emit_file` (post-S-020) still
emits ONE FileNode per file row with ONE extents list (no stream/ADS
children; grep "stream" = zero hits in the refs engine), matching the
og's structural no-ADS shape (I-012). Scope-exclusion rows re-confirmed
(D-26 family).
**Resolution notes:** no code change. Registered as PARITY-DELTAS
**D-59e**. The S-006-fix probe superset row (session-3 addition) is
already registered via D-50.
""",
"I-011-corpus-errata-session2-register.md": """
## Resolution

**Status:** ACKNOWLEDGED (corpus-errata register — all 8 rows filed)
**Verified by:** the 8 rows are recorded in `diff-to-fix/CORPUS-ERRATA.md`
(Session-2 contributions, rows 1-8 of that section); findings cite the
decompiles, not the stale artifacts. The corpus repo is read-only for this
session's scope (CORRECTIONS.md fold-back happens corpus-side).
**Resolution notes:** the leftover action item ("fix
modules/visualization.md:16/:33 when next touched") is completed THIS
session: the stale D16 verdict line still claiming the open
"7-cat/85-ext vs og 8/1489" gap is updated to the M-023-FIXED state
(generated 1,481-ext table, 8 categories). No core change.
""",
"I-012-refs-ads-gap-resolved-shared-limitation.md": """
## Resolution

**Status:** ACKNOWLEDGED (shared limitation — parity confirmed, not a gap)
**Verified by:** re-verification this session on the CURRENT tree (post
S-020/M-022 fixes): `cr_refs_engine.cpp` `emit_file` builds one
`FileNode` + one `FileProperties` + one extents list per file row; the
S-020 attribute walk appends into the single runlist — no named-stream
children anywhere (grep "stream": zero hits). Corpus side: no
stream/named-stream class in the ReFSSearchDLL RTTI census; .rsf type-6
layout carries one DataRunList. GAP-ADS stands as "shared limitation,
superset opportunity — not a parity debt".
**Resolution notes:** no code change. If a future superset emits named
ReFS streams as children (mirroring NTFS ADS, N-011), the pin test named
in the finding must land with it.
""",
"I-013-refs-host-os-runtime-gate-superset.md": """
## Resolution

**Status:** ACKNOWLEDGED (deliberate portability superset — preserved)
**Verified by:** re-verification this session: corpus
`ReFSSearchDLL_decompiled.c` FUN_1800010d0 (:37-49) — `GetVersion()`,
refuse below 6.2 (host gate; re-read directly). Core: the only
"GetVersion" reference in the tree is `kIoctlGetVersion` — an RDA-driver
IOCTL (`src/win/cr_transport.cpp:489`), not a host-OS check; no
`VerifyVersionInfo`/`GetVersion` host gate exists in any engine. The core
scans ReFS on any host with an IStorageDevice.
**Resolution notes:** rationale correct; parity benches on non-Windows
hosts are not og-divergent by this row. Registered as PARITY-DELTAS
**D-59f**. Do not add a host gate.
""",
"I-014-ext4-hfsplus-supersets-and-matches-register.md": """
## Resolution

**Status:** ACKNOWLEDGED (registration group — verified, pinned where cheap)
**Verified by:** this session: ext4 deleted-inode sweep confirmed
default-on and un-gated (`cr_ext4_engine.cpp:644+` — the walk runs the
dtime sweep unconditionally, MAX_INODES 16M bound at :60/:669); the
fragmented-journal extent-map read stands (`read_extent_data`); HFS+
allocation-bitmap grading rows consumed by the S-021 fix (D-56). The
"register after one .rsf comparison" residual on bitmap grading is
carried by the S-021 landing (grading affects confidence only).
**Resolution notes:** no code change. Registered as PARITY-DELTAS
**D-59g**. The two named pin tests
(`Ext4.DeletedInodeSweepStaysDefaultOn`,
`Ext4.FragmentedJournalWalkReadsExtents`) — the sweep is already pinned
by the existing sweep tests in test_ext4_volume.cpp (the sweep's yield is
asserted there); a dedicated anti-regression rename is not required while
the behavior is load-bearing in those tests.
""",
"I-015-preview-architecture-register-og-socket-protocol-vs-core-reader.md": """
## Resolution

**Status:** ACKNOWLEDGED (deliberate architectural difference — register preserved)
**Verified by:** this session: core preview bounds re-confirmed
(`cr_preview.cpp:24` kMaxRead clamp at read time; `:59` kMaxDecode bound
in metadata extraction). The og protocol constants (602-byte 0x25A
header, payload (0, 0x100000], GlobalAlloc 0x42, ports 20171/20172,
100×100 ms) remain recorded in this finding and its evidence file.
**Resolution notes:** no code change. Registered as PARITY-DELTAS
**D-59h**. Parity tests must not byte-compare preview payloads across
the two layers; hosts building a UI get the reference contract here.
""",
"I-016-engine-factory-contract-register-and-fs-code-spaces.md": """
## Resolution

**Status:** ACKNOWLEDGED (deliberate architectural difference + two standing notes)
**Verified by:** this session: core remains a flat C ABI with an
in-process FsFamily switch (no per-DLL factories, no per-FS config
gates); the og FS-code space {3=NTFS, 9=EXT4, 0xB=Btrfs, 0xD=ZFS, 6=def}
vs core {1=NTFS, 4=ext4, 7=Btrfs, 8=ZFS} difference is live in the
parity tables (translation required, never equation).
**Resolution notes:** no code change. Registered as PARITY-DELTAS
**D-59i**. Note 1 (per-family enable flags absent from ScanConfig)
remains a host-capability note, not a parity debt — if a host needs
og-equivalent scan-scope control, add per-family flags then; note 2
(FS-code translation) is the load-bearing rule for every future parity
table.
""",
"I-017-slice-layer-deliberate-supersets.md": """
## Resolution

**Status:** ACKNOWLEDGED (deliberate supersets ledger — preserved)
**Verified by:** this session: `cr_slice.cpp:15-41` eager validation
(null / zero-length / alignment / overflow) re-read directly; per-call
error codes (`cr_error.h`) and the persistent ReadErrorMap
(`cr_devmap.cpp`) confirmed; no sticky-error channel, no null-buffer
guard (the og's −2 stick-error model is the thing we do NOT adopt).
**Resolution notes:** no code change. The optional one-line null-buffer
guard is deliberately NOT added — per-call validation at the ctor
boundary already rejects null parents, and adding a second guard at read
time would duplicate the contract (the og needed it because its ctor
validates nothing). The one behavioral divergence (tail-read
clamp-vs-refusal) remains M-032 (MAJOR tier). Registered as PARITY-DELTAS
**D-59j**.
""",
"I-018-nvme-route-byte-parity-match.md": """
## Resolution

**Status:** ACKNOWLEDGED (positive record — verified MATCH)
**Verified by:** this session: `src/win/cr_health.cpp:244-278`
`read_nvme_health` — IOCTL_STORAGE_QUERY_PROPERTY (0x2D1400) with
STORAGE_PROTOCOL_SPECIFIC_DATA {ProtocolType=ProtocolTypeNvme, DataType
log page, page 2 (health), length 0x1000} re-read directly; matches
DiskHealth_decompiled.c :13351-13375 byte-for-byte on the route.
**Resolution notes:** no code change. The field-surface difference above
the transport stays M-036 (MAJOR tier). This row upgrades any "corpus
silent" NVMe claims to verified MATCH.
""",
"I-019-epm-tool-ops-scope-register.md": """
## Resolution

**Status:** ACKNOWLEDGED (deliberate scope boundary — registered)
**Verified by:** this session: grep over `src/` + `include/` for
EditLdm2 / Ntfs2FAT / VolumeMerge / FatFormat / IMAPI2 — zero hits (the
core's LDM support is read-side only, `cr_ldm.cpp`). Corpus consumer
analysis stands (no recovery module references these surfaces; EPM hosts
only). S-029 (the apply path where the write-side DOES matter for
recovery) is a separate SEVERE finding and is not covered by this
exclusion.
**Resolution notes:** no code change. Registered as PARITY-DELTAS
**D-59k**. Future sessions must not re-litigate these surfaces as gaps;
if the product ever wants spanned-volume repair, the EditLdm2 decode in
this finding is the starting evidence.
""",
"I-020-corpus-doc-corrections-s5.md": """
## Resolution

**Status:** ACKNOWLEDGED (corpus-doc corrections — rows 37-42 filed)
**Verified by:** this session: `diff-to-fix/CORPUS-ERRATA.md` "Session-5
additions" section carries the six corrections (CTbFile = boot.ini
wrapper; CFatVirtualView = Ntfs2FAT conversion engine; CFirstVolumeMergeDeal
= EPM partition merge; EFIBoot = UEFI NVRAM, not ESP files; Fixup =
link-time shared library; CRecoverability extension table in .data,
unsorted, 169 entries). The corpus repo is read-only for this session;
the fold-back to the corpus's own CORRECTIONS.md happens corpus-side.
**Resolution notes:** no core change. The core-side consequences are
carried by S-030/M-040/I-019 (finding references updated by the s5b
audit leg already).
""",
}

count = 0
for fname, res in RESOLUTIONS.items():
    path = os.path.join(BASE, fname)
    assert os.path.exists(path), path
    with open(path, "a") as f:
        f.write(res)
    count += 1
print(f"appended {count} resolutions")
