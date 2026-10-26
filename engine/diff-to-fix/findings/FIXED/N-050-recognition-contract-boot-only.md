# MINOR N-050 — Recognition contract shape: the og's analysers prove content (MFT $Volume, FAT root-dir) before engine dispatch; the core's probe_family is boot-bytes-only

**Severity:** MINOR
**Module:** recognition (orchestrator probe contract)
**Parameters affected:** P3 (false-positive rate — late vs early rejection), P25 (algorithm — two-stage recognition), P42 (surface — probe contract), P54 (module boundaries — analyser layer absent)
**Corpus references:** `re-workspace/reversed/ghidra-mo/NTFSFileSystemAnalyser_decompiled.c` (:4259-4370 — MFT content proof before recognition, see M-043); `re-workspace/reversed/ghidra-mo/FATFileSystemAnalyser_decompiled.c` (:1385-1500 — root-directory verification after FAT-type classification: fixed-root scan for FAT12/16, root-cluster chain walk for FAT32 with 8.3/LFN label extraction `FUN_180002180` :1124-1144; lenient: no "FAT12"/"FAT16" string requirement, unlike FileSystemCheck); `re-workspace/reversed/ghidra-mo/ExtFileSystemAnalyser_decompiled.c` (:3404-3425 — superblock arithmetic cross-checks); the analyser factory contract (`IVolumeAnalyser`, CMomCloneObject functors, NFA:724-729/:939-1102, FFA:1995-2001)
**Core references:** audited `7cf6754` — `src/core/orchestrator/cr_scan.cpp:473-530` (`probe_family`: OEM strings + 55AA/bps + magics only; no secondary content read on any family); engines validate late (boot gates inside each engine, then structured walk failure drops the volume after engine invocation)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og has a dedicated analyser layer whose contract is two-stage: boot-gate, then a cheap content proof (NTFS: MFT $Volume signature; FAT: a readable root directory / label; ext: superblock arithmetic) — recognition happens BEFORE any engine is dispatched. The core folds recognition into `probe_family` boot-byte checks and lets the engines reject late. The concrete NTFS case is M-043; this finding registers the general contract-shape divergence (all families) so it is tracked even where the per-family outcome is benign.

## What the reference does

See the per-family citations; the FAT analyser's root-dir proof and the
ext arithmetic cross-checks are decoded in
`evidence/decompiled-quotes/s5-image-fscheck-persistence.md` §C.

## What our core does

Boot-bytes-only dispatch; each engine's own validators are the second
stage (inside the engine, not before it).

## Gap

Falsifiable: a volume with a spoofed family OEM string but no matching
content invokes the og's analyser (returns not-recognized, engine never
runs) but DOES invoke the core's engine (which then rejects late after
loading structures — wasted I/O and different error surface).

## Impact

Efficiency and error-shape only: the core's late rejection still rejects
(no wrong results — the engines' magic gates hold). The difference is
wasted work per false dispatch and a recognition contract the parity docs
should name. MINOR. (The NTFS-specific torn-DBR consequence — a real
recovery miss — is carved out as M-043 and is NOT covered by this
finding's severity.)

## Reproduction / verification

Corpus-derived (analyser decodes cited; the contract shape is the RTTI
factory + IVolumeAnalyser interface). Core read first-hand. Evidence:
`evidence/decompiled-quotes/s5-image-fscheck-persistence.md` §C.

## Fix plan (for the engineering team)

1. Optional cheap second stage in `probe_family` per family (single bounded
   read): NTFS = first 2 MFT records (FILE magic + fixup) — M-043 step 2;
   FAT = root-dir first-sector readability (single read at the computed
   root offset); ext = already has arithmetic gates in the probe (S-006
   decode); exFAT = boot checksum existence. Log as "content-verified".
2. Document the recognition contract in the parity docs (two-stage
   recognition: boot gate → content proof → engine).

## Tests to add

`SpoofedOemRejectedBeforeEngine` (engine-invocation counter stays zero) —
one test per family added in M-043/M-044's lanes.

## Related findings

M-043 (NTFS case — the only one with a recovery miss), M-044 (UDF — the
missing probe entirely), M-045 (the FAT gates themselves).

## Open questions

The exFAT analyser's recognition body was skimmed, not pinned (corpus
next: ExFatFileSystemAnalyser_decompiled.c around its Analyze — the
result-byte write sites).

## Resolution (session 23 — the FAT + exFAT content proofs; the contract documented)

**Status:** FIXED
**Fixed in commit:** (this session's DFX29 N-050 batch — the commit carrying this file)
**Verified by:** fail-before EXECUTED on the git-checkout-reverted probe (3 new tests): `Orchestrator.SpoofedExfatOemRejectedBeforeEngine` RED (pre-fix: spoofed EXFAT OEM over a FAT volume returned Exfat — the engine WOULD dispatch) and `Orchestrator.SpoofedFatBootUnreadableRootRejected` RED (pre-fix: a FAT boot with an unreachable root still returned Fat); pass-after all 20 Orchestrator tests green, full suite **757/757**. `Orchestrator.RealExfatBootContentVerified` is the positive pin (a wire-struct-valid boot region proves the family on both the old and new probe — it pins against future over-tightening). Two pre-existing expectations CORRECTED toward og parity (both verified against the corpus decode first-hand): ProbeFamily's spoofed-EXFAT-OEM case now yields Fat (the og's multi-analyser outcome — the FAT gates + root proof win under the failed exFAT proof) and UdfNotConfusedWithFat's painted-FAT-boot UDF image now yields Unknown (no valid FAT geometry -> the og's root verification refuses; the old Fat was the spoof-accepting answer).
**Resolution notes:** the recognition contract is now two-stage on every family with a corpus-decoded analyser: NTFS (M-043, already landed), exFAT (NEW: the engine's own validate_boot_region as the cheap proof — OEM-first so a spoofed OEM fails fast in the MustBeZero sweep; the structural rc matches the engine's gate exactly, no recovery-negative strictness: the checksum verdict stays informational on both sides), FAT (NEW: root-dir first-sector readability — FAT12/16 fixed root = reserved+numFats*spf16; FAT32 = reserved+numFats*spf32+(rootCluster-2)*spc, rootCluster>=2 — the cheap form of FUN_180002180's verification, per the finding's own fix plan), ext4 (S-006's arithmetic gates — no change needed per the plan), UDF (M-044, already landed). Each proof emits `recognition: <family> content-verified` on the N-048 logging seam. The contract itself is documented as ground-truth-contracts.md §0 (boot gate -> content proof -> engine, with the per-family corpus citations and the honest HFS+/Btrfs/ZFS/ReFS recognition-gate gap). The finding's open question (the exFAT analyser's unpinned recognition body) is recorded in §0 — the conservative boot-region proof is the registered stand-in until that body is decoded.
