# modules/zfs.md — ZFS engine audit verdicts

Reference: ZfsSearchDLL per corpus artifacts `SearchDLL/zfs-engine--deep-dive.md`,
`refs-btrfs-zfs-engine-algorithms-v2.md` §3, `zfs-vdev--mftmirr--calcpartition.md` §1,
`remaining-engines--ext4-hfs-refs-btrfs-zfs.md` (ZFS section).

Core (audited commit `67193b6`): `src/core/zfs/cr_zfs_engine.cpp` (1,362 LOC — the
largest single-engine file) + `include/cr/zfs/` + `tests/unit/test_zfs_volume.cpp`
(8 areas).

## Behavioral

- **B1 detection** — `M` uberblock magic 0x00BAB10C (4 decompile sites verified), txg at
  UB+0x10, newest-txg-wins, vdev label probing (4 labels, front/back); core adds
  shift-probing 10..13 (4 Kn pools) — superset, register; BE-pool refusal
  (`ERR_UNSUPPORTED`) — documented deliberate hardening.
- **B2 recovery correctness** — `M` nvlist XDR+native decode, micro/fat ZAP, gang blocks
  (3-blkptr headers), BPE (embedded block pointers), per-object compression registry
  (LZJB/LZ4/zle/gzip → codec-tagged extents; lzjb/zle hardened ports in storage module);
  **OQ-T6** multi-vdev (refused, single-disk resolvable only — og extent unknown).
- **B3** — `M` checksum-aware reads at blkptr level.
- **B4 deleted classification** — `NA` with note (documented no-deleted-recovery, CoW
  rationale `cr_zfs_engine.cpp:987–988`; og deleted-ZFS behavior not evidenced in corpus
  — RAWSearch covers content in the og, same posture as Btrfs).
- **B5 orphan** — `M` snapshots surfaced as roots when present in the DSL tree (og
  deep-dive §3 parity; core snapshot surfacing implemented).
- **B7 sparse** — `M` holes via blkptr l0==0 and gang leaders.
- **B8 multi-volume** — **OQ-T6.2** (multi-vdev).
- **B9 timestamps** — `M`-note: core uses `unix_to_filetime` (sub-second dropped) —
  one-second precision where the og's ZPL node carries nanoseconds; recorded as a
  precision note (not graded: og's persisted time semantics for ZFS not decoded;
  revisit with OQ-T6).
- **B10 Unicode** — `M` post-S-005-fix (dnode names, zap child names AND the
  nvlist pool display name decode via `text::utf8_to_u16`;
  `Zfs.NonAsciiNamesDecodedNotByteLifted` covers all three + a CJK pool name).

## Constants and formats

- **C19/C20** — `M` (uberblock magic/slot layout, nvlist entry stride 0x40, types 9/2,
  label geometry); **OQ-T6.1** (og's fixed 0x1C000 anchor vs core's per-label reads —
  core superset pending decode).

## Algorithms

- **A25** — `M` label scan → uberblock → vdev topology → MOS → DSL → ZPL walk.
- **A27** — `M` gang assembly, indirect ladders, per-object codec dispatch.

## State machine

- **S32** — `M` cancel polls in walk loops.

## Performance

- **X51** — `M` per-blkptr reads, 112 KiB label reads, gang 512 B headers.

## Quality

- **Q65 tests** — good (8 areas incl. codec round-trips + tail labels). Gaps: multi-vdev
  refusal path, non-ASCII names (S-005).

## Findings index (this module)

| ID | Severity | One-line |
|---|---|---|
| OQ-T6 | tracked-open | nvlist anchor; multi-vdev DVA resolution; uberblock geometry; checksum depth |
| S-005 | SEVERE | (cross-engine) Names byte-lifted → mojibake — **FIXED (diff-to-fix session)** |

---

## Session 2 re-audit (audited commit `5d6da5f`)

- **B9 timestamps — MATCH re-verified:** core `cr_zfs_engine.cpp:1236-1239,
  1249-1252` `le64(b + SA_LAYOUT1_ATIME)` reads only the first 8 bytes
  (seconds) of the 16-byte SA timestamp attribute. Og
  `ZfsSearchDLL_decompiled.c:24464` calls `FUN_180010ef0(pbVar8, pbVar3,
  ...)` where `pbVar3 = pbVar8 + min(local_28, 8)` — caps the read at 8
  bytes. Both drop the second u64 (nsec). MATCH. (Note: the S-005 fix
  addressed the filename mojibake; the timestamp behavior was correct on
  both sides already.)
- **B9 extra timestamps — I-007 (INFO, superset candidate):** the S-005
  finding's resolution added the ZFS pool display name decoding; this
  session did NOT surface a ZFS-specific extra-timestamp divergence
  beyond what S-005 covered. ZFS B9 verdict: MATCH-with-note (the
  4-timestamp superset pattern of I-007 does NOT apply to ZFS — the og
  reads the same number of ZFS timestamps as the core).

### Session 2 findings index (this module)

(No new ZFS-specific findings in session 2. S-005 was the ZFS B10 finding;
it is now FIXED and lives in `findings/FIXED/`.)

## Session-2 deep pass (T6) — vdev handling decoded (bounded)

- **B1/A25 nvlist anchor** — the og performs ONE fixed read: 16 KiB at 0x1C000
  (parse from +4, limit 0x1BFFC) — `ZfsSearchDLL_decompiled.c :21052`, quote-confirmed.
  The core's per-label reads remain the spec-correct superset.
- **B8 vdev_tree** — the og extracts ONLY the "id" key per child vdev into an id-keyed
  map (`:21089-21150`, key constant "id" extracted from the binary). NO vdev-type
  strings ("mirror"/"raidz"/...) and NO geometry keys ("ashift"/"asize"/"nparity") exist
  anywhere in ZfsSearchDLL.dll (strings scan: 0 hits). **The og build is
  vdev-TYPE-BLIND**: no mirror copy selection, no RAID-Z parity math is possible in it.
  The core's explicit unsupported_vdevs refusal is the honest superset of this.
  OQ-T6 partially resolved — see `tracked-open/` for the refined residual (mirror
  first-child semantics through the id map).
- **B10 names** — the og converts at least one name path via
  MultiByteToWideChar(CP_UTF8, flags=0) (`:24985-25014`, FUN_18001c630) — supports
  M-010's parity verdict for the ZFS edge.
