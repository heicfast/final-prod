# MINOR N-008 — ZIP carve padding: off-by-one cap + 16-byte-granular zero test vs the og's sector-aligned trailing-16-of-sector test
# MAJOR M-005 — Carve scans the whole slice; the og's volume-scan carve is bitmap-gated (free-space only)

Two carve-pipeline divergences filed together.

---

## N-008 — ZIP padding semantics

**Severity:** MINOR · **Module:** carve · **Parameters:** A28, C20
**Corpus references:** decompiled `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c`
:18492–18500 (agent-B verified): padding extension loop cap `0x500001` (5,242,881), test =
**last 16 bytes of each SECTOR** are zero (`[s-4]==0 && [s-8]==0 && [s-12]==0 && [s-16]==0`
at sector-aligned positions), stepping by `sector_size` (device-vtable-sourced, 4 Kn
aware).
**Core references:** `src/core/carve/cr_analysers.cpp:582, :641–655` — cap 5,242,880
(off-by-one vs 0x500001) and a **16-byte-granular** zero-block walk from an arbitrary end
(not sector-aligned) — audited commit `67193b6`.

**Gap:** (a) cap differs by exactly 1 byte (5,242,880 vs 5,242,881); (b) the og extends
only at sector boundaries and tests the trailing 16 bytes of each sector, core extends at
every 16-byte boundary — core can absorb up to 511 bytes more padding per file than the og
on 512-byte-sector disks (and 4095 on 4Kn), inflating carved size slightly; (c) the og
queries the device's real sector size; core hardcodes the 16-byte granularity.

Also in the same validator family: the og's ZIP probe admits `version_needed ≤ 0x13 OR ==
0x62` (:18294, verified — the v2 artifact's "≤ 10" is wrong); the core's indexless walker
breaks on `csize == 0xFFFFFFFF` (ZIP64 marker) while the og's chain-walk continues via
parsed entry sizes. ZIP64-era indexless archives may truncate in core.

**Fix plan:** align cap to 0x500001; make the padding walk sector-strided with the
trailing-16-of-sector test using the slice's sector size (available via the device);
admit version 0x62 in the LFH gate; on `csize == 0xFFFFFFFF` look for the ZIP64 extra
field (0x0001) before breaking. Files: `src/core/carve/cr_analysers.cpp` (zip_common).
**Tests:** `Zip_Padding_Sector_Aligned_Extension`, `Zip_V062_Admitted`,
`Zip64_Extra_Field_Continues_Chain`.

---

## M-005 — Carve target policy (whole slice vs free-space)

**Severity:** MAJOR · **Module:** carve, orchestration · **Parameters:** A25, B3, X47
**Corpus references:** `re-workspace/artifacts/SearchDLL/searchdll-family--architecture.md`
§6 step 4 (agent A/B cross-read): the RAW searcher's scan is an
"unallocated-cluster/free-space pass driven by the volume bitmap" supplied by the FS
engine (IBuildVolumeBitmap interface — the NTFS controller IS the bitmap builder,
deep-dive §1); on raw/lost regions (no bitmap) it scans the region whole.
**Core references:** `src/core/orchestrator/cr_scan.cpp:482–492, :901–923`
(`run_carve_stage` → `carve::carve_slice(pipe, ...)` over the **whole volume slice**;
no bitmap consultation — grep: cr_bitmap.h is included for engine viability, never for
carve gating) — audited commit `67193b6`.

**Gap:** on volume scans (quick/deep), the og carves only unallocated clusters; the core
carves the entire volume, including clusters owned by live files. Consequences cut both
ways: (a) the core surfaces remnants inside allocated regions the og never inspects
(recovery-positive superset); (b) the core's carve layer generates candidates that
duplicate live/structured files and must be deduped downstream (the GAP-S6-8 fix added
carve-vs-structured dedup on the absolute-extent basis — necessary *because* of this
policy), and its false-positive surface (raw-sign layer accepts unvalidated when
tail-paired) is exposed to allocated regions where the og's carver never runs.

**Impact:** scan-time cost (carving 100% of the volume vs the free fraction — on a
half-full volume that is ~2× the carve stage duration); result-set shape differences on
every volume scan; FP-risk surface difference.

**Fix plan:**
1. For volume scans where the engine produced a valid allocation bitmap (NTFS $Bitmap,
   exFAT/FS bitmap), pass a free-space mask into `carve::carve_slice` (new
   `CarveFlags::free_mask`) and restrict window emission to unallocated runs (plus 4 KiB
   boundary slack) — matching the og.
2. Keep whole-slice carving for lost/raw regions (no bitmap — og parity).
3. Register the decision (or the deliberate choice to keep whole-slice for salvage-max)
   in `docs/parity/PARITY-DELTAS.md` with the perf numbers from (1).
4. Tests: `Carve_FreeSpace_Mask_Skips_Allocated` (NTFS volume with live files → carve
   stage emits nothing for fully-allocated live extents); `Carve_Lost_Region_Whole_Slice`
   (pin current behavior).

**Bench note:** the quick_format scenario (FAT tables zeroed) has no bitmap either way —
unaffected.

---

**Related findings:** S-007 (format coverage), S-001-family honesty caps.
**Open questions (OQ-T8):** og dispatch order u16→u32→signs (INFERRED); og whole-device
behavior when no bitmap exists (raw disks) — corpus suggests whole-region, consistent
with the fix plan's step 2.

## Session-2 housekeeping (2026-10-25)

The **M-005 (carve bitmap gating) content** now has a canonical standalone file
(`MAJOR/M-005-carve-bitmap-gating.md`) carrying the decompile proof this leg landed
(`FUN_18000c820 :9446-60` bit test + cluster skip + dispatch suppression). The ZIP half of
this group is refined by **N-027d**: the og's padding loop absorbs NON-EMPTY trailing
sectors after a broken record chain (salvage, `:18462-500`) and never inflates clean
archives; the core absorbs zero-16-byte-blocks after the EOCD (inflating intact ZIPs with
zero slack); version gates are two-tier (quick `≤0x13 || ==0x62` at `:18288-96`;
first-record LFH `≤10` + namelen ∈ [1..259] at `:18389-99`) where the core has none. OQ-T8's
dispatch inference is resolved by this leg (u32 → u16 → list, first-accept-wins — the
artifact's u16-first claim was wrong; see
`../evidence/decompiled-quotes/carve-rawfixer-bitlocker-session2.md`).
