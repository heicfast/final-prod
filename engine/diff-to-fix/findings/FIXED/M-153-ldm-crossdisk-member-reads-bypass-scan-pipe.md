# MAJOR M-153 — cross-disk LDM member reads bypass the retry/cache substrate during scans: the platform resolvers hand raw `win::open_disk` devices to the LDM volume engine walk, where every other scan read rides the pipe (single-disk LDM included) and the og's engines always read through CStorageDeviceWithCache

**Severity:** MAJOR (the M-058/M-136 class on the scan side: transient-error robustness during LDM volume scans — the exact damaged-disk population retries exist for; plus a core-internal inconsistency: single-disk LDM scans ARE cached, multi-disk are not, and the recovery side pipes the same members)
**Module:** orchestrator (the LDM engine-pass block) / storage (the platform LDM resolvers) / cli / abi
**Parameters affected:** R51-adjacent (the scan read pipeline — the candidate family's member reads), #47/#48-adjacent (throughput — the member reads also lose the cache), B1-adjacent (detection correctness under transient errors)
**Corpus references:** `re-workspace/artifacts/SearchDLL/ntfs-engine--deep-dive.md` §8 ("the volume reads go through the device abstraction (CStorageDeviceWithCache → IStorageDevice), which for damaged/locked volumes is the `\\.\EPRDTDRV\{d}\{p}` EPMNtDrv path … with a **read cache** (CStorageDeviceWithCache) in front to survive MFT re-reads economically" — the ENGINE's own device contract, any volume it scans); `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:919095` (the scan driver constructs CStorageDeviceWithCache around the scan device — the 0x400000/sector budget decode, D-72's own cite); `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c:7167` (the engine's `__RTDynamicCast(device, IStorageDevice, CStorageDeviceWithCache)` — the engines EXPECT the cached class and configure against it)
**Core references (current tree `9f3a3cb9`):** `src/core/orchestrator/cr_scan.cpp:1843-1876` (the LDM block: `parse_ldm_database(slice, …)` — piped, the database parse is fine — then `scan_candidate(ldm_devices_.back().get(), …)` at `:1875` where the LdmVolumeDevice reads members through the resolver); the local fallback resolver `:1851-1856` returns `slice` = **the pipe** (single-disk LDM members ARE cached); the platform resolvers hand RAW devices: `src/app/rrecov.cpp:1648` and `src/ui/cr_ui.cpp:2123` (both `(*guid_map)[g] = dev.get()` over bare `win::open_disk` members); the recovery side is ALREADY piped — `src/ui/cr_ui.cpp`'s `session_arm_ldm_basis` wraps each member in its own `build_read_pipe` chain (the d28244df landing's LDM supplement), so scan and export DISAGREE about the readability of the same member disk (the M-136 falsifiable class, scan-side)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Every scan read rides the M-058 pipe (the primary volume, the partition-table targets, the lost-partition candidates, the carve stage, and — via the local fallback resolver — single-disk LDM members), EXCEPT the cross-disk LDM case: when `cfg_.ldm_resolver` is the platform registry (multi-disk dynamic disks), the LdmVolumeDevice's member reads hit raw `win::open_disk` devices with no retry layer and no cache. A transient read error on a member during the LDM volume's engine walk (a hiccup the primary volume's walk would have retried past) fails the engine read outright — files lost from that candidate. The recovery side pipes the same members (d28244df), so the same volume can recover a file at export that its own scan failed to find. The og's engines receive CStorageDeviceWithCache-wrapped devices for every volume they scan (the RTTI check at NTFSSearchDLL:7167 is the engine asserting the cached class).

## What the reference does

The og's scan driver constructs CStorageDeviceWithCache around the scan device (DRWUI:919095, the 4 MiB/sector-budget reader) and hands it to the engine DLLs; the NTFS engine's init RTTI-casts its device to the cached class and configures against it (NTFSSearchDLL:7167 → FUN_18000fae0). The deep-dive's §8 states the contract generally — "the volume reads go through the device abstraction (CStorageDeviceWithCache → IStorageDevice)" — for the engine, hence for every volume the engine scans, LDM volumes included (the og's LDM volumes are served to the engines through the same CPartitionVirtualVolumeDevice family, M-111's trace).

## What our core does

- The LDM block parses the database through the piped slice (fine), constructs each LdmVolumeDevice, and scans it inline (`:1875`) — the volume device's member reads go through the resolver.
- The local fallback resolver (`:1851-1856`) returns the pipe itself — single-disk LDM members read cached+retried.
- The platform resolvers (`rrecov.cpp:1648`, `cr_ui.cpp:2123`) return raw `win::open_disk` devices — multi-disk LDM members read bare: no retry (one transient error fails the engine read), no cache (every MFT/FAT re-read pays the full device round trip).
- The recovery side's `session_arm_ldm_basis` (the d28244df landing) wraps each member in `build_read_pipe` — the same member disk is piped at export and raw at scan.

## Gap

Falsifiable: a two-disk spanned LDM volume whose second member has a transient-fault sector (fails the first read, serves the second) — the scan's engine walk over the volume loses every file whose extents cross the fault (the raw member read fails; the engine walk aborts or skips), while (a) the same fault on the FIRST member (the piped local-disk path) scans clean, and (b) the recovery of the very same files through the session's armed basis succeeds (the member is piped there). One disk, one fault, three different outcomes depending on which layer reads it.

## Impact

Multi-disk dynamic disks are the RAID/span arrays recovery products see in exactly the damaged states retries exist for (failing members, flaky cables). The scan loses files the device could serve on attempt 2; the loss is silent (the candidate surfaces fewer files, no error taxonomy at the member level). The single/multi inconsistency also means the scan's behavior depends on whether a volume happens to span disks — an accident of layout, not a semantic choice.

## Reproduction / verification

- Core side (first-hand, current tree): the resolver sites read (both return raw devices); the LDM block's scan_candidate call read; the local fallback's return of the piped slice read; the recovery-side session_arm_ldm_basis wrapping read (the d28244df landing).
- Corpus side (first-hand): §8's statement, the :919095 ctor, the :7167 RTTI cast — all read this session.
- Execution: the fix's test constructs the two-disk transient-fault fixture (the RED proof — the scan loses the file pre-fix, finds it post-fix).

## Fix plan

1. Wrap each platform-resolved member in its own `build_read_pipe` chain at BOTH resolver-arm sites (`rrecov.cpp`'s scan-arm block and `cr_ui.cpp`'s `arm_ldm_resolver`), the pipes owned beside the members they wrap (the CLI's local vector, `CrScan`'s member set) — the exact shape `session_arm_ldm_basis` already uses on the recovery side (one construction pattern, both sides).
2. No orchestrator change: the resolver indirection carries the pipes transparently (`LdmVolumeDevice` reads through whatever the resolver returns).
3. Tests: (a) the two-disk transient-fault integration — the scan finds the file whose extents cross the fault (RED pre-fix: the raw member read fails); (b) the construction-seam grep pin at both sites (the M-136 precedent); (c) the single-disk path unchanged (already piped via the local fallback — a guard pin).
4. Register the internal-consistency note in the finding's Resolution (single-disk was already piped; multi-disk now matches).

## Tests to add

- `Orchestrator.SpannedLdmTransientMemberFaultScans` — RED today.
- `Orchestrator.LdmMemberResolversConstructThroughBuildReadPipe` (the seam pin).
- `Orchestrator.SingleDiskLdmLocalFallbackStaysPiped` (the guard).

## Related findings

- M-058 (FIXED — the scan read chain's single construction point this fix reuses), M-136 (FIXED — the export-side pipe; this is the scan-side residual on the LDM member surface), d28244df's LDM supplement (the recovery-side member pipes this fix mirrors), M-119 (the partition-boundary semantics — adjacent but disjoint).

## Open questions

None — the corpus contract (engines read cached devices) and the core gap (raw platform-resolver members) are both directly verified.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this push — the S72 lane's third landing, filed and fixed in the same session per §8)
**Verified by:** `Orchestrator.LdmMemberResolversConstructThroughBuildReadPipe` (RED at the stash-revert state: all three grep pins fire — no build_read_pipe at either resolver-arm site and the raw `(*guid_map)[g] = dev.get()` handoff present; GREEN post-fix), `Orchestrator.SpannedLdmTransientMemberFaultScans` (the two-disk reversed-concat fixture with a one-shot transient fault on the volume's FIRST member: the raw-member leg — the pre-fix production shape — loses the whole volume to ONE transient fault [0 files, the silent loss]; the piped-member leg — the post-fix shape — retries the boot-sector read and surfaces DOC.TXT); full suite **1530/1530**.
**Resolution notes:** CONFIRMED-DIVERGENCE, discovered during this lane's M-136/M-135 verification work (the read-substrate census across every scan read path). Corpus verified first-hand: the engines' device contract (ntfs-engine--deep-dive §8 — "the volume reads go through the device abstraction (CStorageDeviceWithCache → IStorageDevice)"), the scan driver's cache construction (DRWUI:919095), and the engine's RTTI assertion of the cached class (NTFSSearchDLL:7167 → FUN_18000fae0). Core-side census: the primary volume, the partition-table targets (S-051's slices wrap the pipe), the lost-partition candidates (lost::materialize over the piped slice), the carve stage, and single-disk LDM members (the local fallback resolver returns the pipe itself) ALL ride the substrate — the cross-disk platform resolvers were the one bare path, and the recovery side already pipes the same members (the d28244df landing's session_arm_ldm_basis shape), making the scan the outlier on its own reads. The fix wraps each platform-resolved member in its own build_read_pipe chain at BOTH resolver-arm sites (the CLI's scan-arm block and the UI's arm_ldm_resolver — `ldm_member_pipes` owned beside the members, the exact shape the session-side arm uses), with the resolver indirection carrying the pipes transparently into the LdmVolumeDevice's member reads. The internal-consistency note registered: single-disk LDM was already piped via the local fallback; multi-disk now matches, and scan/export agree on member readability (both piped). No safety hardening was near this site; the resolver's driver-only posture (a member that will not open through RDA does not resolve) is unchanged — the pipe wraps only successfully-opened members.


## Renumber addendum (the S77 lane's born-collided-seat repair, per SA-9)

This finding was born-collided with the deep-read M-148 (the 047d8a96
collision-rider renumber reserved the seat at 00:38 for the retired deep-read
landing; this lane's 13:43 filing took the same seat without the
FIXED-LEDGER/in-flight allocation check — the N-160 ID-allocation class). Per
SA-9 (earlier-push-wins) the deep-read keeps M-148; this finding renumbers
**M-148 → M-153** (the next free seat after the S77 lane's M-149..M-152
filings). The doc_truth_gate's check_finding_id_uniqueness caught the
collision at the S77 lane's push (the gate's first catch of this class since
the collision-rider-4 repair). The ledger row (lost in a hot-window fold per
the S55 precedent — neither M-148 file carried a row at the rebase head) is
re-landed under M-153. The four in-source `M-148:` comments
(`src/app/rrecov.cpp:1623/:1648`, `src/ui/cr_ui.cpp:908/:2124`) reference
this finding's landing; the S77 lane is READ-ONLY on source and records them
here for the next fix lane's truth sweep.
