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

## Main-auditor leg addendum (session 2, 2026-10-25)

This leg's independent errata register is filed as
`findings/FIXED/I-011-corpus-errata-session2-register.md` (8 corrected artifact
claims). Overlaps with the register above are convergent reads; unique to I-011:
- **NTFS pass-structure + 0x3ff semantics** — both NTFS artifacts describe
  `FUN_18001acc0`'s loop as "the deleted-file pipeline" without noting it parses
  GUID-dump contents (see S-012), and the artifacts' "small-files fast-path keep" is
  actually a destroy fall-through (`:21208`/`:21256`).
- **NTFS flags-0x10** — `ntfs-engine--deep-dive.md:77-80` ("in-use") vs
  `ntfs-engine-algorithms-v2--deleted-viability.md:41` ("not a folder"): the decompile
  settles it as the $FILE_NAME DIRECTORY bit (`FUN_180043700 :53481`, 0x10000000).
- **ZFS argument order** — `zfs-vdev--mftmirr--calcpartition.md:13` swaps
  `FUN_180018220(dev, &buf, 0x4000, 0x1c000)`'s arguments; the read is 0x1C000 bytes at
  offset 0x4000, and "4 labels × 128KB" is contradicted (front region only).
- **HFS+ journal magic** — session-1's guessed 0x6a6e6c78 → the real gate is raw
  0x4A4E4C78 + endian marker 0x12345678 (`:13542-43`, LE journals only).
- Plus: ZIP two-tier version gates; three carve-census mislabels (`MM`=3DS not TIFF-BE;
  `DI`=MP3 not AIFF; `0xd302020`=DXF not MPEG sync); IOCTL 0x60002400 = GetProcName.

Session-2 self-retraction recorded here for the corpus maintainers: this leg's earlier
"fixer checksum = no-skip" reading was wrong — the skip-table {106,107,112} reading (the
parallel leg's binary extraction) is correct; corrected in the leg's evidence files
(`evidence/decompiled-quotes/fat-exfat-session2.md`).
## Session-3 additions (fed back by the diff-audit session-3 raw-signs decode)

| # | Artifact / claim | Correction | Evidence |
|---|---|---|---|
| 19 | `SearchDLL/raw-dual-registry--u16-magic-keys.md` §1: "the carve dispatch tries the u16 magic-key first (cheap) then falls through to the 4CC table and signature DB [dispatch order INFERRED]" | **Inverted on the first two legs.** The decoded dispatch (`FUN_18000cf60`, RAWSearchDLL :9730-9930; call site VA 0x18000cadc disassembled) is: `config/config.ini [User] only_common` gate → **u32 4CC registry first** (exact map lookup, only when the first u32 != 0) → u16 registry (`FUN_18000d3e0`) → linear walk of the whole registry → raw_signs.json layer LAST (`FUN_18000d670`, the NULL edge). | diff-to-fix `evidence/decompiled-quotes/s3-rawsigns-units-and-dispatch.md` §4; finding N-031 |
| 20 | `SearchDLL/rawsearch-carving--deep-dive.md` §3: "Header+tail+size-gate confidence combination ranks overlapping candidates (raw_signs.json `confidence` fields, e.g. ifo `0,44564456,0.96`)" | The sign-string parser (`FUN_18001e510`, :23522-23765) splits on every comma but consumes **only part[0] (offset) and part[1] (hex)**; the confidence decimal (part[2]) is never read anywhere in the function. The per-sign confidence values are dead data in RAWSearchDLL; any overlap ranking comes from table order, not the decimal field. | diff-to-fix `evidence/decompiled-quotes/s3-rawsigns-units-and-dispatch.md` §3; finding N-030 |
| 21 | `SearchDLL/rawsearch-carving--deep-dive.md` §1 (and the raw_signs layer generally): min/max presented without units | The `min`/`max` values are **KiB counts** in the engine: the consumer compares byte spans against `max * 0x400` and emits candidate sizes as `max << 10`, defaulting to 0x7d000 KiB (500 MiB) when max is 0 (`FUN_1800205b0`, :25200-25240). `max=51200` = 50 MiB, not 50 KiB; `max=204800` = 200 MiB. | diff-to-fix `evidence/decompiled-quotes/s3-rawsigns-units-and-dispatch.md` §2; finding S-015 |

| # | Artifact / claim | Correction | Evidence |
|---|---|---|---|
| 22 | `SearchDLL/rawsearchdll--analyser-registry-census.md`: "II\x1a\x00 / MM\x1a\x00 = Minolta MRW" | `Brance::CRWAnalyser` = Canon CRW/CIFF (RAWSearchDLL :1746-1761); MRW is the separate `\0MRM` u32 key (:2498-2510). | s3-rawsearch-inventory §3/§8 (S3-CARVE full registry decode) |
| 23 | census: "C5 D0 D3 C6 = dBase/DBF" | `BruteForce::CEPSAnalyser` = Adobe Illustrator AI/EPS (format id 0x1b, :1975-1987 — re-verified first-hand by the S3 lead at :1985: `local_res10[0] = 0xc6d3d0c5` immediately after the CEPS vftable store). **No dBase analyser exists anywhere in the decoded registry.** Also corrects S-007's addendum format table (see the S-007 correction addendum). | s3-rawsearch-inventory §3/§8 row 3; S-007 correction addendum |
| 24 | census: "!BDN = Bentley/BDN document" | `FilePSTAnalyser` = Outlook PST/OST (:2317-2328). | s3-rawsearch-inventory §3/§8 row 4 |
| 25 | census: "CF AD 12 FE / F7 89 34 2C = MS-DOS exec / thumbnail-repair variant" | both keys → `RawSpeed::DbxAnalyser` = Outlook Express DBX (:2190-2231). | s3-rawsearch-inventory §3/§8 row 5 |
| 26 | census: "3F 5F 03 00 = Nikon NEF/other" | `FileHLPAnalyser` = Windows Help (:2390-2401). | s3-rawsearch-inventory §3/§8 row 6 |
| 27 | census + `raw-dual-registry--u16-magic-keys.md` §2: "E4 52 2C 7B = LZH arc" | Key is `0x7b5c52e4` = bytes **E4 52 5C 7B** (not 2C) and maps to `RawSpeed::OneAnalyser` + `FileONEAnalyser` (format id 0x31, :2156-2177) — the OneNote `{E4525C7B-…}` GUID prefix. LZH is a LIST-layer structural analyser (`Brance::LZHAnalyser`, :3097/:46896+), not a signature key. | s3-rawsearch-inventory §3/§8 row 11; S-007 correction addendum |
| 28 | `raw-dual-registry--u16-magic-keys.md` headline: "39 u16 keys" | 35 registrations / 33 unique u16 keys (the full registration walk). | s3-rawsearch-inventory §4 |

*Convergence note (2026-10-26): the S3-CARVE leg's rows for `MM`=3DS-not-TIFF-BE and
`DI`=MP3-not-AIFF are the same corrections the session-2 main-auditor leg recorded in
I-011 — dropped here as duplicates; two independent decodes agreeing is positive
evidence for both. Rows 19-21 above (raw-signs decode) and this register's rows 22-28
(S3-CARVE registry decode) are complementary: 19-21 = scan-loop mechanics, 22-28 =
per-key analyser attributions.*

## Session-4 additions (fed back by the diff-audit session-4 grading/preview/factory/driver decode)

| # | Artifact / claim | Correction | Evidence |
|---|---|---|---|
| 29 | `DRWUI-subsystems--assistant-grading-preview.md` §2: "node.type == 0xC: grade = 2 # ZFS special-case" | **0xC is NOT ZFS.** `FUN_140088f00` reads the CATEGORY byte (`*(byte*)(node+0x70)>>2`); the category mapper `FUN_14039aa40` (:614863-614904) rewrites type 6→0xC on bit 35 of node+0x89 — 0xC is a derived category value. ZFS is FS code **0xD** (string map :955212-955227: "btrfs"→0xB, "ext4"→9, "zfs"→0xD). The mechanics (0xC→grade 2, guarded by grade != 2) stand; only the label was wrong. | diff-to-fix `evidence/decompiled-quotes/s4-crecoverability-grading-engine.md` §3 corrections (a) |
| 30 | Same artifact §2: "grade adjustment: device.kind ∈ {1,4} → offset −2" | The **−2 is an extent-address offset**, not a grade adjustment: `local_a8 += -2` feeds the extent-end key `base+len+(idx−2)×unit` for the overlap tree (:605752-605768, :605924-605939). Grades are only 0/1/2 and the setter cannot represent −2. | same evidence §3 |
| 31 | Same artifact §4: "GlobalAlloc(GMEM_MOVEABLE, …)" | Flags are **0x42 = GMEM_MOVEABLE|GMEM_ZEROINIT** (:654559). | same evidence / `evidence/decompiled-quotes/` preview quotes |
| 32 | `SearchDLL/engine-matrix--factory-contract-verification.md`: (a) the EXT4 note "decompile grep miss = symbol-table naming artifact"; (b) "No engine deviates from the contract except the Btrfs spelling"; (c) HRESULT return | (a) **REFUTED** — `EXT4SearchDLL_decompiled.c` contains both symbols verbatim at :11 and :44. (b) `EUCreateReFSQuickSearcher` deviates in **arity** (3 params, the extra two forwarded into the 0x210 sub-object ctor) and RAW's 6th export is literally named `GetRawUserInfoMap` (not an EUGetAnalyser* name; and it is the only registry export DRWUI consumes). (c) The factories return void (or 0), failure = null out-slot — no HRESULT. | diff-to-fix `evidence/decompiled-quotes/s4-driver-client-and-factory-contract.md` §5 |
| 33 | `architecture/02-kernel-driver-architecture.md` + the driver-internals artifact: the EPRDTDRV client grammar documented WITHOUT the PID-token component | The grammar is `\\.\EPRDTDRV\{PID-TOKEN}\{disk}\{part|Volume\{n}}` — the driver validates an in-kernel cascading byte-sum PID token at IRP_MJ_CREATE (:262-291); the client mangles GetCurrentProcessId identically (`Device_decompiled.c:50643-50652`). | same evidence §1-§2 |
| 34 | Driver IOCTL range hypotheses (0x2e000-0x2e0ff) | The custom IOCTLs are **0x60002400 / 0x60002404 / 0x60002408** (CTL_CODE(0x6000, 0x900..0x902, METHOD_BUFFERED, FILE_ANY_ACCESS)) + pass-through for everything else; READ completes Information = Length<<9 (512-byte sector units). | same evidence §1 |
| 35 | Any future reading of "the corpus 4 MiB preview window" (this phrase originates in core comments, not corpus docs) | There is no 4 MiB preview cap anywhere in the og. The preview payload cap is **1 MiB** (0x100000, both gates :654557/:654601); the 4 MiB constant (`0x400000/sectorSize`, Device.mo:56545) is the **device read-batching window** applying to all device reads. Also `IInWindowStream` (DRWUI-only) is the 7-Zip/LZMA SDK input interface, not a preview contract. | same evidence §4; core finding M-024 |
| 36 | `searchdll-family--architecture.md:13-15`: factory returns HRESULT | See #25(c): void / null-out-slot. (Folded with 25 for the maintainers' one-pass fix; kept as its own row for the two distinct artifacts.) | same evidence §5 |


## Session-5 additions (fed back by the diff-audit session-5 MOM-capability/decode wave)

| # | Artifact / claim | Correction | Evidence |
|---|---|---|---|
| 37 | `mo-module-map.md:21` + `module-internals-wave2--ldm-reglib-ctbfile.md:89-96`: CTbFile = "TB-scale disk-IMAGE file abstraction / .tbi mount layer" | **CTbFile is a generic refcounted Win32-file wrapper; its ONLY construction site reads boot.ini** (DeviceManager_decompiled.c:51016-51086: path `L"\\boot.ini"` :50993, INI parse of "boot loader"/"default" :51120-51125; no extension/format check in the open path :48160-48245). "Tb" = Todo-Backup lineage (Burn.dll's CTbCreator). The disk-image capability lives in DRWUI CImageFileDevice (extension factory :948237-948409) + Device.mo/libvmdk (VMDK reads :49106-49124) — see finding S-030. | diff-to-fix `evidence/decompiled-quotes/s5-image-fscheck-persistence.md` §A |
| 38 | `mo-module-map.md:35/:54-55`: CFatVirtual{View,DirItem,FileItem} = "virtual FS views over damaged disks"; CFirstVolumeMergeDeal = "merges split volumes" | **Both are EPM partition-management, not recovery**: the CFatVirtual family is the `Ntfs2FATProc` conversion engine (namespace log string NTFSUtil:34084; fabricates a FAT32 DBR from template :34176-34204; WRITES FAT chains :32924; decompress/decrypt of the live NTFS source :28199/:28288) — a write path with no damaged-FAT reading. CFirstVolumeMergeDeal merges partition B into partition A for the EPM CMergePartition op (:34519; siblings :89583/:37994). Registered as scope exclusion in finding I-019. | same evidence §G |
| 39 | `mo-module-map.md:38`: EFIBoot.mo = "EFI System Partition handling" | **CEFIBoot manipulates UEFI NVRAM variables, not ESP files**: Get/SetFirmwareEnvironmentVariableW dynamically resolved (EFIBoot:3281-3290), EFI global GUID {8BE4DF61-93CA-11d2-AA0D-00E098032B8C} (17 sites :3385-4208), Boot####/BootOrder create/patch/delete state machine; no CopyFile/CreateFile in the module. | diff-to-fix `evidence/decompiled-quotes/s5-mom-capabilities.md` §3 |
| 40 | `mo-module-map.md:39` + `MOM-modules/Fixup.md:5`: Fixup.mo loaded "by the MOM contract" | **Fixup.mo is a link-time shared library** (36 exports: IObject family + 13 Bcd_* + 6 BootIni_* + 3 Log_*; no GetModule) imported by six hosts (MainModule, RecoveryModule, ToolsModule, UILogic, CloneModule, PartitionRecovery.dll) — consistent with the dependency matrix's own link-time list. | same evidence §1 |
| 41 | `mo-module-map.md` insight 7 + S-006's scoped remainder: "FATX parser — Xbox-original FATX filesystem support (legacy EaseUS surface)" | **There is no Xbox FATX parser. "Fatx" = FAT-x**: CFatxBlockParser is the FAT12/16/32 boot-sector classifier (parse FUN_180006250 PRD:3553-3595; FATXDBR in FatLib.mo is the classic BPB FatLib:594-690); the Xbox magic 0x58544658 appears nowhere in the corpus (definitive negative grep over reversed/ + og-codebase/). The parity surface is the FAT gate family → finding M-045; S-006's remainder is dissolved. | diff-to-fix `evidence/decompiled-quotes/s5-image-fscheck-persistence.md` §F + S-006's session-5 addendum |
| 42 | `tracked-open/OQ-S4-1` (session 4): the grading extension table is "a sorted string-map blob" in ".rdata" at 0x141bb0500 | The table is in **`.data`** (RVA 0x1bb0500 → file offset 0x1baef00) and is **unsorted** (static build order; the 2000-iteration loader loop DRWUI:605297-605304 inserts into a runtime map that sorts). 169 entries, 9-byte null-padded ASCII stride, first "Raw", last "COMMON" — full dump filed. | diff-to-fix `evidence/decompiled-quotes/s5-oq-s4-1-extension-table-dump.md` |

| 37 | `ext4-hfs-engine-algorithms-v2.md` :78-82: "records grow from the END of the node backward" (the artifact's own marker: "[CODE; INFERRED]") | **INVERTED.** Records grow UP from offset 14 (TN1150; record 0 always at 14, deeper offset slots hold HIGHER values, deepest slot = free-space start). The note was inferred from the CORE's code, not the reference; the og's executable journal-image validator (FUN_180010720 :13094-13108) hard-gates the records-UP shape (u16@node_end-2 == 14 + ascending offsets), and a shipped harvester would find nothing on records-DOWN volumes. Core impact was ACUTE A-005 (fixed): the core's reader/fixtures shared the inversion. | diff-to-fix `evidence/decompiled-quotes/s3-hfsplus-inventory.md` §3a + findings/FIXED/A-005-*.md |

## Session-5 additions (fed back by the never-cited-artifact reading wave)

| # | Artifact / claim | Correction | Evidence |
|---|---|---|---|
| 37 | `SearchDLL/rawsearchdll--family-*.md` deep-dive family files + `rawsearchdll--zip-analyser-carving.md`: "File0001.jpg lives in the UI" (naming-policy claim) | **No FileNNNN scheme exists anywhere in the og.** Carve names are per-analyser at carve time: MP4/MOV template "Lost Name File (<mvhd metadata>)_EaseUS" (RAWSearchDLL:33408/33447), creation-time format :33531, content-embedded names via CZeroEndRawFileInfo (:35668). The vtable+0x48 hook (DRWUI:917292) is a ONE-SHOT per-scan "filter_generalraw" config fetch (RAWSearchDLL:9055-9148), not a namer. | OQ-S4-2 resolution; finding N-063 |
| 38 | Family-file roster counts vs the census §4 (`rawsearchdll--analyser-registry-census.md`): five conflicts | The per-family class counts disagree with the census in five places (12 vs 22, 23 vs 24, 47 vs 46, 15 vs 20, 5 vs 8 — family file vs census). The census's own counts were registry-decode-verified (S3-CARVE); treat the census as authoritative and the family files' rosters as approximate. | session-5 family-file sweep (all 6 family artifacts read; conflicts tabulated in the session scratch context sheet) |
| 39 | S-023's own text + OQ-S4-1: "the 169-entry table at .rdata 0x141bb0500" | The table is in **`.data`** (0x141b9c000..0x142644248), file offset 0x1baef00; `.rdata` ends at 0x141b9bf3c. Structure: char[9] strided C-strings, 169 keys + sentinel, consumed via last-dot substring → ASCII-uppercase → exact set::find (FUN_14038ac90). | `evidence/decompiled-quotes/s5-grading-extension-table-dump.md` |
| 40 | The 8 MOM-module census artifacts (Error/Thread/Common/Config/CheckVersion/Log/bitlockerhelp/FileSystemCheck) read as if they contained behavioral tables | All are **structural censuses** (RTTI lists, vftable counts, string histograms, function-size tables). No error-code tables, no pool sizes, no priorities live in them; the behavioral semantics live in the cross-linked semantic artifacts (05-scan-engine, mo-module-map, DRWUI-thread-architecture). Read the censuses for class inventories only. | session-5 MOM read (context sheet C) |

**E43. `re-workspace/artifacts/SearchDLL/fat-exfat-engine-algorithms-v2.md` §3 row labels**
- **Claim (as labeled):** the `(t-3)&0x7F==0` row is the "volume-label triple" and the `(t-5)&0x7F==0` row is the "allocation-bitmap dentry" check.
- **Correction:** the BYTES are right, the LABELS are wrong. The 0x03/0x83 triple ([+0x20]=0x81, [+0x40]=0x82) is the real-root opening sequence [VOLUME LABEL 0x83][BITMAP 0x81][UPCASE 0x82]; the 0x05/0x85 row is the FILE dentry set check. The og keys the allocation-bitmap shape validation on 0x81 and the upcase shape on 0x82 (decompile :24380-24460: case -0x7f checks reserved@+2..+0x13 zero + cluster@+0x14/length@+0x18 with flags@+1 free = BITMAP; case -0x7e checks reserved1@+1..+3 + reserved2@+8..+0x13 zero with the +4 checksum field free = UPCASE). Session-11's S-033 false positive was built on these labels before the decompile read settled the mapping — folded back so the next session does not repeat it.
- **Evidence:** first-hand read of `exFATSearchDLL_decompiled.c:24380-24460` + `:1847`; the convergent deep-dive §2 ("Allocation Bitmap directory entry (type 0x81)"); findings/FIXED/S-033 (INVALID) carries the full chain.

**E44. `re-workspace/artifacts/SearchDLL/rawsearchdll--zip-analyser-carving.md` entry-point table (LFH row)**
- **Claim (as labeled):** LFH sanity gates are "version_needed <= 10 (offset +8)" and "extradata/flags field at +0x1a in [1, 0x102]".
- **Correction:** the BYTES are decompile-exact; the LABELS/BOUNDS are off. In the spec LFH layout +8 is the compression method (version_needed sits at +4) — the og gates the bytes at +8 (real archives pass: method 0/8), and the second gate's accept window is [1, 0x103]: the decompile rejects when `0x102 < (u16@+0x1a - 1)` (:18385-18388), i.e. accepts through 0x103, not 0x102. Gating the +4 version field (as the label invites) would reject every modern zip — the label is a transcription hazard of the same class as E43's exFAT row labels.
- **Evidence:** first-hand read of `RAWSearchDLL_decompiled.c:18383-18388`; findings/FIXED/N-068 carries the full chain; the fix gates the bytes (decompile-exact), not the labels.

**E45. `re-workspace/artifacts/SearchDLL/zfs-engine--deep-dive.md` §2 ("selects the highest-transaction-group valid uberblock")**
- **Claim (as labeled):** the engine "selects the highest-transaction-group valid uberblock — exactly how ZFS import finds the newest pool state."
- **Correction:** the cited comparison (`plVar12[-0x7e] <= plVar12[2]`) is a NEIGHBOR-txg comparison inside the ring scan (slot-1KiB-back vs current), not a newest-only selection. The caller-level semantics — decoded first-hand from the quick-searcher run loop — iterate EVERY uberblock slot in the label-0 ring and walk EACH txg's tree: FUN_180006b90 :5685-5841 (per-slot `txg=*(u64*)(slot+0x10)` → FUN_180003150 per slot, "\n\ntxg = %llu, last_txg = %llu" progression log), FUN_180003150 :1967-2259 (per-txg `VdevLoc::UberData` registry via FUN_180017310's txg→`std::_Ref_count_obj<VdevLoc::UberData>` map; tree-open FUN_180016c50; "from_txg: 0, birth_txg: <txg>" success log with the unbounded from_txg=0 descent FUN_18001ac60), ring loader FUN_180018930 (`FUN_180018220(dev,&buf,0x20000,0x20000)` = label-0's 128 KiB ring, hardcoded 1 KiB slots). The deep-dive's "newest-only" reading fed core D-05's parity decision — refuted and filed as findings/SEVERE/S-034.
- **Evidence:** first-hand decompile read (DFX30, brace-depth-annotated control-flow extraction); findings/SEVERE/S-034-zfs-ring-txg-history-not-walked.md carries the full chain.

**E46. `re-workspace/artifacts/SearchDLL/remaining-engines--ext4-hfs-refs-btrfs-zfs.md` §BtrfsSearchDLL + `btrfs-engine--deep-dive.md` (the capa table rows)**
- **Claim (as labeled):** "RC4 PRGA (encrypted-dataset support)" — capa-verified.
- **Correction:** the capa rule "encrypt data using RC4 PRGA" false-fired on FUN_180012190, which is the **zlib inflate cluster's sliding-window pump** (dual u16 offset arrays rebased by consumption; adler32 updater FUN_180014c00 with the mod-0xFFF1 dual sums; table-CRC32 updater FUN_180014ee0; the DEFLATE min-lookahead 0x106 and the DAT_180019cc0 length-symbol table in the emit path FUN_1800143a0). No RC4 KSA/PRGA exists anywhere in BtrfsSearchDLL (no 256-iteration key-indexed swap, no S-box state — grep-verified first-hand). The "encrypted-dataset support" reading was capa-rule-name over-read; the family doc's own "?" flag was right. Filed as the OQ-S6-2 resolution.
- **Evidence:** first-hand read of BtrfsSearchDLL_decompiled.c :13797-13904, :15861+, :15997+, :15770-15795, :13906+; capa_capabilities.txt (the XOR basic-block matches 0x18000E0E0/0x180014F31/0x180015160 = the trigger family).
**E46. `re-workspace/artifacts/SearchDLL/remaining-engines--ext4-hfs-refs-btrfs-zfs.md` §BtrfsSearchDLL (capa digest line)**
- **Claim (as labeled):** "Decompression: ZLIB inflate (Btrfs zlib compression) + RC4 PRGA (encrypted-dataset support) + CRC32/adler32 (Btrfs checksums csum/csum_item) — capa-verified."
- **Correction:** the RC4 half is a capa FALSE POSITIVE on function 0x180012190 (the only match for the RC4 rule in this binary). First-hand disassembly of the full 469-byte function: it is a chunked checksum-verified copy pump — per-chunk dispatch at stream+0x2c: type 1 → FUN_180014c00 = adler-32 (0xFFF1 modulus, 5552-blocking), type 2 → FUN_180014ee0 = slicing-by-4 table CRC (DAT_18001a680/1aa80/1ae80/1b280); the rule-matched loops are u16 length-table maintenance (`movzwl -0x2(%rdx); sub; cmovb; mov %ax,(%rdx)`). No `and $0xff` exists anywhere in the function (RC4 index arithmetic is impossible), no 256-entry state/KSA exists anywhere in the decompile (zero `% 0x100` expressions), no keystream XOR, no key material, no rc4/arc4/crypt/cipher strings in the binary. BtrfsSearchDLL has NO RC4 and NO encrypted-dataset support.
- **Evidence:** first-hand disassembly (objdump 0x180012190-0x18001236d) + decompile read (:15861-15992 adler, :15997-16024 CRC) + whole-binary pattern/string scans; `diff-to-fix/evidence/decompiled-quotes/s25-oq-s6-2-btrfs-rc4-false-positive.md`; OQ-S6-2 RESOLVED on it.
- **Convergence (DFX30, same day):** independently decoded with the same conclusion; one additional identification recorded in the OQ-S6-2 convergence addendum — the pump serves the zlib-inflate cluster (FUN_180012370's 0x106 DEFLATE lookahead bound + FUN_1800143a0's DAT_180019cc0 length-symbol tables), matching capa's other hit (0x180011550, "ZLIB inflate or deflate").

## Scan-engine session (Agent 2, 2026-10-28) — 2 new errata candidates

**E-A. `re-workspace/artifacts/config-files/userdata-remoteconfig--signature-and-flags.md` §1**
- **Claim:** EUUserData is "the user-extensible signature database (custom RAW
  signatures / user-extensible **carving**)" — i.e. the file behind the
  "add custom file type" feature as a CARVING feed.
- **Correction:** EUUserData never reaches the RAW carving engine. Its sole
  consumer is DRWUI's `CHKRepairFileSign` (RTTI: `.?AVCHKRepairFileSign@@`):
  read at `%s/EUUserData` (DRWUI :1005387), JSON-parsed (:1005497+,
  `_stricmp "signs"` :1005818), stored in the by-offset/by-ext maps, and
  consulted by the MatchedFile/NotMatchedFile file-signature checker
  (:721520-721680) — the CHK repair chain. RAWSearchDLL has zero EUUserData
  references; the engine's runtime-loaded signature data is raw_signs.json
  (read-only, :23836) + config/RawConfig.ini (:13133) only.
- **Evidence:** first-hand decompile chain (RAWSearchDLL_decompiled.c rg =
  0 EUUserData hits over 80,614 lines; DRWUI_decompiled.c:1005387, :1005497+,
  :1005818, :1002677, :721520-721680; DRWUI.exe RTTI strings CHKRepair*).

**E-B. `re-workspace/artifacts/SearchDLL/rawsearch-carving--deep-dive.md` :61 area**
- **Claim:** the user-info-map/"consulted-alongside" phrasing implying
  EUUserData's signs are consulted by the RAW engine at scan setup (also
  repeated by the M-065 finding's corpus references and the FIX-6 probe note).
- **Correction:** `GetRawUserInfoMap` returns the singleton's +0x90 member = a
  `std::map<std::wstring, count>` of per-extension carve hits, populated
  insert-or-increment by every analyser detection (FUN_18000ed00 :11376-11525;
  first hit creates value 1, repeats increment `plVar10[8]+1`), CLEARED at
  every searcher init (FUN_18000beb0 :9044-9051), and read one-way post-scan
  by DRWUI's report builder into the "Trigger_Raw"/"FileSystem" report tree
  (:227538-:227625). It is scan OUTPUT (statistics), never scan INPUT.
- **Evidence:** first-hand insert-side + consumer-side decode (both directions
  of the same map); the FIX-6 reading had the data flow inverted.
## SA-6 probe-lane rows (2026-10-28 — the OQ-SA-2 resolution's corrections; same evidence bar: decompile cite or mechanical recount)

- **E47** (rawsearchdll--analyser-registry-census.md, the SQLite row): the `SQLi` key (0x694C5153) is NOT a "multimap collision w/ DB chain" — the registry initializer (:1962-1971) constructs CFCPEvtAnalyser (id 0x73) and binds it DIRECTLY to the key; CFCPEvtAnalyser IS the og's SQLite analyser (the class is named for its target: FCP event bundles are SQLite databases). The deep walk additionally relabels the id to 0x75 on the 17-byte "tableactiveactive" schema marker (FUN_18002dfc1) — the FCP-event-database subtype (filed as the core's N-089).
- **E48** (rawsearchdll--family-special.md, the WPG row): "re-validates `*param_2 == 0x677077` → stamps canonical size 0x45" — 0x45 is the FORMAT-ID stamp (FileInfo+0x18, the same slot RAR stamps 0x3A), NOT a size; the WPG size bounds live on the FileInfo: min 0x14 (20 B) at +0x110, max 100,000,000 (100 MB) at +0x118 (FUN_180014030 + the WPGRawFileInfo ctor).

## SCAN-2f niche-batch rows (2026-10-28 — the S-007 batch-1 first-hand registration reads; same evidence bar)

- **E49** (rawsearchdll--analyser-registry-census.md, the HLP cluster row): the key `3F 5F 03 00` (0x00035F3F, retdec :6077) is the CANONICAL WINDOWS HELP magic (the FileHLP cluster, node idx 32) — the census's "Nikon NEF/other" label there is wrong; NEF is TIFF-based and rides the II*/MM* multimap. Consequence: the core implements the key as HLP (FMT_HLP3, shared validate_hlp).
- **E50** (rawsearchdll--analyser-registry-census.md + raw-dual-registry--u16-magic-keys.md, the DXF row): DXF is a KEYED u32 format — key `0x0D302020` (bytes "  0\r") at retdec :5555, node idx 25, a TWO-analyser chain (g146 + g144). The census/u16 artifacts' "list-layer" label is wrong (the ID-conflict note that paired 0x19 with "MPEG-audio" was mis-attributed: 25 = 0x19 = DXF). Consequence: the core keys DXF on the 4-byte magic + the "  0\r\nSECTION" covenant (FMT_DXF).
- **E51** (rawsearchdll--analyser-registry-census.md, the MDB row): the MDB analyser is reached through the U16 registry, key 0x0100 (bytes `00 01`, the first two Jet header bytes) — NOT a u32-keyed format. The full gate is the 20-byte "Standard Jet DB\0" signature (FUN_180015260: key re-check + "dard" u64 @+8 + " DB\0" u32 @+0x10), type-id 39, FileInfo bounds {0x1000, 0xBEBC200} (ctor :21316-21318). The census's "key 0x100 (00 01 00 00)" conflates the u16 key with the full first-dword.
- **E52** (rawsearchdll--analyser-registry-census.md, the TTF row): FileTTFAnalyser rides the SAME u16 0x0100 chain as MDB (a genuine two-analyser multimap on one key — the sfnt version word 00 01 00 00 and the Jet header 00 01 00 00 share their first two bytes). The census's "0x10000/0x100 key space" phrasing mixed the ICO u32 key (0x00010000, bytes 00 00 01 00) with this u16 0x0100 chain.

## SCAN-4 rows (2026-10-28 — the A2 context-recovery re-run's first-hand registration reads; same evidence bar)

- **E53** (rawsearchdll--analyser-registry-census.md, the !BDN row): the key `!BDN` (u32 0x4e444221, file bytes 21 42 44 4E) is NOT a "Bentley/BDN document" — it binds **FilePSTAnalyser, id 0x38** (the Outlook PST magic; registration :2324-2330 first-hand). The validator (FUN_180041eb0 :50000-50017) reads byte@+0xa: ==0x0e → size=u32@0xa8; ==0x17 → size=u32@0xb8; else size=0x100000. Consequence: the core's FMT_PST (keyed 21 42 44 4E) was correct; only the census label was wrong.
- **E54** (raw-dual-registry--u16-magic-keys.md, the 0x4449 row): the u16 key 0x4449 (`ID`) is NOT "AIFF/IANA-family 'ID'-led formats" — it chains **BruteForce::MP3Analyzer → FileMP3Analyser, id 0x2b** (the ID3 lead-in; binding verified :2556-2563, ctor FUN_1800304f0 :37067-37090 first-hand). AIFF keys u32 `FORM` (0x4d524f46) → CFileAIFAnalyzer id 0x50 (verified :2409-2415). The artifact's own :95 "AIFF/'ID'" cross-reference sentence inherits the correction.


## Session-8 wave-2 rows (2026-10-28 — the continuation lane's batch-2 decode + the batch-1 citation truth pass; same evidence bar: decompile cite or mechanical recount)

- **E55** (registry geometry; supersedes the S-007 batch-1 landing's region cites): the u32 keyed registry's bindings live at **:1728-2509 (59 entries)** and the u16 registry's at **:2536-2911 (35 entries)** — total **94 keyed bindings**; the list layer is :2919-4214 (unconditional ~2930-3350 + config-gated 3356-4110). The landing's cited regions ":5450-6600"/":6205-6590" are INI-parsing and std::streambuf code (N-099; evidence/decompiled-quotes/s8-autosave-and-stamps.md §7).
- **E56** (rawsearchdll--analyser-registry-census.md, the `E4 52 5C 7B` row): that key (0x7B5C52E4, :2175-77) is **OneNote** (RawSpeed::OneAnalyser + FileONEAnalyser, GUID-suffix gates :17177-78) — the census's "LZH arc" label is wrong (LZH is list-layer with the u32@+2 `-lhX` probe, :46962-65).
- **E57** (census, the `II\x1A`/`MM\x1A` rows): keys 0x1a4949/0x1a4d4d (:1751/:1759) bind the **CRW (CIFF)** chain (Brance::CRWAnalyser, "HEAPCCDR" grammar :46148-80) — corroborates M-081's misattribution finding from the registration side.
- **E58** (census, the New3DS row): `RawSpeed::New3DSAnalyser` + `File3DSAnalyser` (both ID 1, gates u16@0=='MM' AND u16@+0x10==0x3D3D, :14535-45) is the **Autodesk 3D Studio .3ds** format — "New" = new-generation analyser, not the Nintendo console.
- **E59** (census, the XML/SVG rows): there is no dedicated CXMLAnalyser; the four u32 keys (`<?xm`, UTF8-BOM'<', 0x1cd, `<svg`, :1811-1835) all route into **Brance::SVGAnalyser**, whose string probes discriminate SVG (0x5b) from GPX-class XML (0x13) (:49642-702) — a multi-format router on one analyser.
- **E60** (the WMF placeable convention; the S-007 batch-1 claim): the og's checksum gate is `word[10]@+20 == XOR(words 0..9)` with a zero dword at bytes 16-19 and the standard header at +22 (FUN_1800264d0 :29826-37) — NOT "stored word = XOR of words 0..8". Filed as **S-042** (the core implements the wrong grammar).
- **E61** (FUN_180015260 decode): the u16 0x0100 chain's validator accepts a SECOND signature — `"Standard ACE DB"` → type **0x4C (ACCDB)** (:16515-17), beside "Standard Jet DB" → 0x27. The census's MDB row carries only the Jet signature. Filed as **M-084**.
- **E62** (rsf-format--scan-session.md §5.5 area + DRWUI decode): the auto-save trigger carries **results-found and not-already-saved gates** (call sites :612583/:615166/:616764) and the entry gate is a **byte flag** on a config singleton (:442822-24, FUN_1400b5270 :165951-5) — not an unconditional completion write. Filed as **S-044**.
- **E63** (the WPG ctor cite, correcting also N-093's inherited cite): the WPG bounds {20, 100,000,000} are stamped in the `RawSpeed::WpgAnalyser::WPGRawFileInfo` ctor at **:15700-15701** — the ":20542-20548" cite (batch-1 landing + N-093) points at `FUN_18001a850` (the FCPEvt content extractor). Verified first-hand both regions this session.
- **E64** (the W1a session-list phrasing): the og's list reads **record 1 (session state) THEN record 2** — record-1 also has a WinPE variant layout (:432753) and size-progressive field gates; the "record 2 only" phrasing under-reported record 1's role. The core's read_summary matches the og's actual behavior — phrasing-only correction.


## Session-8 third-lane rows (2026-10-28 — the driver lane + the CHM/FH11 decode convergence; same evidence bar)

- **E65** (rawsearchdll--analyser-registry-census.md, the 0x11c/284 gloss at :99): the numeric key 284 (0x11C, magic bytes `1C 01 00 00`) is **FreeHand 11** — the registration site maps it to `BruteForce::CFH11Analyser`, format id 0x1D (retdec :5654-5665 / ghidra :1951-1962 / asm `0x1800035a1-0x1800035ea`; grammar u64@+4 == 0x02001401_1C040002, u32@+0xC == 0x011C1400, u16@+0x22 == "11"). The census's "(ICO sizes, MTS/ADTS sync)" gloss is a misattribution — MPEG-TS has NO magic key: CMTS/CM2TS are list-only brute-force scanners (constructed at `0x1800053ae-0x180005488`, no map entry). Corroborated convergently by two session-8 lanes and folded into S-041's addendum.
- **E66** (retdec_decompiled.c, the :22635 region — a tooling caveat for future reads): retdec prints 64-bit constants in this function **truncated to their sign-extended low dwords** (`0x7c01fd10`, `-0x5ffff362`), while the binary's actual compares are genuine `mov rax,[mem]; cmp rax,[stack-const]` u64 pairs (asm `0x180016b40-0x180016c14`). The batch-2 CHM blocker ("u64-widened GUID compares conflict with the canonical ITSF v3 layout") was an artifact of this printing behavior, not a real conflict — the compared GUIDs are the canonical ITSF v3 pair {7C01FD10-7BAA-11D0-9E0C-00A0C922E6EC}/{7C01FD11-…E6EC} at +0x18/+0x28. **Lesson: never adjudicate compare-width from retdec's constant rendering alone — read the asm or ghidra's full-constant rendering.**


## Session-8 lead-lane rows (2026-10-28 — the final push-window reconciliation)

Convergence note: the lead lane's five read legs independently decoded the same census
corrections as the remote E53-E66 set (the ONE/LZH key flip, the CRW/MRW flip, the New3DS
row, the XML/SVG router, the WMF convention, the 284=FH11 gloss, the CHM compare-width
artifact) and the same DBF/EPS + dispatch-order corrections already carried by rows 19/23
of the older errata table — the lead lane's duplicates are dropped in the merge; the
remote rows stand. **One distinct row the remote set does not carry:**

- **E67** (the S-007 batch-1 addendum's DMG row, audit-side — filed as **M-090**): "cap
  mirrors the og's own 0xBEBC200" is wrong — 0xBEBC200 is the GLOBAL singleton carve cap
  (RAWSearchDLL :9049); **DMGRawFileInfo's own bound is 0xC800000 (209,715,200)**
  (:49436-49440, read first-hand), and the og's FileDMGAnalyser extent walk targets the
  **"koly" UDIF trailer** (needle 0x796c6f6b at :63806, an 8-zero pre-gate :63843-63844,
  **size = hit + 0x200** :63847 — the standard 512-byte UDIF footer), not the bzip2 EOS
  +10 that core walks. The "koly" constant cross-checks in the shipped binary
  (og-codebase/RAWSearchDLL.dll, file offset 0x53222). Consequence: core's DMG carves are
  truncated before the mandatory footer (typically unmountable) and candidates in the
  200-209.7 MB window are dropped.
## SA-9 lane rows (2026-10-28 — the push-time-reconciled third lane; same evidence bar)

- **E68** (SaveScan/ifileproperties-part1--ntfs-fat-exfat.md:85): the
  "+0x64 | 4 | secondary ID [CENSUS: 35..199,008,480]" label is refuted by
  the SCAN-2f/SCAN-3 instruction-decisive decode (both lanes, convergent;
  the SA-9 lane re-verified the DRWUI persistence side first-hand):
  +0x64 = the low u32 of the FIRST $DATA run's start LCN (the
  location-anchor family; DRWUI_decompiled.c:853462-853464 persists it
  immediately before the type id). This row was queued by the SCAN-2f
  lane's worklog ("corpus erratum row queued (part1:85 'secondary ID')")
  and is recorded here per that queue — the parallel lanes' E53-E67 range
  did not carry it. (Companion context, already on the record elsewhere:
  +0x60 = total $DATA clusters; the census multiplicity is
  allocated-clusters behavior.)
- (Convergence note, no new row: the SA-9 lane's instruction-level CHM
  disassembly — capstone on `og-codebase/RAWSearchDLL.dll` at
  0x180016b40, the u64 GUID compares at 0x180016b9d-0x180016bc3 against
  the stack-materialized constants — independently reaches E66's
  conclusion and its lesson; its evidence file
  `evidence/decompiled-quotes/s9-chm-gate-asm.md` carries the full
  instruction listing. The lane's census MRW/CRW correction is E57's
  content, reached convergently.)

## Session-10 lane-B row (2026-10-29 — the field-parity deep-dive lane)

- **E69** (SaveScan/ifileproperties-part2--ext4-hfs-refs.md — internal
  contradiction + a doubtful [INFERRED] label): part2 §4's family synthesis
  (:114-116) claims the persistent field set is "invariant" across the six classes
  **including FLAGS(u16)** — but part2's own ReFS grammar table (:97-106,
  [CODE]-backed) has **no FLAGS row**. The synthesis over-generalizes beyond its own
  table. Additionally, the lane-B first-hand read of the ReFS population site
  (ReFSSearchDLL_decompiled.c:2105-2135, quoted in full at
  `evidence/decompiled-quotes/s10-refs-population-site.md`) shows the in-memory +0x39
  u32 is **flag-bundle-derived** — `uVar7 = (uint)local_90 | 0x10`, where the SAME
  local_90 bundle feeds the marker at +0x3d (stored as a **u16**, `local_90._4_2_`,
  not the byte part2's table claims), +0x3f and +0x40 — which weakens the table's
  "[INFERRED] objectId / parent anchor" reading of +0x39 (an objectId comes from the
  parse result, not a flags local OR'ed with the has-children bit 0x10). **Not
  settled by this read:** whether the SERIALIZED ReFS grammar includes a FLAGS u16 —
  the deciding probe is the vtable+0x98 serializer trace (part2's cited reader
  FUN_1405f52e0 is the DRWUI-side consumer, not the engine serializer). **Core
  impact: none either way** — the core persists `flags` unconditionally
  (cr_records.cpp:192; a superset under either reading) and carries the ReFS object
  id in `record_ref` (cr_records.cpp:183, refs_engine.cpp:1111). The erratum exists
  so future sessions do not lean on the contradictory synthesis or the unqualified
  [INFERRED] label.
- (Session-10 lane-B verification note, no new row: the three real SaveScan .rsf
  files were parsed first-hand (`evidence/executions/s10-rsf-ground-truth.md`) — the
  documented container grammar (magic, 0x8C header, 0x24 record header, 2-byte chunk
  header, 4 KiB chunked zlib) is byte-confirmed against all three real files; the
  corpus's rsf-format artifact needs NO correction.)

- **E-S11 batch (session-11 S11 lane, 2026-10-29 — the RAWSearchDLL infra walk's
  five precision corrections + the ZFS/ReFS walk's two refinements; every cite
  first-hand):**
  1. **s3-rawsearch-inventory §5.7's "external virtual" chunk driver** — imprecise:
     the chunk-loop driver is `FUN_18000c650` at **vtable slot +0x118** (an in-DLL
     method of the controller, `:9337-9380` — the loop `vt+0x98(chunk)` over
     `range_sectors / sectors_per_chunk` at `:9361-9376`), not an external virtual
     the host installs.
  2. **s3 §5.5's "array of 0x1000 maps"** — the 0x10048-byte result queue
     (`FUN_18000c430 :9304-9324`) is precisely: **4096 × 0x10 per-format
     {list-head,count} buckets** (`_eh_vector_constructor_iterator_(ptr, 0x10,
     0x1000, …)`) + the all-results list @+0x10000 + an
     `IIterator<ListIterator<CRawFileInfo*>>` wrapper @+0x10010 + **two**
     offset-keyed std::maps @+0x10028/+0x10038.
  3. **The format-name mechanism pinned for S-049** — `FUN_18000b7a0 :8594-8621`
     reads `&DAT_18008f070 + id*9`: a **9-byte-stride id→name table** in .data
     (the naming contract's supporting mechanism, previously uncited).
  4. **The user-info singleton's full layout** (extends SCAN-ENGINE-DELTA A4):
     `FUN_180001350 :65-100` — {+0x60: 1, +0x68: 0 (both write-only resets at
     `:9048/:9050` with NO in-DLL reader — semantics unverified), +0x70:
     200000000 (the 200 MB cap, 12+ readers), +0x78: vector of format ids
     1..169, +0x90: the stats map, +0xa0: byte, +0xb0: string, +0xc0: 0xff}.
  5. **DRWUI shares the carve engine's OOM policy** — `DRWUI_decompiled.c:94179`
     carries the identical `RaiseException(0xe0000001, 0xc0000025)` operator-new
     (log-then-die is product-wide; no handler anywhere) — context for N-139's
     product-contract reading.
  6. **N-056/N-072's "backoff" wording refined** — the og's device poll is a
     fixed **1000 ms unbounded** loop (ZfsSearchDLL `:5671-5673`), not a backoff;
     the core's bounded 3×250/500/1000 stays the safer subset (the registered
     direction is unchanged).
  7. **The og's DVA translate is a fixed +4 MiB** (ZfsSearchDLL `:17996`) — the
     core's nvlist-driven vdev offset is identical on whole-disk pools (the
     registered superset; the note pins the og's constant for future
     multi-vdev fixtures).

## S11-lead additions (2026-10-29 — the deep-lanes wave; session-11-lead)

1. **rawsearchdll--analyser-registry-census.md :20-23 ("all three exports …
   expose three projections of the same data") is wrong in one half** — the
   exports (EUGetAnalyserList/Map/Multimap, `RAWSearchDLL_decompiled.c:72170-72208`)
   project the **STATIC** triplet only (+0x30/+0x40/+0x50); the **WORKING**
   triplet (+0x00/+0x10/+0x20) is rebuilt and IAnalyserFilter-filtered per search
   (`FUN_180006b30`, :4386-4434). The census's M-024-era refinement (the module
   map carries it) supersedes the artifact's sentence.
2. **The census's undecoded ":82" row ("Windows thumbnail/repair variant") is
   resolved** — the key `F7 89 34 2C` registers the SAME dual-vtable
   FileZIPAnalyser (id 0x48) as `PK\x03\x04` (`:2231-2247`). If real-world files
   carry this magic the og carves them as ZIP; core has no such signature row
   (flagged to the carve board — no fixture evidence either way this session).
3. **rawsearch-carving--deep-dive.md §1's ZIP gate table conflated the probe and
   walk tiers** — the PROBE gate (`FUN_1800176b0`, `:18294-18302`) admits version
   `u16@+8 ≤ 0x13 or == 0x62`; the WALK gate (`FUN_1800176e0`, `:18385-18386`)
   requires `≤ 10`. Core implements the walk gate (`cr_analysers.cpp:839-845`) —
   the og's NET admission (probe AND walk) is ≤10, matching core; the artifact's
   single-gate table misled N-008-era analysis.
4. **ghidra-repair/REPORT.md lists three phantom binaries** — `EUZipRepairFile.exe`,
   `EURarRepairFile.exe`, `EUVideoRepairFile.exe` ("NO OUTPUT") do not exist
   anywhere in og-codebase (find: zero); the shipped EUFileRepair bundle is 5
   exes (EUAdobeRepair, EUArchiveRepair, EUImgRepairFile, EuOfficeRepairFile,
   EuPDFRepairFile — all register-dispositioned). The REPORT rows are decompile-
   batch phantoms, not a corpus blind spot.
5. **ghidra-local/NTFSSearchDLL is a byte-identical duplicate** of
   ghidra-searchdll/NTFSSearchDLL (md5 `4d6ac40b…` both decompiles) — the S10-F
   function-level sweep's coverage extends to it for free; censuses should count
   it once. Likewise ghidra-misc/EUVideoRepair ≡ ghidra-repair/EUVideoRepair
   (md5 `613ad879…`).
6. **MainModule's System-Restore purge is absent from every corpus artifact** —
   the srclient/SRRemoveRestorePoint chain (`MainModule_decompiled.c:138378-138420`,
   the WQL enumerate-all at `:138773`, the both-drives caller at `:71496/:71511`)
   is not even string-listed in per-binary/MainModule.md; filed core-side as
   M-114 (the corpus-side gap recorded here for the maintainers).
7. **The genuefisystemfile.cpp boot-surgery family is absent from every corpus
   artifact** — the TempSystem/TmpBCD/TmpSoftWare chains
   (`RecoveryModule_decompiled.c:73731-73786`, `:74837-74887`, `:73610-73621`) —
   the READ side appears in MOM-modules/DeviceManager.md:55's string list only;
   filed core-side as M-113 (the corpus-side gap recorded here).
8. **remote_config_data is a verbatim cached HTTP payload, not a serialized
   map** (session-42 decode): `firebaseonlineconfig.dll` `FUN_180003010`
   reads the response via WinHttpReadData, checks the first 7 bytes are
   `"fetched"` (`firebaseonlineconfig_decompiled.c:1703`), and `fwrite`s
   the whole buffer to the file. The framing is the server's own
   `key\0 [len] value\0` scheme with 1-byte lengths (`\x05false`),
   2-byte-LE lengths (`\x83\x02` = the 643-byte `filter_generalraw`
   array; `\x1a\x05` = the 1,306-byte `json_gift`), and `\x00`-escaped
   u16 lengths (`\x00\x3d\x01` = the 317-byte `json_pe`) plus a binary
   bookkeeping tail. Worth one line in
   `artifacts/config-files/userdata-remoteconfig--signature-and-flags.md`
   so the framing is never re-derived. (Core-side impact: none — the
   core consumes none of it; registered I-027 #2/#5 + OQ-S4-2.)
