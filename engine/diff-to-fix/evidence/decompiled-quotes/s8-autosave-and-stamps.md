# S8 evidence — the og's auto-save trigger gates + config model

Read this session (session-8 wave 2; parallel-agent V-B decode, spot-checked
against `re-workspace/artifacts/SaveScan/rsf-format--scan-session.md` and
`reversed/ghidra-drwui/DRWUI_decompiled.c`, corpus `be0cfe9`).

## 1. The auto-save function and its THREE call sites

`FUN_1402857c0` (:442747) is the og's auto-save. Its call sites are
:612583, :615166, :616764 — all three inside scan-phase COMPLETION
handlers. Every site gates the call:

- not-import: `*(int*)(scanCtx + 0x1c8) == 0`
- results-found: `FUN_1403964b0(...) != 0` (a found-count check — an
  empty scan never auto-saves)
- not-already-saved (2 of 3 sites): `*(int*)(scanCtx + 0x388) == 0`

No `IsCancelled` check exists at any trigger site (cancellation polls
exist only INSIDE the save/restore walk itself) — the corpus does not
evidence a "cancelled never auto-saves" gate; the completion-handler
placement is the only cancellation-adjacent mechanism.

## 2. The entry gate is a BYTE FLAG, not a directory field

The sole gate inside `FUN_1402857c0` (:442822-824):

```c
if (FUN_1400b5270(FUN_1400b3c60()) == '\0') { ... proceed ... }
```

`FUN_1400b5270` (:165951-5) is `return *(undefined1 *)(param_1 + 0x10);`
on a global config singleton — a one-byte enable flag where 0 = enabled.
No `char[512]` autosave-directory field exists anywhere in the path (the
`"SaveResultPath = %S"` string at :440979 is a trace log). The og's
save path is composed at the call site, not carried in the scan config.

## 3. Naming, overwrite, and hygiene details

- Timestamp: `GetLocalTime(&local_4d0)` (:442887) — matches rsf-format
  §5.2's "record-2 scanTime stamped GetLocalTime at save".
- Name: `sprintf_s(buf,100,L"%02d_%02d_%02d_%02d_%02d", wMonth,...)`
  (:442893-897) then append `L"\\auto_" + ts + L".rsf"` (:442898-900) —
  the rsf-format §5.5 `auto_<vol>_<M>_<D>_<H>_<m>_<s>` convention.
- Volume part built at :442881-885, with `L"Folder"` appended when
  `vtable+0x110` is set (:442883-886) — a folder-scan naming variant.
- Overwrite: `DeleteFileW` of the target before the write (:442903-907).
- Stale-file hygiene: `DeleteFileA` of a stale path at :442830-33.

## 4. Session LIST reads record 1 THEN record 2 (not "record 2 only")

The DRWUI list builder `FUN_140273cd0` (:432410) enumerates `auto_*` /
`.rsf` files (:432326-331), and per file reads record type 1 (session
state; `DAT_141134c10` find at :432575, grammar consumed :432791-871) and
then record type 2 (volume info; `DAT_141134c00` at :432875 —
volumeName[100], deviceType u32, SYSTEMTIME 16B, stat1/stat2/flags
:432882-932). No tree record (GUID 6cf208d2…) is read on the list path.
A variant branch `*(int*)(info+0x18)==5` (:432753) parses a different
record-1 layout (WinPE-style: `[0x118 blob][volumeName][u32][SYSTEMTIME]
[u64×3][u32][u8][u8]` :432755-789). Volume fields are gated on total
file size (>8000/:432757, >8001/:432770, >8002/:432906 style thresholds
at :432757/:432879/:432889/:432906) — a size-progressive read.

## 5. NTFS +0x64/+0x60: the full stamp chain (corrections to the landing's phrasing)

- The :5126-27 stamps are `+0x53`/`+0x4f` on the INTERNAL file-info struct
  (values from `FUN_1800336f0` :5115). The `CNTFSFileProperties +0x64`
  stamp happens one hop later in the field mapper `FUN_180003020`
  (:1987 `+0x64 <- +0x53`, :1988 `+0x60 <- +0x4f`) — a 20-field
  internal→properties mapper.
- `FUN_18001c660` :22470-71 stamps BOTH +0x64 and +0x60 (the GUID-dump
  twin); its serializer twin is `FUN_180003250` :2264-65 (the landing's
  "FUN_180033250" is a typo — no such symbol).
- **The og PERSISTS +0x60**: rsf-format--scan-session.md :398 documents
  record-4's CNTFS extra as `u32(+0x64); u32(+0x60)` — both dwords on the
  wire. The core carries only +0x64 (nflags bit-8) and derives clusters
  from allocated_size.
- Sparse semantics: any sparse run zeroes the +0x60 total
  (`FUN_1800336f0` :41096-98) — so +0x60 is NOT always derivable from
  allocated size (sparse files: og stores 0; allocated_size is nonzero).
- Resident semantics: the resident path builds
  `CResidentNTFSDataRunList` with a synthetic run `{local_60, 1}`
  (:5192-94) and still flows the stamp — the og's resident +0x64 is
  low-u32(local_60), not a hardcoded 0.

## 6. RSF robustness contracts

- Chunked-zlib with **raw-memcpy fallback on inflate failure** (md :98).
  (The core's crsf reader has the equivalent per-chunk raw fallback, F10
  — MATCH.)
- Name sanitizer forbidden set `/ * : ? " < > |` (md :450-451).

## 7. Registry-region corrections (RAWSearchDLL)

- u32 keyed registry: entries at **:1728-2509** (59 bindings), builder
  `FUN_180002a50` region :1436-2512.
- u16 keyed registry: entries at **:2536-2911** (35 bindings), builder
  `FUN_180004320` at :2517.
- List layer: `FUN_180004fb0` :2919-4214 (unconditional zone ~2930-3350;
  config-gated zone 3356-4110 via `FUN_180010f50` — the RawConfig.ini
  extension-toggle lookup, 18 gated sites).
- The S-007 batch-1 landing's cited regions ":5450-6600"/":6205-6590"
  are INI-parsing (:5440-5519) and std::streambuf (:6202-6269) code —
  not registries. Total keyed bindings: 94 (59+35).
