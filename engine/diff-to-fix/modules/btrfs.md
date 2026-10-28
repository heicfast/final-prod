# modules/btrfs.md — Btrfs engine audit verdicts

Reference: BtrfsSearchDLL per corpus artifacts `SearchDLL/btrfs-engine--deep-dive.md`,
`refs-btrfs-zfs-engine-algorithms-v2.md` §2, `remaining-engines--ext4-hfs-refs-btrfs-zfs.md`
(Btrfs section), `CORRECTIONS.md` row 8.

Core (audited commit `67193b6`): `src/core/btrfs/cr_btrfs_engine.cpp` (799 LOC) +
`include/cr/btrfs/` + `tests/unit/test_btrfs_volume.cpp` (7 areas).

## Behavioral

- **B1 detection** — `M` SB magic "_BHRfS_M" 0x4D5F53665248425F @SB+0x40, SB read 0x1000,
  mirror offsets {0x10000, 0x4000000, 0x4000000000, 0x4000000000000} (core extracted the
  og's .rdata table — ground-truth match, resolves B OQ-1), newest-generation SB wins
  (og rule), device min size 0x11000, sectorsize pow2, `nodesize % sectorsize == 0`;
  **N-006** (core adds nodesize pow2/≤64K/≥sectorsize beyond the og band
  [0x86, 0x86+0xFF7B)); **I-003** (core probes all mirrors vs og abort-on-primary-fail).
- **B2 recovery correctness** — `M` chunk map (logical↔physical), root/FS trees per
  subvolume, inline extents, zlib/zstd mapped to codec-tagged extents; lzo counted-only
  (no lzo codec linked — registered in codec policy; og has zlib only, so core's
  zstd/lzo handling already exceeds og).
- **B3** — `M` node-header checksums (crc32c) validated on metadata nodes (self-validating
  chunk-root discovery); CORRECTIONS #8 honored (no chunk-tree-uuid validation — BTR-2).
- **B4 deleted classification** — `NA` with note: og ships no complete Btrfs scan (NULL
  stub — I-003), and the quick path surfaces live structures only; core's documented
  no-deleted-recovery posture (`(void)root_deleted; :347–348`, CoW rationale) therefore
  has no og counterpart to diverge from. Deleted content on Btrfs comes from RAWSearch in
  the og — see carve module.
- **B5 orphan** — `M` synthetic subvolume names (core's own convention; og's
  CBtrfsQuickTreeBuilder naming not decoded — no contradiction found).
- **B9 timestamps** — `M` unix→filetime_ns.
- **B10 Unicode** — `M` post-S-005-fix (dir-item names decode via the shared
  `text::utf8_to_u16`; `Btrfs.NonAsciiNamesDecodedNotByteLifted`).

## Constants and formats

- **C19/C20** — `M` (SB offsets: magic +0x40, generation +0x48, sectorsize +0x90,
  nodesize +0x94, sys array +0x32B/size +0xA0; key/chunk item layouts).
- **C21** — `M`-note: og tree-construction code 0xB = CBtrfs; core FsKindCode::Btrfs
  value alignment verified in the abi/session wave (see session module — legacy remap
  note).

## Algorithms

- **A25** — `M` (quick-path parity; deep walk is a core superset per I-003).
- **A27** — `M` inline extent handling; compressed-extent codec mapping.

## State machine

- **S32** — `M` cancel per node walk.
- **S35** — `M` mirror fallback (superset per I-003).

## Performance

- **X51** — `M` per-node reads; og's BtrfsCache.txt re-scan cache not replicated
  (perf-only; noted in I-003 as deliberately not replicated).

## Quality

- **Q65 tests** — good (7 areas incl. inline/compressed extents). Gap: non-ASCII names
  (S-005), primary-damaged-mirror detection pin (I-003 test).

## Findings index (this module)

| ID | Severity | One-line |
|---|---|---|
| I-003 | INFO | Core exceeds og: working deep scan + unconditional mirror probing |
| N-006 | MINOR | Nodesize validation stricter than og band (corrupt-SB only) |
| S-005 | SEVERE | (cross-engine) Names byte-lifted → mojibake — **FIXED (diff-to-fix session)** |

---

## Session 2 re-audit (audited commit `5d6da5f`)

- **B9 extra timestamps — I-007 (INFO, superset):** core reads all 4
  Btrfs inode timespecs (`INO_ATIME = 0x70`, `INO_CTIME = 0x7C`,
  `INO_MTIME = 0x88`, `INO_otime = 0x94`) via `cr_btrfs_engine.cpp:83-86,
  625-633, 750-753`. Og's `FUN_180006ba0` at
  `BtrfsSearchDLL_decompiled.c:4430-4446` reads only 3 (ctime + mtime +
  atime); the 4th Btrfs inode timespec at `+0x94/+0x9C` (otime → crtime) is
  NEVER read by the og BTRFS engine. Core superset — every Btrfs file has
  non-zero `created` slot where the og's has zero. Note: og's Btrfs engine
  DOES preserve nsec (`*(uint*)(param_3+0x84)/100` at `:4437`), unlike the
  og's ext4 engine (which drops nsec — see I-006). Internal og inconsistency.

### Session 2 findings index (this module)

| ID | Severity | One-line |
|---|---|---|
| I-007 | INFO | Core reads 4 timestamps (incl. otime→created), og reads 3 (otime dropped) |
## Session-2 update (2026-10-25, core `e93decf`)

**Verdict changes (quotes in `../evidence/decompiled-quotes/btrfs-zfs-refs-session2.md`):**
- **B2/B1 → M-020 (new):** og has chunk-profile machinery — RAID10 geometry gate, 2-copy
  (DUP/RAID1/RAID10) **rotating-mirror** logical→physical resolution, per-node error-map
  integration; core is stripe-0-only. Default-subvolume pick ("default" DIR_ITEM hash
  0x8dbfc2d2, root-5 fallback) vs core synthetic `subvol-<id>`.
- **I-003 re-confirmed with exact quotes:** mirror-0 magic miss ABORTS the SB loop
  (`:10311-12`); core probes all mirrors = superset (registered).
- **N-006 re-confirmed with doc-drift note:** nodesize band upper bound is **inclusive**
  [0x86, 0x10000] (`:10301`) — session-1's "(…+0xFF7B)" text drift only.
- **B3 phrasing corrected:** neither og nor core validates node-header crc32c (core uses
  bytenr/fsid/chunk-tree-uuid identity gates; no crc32c immediates in the og DLL either).
- **S-005 og-anchor for Btrfs: NOT-FOUND** (no conversion import in BtrfsSearchDLL) —
  tracked in OQ-S2M-2.

---

## Session-3 parallel-leg census row (integration 2026-10-26)

| ID | Severity | One-line |
|---|---|---|
| S-028 | SEVERE | XATTR recovery missing: user.DOSATTRIB (Windows attribute semantics), user.* streams, btrfs.compression codec tag — no XATTR (type 0x18) handling anywhere in our engine |

(The multi-stripe/mirror-index gap is the lead leg's M-020; the occupancy-bitmap
cross-engine gap is M-039. This leg's local S-021 renumbered to S-028.)

---

## Session-6 rows (deep continuation, 2026-10-26)

| ID | Severity | One-line |
|---|---|---|
| N-056 | MINOR | Re-scan metadata cache (og persists BtrfsCache.txt subvolume/chunk-tree cache; core cold every scan) + ZFS-side polling in the same file |
| OQ-S6-2 | RESOLVED (s25) — **INVALID premise**: capa false positive (0x180012190 = adler32/crc32 chunk pump, no RC4 essentials anywhere in the binary); erratum E46; no divergence — parity by shared absence |


## Session-33 verdict update (2026-10-26, DFX33 — M-020 FIXED)

**B1/B2 → MATCH (post-fix):** `parse_chunk` decodes the full stripe array
+ geometry; `logical_to_physical` is profile-aware with the og's rotating
mirror for the 2-copy class (SINGLE/DUP/RAID1) — og-exact :2534-2540 — and
the RAID10 geometry gate og-exact :2162-2168. RAID0/RAID10 stripe math is
the REGISTERED SUPERSET (D-79): the og's read path fails both classes
(class 8/0x40 fall to LAB_1800043fe, verified :2521-2583 first-hand —
the session-2 module text's "2-copy (DUP/RAID1/RAID10) rotating-mirror"
phrasing carried the audit's decode error; RAID10 is class 0x40, FAILED
by the og). Raw extents on striped profiles split at stripe boundaries;
compressed extents stay atomic (refused + counted when spanning).

**A26/B5 → MATCH (post-fix):** the default subvolume is picked og-exactly
(the root tree's DIR_ITEM (6, 0x54, 0x8dbfc2d2) spelling "default" with a
ROOT_ITEM located key; fallback root 5) and renders its children directly
at the volume root (ReFS root convention); non-default subvolumes keep the
N-047 containers. `subvol-<id>` remains only the unnamed-non-default
fallback.

**Stats surface:** `unmappable_extents` (profile-refused extents — never
wrong bytes), `mirror_rotations` (non-first-stripe 2-copy accesses).
Error-map integration verified already-by-construction (the Skip-mode
RetryingDevice pipe records sector-granular failures for every engine
read; no per-engine duplicate).

## Session-34 fix leg (DFX34) — N-072 (the re-scan cache half)

| Parameter | Verdict |
|---|---|
| P47/P48 re-scan throughput | **MATCH (mechanism; format REGISTERED as ours)** — the og's BtrfsCache.txt (string evidence :66-67, format undecoded) is answered by `cr_btrfs_cache.h`: OFF by default, caller-supplied dir (the og's beside-the-DLL placement is a documented hazard NOT copied), fsid-keyed single size-bounded file, generation inside as the strict match-or-discard gate; a valid blob skips the chunk chase/walk + root probe (warm-scan nodes_read strictly lower, tree output identical) |
| S36 cross-scan lifecycle | **MATCH** — the cache spans scans via the host seam (BtrfsCacheIo), never beside the engine; trees are re-read and re-validated every scan |

## Session-34 fix leg (DFX34) — N-072 (the ZFS retry half)

| Parameter | Verdict |
|---|---|
| A25 label import steps | **MATCH** — the og's delay-execution import polling (:86) = the bounded label retry: live handles only (is_static_media discriminator), 250/500/1000 ms backoff, cancel-aware in 50 ms slices, label_retries counted, 3-retry bound then ERR_BAD_MAGIC |

## Session-10 S10-F rows (function-level completeness pass, 2026-10-29; audited `44f043a5`)

| Parameter | Verdict |
|---|---|
| A25/A26 descent filter — the `@eaDir` subtree suppression | **MATCH — M-110 FIXED (session 39B)** (the og's FUN_180001cd0 walk :704-711 skips the child walk for a directory named EXACTLY `@eaDir` — the compare is 7 bytes INCLUDING the literal's NUL terminator, instruction-verified against the binary: `@eaDirBackup` descends normally; the finding's prefix-match decode corrected. The core's populate() now emits the @eaDir node EMPTY and never walks its children — Synology EA directories match the og's tree shape; the M-018 boundary pinned by the not-in-pools guard test) |
| class surface — CBtrfsQuickSearch / CBtrfsFileProerties / CBtrfsNoResidentDataRunList / CBtrfsQuickTreeBuilder | **MATCH (semantic)** — the T3b class pass: the quick controller's vtable + walk (FUN_180002120 :843-928, the only og controller per I-003), the props field set (N-059: no og field unpersisted; id_b always-0 registered), the non-resident extent adapter (chunk-map resolve + rotating mirror, M-020), the DRWUI-side tree builder (B5) — evidence s10f-product-frame-register.md §5 |
