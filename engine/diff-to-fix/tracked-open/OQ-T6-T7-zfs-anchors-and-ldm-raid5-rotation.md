# OQ-T6 (tracked-open) — ZFS: og nvlist anchor, multi-vdev DVA resolution, uberblock geometry
# OQ-T7 (tracked-open) — LDM: RAID-5 rotation parity vs LdmManager.mo

---

## OQ-T6 — ZFS open reference questions

**Status:** UNVERIFIED · **Module:** zfs · **Parameters:** B1, A25, B8
**Corpus artifacts to examine next:**
`re-workspace/reversed/ghidra-searchdll/ZfsSearchDLL_decompiled.c` — the caller context of
the fixed nvlist read at `:21044–21100` (`FUN_180018730` parse loop), the uberblock walk
`:32321–32400`, and the DVA-resolution sites in CStreamBase.

1. **nvlist anchor (HIGH-confidence mechanical difference, unresolved intent).** The og
   performs ONE fixed read: `device.read(0x1C000, 0x4000)`, parse from +4, limit 0x1BFFC
   (verified :21050). Our core reads the VDEV_PHYS nvlist region for **each of the 4
   labels** at label+16 KiB (`cr_zfs_engine.cpp:50, :1006–1009`). 0x1C000 = 112 KiB is not
   label0+16 KiB (=0x4000); it may be a second-copy anchor or an og quirk. Until the og's
   caller context is decoded, the core's per-label read stands as the spec-correct
   superset.
2. **Multi-vdev DVA resolution (capability gap, registered D-09 NEEDS-WORK).** The og's
   vdev_tree parser recurses into children (type-9) and builds a per-vdev geometry
   registry — but whether DVAs are resolved across sibling vdevs is NOT decoded. Our core
   refuses non-zero vdev ids (`cr_zfs_engine.cpp:409`) and reports
   `stats.unsupported_vdevs`. Mirror/raidz pools: unknown og yield vs our zero. This is
   the biggest open ZFS question and the corpus's ZfsSearchDLL (981 fns / 45,364 lines)
   has enough material to answer it.
3. **Uberblock geometry.** Og: fixed 1 KiB slots (0x80-qword stride, verified :32365).
   Core: probes shifts 10..13 with hit-count validation (`cr_zfs_engine.cpp:1061–1091`) —
   supports ashift-13 pools the og cannot read. Superset; no action, register.
4. **Checksum-verify depth in CStreamBase** (fletcher2/4, sha256, on-disk blkptr
   checksums?) — decode needed for X-parameter and correctness grading parity.

---

## OQ-T7 — LDM RAID-5 rotation parity

**Status:** UNVERIFIED · **Module:** ldm · **Parameters:** B2, A27
**Corpus artifact to examine next:** `re-workspace/reversed/ghidra-mo/LdmManager_decompiled.c`
(91,475 lines) — the RAID-5 read path (XOR reconstruction loop; look for the column/parity
index arithmetic).

**Precise question:** our core reconstructs RAID-5 members with **left-symmetric**
rotation — `P(row) = N−1−(row mod N)`, data chunk i on column `(P+1+i) mod N`
(`src/core/table/cr_ldm.cpp:1352–1389`, audited `67193b6`, libldm/kernel-documented
layout). Does the og's LdmManager use the same rotation? A different rotation
(left-asymmetric/continuing) would produce **wrong bytes** on every reconstructed member —
an ACUTE if mismatched. The strong prior (both implement Windows LDM's on-disk semantics;
libldm reverse-engineered the same layout from the Windows driver) says left-symmetric,
but the og binary is the ground truth and has not been checked.

**Suggested probe:** locate the XOR loop in LdmManager's volume-read path; extract the
parity-column index formula; compare with `(N−1−(row mod N))`. A synthetic 3-column
RAID-5 LDM fixture read through both (og via Windows, core via the existing
`test_ldm.cpp` harness shapes) would settle it empirically.

**Why it matters:** RAID-5 reconstruction correctness is byte-level; there is no
grading-level tolerance for a rotation mismatch.

**Related:** A-001 (fixing the GUIDs is what makes GPT dynamic disks reach this code at
all).
