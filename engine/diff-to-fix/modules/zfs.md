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
## Session-2 update (2026-10-25, core `e93decf`)

**Verdict changes (quotes in `../evidence/decompiled-quotes/btrfs-zfs-refs-session2.md`):**
- **OQ-T6 RESOLVED:** og probes the FRONT label region only (112 KiB label @0x4000 + 128 KiB
  ring @0x20000; no L1-L3); anchors on "vdev_tree"/"id" only (never name/version/guid);
  newest-txg selection; core's label-field reads are a superset (registered, I-010).
- **B1/B2 → M-019 FIXED (s19):** multi-vdev — the core now resolves every DVA through
  the og's first-entry fallback (foreign reads counted), verifies fletcher4 for checksum
  types {7, 0} over the raw bytes with the 32-byte stored compare, retries the next DVA
  on mismatch/read-failure, and plans L0 extents at the fallback position. Fixtures are
  now checksum-carrying pools. Resolution in findings/FIXED/M-019.
- **B11 → M-038 FIXED (s19):** SA decode is registry-driven — the og's three-object
  chain (master "SA_ATTRS" -> {REGISTRY, LAYOUTS}), by-name attribute resolution,
  offsets by cumulative registered lengths, and the zero-attrs surface for unregistered
  layouts (the og zero-fills at :17636; the core previously DROPPED the object).
  Layout 1 keeps the implicit-default fallback for registry-less pools. Resolution in
  findings/FIXED/M-038.
- **D15-adjacent:** og grading = ZfsDecider txg-based block decisions (from_txg/birth_txg);
  shared-controller grading is undecodable in the DLL (consistent with OQ-S2M-1).
- **Matches confirmed:** DVA encoding (+4 MiB vdev skip), BPE embedded-block layout,
  LZ4-with-BE-length compression site (core's codec registry = superset, registered),
  "root_dataset" enumeration anchor, FS code 13.

### Session-2 addendum (post-reconciliation — engaging the parallel leg's OQ-T6 stance)
The parallel leg's OQ-T6 close-out calls the core's `unsupported_vdevs` refusal "the honest
superset." This leg disagrees on parity grounds with quoted evidence: the og **attempts**
foreign-vdev reads and falls back to the scanned disk (`FUN_1800168b0 :19544-56` — never
fails), so mirror members read correctly and stripe members yield their local halves;
refusing loses that partial yield. RESOLVED s19 in favor of recovery-yield exactly as
this leg argued: the fallback + fletcher4/DVA-retry payload landed (findings/FIXED/M-019).
Label superset rows unchanged (I-010).

---

## Session-3 parallel-leg census rows (integration 2026-10-26)

| ID | Severity | One-line |
|---|---|---|
| M-038 | MAJOR | SA decode layout-1-hardcoded (og is registry-driven by NAME: ZPL_PARENT :24324 / ZPL_CRTIME :24807); non-1 registered layouts drop objects entirely |
| OQ-Z1 | OPEN | dsl_dir/dsl_dataset offsets (ours child_dir_zapobj@24/ds_bp@128 vs kernel @32) — ACUTE-gated probe on a real pool |
| OQ-Z2..Z5 | OPEN | txg decider (:2162-2170); multi-vdev DVA; decompression census (no zlib/lz4 strings in the og binary — it may not decompress at all); Quick-vs-Complete controller delta |

Probes: `tracked-open/OQ-Z-E-FLT-session3.md`. (Lead leg's M-019 covers the
multivdev-fallback/checksum side.)

---

## Session-6 rows (deep continuation, 2026-10-26)

| ID | Severity | One-line |
|---|---|---|
| N-056 | MINOR | Delay-execution label/uberblock import polling missing (og retries with backoff during long walks; core probes once — torn pools fail immediately) |
| INFO-register | — | Core decompresses lzjb/zle/gzip/lz4/zstd (cr_zfs_engine.cpp:19-21); the og models per-object compression (std::map<int64,zio_compress>, remaining-engines:80). No divergence claimable in either direction (both read compressed pools); recorded for the decode-evidence ledger |
