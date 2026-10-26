# OQ-T6 (tracked-open — ALL SUB-QUESTIONS RESOLVED; closed to FIXED/ by DFX30 2026-10-26) — ZFS: og nvlist anchor, multi-vdev DVA resolution, uberblock geometry
# OQ-T7 (tracked-open) — LDM: RAID-5 rotation parity vs LdmManager.mo

---

## OQ-T6 — ZFS open reference questions

**Status:** PARTIALLY RESOLVED (session-2 decompile read — the anchor and the vdev-tree handling decoded; the capability question is decisively BOUNDED) · **Module:** zfs · **Parameters:** B1, A25, B8
**Session-2 findings (2026-10-25, first-hand; quotes in `evidence/decompiled-quotes/s2-zfs-utf8-conversion.md` context and below):**
1. **nvlist anchor CONFIRMED verbatim**: `ZfsSearchDLL_decompiled.c :21052` —
   `FUN_180018220(param_1, &buf, 0x4000, 0x1c000)` — ONE fixed 16 KiB read at 0x1C000
   (112 KiB), parse from +4, limit 0x1BFFC. The core's per-label reads (each of the 4
   labels at label+16 KiB, `cr_zfs_engine.cpp:50,:1006-1009`) remain the spec-correct
   superset; the og's single fixed anchor is now quote-confirmed.
2. **vdev_tree handling decoded (:21089-21150)**: on the "vdev_tree" nvlist entry, the og
   parses the nested nvlist and extracts **only the "id" key** from each child
   (:21113-21123 byte-compares 'i','d'; :21139-21141 big-endian u64 read; :21144-21145
   registers into an id-keyed std::map via FUN_180024110/FUN_1800241b0 — key constant
   DAT_180031570 = ASCII "id" extracted from the binary).
3. **Decisive negative (strings scan of ZfsSearchDLL.dll)**: the binary contains NO
   vdev-type strings ("mirror", "raidz", "replacing", "spare", "cache", "log", "disk",
   "file" — 0 hits) and NO geometry keys ("ashift", "asize", "nparity", "children" —
   0 hits). **The og build is vdev-TYPE-BLIND**: no mirror copy-selection logic and no
   RAID-Z parity math are even possible in it. RAID-Z pool data is unreadable by this
   build by construction; mirrors can at best work by first-child accident.
**Consequence for the core's D-09 (unsupported_vdevs) posture**: the og is NOT a
multi-vdev-capable reference that the core lags behind — both are effectively
single-logical-vdev readers; the core's explicit refusal + `unsupported_vdevs` stats is
the honest superset of the og's type-blind behavior. The remaining refinement: the og's
read path through the id-map (which physical reads serve a (vdev-id, offset) DVA for a
MIRRORED top-level vdev — first child blindly?) — next decode target:
FUN_180024110's map consumers in the DVA resolution path.

**Original questions (for the record):**

1. **nvlist anchor (HIGH-confidence mechanical difference, unresolved intent).** The og
   performs ONE fixed read: `device.read(0x1C000, 0x4000)`, parse from +4, limit 0x1BFFC
   (verified :21050). Our core reads the VDEV_PHYS nvlist region for **each of the 4
   labels** at label+16 KiB (`cr_zfs_engine.cpp:50, :1006–1009`). 0x1C000 = 112 KiB is not
   label0+16 KiB (=0x4000); it may be a second-copy anchor or an og quirk. Until the og's
   caller context is decoded, the core's per-label read stands as the spec-correct
   superset.
2. **Multi-vdev DVA resolution — RESOLVED (M-019's landing + DFX30's first-hand
   re-verification, recorded in OQ-Z3):** the og does NOT resolve across
   sibling vdevs — FUN_1800168b0's registry loop falls back to the FIRST
   entry on any vdev-id miss (never fails). The core MATCHES since M-019
   (foreign-vdev blocks planned at the fallback position,
   `foreign_vdev_reads` counted; D-09 re-verdicted ACCEPTED with the
   pinned og story). The pre-fix core's refusal (`cr_zfs_engine.cpp:409`
   at audit time) was the divergence — closed.
3. **Uberblock geometry — RESOLVED (registered superset, D-04; og constant
   re-confirmed first-hand by DFX30):** Og: fixed 1 KiB slots (0x80-qword
   stride, verified :32365; and the ring loader FUN_180018930 reads the
   128 KiB ring at offset 0x20000 with the hardcoded 0x400 stride — the
   S-034 decode). Core: probes shifts 10..13 with hit-count validation
   (`cr_zfs_engine.cpp:1061–1091`) — supports ashift-13 pools the og
   cannot read. REGISTERED: PARITY-DELTAS **D-04** ("ZFS uberblock ring
   auto-probes ashift shifts 10-13"). No action.
4. **Checksum-verify depth — RESOLVED (M-019's decode + landing):** the og
   gates block reads on the fletcher4 family only — `prop>>0x28 & 0xff`
   = the checksum-type field (blk_prop bits 40-47), gate `(zc==7)||(zc==0)`
   (fletcher4 / off), the interleaved 4-accumulator fletcher4 over the raw
   bytes + a 32-byte memcmp against the stored checksum, with next-DVA
   retry on mismatch or read failure (drops counted). The core implements
   exactly this since M-019 (read_block's {7,0} gate; ChecksumFailDropsAnd-
   RetriesNextDva fail-before-proven). No fletcher2/sha256 path exists in
   the og's engine (no such accumulators beyond the fletcher4 shape).

---

## OQ-T7 — LDM RAID-5 rotation parity

**Status:** RESOLVED (session-2 decompile read, first-hand) · **Module:** ldm · **Parameters:** B2, A27
**Resolution (2026-10-25, session 2):** the og's rotation is **left-symmetric** and the core's
formula is **algebraically identical** — the safety-critical question resolves to MATCH.

- The og's `Raid5VolumeStorage::ReadSector` (mapped via SEH crash-handler strings
  `:88602-:88709` to FUN_180086a90, `LdmManager_decompiled.c :68777-68876`) branches on a
  rotation selector at `this+0x2a8` with FOUR modes: R=0 left-symmetric
  `col = ((N-1 - s%N) + 1 + f) % N`, R=1 left-asymmetric (fill-from-0, skip parity N-1-s),
  R=2 right-symmetric `col = (s%N + 1 + f) % N`, R=3 right-asymmetric (fill-from-0, skip
  parity s%N). Full verbatim quote: `evidence/decompiled-quotes/s2-ldm-raid5-rotation.md`.
- The selector is constructor-initialized to 0 (`:68942`) and has **no other write site in
  the entire 91,475-line decompile** (grep-verified) — R=1/2/3 are unreachable dead code in
  this build. Recorded as INFORMATIONAL finding **I-008**.
- Core `cr_ldm.cpp:1349-1359`: `row = k/(N-1)`, `P = (N-1)-(row mod N)`, `col = (P+1+i) mod N`
  — with the og's s=row, f=i this is exactly `((N-1 - s%N) + 1 + f) % N`. **MATCH.**
- Failure reconstruct: og `ComputeDataWithOtherDisk` (FUN_180085c30, `:68393-68451`) XOR-folds
  every other column, first-read memcpy, and returns failure when a second member is also
  unreadable — identical to the core's `:1372-1389` (surfaced, never zeroed). **MATCH.**
- The same deep pass resolved the GPT-LDM gating question raised by session-2's A-001
  re-verification: the og gates its GPT-LDM analysis on the **real**
  `PARTITION_LDM_METADATA_GUID` (`5808c8aa-7e8f-42e0-85d2-e1e90434cfb3`, extracted from
  LdmManager.mo DAT_1800c4960 — FUN_1800724f0 `:59445`), reads the PRIVHEAD candidate at the
  matching partition's **last LBA** (entry+0x28, `:59448` — same anchor as the core's
  `find_privhead`), and performs **no GPT CRC checks** on this path (same as the core).
  See `evidence/decompiled-quotes/s2-ldm-gpt-gate.md` and finding **A-002** (the current
  core's constant is the fabricated A-001 value and fails real disks).

**Original question (for the record):**
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

---

## Session-14 addendum (DFX14 — T7 half RESOLVED via I-008's landing; T6 stands)

T7 (LDM RAID-5 rotation parity) was resolved by I-008's verification (the
four og rotations are dormant dead-code; the enabled left-symmetric mode
is a proven MATCH — see FIXED/INFORMATIONAL/I-008). T6 (the ZFS nvlist
anchor / multi-vdev DVA / uberblock geometry set) remains open as filed.

---

## Session-28 addendum — OQ-T6 evidence push (N-019's scenario-9 verification)

Re-verification of N-019's "ZFS snapshot" scenario surfaced two new facts
that sharpen this OQ's next decode target:

1. **Corpus side (positive, capa-level):** `SearchDLL/zfs-engine--deep-dive.md`
   §3 states the engine "surfaces snapshot roots when present in the DSL tree
   (family evidence: dataset hierarchy enumeration)" — the og DOES have a
   per-dataset snapshot surface, evidence class = capa digest (not a
   decompiled walk).
2. **Core side (negative-space, verified this session):**
   `cr_zfs_engine.cpp` enumerates the dataset chain (objdir "root_dataset" →
   dsl_dir → head_dataset → ZPL) but has NO snapshot surface: no
   `origin_obj` chase (dsl_dir_phys +16 / dsl_dataset props), no
   `ds_snapnames` ZAP walk, no snapshot-root emission. A pool with snapshots
   surfaces only the head datasets.

**Precise next decode target:** the DSL-tree snapshot enumeration in
`ZfsSearchDLL_decompiled.c` — starting from the dsl_dataset bonus block
(`ds_bp@128` is already parsed by the core; `dsl_dataset_phys` carries
`propszap`/`next_snapobj`/`prev_snapobj` chain pointers) — locate the
snapshot-root walk (the strings scan in the original OQ found NO
"snapshot"/"@"/"com.delphix" strings, so the surface likely rides the
numeric `next_snapobj`/`prev_snapobj` chain rather than a name ZAP; the
ROOT_REF-style naming decode would follow). Until decoded, snapshot
recovery stays correctly out of the core (no hallucinated parity).
