# SUMMARY.md — executive summary (sessions 23+25+28 on top; earlier sessions below)

## Session-28 delta (2026-10-26 — DFX28: the rear-first MINOR lane)

Bootstrap: absorbed the window's parallel claims (DFX23 MINOR front-first,
DFX24 MAJOR, DFX25 INFO-closure 22/22, DFX26 MAJOR-tail, DFX27 MAJOR-head)
via union merges; this lane works the open MINOR tier from the REAR
(converging toward DFX23's front-first progress; ALREADY-FIXED
classification on collision). Baseline **737/737** linux-release green at
the claim tree (`354c555` content); corpus `03ad46c` unmoved.

**N-075 FIXED (MINOR — the event-bus contract shape):** the finding's own
"cheapest honest close" executed — `cr_ui_scan_busy` +
`cr_ui_scan_stopping` synchronous query exports (v2-era append, no bump):
the og's `sigIsBusy(bool&)` ×14 / `sigIsStopping(bool&)` ×8 reference-out
query signals surfaced at the ABI boundary (busy = the live run-lifecycle
atomic; stopping = the LIVE token cancel request, persisting after the
one-shot run — distinct from telemetry's post-run cancelled flag). The
pull-polled progress convention registered as PARITY-DELTAS **D-78**
(renumbered from a D-77 collision with the parallel lane's N-009d ACL
register — the errata convention; the og's 38-consumer poll farm vs the
core's coalesced push + any-thread telemetry pull); the typed
`IUIProgressInfo` mapping documented in ABI-CONTRACT §5
(recovery_progress_fn + queue_count + get_stats + report_json —
deliberately no parallel interface). Proof:
`CrUi.ScanBusyStoppingQueries` (mid-run pause-gate window: busy observed
live, stopping flips at cancel, one-shot persistence after return) +
plain-C edges in `cr_ui_abi_check`; fail-before EXECUTED at compile (the
symbols did not exist). Suite **738/738** at the fix commit.

**Post-merge union counts (session-28 close on the merged tree, absorbing
the parallel N-009/N-025/M-057 landings):** SEVERE **0** · MAJOR **14** ·
MINOR **16** · INFO 0 open (22 acknowledged in FIXED/) · **FIXED/ 174
findings** (175 files incl. RESOLVED-INDEX) · tracked-open 11. Next on
this lane: N-019 (the 12-scenario regression-test gap), then
N-050/N-033/N-072 rear-first.


## Session-31 delta (2026-10-26 — DFX31: the tracked-open HEAD lane opens; OQ-S2M-2's three probes RESOLVED)

Baseline 737/737 green at bootstrap (02b950f lineage through the window's
parallel claims); corpus `03ad46c` unmoved. Lane: the tracked-open HEAD set
(OQ-RS, OQ-S2M-2, OQ-S3-JPEG, OQ-S4-3, OQ-S20-1) — disjoint from the
parallel DFX30's tracked-open tail and every finding-tier lane.

**OQ-S2M-2 RESOLVED (all three probes; zero code change — the decode
confirms landed behavior):**
1. **The +0x68 bonus setter located and fully decoded** — per-engine
   scan-path composers, not the serializer: FAT/exFAT carry a two-factor
   ladder (`+0x68 = boot_quality + fs_quality`, each ∈ {10,45,50};
   FATSearchDLL :6946-6969/:7142-7167, exFATSearchDLL :39504-39526; the
   FAT32 fs bits come from FUN_180009960 with the backup-boot verdict from
   the classifier at :8004-8052; the boot pair's bit 0x100 has NO setter in
   the build — its 50-tier is unreachable); NTFS carries the composite
   (flag-weight base FUN_18000dc70 :10538-73 + boot-validation ±FUN_1800097d0
   :7229-33 + MFT-run stitching +30 per fragment / +50 one-shot for
   >3-gap survivals in FUN_180023cb0). The CalcPartition-observed
   "FAT32 20" = **10+10, both ladders at minimum** — no constant exists
   (session-14's zero-hit grep is explained: accumulator latches on
   registers, not immediates). M-017's landed `fs_bonus()` CONFIRMED — the
   flat values are the honest mapping for the core's single-probe
   architecture; the 187-row 6-decimal log parity stands.
2. **The ZFS "pool display name" never exists** — 0 "name"/"pool" hits in
   ZfsSearchDLL (re-verified); DRWUI's FS-name builder FUN_140510080
   (:842335-96) has no case 13 → "Unknow" in the XML `filesystem` field and
   the CalcPartition log; "ZFS partition" (strings:39717) is the GPT
   type-GUID table. The core's label-`name` read registered as a deliberate
   display-information superset (modules/zfs.md).
3. **The Btrfs name-conversion anchor = the DRWUI render layer,
   CP_UTF8/dwFlags=0** — FUN_140163af0 (:264664-89, restore-path invoke
   :357529) + the FUN_1400d9ef0 family (:190981-97); the engine is
   byte-oriented (0 conversion imports, re-verified); S-005 parity holds;
   the predicted residuals N-016a/N-016b already FIXED (N-029/M-010).

Evidence: `evidence/decompiled-quotes/s31-oq-s2m-2-bonus-setter-zfs-name-btrfs-anchor.md`
(20 verbatim quotes, three negative greps re-run). OQ → FIXED/ with
RESOLVED-INDEX row; modules/lost-partition.md + modules/zfs.md updated.
**Counts after this landing: tracked-open 10** (OQ-Z-E-FLT, OQ-RS, OQ-T6-T7,
OQ-S3-JPEG, OQ-S4-3, OQ-S5-1, OQ-S6-1, OQ-S6-2, OQ-S6-3, OQ-S20-1; the
DFX30 tail lane owns the first six of those). Next on this lane: OQ-S3-JPEG
(the fallback/continuation no-EOI decode).

## Session-23 delta (2026-10-26 — DFX23: the MINOR-first closing wave, batch 1 — N-009 group closed + M-057)

Baseline `716d121` verified green FIRST-HAND (linux-release **737/737**,
incl. win_syntax_check) before any change; corpus `03ad46c` unmoved; lane
claim pushed before finding work (protocol). Mandate: MINOR-first queue,
20+ fixes, fixed findings to FIXED/, docs/counts current after every landing.

**N-009 CLOSED (the group's last two halves):**
**N-009c** — the CLI `--filter` ASCII-mangling fixed for real: the new pure
`cr::text::wide_to_utf8` (locale-independent wide→UTF-8; lone surrogate
halves → one U+FFFD per the shared text policy; both wchar_t widths) feeds
all three filter sites (scan print / tree / recover), and the recover path's
match was aligned to the shared ASCII-case-insensitive contract
(`name_contains_ci` factored out of `name_matches` — the query layer's
`name_substring` semantics; the old recover match was case-SENSITIVE, an
internal inconsistency removed). 3 proof tests (`Text.WideToUtf8*` +
`Text.CliFilterUtf8MatchesDecodedNames` — the CJK-filter-matches-decoded-name
byte property, with the old `'?'` polarity pinned to never match).
**N-009d** — resolved by **D-77 registration** (inherit-destination ACL
posture; the finding's own optional-port text; `preserve_dacl` recorded as
the next step, not silently dropped). Finding → FIXED/ (a/b halves were
already fixed in the s2 wave; the group is now fully closed).

**M-057 NEW-FINDING (MAJOR) filed + FIXED (the gate extension's first
catch):** extending `win_syntax_check` to `src/app` (same M-055 rationale —
the app is equally Windows-only) immediately surfaced that **`rrecov.exe`
has been MSVC-uncompilable since `0a68937`** — `cmd_apply` (the S-029 apply
union landing) references `&e` with no `u32 e` declaration. Fixed (the
declaration, matching the sibling cmd_fix/cmd_undo shape) + the gate now
parses all 8 Windows-only files permanently; fail-before EXECUTED (the
exact `'e' was not declared` diagnostic), pass-after 8/8. Q67 buildability
now covers every Windows-only directory (src/win + src/app).

**Post-fix counts (this batch, union with the parallel lanes' INFO
closure; superseded by the session-28 union below — DFX23's N-025 landing
and DFX28's N-075 landing both post-date this paragraph):** SEVERE **0** ·
MAJOR 14 open (M-057 filed+fixed same session) · MINOR **18** · INFO
**0 open** (22 acknowledged — closed into FIXED/ by the parallel DFX25
lane in the same window) · FIXED **173** files (172 findings +
RESOLVED-INDEX — mine +2, theirs +22, overlapping base 149) ·
tracked-open 11.
Suite **740/740** linux-release at the fix commit (+3 Text tests, gate
extended). Next on this lane: N-007/I-004 → N-025 → N-026 → N-027 →
N-031 → N-036 → N-049 → N-059 → N-065 → N-066 → N-075 → N-019 → N-050 →
N-033 → N-072, then the win-layer/crypto-heavy N-032/N-071/N-064.
## Session-25 delta (2026-10-26 — DFX25: the INFORMATIONAL tier closed into FIXED/; tracked-open resolution wave)

Baseline `716d121` verified green first-hand (737/737 linux-release, full
suite); corpus `03ad46c` unmoved. User directive: INFORMATIONAL +
tracked-open first, 20+ resolutions, acknowledged findings moved to
findings/FIXED/, all docs/counts current.

**INFO tier closed (22/22 → FIXED/):** every INFORMATIONAL finding has been
ACKNOWLEDGED since sessions 2-14 (each file carries its own Resolution with
first-hand verification evidence; the DFX14-T5 pass re-verified 22/22 once).
This session re-verified the tier on the current tree before the move:
resolutions present 22/22; the five load-bearing pins executed green
(UserFileNamedDollarNotHidden, Fat16BadClusterStillCanonical,
MirrorSelectionNewestGeneration, NanosecondsPreservedFromExtraField,
ScanCacheBudgetIsTheRegisteredDivergence); two first-hand spot-checks
re-executed (I-001's blanket `$`-skip re-read at NTFSSearchDLL_decompiled.c
:21186; I-022's per-record decompressed-CRC32 re-read at cr_crsf.cpp
:78/:167/:319). All 22 files moved to findings/FIXED/ with RESOLVED-INDEX
rows; the deliberate divergences they register stay preserved with their pin
tests (the INFORMATIONAL safety-hardening rule — no code change).

**Post-move counts (session-25 close, pre-session-28):** SEVERE **0** · MAJOR 14 · MINOR 19 · INFO **0 open**
(22 acknowledged, all in FIXED/) · **FIXED/ 171 files** (170 findings +
RESOLVED-INDEX) · tracked-open 11 (resolution wave below — see the session's
worklog entries for the per-OQ dispositions).


## Session-23 delta, batch 2 (2026-10-26 — DFX29: N-036 — the ReFS container-map half closed)

**N-036 FIXED (the finding's remaining container-translation half; the
recognizer half had landed with S-020):** the invented top-nibble
translate model (feeding a map nothing ever populated) is REPLACED by the
og formula, re-verified first-hand at BOTH decoded consumers
(FUN_180011e90 :13223-13267 + FUN_180004820 :2355-2386, byte-identical):
identity when block < span / map empty / key absent; key
`(((block/span)>>1)-2)*span`; rewrite `(block & (span-1)) | base`; u64
wrap semantics preserved. The span is DBR +0x34 — the og's
volume+0x148 -> parser+0xa8 wiring (FUN_180011cd0) decoded THIS SESSION,
which settles the finding's "container-cluster count" identification —
now `RefsBoot::container_clusters`. The container map is populated from
FUN_180002a60's system-directory records (0xa0/0xe0-length keys with the
{base, span} tail pair; span == DBR+0x34 is the acceptance gate) during
the ObjectID-table walk (zero extra I/O) + a bounded system-table sweep
(IDs < 0x100 — never doubles the user-tree walk). Fail-before EXECUTED:
translate reverted to identity -> `Refs.ContainerRelativeRunTranslated…
PerOgFormula` RED (42*4096 vs 302*4096) AND `Refs.PhysicalRunFlag20…
Untranslated` RED (both files collapsed onto extent 42 by M-027's
placement-signature dedup — the loser never emitted; a genuine
second-order proof). 4 new tests; suite **744/744**. Residual (recorded
in the Resolution, not blocking): the og's record payload carries its key
at +0x20, so an alternative key-relative shape reading exists — the
auditor's reading is implemented with the unit==span gate as the
discriminator; the real multi-container volume probe (the finding's own
open question) settles it and is the bench stage's next step.

**Post-fix counts:** SEVERE **0** · MAJOR 14 · MINOR **17** · INFO 0 open
(22 acknowledged in FIXED/) · tracked-open 11 · FIXED/ **174** files
(173 findings + RESOLVED-INDEX). Next on this lane: N-049 (lost-record
field richness) → N-050 → N-033 → N-064.

## Session-23 delta, batch 3 (2026-10-26 — DFX29: N-049 verified already-landed + the missing pin)

**N-049 FIXED-ALREADY (via M-042's REC_PARTITION_CONTEXT v2, `63a3a77`):**
the finding's full pinned field list — bps, SPC, hidden_sectors, flags
bit0=size_from_device_end — landed with M-042's resume-disk-truth pass
before this session; verified in-source on the current tree (the v2
struct, both save/load sites, v1 back-compat zeroed). The per-hit
crash-safety title half = M-042's per-hit checkpoints + N-014's atomic
rename. The finding's named-but-missing reproduction ADDED:
`ResumeRevalidation.SizeSourceFlagRoundTrip` (flag true round-trips +
the inverse no-false-positive half) — passes on the current tree, the
FIXED-ALREADY proof. The optional capped-evidence list deliberately NOT
persisted (would fight M-042's re-probe-on-resume design — recorded as
reviewed in the Resolution). Suite **754/754**.

**Post-fix counts:** SEVERE **0** · MAJOR 14 · MINOR **13** · INFO 0
open (22 acknowledged in FIXED/) · tracked-open 11 · FIXED/ **178** files
(177 findings + RESOLVED-INDEX). Next on this lane: N-050 (recognition contract) →
N-033 → N-064.

## Session-23 delta, batch 4 (2026-10-26 — DFX29: N-050 — the two-stage recognition contract)

**N-050 FIXED:** the recognition contract is now boot gate -> CONTENT PROOF
-> engine on every family with a corpus-decoded analyser. NEW proofs: exFAT
(the engine's own validate_boot_region — OEM-first so a spoofed OEM fails the
MustBeZero sweep; the structural rc equals the engine's own gate, the
checksum verdict stays informational on both sides, no recovery-negative
strictness) and FAT (root-dir first-sector readability: FAT12/16 fixed root
= reserved+numFats*spf16; FAT32 = reserved+numFats*spf32+(rootCluster-2)*spc
— FUN_180002180's cheap form per the fix plan). NTFS/ext4/UDF content proofs
were already landed (M-043/S-006/M-044). Every proof emits
`recognition: <family> content-verified` on the N-048 logging seam; the
contract is documented as ground-truth-contracts.md §0 (with the honest
HFS+/Btrfs/ZFS/ReFS recognition-gate gap and the exFAT analyser's unpinned
body recorded). Fail-before EXECUTED (2 of 3 new tests RED on the reverted
probe — the third is the positive pin); two pre-existing expectations
CORRECTED toward og parity (spoofed-EXFAT-OEM -> Fat, the og's
multi-analyser outcome; the painted-FAT-boot UDF image -> Unknown — the old
Fat was the spoof-accepting answer). Suite **757/757**.

**Post-fix counts:** SEVERE **0** · MAJOR 14 · MINOR **12** · INFO 0 open ·
tracked-open 10 (OQ-S2M-2 resolved by the parallel DFX31 lane) · FIXED/
**179** files (178 findings + RESOLVED-INDEX). Next on this lane: N-033
(carve capability ABI) -> N-064 (Dict).

<!-- DFX16 note: session 16 ran as a parallel lane in the same window as
     sessions 17-22; its delta is recorded here for chronology. -->

## Session-23 delta (2026-10-26 — DFX23: the MINOR-first close-out lane, N-025 first landing)

Session bootstrap at `716d121` (DFX16-M013 rebase reconciliation; N-062 the
last fix landing); corpus `03ad46c` unmoved. Baseline before any change:
**737/737** linux-release green. Mandate: minor findings first, 20+ fixes,
fixed findings moved to findings/FIXED/, all docs/counts current.

**N-025 FIXED (the LDM geometry group, both sub-items):** N-012a — the og's
1 MiB config-region gate (`config_size < 0x801` else "ConfigSize larger than
1m ?." -> rc 0xfffffff8, LdmManager_decompiled.c:59748-59759, re-verified
first-hand this session) is now first-class in parse_ldm_database; the old
8192-sector read clamp (an unregistered parse-anyway superset) removed as
unreachable. N-012b — a zero interleave defaults to the og's 64 KiB (0x10000
bytes = 0x80 sectors at the 512-LBA grid; ctor default :47665, use-site
re-default :68631-68637, sector fallback 0x80 :68638-68641 — all sites
re-verified first-hand) on BOTH the striped and RAID-5 layout paths; the old
code lost the volume entirely (zero-capacity layout). Tests:
Ldm.OversizeConfigRejected + Ldm.ZeroInterleaveDefaultsTo64K (both
fail-before RED on the stash-isolated pre-fix tree) + the
Ldm.ConfigSizeExactlyOneMebibyteAccepted boundary pin (2048 keeps parsing).
Suite **740/740**.


## Session-16 delta (2026-10-26 — DFX16: S-029 closed — the SEVERE tier is empty)

Baseline `53629a3` verified green first-hand (584/584 linux-gcc ASan);
corpus `03ad46c` unmoved. The session's first act repaired a pushed-red
main inherited from the window's parallel landings: the M-054 producer pin
`FreeMapProducerMatchesSpecBitMapping` asserted the pre-hardening
`cluster_count == 100` while the landed producer (0ba21d8 defect 3 + the
FreeMap contract + the FAT family's 5002 pin) covers 0..101 — the stale
pin corrected to 102u with the audit-honesty note in the M-054 finding
(the convergent DFX17-side correction eeeccbd is in history).

**S-029 FIXED (SEVERE — the lost-partition APPLY path, the recovery
endgame; the last open SEVERE):** a dedicated apply module
(`cr/apply/cr_apply.h/.cpp`) with the rawfixer transaction pattern
extended to the table writer — plan (read-only; refusals: overlap,
table-full, GPT single-entry v1 scope, capacity/32-bit-LBA-range) →
execute (core-carried consent callback default-deny, S-010 quiesce, A6
table-change race guard, CRUNDO01 snapshot, verified write-back, the
corpus device-control sequence per legacy/modern path — 0x7C100 →
sector-0 → 0x70140 → 0x7C010 vs the 0x7C058 0x18-byte record, then the
3x Sleep(1000) refresh tail) → undo (byte-exact + refresh replay). cr_ui
consent ABI (apply_lost_partition/undo_last/save_undo, ABI-since 1.3);
the Windows RwDevice carries the real IOCTL chain (kernel32-only — the
Win7-inclusive import gate untouched). One corpus correction in the
Resolution: GPT_GenerateSignature = the 32-bit disk signature (MBR +440),
not the GPT GUID (4-byte stack slot at all four call sites — PR:32805/
RM:46338/CM:118357/UL:37590). Safety deltas registered D-72a..d. Proof:
13 `LostApply.*` tests (the fix plan's five named proofs + 7 safety
pins), 2 `CrUi.LostApply*` C-ABI tests, the extended plain-C
`ui_abi_check.c` lifecycle; suite **610/610** at the fix commit.

**Post-fix counts (this session's commit, parallel lanes active in the
same window):** SEVERE **0** · MAJOR 28 · MINOR 41 · INFO 22 (all
acknowledged) · **FIXED/ 101 findings** (RESOLVED-INDEX current) ·
tracked-open 16. Parallel-lane landings absorbed this window: M-005,
M-007, M-008, M-045, M-054, N-030 (+ the M-054 pin correction).
Next on this lane: M-043 (NTFS content-proof + backup boot) → M-013/
M-014 (exFAT) → M-018 → M-029 → M-032 → M-035 → M-049 → M-050 → M-052,
then unclaimed MINORs.

**M-013 FIXED (MAJOR — the exFAT dual-form deleted-set checksum + the
reconstruction oracle):** FUN_18001eba0's dual ROL15 checksums (raw +
InUse-restored with the 0x85/0xC0/0xC1 substitution ladder) decoded
decompile-exact and landed as the UNCONDITIONAL deleted-set gate (failed
sets parked, never emitted — the pre-fix core's FP surface closed);
FUN_1800203a0's parked-set oracle (suffix substitution from
secondary_count down to 1, dual re-test) reconstructs torn sets from
parked orphan stream/name runs, re-walking accepted reconstructions
through the standard emission path. LIVE sets keep the flag-gated
downgrade (D-47 scope-corrected; N-003's premise recorded REFUTED).
Fixture truth: deleted sets stamp SetChecksum (the unstamped model
encoded the refuted premise). 2 fail-before-proven tests + the
inverse-asymmetry pin; suite **730/730**. M-043/M-032/M-050 landed by
the parallel lanes in the same window — next on this lane: M-014 →
M-018 → M-029 → M-035 → M-049 → M-052.

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

**N-046 FIXED:** `PartitionSlice::refresh_geometry()` — the og's
RefreshSectorSize-from-parent surface (FUN_140643870,
DRWUI_decompiled.c:1044889-1044895, re-verified first-hand: parent
vcall +0x28, store at +0x38, returns 1 unconditionally) with
revalidation hardening over the og's blind store (misaligned /
shrunken / zero-sector media changes refused, snapshot untouched);
orchestrator auto-refresh excluded and documented in the header (the
corpus exposes no scan-path caller — an invented trigger would be
hallucinated parity). Slice.GeometryRefreshAfterMediaChange +
Slice.GeometryRefreshRefusesInvalidatingChange; fail-before by
construction (the API did not exist). Finding → FIXED/.

**Counts after this landing (direct file inventory at the N-046 push):**
SEVERE 0 · MAJOR 24 · MINOR 35 · INFO 22 (0 actionable) · tracked-open
16 · FIXED 112 files (111 findings + RESOLVED-INDEX). (The hot parallel
window absorbed S-029/S-030/M-008/M-016/M-032/M-043/M-050/N-041/N-057/
N-058/N-067/N-016a + the S-033 INVALID filing; counts are the live
inventory at this commit.)

**N-040 FIXED (D4 closed; D1/D2/D3/D5 verified already-landed):** the
five ABI/doc truthfulness defects re-verified on the current tree —
four had landed via parallel lanes (§8 SQLite REJECTED-BY-REFERENCE,
S-009's disk-map free state, §6's event-thread line, M-029's FILETIME
unit fix with its Query.ModifiedUnitIsFiletimeEverywhere pin). This
session closed D4: the seven fake "ABI-since 1.1/1.2" header
annotations rewritten to truthful era labels; ABI-CONTRACT §9 made
coherent (v2 = M-036's deliberate feature-batch bump — appends still
require no bump; no v1.1/1.2 ever shipped). New pins:
CrUi.AbiVersionAnnotationsAreTruthful (fail-before RED executed on the
stash-isolated pre-fix tree), CrUi.EventThreadContractPinned,
CrUi.SessionSaveWritesCrsfOnly. Suite 656/656. Finding → FIXED/.

**Counts after this landing (direct file inventory at the N-040 push):**
SEVERE 0 · MAJOR 24 · MINOR 34 · INFO 22 (0 actionable) · tracked-open
16 · FIXED 113 files (112 findings + RESOLVED-INDEX).

# SUMMARY.md — executive summary (session 17's MINOR-first wave on top; earlier sessions below)

## Session-17 delta (2026-10-26 — DFX17: MINOR-first close-out wave)

Session start: the union tree at `b8b0318`+ (baseline verified **562/562** +
21/21 at bootstrap; the hot remote's parallel landings — M-054/M-045/M-002/
M-037/N-030/M-007/M-016/M-043/S-030/M-050+N-058/N-057/M-016 — absorbed by
rebase as they arrived, each re-verified). Corpus `03ad46c` unmoved.
Mandate: **MINOR findings first**, 20+ fixes.

**N-067 — CONVERGED (parallel DFX16 lane landed `6795918` while this lane's
implementation sat behind a rebase conflict):** this leg independently
re-verified the landed fix first-hand against the corpus decompile
(exFATSearchDLL_decompiled.c :24440-24452 case -0x7e — the og's 0x82-dentry
shape gates: reserved1@+1..+3 zero, checksum field @+4..+7 FREE (4 bytes),
reserved2@+8..+0xF zero, the bVar21-required u32@+0x10..+0x13 zero via
LAB_18001db06, first_cluster@+0x14 in [2,0xFFFFFFF5], u64 length@+0x18 in
cluster arithmetic). The landed implementation (og shape gates +
chain-read checksum32 stream validation, soft stats, grade-not-reject)
matches the decompile on every axis. This leg's competing implementation
was DROPPED with an honest record: its wire layout (u16 checksum@+2,
u32 length@+0x10) came from a faulty spec recollection and is REFUTED by
the og's own field map (the free zone is exactly +4..+7; +0x10 is
zero-checked, never a length). Lesson registered: the decompile is the
authority for the og's dentry shapes — never trust unverified spec memory
against it.

**Baseline repair 1 (M-054 test expectation):** the M-054 lane's
`FreeMapProducerMatchesSpecBitMapping` shipped `cluster_count 100u`
against the S-009-hardened coverage semantic (2 virtual + 100 data = 102)
and left main red; repaired to 102u by this lane and, in the same window,
by the parallel lanes (eeeccbd + the 8c02a57 dedupe) — first-landing-wins,
all attempts in history.

**Baseline repair 2 (LeakSanitizer red — converged with the parallel
session-22 lane's N-077 PropsRef landing 98b7d13; first-landing-wins, this
leg's equivalent guard dropped) + N-078 NEW-FINDING (filed + fixed):** main leaked three 264-byte `FileProperties` objects from
`deserialize_tree`'s corrupt-record early exits (the M-050/N-058 landing's
hardening paths — five break sites exit between the per-record allocation
and the node attach without releasing the creation ref). Repaired with a
scope guard covering every early exit; the attach path disarms it. The
new five-path regression test (`Session.CorruptRecordEarlyExitsReleaseProps`)
then caught a SECOND defect on one of those paths: the truncated-carve_note
exit broke without setting `c.fail` — a silent partial load violating
N-058's failed-stream rule — filed as **N-078** (renumbered when the parallel lane took N-077 for the
leak half) and fixed (one-line root
cause: the same fail propagation every sibling site already used).
Fail-before EXECUTED on both (LSan red on main; case (4) returned OK=0).
Post-fix: **614/614 + 21/21 UI + ABI check green, LeakSanitizer clean.**
Finding → FIXED/ (straight in, in-session file+fix); RESOLVED-INDEX row.
Their N-077 landing alone does NOT cover this: case (4) of the new test
fails against leak-only trees — the c.fail gap is real and pinned.

**N-063 FIXED (MINOR):** the og's per-analyser carved-file naming. The
MP4/MOV template decoded FIRST-HAND ("Lost Name File (<time>)_EaseUS" —
literals :33312/:33408, the "%04d:%02d:%02d %02d:%02d:%02d" _localtime64
format :33465/:33531, mvhd v0 BE u32@+0x10 / v1 FIRST-FOUR-BYTES of the
u64@+0x14): `CarveResult::embedded_name` carrier with emit precedence
(embedded_name > namer > default), TWO decode corrections recorded (the
field is the MODIFICATION time; there is NO 1904-epoch conversion in the
og — both quirks replicated + pinned by test), the false "corpus policy"
comment repaired, File%06u registered as the deliberate default
(PARITY-DELTAS D-76 with the UTC-formatting divergence). Fail-before 3/3
RED; 679/679 + 24/24. The CZeroEnd-family membership beyond MP4/MOV is
the honestly recorded remainder (decode each of the 22 SetName callers).

**N-056 FIXED (MINOR):** the WinDestIo failed-rename temp leak closed at
both sites the finding's plan named — commit() deletes the staged temp in
the rename-failure path, and close()'s delete gate is the RENAME STATE
(Open::temp_renamed) instead of handle liveness (the old gate provably
never fired after a failed rename: the handle was nulled pre-rename).
WinDestIo.FailedRenameCleansTemp (structural fail-before pinned in-test)
+ WinDestIo.UncShapedPathsFailCleanly (the UNC-family contracts). The
Windows/MSVC lane proves execution (Linux cannot exercise WinDestIo —
the finding's own deferral); Linux suite 684/684 unaffected.

**Counts after this wave (direct file inventory):** SEVERE 0 · MAJOR ~31 ·
MINOR 23 files · INFO 22 (0 actionable) · tracked-open 16 · FIXED 101
finding files + RESOLVED-INDEX. Queue head: N-046 (geometry refresh),
then N-043 (progress liveness watchdog) and the remaining MINOR tier
(N-068/N-065 left to the DFX16 lane's published queue).

# SUMMARY.md — executive summary (session 10's close-out on top; earlier sessions below)

---
**N-074 FIXED (doc-drift group #2, all five):** CAPABILITY-MATRIX LZNT1
row split (DONE with code+test cites; XPRESS/LZX the SHARED limitation
— the og is LZNT1-only too); GAP-S6-11 CLOSED (the disk-map surface
landed; S-009's free overlay noted; host-side rendering the design);
D-43 re-worded to the 256 KiB block reality (D-51/D-72 cross-refs); D-19
rewritten per the DECODED og collision policy (corpus
:357116-357131 re-verified first-hand — `%s_%d`/cap-10000; our
`name (N)`/cap-64 scheme now the REGISTERED divergence, the
atomic/hash/resume layers still registered supersets); cr_health.h's
command list corrected (0xD1 ships since S-024 — kept; 0xD8 never
issued — dropped). New pin: Health.SmartCommandListMatchesImplementation
(fail-before RED executed on the stash-isolated pre-fix tree). Suite
662/662. Finding → FIXED/.

**Counts after this landing (direct file inventory at the N-074 push):**
SEVERE 0 · MAJOR 24 · MINOR 33 · INFO 22 (0 actionable) · tracked-open
16 · FIXED 114 files (113 findings + RESOLVED-INDEX).

**N-043 FIXED (the honest heartbeat — fix-plan option (b)):** new event
type CR_EVT_HEARTBEAT (appended, no version bump) fired from the shim's
watchdog thread every 5000 ms while run() is in flight and no
CR_EVT_SCAN_PROGRESS arrived in the window — the og's 5 s liveness
watchdog (FUN_140577100, DRWUI_decompiled.c:910565-910585, re-verified
first-hand) in the HONEST variant: the payload carries the standing
telemetry truth, never a synthetic permille advance (the og's +0xe8
coarse bump is the registered divergence, ABI-CONTRACT §6).
CrUi.HeartbeatDuringStall (pause-parked scan = deterministic stall;
fail-before RED executed — the run also caught and fixed a test-design
flaw where ASSERT-before-resume leaked the parked scan). TSan 686/686
CLEAN (new thread); both event consumers taught the type; the
EventThreadContractPinned pin caught the contract-text change and moved
with it in the same commit. Suite 686/686. Finding → FIXED/.

**Counts after this landing (direct file inventory at the N-043 push):**
SEVERE 0 · MAJOR 24 · MINOR 32 · INFO 22 (0 actionable) · tracked-open
16 · FIXED 115 files (114 findings + RESOLVED-INDEX).

**N-060 FIXED (save progress + cancel + atomic saves):** the og's save
window (rsf-format flow section: 40 + 50·processed/(2·remaining) with
per-child IUIProgressInfo::IsCancelled teardown, re-verified first-hand)
mapped honestly: per-node `session::SerializeProgress` hook through
serialize_tree (ERR_CANCELLED on false) + save_session pass-through +
the shim's coalesced CR_EVT_SAVE_PROGRESS (9u; the save's own 0..1000
stage_permille, overall stays the standing 1000 — the og's 40→65%
window would regress our monotonic permille; not adopted, registered)
+ the SAVE-SCOPED cr_ui_scan_save_cancel export (the mid-fix
correction: the og's IsCancelled is the save object's state, NOT the
scan flag — save-on-scan-cancel stays intact) + atomic temp-then-rename
saves (shim + CLI — the N-014 discipline; a torn save never destroys
the previous good session). Session.SaveEmitsProgress /
SaveCancelsCleanly + CrUi.SaveProgressEventsAndAtomicCancel; fail-before
by construction. Suite 716/716. Finding → FIXED/.

**Counts after this landing (direct file inventory at the N-060 push):**
SEVERE 0 · MAJOR 24 · MINOR 31 · INFO 22 (0 actionable) · tracked-open
16 · FIXED 116 files (115 findings + RESOLVED-INDEX).

**N-048 FIXED (the engine logging seam — the fix plan's recommended
minimal form):** `cr::log` (levels Error/Warn/Info; a host-registered
sink, default NULL = silent with a zero-overhead fast path; bounded
formatting; serialized synchronous deliveries) + the og's event set as
emission sites: scan START/END brackets (the qa-logs semantics —
extent/fs_hint/stages; rc/all counters/error-ranges/pause-compensated
duration), stage boundaries, committed checkpoints, lost-candidate
resolutions (the CalcPartition.log truth: lba/sectors/fs/score), and
the retry pipe's read-failure exhaust (Error — PartitionRecoveryD's
read-error trail). The CLI's --log file is the host sink (the
EPMLOG.log role). D-75 registers the single-process sink-not-transport
divergence (no shared mapping/ring/async queue — the core has no
second process). Orchestrator.LogSilentByDefault +
LogSinkReceivesScanBrackets — fail-before RED executed (the emission
sites reverted, the seam present: zero lines). Suite 732/732.

**Counts after this landing (direct file inventory at the N-048 push):**
SEVERE 0 · MAJOR 24 · MINOR 30 · INFO 22 (0 actionable) · tracked-open
16 · FIXED 117 files (116 findings + RESOLVED-INDEX).

**N-061 FIXED (both halves):** the persistent run-log is the N-048
seam + the CLI's --log file (the EPMLOG.log role) with the qa-logs
semantic set at the core (START/END brackets with extent/stages and
rc/counters/error-ranges/pause-compensated duration, stage
boundaries, committed checkpoints, candidate resolutions with the
CalcPartition.log scored truth); default-silent — the conservative
read of the channel-gating open question. The session auto-naming
helper: session::auto_session_name_at(label, tm) — byte-exact vs the
corpus sample `auto_Local Disk (I_)10_14_23_13_21` (the forbidden-set
sanitization incl. ':' → '_', LOCAL-time M_D_H_m_s, UTF-8 out);
Session.AutoNamingConvention (fail-before by construction). The CLI's
--out stays explicit (frozen contract — auto-save-on-default is the
og's UI policy, not the library's). Suite 733/733.

**Counts after this landing (direct file inventory at the N-061 push):**
SEVERE 0 · MAJOR 24 · MINOR 29 · INFO 22 (0 actionable) · tracked-open
16 · FIXED 118 files (117 findings + RESOLVED-INDEX).

**N-062 FIXED (the memory monitor on the same seam):** the og's
CMemMonitorManager pressure tracking mapped onto the N-048 cr::log
seam: the pure `check_memory_pressure(threshold_mb, working_set_mb)`
(Warn line with both numbers; OFF at 0 — the conservative library
default; the og's always-on monitor is its UI process's choice) + the
platform sampler in the shim (statm / GetProcessMemoryInfo; a failed
sample never warns) riding the heartbeat watchdog's 5 s cadence (the
watchdog now starts on events OR the armed monitor) +
`cr_ui_scan_config.memory_warn_mb` (appended, zero-init safe). NEVER
coupled to scheduling (the finding's own rule). Orchestrator.
MemoryMonitorEmitsUnderPressure + the HeartbeatDuringStall extension
(fail-before RED executed on the stash-isolated tree). Suite 734/734.

**Counts after this landing (direct file inventory at the N-062 push):**
SEVERE 0 · MAJOR 24 · MINOR 28 · INFO 22 (0 actionable) · tracked-open
16 · FIXED 119 files (118 findings + RESOLVED-INDEX).

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

## Session-21 delta (2026-10-26 — DFX21: the tail-half MAJOR lane opens with M-044)

Baseline the union tree at `b7dd874`-lineage (595/595 linux-release verified
first-hand; corpus `03ad46c` unmoved). Lane: the tail half of the unclaimed
MAJOR tier (queue split against the DFX19 head-half and the DFX20/ex-DFX18
M-033 lane — three same-window lane-ID collisions resolved per the
first-landing-wins protocol, this leg stamped DFX21).

**M-044 FIXED (UDF recognition):** the og's CFatVolumeAnalyser probe
(FUN_180002580 :1413-1435, re-verified first-hand) reproduced exactly in
`probe_family` — boot-bytes fast-reject (jump code short-circuits the probe),
AVDP sector at +0x100 device sectors, Main-VDS sector at u32@(AVDP+0x14),
"UDF"@[0x19..0x1b] -> `FsFamily::Udf` (new value 11, append-only). No
structured engine exists on either side (verified against the factory-map
artifact: the og's 9-engine set has no UDF SearchDLL) — carve-only
disposition, proven end-to-end (auto-probe -> carve recovers a planted JPEG,
quick/deep yield nothing, VolumeInfo.fs_kind == 11 round-trips). cr_ui ABI
fs_hint 11. 4 tests, all fail-before; suite 599/599.

Session counts at this landing: MAJOR open 30 -> 29 (M-044 moved to FIXED/),
FIXED set grows to 99 files in findings/FIXED/.

**M-028 FIXED (preview caps bypass):** the audit's executed bypass
(65,568 device bytes pulled against a 4,096-byte cap with truncated=false
on the docx/zip path) reproduced first-hand as the fail-before, then closed
architecturally: `PreviewReader::read_cost` prices every fetch before the
device is touched (compressed windows cost the whole psize block),
`Window::fetch_any` gates on cost, all four docx reads route through the
new `Window::checked_read` (honest archive/zip degrade + truncated=true),
the over-budget head path propagates truncated, and a post-extraction
device-delta belt guards future parsers. Header provenance corrected (og
preview cap = 1 MiB at the UI socket layer per I-015; IInWindowStream =
7-Zip's LZ input interface; the core's 4 MiB kept as documented superset)
+ the compressed-extent whole-block cost disclosed. 5 tests fail-before;
fuzz tight-budget trap added; suite 662/662. MAJOR open 28.

**M-021 FIXED (carve post-processor hook):** the og's EUVideoRepair binding
(controller+0x158, optional LoadLibrary; the per-candidate deferred poll at
FUN_18000d670 :10130-10136 — both re-verified first-hand) now has its core
equivalent: versioned `CarvePostProcessor` + `CarveFlags::post_processor`
(null = the unbound degrade); fires per validated candidate pre-insertion on
both emit layers; a FEED never a gate (the og's poll never drops a
candidate). The repair engine itself stays scope-excluded
(PARITY-CORE-REC:164) — the hook point is the parity surface. 2 tests
(hook-invoked fail-before-proven by wiring revert; degrade-when-absent);
suite 685/685. MAJOR open 27.

## Session-19 delta (2026-10-26 — DFX19: MAJOR-first lane, first landing M-016)

Bootstrap `ff734d7` (union of the parallel lanes' claims: DFX18 on the MINOR
tier, this leg on the unclaimed MAJOR batch); baseline suite **594/594**
Linux RelWithDebInfo + ASan. Corpus `03ad46c` unmoved.

**M-017 FIXED (fourth landing):** lost-partition scoring — the core's
absolute structural points are replaced by the og's content model:
every surviving candidate is materialized and walked by its engine
(progress-budgeted), the count is weighted per FS (f+50·d NTFS /
f+10·d HFS+ / (f+d)·50 FAT) plus the observed bonus (NTFS 60/30, FAT
20) plus the +500 table-match — the old model's overlap penalties were
the polarity INVERSION of exactly this bonus. Score = ln(count)/
ln(max) in permille with the og's edge cases (count<=1 identity
quirk); structural points tie-break. AdjustPartitionInfoOrder ported:
containment-in-known drop (table-matched candidates exempt), stable
sort, and the :842776-93 size-match promotion re-decoded first-hand
(the iVar16 counter — an adjacent matcher takes the break path the
naive reading misses). 4 fail-before tests incl. the CalcPartition
log-row parity test. Suite 691/691. findings/FIXED/M-017; matrix
lost-partition A30 MATCH.

**M-038 FIXED (third landing):** ZFS SA registry-driven decode — the
core's SA decode was layout-1-hardcoded (`layout != 1 -> nullptr`,
dropping every object with a registered non-1 layout). Now: the og's
three-object chain (master "SA_ATTRS" -> {REGISTRY, LAYOUTS},
:22934-23140), by-name attribute resolution (ZPL_PARENT :24324 family),
offsets by cumulative registered lengths over each layout's slot order,
and the og's zero-attrs surface for unregistered layouts (:17636 — the
object surfaces, attrs zeroed — where the core dropped the file
entirely). Layout 1 keeps the implicit-default fallback for
registry-less pools. Fixtures now carry the real registry chain (a 2K
fzap registry — 14 entries don't fit 512B — plus a layouts fzap with
the 14-slot layout 1 and a differently-ordered 4-slot layout 2, decoded
by the new registryfile object). ZapEntry gained raw_value (fzap arrays
exceed 8 bytes). 2 fail-before tests. Suite 687/687. findings/FIXED/
M-038; matrix zfs B11 MATCH.

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
# (session-20 delta below — the session-22 file header above is current)

## Session-20 delta (2026-10-26 — DFX20: M-033, the BitLocker in-stack decrypt)

Leg ID history: stamped DFX17 at bootstrap, re-stamped DFX18 then DFX20
(two parallel legs squatted each name in the same window; the union kept
all three bootstrap entries verbatim). Bootstrap baseline **584/584**
linux-gcc (RelWithDebInfo + ASan) at `5d63124`; after absorbing the
parallel lanes' landings (M-007, M-008, M-045, M-054, M-005, N-030, N-041,
N-069) the pre-fix baseline re-verified **608/608** (the count grew with
the absorbed waves). Corpus `03ad46c` unmoved. Lane claim: the UNCLAIMED
MAJOR tier (M-033 first), posted before any finding work.

**M-033 FIXED (the in-stack BitLocker decrypt — CONFIRMED-DIVERGENCE,
verified against the corpus first-hand):** the og decrypts locked volumes
in-stack (architecture doc §8: "FVE keys -> XTS-AES decrypt
(BitLockerLib/OpenSSL)"), engines see plaintext; our core refused locked
volumes unless the OS had them unlocked. The fix lands the layer exactly:
`DecryptingDevice : IStorageDevice` (cr_decrypt_device.h) wraps the volume
slice with per-sector LE-index XTS tweaks — physical -> slice -> [XTS]
-> engine, the og's order. AES-128/256 + XTS are clean-room
(cr_aes.cpp, no new third-party dependency — the tree stays
zlib/lz4/zstd-only and Win7-inclusive) and pinned **byte-exact against
vectors generated independently by pycryptodome AND OpenSSL EVP 3.5**
(the EVP interface the og's BitLockerLib statically links) plus the
FIPS-197 Appendix C KATs; XTS weak-key halves are rejected (OpenSSL 3.x's
own rule). Key surface = the og's own API shape: the user supplies the
FVEK (GetFullVolumeEncryptionKey's >=0x40-byte output, decompile
:27075-27107) via `scan --volume X: --bitlocker-fvek <hex>` or the
additive ABI export `cr_ui_open_volume_with_key` (no version bump —
the documented additive policy). **Safety posture preserved**: locked
WITHOUT a key keeps the ERR_FVE_LOCKED refusal (S61
legitimate-unlock-only; FB-B01 never-scan-blind); auto-unlock-able
volumes still take the OS path; a wrong key yields honest detection
failure, never wrong-file results. The recovery-password/BEK -> VMK ->
FVEK unwrap chain is out-of-corpus (ParserBitLocker :27633's datum parse
sits behind opaque C++ graphs — FUN_180042350/FUN_180044e90; the
finding's own open question says so, confirmed first-hand) — filed as
tracked-open **OQ-S20-1** with the exact decompile entry points and next
step. Fail-before intrinsic (the device/crypto types did not exist);
13 tests in test_bitlocker_xts.cpp prove AES KATs, XTS cross-impl
vectors, whole-sector vectors, round trips, weak-key rejection, tweak
separation, plaintext reads through the wrapper, byte-exact
unaligned/cross-sector reads, transport-error propagation, and the
M-033 acceptance (an XTS-encrypted FAT16 volume is opaque to the raw
engine and scans correctly through the decrypt device with the key).
Suite 608/608 at the M-033 landing; **685/685** after M-042.

**M-042 FIXED (the resume disk-truth pass):** CONFIRMED-DIVERGENCE on all
three falsifiable deltas (blind restore, no partinfo equivalent,
config-gated candidate persistence, thin per-record payload), fixed per
the finding's own 5-step plan. lost::revalidate_candidate re-runs the
SAME per-family probe the discovery uses, at the persisted start LBA,
through the CURRENT disk, at both device-bearing surfaces (the listing
loads gained revalidate_disk - cmd_recover now opens the source disk
BEFORE the session load; cmd_apply's candidate listing is re-validated on
the once-opened view it reuses for plan_apply - and run() start for the
resume flows, whole-disk basis only). REC_PARTITION_CONTEXT v2
(append-only, length-versioned like SessionState v2) carries
bps/SPC/hidden/flags; REC_DISK_LAYOUT (type 8) is the partinfo.dat
equivalent with the layout_changed signal surfaced at resume (CLI warns;
never enforced); a checkpoint is FORCED the moment discovery produces
candidates (sink-gated - the no-.dat-side-channel residue registered, not
papered over). **Executed fail-before**: the re-validation gates disabled
-> ResumeDropsStaleCandidateAfterRelayout RED on both surfaces; restored
-> green. 6 tests in test_resume_revalidation.cpp; suite 685/685.

**Counts after this push (direct file inventory):** SEVERE **1** (S-029,
DFX10's claim) · MAJOR **26** (M-005/M-007/M-008/M-037/M-045/M-046/
M-054 landed by parallel lanes since session-15's count; M-033 + M-042 this push)
· MINOR **41** (parallel lanes landed N-041/N-069/N-030 since the
session-15 count) · INFO 22 (all ACKNOWLEDGED) · FIXED/ family
**103 files** (incl. RESOLVED-INDEX; +M-033 +M-042 this push; the parallel waves'
M-005/M-007/M-008/M-045/M-054/N-041/N-069/N-030 already inside) ·
tracked-open **17** (+OQ-S20-1 this push). Top remaining risks: S-029 (the last SEVERE), then
the MAJOR tier by user impact — M-042 (session-resume revalidation),
M-017 (lost-partition scoring), M-019/M-020/M-038 (ZFS/btrfs parity
depth), M-039 (occupancy bitmap) are this lane's next queue.
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

**Session-11 landing 3 — N-008 FIXED (ZIP padding + ZIP64 chain; gate half via
N-068):** the salvage extension is now the og's (:18492-18508) — data-bearing
trailing sectors absorbed sector-strided after a BROKEN chain (cap 0x500001,
device sector size, 4Kn-aware), while the INVENTED zero-absorption after
clean EOCDs is REMOVED (intact ZIPs are never inflated with zero slack —
N-027d's settled reading; the old `ZipZeroPaddingAbsorbed` pin rewritten to
the correct `ZipCleanArchiveNotInflatedByZeroSlack`). ZIP64 indexless
archives no longer truncate: the 0xFFFFFFFF csize sentinel resolves through
the LFH extra field's 0x0001 record. The "admit 0x62" item is REFUTED at
the dispatch layer (the probe tier's ≤0x13||==0x62 is a pre-filter; the
dispatch's ≤10 is the final word — 0x62 > 10). Tests: ZipSalvageAbsorbsData
Sectors + ZipCleanArchiveNotInflatedByZeroSlack + Zip64ExtraFieldContinues
Chain. Suite 685/685 ASan; 55/55 UBSan carve. Counts: MINOR 28 · FIXED 85
(the parallel lanes' N-040/N-046/N-057/N-058/N-077/M-002/M-007/M-044/M-047/
M-050/M-051/M-054/M-056 landings share the window).

**Session-11 close-out (DFX16 leg):** this leg's verified landings —
**N-069** (CAB dual-key), **N-067** (exFAT upcase integrity), **N-068**
(ZIP LFH gates; EOCD half FIXED-ALREADY by the CD anchor), **N-008** (ZIP
salvage extension + clean-archive honesty + ZIP64 chain), **S-033**
(NEW-FINDING filed during N-067 verification and REFUTED the same session
by the og's own decompile walk — INVALID with corpus errata E43; the
exFAT type constants were RIGHT all along), plus the hygiene moves
(N-013/N-015/N-018/N-042 → FIXED/), corpus erratum E44 (the ZIP
entry-point table's labels/bounds), the 5aaf044 "style"-clobber
root-cause + the convergent N-077 leak repair, and multiple union-tree
repairs during the heavily-parallel window. **Final counts at this leg's
close (direct file inventory): SEVERE 0 · MAJOR 18 · MINOR 29 (27 fully
unresolved + N-009/N-036 partials) · INFO 22 (all ACKNOWLEDGED) · FIXED
135 finding files · tracked-open 11.** Suite 691/691 linux-gcc ASan +
640/640 linux-ubsan on the closing tree. No release tag: the audit is
not closed (the MAJOR tier and the session/health/log minor families
remain — see the worklog handoff for the lane map).

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

**N-026 FIXED (the ReFS page/gate group — mixed resolution, 2 fixes + 2 audit
corrections):** N-013b — the DBR geometry gates now match the og exactly: bps
ladder {256,512,1024,2048,4096} (FUN_180012390:13544-13546, re-verified
verbatim) and the cluster product gate {4 KiB, 64 KiB} (:13547-13549); the old
core REFUSED 256/1024/2048-bps volumes the og opens and ACCEPTED 16/32-KiB
cluster shapes the og refuses — both directions closed (the 1.x-3.x version
superset stays: I-004/D-48b). N-013d — the volume open reads the og's 4 KiB
(:2425-2444). N-013a INVALID (first-hand decode: the og's "dynamic body
offset" is 0x50 BY CONSTRUCTION — the ref parser hardcodes the 0x30 extension,
FUN_18000ff60:11875; the audit's `*(uint*)(page+4)` was Ghidra's
parsed-object view, not a raw-page field). N-013c INVALID (the "mid-volume
second copy" is a misattribution: the cited formula is the container-map key
in FUN_180011e90 — N-036's decode, confirmed by the full-function read; the
og reads the superblock at the SINGLE metadata base spc×0x1E, :2457, and
probes no backup; the core's {30,n-3,n-2} set stays the superset —
implementing the audit's "mid-volume probe" would have been hallucinated
parity). Tests: Refs.Bps256Accepted / Bps1024Accepted / Cluster16KRejected /
OpenReadIsFourKiB (all fail-before RED) + Cluster64KAccepted boundary pin.
Suite **748/748**.

