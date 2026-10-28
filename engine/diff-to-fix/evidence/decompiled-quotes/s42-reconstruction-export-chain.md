# s42 evidence — the reconstruction/export chain (S-059 / S-060 / M-135 / M-136's shared worksheet)

**Corpus:** `easeus-drw-research` @ `be0cfe9`; decompile = `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c` (cites re-read first-hand this session).
**Core:** `core_recovery` @ `b26017a7`.

## §1 The per-item export body (`FUN_1405414e0` region, :874256-874791)

Post-copy restoration block (verified first-hand):

```c
:874685  if (lpFileName != 0) {
:874686      uVar9 = (**(code **)(*plVar28 + 0x38))(plVar28);        /* attribute word   */
:874687      BVar11 = SetFileAttributesW(lpFileName, uVar9 & 0xfffd); /* in-use bit clear */
:874688      if (BVar11 == 0) ReleaseTrace("Failed to set file attribute");
:874691      hFile = CreateFileW(lpFileName, 0xc0000000, 3, 0, 3, 0x80, 0);
:874693      if (hFile == -1) ReleaseTrace("Failed to set file time");
:874697      else {
:874698          uVar12 = (**(code **)(*plVar28 + 0x80))(plVar28);   /* the mask          */
:874698-709      bit 0x10 → +0x58 getter → last-access FILETIME*
                     bit 0x20 → +0x60 getter → last-write  FILETIME*
                     bit 0x8  → +0x50 getter → creation    FILETIME*
:874714          BVar11 = SetFileTime(hFile, creation, access, write);
:874715-16        if (BVar11 == 0) ReleaseTrace("Failed to set file time");
:874718          CloseHandle(hFile); }
```

Both restorations are **non-fatal** (trace-and-continue). The `SetFileTime` at :874714 is the ONLY one in the 2.38M-line decompile (grep-verified) — refuting the trail's N-009d-row premise ("og never calls SetFileTime", `modules/writer.md:67-75`).

## §2 The stream dispatch (:873629-873671) — recovery reads are content-object-mediated

`FUN_140540670`: content at `props+0x30` → `vt+0x78` GetStream; stream type 0 → `CFileRecoverWorker::ReadAndWriteDataRunList` (when `*param_7==0 || GetType()==5 HFS+`); stream type 0xe → small-resident read; else the chunked copy. I.e. export reads flow through the engine/scan-session stream abstraction (arch/04 §7.5), not a raw handle.

## §3 The runlist copy path (:875503-875855) — sparse + failure semantics

```c
:875661  memset(buf, 0, chunk);                    /* pre-zeroed            */
:875662  if (*plVar6 != -1) {                     /* sparse run (LCN==-1): read SKIPPED */
:875663-668    …clamp read range to volume GetSectorTotalSize…      /* beyond-EOF clamp */
:875672-673    read(source, buf, lVar11, uVar2);
:875675-682    if (read == 0 && progress-check == -1) { error 9; break; }  /* the cancel-ish gate */
:875684-690  else if (read == 0) {
                 LogError("Failed to ReadSector! ");
                 LogError("m_s64StartSector:%I64d,\tpDataRuns64LCN:%I64d,\tm_nSectorPerCluster:%I64d", …);
                 LogError("StartSector:%I64d,\tReadCount:%I64d,\tSectorTotalSize:%I64d", …);
                 progress-skip-ahead; error 9; break; }               /* file stays PARTIAL */
:875699-706  write(buf…) }                                            /* zeros for holes     */
```

Write failure → `LogError("Failed to write file! %s %d", "CFileRecoverWorker::ReadAndWriteDataRunList", …)` → error 1, break (:875754-761 and the 0x60e/0x61d variants :875793-815). Cancel → error 10 (:875654-659). ZFS embedded-payload tail written from the runlist object (:875788-838).

## §4 The chunked copy path (:874797-875088) — the sparse chunk typing

`FUN_1405421a0` chunks the datarun list into 16-unit blocks; offset==-1 runs become type-1/2 chunk descriptors (:874877-889, :874969-970). `FUN_140542590`: type-0 → 1 MiB read+write (read return ignored → zeros on failure :875045-49); **type-2 → 1 MiB zero-filled, written, no read** (:875062-75); type-1 → nested runlist read with pre-zeroed buffer (:875057-60). Destination-write -1 → error 10 (:875078-81).

## §5 The per-item result recorder `FUN_140541070` (:874031-874241)

Error-9 accounting is settings-gated (:874072-76). On failure: the error pushes into the collection (`FUN_140302570(obj+0x80,…)`), failed-count/size counters +0x58/+0x60 tick, **the tree node is marked `*(node+0x73) |= 1`** (:874087), observer fires, `LogInfo("Failed to recover file: %s", name)` (:874129), and the destination is probed via `std::filesystem` exists/file_size (:874131-205; missing-file observer :874210-37). The numbered user-facing causes (english_qm.json :448, :538, :598, :640, :664, :682): 1 insufficient storage, 2 path too long, 3 garbage name, 4 name likely lost, 5 sector location not found, 6 free quota exceeded. (Error-int → cause mapping lives in `FUN_1400ed760`, undecoded — M-135's open question.)

## §6 Core-side pins (at b26017a7)

- Sparse dropped: `cr_runlist.cpp:187-197` (`if (r.sparse) continue;`); hole units emit no extents while the comments claim writer zero-fill: `cr_mftwalk.cpp:262-271`, `:340-352`; `Extent` has no logical offset: `cr_node.h:109-120`; the cumulative walk + `ERR_TRUNCATED`: `cr_safewriter.cpp:405-451` (rescue via resident tail :415-438; refusal :440-444).
- No metadata restoration: `cr_safewriter.h:39-63` (DestIo surface), `cr_destio.h:17-46`; grep `SetFileTime|SetFileAttributes|utimensat|utimes|set_times|fchmod` over `src/`+`include/` → zero hits.
- Zero-fill-and-continue: `cr_safewriter.cpp:498-503`; Salvaged commit :532-547; delete-on-fail `cr_destio.h:36-43`; per-item collection :616-644.
- The raw-device handoff: `rrecov.cpp:2080-2082`, `cr_ui.cpp:4027`; the scan's pipe (unused by recovery): `cr_scan.cpp:155-194`.
- No sparse-file writer test anywhere: no `test_safewriter.cpp`; zero sparse hits in `test_writer_dirs.cpp`/`test_ldm_recovery.cpp`; `test_runlist.cpp:36-49` covers decode only.
