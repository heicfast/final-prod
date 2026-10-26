# AUDIT-FALLBACKS — Refactor-B fallback/degraded-path census

**Task ID:** Refactor-B (read-only audit; one output file; no source changes)
**Date:** 2026-10-23 (session clock)
**Pins:**
- core_recovery @ `08b44ac28bdcdd6263e7fe20fe0b597c141f87d9` (main; working tree clean)
- easeus-drw-research (RE corpus) @ `18d6f14a1add9fc761ed7b3732bb1715b15aa79f`
- rda-wizard @ `e45ddcad919357b30e37b7e92b06fde99b07e81c`
- rda-driver @ `7e88e38c585604c077517027a4cce30863369c6c` (v0.2.0)

**Central question (user):** does the og codebase/architecture have any fallback for driver or
engine load — and if not, why does ours? Remove invented degradation ("slop") after
verification.

## 1. Methodology

1. Line-by-line read of every listed core file (win layer, orchestrator, storage, carve,
   health, repair, preview, query, recovery, session, compress, CLI) plus the corresponding
   headers; every conditional that changes/swallows/degrades behavior on failure was
   enumerated.
2. Reference side: `rda-wizard/docs/TRANSPORT-CONTRACT.md` §2.2/§4/§9 (verified evidence chain
   for the transport duality), MOM-modules artifacts (Device/DeviceManager/DeviceAdapter/
   FileSystemAnalyser/FileSystemCheck/Error), `searchdll-family--architecture.md` + per-engine
   deep-dives (NTFS/HFS+/EXT4/ZFS/Btrfs), `eprdtdrv-sys--driver-internals.md` +
   `epmntdrv-trio--naming-pid-handshake.md`, `architecture/04-recovery-workflow.md`,
   DRWUI thread/main-ui architecture docs, UIUX scan-phase deep-dive, DiskHealth artifact,
   `config-files/config-data-reference.md`.
3. Direct decompiled-tree greps: `ghidra-mo/Device_decompiled.c` (transport fallback sites —
   see FB-T01), `ghidra-searchdll/*` (CreateFileW usage, imports for zlib/lz4/zstd).
4. Cross-check against `docs/PARITY-DELTAS.md` (D-01…D-35) and `docs/error-fix-registry.md`
   (F1–F16) so every RETAIN-AS-SAFETY row maps to its registered delta; unregistered
   deliberate behaviors are flagged as DELTA-CANDIDATE (per the registry's own rule: "a delta
   that is not in this registry is a bug, not a delta").
5. Verdict framework: REMOVE / MATCH / RETAIN-AS-SAFETY / DELTA-CANDIDATE / UNVERIFIED.

**Pre-established fact confirmed (not re-litigated):** the direct-access transport duality
(RDA driver path with `\\.\PhysicalDriveN` / `\\.\HarddiskVolumeN` fallback) IS reference
behavior. New decompiled evidence found this session (beyond TRANSPORT-CONTRACT §9):
`ghidra-mo/Device_decompiled.c` pairs `\\.\EPRDTDRV\%d\%d` → `\\.\PhysicalDrive%d` at four
sites (lines 7793/7822, 50628/50659, 52414/52443, 54621/54672) and
`\\.\EPRDTDRV\%d\Volume\%d` → `\\.\HarddiskVolume%d` (lines 329/358), each gated on the
driver probe `FUN_1800577e0()` (return 0 → driver path; non-zero → direct path), with the
PID-token transform inlined byte-identically to our `rda::compute_pid_token`. The og does NOT
print a transport label at these sites; the "no-admin degraded mode" is surfaced at UI level
(epmntdrv-trio §6: "This is the 'no-admin' degraded mode visible in the UI flows").

## 2. Census

Columns: ID | our file:line | condition | what we do | what og does (evidence) | verdict | action.

### 2.1 Transport / driver layer (win)

| ID | our file:line | condition | what we do | what og does (evidence) | verdict | action |
|---|---|---|---|---|---|---|
| FB-T01 | rrecov.cpp:137-158 (`open_disk_view`) | `rda_available()` fails (GLE out) | print `[transport] RDA not available (\\.\RDA CreateFile error %lu) - direct path`, open `\\.\PhysicalDriveN`; only if that also fails refuse with `cannot open disk %u (elevate?)` | driver probe `FUN_1800577e0()` non-zero → format `\\.\PhysicalDrive%d` directly (Device.mo sites above); probe failure is the only fallback condition; no print at the site (UI-level surface); refusal only when the direct open fails | **MATCH** | Keep. Deviation (broader condition, see FB-T02) + deviation (label print — sanctioned by D-24/ADR-5 truthful-degradation). No change. |
| FB-T02 | rrecov.cpp:144; cr_transport.cpp:618-625 (`open_disk`) | driver PRESENT but the grammar open fails | print `RDA open failed (%hs) - direct path`; fall back to direct | og condition is the availability probe only — at the probed sites an EPRDTDRV CreateFile failure is not re-tried with PhysicalDrive in the same expression (Device.mo 7793-7822); whether other og sites retry on open-failure is not decoded | **MATCH** (minor deviation, defensible) | Keep (an unopenable grammar path must not kill a scan; direct is sufficient for unlocked volumes — epmntdrv-trio §6). Register the condition-superset note in PARITY-DELTAS D-22 remarks or CORE-CONTRACT. |
| FB-T03 | cr_transport.cpp:721-759 (`open_disk_rw`) | RDA RW handle open fails / geometry 0 | fall back to direct `\\.\PhysicalDriveN` GENERIC_RW | og writes go "through the same raw path (partition table fixes) or via normal file APIs" (04-recovery-workflow §7); Device.mo duality applies to disk handles generally | **MATCH** | None. |
| FB-T04 | cr_enumerate.cpp:98-112 | enumeration | always direct `\\.\PhysicalDriveN`; never needs the driver | og `GetDriverInfo` = `GetLogicalDrives`/`GetDriveTypeW` drive-letter topology, NO SCM/driver APIs (getdriverinfo--tb-module-decode.md:20-29); TRANSPORT-CONTRACT §9 confirms "enumeration never needs the driver" | **MATCH** | None. |
| FB-T05 | cr_transport.cpp:237-246 (`open_bare_handle_`) | RW open of the control device fails | re-open GENERIC_READ and proceed | reference client opens the bare global handle GENERIC_READ (rdacli.c:42-47, contract §7.1); all control IOCTLs are FILE_ANY_ACCESS so read-only is functionally complete | **MATCH** | None (documented in TRANSPORT-CONTRACT §8). |
| FB-T06 | cr_transport.cpp:264-269 (`RdaClient::open`) | GET_INFO fails on an opened control device | `have_info_=false`, `ioctl_base_=0`, proceed (silent capability degrade) | TRANSPORT-CONTRACT §9: "GET_INFO failure on a successfully-opened device = wrong/ancient driver → treat as 'wrong driver'". og's EPMNtDrv has no GET_INFO at all (extended IOCTLs postdate the reference binaries — contract §9), so there is no og behavior to match; our own contract doc prescribes refusal | **REMOVE** (the silent-degrade branch) | Delete the tolerated-failure semantics: on `query_info()` failure `RdaClient::open` must return nullptr with a distinguishable error (e.g. reuse `ERR_OPEN_FAILED` + expose `last_error=wrong-driver`), so callers take the direct path instead of issuing base-0 session IOCTLs at a foreign driver. Currently unreachable from rrecov (RdaClient is the wizard binding surface), so risk is contained but real for the UI. |
| FB-T07 | cr_transport.cpp:449-462 (`session_get_next_block`) | the GETNEXTBLOCK IOCTL fails (ANY NTSTATUS) | treat as `stream_done=true, progress=100, OK` | driver completes drained pumps with `STATUS_CANCELLED 0xC0000120` (contract §3.5, §5 GETNEXTBLOCK row); other statuses (`STATUS_INVALID_HANDLE`, `STATUS_NO_MEMORY`, …) are real failures per the same table | **REMOVE** (over-broad) | Narrow the success-mapping to the drain signal only: on DeviceIoControl failure check `GetLastError()` (0xC0000120 maps to `ERROR_GEN_FAILURE`) → stream done; anything else → return the error (`ERR_READ_FAILED`). Delete the blanket `if (!ctl_ioctl(...)) { *stream_done=true; ... return true; }` branch and replace with the two-case check. |
| FB-T08 | cr_transport.cpp:63-84 (alignment query fails), 87-93 (geometry-EX fails) | optional geometry IOCTLs fail | physical=logical fallback; 512+GET_LENGTH_INFO fallback | og normalizes sector size above the driver ("InitSectorSize in EUDCPEPM and user-mode geometry queries" — eprdtdrv-sys §3 note); D-04 registered (byte-anchored geometry, 4Kn first-class) | **RETAIN-AS-SAFETY** | Maps to D-04. No change. |
| FB-T09 | cr_transport.cpp:147-155 (`open_path`) | both geometry IOCTLs give 0 bytes | `GetFileSizeEx` last resort with 512-byte sectors | no og equivalent visible in corpus (og derives sizes from volume extents / MSFT_* WMI — Device.md strings) | **DELTA-CANDIDATE** | Harmless exotic-handle fallback; register as a D-04 remark or drop if the wizard never passes file handles. |
| FB-T10 | cr_transport.cpp:78-81 sane-range guard on alignment descriptor | descriptor carries nonsense (0 / huge) | keep geometry-EX value | og: no equivalent guard decoded | **RETAIN-AS-SAFETY** | Hardening (audit row for the VHD cache-line bug). Maps to D-04 family. |

### 2.2 Orchestrator (cr_scan.cpp)

| ID | our file:line | condition | what we do | what og does (evidence) | verdict | action |
|---|---|---|---|---|---|---|
| FB-O01 | cr_scan.cpp:287-336 (`probe_family`) | boot-sector probe reads fail / no signature | return `FsFamily::Unknown` (not an error) | FS type comes from partition-table / boot-sector probe by PartitionRecovery analysers (04-recovery-workflow §3.2); unidentifiable → no engine selected in the FS→factory map (FUN_140577d40 switch) | **MATCH** | None. |
| FB-O02 | cr_scan.cpp:614-616 | fs_hint Unknown after probe | `rc = OK` — "quick yields nothing; deep/carve still run" | Advanced scan registers ALL full searchers + RAWSearcher regardless of FS (FUN_140580720, 04-recovery-workflow §3.3); unknown-FS volume → no FS engine runs, RAW carving still does | **MATCH** | None. (Answers Q1 partially — see §4.1.) |
| FB-O03 | cr_scan.cpp:358-360; cr_retry.cpp:86-90, 97-98 | persistent transport read failure mid-scan | `RetryMode::Skip`: retry ×3, bisect to sector granularity, record in ReadErrorMap, zero-fill, return OK | og: EXT4 engine's `CEXT4CatchBufferDeviceProxy` retries with a shifted window (0xE00 buffer, 0x400 backtrack) and "salvages what it can instead of failing the whole group scan" (ext4-engine--deep-dive §3); product-wide UX copy `BadSectorTip: "Program is trying to skip bad sectors to find data."` + `BadSectorTipsDlg` (config-data-reference.md:55; UIUX/03-window-class-map.md:125). NOT registered in PARITY-DELTAS (only D-19's recovery-queue mention of "zero-fill Skip policy") | **DELTA-CANDIDATE** (register) | Not slop — the concept is reference behavior. Two actions: (1) add a PARITY-DELTAS entry (scan-side retry/skip policy: og = per-engine salvage proxies + skip messaging, ours = transport-wide bisection + ReadErrorMap + zero-fill; mechanism differs, outcome class matches); (2) note the og surfaces the state in UI (`BadSectorTipsDlg`) — our CLI prints error ranges to the log only (rrecov.cpp:497-500); the wizard must render the map (already tracked as G-06 telemetry). |
| FB-O04 | cr_scan.cpp:373-377 | `CachedReader::create` fails (bad params) | pipe silently degrades to bare RetryingDevice / raw slice | og `CStorageDeviceWithCache` is always constructed in the engine IO path (cr_device.h header contract; ntfs-engine internals) | **DELTA-CANDIDATE** | Unreachable in practice (fixed 8 MiB/64 KiB params), but silent: log/telemetry-counter the degraded pipe so it is never invisible. |
| FB-O05 | cr_scan.cpp:497-499 | LDM database parse | `parse_ldm_database` self-rejects non-LDM after one 512-byte read; LDM branch only on `found` | og LdmManager.mo owns dynamic-disk databases; non-LDM disks are simply not dynamic | **MATCH** | None (self-rejection is the detection, not a degrade). |
| FB-O06 | cr_scan.cpp:511-513 | LDM layout `Broken` or no copies | skip scannable volume creation ("surfaced via the LdmInfo report") | og: broken LDM volumes are reported, not scanned (D-08 clamp-not-abort family) | **RETAIN-AS-SAFETY** | Maps to D-08. No change. |
| FB-O07 | cr_scan.cpp:699-724 (parallel), 726-733 (serial) | lost-candidate engine scan fails | `(void)scan_candidate(...)` — "candidate misses never abort the scan"; only ERR_CANCELLED propagates | og: lost partitions inserted as additional scan targets and "then scanned by the appropriate FS DLL" (searchdll-family §6.3; 04-recovery-workflow §3.4); per-candidate failure semantics not decoded | **MATCH** (with UNVERIFIED nuance) | Candidates are speculative by construction (og's model too). Keep; the UNVERIFIED register tracks the og per-candidate rc behavior (§5 U-02). |
| FB-O08 | cr_scan.cpp:752-831 (`scan_candidate`) | engine open/parse fails on a candidate | rc discarded, rc=OK ("open failure = not a viable NTFS volume: not a scan error") | as FB-O07 | **MATCH** (candidate scope only) | Important scope note: this leniency must NEVER apply to the main volume — and it does not (see FB-O10). |
| FB-O09 | cr_scan.cpp:532-547 (+ per-family cases 548-613) | structured engine returns rc != OK on the MAIN slice | rc propagates → `run()` stamps it → CLI prints `scan failed` and exits 1 (rrecov.cpp:503-506); carve does NOT run | og engine errors are HRESULT `0x5002xxxx` surfaced through controller plumbing with a verbose cout channel `"Return Error:0x%x File:%s Line:%d Fuction:%s"` (hfsplus-engine--deep-dive:53; ntfs-engine--deep-dive:155-158); whether DRWUI stops the whole scan or continues to RAW on engine failure is not decoded | **UNVERIFIED** (our behavior is the conservative one) | Track U-01 (§5). If the corpus ever proves og falls through to RAW on structured failure, revisit; today no evidence either way. |
| FB-O10 | cr_scan.cpp:947-948 | session record CRC bad (`0x50030002`) | skip the record, keep going (F10) | og `.rsf` has NO checksums; torn tails parse as empty (error-fix-registry F10; rsf-format artifact) | **RETAIN-AS-SAFETY** | Maps to D-01 + F10 (resumable-by-design). No change. |
| FB-O11 | cr_scan.cpp:967 | `deserialize_error_map` fails | error map silently dropped from the loaded session | og has no error map at all (cr_devmap.cpp header: "reference had no persistent error map") | **DELTA-CANDIDATE** (low) | One-line fix candidate: count and surface dropped maps (a corrupt error map hides damage history). Not urgent. |
| FB-O12 | cr_scan.cpp:990-991 | unknown record type in session | skipped silently (forward compat) | og rsf reader is record-GUID-driven and tolerant (rsf-format artifact) | **MATCH** | None. |
| FB-O13 | cr_scan.cpp:1012-1014 | stage_done_mask ⊄ configured_mask | `ERR_SESSION_FORMAT` — never a silent partial resume | no og equivalent (og sessions carry no completion mask) | **RETAIN-AS-SAFETY** | Maps to G-07/D-01 truth rules. No change. |

### 2.3 Storage (retry / cache / slice / devmap)

| ID | our file:line | condition | what we do | what og does (evidence) | verdict | action |
|---|---|---|---|---|---|---|
| FB-S01 | cr_retry.cpp:50-53 | deterministic errors (OOB, bad-param) | not retried — fail immediately | sensible; og has no transport-wide retry at all (driver returns forwarded NTSTATUS immediately — contract §6) | **RETAIN-AS-SAFETY** | Part of the FB-O03 delta family. |
| FB-S02 | cr_cached.cpp:61-68 | underlying block read fails | block NOT cached; read fails; next read retries the transport (audit row 38) | og `CStorageDeviceWithCache` semantics not decoded at this granularity | **RETAIN-AS-SAFETY** | Correctness fix (was a pinned-forever-invalid bug). |
| FB-S03 | cr_cached.cpp:83-87 | invalid block reached on hit path | `ERR_READ_FAILED` | as above | **RETAIN-AS-SAFETY** | None. |
| FB-S04 | cr_slice.cpp:15-50 | slice creation with bad bounds / unaligned start | refuse (`ERR_BAD_PARAMETER` / `ERR_RANGE_OUT_OF_BOUNDS`); reads clamp | og `CPartitionVirtualVolumeDevice` bounds every engine view (cr_slice.cpp header, corpus 05 §4.2) | **MATCH** | None. |

### 2.4 Carve (cr_carve.cpp)

| ID | our file:line | condition | what we do | what og does (evidence) | verdict | action |
|---|---|---|---|---|---|---|
| FB-C01 | cr_carve.cpp:299-302 | carve window read fails (post-retry) | count the window's bytes as scanned, `return OK` (window silently skipped) | og RAWSearchDLL reads through the same volume IO (04-recovery-workflow §4b); failure behavior per window not decoded; og's salvage analog is the per-engine CatchBuffer (ext4 deep-dive §3) | **DELTA-CANDIDATE** | Two issues: (1) silence — the skipped window never appears in any stat (only the retry pipe's error map knows); (2) truth — `bytes_scanned += counted` claims coverage of bytes that were NOT scanned (the G-05 "honest fill-rate" comment two lines above is violated on this path). Action: record skipped-window bytes separately (`stats` or carve stats field) and do NOT count them as scanned; surface in the log like the scan error ranges. |
| FB-C02 | cr_carve.cpp:92-99 | signature grammar validator rejects | `rejected` counter, continue | og F8: header-only raw_signs entries inflate counts — ours requires grammar validation (error-fix-registry F8) | **MATCH** | Maps to F8/D-13. |
| FB-C03 | cr_carve.cpp:101-126, 237-240 | duplicate of structured/carved extent | `duplicates` counter, drop | og F9: no dedup between structured and carved (error-fix-registry F9) | **MATCH** (fix direction) | Maps to F9/D-13. |
| FB-C04 | cr_carve.cpp:171-280 | raw-sign layer (139-format table, 512-stride, tail pairing) | probe + confidence-ranked emit | og raw_signs.json 139 extensions, header+tail confidence accumulation (searchdll-family §4.3) | **MATCH** | Maps to D-13 for the delta details. |

### 2.5 Health (cr_health.cpp, cr_nvme.cpp, rrecov)

| ID | our file:line | condition | what we do | what og does (evidence) | verdict | action |
|---|---|---|---|---|---|---|
| FB-H01 | cr_health.cpp:172-179 | ATA passthrough fails | legacy `SMART_RCV_DRIVE_DATA` fallback; if that fails → `ERR_HEALTH_UNAVAILABLE` | og DiskHealth (CrystalDiskInfo lineage) has a multi-transport accessor set: `GetSmartAttributeSat -1A/-2/-2A`, `GetSmartAttributeCsmi`, `GetSmartAttributeNVMeRealtek` (diskhealth--smart-companion.md §2) | **MATCH** | Audit row 43 documented. Our two-transport set is a subset of og's five; CSMI/bridge variants would be additions, not removals. |
| FB-H02 | cr_health.cpp:244-279 | NVMe health query fails | `ERR_HEALTH_UNAVAILABLE` | og: `INTERFACE_TYPE_NVME` + virtual-NVMe awareness (same artifact) | **MATCH** | None. |
| FB-H03 | cr_health.cpp:281-300 | `fsutil` TRIM query fails / unparseable | `ERR_HEALTH_UNAVAILABLE`; else parse `= 0` → ON | og DRWUI parses the same fsutil output (code comment cites corpus DRWUI parsing; UIUX home-page TRIM strings) | **MATCH** | None. |
| FB-H04 | rrecov.cpp:788-841 (`cmd_health`) | SMART unavailable | print `SMART unavailable (rc=…) - trying NVMe...`, continue; exit code **0** even when SMART AND NVMe both fail | og: health is surfaced product-wide (DRWUI SSD/TRIM warnings + the DiskHealth companion app); "unavailable" is a rendered state, not a silent success | **DELTA-CANDIDATE** | Keep the truthful prints; change the exit contract: return non-zero (e.g. 3) when EVERY health source is unavailable so scripts/tests cannot mistake total failure for success. (CLI-only change; matches the K-01 fix class.) |
| FB-H05 | cr_health.cpp:302-319 | seek-penalty query fails | `looks_like_ssd` returns false → CLI labels `present (rotational?)` | og DiskHealth names interfaces explicitly per disk | **DELTA-CANDIDATE** (low) | Distinguish query-failed from penalty-present (tri-state or print `unknown`). One-line CLI label change. |

### 2.6 BitLocker (cr_bitlocker.cpp, rrecov)

| ID | our file:line | condition | what we do | what og does (evidence) | verdict | action |
|---|---|---|---|---|---|---|
| FB-B01 | cr_bitlocker.cpp:19-32, 74-80 | boot-sector read fails (locked volume, access denied) | `read_boot_sector` false → `is_fve_volume` false → `status()` returns **Clear** | og: locked BitLocker volumes route to `CBitlockerStartWnd` unlock dialogs (04-recovery-workflow §2, §10); a locked volume is never treated as clear | **REMOVE** (misclassification) | Distinguish read-failure from no-signature: `is_fve_volume` is a two-state probe over an operation with three outcomes. Minimal change: have `read_boot_sector` expose the failure (or add `is_fve_volume_ex(letter, bool* readable)`), and `status()` return `Status::Unknown` (the enum value already exists and is currently unreachable except by default) when the sector could not be read. The CLI then refuses with the unlock guidance instead of proceeding to scan a locked volume as if clear. |
| FB-B02 | cr_bitlocker.cpp:100-128 | unlock requested | PowerShell `Unlock-BitLocker` → on rc!=0 `manage-bde -unlock` fallback → `ERR_FVE_LOCKED` | og bitlockerlib: `PSUnlockBitLocker` / `manage-bde -unlock` "pre-PowerShell fallback — let Windows itself unlock the volume" (bitlockerlib--bitlocker-implementation.md:17,44) | **MATCH** | Maps to D-16/F13. |
| FB-B03 | cr_bitlocker.cpp:191-218 | auto-unlock | enumerate `HKLM\...\FVEAutoUnlock` binary BEK blobs → temp `.BEK` → `-KeyFilePath`; failure → `ERR_FVE_LOCKED` | og DRWService reads the same registry via the DRWBitlocker pipe (04-recovery-workflow §4a; DRWService artifact) | **MATCH** | Maps to D-16 (no crypto in-process — the deliberate F13 delta). |
| FB-B04 | rrecov.cpp:162-174 | volume locked and auto-unlock fails | print the exact `Unlock-BitLocker -MountPoint %c: -RecoveryPassword <key>` guidance and REFUSE the scan (exit 1) | og: unlock dialogs (password/recovery-key), never a silent skip | **MATCH** | Truthful degradation surface (D-16). Error code `ERR_FVE_LOCKED` is returned by the library; the CLI prints guidance — no silent skip anywhere. |

### 2.7 Session / CRSF (cr_crsf.cpp, cr_records.cpp)

| ID | our file:line | condition | what we do | what og does (evidence) | verdict | action |
|---|---|---|---|---|---|---|
| FB-G01 | cr_crsf.cpp:289-295 | per-chunk inflate fails | fall back to the raw chunk payload (F10) | og .rsf: no checksums; torn data parses as garbage/empty (F10) | **RETAIN-AS-SAFETY** | Maps to D-01/F10. The record-level CRC then catches the corruption (0x50030002 → FB-O10 skip). |
| FB-G02 | cr_crsf.cpp:236-241 | record header magic broken | skip forward in UNIT steps until a well-formed header or EOF (F10 resumability) | as above | **RETAIN-AS-SAFETY** | Maps to D-01/F10. |
| FB-G03 | cr_crsf.cpp:242-246 | unfinalized tail record (chain=0, crc=0) | stop indexing (truncated save tolerated) | og: torn tails parse as empty (F10) | **RETAIN-AS-SAFETY** | Maps to D-01/F10. |
| FB-G04 | cr_crsf.cpp:28-34 | deflate not strictly smaller | store raw (bit15 clear) | og rsf chunk framing: "stored-when-not-smaller" comparison rule (rsf-format artifact; D-01) | **MATCH** | None. |
| FB-G05 | cr_records.cpp:277-281 | tree deserialization cursor fails mid-record | error only when created < count (partial trees never silently claimed complete) | og rsf loader semantics not decoded at this granularity | **RETAIN-AS-SAFETY** | None. |
| FB-G06 | cr_crsf.cpp:296-299 | decompression bomb (>64 MiB record) | `ERR_NO_MEMORY`, data cleared | no og equivalent (P1-7 hardening) | **RETAIN-AS-SAFETY** | None. |

### 2.8 Recovery (cr_safewriter.cpp, cr_report.cpp, cr_destio.cpp, rrecov)

| ID | our file:line | condition | what we do | what og does (evidence) | verdict | action |
|---|---|---|---|---|---|---|
| FB-R01 | cr_safewriter.cpp:90-103 | queued node's extents intersect the destination extent | hard refusal `ERR_DEST_OVERLAP` (item state Refused) | og destination picker warns ("We highly recommend saving the found data to a different drive…" — DRWUI main-ui architecture, ChooseDestinationPathDialog); ExportThread opens `\\.\X:` for "writability + same-volume detection" (04-recovery-workflow §7.1-7.3); exact refuse-vs-warn semantics UNPINNED (D-17 NEEDS-WORK) | **RETAIN-AS-SAFETY** | Maps to D-17 (+D-20 UI gate). Keep the hard refusal regardless (D-17's own stance). |
| FB-R02 | cr_safewriter.cpp:292-299, 320-324 | source read fails / compressed block undecodable mid-recover | zero-fill the span, continue, hash will mismatch (salvage); `hash_mismatch` counted only when a stored hash exists | og: per-target export failures are COLLECTED for the recovery log — "%1 files/%2 failed; for details, you may check the recovery log" (04-recovery-workflow §7.5) | **DELTA-CANDIDATE** | Two gaps: (1) when the item has NO stored hash (`hash_valid=false`), a zero-filled item is reported `Done` with `hash_status=computed` — the zero-fill is invisible in the report; (2) the recovery read path has NO retry pipe (cmd_recover reads the raw slice directly), so a transient failure becomes permanent zero-fill. Action: add a per-item `had_unreadable` flag set at the zero-fill sites and surface it as a report outcome (e.g. `salvaged`), so salvage is truthful like og's failed-count log. Registered behavior lives in D-19's "zero-fill Skip policy" — extend that entry. |
| FB-R03 | cr_safewriter.h:86 | `skip_hashless` config | DEAD: never read by cr_safewriter.cpp; behavior is unconditional (hash-less items always recover, hash_status=Computed) | n/a (our own config surface) | **REMOVE** | Delete `bool skip_hashless = false;` from `WriterConfig` (cr_safewriter.h:86) — or wire it if the policy is actually wanted. Also correct PARITY-CORE-DOM.md:287 which cites it as a verified behavior knob. |
| FB-R04 | cr_safewriter.cpp:46-78, 188-193 | name collision / pre-existing final | Rename (`name (N).ext`) / Skip (idempotent resume) / Overwrite policies | og collision handling unevidenced (D-19: "no collision policy in any artifact — AUDIT-UI §5.7 'unevidenced in og'") | **RETAIN-AS-SAFETY** | Maps to D-19/F11. |
| FB-R05 | cr_safewriter.cpp:353-368 | hash mismatch after write | content committed (salvage), per-item `HashMismatch` outcome + aggregate; CLI exit 2 | og: F11 (reference overwrites on collision; no hash verify at all) | **RETAIN-AS-SAFETY** | Maps to D-19/F11. |
| FB-R06 | rrecov.cpp:651-652 | destination resolves to the SOURCE disk | print `[F12] WARNING: destination is on the SOURCE disk %u`, then enqueue (per-item refusal follows via FB-R01) | og warns at the destination dialog (D-17) | **RETAIN-AS-SAFETY** | Maps to D-17/D-20. Note the CLI print + writer refusal are defense-in-depth, both truthful. |
| FB-R07 | cr_report.cpp:128-158, 478-494; rrecov.cpp:731-754 | pass aborted / items pending | report still emitted; pending items NEVER claimed done | og: recovery log carries failures (04-recovery-workflow §7.5) | **RETAIN-AS-SAFETY** | Maps to G-09/D-19. |
| FB-R08 | rrecov.cpp:755 | per-item errors or hash mismatches | exit code 2 (distinct from 0 success / 1 hard failure) | no og CLI analog (og is a GUI) | **RETAIN-AS-SAFETY** | None. |
| FB-R09 | cr_destio.cpp:61-73, 80-112 | uncommitted temp / mkdir chain | temp removed on close (never a half-written final); `ERROR_ALREADY_EXISTS` on a DIRECTORY is success, on a FILE is `ERR_DEST_UNWRITABLE` | og: `CreateDirectoryW` path reconstruction (04-recovery-workflow §7.5) | **RETAIN-AS-SAFETY** | Maps to G-08. |

### 2.9 Preview / query (cr_preview.cpp, cr_query.cpp)

| ID | our file:line | condition | what we do | what og does (evidence) | verdict | action |
|---|---|---|---|---|---|---|
| FB-P01 | cr_preview.cpp:29, 61, 67, 73, 90-92 | extent read/decode fails during preview | zero-fill the requested window, record in `unreadable_` map (surfaced), never abort the reader | og preview failures surface diagnostics ("failed got socket/send data to peer" — 04-recovery-workflow §6) | **RETAIN-AS-SAFETY** | Maps to G-10/D-28 (ADR-8 bounded workers; every failure surfaces a diagnostic). |
| FB-P02 | cr_preview.cpp:703-712 | unrecognized format | `kind="unsupported"` (truthful), text-sniff otherwise | og: per-format preview windows with fallback families | **RETAIN-AS-SAFETY** | Maps to D-28. |
| FB-P03 | cr_query.cpp:100-137 | filter predicate | pure query — no degradation paths (count-only fast path is an optimization, same results) | og filters client-side over the loaded tree (D-29) | **MATCH** | Maps to D-29/ADR-9. |

### 2.10 Compress (cr_decomp.cpp)

| ID | our file:line | condition | what we do | what og does (evidence) | verdict | action |
|---|---|---|---|---|---|---|
| FB-X01 | cr_decomp.cpp:18-23, 126-148, 169-180; CMakeLists.txt:47-71 | zstd/lz4 not found at BUILD time | codecs compiled out; `codec_available()` returns false; `decompress_block` returns false → downstream zero-fill (FB-R02) / unreadable (FB-P01) | og NEVER runs without its decompressors: BtrfsSearchDLL carries ZLIB inflate/deflate + adler32/crc32c capa-verified (btrfs-engine--deep-dive §2); ZfsSearchDLL reimplements lzjb/gzip in-engine (zfs-engine--deep-dive §1/§4) — and has NO zstd/lz4 imports or strings (imports + decompiled grep: zero hits), i.e. og's ZFS support is lzjb/gzip-only and always present | **REMOVE** (the build-time optionality) | The codecs themselves are a superset of og (fine — D-05/D-09 territory), but the OPTIONALITY is invented degradation: a build without zstd silently zero-fills zstd pools with only a hash-mismatch signal, and `codec_available()` is consulted by NO production caller (test-only — grep: test_preview.cpp:168). Actions: (a) make lz4/zstd REQUIRED for production builds (they are already vcpkg.json dependencies; turn the CMake `if(CR_LZ4_LINK)` into `find_library`-or-`FATAL_ERROR`, keep the fuzz Makefile's pkg-config relaxation); or minimally (b) call `codec_available()` at scan time when a compressed extent is first seen and fail THAT item/volume loudly (distinguish codec-missing from malformed). |

### 2.11 CLI (rrecov.cpp)

| ID | our file:line | condition | what we do | what og does (evidence) | verdict | action |
|---|---|---|---|---|---|---|
| FB-K01 | rrecov.cpp:227-250 + 1252-1255 | `enumerate_disks` fails | print `enumeration failed` — and **exit 0** (`print_disks` is void; `wmain` returns 0) | og home screen renders disk state; enumeration failure is a visible error state, never a silent success | **REMOVE** (exit contract) | Make `print_disks()` return `int` (0/1) and propagate in `wmain` (`if (cmd == L"list") return print_disks();`). The print is already truthful; only the exit code lies. |
| FB-K02 | rrecov.cpp:431-435 | `--deep` on a volume target | print `[note] --deep on a volume: lost-partition discovery needs --disk (skipping that stage)`, set whole_disk=false | og lost-partition scanning is a whole-disk strategy (`CDoScanLostPartition`) — volume targets never run it | **MATCH** | Truthful adaptation print. None. |
| FB-K03 | rrecov.cpp:139, 144, 148, 153, 156 | transport selection | every path labeled (`RDA driver path` / `direct path` / exact CreateFile GLE) | og: no print at the Device.mo site; degraded mode surfaced at UI level (epmntdrv-trio §6) | **RETAIN-AS-SAFETY** | Maps to D-24/ADR-5 (truthful transport label — the UI contract requires it). |
| FB-K04 | rrecov.cpp:456-459 | LDM member open fails | `open_disk(prefer_rda=true)` then retry `prefer_rda=false`; unresolved disks simply don't resolve | og cross-disk LDM semantics: D-08/LDM audit; member-miss handling not decoded | **DELTA-CANDIDATE** (low) | Consistent with FB-T02's broader-fallback stance. None required. |
| FB-K05 | rrecov.cpp:503-506 | scan rc != OK | `scan failed: <text> (0x%08x)`, exit 1 | og: engine HRESULT surfaces + UI error states (see FB-O09) | **RETAIN-AS-SAFETY** | Truthful. None. |
| FB-K06 | rrecov.cpp:706-710 | items refused for overlap | `queued N item(s) (M refused: destination overlaps source)` | og: warning-level (D-17) | **RETAIN-AS-SAFETY** | Maps to D-17. |

### 2.12 Header/comment drift (contract comments vs code)

| ID | our file:line | drift | verdict | action |
|---|---|---|---|---|
| FB-L01 | cr_transport.h:23-26 | Comment claims `open_rda_partition` "Tries: `\\.\SVC\<token>\<disk>\<part>`, `\\.\SVC\<token>\<disk>`, `\\.\SVC\<token>\Volume\<n>`" — a cascade. Implementation (cr_transport.cpp:515-538) formats ONLY the 3-segment path; the 2-segment/volume variants exist as separate functions (`RdaClient::open_disk_grammar` / `open_rda_volume`) | **MATCH** (comment wrong) | Fix the comment to describe the single-path function (or implement the cascade if the wizard contract wants it — TRANSPORT-CONTRACT §2.1 documents all three as separate grammar branches). |
| FB-L02 | cr_transport.h:50-52 | Comment claims `volume_letter_to_disk` uses "IOCTL_STORAGE_GET_DEVICE_NUMBER + extents"; implementation (cr_transport.cpp:761-804) uses `IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS` + table-parse for the partition number | **MATCH** (comment stale) | Update the comment. |
| FB-L03 | cr_scan.h:37 | `FsFamily::CarvedOnly = 5` is referenced NOWHERE in src/ (grep: 0 hits) — dead enumerator | **REMOVE** (name) | Delete the enumerator NAME but keep value 5 reserved (`/* 5 reserved (was CarvedOnly; never used) */`) — renumbering would shift Hfsplus/Btrfs/Zfs/Refs (6..9) and break the serialized fs_kind contract documented at cr_scan.h:38-39 and D-02. |
| FB-L04 | cr_retry.h:8-9 | "Skip mode … (opt-in; the default Fail mode reports ERR_RETRY_EXHAUSTED)" — true at API level (`RetryPolicy.mode = Fail`, cr_device.h:79), but the production orchestrator ALWAYS sets Skip (cr_scan.cpp:360) | **MATCH** (comment incomplete) | Amend the comment: "production scans always opt in (cr_scan.cpp); Fail remains the library default." |
| FB-L05 | cr_safewriter.h:86 + PARITY-CORE-DOM.md:287 | `skip_hashless` documented as a verified policy knob; code never reads it | **REMOVE** | Same as FB-R03. |

## 3. Verdict summary

Total enumerated conditionals: **56** (rows above; FB-O03 counted once though it spans retry+scan).

| Verdict | Count | Rows |
|---|---|---|
| MATCH | 22 | FB-T01, T02, T03, T04, T05, O01, O02, O05, O07, O08, O12, S04, C02, C03, C04, H01, H02, H03, B02, B03, B04, G04, P03, K02 (+ comment-drift MATCHes L01, L02, L04 counted separately below) |
| RETAIN-AS-SAFETY | 21 | FB-T08, T10, O06, O10, O13, S01, S02, S03, R01, R04, R05, R06, R07, R08, R09, P01, P02, G01, G02, G03, G05, G06, K03, K05, K06 |
| REMOVE | 6 | FB-T06 (silent GET_INFO tolerance), FB-T07 (blanket session-done), FB-B01 (locked-read → Clear), FB-R03/FB-L05 (dead `skip_hashless`), FB-X01 (optional zstd/lz4), FB-K01 (list exit-0), FB-L03 (dead `CarvedOnly` name) |
| DELTA-CANDIDATE | 10 | FB-O03 (register Skip/retry policy), FB-O04, FB-O11, FB-C01, FB-H04, FB-H05, FB-R02, FB-T09, FB-K04, FB-X01(action-b) |
| UNVERIFIED | 2 | FB-O09 (og whole-scan behavior on structured-engine failure), FB-O07-nuance (og per-candidate rc semantics) |

(Header-drift rows L01/L02/L04 are MATCH-with-comment-fix; L03/L05 fold into REMOVE.)

## 4. Specific questions

### 4.1 Q1 — do we ever silently proceed carved-only when a structured engine fails?

**No, not for the main volume.** `run_engine_pass` propagates every structured engine's rc
(cr_scan.cpp:532-613); `run()` stamps it (cr_scan.cpp:419-440) and `cmd_scan` exits 1 with the
code (rrecov.cpp:503-506) — carve does not run. The only proceed-paths are: (a) fs_hint
Unknown (probe could not identify a family — FB-O02, which matches og's always-registered RAW
searcher on advanced scans), and (b) lost-partition candidates, where per-candidate rc is
discarded (FB-O07/O08) — candidates are speculative volumes in og's model too. The og-side
behavior for a genuine engine failure mid-scan is UNVERIFIED (U-01): corpus shows engine
HRESULT 0x5002 plumbing and a verbose error channel, but not the stop-vs-continue decision.

### 4.2 Q2 — is RetryMode::Skip reference behavior?

**Concept yes, mechanism ours.** The og's EXT4 engine literally implements skip-and-salvage
(`CEXT4CatchBufferDeviceProxy`, 0xE00 catch buffer with 0x400 backtrack — "salvages what it
can instead of failing the whole group scan"), and the product ships user-facing copy for it
(`BadSectorTip: "Program is trying to skip bad sectors to find data."` + `BadSectorTipsDlg`).
Our transport-wide retry+bisect+zero-fill+ReadErrorMap is a generalization, currently
UNREGISTERED in PARITY-DELTAS (D-19 mentions the writer-side zero-fill only). Verdict:
DELTA-CANDIDATE — register it; do not remove. Deviation to note inside the entry: og's
salvage re-reads around the bad spot; ours zero-fills the irrecoverable remainder — the
zero-filled bytes are recorded in the error map (og has no persistent map at all).

### 4.3 Q3 — does the reference ever run without its decompressors?

**No.** BtrfsSearchDLL carries zlib (inflate/deflate/adler32 — capa-verified);
ZfsSearchDLL reimplements lzjb/gzip in-engine. Neither ZfsSearchDLL nor BtrfsSearchDLL has
any zstd/lz4 import or string (imports + decompiled grep = zero hits) — og's ZFS compressed
support is lzjb/gzip-only, always compiled in. Our zstd/lz4 codecs EXCEED og (fine), but our
build-time optionality + never-consulted `codec_available()` is invented degradation → REMOVE
the optionality (FB-X01).

### 4.4 Q4 — health unavailability degradation vs og

Ours: two-transport SMART (ATA passthrough → legacy SMART_RCV) then `ERR_HEALTH_UNAVAILABLE`;
NVMe via protocol-specific query; TRIM via fsutil. Og: five-transport SMART accessor family
(SAT variants, CSMI, NVMe bridge) in the CrystalDiskInfo-lineage DiskHealth companion;
DRWUI renders SSD/TRIM warnings. Degradation is truthful at the library level (explicit
error code); the CLI's exit code is not (exit 0 on total unavailability — FB-H04, fix
pending). No silent health fabrication anywhere (no invented attribute values).

### 4.5 Q5 — BitLocker surface degrades truthfully?

Yes, with one exception. Locked-with-readable-boot-sector: detect → auto-unlock → guidance +
refusal, `ERR_FVE_LOCKED` from the library, manage-bde/PowerShell duality matching
bitlockerlib — all MATCH/registered (D-16). The exception is FB-B01: when the boot sector
itself cannot be READ (typical for a locked volume that denies `\\.\X:` reads), we
misclassify as Clear and proceed — REMOVE (return Unknown and refuse). No silent skip
anywhere else; the F13 no-decrypt directive is honored with a truthful surface.

### 4.6 Q6 — CLI degradation prints vs og's UI behavior

Our prints (`[transport] …`, `[bitlocker] …`, `[note] …`, `[F12] WARNING …`, `SMART
unavailable … - trying NVMe...`) implement the truthful-degradation mandate (D-24 ADR-5);
og surfaces the same classes at UI level (no-admin degraded mode per epmntdrv-trio §6,
unlock dialogs, BadSectorTipsDlg, destination recommendation string). The og has NO
console-output contract to match, so these are UI-substitute surfaces, not slop. The two
truth defects are exit codes: `list` exits 0 on enumeration failure (FB-K01) and `health`
exits 0 on total unavailability (FB-H04) — both REMOVE-class fixes.

## 5. UNVERIFIED register

| ID | Question | Where to resolve |
|---|---|---|
| U-01 (FB-O09) | When a structured engine fails on a volume (HRESULT 0x5002…), does DRWUI stop the scan, continue to RAW, or show a per-volume error while continuing? | Decode the `CDoScanPartition` engine-rc consumer in `ghidra-drwui` (searchdll controller plumbing; the `Return Error:` cout channel is engine-side only). Our current behavior: abort (conservative). |
| U-02 (FB-O07 nuance) | Does og abort a lost-partition scan set when one candidate's FS engine fails, or continue? | Same DRWUI decode, `CDoScanLostPartition` strategy. Our current behavior: continue (matches the speculative-candidate model). |
| U-03 (inherited) | D-17's open action: decode `CRecoveryMgr::ExportThread`'s same-volume branch (refuse-vs-warn granularity). | Already tracked by D-17 NEEDS-WORK; not re-audited here (out of scope: no code change until pinned). |

## 6. REMOVE list (prioritized)

1. **FB-T07** `cr_transport.cpp:449-462` — blanket "any IOCTL failure = stream done, OK" in
   `session_get_next_block`. Replace with a drain-signal-only check (0xC0000120 /
   `ERROR_GEN_FAILURE`); all other failures return an error. Highest priority: it can mask
   real session failures as 100%-complete imaging.
2. **FB-B01** `cr_bitlocker.cpp:19-32/74-80` — boot-sector read failure classified as
   `Clear`. Make `status()` return `Unknown` on unreadable; CLI refuses. Second priority:
   user-visible wrong result on locked volumes.
3. **FB-K01** `rrecov.cpp:227-250/1252-1255` — `list` exits 0 on enumeration failure.
   Propagate a non-zero exit.
4. **FB-X01** `CMakeLists.txt:47-71` + `cr_decomp.cpp` — zstd/lz4 build-time optionality
   (with runtime `codec_available()` consulted by no production code). Make both REQUIRED
   for production builds (already vcpkg deps) or fail loudly per-item on codec-missing.
5. **FB-T06** `cr_transport.cpp:264-269` — `RdaClient::open` tolerates GET_INFO failure and
   proceeds at `ioctl_base_=0`, contradicting TRANSPORT-CONTRACT §9 ("treat as wrong
   driver"). Return nullptr with a wrong-driver error so callers take the direct path.
6. **FB-R03/FB-L05** `cr_safewriter.h:86` — dead `skip_hashless` knob (and the
   PARITY-CORE-DOM.md:287 claim built on it). Delete or wire.
7. **FB-L03** `cr_scan.h:37` — dead `CarvedOnly` enumerator name; reserve value 5, delete
   the name (do NOT renumber — serialized fs_kind contract, D-02).
8. **FB-H04** (fix-class REMOVE) `rrecov.cpp:840-841` — health exit 0 on total
   unavailability; return a distinct non-zero code.

## 7. Top MATCH deviations to note (no code change mandated)

- **FB-T02:** our fallback condition is broader than og's (we fall back on grammar-open
  failure even when the driver is present; og's decoded sites gate only on the availability
  probe). Defensible; register as a remark.
- **FB-O03:** Skip-mode zero-fill vs og's CatchBuffer re-read salvage — register the delta.
- **FB-H01:** our SMART transport set (2) is a subset of og's (5: SAT ×3, CSMI, NVMe
  bridge); additions welcome, not required.
- **FB-K03:** transport labels are prints in ours, UI states in og — sanctioned by D-24.

## 8. What is NOT slop (explicitly cleared)

The driver/direct transport duality (FB-T01..T04) is verified reference behavior
(Device.mo decompiled sites + TRANSPORT-CONTRACT §9) — it stays. Safety hardenings
(destination-overlap refusal, RawFixer consent-only dry-run default with S3 refusal paths,
BitLocker legitimate-unlock-only, CRSF resumable corruption handling, bounded preview) are
all registered deltas (D-16/D-17/D-19/D-20/D-01/D-28) and are NOT degradation slop.
