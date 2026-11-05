# s10-field-parity-table.md — the ifileproperties ↔ cr_records per-field parity table (N-131 evidence)

**Session 10 (2026-10-29). Core `84a1832`. Corpus `be0cfe9`.**
**Method: parallel deep-read agent produced the full per-family table from the three
ifileproperties artifacts + rsf-record4 grammar vs `cr_records.cpp`/`cr_node.h`; the lead
re-verified the load-bearing rows first-hand (the persisted-field block `cr_records.cpp:175-270`,
the +0x60 decline comment `cr_node.h:209-216`, the parent-anchor absence greps, the
HFS+/Btrfs/ZFS secondary carriers).**

## Scope

og side = record-4 per-node persistence (wrapper grammar + per-FS content object,
parts 1-3). Core side = `serialize_tree`/`deserialize_tree` (`cr_records.cpp:89-571`,
REC_RESULT_TREE) — the only per-node persistence.

## Verdict summary

- **MATCH (with re-encoding notes)** on every load-bearing per-FS field family: names
  (incl. `name_raw` byte preservation, N-016b), sizes, times (core = 4 qwords, og = 3),
  markers/deleted-state, own-file anchors (`fat_origin_cluster` bit7 :258-259,
  `ntfs_origin_lcn` bit8 :268-269 — the OQ-SA-1 landings), runlists (re-encoded to
  absolute extents; og persists verbatim datarun bytes — registered canonicalization),
  HFS+ secondary/type carriers (:248-249), Btrfs id-a/id-b/generation (:244-246), ZFS
  secondary flags (:247), the full RAW carve-provenance block (:221-237), root stats
  (M-047), grade/overlap (S-023), short names (N-011a).
- **SUPERSET (core-only, 13):** `times.changed`, `allocated_size`, `attributes` u32,
  `record_ref` (own identity for NTFS/HFS+/ZFS where the og persists none),
  `origin`/`confidence`, `grade`, `sha128` content hash, `carve_confidence` 0..1000,
  `name_raw`, `resident_data` bytes, per-extent `codec/psize`, `short_name` as a distinct
  field.
- **ABSENT (og persists, core does not — the new items):**
  1. **NTFS parent-reference anchor** — part1 :84 (qword +0x40; the FRN-family parent
     datum OQ-SA-1's mask-read touched at :61-65 but never dispositioned for persistence).
     Core: structural `parent_id` only (cr_records.cpp:175); `rg parent_anchor|parent_reference|parent_frn`
     over src/+include/ → **0 hits** (verified first-hand).
  2. **FAT parent-directory anchor** — part1 :129 (u32 +0x42). Same absence.
  3. **exFAT +0x54 u32** — part1 :148 (the anchor twin; the census "type-registry ID"
     reading is undermined by the same OQ-SA-1 refutation pattern as NTFS +0x60).
  4. *(owned already)* NTFS +0x60 total-clusters — **N-100** (MINOR, active). The
     `cr_node.h:209-216` decline rationale re-verified verbatim this session.
  5. *(owned already)* the wrapper base-flag/scalar set — **M-050** (FIXED) registered
     the deliberate not-persisted set; this session's table extends the named set: the
     resolution names hidden-root/f_6c/f_68/f_71 but NOT `f_48`(−1), `flag_91_bit0`,
     `flag_72` bits, `flag_73_bit1`, `f_1c8/f_1d0/f_1d4`, `f_1e4/f_1e8/f_1ec/f_1f0`,
     `f_200/f_208`, `f_288/f_289` — same class (undecoded semantics, carrying opaque
     bytes would invent structure), recorded here (M-050 itself retired to history by the parallel purge; this table is the surviving completeness record).
  6. CGeneric +0x3A secondary u16 — constant 0 across all 6,959 sampled records
     (part3 :102); zero information loss; recorded here, no finding.
  7. HFS+ system-node class marker (marker-1 `HFSPlusSystemNodeProperties` variant,
     part2 :36-38) — representational only (the core's fs_kind carries the family; the
     og's named volume-node emission is N-047/FIXED); recorded here, no finding.
- **Container notes:** the u16 name-length cap vs the og's u64 (ZFS) prefix is bounded by
  ZFS's own 255-byte filename limit — no real capacity divergence. The per-child
  `present_marker` resync point is replaced by the container CRC32 (cr_crsf.cpp:324-328).

## The new finding

Items 1-3 → **N-131** (MINOR, the N-100 precedent class: FS-native cross-check datum the
og persists and the core declines; structural `parent_id` preserves the hierarchy, so no
recovery-correctness impact — the loss is reload-time orphan cross-validation and
FS-native lineage re-derivation).

## Caveats recorded for the corpus

- **C1 (→ erratum E69):** part2's ReFS table omits FLAGS while §4 claims invariance; the
  population-site read (`s10-refs-population-site.md`) shows +0x39 flag-bundle-derived.
- **C2:** no ext4 generation field exists on either side (the expectation was wrong).
- **C3:** NTFS +0x60/+0x64 semantics per OQ-SA-1's refutation-and-remap.
- **C4:** the core's `flags` u16 is its own convention (fat_quick.cpp:363-368 warns
  explicitly); og FLAGS semantics are redistributed across deleted_state/attributes.
