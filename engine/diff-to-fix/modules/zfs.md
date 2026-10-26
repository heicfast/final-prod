# modules/zfs.md — ZFS engine audit verdicts

Reference: ZfsSearchDLL per corpus artifacts `SearchDLL/zfs-engine--deep-dive.md`,
`refs-btrfs-zfs-engine-algorithms-v2.md` §3, `zfs-vdev--mftmirr--calcpartition.md` §1,
`remaining-engines--ext4-hfs-refs-btrfs-zfs.md` (ZFS section).

Core (audited commit `67193b6`): `src/core/zfs/cr_zfs_engine.cpp` (1,362 LOC — the
largest single-engine file) + `include/cr/zfs/` + `tests/unit/test_zfs_volume.cpp`
(8 areas).

## Behavioral

- **B1 detection** — `M` uberblock magic 0x00BAB10C (4 decompile sites verified), txg at
  UB+0x10, ring-walk: EVERY valid uberblock state is walked newest-first with
  cross-state identity dedup (S-034 FIXED — the og's quick searcher iterates every
  label-0 ring slot and walks each txg's tree; the old newest-only verdict was a
  refuted premise); vdev label probing (4 labels, front/back); core adds
  shift-probing 10..13 (4 Kn pools) + all-four-labels ring collection — superset,
  register; BE-pool refusal (`ERR_UNSUPPORTED`) — documented deliberate hardening.- **B2 recovery correctness** — `M` nvlist XDR+native decode, micro/fat ZAP, gang blocks
  (3-blkptr headers), BPE (embedded block pointers), per-object compression registry
  (LZJB/LZ4/zle/gzip → codec-tagged extents; lzjb/zle hardened ports in storage module);
  **OQ-T6** multi-vdev (refused, single-disk resolvable only — og extent unknown).
- **B3** — `M` checksum-aware reads at blkptr level.
- **B4 deleted classification** — `M` (S-034 FIXED): the ring-txg-history walk recalls
  deleted-in-window files as Deleted nodes (the og walks every ring state; the old
  "no-deleted-recovery, CoW rationale" note was the refuted newest-only premise —
  deleted content beyond the ring window still surfaces via RAW carving only, same
  posture as Btrfs).
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
  **s31 (OQ-S2M-2 #2):** the og NEVER displays a pool name — ZfsSearchDLL has
  0 "name"/"pool" hits and DRWUI's FS-name builder (FUN_140510080) has no case
  13 → "Unknow" in every consumer (XML `filesystem` field, CalcPartition log).
  The core's pool-name decode is therefore a deliberate superset (more info
  than the og surfaces), not a divergence; registered in PARITY-DELTAS terms
  as a display-information superset.

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
| OQ-Z1 | RESOLVED (s25) — **A-008 ACUTE filed+FIXED**: child zap was read at +24 (the dd_origin_obj slot); kernel +32; children silently missing on real pools; ghost datasets on clones. Fail-before RED 3/3; 739/739 after |
| OQ-Z2..Z5 | OPEN | txg decider (:2162-2170); multi-vdev DVA; decompression census (no zlib/lz4 strings in the og binary — it may not decompress at all); Quick-vs-Complete controller delta |

Probes: `tracked-open/OQ-Z-E-FLT-session3.md`. (Lead leg's M-019 covers the
multivdev-fallback/checksum side.)

---

## Session-6 rows (deep continuation, 2026-10-26)

| ID | Severity | One-line |
|---|---|---|
| N-056 | MINOR | Delay-execution label/uberblock import polling missing (og retries with backoff during long walks; core probes once — torn pools fail immediately) |
| INFO-register | — | Core decompresses lzjb/zle/gzip/lz4/zstd (cr_zfs_engine.cpp:19-21); the og models per-object compression (std::map<int64,zio_compress>, remaining-engines:80). No divergence claimable in either direction (both read compressed pools); recorded for the decode-evidence ledger |


---

## Session-33b row (S-034 fixed, 2026-10-26)

| ID | Severity | One-line |
|---|---|---|
| S-034 | SEVERE → **FIXED** | Ring txg-history walk: every valid uberblock state walked newest-first, cross-state identity dedup (dataset, parent, name, objid, bp0 birth); deleted-in-window files surface as Deleted in place; modified-in-window recalls the prior version as a Deleted sibling; torn states skipped like the og's tree-open failure. Decode items (1)(2)(3) answered first-hand (collector = plain push_back, no engine dedup; per-state tree walk is the state's own content; last_txg display-only). Tests: RingWindowDeletedFileRecalled / RingWindowUnchangedEntriesEmitOnce / RingWindowModifiedFilePriorVersionRecalled / RingTornNewestStateFallsBackToOlder (all fail-before RED). `ring_states` telemetry added |

## Session-34 fix leg (DFX34) — N-072 (the label-retry half)

| Parameter | Verdict |
|---|---|
| A25/S35 label import + recovery | **MATCH** — the og's delay-execution import polling (remaining-engines artifact :86) landed as the bounded label retry: the sweep re-runnable, live handles retry 3x with 250/500/1000 ms cancel-aware backoff, static media (image files — the new `is_static_media()` device discriminator, forwarded through every wrapper) never retries; `label_retries` on ZfsScanStats; the BE-pool refusal propagates unchanged |


## Session-37 row (DFX37 — the convergent S-034 landing; the DFX33b engine stands)

| ID | Severity | One-line |
|---|---|---|
| S-034 | SEVERE → **FIXED** | CONVERGENT second fix the same window (the DFX33b landing above is canonical per first-landing-wins): same corpus decode executed independently (ring iteration FUN_180006b90 :5685-5841, addBlock chain, FUN_180012450 plain ctor = no engine dedup, last_txg display-only :5703); the DFX37 variant (deleted-tree parallel emission, identity = head+objid+logical birth @blkptr+80) superseded — its Resolution preserved in findings/FIXED/S-034 as the convergent record, including the fixture block-placement lesson (a second state's 16K dnode-array footprint spans +0x4000; the root ZAP must sit past its end). |