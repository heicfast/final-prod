# CORPUS-ERRATA.md — corpus errata candidates surfaced by diff-to-fix audit

This file catalogues errata candidates the diff-to-fix audit surfaced in
the `easeus-drw-research` corpus. Each entry cites the corpus file + line,
the claim, the corrected interpretation, and the audit evidence. The
corpus carries its own errata at `re-workspace/artifacts/CORRECTIONS.md`
(8 rows at audit time); this file is the audit's contribution to that
running list.

These errata are NOT applied to the corpus (the audit is read-only on
the corpus). They are recorded for the corpus maintainers to fold in.

## Session 1 contributions (already in the corpus's CORRECTIONS.md if folded)

(See `re-workspace/artifacts/CORRECTIONS.md` for the 8 rows session 1
folded in. Session 1's main contributions were the GUID byte-order
correction (A-001 finding) and the exFAT stream-gate polarity settlement
(N-003 finding).)

## Session 2 contributions (18 new errata candidates)

### From S2-A (NTFS hard-link/ADS/sparse/reparse/$ATTRIBUTE_LIST audit)

**E1. `re-workspace/artifacts/SearchDLL/ntfs-engine--deep-dive.md` §3 lines 77-82**
- **Claim:** "(props->flags & 0x10) == 0" is the "DELETED-FILE BRANCH" and "NTFS in-use flag (0x1 in FILE record flags byte maps to props bit 4)".
- **Correction:** Bit 0x10 of `props+0x23` is the DIRECTORY bit, set at `FUN_180006db0:5329` (`if (uVar4 != 0) { uVar14 = 0x10; local_86 = 0x10; }` where `uVar4 = FUN_180043700(lVar7)` is the directory flag from `$FILE_NAME`). The actual in-use flag is at record header `+0x16` (`rh.flags & 0x0001`), read by `FUN_180043060` and stored as a boolean at `props+0x3d` (`FUN_180006db0:5374-5380, 5426`). The check `(uVar7 & 0x10) == 0` means "NOT a directory", not "deleted".
- **Evidence:** first-hand decompile read of `NTFSSearchDLL_decompiled.c:5329, 5374-5380, 5426`; the audit's own NTFS-5 correction (`cr_mftwalk.h:30-37` comment) already documents this — the corpus prose is the stale reading.

**E2. `re-workspace/artifacts/SearchDLL/ntfs-engine-algorithms-v2--deleted-viability.md` §1 lines 25-26**
- **Claim:** "if record_in_use(rec): // FUN_180041970: parsed+0x20 != 0  continue  // ONLY NOT-IN-USE (deleted) records pass"
- **Correction:** `FUN_180041970` (`NTFSSearchDLL_decompiled.c:52411-52415`) is `return *(longlong *)(param_1 + 0x20) != 0;` — this checks the `base_record` field at offset `0x20` in the `RecordHeader` struct (`cr_ntfs_wire.h:70` `u64 base_record;`), NOT the in-use bit. The correct interpretation: EXTENSION records (`base_record != 0`) are skipped; BOTH in-use and deleted non-extension records pass. `FUN_18001acc0` processes both, and the destroy rule fires for both.
- **Evidence:** first-hand decompile read of `NTFSSearchDLL_decompiled.c:52411-52415`.

**E3. `re-workspace/artifacts/SearchDLL/ntfs-engine-algorithms-v2--deleted-viability.md` §1 line 44**
- **Claim:** "if size >= 0x3FF: # only files ≥ 1 KiB get the viability walk"
- **Correction:** The decompile (`FUN_18001acc0:21208`) shows `if (0x3ff < lVar18)` — strict GREATER THAN (`>`), not `>=`. The core's strict `>` (`cr_mftwalk.cpp:666` `probe_size > flags.viability_min_size` with `viability_min_size = 0x3FF` at `cr_mftwalk.h:58`) matches the decompile. Test `NtfsVolume.ViabilityBoundaryExactFastPath` (`test_ntfs_volume.cpp:1598-1645`) pins this: a 1023-byte deleted file with fully-reused cluster grades `Excellent` (fast-path, no bitmap probe). The corpus's `>=` would probe it and grade `Overwritten`.
- **Evidence:** first-hand decompile read of `NTFSSearchDLL_decompiled.c:21208`; first-hand core read of `cr_mftwalk.cpp:666` and `cr_mftwalk.h:58`.

**E4. `re-workspace/artifacts/SearchDLL/ntfs-engine--deep-dive.md` §6 line 121**
- **Claim:** "0x40 / 0x80 — `$OBJECT_ID` / `$DATA` (checks at 51006/54487)"
- **Correction:** Misleading. Line 51006 is inside `FUN_18003f480` (boot-sector validation, `:50986-51033`), specifically the sectors-per-cluster power-of-two check (`sVar2 == 0x40` is one of the pow2 values tested at `:51006`). It is NOT `$OBJECT_ID` attribute parsing. The og's `FUN_180006db0` does not dispatch on attribute type 0x40. The 54487 cite for `$DATA` is correct (`FUN_1800069e0` iterates type 0x80).
- **Evidence:** first-hand decompile read of `NTFSSearchDLL_decompiled.c:51006` (boot-sector validation context) and `:54487` (`$DATA` iteration context).

**E5. `re-workspace/artifacts/SearchDLL/ntfs-engine--deep-dive.md` §3 lines 85-86**
- **Claim:** "reference-counted property objects (`_Ref_count_base` RTTI; `LOCK()/UNLOCK()` interlocked AddRef/Release at 21170-21205 — the engine is thread-safe for the UI's async workers)"
- **Correction:** Lines 21170-21205 in `FUN_18001acc0` are the viability+destroy block: `:21173` reads `props+0x23` (flags), `:21186` system-flag/`$`-name/null-name filter, `:21190` directory gate, `:21208` viability size gate, `:21217-21248` runlist iteration with bitmap test, `:21249` destroy rule (`0 < iVar21 && iVar20 == iVar21`), `:21256-21262` destroy path (`props->Release()`). The "LOCK()/UNLOCK() interlocked AddRef/Release" interpretation is a misread; those lines are viability logic. (The og DOES use `std::shared_ptr`-style refcounting for `CNTFSFileProperties`, but the cited lines are not where it happens — the actual `AddRef`/`Release` calls are at `:21187` `(**(code **)(*plVar10 + 0x138))(plVar10, 1)` and `:21257`/`:21260`, which are the destroy-path `Release()` calls, not thread-safety primitives.)
- **Evidence:** first-hand decompile read of `NTFSSearchDLL_decompiled.c:21170-21262`.

**E6. `re-workspace/artifacts/SearchDLL/ntfs-engine-algorithms-v2--deleted-viability.md` §1 lines 60-66**
- **Claim:** "bitmap = controller.field_+0x40 or +0x48 # `$Bitmap` objects" and "when NTFS deletes a file it frees the file's `$Bitmap` bits. A deleted MFT record whose data-run clusters still read as allocated in the live `$Bitmap` means those clusters were re-used by other files".
- **Correction:** WRONG per OQ-T1 resolution (already documented in `findings/FIXED/OQ-T1-ntfs-secondary-claim-map-semantics.md`): the bitmap is a scan-built claimed-sector bit-set (ctor `FUN_18002c330`, bits set by `FUN_180017190` at `:18409` marking already-emitted files' run sectors), NOT the volume `$Bitmap`. The "live `$Bitmap`" interpretation is the source of the S-001 finding's original framing; the resolution corrected it.
- **Evidence:** first-hand decompile read of `NTFSSearchDLL_decompiled.c:18409, FUN_18002c330 ctor`.

### From S2-B (timestamp/timezone/Unicode audit)

**E7. `re-workspace/artifacts/SearchDLL/fat-exfat-engine-algorithms-v2.md` §6**
- **Claim:** "100% [CODE] for the FAT16 Next translation table, CDoubleFATTable composition, the FAT32 loop math/cycle-tree/"."-detection, the exFAT dentry type classes + field validations + backward triple walk, and the shared-props finding."
- **Correction:** The artifact is SILENT on timestamp decode (no mention of `DosDateTimeToFileTime`, the tenths-byte discard, or the `tz_offset` handling). Not strictly wrong; the breadth claim could mislead a reader into thinking timestamps are covered. Suggest an explicit "timestamp decode out of scope" note.
- **Evidence:** first-hand read of `fat-exfat-engine-algorithms-v2.md`; first-hand decompile reads of `FATSearchDLL_decompiled.c:32135-32138` and `exFATSearchDLL_decompiled.c:5215-5235`.

**E8. `re-workspace/artifacts/SearchDLL/ext4-hfs-engine-algorithms-v2.md` §3**
- **Claim:** covers B-tree traversal but not the timestamp conversion math.
- **Correction:** Not wrong; same suggestion — add an explicit note that the HFS+ Mac-epoch fold math (`(mac_sec + 9561628800) * 10000000`) is out of scope. The `0x239eae080 = 9561628800` constant appears in `HFSPlusSearchDLL_decompiled.c:8012` and matches the core's `mac_to_filetime` exactly.
- **Evidence:** first-hand read of `ext4-hfs-engine-algorithms-v2.md`; first-hand decompile read of `HFSPlusSearchDLL_decompiled.c:8012, 8027, 8042`.

**E9. `re-workspace/artifacts/SearchDLL/refs-btrfs-zfs-engine-algorithms-v2.md` §2**
- **Claim:** covers superblock generation but not the inode timestamp decode.
- **Correction:** Not wrong; same suggestion — add an explicit note that the Btrfs inode timestamp decode (`(sec + 11644473600) * 10000000 + nsec/100` at inode+0x70/0x78/0x7c/0x84/0x88/0x90) is out of scope. The `0x2b6109100 = 11644473600` constant appears in `BtrfsSearchDLL_decompiled.c:4437` and matches the core's `unix_to_filetime_ns` exactly.
- **Evidence:** first-hand read of `refs-btrfs-zfs-engine-algorithms-v2.md`; first-hand decompile read of `BtrfsSearchDLL_decompiled.c:4430-4446`.

**E10. `re-workspace/artifacts/SearchDLL/ntfs-engine-internals.md` (Record processing section)**
- **Claim:** mentions "$FILE_NAME (0x30) parsed by `CNTFSFileRecordFileNameAttribute` — names, parent references, timestamps" and "$STANDARD_INFORMATION (0x10) — 256-byte structure scans visible at record compare" but does NOT decode the time precedence rule.
- **Correction:** This is the gap session-1 noted as OQ; it remains open. The og's `FUN_180006db0:5338-5341` sets from `$FILE_NAME`, then `:5347-5350` OVERRIDES with `$SI` times if `$SI` exists and is valid. The core's `cr_mftwalk.cpp:734` `props->times = has_si ? si_times : best->times;` matches the decompile.
- **Evidence:** first-hand decompile read of `NTFSSearchDLL_decompiled.c:5338-5350`.

### From S2-C (safety/writer/session audit)

**E11. `architecture/04-recovery-workflow.md §7.5`**
- **Claim:** "writes go to destination via normal file APIs"
- **Correction:** Understates that the og ALSO preserves ACLs via `SetFileSecurityW` (not just data writes). Should cross-link to the `Security_Proc::CFileSecurityInfo` evidence in `RecoveryModule_decompiled.c:78000-78026`. The current text "writes go to destination via normal file APIs" implies a plain byte copy, omitting the ACL-preservation step that the og performs.
- **Evidence:** first-hand decompile read of `RecoveryModule_decompiled.c:74665-74671, 78000-78026`.

**E12. `re-workspace/artifacts/SaveScan/rsf-format--scan-session.md §5.3`**
- **Claim:** record 3 carries a "512-byte boot sector snapshot" at `+0x518`
- **Correction:** Confirmed in decompile. Should note that the core's `VolumeInfo` stores only a CRC32 of the boot sector (`cr_records.cpp:310`, `cr_scan.cpp:560-564`), not the full 512 bytes — a 32-bit collision risk vs the og's byte-exact comparison. The doc currently implies byte-exact identity verification is the standard; the core's CRC32 approach is weaker.
- **Evidence:** first-hand read of `rsf-format--scan-session.md §5.3`; first-hand core read of `cr_records.cpp:310` and `cr_scan.cpp:560-564`.

**E13. `architecture/04-recovery-workflow.md §10`**
- **Claim:** "Locked volumes: EUDCPEPM/EUEDKEPM hook drivers + volume lock via PartitionRecovery pending-op engine"
- **Correction:** This is the repair-path `FSCTL_LOCK_VOLUME` + `FSCTL_DISMOUNT_VOLUME` sequence (`RecoveryModule_decompiled.c:44686-44720`). Should explicitly note that the core's repair path (`cr_transport.cpp:822` `RwDevice`) does NOT perform this lock — a divergence (S-010 finding). The §10 text currently frames it as a generic "locked volumes" handling without flagging the repair-path-specific gap.
- **Evidence:** first-hand decompile read of `RecoveryModule_decompiled.c:44686-44720`; first-hand core read of `cr_transport.cpp:822`.

**E14. `re-workspace/artifacts/SaveScan/rsf-format--scan-session.md §2.1`**
- **Claim:** the og's record header is `Magic[16] + SessionId[16] + chain` (36 bytes) with NO CRC field.
- **Correction:** Accurate for the og. Should note that the core's CRSF format (`cr_crsf.h:32` `CRSF_RECORD_HEADER_BYTES = 24`) uses a 24-byte header (`Magic[4] + type[4] + session_id[8] + chain[4] + crc[4]`) WITH an explicit per-record CRC32 — a hardening superset (F10). The current doc could be misread as "the og has no integrity check at all" when in fact zlib's Adler-32 provides implicit integrity.
- **Evidence:** first-hand read of `rsf-format--scan-session.md §2.1`; first-hand core read of `cr_crsf.h:32` and `cr_crsf.cpp:166,184-198,323-327`.

### From S2-D (performance/quality/API audit)

**E15. `re-workspace/artifacts/SearchDLL/ntfs-engine-internals.md:71`**
- **Claim:** "batch sizes adapt to RAM" (asserted, not cited)
- **Correction:** The `NTFSSearchDLL.dll` imports `GlobalMemoryStatusEx` (verified at `strings_ascii.txt:838`) but the corpus doc doesn't quote the call site. The claim may be correct (the import is present) but the citation is incomplete. Recommend: cite the decompile line of the `GlobalMemoryStatusEx` call + the batch-size decision ladder.
- **Evidence:** first-hand read of `NTFSSearchDLL.dll/strings_ascii.txt:838`; corpus doc cites no decompile line.

**E16. `re-workspace/artifacts/SearchDLL/ntfs-engine-internals.md:73`**
- **Claim:** "Worker coordination via Vista+ condition variables (InitializeConditionVariable / SleepConditionVariableCS / WakeAllConditionVariable — dynamically resolved via GetProcAddress, with pre-Vista fallback)."
- **Correction:** The `NTFSSearchDLL.dll` strings dump shows `GetProcAddress` (line 854) but NOT `InitializeConditionVariable` / `SleepConditionVariableCS` / `WakeAllConditionVariable`. Either the strings are obfuscated (unlikely for CRT API names) or the og uses a different sync primitive (e.g. CRITICAL_SECTION with event objects, which IS in the strings: `InitializeCriticalSection` line 834, `CreateEventW` line 852, `WaitForSingleObjectEx` line 851, `SetEvent` line 849, `ResetEvent` line 850). The corpus claim may be incorrect or applies to a DIFFERENT engine (e.g. DRWUI.exe, not NTFSSearchDLL.dll). Recommend: re-verify which binary actually uses condition variables.
- **Evidence:** first-hand read of `NTFSSearchDLL.dll/strings_ascii.txt` (no condition-variable imports); strings show CRITICAL_SECTION + event objects.

**E17. `re-workspace/artifacts/DRWUI-exe--thread-architecture.md:56`**
- **Claim:** "CommonThreadPoolMgr: the shared thread pool — general-purpose work-stealing pool the one-off threads defer to."
- **Correction:** No "work-stealing" evidence is cited. Core's `cr::ThreadPool` (`cr_parallel.cpp`) is a plain MPMC FIFO, NOT work-stealing. If the og's pool is genuinely work-stealing, that's an algorithmic divergence (work-stealing has better tail-latency under imbalance; FIFO has better cache locality under uniform work). The corpus claim needs a decompile citation; without one, treat as unverified. (Low priority — both designs are valid for the scan workload.)
- **Evidence:** no citation in corpus doc; first-hand core read of `cr_parallel.cpp` (plain MPMC FIFO).

**E18. `architecture/05-scan-engine-architecture.md:122`**
- **Claim:** "EPMNtDrv (\\.\EPRDTDRV\<d>\<p>) — MDL async sector IO, 512B sectors."
- **Correction:** Core's RDA transport (`cr_transport.cpp:42-58`) uses `OVERLAPPED` + `ReadFile` + `GetOverlappedResult` with the F11 unscale (`got == len * kLegacyInformationScale`). The "MDL async sector IO" claim implies the driver uses `IRP_MJ_READ` with MDL — which is consistent with `ReadFile` on a device handle. But "512B sectors" is too narrow: the rda-driver contract supports configurable sector sizes (`session_sector_size_` in `cr_transport.cpp:497`). The corpus claim should be softened to "logical-sector-granular" not "512B."
- **Evidence:** first-hand core read of `cr_transport.cpp:42-58, 497`.

## How to apply these errata

The corpus maintainers should:
1. For each errata, verify the cited decompile/core evidence.
2. Either correct the corpus claim in-place (with a "corrected on
   YYYY-MM-DD per diff-to-fix audit" annotation) or add a footnote pointing
   to this file.
3. Where the errata is "out of scope" rather than "wrong" (E7, E8, E9),
   add an explicit scope note rather than expanding the artifact.
4. Where the errata contradicts a session-1 finding that has since been
   resolved (E6, E10), update the finding's evidence section to cite the
   corrected corpus text.

This file is append-only. Future audit sessions should add their errata
candidates below this line.
