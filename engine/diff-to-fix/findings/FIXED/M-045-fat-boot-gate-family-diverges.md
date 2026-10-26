# MAJOR M-045 — FAT boot-validation gate family diverges: bps any-512-multiple vs pow2-512..4096, SPC unbounded vs ≤128, missing FAT32-label/numFATs/cluster-cap gates, and no band-ordered flavor retry

**Severity:** MAJOR
**Module:** fat (boot validation — lost-partition probe + engine gate)
**Parameters affected:** P1 (detection correctness — both false-accept and false-reject classes), P3 (false-positive rate), P19 (magic/signatures — the +0x52 "FAT32   " label gate), P20 (struct offsets/semantics), P25 (algorithm — band-ordered multi-try vs deterministic), P65 (test coverage of each gate)
**Corpus references:** `re-workspace/reversed/ghidra-mo/PartitionRecoveryD_decompiled.c` — `CFatxBlockParser::parse` `FUN_180006250` (:3553-3595) via classification `FUN_180004610` (:2516-2608): BitLocker pre-gate (:2604-2606); spf/totSec selection (:2527-2538); **REJECT if bps==0 or SPC==0** (:2539-2541); **REJECT if (bps & 0x1FF)!=0 — any 512-multiple accepted, including 1536/2560/8192** (:2542-2543, :2600-2602); clusterCount formula (:2543-2546); **band→validator try-order <0xFF5→[12,16,32], <0xFFF5→[16,32,12], <0xFFFFFF0→[32,16,12], ≥0xFFFFFF0→REJECT** (:2547-2598); FAT12 validator `FUN_1800048f0` (:2613-2645: numFATs≠0, not both totals 0, SPC∈{1..128} pow2, **SPF16≠0 AND ≤200**, 55AA); FAT16 `FUN_180004a40` (:2650-2680: same minus ≤200); FAT32 `FUN_180004b70` (:2685-2726: **memcmp(boot+0x52,"FAT32   ",8)==0**, numFATs≠0, total32≠0, SPC∈pow2≤128, SPF32≠0, 55AA); size sanity `FUN_1800041c0` (:2260-2310: firstDataSector<totSec else reject 0xfffffffa, wired at :3569-3573); CBlockParseWin32 registration (:646-684); S-006's scoped remainder resolved: **CFatxBlockParser is the FAT12/16/32 boot parser, NOT Xbox FATX** — "Fatx"=FAT-x, FatLib.mo `FATXDBR` is the classic BPB (FatLib_decompiled.c:594-690), and the Xbox magic 0x58544658 appears NOWHERE in the corpus (definitive negative grep)
**Core references:** audited `7cf6754` — `src/core/fat/cr_chain.cpp:261-319` `validate_boot` (lead-verified first-hand): bps must be **512-4096 AND power-of-2** (:266-268 — rejects 1536/2560/8192 the og accepts); SPC pow2 but **unbounded** (:269-271 — accepts 256+ the og rejects); **no "FAT32   "@+0x52 label check**; **no numFATs≠0 check** (fat_area=0 can pass); **no ≥0xFFFFFF0 cluster-count cap**; flavor is deterministic `flavor_from_clusters` (<4085/<65525, :12-19) — **no band-ordered multi-try**; 55AA (:262) and firstDataSector<total (:305) MATCH; probe side `src/core/lost/cr_candidates.cpp` `probe_fat` rides the same validator (per S-006/S-019 architecture)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's boot-sector classifier and the core's `validate_boot` disagree on six specific gates: the og accepts ANY 512-multiple bytes-per-sector (1536, 2560, 8192…) while the core demands a power-of-two in [512,4096]; the og caps sectors-per-cluster at 128 while the core accepts 256 and beyond; the og requires the "FAT32   " label at +0x52 for FAT32 while the core never checks it; the og rejects numFATs==0 while the core can accept it; the og rejects cluster counts ≥0xFFFFFF0 outright while the core classifies them as FAT32; and the og tries FAT flavors in band-ordered succession (12→16→32 or 16→32→12 depending on cluster count) while the core picks deterministically with no retry. Each gate cuts in a different direction — some og-accepts/core-rejects (missed lost-partition candidates), some og-rejects/core-accepts (noise candidates the og suppresses).

## What the reference does

See the citations block — the complete gate table with field offsets,
comparands, and the try-order bands is decoded in
`evidence/decompiled-quotes/s5-image-fscheck-persistence.md` §F. Note the
og's SPF≤200 FAT12 quirk and the "FAT32   " label requirement (the
FileSystemCheck CCheckFAT layer requires the same label, FSC:1440 — two
independent og sites).

## What our core does

See the core citations — the six-gate contrast table is in §F of the same
evidence file, lead-verified line-by-line this session.

## Gap

Six falsifiable gate deltas (each independently testable with a crafted
BPB): (a) bps=1536 FAT16 → og candidate, core invisible; (b) SPC=256 → og
reject, core accept; (c) FAT32 BPB without the +0x52 label → og reject,
core accept; (d) numFATs=0 → og reject, core accept; (e) clusterCount
≥0xFFFFFF0 (a spec-legal 0x0FFFFF00-cluster FAT32) → og reject, core
FAT32; (f) a BPB whose cluster count lands in a band where the first
flavor's validator fails but a later one passes (e.g. spf16=0 with
rootEntries≠0 and <0xFF5 clusters) → og tries FAT12/FAT32 after FAT16
fails, core's deterministic shape-then-count pick can refuse where a retry
would classify.

## Impact

Lost-partition discovery and volume-gate correctness: class (a) and (f)
miss real candidates the og surfaces (user-observable: a lost FAT volume
with unusual geometry is invisible); classes (b)/(c)/(d)/(e) admit noise
or spec-odd volumes the og suppresses (false-positive candidates). None
produce wrong FILE bytes — the engine's chain walk still validates — so
MAJOR, not SEVERE: this is the same surface class as S-006/S-019 (probe
gates), but per-gate edge cases rather than whole missing families.

## Reproduction / verification

Corpus-derived (the full validator chain re-read first-hand; the negative
Xbox-magic grep is definitive). Core-side `validate_boot` read first-hand
this session. All six gates are Linux-executable as unit fixtures (craft
512-byte BPBs). Evidence:
`evidence/decompiled-quotes/s5-image-fscheck-persistence.md` §F.

## Fix plan (for the engineering team)

1. Align `validate_boot` (cr_chain.cpp:261-319) gate-by-gate: (a) accept
   any 512-multiple bps in the ENGINE's parse path only if downstream
   arithmetic is bps-generic (it is — all math is bps-multiplied); keep a
   sanity ceiling (og has none; recommend 32768 to avoid absurd volumes —
   document as a deliberate bound); (b) cap SPC at 128 (og gate; also
   guards cluster_bytes overflow); (c) require "FAT32   "@+0x52 when the
   flavor resolves FAT32 — make it a SOFT gate first (log + grade-down, not
   reject) pending a real-volume fixture sweep, because some format tools
   write nonstandard labels — NOTE: the og enforces it hard at TWO sites
   (classifier + CCheckFAT); decide with fixtures; (d) reject numFATs==0
   (hard — a FAT with zero FATs is unparseable; today's fat_area=0 path is
   a latent bug); (e) reject clusterCount ≥0xFFFFFF0 (og gate; spec max is
   0x0FFFFFF6 so this only cuts spec-overflow noise); (f) add band-ordered
   flavor retry mirroring the og's try-order (a 12-line loop over the
   band's validator list).
2. Probe parity rides for free: `probe_fat` routes through `validate_boot`
   (S-006 architecture).
3. Tests: `FatBps1536Accepted` (og parity), `FatSpc256Rejected`,
   `Fat32LabelMissingRejected` (or soft-flagged per decision),
   `FatZeroNumFatsRejected`, `FatClusterCountOverflowRejected`,
   `FatBandRetryClassifiesSecondFlavor`, `FatFirstDataSanityHolds`.

## Tests to add

The seven named tests; (a)/(d)/(f) are the fail-before proofs (opposite
directions).

## Related findings

S-006 FIXED (the probe family this gates; its FATX scoped remainder is
RESOLVED by this finding — no Xbox parser exists, folded here), S-019
FIXED (BitLocker pre-gate already parity), N-034 (FAT constants tracked),
M-015 (FAT load validation — engine-side layer of the same family),
M-044 (UDF probe rides the same boot sector before the FAT catch-all).

## Open questions

Whether the og's ENGINE (as opposed to the lost-scan classifier) also
accepts non-pow2 bps — the FATFileSystemAnalyser's validators
(FFA:429-504) check SPC bitmasks but the bps gate there was not fully
decoded; the fix leg should pin FFA's bps check before relaxing the
engine gate (fixture evidence preferred).

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's M-045 commit — see `git log` for "fix: M-045")
**Verified by:** `FatEngine.FatBps1536Accepted`, `FatEngine.FatZeroNumFatsRejected`,
`FatEngine.FatClusterCountOverflowRejected` (all three fail-before PROVEN by
temporarily restoring the old gates and rebuilding: 3/3 FAILED; fix restored →
green), plus `FatEngine.FatBps32768CeilingAndNonMultipleRejected` (the documented
bound pin). Full suite 588/588 Linux Release.
**Resolution notes:** verified-first-hand against the corpus this session
(PartitionRecoveryD FUN_004610 :2516-2608 classifier + FUN_0048f0/4a40/4b70
validators, all re-read in the decompile). IMPORTANT context: the finding's six
gates were substantially absorbed by the parallel N-034/N-010b landings BEFORE
this session — `lost_boot_flavor` (cr_chain.cpp) already implements the og
classifier EXACTLY (band-ordered retry <0xFF5→[12,16,32], <0xFFF5→[16,32,12],
<0xFFFFFF0→[32,16,12]; any-512-multiple bps; the "FAT32   "@+0x52 label;
numFATs≠0; SPF16≤200 FAT12 quirk; cluster-cap; 55AA) and `boot_passes_open_gates`
already hard-requires the FAT16/FAT32 labels at engine open (FUN_18002f970
decode); the SPC-set pin and label tests exist (SpcDomainMatchesOgSet,
BootStringGate*). This session closed the three REMAINING validate_boot
residuals the finding's falsifiable claims still hit end-to-end (probe_fat runs
validate_boot as its FIRST gate, so these three still made og-accepted
candidates invisible): (a) bps is now ANY 512-multiple ((bps & 0x1FF) == 0, the
og :2542 rule) with the documented deliberate 32768 ceiling the og lacks
(PARITY-DELTAS row; downstream arithmetic is bps-multiplied so non-pow2 sizes
are safe — the ceiling only cuts absurd-volume noise); (d) numFATs == 0 is a
hard reject (the old fat_area=0 latent path); (e) cluster_count ≥ 0xFFFFFF0 is
rejected (the og classifier's final-band else-branch; spec max 0x0FFFFFF6).
Gate (b) SPC-256 was a documented false positive (u8 unrepresentable —
SpcDomainMatchesOgSet walks the whole domain); gate (c) is hard at BOTH og
sites and both core layers; gate (f) is live in lost_boot_flavor. The finding's
open question (FFA's engine-side bps gate undecoded) stands as recorded; the
classifier-parity rule was chosen for validate_boot to restore probe coherence
(a candidate lost_boot_flavor accepts must survive validate_boot).
