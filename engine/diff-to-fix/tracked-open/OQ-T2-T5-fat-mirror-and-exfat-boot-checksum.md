# OQ-T2 (tracked-open) — CDoubleFATTable primary→mirror switch trigger undecoded
# OQ-T5 (tracked-open) — og exFAT ChecksumDBR per-byte predicate undecoded (skip-set question)

Two independent unresolved reference behaviors, filed together for one Ghidra session.

---

## OQ-T2 — FAT mirror switch

**Status:** UNVERIFIED · **Module:** fat · **Parameters:** A27, S35
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

**Status:** UNVERIFIED · **Module:** exfat (+ rawfixer) · **Parameters:** B1, C19, F63
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
