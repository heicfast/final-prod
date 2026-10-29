# S50 evidence — the ifileproperties-part1 full census (the under-cited flag discharged)

**Lane:** S50 continuation audit lane. **Artifact:**
`re-workspace/artifacts/SaveScan/ifileproperties-part1--ntfs-fat-exfat.md`
(189 lines, read in full — the artifact the SURFACE-INVENTORY flags as
"under-cited (×1)" on the ntfs/fat/exfat/session module rows).
**Pins:** corpus `be0cfe9`, core `93461483` (code-identical to `0d7b1d9d`).
The census table below was produced by the part1 census lane and its
load-bearing new claims (the clone() short_name drop, the +0x80 constant
family) were re-verified first-hand by the lead before filing.

## Census summary (item-level)

**MATCH 51 · DIVERGENT 13 · ABSENT 4 · N/A 7.** Of the 17 non-MATCH rows,
15 are ALREADY OWNED by the trail:

| Row | Owner |
|---|---|
| slot +0x20 / NTFS +0x60 "type-registry ID" reading | N-100 (FIXED, nflags bit9) — the label refuted, the datum carried as ntfs_total_clusters |
| +0x38 FLAGS bit family (5 rows: NTFS/FAT/exFAT flags + bit census) | F-A1-5/NTFS-5 + s10-table C4 (the deliberate convention, cr_mftwalk.h:31-37) |
| +0xA0 content type-code table vs the factory map | Deliberate F16 (cr_types.h:72-93 = the og's own FACTORY map; legacy remap cr_records.cpp:379-381) |
| 10-class polymorphic vtable -> single struct | D-21 (in-process shared node library) |
| +0x64 "secondary ID" label | Corpus erratum E68 + OQ-SA-1 (FIXED, nflags bit8, ntfs_origin_lcn) |
| DataRunList raw datarun bytes -> resolved extents | The registered canonicalization (s10-field-parity-table) |
| FAT OEM-byte/deleted-glyph name presentation | N-087/N-172/N-176 (all dispositioned) |
| NTFS +0x40 / FAT +0x42 parent anchors; exFAT +0x54 | N-131 (open — the battery re-verified it LIVE) |

**The two NEW items (filed this session):**

1. **N-189 — `FileProperties::clone()` drops `short_name`.** The og's clone
   copies the whole props block (the core's own comment admits the contract:
   cr_node.cpp:75-76 "the og's clone copies the whole props block"); the
   core's clone (cr_node.cpp:49-80, read first-hand) copies name, name_raw,
   times, extents, both NTFS anchors, carve provenance and the hash — but
   NOT `short_name`. Fire path: the hard-link fan-out clone
   (cr_mftwalk.cpp:981-992, `(!flags.single_link_per_record && parents>1)
   || link_name != best`) and the UI preview clone (cr_ui.cpp:3795) — a
   cloned link loses the 8.3 name the canonical props captured at
   cr_mftwalk.cpp:856-865 (N-011a). The field persists in sessions
   (cr_records.cpp:156-157/:207-212, nflags bit2), so cloned links' session
   records lose the axis. No existing finding covers it (M-007 covers the
   fan-out shape; N-011a/N-024 landed the field, not the copy).

2. **I-047 — the +0x80 per-class format/icon-ID constant family.** The
   part1 artifact documents the vtable's +0x80 slot returning per-class
   constants NTFS 0x2FF / FAT 0xFF / RAW 0x1CF ([CODE]-backed). DRWUI-side
   decode (first-hand): `FUN_1405209f0 { return 0x2ff; }` (:853378-853381)
   and `FUN_140c41350 { return 0x1cf; }` (:1875864-1875873) — the statically
   linked copies of the class vtables. The core has no carrier (grep
   `0x2FF|0x1CF` over src/ -> 0 hits) and its display axis is the FileFilter
   extension category (cr_query.cpp:289-293, M-029). The og-side CONSUMPTION
   of the constant remains undecoded (the +0x80 call sites found in
   DRWUI return bytes — other vtables; part1 itself does not decode the
   consumer), so per the no-hallucination rule the disposition is an INFO
   register with a precise open question, not a divergence finding.

## The four module rows' under-cited flag

Discharged: the census walked every slot (+0x00..+0xB0), the second
type-code/factory table, the FPL-root stamping, and the NTFS/FAT/exFAT
field-by-field schemas (every offset/width row), and reconciled with the
trail's own prior counts (modules/session.md:218-239's 16 MATCH / 1 MISSING
/ 1 UNDETERMINED / 5 N-A). The session-module counts are consistent with
this slot-level table; the part1 artifact is now fully cited-by-disposition
(none of its rows hide an unfiled divergence except the two items above).
