# OQ-T2 (tracked-open) — CDoubleFATTable primary→mirror switch trigger undecoded
# OQ-T5 (tracked-open) — og exFAT ChecksumDBR per-byte predicate undecoded (skip-set question)

Two independent unresolved reference behaviors, filed together for one Ghidra session.

---

## OQ-T2 — FAT mirror switch

**Status:** RESOLVED (session-2 decompile read, first-hand — trigger decoded; cross-check intent refined below) · **Module:** fat · **Parameters:** A27, S35
**Resolution (2026-10-25, session 2):** the og consults FAT2 **per-entry, only when the
primary's canonicalized GetNext(cluster) returns 0 (FREE)**. Non-zero primary results —
**including BAD** — are served from the primary with no mirror consult
(`FATSearchDLL_decompiled.c :54215-54219`, full quote in
`evidence/decompiled-quotes/s2-fat-double-fattable.md`). This resolves the INFERRED
"choose-healthy" semantics of the artifact docs: it is a per-entry zero-fallback, not a
whole-copy validation or majority vote. Two consequences:
1. **N-022 filed (MINOR):** the core consults the mirror on primary FREE *or BAD*
   (`cr_chain.cpp:104-110`); the og consults only on FREE. On torn FAT pairs where the
   primary marks a cluster BAD but the mirror retains the real chain link, the core
   continues the chain (recovers a longer file) while the og truncates at the BAD
   cluster — a result-set superset the og does not produce.
2. When the mirror IS consulted, the og **gates** the mirror's answer (dirty-EOC family
   accept-set and three mirror self-consistency cross-checks — verbatim in the evidence
   file) instead of serving it raw; the core serves any non-zero/non-BAD mirror value.
   The exact intent of the three cross-checks (deleted-chain reconstruction gating for
   quick-format volumes) is recorded with quotes; next decode target if ever needed:
   the CFatCompleteSearch chain-walk caller of IFATTable slot 2.
Also decoded: the og has a **load-time FAT-location fallback** — when the declared FAT
region yields an invalid CDoubleFATTable (both children null) and volume flag 0x10 is
set, it walks an internal LBA-keyed structure map for an alternative FAT image and
rebuilds the double table from there (:3805-3949, quoted) — recorded in
`modules/fat.md` as an observation for the remnant path.
**Corpus artifact to examine next:** `re-workspace/reversed/ghidra-searchdll/FATSearchDLL_decompiled.c`
— `CDoubleFATTable` ctor `FUN_180041d00` and its `Next` override; the composition wrapper
(fields +8/+0x10 child IFATTables).

**Precise question:** when does the og consult FAT2? Our core consults the mirror per-entry
when the primary reads FREE (0) or BAD
(`src/core/fat/cr_chain.cpp:102–110`, audited `67193b6`). The corpus documents
CDoubleFATTable as "divergence tolerated (choose-healthy), recovery when one copy torn"
(`fat-exfat-engine-internals.md` §2) but marks the exact fallback switch **INFERRED**
(`fat-exfat-engine-algorithms-v2.md` §1).

**Why it matters:** on volumes with one torn FAT copy, the entry the core serves (and thus
chain shape) may differ from the og's if the og's trigger is e.g. whole-copy validation or a
majority vote rather than per-entry 0/BAD fallback.

**Suggested probe:** decompile `CDoubleFATTable::Next`; identify the branch that selects
child +8 vs +0x10; record the compared condition (entry equality, zero-ness, BAD-ness, or a
pre-validated copy flag set at load time).

---

## OQ-T5 — exFAT boot checksum predicate

**Status:** RESOLVED (session-2 decompile read + binary extraction — MATCH) · **Module:** exfat (+ rawfixer) · **Parameters:** B1, C19, F63
**Resolution (2026-10-25, session 2):** the per-byte predicate of `ExFATFixer::ChecksumDBR`
(FUN_18000e7e0, `RawFixer_decompiled.c :9353-9381`) is `std::find` (FUN_18003ab40,
:41083-41092) over a 4-int array at RVA 0x180079a40, extracted from the binary:
**{106, 107, 112, −1}** — exactly the exFAT spec skip-set (VolumeFlags@106-107,
PercentInUse@112) plus a −1 terminator no loop index can match. The loop bound is
`sector_size × 0xB` (11 sectors) with the spec rotation. **The core's spec-skip-set
implementation (`cr_exfat_dentry.cpp:26-36`) matches the og exactly — OQ-T5 → MATCH.**
Full decode: `evidence/decompiled-quotes/s2-exfat-checksumdbr.md`. Corpus-erratum
confirmation: `rawfixer-v2--method-inventory.md` §4's "bytes 11..209 + 510" coverage
description is wrong (the coverage is 11 sectors with skip-set {106,107,112}) — feed back.

**Original question (for the record):**
**Corpus artifact to examine next:** `re-workspace/reversed/ghidra-partition/RawFixer_decompiled.c`
— `ExFATFixer::ChecksumDBR` = `FUN_18000e7e0` (:9353–9381), the exception-machinery
predicate path (`FUN_180023220`/`FUN_180001700`/`FUN_180023230`).

**Precise question:** the loop computes the rolling checksum
`c = ((c & 1) << 31) + (c >> 1) + byte` over `sector_size × 11` bytes, but each byte passes
through an undecoded predicate (exception_ptr plumbing confounds the decompile). Does the
predicate skip exactly the spec's skip-set (bytes 106/107/112 of sector 0), nothing, or
something else?

Our core implements the spec skip-set exactly
(`src/core/exfat/cr_exfat_dentry.cpp:26–36`, audited `67193b6`; shared with the repair
engine). If the og's skip-set differs, og-stored checksums on real volumes (written by
Microsoft's formatter, which follows the spec) would fail the og's own ChecksumDBR — which
would be observable in the wild; the strong prior is spec-skip, but the decompile must settle
it.

**Note:** the artifact `PartitionRecovery/rawfixer-v2--method-inventory.md` §4 describes the
coverage as "bytes 11..209 + 510", which contradicts the verified loop bound
(`sector_size × 0xB`). The loop bound is authoritative; the artifact description is corpus
erratum candidate #4 (feed back).

**Why it matters:** ChecksumDBR gates RawFixer's repair decisions (and possibly the volume
analyer's acceptance); a skip-set mismatch flips repair verdicts on volumes where
VolumeFlags/PercentInUse differ from the checksummed state.

**Suggested probe:** Ghidra on FUN_18000e7e0; resolve the predicate's exception path (likely
a bounds/`operator[]` guard artifact from `std::vector` access); enumerate skipped indices;
compare with {106,107,112}.
