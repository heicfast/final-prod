# SUMMARY.md — executive summary (diff-fix session 22 on top; earlier sessions below)

## Session-22 delta (2026-10-26 — DFX22: win-layer build-integrity M-055 + the MINOR-first queue)

Session lane re-stamped 19→22 (IDs 19/20/21 squatted by parallel legs in
the same window). Bootstrap: pulled the parallel wave (`e21de3e`→`93d7fa6`:
S-029 + S-030 landings close the SEVERE tier, M-005/M-008/M-016/M-032/
M-043/N-041/N-016a, D-70/D-71); baseline suite **629/629** Linux Release
green with this session's additions; corpus `03ad46c` unmoved.

**M-055 NEW-FINDING (MAJOR) filed + FIXED (first landing):** the
Windows-only platform layer has been UNCOMPILABLE since 2026-10-25 21:07 —
four independent structural breaks from three concurrent parity-fix
landings that no prior session executed: (1) `915bce3` (N-015) nested
`WinDestIo::free_bytes` inside `write()`'s body (missing closing brace);
(2) `3b5397a` (N-013) defined `volume_extents`/`destination_volume_letter`
AFTER the namespace closers (unqualified `VolExtent` error + the
header-declared functions left undefined — the claimed destination-arming
could never link); (3)+(4) `d8f6808` (N-041) left a stray `} // namespace
rda` closer that closes `namespace win` mid-file AND deleted
`RdaTargetDevice::heal_mu_` while keeping `read_at`'s heal lock on it.
The windows Actions job's build-step failures over this window were
visible and unaddressed. Fixed all four; landed the permanent gate:
`tools/win_syntax_check.sh` + `tests/win-stub/` (four declaration-only
SDK-faithful stub headers) wired as ctest `win_syntax_check` on the Linux
gcc lane — the fail-before/pass-after run (exit 1 pre-fix with the exact
diagnostics, exit 0 post-fix, all seven win-layer files parsing incl. the
S-030 `cr_image_win.cpp`) is the finding's proof. Suite **629/629**.
The N-056 queue proceeds on top of this (same file, its fix plan was
unexecutable while the layer didn't parse).

## Session-18 delta (absorbed by the DFX22 merge — parallel lane, verbatim) (2026-10-26 — DFX18: the minors-first wave, lane-disjoint from DFX17)

Session bootstrap at `eeeccbd` lineage (prompt push `e21de3e`, lane claim
`e61c2ff`); corpus `03ad46c` unmoved. Baseline before any change:
linux-release suite **592/592** green at the claim tree (the 584-at-clone
baseline absorbed the parallel landings M-007/M-054/M-045/M-046/M-037/
M-031/N-030 after the rebase). Mandate: minor findings first, 20+ fixes,
fixed findings moved to findings/FIXED/, all docs/counts current. Lane
claim posted in the worklog BEFORE any finding work: the DISJOINT
remainder of the MINOR tier (DFX17 owns N-067/N-068/N-063/N-065/N-056/
N-029 + group files).

**N-045 FIXED (registered deliberate divergence — the finding's own fix
plan):** the 8 MiB / 256 KiB reactive-LRU scan cache vs the og's 4 MiB
prefetch read-ahead `CStorageDeviceWithCache` (DRWUI_decompiled.c
:919080-:919092 `0x400000/bytes_per_sector` + 0x2000-block fallback;
ctor :962250-:962597 — re-verified first-hand this session, both the
budget cite and the session-6 prefetch-engine addendum). Registered as
PARITY-DELTAS **D-72** (budget + algorithm, benchmark-gated change
policy); divergence block in `include/cr/storage/cr_cached.h`;
construction-site cite in cr_scan.cpp; docs/benchmarking.md §4 intent
note. Doc-truth pin `Cached.ScanCacheBudgetIsTheRegisteredDivergence`
via the new `CR_SOURCE_DIR` test define (tests/CMakeLists.txt): reads
the source constant, the D-72 row, and the header block at runtime —
fail-before executed RED on the stash-isolated pre-fix tree, pass-after
green; suite 592/592. Finding → FIXED/; RESOLVED-INDEX row; matrix
storage P47/P48 cell → MATCH(registered); storage.md verdict updated.

**Counts after this landing (direct file inventory at the N-045 push,
pre-rebase tree):** SEVERE 1 (S-029) · MAJOR 32 · MINOR 40 · INFO 22
(0 actionable) · tracked-open 16 · FIXED 96 files (95 findings +
RESOLVED-INDEX). (Post-rebase note: the parallel session-10 close-out
below landed S-029 + M-043 + M-016 + N-016a in the same window — the
live inventory at this commit is lower; see the session-10 section.)


## Session-10 delta — the SEVERE tier close-out (2026-10-26)

This lane (DFX10) executed the published order S-009 -> S-029 -> S-030
against a hot remote (three independent S-009 implementations converged;
S-030 landed from the DFX8 lane mid-session):

**S-009 — converged + hardened:** this lane's independent fix rebase-
absorbed into the landed storage::FreeMap implementation (c55bd45);
first-hand verification of THEIR code found and closed four defects (FAT
+ exFAT virtual-cluster false-free over the root-dir/boot tail; exFAT
2-cluster under-coverage; the torn-bitmap read painting zeros free) plus
the found-content rule convergence (deleted/carved content cells stay
Unknown — the finding's plan verbatim). 4 pin tests; suite 577/577.

**S-030 — landed by the parallel lane (b30354d):** flat/VMDK/VHD image
devices + --image CLI (re-verified green in this lane's baselines).

**S-029 — CLOSED (this lane):** the lost-partition apply path, the
recovery endgame. New consent-gated cr/apply module on the rawfixer
seams: plan_apply (read-only, containment-refusing) + execute_apply
(A1 consent callback — denied OR absent = ERR_APPLY_CONSENT_DENIED with
zero writes; A2 stale-table revalidation; quiesce first; A4 transaction
with read-back verify + rollback) + the shared CRUNDO01 undo. CLI
`rrecov apply --disk N --session F --candidate N [--apply]`; win layer
`refresh_disk_properties` (the og's UPDATE_PROPERTIES + settle). 10
Apply.* tests; suite **590/590** Linux ASan. PARITY-DELTAS D-68/D-69.

**The SEVERE tier is CLOSED** (0 open). Live inventory at this push:
MAJOR 33 · MINOR 44 · INFO 22 · FIXED 95 · tracked-open 16. Active lane
claims respected: DFX15 (MAJOR head), DFX9-T7 (M-052/M-035/M-018/
M-013/M-014), DFX14 (M-005).

# SUMMARY.md — executive summary (diff-fix session 19 on top; earlier sessions below)

# SUMMARY.md — executive summary (diff-fix session 15 on top; earlier sessions below)

## Session-19 delta (2026-10-26 — DFX19: MAJOR-first lane, first landing M-016)

Bootstrap `ff734d7` (union of the parallel lanes' claims: DFX18 on the MINOR
tier, this leg on the unclaimed MAJOR batch); baseline suite **594/594**
Linux RelWithDebInfo + ASan. Corpus `03ad46c` unmoved.

**M-019 FIXED (second landing):** ZFS multi-vdev + integrity — read_raw
drops the `vdev != 0` refusal (the og's registry first-entry fallback,
FUN_1800168b0 :19544-56 — foreign reads now served and counted);
read_block gains the og's exact fletcher4 gate (`(zc==7)||(zc==0)`,
interleaved 4-accumulator port) over the RAW bytes with the 32-byte
stored compare and next-DVA retry on mismatch and read failure; the L0
extent walk plans foreign-vdev blocks at the fallback position instead
of skipping them. Fixtures upgraded to checksum-carrying pools (type-7
fletcher4 stamped in dependency order; the fixture uses an independent
textbook encoding so engine and fixture cross-check). Fail-before
proven both claims: the torn-DVA0 decoy zap entered results as valid
(now dropped + DVA1 retry recovers the real entries); the foreign-vdev
file was left extentless (now planned at the fallback position). Suite
614/614. findings/FIXED/M-019; matrix zfs B1/B2/A25/D15 MATCH.

**M-016 FIXED (first landing):** LDM TOCBLOCK candidate set/order — the
core's list was `{+1,+2,+2045,+2046}` (config+0 never probed; +1 outranked
+2 on conflicts) while the og probes `{+2,+0,+1}` first-valid-wins
(LdmManager FUN_180072ad0, re-verified first-hand incl. the hard-fail
name-check tail). Now `{2,0,1,2045,2046}` — og probe order first, kernel
tail-copy superset behind. Fail-before proven both claims: a config+0-only
disk went ERR_BAD_CHECKSUM (all volumes missed) → full parse; a +1/+2
conflict followed the stale +1 VMDB (committed_seq 42) → now the +2 anchor
(99). The og's stop-at-magic hard-fail on corrupt bitmap names stays
un-adopted (forgiving per-copy walk = registered recovery superset). 3
tests; suite 594/594; findings/FIXED/M-016; matrix table+ldm B1/C20 MATCH.

## Session-15 delta (2026-10-26 — DFX10: the S-009 third-implementation convergence + M-054) (2026-10-26 — DFX10: the S-009 third-implementation convergence + M-054)

Session bootstrap `7c44bdc` (baseline 533/533 Release verified first-hand);
mid-flight the remote advanced to `d487928` (the parallel lanes' S-009
free-overlay landing via `storage::FreeMap`, S-030 disk-image targets, M-002,
M-051, M-037, N-073, N-042/N-044 fixes) — absorbed by rebase; the union rules
applied (the landed implementation is the base). Corpus `03ad46c` unmoved.

**S-009 (convergence record):** this session had built a THIRD independent
implementation (`cr::AllocClusterMap` — used-bits, slice-relative,
byte_origin, fail-open truncation, RLE persistence; 5-test suite +
behavioral fail-before proofs) when the parallel lane's design landed first.
The landed `storage::FreeMap` stands; the third design is preserved on
branch `s009-allocmap-impl` (`fb2bb91`) for the record and dropped from the
tree. S-009's own Resolution (the landed one) remains operative.

**M-054 (NEW-FINDING, MAJOR, filed + FIXED this push):** discovered during
the third-implementation's exFAT producer verification — the engine's
allocation-bitmap `Bitmap::test` read bit `c` for cluster `c`, but the MS
exFAT spec §7.1 pins bit `i` = cluster `i+2` (the first cluster has index 2).
TWO consumers inherited the shift: deleted-viability grading (wrong
confidence grades on every real volume) AND the just-landed S-009 free-map
producer (free overlay painted cluster `c` per cluster `c+2`'s state). The
core's own fixtures encoded the same shift, so the suite was
self-consistently wrong. Fixed: `Bitmap::test` reads bit `c-2` (clusters 0/1
reserved-never-heap); both fixtures write the spec-correct bit; grading
expectations unchanged (the intended behavior); NEW pin test
`ExfatEngine.FreeMapProducerMatchesSpecBitMapping` guards the producer seam.
**Fail-before EXECUTED on the landed base**: fixtures corrected + engine
body reverted → `ExfatEngine.EndToEnd` red (over.dat Excellent instead of
Overwritten) AND the new producer pin red; fixed → green. Suite after:
**574/574 Release + 574/574 ASan + 574/574 UBSan** (three lanes, executed).

Also this push: a committed conflict-marker remnant repaired in worklog.md
(the trailing `>>>>>>> 33c0722` at :9187 — the zero-markers standard).

**Counts after this push (direct file inventory):** SEVERE **1** (S-029 —
S-030 landed by the parallel lane) · MAJOR 34 · MINOR 48 · INFO 22 (all
ACKNOWLEDGED) · FIXED/ family **87 files** (incl. RESOLVED-INDEX; +M-054
this push; S-009/S-030/M-002/M-051/M-037/N-073 by the parallel lanes).
Top remaining risks: S-029 (the apply-path scope decision — the last
SEVERE), then the M-tier by user impact (M-005 the carve gate next — same
bitmap plumbing, one consumer).
# SUMMARY.md — executive summary (session 11 DFX11 on top; earlier sessions below)

## Session-11 delta (DFX11, this leg — S-009 verified + M-007 landed)

Session arc: bootstrap 7c44bdc (baseline 534/534) -> the S-009 convergence
(this leg implemented the full free-overlay stack independently; the
parallel DFX8-legacy lane landed first at c55bd45/a3e5fd8 — first-landing
wins, this leg's implementation dropped cleanly and the landing verified
first-hand at 575/575 with two corpus arbitrations recorded in the worklog:
the FAT free-map is og-faithful per searchdll-family :98 "volume bitmap
from FS engine"; the bitmap-truth free rule is closer to the og than the
finding's plan text) -> M-007.

**M-007 FIXED (this push):** hard-link emission parity.
`WalkerFlags::single_link_per_record` (default TRUE = the og's model: ONE
node per record - the namespace-best $FILE_NAME from the FULL list,
first-seen wins ties, under ITS parent; corpus FUN_180006db0 :5273 +
FUN_1800448b0 :54124-54162 + FUN_180043330). FALSE = the registered
opt-in fan-out (per-parent nodes, per-link names; PARITY-DELTAS D-65);
the hardlinks stat counts only in fan-out mode. The fan-out path's
per-parent name selection was REPAIRED in the same change (per-parent
names from that parent's OWN entries - a pre-existing divergence-class
bug no earlier fixture exposed: on mixed-rank multi-parent records every
parent inherited the highest-rank sibling's name). 4 tests (2 new +
EndToEndWalk/AdsAttachesToFirstLink rewritten for both modes); fail-before:
walker-revert (3 nodes vs 1; the Win32/DOS links unfindable). Suite
582/582. Counts after this push: SEVERE 2 (S-029, S-030) · MAJOR 36 ·
MINOR 48 · INFO 22 (0 actionable) · tracked-open 16 · FIXED 83 files
(82 findings + RESOLVED-INDEX).

**M-008 FIXED (this push — premise corrected against the corpus):**
`WalkerFlags::single_hop_attrlist` (default FALSE = the multi-hop BFS
superset, D-73, per the finding's own recommendation); TRUE = og parity
(depth-1; corpus FUN_180041df0 :52615-52687). The finding's "deleted
records never chased" premise was STALE — it read FUN_18001acc0 (the
GUID-DUMP pass 2, S-012's decode) as a deleted-phase walk; pass 1
(FUN_180017390 :18423+) emits every record AND builds the child list
(:18461-18464, plVar9 non-NULL), so the og chases deleted records at
depth 1 too — the fix-plan's deleted-gate step was REJECTED (corpus
wins) and OQ-S2A-1 resolved by the same evidence. 3 tests (2 new +
NestedAttrlistChase as the default guard); fail-before: walker-revert
(2 vs 1 extents). Cross-lane repair in the same push: the M-054
landing's +2 coverage hardening missed the S-009 leg's
FreeMapProducerMatchesSpecBitMapping expectation (100 -> 102). Suite
593/593. Counts after this push: SEVERE 2 · MAJOR 34 · MINOR 48 ·
INFO 22 · FIXED 84 files (83 findings + index).

**M-032 FIXED (this push — the fix plan's own decision path 1):** slice
tail-read semantics. The decision gate executed first: the engine audit
found no reliance on the og's clamp+zero-fill contract (HFS+ journal ring
reads bounded within the ring; all other reads explicitly bounded;
refusals degrade via damage counters). The safer refusal stays; the
divergence registered D-74; pinned by Slice.TailReadRefusalPinned + the
updated BoundsEnforcement (corpus FUN_180009290 :7219-7264; the og's
sector-unit under-zeroing defect noted, NOT adopted). No behavior change
- the opt-in clamped read from the finding's path (2) is the documented
next step WHEN an engine needs it. Suite 594/594 (Slice +1).

**M-050 + N-058 FIXED (this push — one loader change, per M-050's plan):**
the carrier audit re-scoped post S-023/M-051/M-047: grade+overlap,
provenance, root stats, short names ALREADY carried (the predicted schema
debt was paid by those landings) — now PINNED by
Session.GradeRoleAndProvenanceRoundTrip + OldFormatStillLoads; roles
persist as strings (richer than the og's u32 pair). Deliberate
NOT-PERSISTED, OQ-tracked: hidden-root bit, f_6c/f_68, f_71 (undecoded —
carrying opaque u16s would invent structure). N-058's four loader
hardenings in the same change: forward parents DEFERRED + resolved
post-loop (reparented count surfaced via the new optional out-param);
marker outside {0,1} fails the record; c.fail alone fails the load;
name/role lengths bounds-checked BEFORE allocation. 6 tests; suite
602/602. Counts: SEVERE 2 · MAJOR 32 · MINOR 46 · INFO 22 · FIXED 86
files (85 findings + index).

**M-029 (partial — three sub-items closed, the finding stays ACTIVE):**
Type sort CLOSED (SortKey::Type, ABI code 7 append-only — iconIndex
category order then extension then name; pinned with the full 8-row
sequence); the modified-unit doc lie FIXED (FILETIME 100-ns since 1601 —
the old "Unix seconds" line was false; pinned by
Query.ModifiedUnitIsFiletimeEverywhere; the stale Category enum row
fixed too); paging DOCUMENTED as the deliberate difference (no og
page-size constant — UI-layer virtualization). Remaining (decode-blocked,
next steps recorded in the finding): the bucket-ID translation (the og's
buckets select by node+0x89 date TAGS, not ranges) + the general-raw
naming bucket. Suite 636/636.

**M-049 (partial):** the header's false parity claim REWRITTEN to the
value-space truth (family-prefix convention only, D-68; the og has only
two corpus-documented 12-digit codes — renumbering to them would break 26
others arbitrarily); err_text tested for the first time (18 codes,
distinct texts, fallback, OK=0). The engine-side error-context subset
stays open (design-gated; next step in the finding). Suite 643/643.

# SUMMARY.md — executive summary (diff-fix session 14 on top; earlier sessions below)
## Session-14 delta (2026-10-26 — DFX14: bootstrap, two merge repairs, the reconciliation wave)

Session bootstrap `7c44bdc` (baseline **533/533** Linux Release, no flakes);
corpus `03ad46c` unmoved. Two merges absorbed the parallel legs' landings
(S-009 free-overlay by the DFX8-legacy lane c55bd45/74e4d36; N-020 doc drift
+ the DFX12 bookkeeping wave; N-042/N-044 + the DFX11 hygiene wave). The
second merge caught and repaired a disguised rollback: **5aaf044
("style(lint)") was a four-finding semantic clobber** — S-009's cr_ui.cpp
snapshot capture, N-042's decidegrees print, N-044's --whole-disk rename, and
M-012's cr_chain mirror scrutiny, 83 lines deleted under a formatting label.
Restored from the verified-green f15c286 state in the same merge (c4e37d1);
suite 547/547 then 553/553 proved it. Protocol lesson recorded: never absorb
a "style" commit without diffing it.

**N-073 FIXED-ALREADY (this push):** the ext4 CatchBuffer engine-path residual
— N-004's transport-layer landing (`7f03c4b`) covers it by composition: every
orchestrator-driven engine read goes through the retry pipe (cr_scan.cpp:714/
723 pass `pipe` into run_quick/run_deep). Proven with the three engine-path
tests this finding's own plan named, via an ExactFaultDevice alignment-fault
model on the production pipe shape: `Ext4Engine.CatchBufferBacktrackSalvage`
(tree intact, salvaged range never recorded bad), `Ext4Engine.BacktrackOff-
ZeroFillsTheWindow` (the policy flag reproduces the pre-N-004 world — the
finding's exact claim), `Ext4Engine.ZeroFillOnlyWhenBacktrackAlsoFails` (only
the truly unreadable half zeroed; error map exactly [5632,512)). Suite
**553/553**. Finding → FIXED/; RESOLVED-INDEX row; matrix ext4 row; ext4.md
S35 + both index rows (incl. the N-057→N-073 renumbering correction).

**M-029 duplicate reconciled (this push):** the merge-era fragment
`M-029-query-model-gaps-sort-paging-buckets-server-push.md` (a stray session-5
addendum under a duplicate ID — no title, no finding header) folded verbatim
into the canonical M-029 file and removed. The canonical remains ACTIVE (Type
sort key, bucket-ID translation, paging parity claim, server-side push).

**M-005 FIXED (this push):** the carve allocation-bitmap gate. Corpus
re-verified first-hand before implementing (RAWSearchDLL :9440-9466 +
FUN_180058df0 :67842-67856) — three details richer than the finding's
paraphrase, all folded in: OOB/no-bitmap dispatch proceeds (raw-disk rule);
the cluster jump requires cluster alignment; the og's bitmap is the USED
map so the core's inverted FreeMap flips the gate's polarity. The gate
lives in BOTH dispatch layers (grammar per-byte + raw-sign 512-stride);
the map enters via CarveFlags.free_map with the orchestrator rebasing
stats.free_map (S-009's producer surface) slice-relative at the call site.
Tests: Carve.BitmapSkipsUsedClusters + Carve.NoBitmapCarvesEverything
(null AND invalid maps carve all). Bench (scripts/bench_m005.cpp, 16 MiB
LCG-random, 50% used, median of 3, two invocations): 1137.8/1124.7 ms
ungated vs 606.3/594.4 ms gated = **1.88x/1.89x** (plan expected >= 1.5x).
Bench lesson recorded: all-0xFF fixtures are adversarial (FMT_MPGA_FF's
{0xFF,0xFF} magic — parity with the og's FF sync family); representative
mixed-content fill is the honest measurement. Suite 575/575.

**M-043 FIXED, one sub-claim DISPROVEN (this push):** NTFS recognition
content proof. Corpus re-verified FIRST — and it overturned the finding's
first half: FUN_180005470 decoded in full shows the analyser gates on the
PRIMARY boot and bails before result 4; its "backup-boot verification"
(:4309-4327) is a memcmp storing the verified location at +0x510 as the
REPAIR input — and NTFSSearchDLL carries ZERO backup reads. The og has NO
scan-path backup fallback; the core's refuse-then-rawfixer shape was
already parity on that half (no fallback code added — the finding's text
is corrected, not "fixed" by breaking parity). The CONFIRMED half landed
at the exact og boundary: probe_family's NTFS branch now applies the
og boot gates (bps 512-multiple <= 0x1000; SPC pow2 — NFA :4292-4307) +
record-0 proof (FILE magic; USA count [1,0x11); USA offset [0x2a,0x201) —
FUN_180005180 :4038-4058), one 512-byte read at $MFT LCN. Executed
fail-before: ProbeFamily's spoofed-OEM leg (a FAT volume with a spoofed
"NTFS    " OEM used to dispatch the NTFS engine — the existing test
ASSERTED the divergence). New: NtfsProbeContentProof (5 legs) +
TornPrimaryBootWithIntactBackupStillRefusedUntilRepair pin. Suite 599/599.

**Counts after this wave (direct file inventory):** SEVERE 2 (S-029, S-030) ·
MAJOR 35 · MINOR 48 · INFO 22 (0 actionable) · tracked-open 16 · FIXED 84
files (83 findings + RESOLVED-INDEX).

# SUMMARY.md — executive summary (diff-fix session 11 on top; earlier sessions below)

## Session-11 delta (2026-10-26 — the MINOR-first close-out wave)

Session start `a5bbcfa` (DFX7-T2 inventory head; S-032/N-018/N-042 legs
already landed by the parallel sessions); corpus `03ad46c` unmoved.
Baseline before any change: linux-gcc (RelWithDebInfo + ASan) **534/534**
green. Mandate: minor findings first, 20+ fixes, fixed findings moved to
findings/FIXED/, all docs/counts current.

**Audit-trail hygiene wave 1 (this entry):** four fully-resolved MINOR
findings moved from MINOR/ to FIXED/ with RESOLVED-INDEX rows — **N-013**
(all-extent destination arming; image→same-volume sub-claim REFUTED with
corpus evidence, `CrUi.ImageSourceDestinationNearImageAccepted` pins),
**N-015** (`ERR_DEST_FULL` end-to-end), **N-018** (UBSan lane, `b1f9210`),
**N-042** (one decidegree source, `3678cbf`). Matrix cells
(writer+report ×2, health ×2, quality) + modules/writer.md + modules/health.md
verdicts updated; a pre-existing joined-row corruption in the matrix
session-5 section (carve ‖ lost-partition fused into one line) repaired.
**Convergence note (rebase):** the parallel DFX12 lane landed an overlapping
reconciliation (`f4fe677`) in the same window — moving N-004/N-005/N-006
(now all three FIXED — the group file restored from the b2bd91b truncation)
and **N-044** (`779eb21`: `--deep` renamed `--whole-disk`, deprecated alias
kept) in addition to the same four. Union kept both records; the duplicate
RESOLVED-INDEX rows were reconciled to one set in the same push. The group
files **N-009 and N-036 stay in MINOR/** until their open remainders
(N-009c CLI --filter UTF-8, N-009d ACL preservation; N-036 container-map
half) close.

**Counts at session-11 start (direct file inventory at `a5bbcfa`, after
the hygiene move):** SEVERE 3 (S-009, S-029, S-030) · MAJOR 39 ·
MINOR 51 (44 fully-unresolved files + N-004/N-006 + N-009 + N-036 groups
with open remainders) · INFO 22 (all ACKNOWLEDGED) · FIXED 76 finding
files + RESOLVED-INDEX · tracked-open 16. Subsequent session-11 landings
append their deltas below as they push.

**Session-11 landing 1 — N-069 FIXED:** CAB lowercase-key parity. The og's
registry carries duplicate keys for case-insensitive formats ("MSCF"/"mscf"
— census :94, "deliberate registry bloat for correctness" :127); the core
keyed uppercase only. `FMT_CAB_LC` appended (serialized ids stable) with a
registry row sharing `validate_cab`; the fail-before run exposed a second
site the fix plan missed — `validate_cab`'s internal uppercase-only magic
re-check — now accepting exactly the two registered spellings.
`Carve.CabLowercaseMagic` (fail-before: lowercase cabinet never probed;
pass-after: both cases carve, lowercase node carries FMT_CAB_LC). Suite
568/568 ASan; lint clean. Post-fix counts: MINOR 48 · FIXED 80 findings.

# SUMMARY.md — executive summary (diff-fix session 10 on top; earlier sessions below)

## Session-10 delta (2026-10-26 — DFX10: the S-030 convergence push)

Session start `5ff4db7` (post-T1 bootstrap on `8e79960`-lineage); corpus
`03ad46c` unmoved. Baseline 533/533 Linux Release re-verified at start.
CONVERGENCE NOTE: a parallel session-8 lane landed S-030's first cut
(flat/VMDK/VHD, `b30354d`) mid-flight; this session had independently
implemented the full surface — the union keeps the first landing's audit
record and lands this lane's superset engine (VHDX + log overlay, the
differencing families, .dmg flat-parity correction, the qemu-fixture
compatibility proof layer, image_path widened 256→512). The finding file
carries both Resolutions.

**S-030 FIXED (converged):** the disk-image scan-target surface — the og's
"Import a disk image" input class (extension factory DRWUI:948237-948409,
flat fallback :948389-948393, LBA0 classify :275877-880; Device.mo's
libvmdk block device :49106-49124 with extent files + parent chains
:48965-49100). Pure-core `cr::storage::open_image` behind an injected
`ImageFileIo` seam: flat raw (.img/.dmg/.raw — .dmg opened raw, exactly the
og's fallback, NOT Apple-UDIF parsing), VMDK (monolithicSparse +
descriptor extents FLAT/SPARSE/ZERO, grain directory/table translation,
differencing via parentFileNameHint, depth cap), VHD (fixed + dynamic +
differencing: footer/dynamic-header one's-complement checksums, BAT,
512-byte sector bitmaps, W2ku/W2ru parent locators), VHDX dynamic (header
pair CRC-32C + highest-sequence, region table, metadata, chunk-ratio BAT)
with a READ-ONLY in-memory log replay overlay (qemu vhdx-log.c walk
semantics; D-64d — a recovery tool never rewrites its input). CLI
`scan --image` / `recover --image`, identity-card v3 tail `image_path` +
resume warning, repair-on-image stays analysis-only. Registered
non-adoptions D-64a/b/c: .edisk proprietary, compressed VMDK/VMFS, VHDX
differencing — all refused with precise errors, never silent mis-reads.
**Proof discipline worth recording:** every format constant was pinned
empirically on qemu-img-10.0.13-written files before coding; the committed
compat fixtures (tests/data/images) caught two transposition bugs
(VHDX log-descriptor field order; metadata-region GUID data3) that
in-memory builders — written from the same misread spec-memory — could
never catch. 20 new tests; suite **555/555** Linux Release. New module
row: modules/imaging.md.

**Counts after S-030 (direct file inventory at this push):** SEVERE 2
(S-009, S-029) · MAJOR 39 · MINOR 55 · INFO 22 = 118 active / 73 FIXED
finding files + RESOLVED-INDEX.

**Session-11 landing 2 — S-033 (NEW-FINDING) INVALID + N-067 FIXED:** during
the N-067 verification pass this leg filed S-033 (exFAT type-constants
"shifted") and REFUTED IT THE SAME SESSION with the og's own decompile walk
(:24380-24460): case 0x81 = the bitmap-shape validation (flags@+1 free,
reserved@+2..+0x13 zero, cluster@+0x14/length@+0x18); case 0x82 = the
upcase-shape validation (reserved1@+1..+3, checksum@+4 free, reserved2
@+8..+0x13). Bitmap=0x81/upcase=0x82/label=0x83 — the core's constants
were RIGHT all along; the local branch was reverted pre-push; corpus erratum
E43 filed (the v2 artifact's garbled §3 row labels). N-067 then landed on
the verified constants: the upcase volume-integrity check (og shape gates +
chain-read checksum32 vs TableChecksum, soft `ExfatScanStats` signal,
grade-not-reject) with a real 64-entry fixture table at cluster 60.
`UpcaseTableChecksumValidated` + `UpcaseLengthSanity` (fail-before: the
assertions were RED with the validation unwired). Suite 597/597 ASan;
42/42 UBSan on the touched paths; lint clean. Counts: MINOR 47 · SEVERE 1
(S-029) · FIXED 82 (incl. S-033 INVALID record).

# SUMMARY.md — executive summary (diff-fix session 9 on top; earlier sessions below)

## Session-9 delta (2026-10-26 — baseline repair + the SEVERE/MAJOR/MINOR fix waves)

Session start `c231f2f` (audit-trail lineage; the S-021 journal-gate
re-append head); corpus `03ad46c` unmoved. Baseline pull found Linux CI RED:
`cr_ui_abi_check` SEGV — root-caused first, before any finding work (a
session does not fix findings on a red baseline).

**NEW-FINDING M-053 (MAJOR) filed — FIXED-ALREADY (rebase union):** the
M-015 landing (`6dc1e32`) updated the two C++ FAT16 fixture builders with
the canonical media pair but missed the third —
`tests/ui_abi_check.c:build_fat16()` (the ported C-consumer twin). From
`6dc1e32` the plain-C ABI proof was dead (scan `ERR_BAD_MAGIC` on the
invalid fixture → NULL rows → SEGV at :576) and the Linux CI gate red on
every push in the window. This session root-caused it independently
(worktree bisect: parent `63d9067` green / landing SEGV; standalone repro
of the full failure chain; own fixture repair 518/518 green) and filed the
finding per the discovered-divergence SOP — while the parallel DFX7 lane
landed the byte-equivalent fixture repair inside `b1f9210` (N-018's UBSan
defect set) WITHOUT filing the regression. The rebase union kept their
bytes; M-053 is the formal record of the red window, classified
FIXED-ALREADY with both proofs. Evidence:
`evidence/executions/s9-m053-abi-fixture-regression.md`.

**Counts at session-9 start (direct file inventory at `c231f2f`):**
SEVERE 5 (S-009 residual, S-029, S-030, S-031 stub — timestamp residual,
S-032) · MAJOR 40 · MINOR 55 · INFO 22 = 122 active; findings/FIXED/ 67
finding files + RESOLVED-INDEX; tracked-open 16 files. After the M-053
filing+fix: active count unchanged (122 — the finding was born fixed in the
same push); FIXED 68 finding files. Subsequent session-9 landings append
their deltas below as they push.

# SUMMARY.md — executive summary (session 12 on top of session 9; earlier sessions below)

## Session-12 delta (2026-10-26 — the minors-first reconciliation wave, DFX12)

Session start `7c44bdc` (bootstrapped at `7444fd6` after rebase onto the
parallel lanes' N-044/UBSan-CI landings); corpus `03ad46c` unmoved.
Baseline before any change: Linux Release suite **533/533** green.
Order per user mandate: MINOR-first waves; every finding independently
re-verified before fixing; fixed findings move to `findings/FIXED/`.

**Audit-trail reconciliation (this wave):** six MINOR files whose fixes
landed in prior sessions (commit evidence verified first-hand: `7f03c4b`,
`915bce3`, `b1f9210`, `3678cbf`, `779eb21`) had never been moved out of
MINOR/ — N-004/N-005/N-006 (group), N-013, N-015, N-018, N-042, N-044
now live in FIXED/ with RESOLVED-INDEX rows. **Audit-trail damage
repaired:** the b2bd91b "housekeeping" commit had accidentally truncated
the N-004/N-005/N-006 group file from 99 lines to the 7-line N-005 stub
(the original finding text + the N-004/N-006 Resolution sections were
lost from the working tree; git history held them) — restored in full
and all three cited test sets verified present (Retry.Backtrack* x3,
HfsPlus.LargeBlockSizeAccepted, Btrfs.NonPow2NodesizeRejectedRegistered).
N-009 (c/d sub-rows open) and N-036 (container-map half open) stay in
MINOR/ as partially resolved by design.

**Counts after the reconciliation (direct file inventory at this
commit):** SEVERE 3 (S-009, S-029, S-030) · MAJOR 39 (incl. the known
duplicate M-029 pair) · MINOR 49 · INFO 22 (all ACKNOWLEDGED — 0
actionable) · tracked-open 16 · **FIXED/ 79 files (78 findings +
RESOLVED-INDEX)**. Session fix tally (resolutions this session): 6
moved-with-evidence + the group-file restoration.

**Carve-magic quartet FIXED — CONVERGENT LANDINGS (this wave):** two
parallel legs independently implemented the same four findings from the
same corpus citations (the parallel lane's `b2f48c6` landed first and is
canonical: bounded-buffer MKV DocType webm/matroska walk + the 0x200
element-bounds gate; FMT_HEIF registry row for the HEIF relabel;
FMT_TIFF_EP claims the IIU key with FMT_RW2 as the registered salvage
D-63a; RAF keyed on the FUJI 4CC with the variant path, JPEG prefix
registered D-64a). This session's convergence contributions: the
independent corpus decode of the artifact's second constant 0x67805318
(the Segment element ID 0x18538067 read little-endian — recorded in the
finding + module doc for future audits), a second fail-before proof set
(8/10 of this leg's name-identical tests RED on the stash-isolated
pre-fix engine), and the finding-file moves to FIXED/ with the union
Resolutions. **NEW-FINDING N-076 (DFX12, unique to this session):**
while building N-054's fixtures: the canonical RAF version gate
rejected every real-world RAF — the version at +16 is a 4-byte ASCII
string ("0100"/"0201") misread as binary (LE values ~8e8 above the
0x2000 cap); BOTH quartet landings carried the defect; the row was dead
for real files since introduction, zero fixture coverage. Fixed in the
union follow-up: ASCII-digits OR small-binary acceptance;
Carve.RafRealAsciiVersionAccepted (fail-before executed RED on the
pre-fix engine). Suite **563/563** linux-release post-union.

**Counts after the quartet (direct file inventory at this commit):**
SEVERE 1 (S-029 — S-009 and S-030 landed by the parallel lanes in the
same window) · MAJOR 36 · MINOR 45 · INFO 22 (all ACKNOWLEDGED) ·
tracked-open 16 · **FIXED/ 89 files (88 findings + RESOLVED-INDEX)**. Session resolution tally so
far: 6 reconciliation moves + the N-076 discovery-fix + the quartet
convergence work. Ongoing: MINOR-first waves; N-073 remains a probable
N-004 duplicate — verification queued.

**Carve-minors batch FIXED (this wave):** N-069 (CAB lowercase mscf key —
the og's duplicate-key policy for case-insensitive formats, census
:94/:127 re-read first-hand: a second FMT_CAB_LC row claims the
lowercase magic with the same validate_cab;
Carve.CabLowercaseMagic, fail-before by construction — the byte-exact
scan could never match a key no row claimed) and N-030 (raw-sign
confidence semantics — resolved as the REGISTERED DELIBERATE SUPERSET
its own fix plan calls option (b): the field is dead data in the og
(FUN_18001e510 :23522-23765 never reads part[2]; erratum 19), the core's
winner-selection + (hdr+tail)/2+100 blend are pinned by
RawSignLayerHeaderOnlyAndSharedMagic / RawSignTailPairWithinFormat,
PARITY-DELTAS D-66 carries the do-not-remove guidance, and option (a)
stays gated on the undecoded FUN_18000d670 grade emission). Suite
**577/577** linux-release. Counts: MINOR 43 · FIXED/ 91 files (90
findings + index). Session unique resolutions: 9 (6 reconciliation +
N-076 + N-069 + N-030).

**Dead-surface batch FIXED (this wave):** N-041 — the wire-or-delete
decision executed as DELETE (the finding's own recommendation): the
structured-open RdaClient class + the plumbing only it reached
(rda_bind_structured, the device's structured_/OPEN_TARGET reopen
branch, the orphaned grammar-bridge helpers) + volume_letter_to_disk —
~16 KB / 511 diff lines of zero-caller code removed. The wire structs
stay in cr_rda.h as the documented driver-contract mirror (marked); a
CI dead-surface absence lint (the fix plan's
Transport.GrammarPathIsSoleSurface, implemented as a lint-job grep gate)
guards the symbols' return. The PARITY-REFACTOR-PLAN D-A KEEP ruling
was SUPERSEDED with evidence (its cited TRANSPORT-CONTRACT §8 no
longer exists in the tree; the "UI reads info()/ioctl_base()/session_*
through it" claim is disproven by the exhaustive zero-caller grep).
PARITY-ARCH.md:152's refuted claim corrected; AUDIT-ARCH ARCH-DEAD-1
RESOLVED / ARCH-DEAD-2 re-annotated; CAPABILITY-MATRIX +
DRIVER-UTILIZATION + PARITY-ORCH rows updated; FB-T06/FB-T07 moot.
Suite 584/584 linux-release; format-churn hunks reverted to HEAD bytes
so the format-18 CI gate sees only the intended diff. Counts: MINOR 42
· FIXED/ 96 files (95 findings + index). Session unique resolutions:
10 (6 reconciliation + N-076 + N-030 + N-041; N-069/N-051..N-054
convergent with the parallel lanes).

**Text-policy batch (this wave):** N-016a (the N-029 group's first half)
FIXED — text::utf8_to_u16 now follows the maximal-subpart policy (RFC 3629
§3.1; MultiByteToWideChar(CP_UTF8, flags=0) semantics, the og's
restore-path conversion): ONE U+FFFD per ill-formed maximal subpart
instead of one per bad byte — a truncated/interrupted sequence consumes
its lead + the continuations that extended it; a fully-read but invalid
sequence (overlong/surrogate/range) consumes all of it; C0/C1 excluded at
lead classification (RFC 3629). Text.MaximalSubpartReplacement (the
finding's named vectors) + four re-pinned policy tests; fail-before
executed on the stash-isolated pre-fix decoder (4 RED). N-016b (raw-bytes
persistence in sessions — versioned name_raw field) remains open with the
precise schema step recorded in N-029's Resolution. Suite 594/594
linux-release.

**Table-visualization batch FIXED (this wave):** N-057 — all three
sub-items: (1) TableLayout::Ebr assigned on extended chains (the
recommended SET; the ABI gains the APPENDED code 3 with 0/1/2 frozen,
the CLI prints EBR, plain MBR pinned; serialized-value stability is why
deletion was rejected); (2) the 64 MiB overlay-budget growth path tested
at the exact boundary (at-budget does NOT grow — the loop's `>`
semantics pinned); (3) the write-only `total` accumulator deleted.
Fail-before executed (both table tests RED on the pre-fix parse). Suite
600/600 linux-release. Counts: MINOR 41 · FIXED/ 97 files. Session
unique resolutions: 12 (6 reconciliation + N-076 + N-030 + N-041 +
N-016a + N-057).

## Session-8 delta (2026-10-26 — the closing-verification fix session, SEVERE lane first)

Session start `4a924d1` (rebased bootstrap `3494606`); corpus `03ad46c`
unmoved. Baseline before any change: Linux Release suite **500/500** +
`cr_ui_tests` 21/21. Order per mandate: SEVERE → MAJOR → MINOR → INFO →
TRACKED-OPEN; every finding independently re-verified before fixing.

**S-009 FIXED (SEVERE — the disk-map free overlay; the last open half of
the S-009/M-006 split file):** the engine bitmap truth is now produced,
consumed, and persisted. `storage::FreeMap` (new type) filled by NTFS
(`$Bitmap` inverted), FAT (table entry-0 = free), exFAT (allocation-bitmap
dentry inverted) — rebased to the absolute device basis exactly like the
tree extents; `build_disk_map` paints `kCellAllocFree` only on
still-unknown cells fully covered by bitmap-free clusters (USED and FsBad
are never overridden — hostile-map precedence proven by test);
`REC_ALLOC_MAP` (new optional record) persists the DOWNSAMPLED
cell-granular map (the same 64 MiB-budget sizing — ≤ 8 MiB on a 16 TiB
volume, never a raw $Bitmap copy) and restores it as the session's
cluster hint; both snapshot paths value-carry the truth (session
snapshots: errors + free map + view bounds; scan snapshots: the live
telemetry error map + the completed map). Doc drift retired (the header's
"not persisted (registered follow-up)" rule; ABI-CONTRACT §7's "3 bad
(marked by the FS)"). 4 fail-before-by-construction tests (the finding
itself grep-verified `kCellAllocFree` had ZERO assignment sites); suite
**515/515 + cr_ui_tests 21/21 + cr_ui_abi_check** green on Release AND
ASan (merged tree incl. the parallel lanes' S-032/N-018 landings).
N-057's bad-overlay coverage item paid; Ebr enum + budget-growth test
remain its scope.

**NEW-FINDING M-053 (MAJOR, filed + FIXED):** discovered during the S-009
verification pass — the C-ABI consumer check (`cr_ui_abi_check`, registered
in ctest) has been DEAD since M-015 landed (`6dc1e32`): the M-015 fixture
update missed `tests/ui_abi_check.c`, whose FAT16 image lacks the canonical
media words, so every scan of it returns `ERR_BAD_MAGIC` and the check's
non-halting CHECK macros cascade into a `rows[0]` NULL-deref crash
(release: silent exit 139 with buffered output lost; ASan: the SEGV
report). Root-caused with a deterministic plain-C reproducer (the sole
differing bytes between the passing C++ fixture and the failing C fixture
are the two media-word pairs). Fix: canonical media words in the fixture +
a precondition bail-out after a failed scan (a real consumer checks the
return code and stops). Engine untouched — M-015's exact-pair validator is
correct og parity. CONVERGENT with the parallel N-018 lane's fixture fix
(`b1f9210`/`abf5663`); this leg's unique contribution is the formal finding
record + the bail-out hardening. `cr_ui_abi_check` green on release AND
ASan (executed both lanes).

**S-030 FIXED (SEVERE — disk-image scan targets):** the og's import
surface re-implemented from format understanding in the new core module
`cr/storage/cr_image.{h,cpp}`: flat files, VMDK (monolithicSparse with
embedded descriptors + standalone descriptors whose FLAT/SPARSE/ZERO
extent lines open through the host-injected ops seam, grain-directory/
grain-table mapping with grain-segment batching), VHD (fixed = flat minus
the trailing footer; dynamic = BAT + per-block sector bitmaps), content
sniffing incl. the TRAILING-footer VHD identity (the tests caught the
head-only sniff missing fixed VHDs), and the og's LBA0 MBR/GPT
classification. The core stays OS-header-free (byte-source injection);
the CLI gains `scan --image FILE` / `recover --image FILE`; the session
identity card gains the v3 image-path tail (v1/v2 back-compat pinned).
8 tests incl. engine scans over a sparse VMDK (< half the flat size,
byte-identical readback) and a dynamic VHD; fuzz_image added to the fuzz
suite; D-64 registers the edisk/dmg exclusions + VHDX/parent-chain
remainders. Suite **525/525 + 21/21 + abi check** green on Release AND
ASan. SEVERE tier: **only S-029 remains** (the consent-gated
lost-partition apply path).

**Counts after the session-8 SEVERE lane (direct file inventory):**
ACUTE 0 · **SEVERE 1** (S-029 — S-032 was fixed by the parallel lane
in-window) · MAJOR 39 (M-053 filed+fixed in-session) · MINOR 55 ·
INFO 22 (all ACKNOWLEDGED) = **117 active / 74 FIXED** (live inventory:
findings/FIXED/ = 75 files incl. RESOLVED-INDEX).

## Session-6 delta (2026-10-26 — the S-024 decoded-truth upgrade)

Mandate: fix ALL tiers; this leg re-verified the landed S-024 grading
fix against the corpus and found the first landing's four documented
analog guesses superseded by decodeable truths. The corpus wins: the
analog temperature-Caution rule (a divergence — the og grader has NO
temperature branch, 0x1808 is never read), the is-SSD analog gate for
SSD-life (the og gates by decoded vendor families, FUN_0040dad0), the
NVMe spec-bit split for critical_warning (the og grades ANY nonzero
byte Bad) and worst-wins aggregation (the og is
bad-count/caution/graded-signal in FUN_0040cdf0) — all replaced with
the exact decoded rules, plus `model_says_ssd` (the 0x1724 writer list
:6300-6344 — the finding's open question #1, now decoded),
`classify_family` (30+ vendor families — open question #2, decoded),
the family-0x13 NVMe path, duplicate-attr-id Unknown, the 5/197/198
all-FF guard asymmetry, and the failing-bit superset properly
registered as D-63. Evidence:
`evidence/decompiled-quotes/s6-health-grading-decode.md`. 12 new/rewritten
Health tests (suite 464 on this tree: 460 pass + the 4 pre-existing
Ext4Engine.Backup* reds owned by the parallel S-027 lane); fail-before by
construction (the landed header lacks classify_family/make_grade_inputs).
Classifier residual: the exotic-vendor thunks stay generic 0/1 — a pure
data gap, engine consumes all family codes; exact next decode sites
listed in the evidence §5.

**M-036 FIXED (this push):** NVMe health's full row set — host reads /
writes, controller busy time, error-log entries (spec offsets; the og's
route was already a byte-parity MATCH per I-018) — plus the data-unit →
byte conversion exactly as the og computes it (×512,000,
DiskHealth_decompiled.c :5589-5614, re-verified first-hand; a full
64-bit multiply). NvmeHealthInfo extends, the cr_ui C-ABI report
appends the six fields with CR_UI_ABI_VERSION 1 → 2 (ABI-CONTRACT §9;
contract docs updated in the same change), the CLI prints bytes like
the og. 2 new tests + the moved 192-byte gate; suite 485/485 Linux
Release. N-009a's pin re-verified in the same file.

**Session-6 leg final counts (direct file inventory at 70ebced, after
the parallel legs' concurrent landings):** FIXED/ 70 findings ·
**SEVERE 4** (S-009, S-029, S-030, S-032) · MAJOR 39 · MINOR 55 ·
INFO 22 (all ACKNOWLEDGED) · tracked-open 16. This leg closed S-024
(decoded-truth upgrade) + M-036 + N-055; the parallel legs closed
S-007/S-025/S-027/S-028/S-031/M-011/M-012/M-015/A-005/A-007 and more
during the same window. Suite 526/526 at 26d842b. Top remaining risks:
S-029 (apply-path scope decision pending), S-030 (mid-flight disk-image
lane — re-verify landed state first), S-009 (unclaimed since session 1).

# SUMMARY.md — executive summary (session 7 on top of session 5b; earlier sessions below)

## Closing-session delta — S-032 (2026-10-26)

**S-032 FIXED (this push):** archive inner-entry inventories (the og's
analysers return the complete inventory; the results tree presents it —
rawsearchdll--zip-analyser-carving.md :29-38/:64,
analyser-granular-batch1 :36-41, all re-verified first-hand). ZIP CD walk +
indexless LFH chain + RAR4 FILE blocks collect bounded entries; carved
archive nodes gain Carved+Deleted children with precise in-parent extents
(registered superset: per-entry recovery). DISCOVERED+FIXED in the same
change: the RAR4 validator's HEAD_SIZE off-by-one (no positive real-layout
fixture had ever existed — real archives now parse). Remainders tracked:
RAR multi-part stitching; RAR5 entries (clean-room surface). 3 tests, all
fail-before; suite 517/517; ASan/UBSan clean over the carve suite.

## Session-7 delta (2026-10-26 — the close-all-tiers fix session, low-severity-first order)

Mandate (user-specified ordering): INFORMATIONAL → MINOR → MAJOR → SEVERE →
TRACKED-OPEN, with every finding independently re-verified against corpus
`03ad46c` and the running code before any change; fixes proven with tests;
audit trail exact. Session start `c75f646` + phantom-08c66f8 repair
(`4702ec8`); corpus `03ad46c` unmoved.

**INFO tier CLOSED (this push):** all 18 INFORMATIONAL findings now carry
Resolution sections — I-001/I-002/I-003 were already acknowledged (prior
sessions); the remaining 15 (I-006..I-020) were verified this session and
ACKNOWLEDGED with first-hand citations (no rationale was found wrong; no
code change was required by any). Actions landed with the batch:
the I-009 session-3 addendum's pin test
`CarveEngine.CarvesMidSectorStart` (PNG at sector+137 — the byte-granularity
superset over the og's whole-sector stepping; guards the M-025 rework);
PARITY-DELTAS **D-59a..D-59k** (the registration rows the findings' fix
plans demanded); modules/visualization.md's stale D16 gap figure updated
(I-011's leftover action — M-023 FIXED superseded it). Suite **440/440**
Linux ASan green. Committed conflict markers in CROSS-REF-MATRIX.md
(phantom `08c66f8` leg, introduced by `c75f646`) resolved as DFX7-T0.

**Counts after the INFO batch (direct file inventory at the push head):**
ACUTE 0 · SEVERE 9 · MAJOR 38 · MINOR 31 · INFO 18 (all ACKNOWLEDGED —
0 actionable) = 96 active / 49 FIXED; INFO tier now zero-open.

## Closing-session verification delta (2026-10-26 — the df5 lineage verification pass)

The S-003/S-021 lane (claimed at ff05314) was landed by the parallel close-all-tiers
leg at c75f646; this lineage executed the independent verification pass it had
already completed the corpus decode for (CHFSPlusJournal ctor/walker/validator
asm-exact + TN1150 journal-section fetch verbatim). **Corrections landed:** the
false-parity JIB layout (a 'jnlx'@+4 magic TN1150 does not have — device_signature[8]
lives there; a silent zero-harvest on every real journaled volume, fixtures encoded
the same fiction), the two missing og entry gates (VH kHFSVolumeJournaledBit,
JIB kJIJournalInFSMask), the asm-strict node validator (mid-chain offset-table
violations REJECT), the missing attributes branch (decmpfs size-only fallback), the
header-invalid fallback salvage ladder (OQ-S003 RESOLVED — FUN_180003ea0 decoded:
scratch-node harvest, never a live-tree install), and the in_fork u64 overflow.
The max_blocks probe-marker derivation is RESOLVED ((blhdr_size-16)/16). Fail-before:
6 tests red on the pre-correction tree; pass-after 491/491.

**NEW-FINDING A-005 (ACUTE) filed during the same verification:** the
extents-overflow record parser read its 8 descriptors from key+10 — the
TN1150-verbatim key is 12 bytes (keyLength field + 10 data bytes), so fragmented
forks (>8 extents) silently lost their tail extents (the bounds clamp dropped the
garbage descriptor). Zero prior test coverage (every fixture's overflow fork was
empty). FIXED this session with the first populated overflow fixture; fail-before
executed (1/2 extents) — `evidence/executions/a005-extents-off-by-two.md`.

**Counts after the closing-session verification pass (direct file inventory at
the correction commit):** ACUTE **1** (A-006 rawfixer 4Kn — the diff-fix s4 leg's
claimed lane; A-007 extents-overflow was filed AND fixed in-session, moved to
FIXED/) · SEVERE 5 (S-009-remainder, S-029, S-030, S-031, S-032 — the parallel
legs closed S-007/S-023/S-024/S-025/S-027/S-028 in the same window) · MAJOR 42 ·
MINOR 44 · INFO 20 = **112 active** / **63 FIXED-family files** (62 findings +
RESOLVED-INDEX, incl. OQ-S003 resolved this pass; A-007 among them). The parallel
legs' own SUMMARY sections below carry their in-flight counts; this paragraph is
the closing-session's file-inventory truth at its correction commit.

## Session-5b delta (2026-10-26 — the never-dispositioned MOM/product-capabilities leg)

Mandate: find what ALL prior sessions missed, verify landed fixes, go deeper on
og-vs-core, mark remaining surfaces as findings. Method: coverage-gap map (corpus
inventory 11,495 files vs every citation in diff-to-fix/+worklog+AUDIT docs)
exposed the entire `ghidra-mo`/`ghidra-partition` `.mo` family and the
ghidra-partition remainders as never-audited; four parallel read agents (A:
boot-repair/partition-write/OS-aware; B: disk-image/device layer; C: FS
analysers/check phase/virtual views; D: persistence/logging/LDM-write/FATX) +
lead verification of every load-bearing citation. **ID coordination:** this leg's
13 findings are renumbered past the storage/census parallel leg's
S-024..S-028/M-031..M-039/N-045..N-047/I-017/I-018 (no content overlap — pure ID
deconfliction, append-only convention).

**Counts after session 5b (direct file inventory at the rebase head on
`c75f646`):** ACUTE 0 · **SEVERE 9** (S-007, S-009-partial, S-023, S-024,
S-025, S-027, S-028, S-029, S-030) · **MAJOR 38** · **MINOR 31** ·
**INFO 18** = **96 active** / **49 FIXED** (findings/FIXED/ by file inventory
minus RESOLVED-INDEX; includes the fix legs' S-012/S-020/S-021/S-003/S-022/
S-026/M-022/M-023/N-037 landings visible at rebase time).

**The session's headline discoveries (all first-time findings; renumbered IDs):**

- **S-029 (SEVERE)** — the lost-partition APPLY path: the og writes the
  recovered partition into the table (RecoveryModule `CreateRecoveryPartitionObject`
  → PartitionRecovery.dll IOCTL chain 0x7C100/0x7C010/0x7C058 + GPT GUID
  regeneration); the core is contractually read-only (cr_lostpart.h:91-92). The
  endgame of lost-partition recovery is absent — scope decision forced
  (consent-gated apply vs documented exclusion).
- **S-030 (SEVERE)** — disk-image scan targets: the og scans
  .vmdk/.vhd/.vhdx/.edisk/.img/.dmg as first-class devices (DRWUI
  CImageFileDevice extension factory + Device.mo/libvmdk with extent/parent-chain
  support); the core accepts only --disk/--volume. A whole input class missing.
- **M-040 (MAJOR)** — the boot-chain repair family: Fixup.mo binary-patches BCD
  hive cells (newLBA<<9 at offset fields, dual-store Boot\BCD + ESP store,
  ZwUnloadKey force-unlock, DASD flush), EFIBoot.mo manipulates UEFI NVRAM
  Boot####/BootOrder (GUID 8BE4DF61), boot.ini ARC rewrite, post-op
  `ChkDsk.exe %C: /F /X` + Bcd_RestoreBcdFile undo — wired into the
  RecoveryModule/MainModule apply/undo chains; core has zero of it and no
  disposition.
- **M-041 (MAJOR)** — the damage-audit/check phase: CCheckFAT (pairwise
  FAT-copy compare 0x1005, entry-hole + duplicate-reference scans, dir-tree
  audit incl. cycles/cross-links/size≠chain, orphan sweep) and CCheckNTFSVolume
  ($MFT/$MFTMirr proofs, version gate {1.2,3.0,3.1}, $Bitmap structural,
  MFT×$Bitmap cross-check), error taxonomy 0x1001-0x100e, WinChkdsk delegation.
  Core has only repair-oriented rawfixer checks.
- **M-042 (MAJOR)** — lostpart.dat resume semantics: the og re-validates each
  persisted candidate against the disk and drops stale records (with a
  partinfo.dat whole-disk layout snapshot); the core's session restore is
  blind. Full 0x448-byte record layout decoded and cross-validated against the
  shipped .dat files.
- **M-043 (MAJOR)** — NTFS recognition: og requires MFT $Volume content proof +
  verifies the volume-end backup boot; the core's scanner never reads the NTFS
  backup DBR (torn-primary volumes require --repair).
- **M-044 (MAJOR)** — UDF recognition family missing (og analyser result 0xc;
  core has no UDF branch in probe_family).
- **M-045 (MAJOR)** — FAT boot-gate family: six concrete gate deltas (bps
  any-512-multiple vs pow2-512..4096; SPC ≤128 vs unbounded; "FAT32   "@+0x52
  label; numFATs≠0; cluster ≥0xFFFFFF0 cap; band-ordered flavor retry).
  Resolves S-006's FATX scoped remainder: **CFatxBlockParser is the
  FAT12/16/32 parser — no Xbox FATX exists in the og** (definitive negative
  grep; corpus-errata row 41).
- **N-048/N-049/N-050 (MINOR)** — engine logging seam absent (3 og layers);
  persisted lost records thinner (no bps/SPC/evidence); recognition contract
  boot-only (no content proof before engine dispatch).
- **I-019/I-020 (INFO)** — EPM tool ops registered as scope exclusion
  (EditLdm2*, Ntfs2FATProc conversion, partition merge/resize, FatFormat,
  Burn — no recovery-flow consumers); corpus-doc corrections register
  (CTbFile is a boot.ini wrapper not an image abstraction — the real image
  layer is DRWUI+libvmdk; CFatVirtualView is conversion not recovery;
  EFIBoot is NVRAM not ESP; Fixup is link-time not MOM; the grading table is
  .data/unsorted).

**Resolutions this session:** OQ-S4-1 (the 169-entry grading extension table
dumped — `.data` 0x141bb0500, 9-byte stride, first "Raw", last "COMMON";
S-023's fix-plan step 3 input complete); OQ-S4-2 (the RAW naming hook =
single-site "filter_generalraw" category display-name mapping via DRWUI's
firebase.json — no per-file filter; M-029 §Gap (b) corrected); S-006's FATX
remainder dissolved; S-019's fix verified in-source; two stale duplicate
matrix rows consolidated; two accidental finding-file duplicates caught and
fixed in-session.

**Top-risk update (post session 5b):** #1 S-023 (grading — NOW UNBLOCKED:
the extension table is dumped), #2 S-024 (health grading — the storage/census
leg's; same engine lane), #3 **S-029 (apply-path scope decision — blocks
M-040)**, #4 S-020's family remainder (S-025/S-027/S-028 — ext4/Btrfs
lanes claimed by the fix legs), #5 **S-030 (image scan targets — also
unblocks og-vs-core executed comparisons on identical image fixtures)**,
#6 **M-041 (check phase — error taxonomy test-scoped)**, #7 M-028 (preview
caps, executed proof), #8 S-007 (carve formats — lane claimed), then the
M-042/M-043/M-045 family and the prior backlog order below.



# SUMMARY.md — executive summary (session-5 audit leg on top of the closing/fix sessions; earlier sessions below)

## Session-5 audit delta (2026-10-26 — the citation-gap / remaining-surface READ-ONLY session; full detail in the Session-5 delta section below)

The audit leg's full delta (24 findings: A-005 ACUTE rawfixer 4Kn stack-buffer
overflow; S-031 carved-deleted; S-032 archive inventories; M-046..M-051;
N-051..N-064; I-021/I-022; OQ-S4-1 RESOLVED convergently with the fix leg —
structure+consumption decode here, the extension-list dump there, both folded
into S-023; OQ-S4-2 RESOLVED; corpus errata 37-40; suite 366/366 evidence at
`7cf6754`) is recorded verbatim in the "Session-5 delta" section below — keep
reading. **Counts after session-5 merge (direct file inventory at the merged tree):
1 ACUTE (A-005) · 10 SEVERE · 43 MAJOR · 44 MINOR · 20 INFO = 118 active ·
54 FIXED-family files (N-055 fixed by the parallel S-024 leg) (incl. RESOLVED-INDEX + the OQ-T1 record). The session-5
leg filed 25 findings (A-005; S-031/S-032; M-046..M-051; N-051..N-064;
I-021/I-022). The parallel fix legs' own counts sections below are historical
snapshots, superseded by this line.**

## diff-to-fix closing session delta (2026-10-26 — the "fix ALL tiers" session)

Session start `b65f1e6` (baseline 396/396 Linux Release, verified first-hand on
this host). Corpus `03ad46c` (unmoved). Mandate: close every finding — verify
each independently against the corpus AND the running code before any change;
fix with tests that fail before and pass after; keep the audit trail exact.

**S-026 FIXED (this push):** ext4 meta_bg group-descriptor resolution. The
verification went beyond the decompile — two REAL `mke2fs -O meta_bg` images
were built with e2fsprogs 1.47.2 (1KiB/64B-desc and 4KiB/64B-desc) and the
layout pinned empirically (dumpe2fs + hexdump byte-matching). Implemented the
kernel/e2fsprogs-correct arithmetic (`meta_group_size = block_size/desc_size`;
descriptor block at `first_data_block + m*meta_size*bpg`, +1 only for m==0;
`s_first_meta_bg` classic front region; lazy bounded per-meta-group cache).
The og's stride (`s_desc_size`) and missing-`first_data_block` defects are
deliberately NOT adopted — registered as **PARITY-DELTAS D-58**; OQ-E2
RESOLVED by the same evidence. 5 tests (4 fail-before proven on the
engine-reverted build); suite **401/401** Linux Release; ASan+UBSan fuzz
smoke clean over structured meta_bg seeds.

**S-027 FIXED (this push):** ext4 backup-superblock rescue (the og's
engine-side lost-volume walk, FUN_180001b00 slot 53 — re-verified first-hand
incl. the vtable .rdata reference). Implemented inside scan_ext4: canonical-
position probe grid (1 MiB, both +0/+1024 families), exact volume-base
recovery from the backup's own s_block_group_nr (verified on real images:
group-1 copy = 1), GDT ladder front-first then backup-GDT-after-SB (full
table classic / meta-group-0 for meta_bg, verified). Og boot-area rebase
NOT adopted → **PARITY-DELTAS D-59**. 4 tests (3 fail-before); suite
434/434 serial; ASan/UBSan fuzz smoke clean.

**S-025 FIXED (this push):** ext4 journal reparenting (the og's two-pass
model + orphan parent map + iNode%I64d fallback — FUN_180003de0 :2773/:2778,
FUN_1800043f0 :3077-3145, FUN_1800041e0 :2986-2991, all re-verified first-hand).
One-structural-pass harvest + topological pre-pass with full inode truth +
live anchors; claim checks via live_nodes (files too — latent duplicate gap
closed); sweep skip-set for journal-emitted inodes. 3 tests, all fail-before;
suite 465/465; ASan/UBSan fuzz smoke clean. The ext4 SEVERE family
(S-025/S-026/S-027) is now fully closed by this session.

**Counts after S-025 (direct file inventory):** ACUTE 0 · **SEVERE 7** ·
MAJOR 36 · MINOR 31 · INFO 18 = **92 active** / **53 FIXED** (counts
re-baselined against the parallel legs' concurrent landings: S-003/S-021
HFS+ journal pair, S-020 ReFS attribute layer, M-022/M-023/M-026/M-027,
the minors wave, INFO tier ACKNOWLEDGED).



## Close-all-tiers session delta (2026-10-26 — the "fix every finding" session)

Session start `3278a22` (bootstrap on `bf7df39`; suite 396/396 baseline). Corpus
`03ad46c` (unmoved). Mandate: verify every finding independently against the
corpus before fixing; resolve ALL tiers; resolved findings move to `findings/FIXED/`
with Resolution sections; matrix/modules/counts updated as fixes land.

- **M-023 FIXED** (category model — the S-023 dependency): Category enum
  +Email(7)/+Bookmarks(8); the 64-entry hand table replaced by the generated
  1,481-entry table (scripts/gen_filefilter.py from the corpus FileFilter.xml;
  first-writer-wins for the 8 cross-listed); binary-search lookup;
  category_mask default 0xFF→0x1FF (Bookmarks bit). Fail-before: behavioral
  probe (camera-RAW → Other pre-fix) + enum-by-construction (Email/Bookmarks
  absent pre-fix). 5 tests; suite 401/401 Release + 401/401 ASan(leaks).
- **OQ-S4-1 RESOLVED** (the 169-entry grading extension table at .rdata
  0x141bb0500 — extracted first-hand from the corpus DRWUI.exe: 169 × 9-byte
  entries {8-char NUL-padded key, flag byte 0}, first key "Raw"; the dump
  feeds S-023's deep-validation gate).
- **S-023 FIXED** (the grading engine — the largest missing capability):
  tree::Grade {Low,Average,High} with the og's exact setter semantics
  (monotonic-max, grade-2 sticky, overlap flag); the post-scan pass
  (cr_grading.cpp) implementing the decoded decision tree (size floor,
  Email-category fast path, existing-file rule, carve-device small-file
  rule, NTFS device-index rule + QuickScanDeleteExtend counter,
  extension-table-gated extent-end twin detection with the FAT-family −2
  address offset); per-selection regrade (grade-1 only); RecoveryAssistant
  statistics (8 counters + 8 category counts + matched/total with the
  small-result heuristic); ABI surface (node grade/overlap, query-row grade,
  sort key 6, cr_ui_scan_assistant, cr_ui_session_regrade); session
  persistence (node bit 3 + REC_ASSISTANT_STATS record 6); the report's
  scan_analysis block (CLI + ABI paths). Registered non-adoptions: the
  UNDECODED NTFS vt+0xb8/vt+0x100 sub-rule (no blind structural guess) and
  the og's internal matched/total composition (matched = grade-2 count,
  documented). 15 unit tests + C-consumer lifecycle; fail-before by
  construction (capability absent). Suite 429/429 Release + 429/429 ASan.
  **In-walk Confidence untouched** (allocation-axis superset).

**Counts after S-023 (direct file inventory):** ACUTE 0 · SEVERE 10 · MAJOR 33 ·
MINOR 37 · INFO 16 = **96 active** / **35 files in FIXED/** (34 findings +
RESOLVED-INDEX).

(Reconciled post-rebase: the parallel legs landed S-003/S-021/S-026, M-022,
N-037..N-040 and more while this session worked — the live counts below are
the file inventory at this commit.)

- **S-024 FIXED** (health grading): HealthGrade (Unknown/Good/Caution/Bad)
  + the pure-core rule set (cr_grade.cpp): 5/197/198 value<thr->Bad /
  raw>=1->Caution; attr-1 read-error rule; the SSD-life 10-attr vendor set
  (is-SSD gated; 0->Bad, <=10->Caution); temp 50/60 by class; NVMe
  critical-warning bit matrix + the life rule. 0xD1 READ_THRESHOLDS issued
  (vendor thresholds). CLI health prints per-attr threshold/grade + the
  disk verdict. 8 tests; suite 460/460 Release. AUDIT-PIPELINE's "failing
  derivation: UNVERIFIED" row -> SUPERSEDED (the 4-status model).

**Live counts (direct file inventory after S-024):** ACUTE 0 · SEVERE 7 ·
MAJOR 38 · MINOR 31 · INFO 18 · tracked-open 12 = **94 active** / **51 files in FIXED/**.

## S-012 verification-and-correction delta (2026-10-26 — diff-fix session 4, the reconciliation leg)

Bootstrap `597a9c6` (350/350 baseline re-run in-container); corpus `03ad46c`
(unmoved). This leg independently implemented S-012 steps 1+2 in parallel
with the landed `5f1a212` leg (local commit discarded during the
pull-frequently reconciliation — the landed leg's size-0 rescue made it the
more complete substrate), then **re-verified the landed implementation
line-by-line against the corpus** and corrected three divergences, each
corpus-proven and fail-before/pinned:

- **Dump-walk stride** — the og strides the RECORD SIZE on every path
  (uVar8 after holes LAB_18001b0cb :21293 and after emit :21151; uVar16 =
  max(0x400, rs) only after props-fail/skip/destroy, coinciding through the
  boot clamp). The landed code's one-sector hole stride examined positions
  the og never visits; its max(sector, 0x400) parse bound truncated 2/4 KiB
  records. Pinned by `GuidDumpStrideIsRecordAligned` +
  `GuidDumpLargeRecordParsedFully` (both fail on the landed tree).
- **Pseudo record#** — corrected to the og's exact `lVar18 =
  local_c0/record_size + chunk byte base` arithmetic (:21150-51).
- **N-040 (new, FIXED)** — the pass-1 attribute filter: the og's gate is
  (og-15 name table) AND ($SI SYSTEM+HIDDEN) — FUN_180040f20's 15-entry
  table decoded first-hand — with NO $FILE_NAME-flags system filter; the
  old `dos_flags & 4` gate dropped pagefile.sys-class user files the og
  keeps. Gate + flag + counter removed; pinned by
  `MainWalkKeepsSystemFlaggedUserFile`. In `findings/FIXED/`.
- D-51 corrected in place; the D-50/D-51 numbering collision (section-K
  bullets vs headings, plus a duplicated D-51 FAT heading) registered as
  errata with the FAT entry renumbered D-56. Suite 399/399 Linux ASan;
  hostile ASan smoke 300 mutated fixtures (297 exercising pass 2, clean).
## Session-6 delta (2026-10-26 — the deep-continuation audit: never-cited surfaces + fix verification)

Session start `022c81d`-lineage (landing `bf7df39` mid-bootstrap); corpus `03ad46c` (unmoved).
Mandate (user directive): find what earlier sessions missed, verify/update the fixed things,
go deeper on og-vs-core, mark ALL remaining features/implementations/functions as findings,
focus on minor detail. The session-5 citation-gap lanes (defined at `2001f7c`, never executed)
were this session's scope — executed end-to-end via 5 parallel read agents + first-hand
verification of every load-bearing citation before filing (three stale agent claims filtered
by verification: FAT alt-EOC [N-002 fix live], NTFS BAAD [MATCH both sides], FAT quick-scan
threading [registered D-30x]).

**Multi-leg ID reconciliation (the parallel audit legs landed S-029..S-032, M-040..M-051,
N-048..N-064, I-019..I-022 in the same window):** this session's local M-041/M-042/N-050
CONVERGED with the landed S-031/S-032/M-051+N-059 (same census artifacts, independently
decoded) — dropped with harvest-addenda on the landed findings; the 14 unique findings
RENAMED past the taken IDs (M-052, N-065..N-075, I-023/I-024). The parallel fix legs also
landed fixes this window (S-007 primary set incl. the MPEG sync family, S-020/S-023/S-024,
S-025..S-028, M-010, M-022..M-027, N-005/N-011/N-012/N-024/N-028/N-038/N-039 …) — this
session re-verified the overlaps first-hand and posted post-resolution corrections where
its addenda were authored pre-fix (S-007: bare-MPEG FIXED — registry now carries
FMT_MPGA_E2..FF + ADTS with validate_mp3_raw; S-024: 0xD1 now issued at cr_health.cpp:192-200,
only the 0xD8 claim remains stale).

**First-time-cited corpus artifacts (23):** the 6 rawsearchdll carving-family artifacts +
analyser-registry census + dual-registry u16 decode + zip-analyser carving; rsf-record4
grammar + ifileproperties-part3; per-engine-function-map; MOM Error/Thread/Common/Config/
PartitionRecoveryD + mom-module-system; WinPE boot-config + eusendmessage; config-files
drwcfg/userdata-remoteconfig/vendor-logs; UIUX 13/19/20/24/25. **First-time-cited core files
(8):** cr_elevate, cr_destio, cr_snapshot, cr_cached, cr_decomp, cr_sha256, cr_error,
cr_diskmap (header).

**This session's surviving new findings (14 unique + 3 convergent harvests):**
- **MAJOR (1):** M-052 (FAT/exFAT recycle-bin $I/$R pairing missing — og IRecycleBinNode2UI;
  core grep 0; the NTFS-side model exists in-repo to generalize).
- **MINOR (11):** N-065 (writer collision `name_N`/cap 10000 + mkdir 0/0xB7 gates — refutes
  D-19's "unevidenced" with decompile evidence), N-066 (identity card weaker: firmware/WWN/
  interface + 512-byte boot snapshot vs CRC32), N-067 (exFAT upcase validation skipped),
  N-068 (ZIP EOCD anchor + LFH sanity gates — re-verified absent post-S-007-fix), N-069
  (CAB mscf case key — re-verified absent), N-070 (overlap scoring + two-generation
  fallback), N-071 (enumeration breadth), N-072 (Btrfs re-scan cache + ZFS label polling),
  N-073 (ext4 catch-buffer shifted-window salvage), N-074 (doc-drift group #2 — five stale
  claims incl. the refuted D-19 and the 0xD8 residual), N-075 (event-bus query-signals +
  pull-progress + typed IUIProgressInfo).
- **INFO (2):** I-023 (DRWCfg persistent-config scope register), I-024 (PartitionRecoveryD
  generator-sets + .dat persistence register).
- **Convergent harvests (addenda on landed findings):** S-031 (session round-trip mechanics
  + timestamp shape + adjacent-fields pointer), S-032 (results-tree consumer cite + size-
  from-sum + hostile-cap + nested OQ), N-059 (HFS+ mode/type half + cross-engine ID space).

**Addenda to active findings (9):** S-007 (family layer — roster with per-family provenance
+ post-resolution correction), S-009 (stale premise corrected + free-overlay residual with
concrete fix plan), S-024 (doc-truth sub-item + post-resolution correction), M-017
(CalcPartition class evidence), M-030 (WinPE driver selection: eprdtdrv absent from PE,
ebrntdrv/epmdkdrv via NtLoadDriver, win7older==ebrntdrv MD5), M-029 + N-033 (partial fixes
verified in-tree), N-045 (cache algorithm divergence + cite refresh). **New tracked-open
(3):** OQ-S6-1 (PartitionRecoveryD generator-set semantics), OQ-S6-2 (Btrfs RC4 target),
OQ-S6-3 (CRAW +0xF4/anchor-B/checksum).

**Counts after the session-6 reconciliation (direct file inventory, refreshed through the
last mid-session landings):** ACUTE 0 (A-005/A-006 both fixed in-window) · SEVERE 5
(S-009 residual, S-029, S-030, S-031, S-032 — S-031's deleted-flag half fixed and
verified mid-session; the timestamp residual stays open) · MAJOR 40 · MINOR 55 · INFO 22
= **122 active / 68 FIXED**; 16 tracked-open files. (The parallel legs' fix wave moved
34 findings to FIXED in the same window — S-007, S-020..S-028, S-031, M-010, M-022..M-027,
N-005, N-011, N-012, N-024, N-028, N-038, N-039, A-005, A-006 among them — while their
audit legs filed A-006, S-029..S-032, M-043..M-051, N-048..N-064, I-021/I-022; this
session's addenda re-verified the overlapping fixes first-hand where its citations
touched them.)

**Recommended first-10 from this session's crop (for the fix legs):**
1. M-052 (recycle pairing — generalize the NTFS model; 3 fail-before tests specified)
2. S-031's two-site emission fix (this session's round-trip mechanics make the reload
   consequence precise — S-031 is the landed canonical finding)
3. N-074 (five doc fixes + the 0xD8 residual — an hour of work, consumer trust)
4. N-068 (ZIP EOCD anchor + LFH gates — two small validators; re-verified absent)
5. N-065 (writer collision/mkdir alignment + D-19 rewrite — decompile-evidenced)
6. N-070 (overlap scoring — collect-then-max instead of break)
7. S-032's inventory payload (CarveResult extension; this session's hostile-cap +
   size-from-sum additions fold into its fix plan)
8. N-067 (exFAT upcase validation — grade-not-reject)
9. N-075 (busy/stopping query exports or the deliberate-difference registration)
10. N-073 (ext4 catch-buffer backtrack — fault-injection test specified)

---



## Minors-fix session delta (2026-10-26 — the "fix all tiers, focus the 33 minors" session)

Session start `ed00020` (baseline 345/345 Linux Release; +1 test from the concurrent
transport wave `a4053c4` → 346). Corpus `03ad46c` (unmoved). Mandate: verify every
finding independently against the corpus before fixing; focus the MINOR tier.

**Batch 1 — FAT/exFAT/lost-partition minors (this push):**
- **N-022 FIXED** (mirror consult on primary FREE only by default; BAD-salvage opt-in;
  fail-before proven on the pre-fix worktree).
- **N-023 group FIXED** (N-010a flavor boundary 65524; N-010b engine-open gates;
  N-010c date sanity behind `strict_date_sanity`; N-010e exFAT byte-OR reserved form;
  N-010d FIXED-ALREADY via S-011; N-010f registered D-51). **SPC sub-claim corrected
  as a false positive** (u8 field domain — audit correction, no code break).
- **N-034 FIXED** (og deep-scan ladder `fat::lost_boot_flavor` + `probe_fat` rewiring;
  engine-open layering pinned; SPC sub-claim correction shared with N-010b).
- **N-001 duplicate reconciled** (stale copy in MINOR/ removed; FIXED/N-001 canonical).
- PARITY-DELTAS **D-51** added (5 registered deltas: FAT12 engine-open exemption,
  date-gate default OFF, mirror-BAD salvage opt-in, mid-chain FREE/BAD shape,
  underflow guard).
- Suite **366/366** Linux Release (346 + 20 new tests; 6 fail-before discriminators
  proven against the pre-fix source in a worktree).

**Counts after batch 1 (direct file inventory):** ACUTE 0 · SEVERE 8 · MAJOR 22 ·
**MINOR 29** · INFO 12 = **71 active** / **29 FIXED** (FIXED/ holds 29 findings +
RESOLVED-INDEX).

**Batch 2 — NTFS minors (this push):**
- **N-011 FIXED-ALREADY** (emit_ads flag landed with `367561d`; missing parity
  pin + superset guard added this session).
- **N-012 FIXED** ($SI attribute source + 0x7FF mask; 2 fail-before tests).
- **N-024 group FIXED/REGISTERED** (short_name field + session persistence;
  boot ladder = LoadNtfsVolume exactly — 5 fail-before/rescoped tests; stride
  quirk + namespace ranking + error-facility mapping registered; D-52).
- **N-039 FIXED** (0x20000 gate kept — the og's 0x200 mask registered as an og
  defect; >64 MiB $J tail mining; fixture comment corrected; 4 tests).
- Cross-lane green-main fix: the S-012 leg's ASan leak (caller-held
  FileProperties ref never released at the GUID-dump emit) — suite now
  384/384 under ASan detect_leaks=1 too.
- Suite **384/384** Linux Release (+18 tests since session start).
- NOTE: parallel audit legs landed concurrently (S-023..S-028, M-028..M-030+,
  N-040..N-047, I-015..I-018 inventory; their own fixes). The minors-focus
  session continues against the live inventory.

**Batch 3 — HFS+ minors (this push):**
- **N-028 group FIXED/REGISTERED** (sink filter both sources; 'prdf'
  folderization; rsrc superset registered D-53).
- **N-038 FIXED** (ownerFlags 0x20 → rsrc fork + decmpfs completion).
- **N-005 FIXED** (block bound 16 MiB + product sanity; floor registered).
- Suite **392/392** Linux Release (+8 tests; 3 fail-before discriminators
  proven against the pre-batch source).



**Audited commit:** session 1 `67193b6` → session 2 `5d6da5f`/`8574039`/`5f638a3` →
session 3 `2ce2e6b` + in-flight fixes through `34b3441`+rebase → session-3 continuation
through `43149ff` (S3 lead) → fix legs through `ed00020` → session 4 `63a9167`-lineage
rebased to `204137b` on `ed00020` → **session 5 (this leg): `d870dd3`-close + the S-019
landing at `7cf6754`. Corpus:** `03ad46c` (unmoved).
**Audit dates:** 2026-10-25 (sessions 1-3) · 2026-10-26 (session-3 continuation legs +
session 4 + session 5).
**Status:** session 5 = the never-cited-artifact + coverage-hole-closing leg: a systematic
citation-gap sweep (83 cited corpus paths vs the full artifact tree) drove a reading wave
over every never-cited artifact (6 RAWSearchDLL family files, rsf-record4,
ifileproperties-part3, 8 MOM censuses, 9 UIUX files, WinPE pair, config-files) and every
never-cited core file (cr_elevate, cr_destio, cr_snapshot, cr_cached, cr_decomp, cr_err,
cr_diskmap); 24 new findings including the first ACUTE since A-004; OQ-S4-1 RESOLVED (the
grading table dumped byte-exact); OQ-S4-2 RESOLVED-with-residual; corpus errata rows
37-40; S-019's landing verified. Suite 366/366 at the audited tree (evidence:
`evidence/executions/s5-baseline-suite-green.md`).

## Session-5 delta (2026-10-26 — the citation-gap / remaining-surface session)

Mandate: find what ALL prior sessions missed (by systematic inventory, not sampling),
verify the fixed things, go deeper on og-vs-core, mark every remaining feature/function.

- **The gap analysis (new method this session):** extracted every corpus path cited
  across findings+modules (83 unique) and diffed against the artifact tree; same method
  over core src/include basenames. Never-cited in-scope surfaces found and READ IN FULL:
  the 6 RAWSearchDLL family artifacts + per-engine-function-map; SaveScan rsf-record4
  (tree-node grammar) + ifileproperties-part3 (btrfs/zfs/RAW/generic); MOM
  Error/Thread/Common/Config/CheckVersion/Log/bitlockerhelp/FileSystemCheck; UIUX
  31/13/19/06/24/25/20/33/34; WinPE eusendmessage + boot-config; config-files
  drwcfg-bin/userdata-remoteconfig/vendor-logs; core cr_elevate/cr_destio/cr_snapshot/
  cr_cached/cr_decomp/cr_err/cr_diskmap; og-codebase data files (EUUserData, Dict,
  DataFile.ini dispositions).
- **A-005 (ACUTE — first since A-004):** rawfixer 4Kn stack-buffer overflow — the FIX-3
  4Kn offset fix changed read LENGTHS to `sector` but left the 512-byte stack buffers;
  3,584-byte overflow per read on 4Kn devices; CI blind (tests pin 512). Co-located
  M-046: exFAT 512-byte repair plans + LDM GPT-path `*512` hardcodes (4Kn wrong offsets).
- **S-031 (SEVERE):** carved files never marked deleted — og FLAGS=0x0020 constant on
  all carved records; the core's `deleted_only` query and `deleted_count` exclude the
  ENTIRE carve result set (cr_query.cpp:114 / cr_carve.cpp:174-182).
- **S-032 (SEVERE):** archive inner-entry inventories absent — the og returns the
  complete ZIP entry list (presented as archive contents) and stitches multi-part RARs;
  the core emits one flat node per archive.
- **MAJOR wave (6):** M-046 (4Kn LDM/exFAT plans), M-047 (session stats written-then-
  discarded + direct-only refresh vs the og's trusted recursive image), M-048 (EUUserData
  user-signature extensibility — 111 extensions, no counterpart), M-049 (error model:
  false "reproduces the reference convention" claim; og 12-digit documented codes +
  error objects), M-050 (~15 unpersisted og node fields incl. grading bits + hidden
  roots), M-051 (RAW provenance fields: timestamp/checksum/note/anchors).
- **MINOR wave (14):** N-051 MKV DocType, N-052 HEIC relabel, N-053 IIU binding,
  N-054 RAF/JPEG strictness (both directions), N-055 SMART 0xD1 thresholds (health
  disposition CORRECTED — data-layer gap, not presentation), N-056 WinDestIo temp leak,
  N-057 TableLayout::Ebr dead + diskmap test debt, N-058 session load validation deltas,
  N-059 btrfs/zfs session extras, N-060 save progress/cancel, N-061 run-log + session
  auto-naming, N-062 memory monitor, N-063 carve naming invented (OQ-S4-2 resolution +
  a corpus-artifact correction), N-064 Dict password-dictionary unlock aid.
- **INFO registers:** I-021 (og always-elevated manifests — new binary evidence;
  EuSendMessage installer-only; repair toggle; CRecoveryFilter layering; PENetwork SMB;
  DRWCfg out-of-scope), I-022 (core supersets: CRC32/resync/bomb-caps, structured btrfs
  extents, decoded zfs runlists, extra props).
- **OQ resolutions:** OQ-S4-1 RESOLVED (169-entry grading table dumped byte-exact from
  DRWUI.exe .data 0x141bb0500 — char[9] strided, last-dot/uppercase/exact-find
  consumption, `""` + dead-`Raw` quirks; S-023's fix plan step 3 now actionable).
  OQ-S4-2 RESOLVED-with-residual (the +0x48 hook = one-shot per-scan
  "filter_generalraw" config fetch; NO og FileNNNN scheme — corpus deep-dive artifact's
  contrary claim filed as errata row 37; residual = DRWUI merge-rename decode).
- **Fix-verification:** S-019's landing (7cf6754) verified in-tree with its audit trail
  (RESOLVED-INDEX row 27, matrix, module, 3 fail-before tests).
- **Corpus errata rows 37-40:** the File0001 naming claim, the family-roster count
  conflicts (census authoritative), the .rdata→.data grading-table correction, the
  MOM-censuses-are-structural meta-note.
- **Counts at session-5 close (direct file inventory):** **110 active** (1 ACUTE:
  A-005 · 10 SEVERE: S-003, S-007, S-009-partial, S-012, S-020, S-021, S-022, S-023,
  S-031, S-032 · 31 MAJOR: M-002, M-005, M-007, M-008, M-010..M-051 minus 6 in FIXED/ ·
  52 MINOR: N-001..N-064 minus the FIXED/INVALID ones · 16 INFO: I-001..I-022 minus
  I-004/I-005 in FIXED/) · **27 FIXED** (latest: S-019) · 8 tracked-open (2 resolved
  this session: OQ-S4-1, OQ-S4-2-resolved).

**Top-risk update (session 5):** #1 **A-005** (ACUTE — memory corruption on 4Kn; smallest
verified fix: resize 4 buffers + sector-correct plans, one review unit with M-046), then
S-031 (deleted-view excludes all carved files — one-line producer fix + tests), S-032
(archive inventories), S-023 (grading — now unblocked by the table dump), S-020 (ReFS
attribute layer), S-012 (NTFS pass-2), M-047/M-050 (session schema wave — reserve the
grading bits while landing M-047), then M-048 (user signatures), M-049 (error model),
and the M-011..M-015 family.
**Status:** session 4 = the never-audited-modules leg: preview, query, transport audited
for the first time; the bootstrapped-but-never-executed API-session lane executed
(factory contract, driver-client pattern, measured suite/ABI verification); grading
engine decoded end-to-end. 11 new findings; OQ-S2M-1 resolved; M-022 OQ-1/OQ-3
resolved; M-023 counts corrected; corpus errata rows 29-36 filed. Suite 306/306 +
C-ABI check OK at this leg's audited tree (evidence: `evidence/executions/s4-suite-green-and-abi-check.md`).

## Session-3 storage/census parallel-leg integration (2026-10-26)

A second session-3 parallel leg (the *storage/census* leg — raw_signs consumer
decode, OQ-T8 dispatch, lost-partition scoring, ReFS first-class,
CPartitionVirtualVolumeDevice, FileFilter+health, ext4/HFS+/Btrfs/ZFS census)
rebased onto the reconciled main state after the S3-lead/S4 legs landed. Its
**42 locally-filed findings were reconciled against the lead legs' filings**:

- **19 kept, renumbered** past the taken IDs (S-024..S-028, M-031..M-039,
  N-045..N-047, I-017/I-018) — the genuinely-new surfaces: **health grading
  (S-024)**, **ext4 journal reparenting / meta_bg / backup-SB scan /
  dir-parse resilience (S-025..S-027, M-037)**, **Btrfs xattrs (S-028)**,
  **ReFS per-file progress (M-031)**, **the storage/slice layer (M-032
  tail-read semantics, M-033 BitLocker in-stack decrypt, M-034 NAS, N-045/N-046,
  I-017 — first audit of that layer; new `modules/storage.md`, complementing
  `modules/transport.md`)**, **engine-side FileFilter validation (M-035)**,
  **NVMe field coverage (M-036)**, **ZFS SA registry (M-038)**, **cross-engine
  occupancy bitmap (M-039)**, **NVMe route MATCH record (I-018)**.
- **23 dropped as superseded** by the lead legs' filings (which landed first):
  local A-004→S-020; S-012→N-036; S-013→I-012; S-014/S-015/M-014/M-015/I-011→
  M-017 (independent numeric corroboration added as an addendum); S-016→M-023;
  S-022→S-015 (FIXED); M-011/M-013→M-022; M-022→M-020; M-025→N-030 (tail-formula
  + 2 MiB cap + 500 MiB default folded in as an addendum); N-023/N-024→N-026/
  N-037/I-013; N-027/N-028→N-020/N-040/M-029; I-009→M-026 (cross-ref added);
  I-010/I-016→M-017/N-031 (OQ-T9/T8 both resolved by the lead legs);
  I-014/I-015→I-014.
- **New evidence:** `s3-virtual-volume-device.md`, `s3-filefilter-health.md`,
  `s3-rawsigns-consumer-decode.md` (the CommonRawAnalyser loader/matcher/tail
  decode). New tracked-open files: OQ-RS (8 ReFS probes), OQ-Z-E-FLT (ZFS
  dsl-offsets **ACUTE-gated probe**, meta_bg stride, orphan reachability,
  FileFilter blast radius). OQ-S4-1 gains the scorer wiring answer.
- **Convergence highlights:** both S3 legs independently decoded the
  dSuggestted scorer (fits to 3e-5 vs 6 decimals on different log rounds) and
  the dead raw_signs confidence field — strong corroboration for the fix lanes.
- **Baseline note:** this leg's findings were authored against `7fd00e0` (the
  pre-S4 tree); the integration verified each against the reconciled main state
  (through `d374064`). A-002 was live at this leg's baseline and is FIXED on
  main (`7f2c83e`) — this leg's re-verification at `7fd00e0` is retained as
  historical evidence in its worklog entry.

## Counts per severity (active — excludes FIXED/) — post-integration (file inventory)

| Severity | Active | FIXED | Notes |
|---|---|---|---|
| ACUTE | 0 | A-001(p), A-002, A-003, A-004+S-016 | — |
| SEVERE | 13 | S-001/2/4/5/6/8/10/11/13/14/15/17(+18 invalid) | +S-024..S-028 (this integration) |
| MAJOR | 34 | M-001, M-006(in S-009), M-009 | +M-031..M-039 (this integration) |
| MINOR | 41 | N-002/3/10/21 | +N-045..N-047 (this integration) |
| INFORMATIONAL | 16 | — | +I-017/I-018 (this integration) |
| tracked-open | 11 files | OQ-T1/2/5/7/8/9 + S2 OQs resolved | +OQ-RS, OQ-Z-E-FLT (this integration) |
| **Total active** | **104** | **27** | by direct file inventory at `d374064`+this commit |

(The previous section's "61 active / 23 FIXED" reflected the S4 wave-1 close
before the S4 continuation and this integration; the inventory above is the
authoritative post-integration count. **S-023's grading engine + this leg's
S-024 health grading are natural fix-lane companions.**)

## Session-4 delta (2026-10-26 — the never-audited-modules + API-lane session)

Mandate: find what ALL prior sessions missed, remove/verify the fixed things, go
deeper on the og-codebase vs core comparison, mark remaining surfaces as findings.
State at session-4 wave-1 close (direct file inventory, `204137b`):

- **First-time module audits (zero prior coverage — matrix had no rows):**
  - **preview** → M-028 (MAJOR — Caps.max_read bypass on the zip/docx path with
    EXECUTED proof: 65,568 device bytes pulled against a 4,096-byte cap,
    `truncated=false`; plus the "4 MiB corpus preview window"/IInWindowStream
    provenance misattributions — the og's preview cap is 1 MiB and IInWindowStream
    is the 7-Zip LZ interface; compressed extents read whole psize) and I-015
    (INFORMATIONAL — the og's preview is a UI-layer socket protocol: 602-byte header,
    ≤1 MiB BMP, 10 s wait, 100 MB eligibility; the core's in-process reader is the
    deliberate equivalent).
  - **query** → M-029 (MAJOR — beyond M-023's category gap: sort keys (og 6 columns
    incl. Type; core 5, no Type), paging (og virtualized, no page-size constant; core
    offset/limit default 100 — parity-unclaimable, must be recorded deliberate),
    Date/Size bucket IDs (0x100..0x105 / 0x1000..0x1006 incl. Custom + skipped IDs),
    7-group AdvanceFilterCondition, two filter-bar modes, SERVER-SIDE filter push,
    FilterFolder/TxtViewFiles noise filters, 34 RawConfig carve toggles; plus the
    FROZEN `tree --json` contract's `modified` unit lie — docs say Unix seconds, wire
    is FILETIME 100-ns-since-1601, verified in the FAT engine store path).
  - **transport** → M-030 (MAJOR — driver-utilization pattern: no WinPE environment
    gate, no ERROR_INVALID_HANDLE reopen ladder, no terminal IOCTL_VOLUME_OFFLINE in
    the quiesce chain, 32-disk enumeration cap vs og's 200, silent RDA-open fallback
    at the ABI level, and the 64 KiB envelope provenance correction — the og device
    layer chunks at 4 MiB disk / 7 MiB volume buffered-synchronous, NO 64 KiB rule;
    M-009/GAP-S6-13's corpus-grounding narrative corrected to Windows-heuristic) and
    N-041 (dead RdaClient copy-session protocol + volume_letter_to_disk, zero callers).
- **The bootstrapped-but-never-executed API-session lane, executed:** S-023 (SEVERE —
  no post-scan grading engine: the og's CRecoverability "Recovery Chances" model —
  grades {0,1,2} in bits 5-7 of node+0x73, monotonic sticky setter, decision tree
  decoded rule-by-rule, per-selection re-grade, small-result heuristic — has NO core
  counterpart; the core's 5-tier in-walk Confidence is a different decision system;
  RecoveryAssistant statistics (8 counters + 6 category counts) absent; **resolves
  tracked-open OQ-S2M-1**: engines emit grade=1 only, all verdicts are DRWUI-side),
  I-016 (INFORMATIONAL — engine factory contract 9/9 verified: EUCreate pairs, ReFS
  3-arg arity exception, Btrfs NULL-stub, RAW's GetRawUserInfoMap, the host vtable
  interface incl. IAuxiliaryBitmap + cancel-poll; FS-code-space translation warning;
  engine-enable gates note).
- **MINOR findings:** N-040 (ABI doc truthfulness bundle: nonexistent SQLite
  session-index sidecar documented as shipped, disk-map "2 free" state, event-thread
  contract contradiction, unrepresentable ABI sub-versions, frozen-contract unit lie),
  N-042 (CLI health temperature fractional digit is the Kelvin remainder —
  mathematically wrong), N-043 (progress liveness watchdog: og advances coarse
  progress every 5 s on silent engines; core freezes honestly), N-044 (CLI `--deep`
  sets whole-disk discovery, not the deep stage — naming hazard vs the og's
  "Deep Scan" toggle).
- **Resolutions/addenda:** OQ-S2M-1 RESOLVED (→ S-023 evidence); OQ-S2M-2 partial
  (NTFS +30/+50 bonus setters found at NTFSSearchDLL :28804/:28944/:28973/:29002 +
  generic adder FUN_180024990; FAT32-20 setter NOT-FOUND; ZFS/Btrfs negative evidence
  re-confirmed); M-022 OQ-1 (FUN_18000ea30 = $I recycle run-list builder decoded —
  convergent with the S3-REFS leg's decode, which also filed S-020) + OQ-3 (ReFS
  quick-search dot-prefilter = memchr(name,'.') gate, dot-less names skipped,
  kind-flag bypass) RESOLVED; M-023 counts corrected (core = 6 real categories +
  None sentinel / 64 extensions vs og 8 / 1,481 distinct). Three new tracked-open
  OQs: S4-1 (169-entry grading extension table dump), S4-2 (RAW naming-hook
  consumption sites), S4-3 (chunking relative perf — Windows bench leg needed).
- **Verification hygiene:** A-004 + S-016 fixes verified first-hand in-source at the
  session-4 tree (`event_fn_` live-sink at every emit site); suite 306/306 Linux +
  `cr_ui_abi_check` OK executed (evidence file); parallel-leg fixes landing in the
  same window were reconciled by rebase (S-013 `d2d9b03`, S-014 `ed00020` verified
  present in-tree by the rebase read; their own legs' verification records stand).
- **Cross-leg coordination:** this leg's IDs renumbered past the parallel S3/S4 legs'
  S-016..S-022 / M-024..M-027 / N-033..N-039 / I-013/I-014 (my S-017→S-023,
  M-024..M-026→M-028..M-030, N-033..N-037→N-040..N-044, I-013/I-014→I-015/I-016);
  corpus errata rows 29-36 filed after the S3-CARVE leg's 22-28; the parallel legs'
  matrix gained a stale duplicate carve row during their own merge — removed with a
  note in the legend.
- **Counts at session-4 wave-1 close (direct file inventory):** **86 active**
  (9 SEVERE: S-003, S-007, S-009-partial, S-012, S-019..S-023 · 25 MAJOR: M-002,
  M-005, M-007, M-008, M-010..M-030 · 38 MINOR: N-001..N-044 minus 5 in FIXED/ ·
  14 INFO: I-001..I-016 minus I-004/I-005 in FIXED/) · **26 FIXED** (latest: S-013,
  S-014, S-015, S-016, S-017-ftyp, S-011, S-006, N-021).

**Top-risk update (post wave-1):** the roadmap's #1 is now **S-023** (grading engine
absent — the og's headline feature surface has no counterpart; the largest single
missing capability found by any session), then S-020 (ReFS attribute layer — S3-REFS),
S-012 (NTFS pass-2), S-019/S-021/S-022 (the S3 audit legs' SEVEREs), M-028 (preview
caps bypass — executed proof, smallest fix), M-029/M-030 (query/transport model
gaps), then the M-011..M-015 family and M-017 port.



## Session-3 delta (2026-10-25 — the "go deeper, remove the fixed, mark the remainder" session)

Mandate: find what sessions 1-2 missed, verify/retire the fixed things, go deeper on
the og-codebase vs core comparison. State at session-3 close:

- **Retired (verified first-hand):** S-010 FIXED by the fix leg (`3e34ed2`+`414f56c`+
  `34b3441`, already in FIXED/; this session reconciled the stale SUMMARY that still
  listed it as top risk #1); M-009 FIXED (`7fd00e0`, `RwDevice::read_at` now routes
  through `aligned_read_at` — verified in-tree, Resolution added).
- **Fixed MID-SESSION (verified first-hand):** A-002 — `7f2c83e` restored the real
  Microsoft/og LDM GUIDs (`…0434cfb3`/`…714a69ad`; this leg re-extracted them from four
  og binaries independently, converging with the third leg's evidence and the 6-source
  external table) and de-self-referenced the tests. The #1 roadmap item is CLOSED.
  **S-011 was FIXED by the fix session after session-3 close** (tz subtraction
  removed; on-disk time carried through the shared FAT-family datetime core
  exactly like FAT; 3 guards, fail-before proven; suite 325/325 Linux ASan).
- **New findings (7):** S-015 (SEVERE — raw_signs.json min/max are KiB in the og
  (`max << 10`, default 0x7d000 KiB) vs bytes in the core: the 139-format raw-sign
  recall window is 1024x too small, 50 KiB vs 50 MiB); M-022 (ReFS file-value grammar:
  og type@key+0 dual-layout {0x178 old / >=0x29 modern, gated on system-dir-count >= 8}
  + $I recycle handler + all-rows iteration vs core 0x80@key+8 first-match-break);
  M-023 (filter model: 8-cat/1,481-ext runtime XML vs 7-cat/64-ext hard-coded;
  Email+Bookmarks missing — filed from the old unnumbered GAP-cats row); N-030
  (raw_signs confidence field is dead data in the og — parser consumes offset+hex only;
  the core invents confidence semantics); N-031 (carve dispatch deltas: og
  u32→u16→linear→sign-DB order decoded, sector-relative stride, all-zero-sector skip,
  only_common config gate); N-032 (health transports: og companion's CSMI/USB-bridge/
  Realtek-NVMe accessors missing in core); I-012 (GAP-ADS RESOLVED: the og does not
  recover ReFS ADS either — .rsf/RTTI/emission evidence; core MATCH, shared limitation,
  mandate clause satisfied by parity).
- **OQ-T8 RESOLVED** (the decode above; the parallel s2-third-leg's convergent evidence
  block had the u16/sweep legs swapped — corrected with disassembly proof, addendum
  appended to their evidence file). OQ-T9 was resolved by the s2-third-leg (M-017,
  187-row-verified ln(count)/ln(max) scoring model — convergent read this session).
- **Cross-leg coordination:** IDs renumbered past the s2-third-leg's S-012..S-014 /
  N-023..N-029 (this leg's raw-signs findings became S-015/N-030/N-031); corpus errata
  19-21 filed (dispatch order inversion, dead confidence field, KiB units).
- **Architectural verification:** CPartitionVirtualVolumeDevice (the og's virtual-volume
  device layer) = the core's IStorageDevice + slice adapters (equivalent; GAP-S6-8's
  absolute_base covers the base semantics).
- **Coverage added:** ReFSSearchDLL file pipeline (first deep pass in any session),
  RAWSearchDLL raw_signs loader/parser/scan-loop/dispatch (first), FileFilter.xml
  (first), diskhealth companion transports (first), exFAT/FATCalcPartition verified
  convergent via M-017's decode.

## Session 2 + parallel-leg combined delta

Session 2's mandate (find what session 1 missed, continue the audit, remove the fixed
things) landed in two parallel legs, reconciled here:
- **Parallel leg (db7324a/1882847/5f638a3):** 17 new findings (S-010, S-011, M-007,
  M-008, M-009, N-011..N-020, N-021, I-006, I-007), FIXED/ split, RESOLVED-INDEX,
  CORPUS-ERRATA (18 candidates), module-file updates, new open questions
  (OQ-S2A-1..4, OQ-S2C-3).
- **This leg (rebased on top):** independently re-verified every landed fix first-hand
  against the corpus decompiles (S-002 `c9e8860`, S-004 `51ac671`, S-008 `e0175b3`,
  N-002/N-003 `fb44073`, N-010 `cb8dafd`, M-006 `e30b84d`, I-001..I-005 `f74a5dd`),
  re-ran the full suite at `8574039` (313/313 Linux ASan), CLOSED all of them, moved
  S-002 to FIXED/ (the parallel leg's summary listed it open — stale), and filed
  **M-010** (S-005's residual invalid-UTF-8 edge: ext4/Btrfs name bytes destroyed vs og
  raw passthrough; ZFS edge verified parity first-hand via FUN_18001c630).
## ⚠️ THIRD-LEG ADDENDUM (LDM deep-pass leg, session 2) — ~~ACUTE A-002 filed~~ **A-002 FIXED (fix session)**

**A-002 RESOLVED (fix session, this file's top-priority item):** the two corrupted LDM
GPT type-GUID constants were restored byte-for-byte to the real Microsoft
`PARTITION_LDM_METADATA_GUID`/`PARTITION_LDM_DATA_GUID` values (= the pre-`1cb6a43`
bytes, independently re-verified this fix session against the og binaries' own compare
operands: LdmManager.mo DAT_0x1800c4960, Partition.mo DAT_0x180036630/0x36640; full
1,467-file corpus scan: fabricated values in 0 files). The four regression tests were
de-self-referenced (fixtures now hard-code the real bytes / winioctl text forms instead
of copying the constants) and three new guards added
(`Ldm.GptDynamicDiskRealCanonicalGuidsParse`, `Gpt.LdmGuidConstantsMatchMicrosoftWinioctlText`,
`Ldm.GptDynamicDiskFabricatedGuidsRejected`). Fail-before proven: 6/7 guard tests fail
on the fabricated constants; suite 320/320 Linux ASan post-fix. See
`findings/FIXED/A-002-*.md` Resolution. A-002 is CLOSED; the remaining active
ACUTE is session-3's A-004 (event-range config-sink mis-delivery — see the
session-3 delta below), which the fix session takes up next.

**Session-2 third-leg state at close (2026-10-25):** this leg resolved OQ-T2 (FAT mirror
consult — per-entry zero-fallback, N-022 filed), OQ-T5 (exFAT ChecksumDBR — spec skip-set
{106,107,112}, core MATCH), OQ-T7 (RAID-5 rotation — left-symmetric both sides, I-008),
and bounded OQ-T6 (the og's ZFS engine is vdev-type-blind: zero vdev-type/geometry keys
in the binary; fixed 0x1C000 anchor quote-confirmed; id-only vdev registry — the core's
explicit unsupported_vdevs refusal is the honest superset). OQ-T9's CalcPartition
machinery is located (vtable slot 4, Fat32 direct-field fast path) for a future scoring
decode. **A-002 remains the roadmap's #1 item** (two-line constant restore + de-self-
referencing the two regression tests).


**A-002 (ACUTE, fix-induced regression, highest priority in the whole roadmap):** the
A-001 fix `1cb6a43` replaced the two LDM GPT type-GUIDs — which were **byte-for-byte
the real Microsoft `PARTITION_LDM_METADATA_GUID`/`PARTITION_LDM_DATA_GUID`** — with
fabricated values (`5808c8aa-8e9f-4c87-…`/`af9b60a0-384e-4f7d-…`) that appear in no og
binary, no authoritative source (Microsoft Learn `PARTITION_INFORMATION_GPT`, ntdoc,
MagnumDB, phnt, Linux kernel efi.h, Wikipedia all carry the real values), and no real
disk. **On the current tree every real Windows GPT dynamic disk fails LDM discovery
(ERR_BAD_MAGIC) — zero dynamic-volume recovery.** Session-1's A-001 LDM rows were a
false positive (mixed-endian mis-decode; its executed "proof" fixture carried the
fabricated GUIDs, which the pre-fix core correctly rejected); the fix then made the
wrong fixture pass, and the new regression tests are self-referential (they write the
constants into the fixtures). Executed proof: the suite's own
`GptDynamicDiskCanonicalGuidsParse` body with ONLY the entry GUIDs swapped to the real
Microsoft bytes **fails** on the current tree and **passes** with the pre-`1cb6a43`
constants — `findings/ACUTE/A-002-*.md` + `evidence/executions/s2-ldm-real-guid-proof.md`.
**Fix = restore the two pre-`1cb6a43` constant lines + hard-code the real bytes in the
two regression tests (de-self-reference them).**

Also from the same LDM deep pass: **OQ-T7 RESOLVED** (RAID-5 rotation = left-symmetric
on both sides, algebraically identical formulas; the og's other three rotation modes
are dormant dead code — finding **I-008**; reconstruct-on-failure semantics identical);
the GPT-LDM gating question resolved (og gates on the real metadata GUID, PRIVHEAD
candidate at the partition's last LBA, no CRC checks — all matching the core);
**A-001 REOPENED** for its false-positive LDM half (its ESP/MSR/BIOS-boot/Linux half
stands fixed). The invalid-UTF-8 name-fidelity finding this leg drafted (N-010) is
**superseded by M-010** (same substance, filed by the close-out leg — dropped here,
ID retired); this leg's evidence quote `evidence/decompiled-quotes/s2-zfs-utf8-conversion.md`
supports M-010. This leg's dormant-rotations finding is renumbered **I-008** (ID
collision with the parallel leg's I-006 ext4-nanoseconds, cleared).

## Counts per severity (active — excludes FIXED/) — session-3 reconciled

| Severity | Active | FIXED (in `findings/FIXED/`) | IDs (active) |
|---|---|---|---|
| ACUTE | **0** | A-001 (partial — ESP/MSR half stands fixed; LDM half false-positive), A-003, **A-002 (`7f2c83e` — real Microsoft/og GUIDs restored, session-3-verified; tests de-self-referenced)**, **A-004 + S-016 (`c33236f` — live-sink ErrorRange delivery + live snapshot error map)** | — |
| SEVERE | 5 | S-001, S-002, S-004, S-005, S-006, S-008, S-010, **S-011 (tz parity; D-49 + host-independence correction)**, **S-013 (windowed-FAT accessor)**, **S-015 (raw-sign KiB units + per-format tails)**, **S-017 (1cc08c3)**, **S-014 (s4 — live+offline auto-unlock, cr/reghive module)**, **S-019 (7cf6754 — lost BitLocker FVE probe)**, S-018 (INVALID), **S-012 (diff-fix session — two-pass NTFS; D-51)**, **S-022 (diff-fix session — INDX + $INDEX_ROOT second-source harvest)** | S-003, S-007, S-009 (split pending N-020), **S-020, S-021 (S3 audit legs)** |
| MAJOR | 18 | M-001, **M-009 (7fd00e0 — 64 KiB envelope on the repair read path, session-3-verified)** | M-002, M-005, M-007, M-008, M-010, M-011..M-021, **M-022 (S3: ReFS file-value grammar)**, **M-023 (S3: filter model)** |
| MINOR | 26 | N-002, N-003, N-010-theirs, **N-021 (this fix session)** | N-001, N-004..N-009 (grouped), N-011..N-022, N-023..N-029 (S2-third-leg, grouped), **N-030..N-032 (S3)** |
| INFORMATIONAL | 10 | — | I-001..I-011, **I-012 (S3: GAP-ADS resolved — parity, shared limitation)** |
| tracked-open | 6 files | OQ-T1/T2/T5/T7/T8/T9 + all 9 S2-third-leg OQs RESOLVED across legs | OQ-T3*, OQ-T4*, OQ-T6 (bounded), OQ-S2A-1..4, OQ-S2C-3, OQ-S2M-1/2 (see files) |
| **Total active findings** | **95** | **36** | |

*(Session-4 MERGE re-baseline (direct file inventory after integrating the parallel
diff-fix/api/audit legs): 11 SEVERE + 34 MAJOR + 37 MINOR + 16 INFO = 98 active files,
33 FIXED files, 11 tracked-open. This inventory supersedes every earlier count table in
this file — including the wave-1 "86 active" and the mid-session "104 active" figures
(both pre-merge snapshots). Session-4 S-012 was a CONVERGENT dual-agent fix — the landed
implementation (5f1a212 + leak follow-ups) stands; the parallel verification is in the
S-012 Resolution's convergence addendum.*

*(Session-4 continuation: **S-020 (SEVERE) + M-022 (MAJOR) + N-037 (MINOR) FIXED** by
the ReFS attribute-layer landing — the counts move to 10 SEVERE + 33 MAJOR + 36 MINOR
+ 16 INFO = 95 active, 36 FIXED (N-036 half-fixed stays active with its container-map
residual). Fail-before 6/6, suite 402/402 Linux ASan.)*

*(Session-4 continuation 2: **M-026 + M-027 (MAJOR) FIXED** — the ReFS deleted model
(stale-MSB-page sweep) + object identity/recycle routing. Counts: 31 MAJOR active,
38 FIXED. Fail-before 3/4 (the 4th a regression guard), suite 429/429 Linux ASan.
The whole ReFS engine tier is now landed: attribute layer + deleted model + identity
+ recycle; remaining ReFS items are N-036's container map and the OQ-2 real-volume
dump.)*

*(Session-4 continuation 3: **the INFORMATIONAL tier CLOSED** — I-006..I-020 all
ACKNOWLEDGED with first-hand verification of each deliberate-difference rationale
(code/corpus spot-checks recorded per finding; I-018 is a positive MATCH record;
I-012 closes the ReFS ADS question as shared limitation). The INFO tier joins
I-001/I-002/I-003 (acknowledged earlier). No code changes — the mandate's rule for
safety-hardening/superset registrations: preserve the divergences, never "fix"
down to the og.)*

**Note on S-009/M-006:** the S-009 disk-map component was FIXED via GAP-S6-11
(`56cce87` — `cr_diskmap.cpp/.h` verified in-tree by this leg); the M-006 fuzz-CI
component was FIXED (`e30b84d`). The combined finding file stays in `SEVERE/` until the
split tracked in N-020 lands.

**Note on ID collisions (both resolved):** (1) parallel-leg N-010 (FAT BAD sentinel,
FIXED) vs session-2 N-021 (FAT invalid-date) — renumbered to N-021. (2) this leg's
ext4/Btrfs invalid-UTF-8 finding (briefly M-007 in a local commit) renumbered to
**M-010** to avoid colliding with the parallel leg's M-007 (NTFS hard-link fan-out).

## Remediation addition
| 2026-10-26 (diff-fix) | S-012 | **FIXED** | Two-pass NTFS landed: destroy default off (pass-1 membership), GUID-dump pass 2 (claim map built during the walk, 0x400/512 stride, og-15 metafile gate, pseudo-# dedup, destroy on all-run-starts-claimed / <=0x3ff / $-name, folders never), size-0 rescue (mirror resident harvest + post-walk same-name merge, 0x800 skip). 5 tests (4 fail-before proven); suite 339/339 + ASan 339/339; D-51 |
| 2026-10-26 (diff-fix) | S-022 | **FIXED** | INDX + $INDEX_ROOT second-source harvest in the remnant path (og validator/chain/entry gates, in-use only never slack, 8 KiB window overlap, D-N re-decided on resolved evidence). 4 tests (3 fail-before proven); suite 358/358 + ASan 358/358 |

## Counts per module (active findings)

| Module | ACUTE | SEVERE | MAJOR | MINOR | INFO | OPEN | Complete |
|---|---|---|---|---|---|---|---|
| ntfs | – | – | 2 (M-007, M-008) | 3 (N-001, N-011, N-012) | 1 (I-001 ack) | OQ-S2A-1..4 | ✔ |
| fat | – | – | – | 3 (N-004, N-021, N-022) | 1 (I-002 ack) | OQ-T2 **RESOLVED** | ✔ |
| exfat | – | 1 (S-011) | – | – | – | OQ-T5 | ✔ |
| ext4 | – | 1 (S-003 OPEN) | 1 (M-010) | 1 (N-004) | 2 (I-006, I-007) | – | ✔ |
| hfsplus | – | 1 (S-003) | – | 1 (N-005) | 1 (I-007) | – | ✔ |
| btrfs | – | – | 1 (M-010) | 1 (N-006) | 2 (I-003 ack, I-007) | – | ✔ |
| zfs | – | – | – | – | 1 (I-007) | OQ-T6 | ✔ |
| refs | – | – | (project GAP: ADS) | 1 (N-007) | 1 (I-004 ack) | – | ✔ |
| ldm/table | – | – | 1 (M-002) | – | 1 (I-008) | OQ-T7 **RESOLVED (this leg)** | A-002 **FIXED** (fix session) ✔ |
| lost-partition | – | 1 (S-006) | – | – | – | OQ-T9 | ✔ |
| carve | – | 1 (S-007) | 1 (M-005) | 1 (N-008) | – | OQ-T8 | ✔ |
| session | – | – | – | 1 (N-014) | (I-005.7 ack) | – | ✔ |
| writer+report | – | – | – | 3 (N-009b, N-009d, N-013, N-015) | – | – | ✔ |
| health | – | – | – | 1 (N-009a) | – | – | ✔ |
| rawfixer | – | 1 (S-010) | 1 (M-009) | – | – | OQ-S2C-3 | ✔ |
| orchestration | – | – | – | 2 (N-016, N-017) | – | – | ✔ |
| visualization | – | 1 (S-009 partial) | (project GAP: filter cats) | – | – | – | ✔ |
| abi (cr_ui) | – | (S-009 partial) | – | – | – | – | ✔ |
| cli | – | – | – | 1 (N-009c) | – | – | ✔ |
| quality (cross-cutting) | – | – | (M-006 partial) | 3 (N-018, N-019, N-020) | – | – | ✔ |

*(M-010 counts once per affected engine row for navigation, as S-005 did.)*

## Top risks (ranked — fix-session state)

1. **A-004 (ACUTE — event-range fires through a config sink; the only active ACUTE
   post-`7f2c83e`)** — crash-class latent defect on the public API surface; one-line
   fix + two tests per its plan. **← the fix session's next item.** Then **S-015
   (session 3) — raw-sign recall window 1024x too
   small** (raw_signs.json min/max read as bytes vs the og's KiB): every raw-sign
   format caps at 50 KiB (200 KiB for the media entries) where the og carves 50 MiB
   (200 MiB) — the largest single recall gap found in session 3; one-generator fix.
2. ~~**S-011 — exFAT `tz_offset` applied (core) vs ignored (og).**~~ **FIXED**
   this fix session (tz subtraction removed; on-disk time through the shared
   FAT-family datetime core, host-independent on-disk-as-UTC; guards
   TimestampsMatchOgLocalAsUtc / TimestampsZeroTzPassThrough /
   TimestampsMatchExplorerDisplay).
3. **S-003 — HFS+ journal replay missing.** Crashed volumes lose journal-carried recent
   state. Journal replay mechanics decoded first-hand (`b85c084` evidence addendum);
   fix deferred with precise scope.
4. ~~**S-006/S-007 — discovery coverage.**~~ **S-006 FIXED** this fix session
   (ext = og parity; HFS+/Btrfs/ZFS/ReFS superset anchors; FATX scoped
   remainder); **S-007 remains open** — carve misses GIF/BMP/headerless-MP3/
   AAC/RTF/OLE-Office/MPEG/DWG.
5. **M-007/M-008 (session 2, superset) — NTFS hard-link fan-out + `$ATTRIBUTE_LIST`
   multi-hop chase.** Both SUPERSET divergences where the og's behavior is a limitation.
   Opt-in parity flags recommended.
6. **M-010 (this leg) — ext4/Btrfs invalid-UTF-8 names destroyed** (U+FFFD baked; og
   preserves raw bytes; ZFS edge is verified parity). Permanent loss of original name
   bytes for legacy-locale filenames.
7. **M-009 (session 2, perf) — RawFixer bypasses the 64 KiB direct-I/O envelope.**
   ~3.5× slow path on 4Kn NVMe. **FIXED** (`7fd00e0`, verified first-hand `14f3a29`;
   in `findings/FIXED/`).
8. ~~**OQ-T7 — LDM RAID-5 rotation unverified.**~~ **RESOLVED** (left-symmetric both
   sides; og's other rotations dormant — I-008).

## Recommended fix order (first 20 — post-reconciliation)

| # | Finding | Rationale |
|---|---|---|
| 1 | S-010 | safety regression on the repair path; can corrupt volumes during concurrent access |
| ~~2~~ | ~~S-011~~ | **FIXED this fix session** (tz subtraction dropped; 3 guards; suite 325/325) |
| 3 | S-003 | HFS+ journal replay (mechanics decoded; fix scoped) |
| 4 | S-006 | lost-partition probe families (engine wiring only) |
| 5 | S-007 | carve formats GIF/BMP/MP3/AAC/RTF/OLE/MPEG/DWG |
| 6 | M-009 | RawFixer bypasses 64 KiB envelope; one-line fix (route through aligned_read_at) |
| 7 | M-007 | hard-link fan-out superset; opt-in parity flag (mirrors S-001's pattern) |
| 8 | M-008 | `$ATTRIBUTE_LIST` chase scope+depth superset; opt-in parity flag |
| 9 | M-010 | lossless invalid-byte mapping (U+E000+byte) for ext4/Btrfs; ZFS stays U+FFFD |
| 10 | M-002 | EBR torn-chain salvage |
| 11 | M-005 | carve bitmap gating (perf + FP surface) |
| 12 | N-021 | FAT invalid-date yields non-zero FileTime vs og zero; one-line guard |
| 13 | N-013 | dest-overlap optional + first-extent-only; composes with S-010's lock fix |
| 14 | N-014 | stage checkpoint not atomic; atomic-rename fix |
| 15 | N-015 | writer no disk-full pre-check; composes with S-010's lock fix |
| 16 | N-016 | emit_boundary_events double-call bug; one-line removal |
| 17 | N-017 | cancel watcher coverage inconsistent; one-line parallel_for poll |
| 18 | N-018 | UBSan CI lane; mirror the TSan lane |
| 19 | N-019 | 12 missing per-parameter regression tests |
| 20 | N-020 | 6 stale doc claims; split S-009/M-006 finding file; move both to FIXED/ |

## Audit coverage statistics

- **Core read (sessions 1+2):** 54 src .cpp (26,867 LOC), 56 headers, 38 test files
  (313 TEST cases / ~17,730 LOC at `8574039` — re-run green this leg) — agents C+D
  exhaustive; session-2 agents added line-by-line reads of the NTFS mftwalk, all 8
  engine .cpp, query/writer/session/transport/rawfixer/orchestrator.
- **Corpus read (sessions 1+2):** 9 architecture docs, 274 artifact docs, raw decompiled
  trees. Session 2 added first-hand reads of `ghidra-partition/*.c` (RecoveryModule,
  PartitionRecovery, RawFixer) that session 1 did NOT use — these contained the canonical
  FSCTL_LOCK_VOLUME + FSCTL_DISMOUNT_VOLUME sequence (S-010) and the ACL-preservation
  `SetFileSecurityW` calls (N-009d refinement). This leg re-verified first-hand:
  HFSPlusSearchDLL FUN_180009c10 (S-002), EXT4SearchDLL jsb loop (S-004),
  RawFixer FUN_18005de40/FUN_18005df00 (S-008), FATSearchDLL mask-24 + FUN_180041c80
  (N-002/N-010), ZfsSearchDLL FUN_18001c630 (M-010), EXT4/Btrfs import censuses (M-010).
- **Executed verifications:** core suite 313/313 Linux ASan at `8574039` (this leg);
  fail-before/pass-after proofs cited in each fix's Resolution.
- **Not executable here:** og-execution (no Windows/wine) and new Ghidra decompiles —
  all such questions are in `tracked-open/` with precise probes.

## Honesty notes

- Session 1's NTFS B7 "MATCH" verdict for hard-link/ADS/sparse was **REFUTED** by the
  parallel leg's line-by-line decompile read. Three new findings (M-007, M-008, N-011)
  and one minor (N-012) surfaced. The session-1 verdict was based on a misread of the
  LOCK/unlock block as hard-link refcount management.
- Session 1's Q67 "M" (sanitizer cleanliness) was incomplete — UBSan is not enabled
  anywhere (N-018). Session 1's Q65 "M" (test coverage) was aspirational — 12 of 13
  audit-prompt scenarios lack isolated tests (N-019). Session 1's Q68 "M" (documentation
  accuracy) had 6 stale claims (N-020).
- The parallel leg's SUMMARY listed S-002 as open — **stale**: the fix (`c9e8860`) is
  in the `8574039` baseline and was independently re-verified first-hand this leg
  (FUN_180009c10 tree-8/keyLength/recordType/value-offset reads); S-002 now lives in
  `findings/FIXED/`.
- The fixed findings are preserved in `findings/FIXED/` for the audit trail and are NOT
  counted in the active totals. `findings/FIXED/RESOLVED-INDEX.md` catalogues each
  resolved finding's title, fix commit, and resolution summary.
- The 7 ACKNOWLEDGED informational findings (I-001..I-007) are deliberate supersets,
  pinned with regression tests; they stay in `findings/INFORMATIONAL/` because they are
  documented improvements, not bugs.
- The 18 corpus errata candidates are cataloged in `diff-to-fix/CORPUS-ERRATA.md`.

## Remediation log (combined: all legs)

| Date | Finding | Verdict | Evidence |
|---|---|---|---|
| 2026-10-26 (s5) | S-021 + S-003 | **FIXED** | HFS+ journal differential harvest (the og's PRIMARY deleted surface): JournalInfoBlock→jnl_header gates (LE)→ring walk with split reads→leaf images→fork classification→live differential→deleted-tier emission with bitmap grading + N-028a sink filter + extents fallbacks. S-003's "replay" premise corrected (scratch install + unchanged live walk); the fallback salvage tracked (OQ-S003). 4 tests (fail-before 3/4 on the stashed engine); 423/423 ASan; PARITY-DELTAS D-56 |

| 2026-10-26 (S4) | A-005 | **FIXED** (ACUTE) | sector-sized buffers + sector-correct plans; fail-before = the ASan stack-buffer-overflow itself (git-stash proof); 3 4Kn tests; suite 477/477 ASan; D-61 |
| 2026-10-26 (S4) | S-007 (primary) | **FIXED** (niche ~25-format tail scoped in the finding) | 27 registry entries + 9 validators, corpus-exact keys; suppress_overlap + 10-frame gate registered in D-57; fail-before 3/3; suite 436/436 ASan; hostile smoke clean || 2026-10-25 (s4) | S-019 | **FIXED** | probe_bitlocker with the og's exact classifier gate (OEM memcmp -> type 0xB), dispatch before the FAT catch-all; boot-sig/fve-shape scoring (grade-only shape); FsFamily::BitLocker + probe_family branch; marked device-tail size fallback. 3 fail-before/pass-after tests; suite 365/365 ASan. `findings/FIXED/S-019-*.md` |
| 2026-10-25 (s4) | S-014 | **FIXED** | Live path -> Control\FVEAutoUnlock (DRWService :104) + per-GUID Data blobs with og caps (50-char GUID / 256-B blob / 100 records); NEW portable cr/reghive regf parser (offsets decoded from the og's RegLib_decompiled.c) + offline hive path (bitlockerhelp flow, hardcoded ControlSet001 per the corpus) + mounted-volume probe; GUID-match-first selection. 11 Linux + 5 Windows tests + fuzz harness; suite 341/341 ASan. `findings/FIXED/S-014-*.md` |
| 2026-10-25 | A-001 | **FIXED** | canonical GUIDs (`1cb6a43`); in `findings/FIXED/` |
| 2026-10-25 (S3) | A-002 | **FIXED** | `7f2c83e` real Microsoft/og GUIDs restored; tests de-self-referenced + negative guards; session-3 independent og-binary re-extraction converges (4 binaries / 7 sites) |
| 2026-10-25 (S3) | S-010 | **FIXED** | `3e34ed2`+`414f56c`+`34b3441` volume quiesce on the repair write path; 3 fail-before lock tests; SUMMARY stale-row reconciled this session |
| 2026-10-25 (S3) | M-009 | **FIXED** | `7fd00e0` RwDevice::read_at routes through the 64 KiB envelope; verified first-hand this session |
| 2026-10-26 | **M-010** | **FIXED** | lossless invalid-UTF-8 name mapping for ext4/Btrfs (U+E000+byte, bijective, with the inverse); ZFS keeps U+FFFD (the og asymmetry pinned); Text.* + Btrfs.Latin1* + updated NonAscii* tests; suite 462/462 Linux ASan |
| 2026-10-26 | **A-005 (NEW, discovered during S-021 verification)** | **FIXED** | HFS+ B-tree record-span convention inverted vs TN1150/the og's validator — on real volumes only record 0 per node was emitted; for_each_record spans + all fixtures corrected (one builder point); LeafNodeLayoutMatchesOnDiskFormat guard; fail-before: the old reader fails EndToEndWalk + both Decmpfs tests against the corrected fixtures |
| 2026-10-26 | **S-021** | **FIXED** | the journal deleted-record harvest (the og's PRIMARY deleted HFS+ surface): JIB/jhdr gates, ring walk (u16 header test - a decode correction), og node-image validator, fork classification by volume ranges, catalog differential + extents side map, sweep dedup; 4 tests; suite 401/401 Linux ASan; decode corrections recorded in evidence s3-hfsplus-inventory.md §3a + errata E37 |
| 2026-10-26 | **A-005 (NEW, discovered during S-021 verification)** | **FIXED** | HFS+ B-tree record-span convention inverted vs TN1150/the og's validator — on real volumes only record 0 per node was emitted; for_each_record spans + all fixtures corrected (one builder point); LeafNodeLayoutMatchesOnDiskFormat guard; fail-before: the old reader fails EndToEndWalk + both Decmpfs tests against the corrected fixtures |
| 2026-10-26 | **S-021** | **FIXED** | the journal deleted-record harvest (the og's PRIMARY deleted HFS+ surface): JIB/jhdr gates, ring walk (u16 header test - a decode correction), og node-image validator, fork classification by volume ranges, catalog differential + extents side map, sweep dedup; 4 tests; suite 401/401 Linux ASan; decode corrections recorded in evidence s3-hfsplus-inventory.md §3a + errata E37 |
| 2026-10-25 | M-001 | **FIXED** | record-size ladder (`5d6da5f`); in `findings/FIXED/` |
| 2026-10-25 | S-001 | **FIXED** | destroy rule + sparse-run sub-fix (`e93decf`); in `findings/FIXED/` |
| 2026-10-25 | S-002 | **FIXED** | attributes B-tree + decmpfs modes 3/4 (`c9e8860`); independently re-verified this leg; in `findings/FIXED/` |
| 2026-10-25 | S-004 | **FIXED** | ext4 journal every-block salvage + jsb gate (`51ac671`); independently re-verified this leg; in `findings/FIXED/` |
| 2026-10-25 | S-005 | **FIXED** | shared UTF-8 decoder (`2f3237d`); residual edge split to M-010; in `findings/FIXED/` |
| 2026-10-25 | M-009 | **FIXED** | RwDevice::read_at routed through the GAP-S6-13 64 KiB envelope (same aligned_read_at the scan devices use); structural routing parity + envelope unit tests; 4Kn executed numbers deferred to the bench harness (no Windows/4Kn host here - same limitation the finding records) |
| 2026-10-26 | **A-002** | **FIXED** | Real Microsoft LDM GUIDs restored in `cr_table_wire.h` (PARTITION_LDM_METADATA_GUID {5808c8aa-7e8f-42e0-85d2-e1e90434cfb3} / PARTITION_LDM_DATA_GUID {af9b60a0-1431-4f62-bc68-3311714a69ad}); regression tests de-self-referenced to hard-coded real bytes; negative guard (fabricated values → ERR_BAD_MAGIC) + bytes→text guard added; harness + probe log + parity doc corrected; suite 303/303 Linux ASan |
| 2026-10-26 | **A-004 + S-016** | **FIXED** (same delivery path, one commit) | A-004: ErrorRange delivered through the live sink (`event_fn_`), not the config copy (null-call crash / mis-delivery on the documented set_event_sink pattern); S-016 (discovered during A-004's fail-before proof): `tel_counts_` now feeds the live retry-pipe error map into the snapshot before every boundary emit — the ErrorRange event stream was dead code (snapshot error map was merged only after the emitters). Two ScanEvents tests (ErrorRangeGoesToLiveSink, SinkSwitchDeliversAllTypes); suite 305/305 Linux ASan || 2026-10-25 | S-010 | **FIXED** | volume quiesce on the repair write path (IWriteDevice lock contract, execute() lock-before-write + zero-write refusal, Windows RwDevice runs the corpus RecoveryModule LOCK/DeleteMountPoint/Flush/settle/verify/DISMOUNT sequence); finding's destination-lock bullet corrected (corpus: destination open is writability+detection only); 3 fail-before tests, suite 316/316 || 2026-10-26 | S-006 | **FIXED** | lost-partition probe families: ext (og CExtX accept gates decoded + GDT liveness, SB-derived size) + HFS+/Btrfs/ZFS/ReFS superset anchors (D-50; ReFS rescued from the FAT catch-all); 7 tests, 5 fail-before proven; suite 332/332 Linux ASan. FATX = scoped remainder (accept criteria not corpus-decoded); M-017 scoring = own finding |
| 2026-10-26 | S-011 | **FIXED** | exFAT tz subtraction removed — on-disk time through the shared FAT-family datetime core (og parity, host-independent); wire tz fields parsed-but-unapplied (documented in PARITY-DELTAS + AUDIT-FALLBACKS); 3 guards (TimestampsMatchOgLocalAsUtc 0x88 + TimestampsMatchExplorerDisplay 0x20 fail-before/pass-after, TimestampsZeroTzPassThrough sentinel); suite 325/325 Linux ASan |
| 2026-10-25 | S-008 | **FIXED** | RawFixer pre-scan wiring + CheckMFT/CheckFatTable/FixByScanSector (`e0175b3`); independently re-verified this leg; in `findings/FIXED/` |
| 2026-10-26 | **S-015** | **FIXED** | raw_signs min/max KiB units restored (single conversion point in gen_rawsigns.py; zero-max asserted; table regenerated — every format window was 1024x too small, floor 1024x too low); tail walk rebuilt (chunked forward scan, named 8 MiB bound, og full-window range documented); per-format tail scoping (cross-format pairing fabricated close positions — discovered during the fix); 6 fail-before/pass-after tests; suite 310/310 Linux ASan |
| 2026-10-26 | **S-017** | **FIXED** | ftyp-family re-keyed on the box TYPE ("ftyp", header_offset 4 — the box size is data, the pre-fix magic baked literal zeros so NO real MP4/MOV/M4A/HEIC could ever match); match primitive extracted + shared with a NEW F8 self-probe (structural rule: four-zero-byte magic prefixes rejected; candidate-shift assertion); conservative naked-moov salvage (FMT_MOV_NAKED, og MovAnalyzer mirror, tried last); 6 fail-before/pass-after tests; suite 320/320 Linux ASan || 2026-10-25 | S-009 (disk-map half) | **FIXED** | GAP-S6-11 disk-map data producer (`56cce87`); split of the combined file tracked in N-020 |
| 2026-10-26 | **S-018** | **INVALID** (premise refuted) | "zero 0xD9 occurrences in the og analyser bodies" was a grep artifact — Ghidra renders 0xD9 as the signed char -0x27; 5 FF D9 sites exist in the modern JpegAnalyzer and its accept path is EOI-gated (`uVar21 == 0 -> early return`); core validator UNCHANGED; residual (fallback sub-analysers + continuation outcome) tracked-open as OQ-S3-JPEG || 2026-10-25 | M-006 | **FIXED** | fuzz CI re-enabled (`e30b84d`); verified this leg (ci.yml :147-160) |
| 2026-10-26 | **S-013** | **FIXED** | exFAT windowed-FAT accessor (og 1 MiB windows, no cap) as the only fallback — big-FAT volumes never truncate chains to one cluster; whole-FAT fast path kept; fat_window_refills telemetry; fail-before 2/2 (extent coverage 512->1536, second-cluster dir walk); suite 322/322 Linux ASan + hostile-entry smoke || 2026-10-25 | N-002 | **FIXED** | FAT mask-24 EOC family (`fb44073`); independently re-verified this leg; in `findings/FIXED/` |
| 2026-10-25 | N-003 | **FIXED** | exFAT checksum og-parity default (`fb44073`); independently re-verified this leg; in `findings/FIXED/` |
| 2026-10-25 | N-010 (FAT BAD sentinel) | **FIXED** | ordering fix (`cb8dafd`); independently re-verified this leg; in `findings/FIXED/` |
| 2026-10-25 | OQ-T1 | **RESOLVED** | secondary claim map semantics (decompile read); in `findings/FIXED/` |
| 2026-10-25 | I-001..I-005 | **ACKNOWLEDGED** | deliberate supersets verified + pin tests (`f74a5dd`); re-verified this leg |
| 2026-10-25 (S2) | S-010, S-011, M-007, M-008, M-009, N-011..N-020, N-021, I-006, I-007 | **NEW** | parallel session-2 leg, wave 2 |
| 2026-10-25 (S2, this leg) | M-010 | **NEW** | S-005 residual invalid-UTF-8 edge; ZFS-side parity verified first-hand (FUN_18001c630) |
| 2026-10-25 (fix session) | A-002 | **FIXED** | LDM GPT type-GUID constants restored to the real Microsoft values; 4 tests de-self-referenced + 3 new guards; fail-before 6/7, suite 320/320 Linux ASan; session-1 evidence chain corrected (see `findings/FIXED/A-002-*.md`) |

## Session-2 verification log (this leg, baseline `8574039`)

| Finding | Verdict | Independent evidence (first-hand this leg) |
|---|---|---|
| S-002 | **CLOSED** (`c9e8860`) | FUN_180009c10 re-read :7690-7859 (tree-8 search, keyLength `(nameLen+7)*2`, recordType 0x10 gate, value u64@+8); core walk_attributes_tree + kZlib extents in-tree; tests green 313/313 |
| S-004 | **CLOSED** (`51ac671`) | jsb loop re-read :12195-12310 (0xc03b3998 magic, blocktype {3,4}, 1 MiB windows, per-block cancel); core jsb gate + every-block parse in-tree; tests green |
| S-008 | **CLOSED** (`e0175b3`) | FUN_18005de40/FUN_18005df00 re-read :64981/:65013 ('FILE' magic + record-name); core method surface + pre-scan consent wiring in-tree; 16 tests green |
| N-002 | **CLOSED** (`fb44073`) | mask-24 family re-read :8030-8048; core cr_chain.cpp :73-80 raw-entry tolerance; test green |
| N-003 | **CLOSED** (`fb44073`) | validate_entry_sets default false (og parity); D-47 registered; both pins green |
| N-010 | **CLOSED** (`cb8dafd`) | FUN_180041c80 re-read :54124-54137 (0xfff7 → 0xffffff7 → BROKEN); core CHAIN_BAD-before-reserved-range; pin green |
| M-006 | **CLOSED** (`e30b84d`) | fuzz lane re-enabled in ci.yml :147-160 (codeql deferral documented) |
| I-001..I-005 | **ACKNOWLEDGED** (`f74a5dd`) | rationale re-verified; pins UserFileNamedDollarNotHidden / Fat16BadClusterStillCanonical / Btrfs.MirrorSelectionNewestGeneration green; D-12/D-39/D-46/D-47/D-48a-c registered |

---

# Third leg — main-auditor close-out (session 2, 2026-10-25; core `e93decf` audited, tree re-verified at `34b3441`)

The decompile-exhaustive pass session 1 never made (11 parallel read agents + main-auditor
first-hand verification of every ACUTE/SEVERE claim), the 424-file corpus coverage sweep,
and the post-closeout-code audit — re-applied onto this reconciled tree with IDs renumbered
past the two prior legs (**A-004, S-012..S-014, M-011..M-021, N-023..N-029, I-009..I-011**,
OQ-S2M-1/2; one candidate RETRACTED by this leg's own re-read).

## Verification of this tree's state (read-only facts the roadmap needs)

1. **A-002 is still UNFIXED on the current tree** — verified at `34b3441`:
   `include/cr/table/cr_table_wire.h:87-89` still carries the fabricated
   `{5808C8AA-8E9F-4C87-…}`/`{AF9B60A0-384E-4F7D-…}` LDM constants with comments claiming
   `PARTITION_LDM_METADATA_GUID`/`PARTITION_LDM_DATA_GUID` identity (the real Microsoft
   values differ in fields 2-3: `…-7E8F-42E0-85D2-…`/`…-1431-4F62-…`). `23adad1` filed the
   proof and the regression fixture but the constant restore itself never landed (the
   commit touches no source file). **A-002 remains the roadmap's #1 item** — agreed with the
   third-leg close-out, now with a current-tree citation.
2. **M-006 verified FIXED** (`e30b84d`; fuzz job re-enabled, codeql deferral documented).
3. **The `fb44073` N-003 fix introduced a divergence:** its `validate_entry_sets = false`
   default rests on a "verified-absence" premise the decompile refutes (`FUN_18001eba0` =
   the dual-form checksum incl. deleted sets — quoted in the FIXED/N-003 note). Og parity
   = validating; **M-013** carries the correct fix. Recommend scheduling it with the next
   exFAT pass rather than leaving the false default.
4. **S-001's landed destroy rule is mis-scoped** (S-012): og pass 1 emits every live-MFT
   deleted record ungraded/undestroyed; the destroy+viability machinery lives in a
   GUID-dump pass 2 the core does not have. First fix step is one flag flip (restore
   membership parity); the real feature is pass 2.
5. **Empirical:** full Linux suite at `e93decf` → **282/282 PASS** (this leg's own run,
   Release build; `evidence/executions/build-suite-session2.md`). ASan build not runnable
   in this container (memory reaping) — CI carries it.

## Third-leg additions (all active; renumbered)

| Severity | New IDs | One-liners |
|---|---|---|
| ACUTE | **A-004** | ErrorRange events fire through the config sink while the guard checks the live sink (latent null-call crash on the documented `set_event_sink` pattern) |
| SEVERE | **S-012** | NTFS single-pass: og's GUID-dump pass 2 (viability/destroy/0x3ff/blanket-$) + size-0 rescue missing; destroy mis-scoped into the main walk |
| SEVERE | **S-013** | exFAT: FAT > 32 MiB silently dropped → every FAT-chained file/directory truncates to one cluster (~34 GB+ volumes) |
| SEVERE | **S-014** | BitLocker auto-unlock: wrong live registry path (Services vs Control) + offline-hive path absent + unassociated enumeration (F61 — never scored before) |
| MAJOR | **M-011..M-015** | FAT table-size windows · mirror policy (subsumes N-022's sub-case) · exFAT dual-form checksums+oracle (supersedes N-003's premise) · exFAT structural gates · FAT table validation |
| MAJOR | **M-016, M-017** | LDM TOCBLOCK candidates · lost-partition scoring formula (OQ-T9 resolved: ln(count)/ln(max), +500 table-match, count==1 quirk — 187-row-verified) |
| MAJOR | **M-018..M-021** | FilterFolder junk filter · ZFS multi-vdev fallback + fletcher4/DVA-retry · Btrfs chunk profiles + default subvol · carve video-repair hook |
| MINOR | **N-023..N-029** | FAT/exFAT gates · NTFS gates/fields · LDM geometry · ReFS pages · carve/writer · HFS+ sink/forks · U+FFFD residuals (cross-refs M-010) |
| INFO | **I-009..I-011** | carve probing supersets · scope-exclusion/supersets register · corpus errata (8 claims; feeds CORPUS-ERRATA.md) |
| tracked-open | **OQ-S2M-1/2** | NTFS grading locus (D15) · SearchDLL bonus setter / ZFS pool name / Btrfs anchor |

**Retraction (recorded):** the exFAT fixer-checksum "no-skip" candidate — this leg's own
decompile re-read found the skip-table {106,107,112} (`std::find == end()` over the
−1-terminated `DAT_180079a40`); the core's spec-skip is a MATCH, the parallel leg's binary
extraction was right. Corrected in `evidence/decompiled-quotes/fat-exfat-session2.md`.
This leg also corrects its own first A-001 verification note (FIXED/A-001): the parallel
leg's executed proof wins — recorded in place.

**Direction corrections to prior findings (fix plans replaced in-place):** S-002 (og never
decompresses decmpfs — size-from-attributes-tree + raw-rsrc content is the parity target;
landed `c9e8860` should be audited against that boundary) · S-003 (journal =
differential-salvage evidence vs the overlay model — both readings recorded, consumption-
side quotes favor salvage) · S-004 residue (journal **inode-image salvage**, magic 0xf30a —
not covered by `51ac671`).

**Counts (all three legs combined, active — post session-3 A-002 fix):** 1 ACUTE
(A-004) · 9 SEVERE (S-003, S-006, S-007, S-009-remainder, S-011, S-012, S-013,
S-014 + S-010 per leg-3's table) · 17 MAJOR (M-002, M-005, M-007..M-021) · 24 MINOR
files · 11 INFO · 2 new tracked-open files. FIXED/: 13 findings (A-002 added — real
Microsoft LDM GUIDs restored; tests de-self-referenced; negative guard added) + all
OQ-T1..T9 resolutions. **Recommended order now: A-004 (one-line event-sink fix
restoring crash-safety), then S-012 step 1 (one flag flip restoring og deleted-file
membership), then S-011/S-013/S-014, then the leg-3 majors in M-011..M-021 order.**

Read-only compliance held throughout: only `diff-to-fix/` + `worklog.md` written; the
282/282 run was out-of-tree (Release); no source, test, build, workflow, or doc file of
either repository was modified.

---

## Session-3 PARALLEL-LEG addendum (2026-10-26 — S3 lead's wave-1, reconciled onto main)

A second session-3 leg (S3-CARVE / S3-LOST / S3-REFS parallel agents + lead) worked the
same mandate concurrently with the leg above and reconciles here. Its 17 findings were
**renumbered past the IDs that landed on main first**; five were merged as addenda into
this tree's findings (convergent decodes), one was dropped as a duplicate, one as a
superseded constant set. Final disposition:

**New standalone findings (14):**

| ID | Severity | Module | One-line |
|---|---|---|---|
| S-017 | SEVERE | carve | ftyp-family signatures unreachable — MP4/MOV/M4A/HEIC carve dead in core (box SIZE encoded as literal zeros; executed proof, 0 grammar hits for 4 planted real BMFF files) |
| S-018 | SEVERE | carve | JPEG EOI policy: neither og analyser generation requires EOI (size = SOF arithmetic); core requires EOI+SOF+SOS → truncated photos salvaged by og, dropped by core |
| S-019 | SEVERE | lost-partition | BitLocker "-FVE-FS-" lost-partition probe family missing (S-006 under-listed the og; executed string-probe evidence; distinct from S-014's win-layer unlock-path finding) |
| S-020 | SEVERE | refs | ReFS resident/inline content never materialized: FUN_18000e490/ea30 build CRefsResidentDataRunList (lead re-verified first-hand at :10413-10462); core's resident_data never populated + val_len ≥ 0x28 gate drops resident rows → inline files metadata-only. Answers M-022's fix-plan step 4; needs M-022's grammar walk first |
| M-024 | MAJOR | carve | No runtime format-filter surface vs og RawConfig.ini [Filter] (37 toggles, STL/VPJ default-off) — registry rebuilt per search |
| M-025 | MAJOR | carve | Read model: og 8 MiB ping-pong + sector dispatch + pending-CRawFileInfo streaming across chunks; core 1 MiB byte-granularity stateless + extent re-reads. M-021's hook fix depends on this state machine |
| M-026 | MAJOR | refs | ReFS deleted-recovery model: og stale-"MSB+"-page sweep (FUN_180005330) force-marks unreachable-page records deleted; core harvests row-flag 0x4 + ID2 rows — different on-disk premise (CoW medium favors the og's model) |
| M-027 | MAJOR | refs | ReFS object-identity keep-one + Recycle-Bin routing ($R*/S-*/SVI collection, $I resident read) missing; core emits hard-linked objects N times |
| N-033 | MINOR | carve | Analyser registry exports (List/Map/Multimap) consumed by the FS SearchDLLs (registry snapshot clones); no core ABI to enumerate carve formats |
| N-034 | MINOR | lost-partition | FAT lost-boot gate constants: numFATs ∈ {1,2}, FAT12 spf ≤ 200 probe, root_entries×32 % bps — complements N-010b's engine-side gates (same og constants, `src/core/lost/` consumer) |
| N-035 | MINOR | lost-partition | Lost-candidate extents are data-area-anchored in the og (EndSec = DataAreaStart + totalSector) vs core whole-volume — the M-017 containment/16-MiB rules consume different extents |
| N-036 | MINOR | refs | ReFS datarun flags: og requires 0x40 + container-map translation for non-physical runs; core requires 0x10, map never populated |
| N-037 | MINOR | refs | ReFS runlist granularity validation (og discards cluster-inconsistent runlists as FP guard) missing in core |
| I-013 | INFO | refs | ReFS host-OS runtime gate (og refuses < Win8 hosts) — core host-agnostic, deliberate portability superset |

**Merged as addenda (5) + drops (2):** dispatch chain semantics + id-8 continuation
records → **N-031 addendum** (convergent with its decode; adds u16 insertion-order
chains, TTF-vs-MDB u32 pre-emption, TIFF-family same-class relabeling); EUVideoRepair
per-chunk feed + stream-geometry submission → **M-021 addendum**; RTTI proof that
*CalcPartition classes are geometry calculators (slot 4 = sectors-per-cluster, executed
vtable walk) + full AdjustPartitionInfoOrder decode (containment drop, FS-grouped
dedup, 0.1/0.3 tie rules, ±16 MiB promotion, cap 2/4/5/6, stable merge sort) →
**M-017 addendum** (OQ-T9's premise corrected — the classes are NOT scorers; the
DRWUI decode above is the real model, independently confirmed to identical constants);
resident-materialization answer → **M-022 addendum**; byte-granularity pin test →
**I-009 addendum**; lost-probe superset row → **I-010 addendum**. Dropped: this leg's
N-029 (ReFS boot-geometry gate — duplicate of N-026's N-013b, which is more precise)
and its stand-alone I-009/I-010/M-013/M-014/N-023/N-024 (superseded by the merges
above).

**S-007 CORRECTED (decompile re-verified first-hand by this leg's lead at
RAWSearchDLL_decompiled.c:1985 / :2156-2177 / :3097):** the session-3 addendum inside
S-007 carries two wrong rows — `C5 D0 D3 C6` is **CEPSAnalyser (AI/EPS)**, not dBase
(no DBF analyser exists anywhere in the decoded registry), and the LZH row's bytes are
wrong on both counts: `E4 52 5C 7B` is **OneNote** (OneAnalyser, format id 0x31);
LZH is a list-layer structural analyser (LZHAnalyser). The fix plan's DBF validator is
dropped; OneNote magic + list-layer LZH take its place. Errata rows 22-28 filed.

**A-002 status chain (for the audit trail):** this leg independently re-verified the
og GUIDs (re-executed the LdmManager.mo/Partition.mo probe + external cross-check)
while the tree still carried the wrong constants — then found the fix leg had landed
`7f2c83e` mid-session on main; the fix is verified (constants = `5808c8aa-7e8f-42e0-
85d2-e1e90434cfb3` / `af9b60a0-1431-4f62-bc68-3311714a69ad`, tests de-self-referenced
with text-compare + negative guard at test_crc32_gpt.cpp:394-395/:432-433,
test_ldm.cpp:616-617). **A-002 stays CLOSED.**

**OQ-T9 tracking reconciled:** the tracked-open file's stale UNVERIFIED header is
replaced with the full resolution (both legs' decodes merged; residues → S-019/N-034/
N-035 + OQ-S2M-2's bonus setter).

**Counts after this addendum (by direct file inventory):** ACUTE **0** (A-004 now
FIXED — moved by `c33236f`, which also fixed S-016) · SEVERE 9+4 = **13** (S-003,
S-006, S-007, S-009-partial, S-011, S-012, S-013, S-014, S-015 + S-017..S-020) ·
MAJOR 18+4 = **22** · MINOR 27+5 = **32** · INFO 10+1 = **11** · **total active 78**
(20 in FIXED/). Top-risk insertion: **S-017/S-018 join the carve priorities** (media
formats the og recovers and the core cannot — S-017's ftyp bug makes the four most
common video/photo containers uncarvable TODAY), **S-020 joins the ReFS first-class
blockers** (with M-022/M-026/M-027), **S-019 joins S-006's probe wiring** (implement
together).

---

## Session-3 S3-parallel wave-2 addendum (2026-10-26 — engine remainders closed)

The continuation leg's second wave closed the engine remainders that survived both
prior legs' coverage (NTFS attributes/USN/second-source; HFS+ journal consumer;
ext4/HFS+ inventory residues):

| ID | Severity | Module | One-line |
|---|---|---|---|
| S-021 | SEVERE | hfsplus | Journal deleted-record harvester missing (GetDeleteRecord :13637-14252 + emit driver) — the og's PRIMARY deleted surface; core parses journal_info_block and never uses it |
| S-022 | SEVERE | ntfs | Orphaned-record sweep missing: INDX/MFT records outside live $MFT + $INDEX_ROOT $FILE_NAME harvest — second-source names for files whose MFT records are gone; core has NO INDX decode |
| N-038 | MINOR | hfsplus | Fork-selection predicate: og record byte +0x29 bit 0x20 → rsrc fork + decmpfs mark vs core empty-fork substitution (field-name UNVERIFIED; refines N-015b) |
| N-039 | MINOR | ntfs | $UsnJrnl gate: og reason & 0x200 (COMPRESSION_CHANGE) with dead harvest-all flag vs core true FILE_DELETE — core is the correct side; bounds deltas both ways |
| I-014 | INFO | ext4+hfsplus | Supersets registered: deleted-inode sweep; fragmented-journal-walk correctness (og physical+1 is wrong); allocation-grading (UNVERIFIED); hard links/hot files/decmpfs modes 1-2 MATCH-by-omission; ext4 X51 MATCH |

NTFS remainder closed with MATCH verdicts (lead-verified by targeted greps): $EA/
$EA_INFORMATION/volume/$LOGGED_UTILITY_STREAM ignored by BOTH engines (zero type
comparisons in the og decompile; core dispatches 0x10/0x20/0x30/0x80/0xA0 only);
compression engines MATCH (both LZNT1-only; LZX/XPRESS = shared limitation via the
sparse+WOF path). EFS content-grading = UNVERIFIED (no fixture, no code either side).
OQ-T9's residue (the dead agent's USN probe) is dispositioned inside N-039 — its
premise was inverted, probe retained as core-behavior evidence.

**Counts after wave 2 (file inventory):** ACUTE **0** · SEVERE **15** (S-003, S-006,
S-007, S-009-partial, S-011..S-015, S-017..S-022) · MAJOR **22** · MINOR **34** ·
INFO **12** · **total active 83** (20 FIXED). The deleted-file recovery surface now
carries the two largest engine gaps: **S-021** (HFS+ journal harvest) and **S-022**
(NTFS INDX second source) — both are name/metadata-yield gaps on the product's
flagship scenario, ranking with S-012 (NTFS two-pass membership) and S-020 (ReFS
resident content) in the recommended fix order (see the fix session's lane note:
S-012's flag flip first, then the S-021/S-022 substrates, then the S-017/S-018 carve
pair with M-025's state machine).

**Wave-2 post-rebase correction (S3 lead, 2026-10-26):** while wave 2 was in
flight, the parallel fix legs landed `1cc08c3` (**S-017 FIXED** — BMFF entries
re-keyed on the ftyp box TYPE, match_signature extracted, F8 structural self-probe
rejects the all-zero-magic class, FMT_MOV_NAKED salvage, 6 fail-before/pass-after
tests, suite 320/320) and `2adfa29` (**S-018 INVALID** — the JPEG EOI premise was a
grep artifact: Ghidra renders 0xD9 as the signed char `-0x27`, and the modern
analyser's FF D9 sites at :39705/:39719/:39750/:39968/:39989 gate its accept/size
path; the S3 lead independently re-verified the five sites first-hand this session
— the refutation is correct; the legacy-body residual + fallback/continuation
questions are tracked in `tracked-open/OQ-S3-jpeg-fallback-and-continuation.md`;
no code change, correctly). Also landed: S-006 fixed (`abe8e36`), N-021 fixed
(`6fc8c0e`), S-015 fixed (`021e2e6`), and the api-session leg's ABI wave
(`a3fdab4` gate docs + `1f08c20` carve-format registry ops #7-#8 — which addresses
M-024/N-033's ABI surface; verify at the next fix-verification pass).
**Re-baselined counts:** ACUTE 0 · SEVERE **13** (S-003, S-007, S-009-partial,
S-011..S-014, S-019..S-022) · MAJOR 22 · MINOR 34 · INFO 12 · **total active 81**
(22 in FIXED/ — S-017, S-018(invalid), S-006, S-015, N-021 joined the fixed set).

**Final session-3 continuation close-out count correction (S3 lead, by direct file
inventory at `4d138f2`):** S-011's fix also landed mid-flight — the live inventory
is **0 ACUTE · 10 SEVERE (S-003, S-007, S-009-partial, S-012, S-013, S-014, S-019,
S-020, S-021, S-022) · 22 MAJOR · 33 MINOR · 12 INFO = 77 active / 24 FIXED**. The
session-3 continuation's own delta (all legs combined): +28 new findings filed
(S-017..S-022, M-024..M-027, N-033..N-039, I-013, I-014 + the merged addenda),
OQ-T8/OQ-T9/GAP-ADS resolved, 7 findings fixed or dispositioned by the parallel fix
legs in the same window (S-006, S-011, S-015, S-017, S-018-INVALID, N-021, plus the
A-002/A-004/S-016/M-009 retirements earlier in the session), and the remaining
unverifieds are all in tracked-open with precise next artifacts.

*(Session-4 continuation 4: **M-011 + M-012 + M-015 (MAJOR) FIXED** — the FAT
factory trio (table windows / mirror scrutiny + mirror-only / whole-table
validation + copy retry). Counts: 28 MAJOR active, 41 FIXED. Fail-before 2/2 +
3/3 + 2/2; suite 463/463 Linux ASan after five fixture builders gained the
canonical FAT16 media pair (real-volume shape).)*

---

## Fix-session-4 delta (2026-10-26 — A-005 + S-021; the S-012 convergence recorded in the worklog)

**A-005 (ACUTE, NEW — discovered verifying S-021):** the HFS+ B-tree record-span
convention was INVERTED vs the on-disk format (TN1150 records-UP from 14; the og's
journal-image validator FUN_180010720 hard-gates [end-2] == 14 + ascending offsets).
The core's reader and fixtures shared the inversion — on real volumes only record 0
of each leaf node was emitted (the entire HFS+ surface: catalog walk, attributes,
freed-node sweep, decmpfs). Fixed in for_each_record + make_leaf_node (single
fixture-builder point); fail-before proven (old reader + corrected fixtures →
EndToEndWalk/Decmpfs failures). The session-1 corpus artifact's "records grow from
the END backward" note was an inference from the core's own code — corrected as
errata E37.

**S-021 (SEVERE) FIXED:** the journal deleted-record harvest — the og's PRIMARY
deleted surface for HFS+ — implemented from a fresh first-hand decode (three
corrections to the audit's rendering: the u16 header test, the 16-byte entry
layout, the JIB/jhdr field map; all in evidence/decompiled-quotes/
s3-hfsplus-inventory.md §3a). Catalog records pulled from journal transaction
images are emitted as deleted results with the og's differential (absent-key or
different-fileID) + extents side map; the freed-node sweep dedups against the
harvest. 4 tests + the A-005 guard test; full suite **401/401** Linux ASan.

**Counts after this fix (file inventory):** ACUTE **0** · SEVERE **10** (S-003,
S-007, S-009-partial, S-020, S-023..S-028) · MAJOR 22 · MINOR 33 · INFO 12 ·
**total active 77** / **35 FIXED** (7 tracked-open files: OQ-T2/T5, OQ-T6/T7,
OQ-T8/T9, OQ-S2M-1/2, OQ-S3-JPEG, OQ-S012, + the S4-wave OQ files).


---

## Fix-session-4 delta (2026-10-26 — A-005 + S-021; the S-012 convergence recorded in the worklog)

**A-005 (ACUTE, NEW — discovered verifying S-021):** the HFS+ B-tree record-span
convention was INVERTED vs the on-disk format (TN1150 records-UP from 14; the og's
journal-image validator FUN_180010720 hard-gates [end-2] == 14 + ascending offsets).
The core's reader and fixtures shared the inversion — on real volumes only record 0
of each leaf node was emitted (the entire HFS+ surface: catalog walk, attributes,
freed-node sweep, decmpfs). Fixed in for_each_record + make_leaf_node (single
fixture-builder point); fail-before proven (old reader + corrected fixtures →
EndToEndWalk/Decmpfs failures). The session-1 corpus artifact's "records grow from
the END backward" note was an inference from the core's own code — corrected as
errata E37.

**S-021 (SEVERE) FIXED:** the journal deleted-record harvest — the og's PRIMARY
deleted surface for HFS+ — implemented from a fresh first-hand decode (three
corrections to the audit's rendering: the u16 header test, the 16-byte entry
layout, the JIB/jhdr field map; all in evidence/decompiled-quotes/
s3-hfsplus-inventory.md §3a). Catalog records pulled from journal transaction
images are emitted as deleted results with the og's differential (absent-key or
different-fileID) + extents side map; the freed-node sweep dedups against the
harvest. 4 tests + the A-005 guard test; full suite **401/401** Linux ASan.

**Counts after this fix (file inventory):** ACUTE **0** · SEVERE **10** (S-003,
S-007, S-009-partial, S-020, S-023..S-028) · MAJOR 22 · MINOR 33 · INFO 12 ·
**total active 77** / **35 FIXED** (tracked-open: the existing OQ files, now
including OQ-S012).


**Fix-session-4 addendum (M-010):** the invalid-UTF-8 name edge closed — ext4/Btrfs
names now carry a REVERSIBLE encoding of their raw bytes (U+E000 + byte) instead
of baked U+FFFD; ZFS stays replacement (the og's own asymmetry). Suite 462/462.

**ID-collision note (DF4, 2026-10-26):** two A-005s were filed in the same
window by parallel legs — the HFS+ record-span inversion (FIXED, this session)
and the rawfixer 4Kn stack-buffer overflow (open). The rawfixer finding is
renumbered **A-006** (findings/ACUTE/A-006-rawfixer-4kn-stack-buffer-overflow.md);
the hfsplus one keeps A-005 (its identity is baked into the Resolution,
RESOLVED-INDEX, errata E37 and the module docs). Same protocol as the two prior
collisions (recorded above).
- **S-025 FIXED** (ext4 journal reparenting): the og's two-pass model —
  pre-walk harvest (parent map from "."/".." pairs; "."-less blocks stay
  owner-less orphans per S-004), post-walk emit reparenting under live
  parents (record index) or salvaged parents (fixpoint chains); unresolvable
  chains flat with real names (the finding's iNode-naming expectation
  corrected per the corpus: that naming is the sweep's). FILE inodes now
  enter visited_inodes — a LATENT live-file duplicate bug the flat path
  hid, exposed by the claim-interaction test and fixed with it. 3 tests;
  473/473 Release + 473/473 ASan(leaks).

**Live counts (after S-025):** ACUTE 0 · SEVERE 5 · MAJOR 36 ·
MINOR 31 · INFO 18 · tracked-open 12 = **90 active** / **56 files in FIXED/**.

- **S-028 FIXED** (btrfs xattrs): KEY_XATTR(24) dispatch — user.DOSATTRIB
  lands the Windows bits on the node (Samba hex text parsed); other user.*
  xattrs surface as ..xattr/<name> child records with resident values
  (the N-011 ADS shape); btrfs.compression tags the per-file codec property
  (zlib=1/"zlo"=2/zstd=3, the og's exact strings incl. the vendor
  misspelling) as metadata alongside the per-extent truth (I-014 B4).
  3 tests; 484/484 Release.

**Live counts (after S-028):** ACUTE 0 · SEVERE 5 · MAJOR 42 ·
MINOR 44 · INFO 20 · tracked-open 12 = **111 active** / **60 files in FIXED/**.
