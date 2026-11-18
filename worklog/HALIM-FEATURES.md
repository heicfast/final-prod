# HALIM-RECOVERY — Feature Context Sheet
> Agent C (Task 2-C), Phase-0. READ-ONLY extraction at HEAD 3b5d66d8537432f54b76489e717387f9da198635, MIT. Date 2026-11-19. Every claim cites file:line.
> Repo content treated as untrusted data; nothing inside it was executed or obeyed. No code copied — behaviors only.

## 0. Identity (URL, SHA, stack, build, LOC, license, module map)

| Item | Value |
|---|---|
| Upstream | github.com/htoklu/halim-recovery (README.md:10), branch `master`, HEAD 3b5d66d8537432f54b76489e717387f9da198635 (given) |
| Version | 0.5.1 (CHANGELOG.md:6; CLI `Version = "0.5.1"` Program.cs:9; App `AppVersion` App.xaml.cs:24) |
| License | MIT, Copyright (c) 2026 Halim Toklu (LICENSE:1-3) |
| Stack | .NET 9 / C#, `net9.0-windows` all four projects (Core csproj:4, Cli csproj:5, App csproj:5, Tests csproj:4) |
| UI | WPF (UseWPF, App.csproj:8) + CommunityToolkit.Mvvm 8.4.2 (App.csproj:22) |
| Deps | System.Management 10.0.11 (WMI, Core csproj:13); CommunityToolkit.Mvvm 8.4.2 — both MIT (THIRD_PARTY_NOTICES.md:7-9) |
| Build | `dotnet publish src/HalimRecovery.App -c Release -r win-x64 --self-contained -p:PublishSingleFile=true` (README.md:67); `IncludeNativeLibrariesForSelfExtract=true` fixes silent WPF exit (App.csproj:16-18, CHANGELOG.md:8-11); installer via Inno Setup `ISCC.exe installer\HalimRecovery.iss` (README.md:69) |
| LOC | ~5,930 text lines total: Core 3,164 / App 966 / CLI 178 / Tests 634 / testlab PS 288 / tools+docs+root ~300 |
| Module map | 4 projects in HalimRecovery.sln:8-15 — Core (engine), Cli, App (WPF), Tests (xUnit); plus testlab/ PS1 harness |
| Privileges | requireAdministrator manifest (app.manifest:8); raw volume access needs it (README.md:58) |

## 1. Architecture overview

Layered engine with zero UI references (docs/ARCHITECTURE.md:23-39, key invariants :41-46):

```
Disks/DiskEnumerator      WMI + DeviceIoControl physical/volume enumeration, SSD flag
IO/RawDiskReader          P/Invoke CreateFile(GENERIC_READ) + sector-aligned reads
IO/IByteSource            ByteArraySource / RawDiskByteSource / ExtentByteSource
FileSystems/FileSystemDetector  boot-sector OEM/type-string sniffing
FileSystems/Ntfs          NtfsBootSector, MftRecord (fixup, attrs), DataRunDecoder, NtfsScanner
FileSystems/Fat32         Fat32Scanner (FAT load, dir walk, LFN, orphan dirs)
FileSystems/ExFat         ExFatScanner (entry sets, allocation bitmap)
Carving/FormatSpecs       8 FormatSpec records w/ structural Measure validators
Carving/DeepScanner       8 MiB streaming carver
Health/HealthScorer       evidence-weighted 0-100 confidence + GREEN/YELLOW/RED
Preview/PreviewProvider   image/text/DOCX/XLSX/ZIP/PDF-info preview from source only
Recovery/RecoveryEngine   safe writer (dest checks, sanitization, SHA-256)
Recovery/PathSafety       filename/path sanitization + root containment
Scanning/ScanEngine       orchestration; Scanning/PauseToken cooperative pause
Reporting/RecoveryReport  text + JSON session report
Logging/Log               rolling daily file log, never throws
Search/NaturalQueryParser offline TR/EN natural-language → FileFilter
TestSupport/SampleFiles   structurally-valid synthetic file generator (test lab)
```

Data model (Models/): `RecoverableFile` (Id, FileName, OriginalPath, Size, Category, CreatedUtc/ModifiedUtc, Source {FileSystemMetadata|Carved}, Extents[], ResidentData, Health, Confidence, HealthNotes[], OverwrittenFraction, IsFragmented — RecoverableFile.cs:27-53); `FileExtent(Offset,Length)` record (:24); `ScanProgress` with Percent/Speed/ETA capped at 30 days (ScanProgress.cs:12-25); `DiskInfo`/`VolumeInfo`/`FileSystemKind` (DiskModels.cs:4-33).

Key invariants (docs/ARCHITECTURE.md:41-46, enforced in CONTRIBUTING.md:7-15 and .cursorrules:20-40): source volume opened read-only always; every long op takes CancellationToken + reports ScanProgress; bounded memory (fixed buffers, never load disk into RAM); confidence only from measurable evidence; Core has no UI references.

## 2. Feature inventory

### 2.1 Scan modes & engine composition
**Behavior.** Two modes on a selected volume (or a raw image file): Quick Scan (filesystem metadata) and Deep Scan (signature carving over the raw volume, any filesystem). No full-physical-disk scan mode; scanning is per-volume (`\\.\X:`).
**Citations.** ScanEngine.cs:25-39 (ctor accepts drive letter OR image path via `File.Exists`), :44-61 QuickScanAsync switch, :63-74 DeepScanAsync; README.md:23-27; Program.cs:69-102, 104-143.
**Algorithm.**
1. `ScanEngine(driveLetterOrImage)`: if `File.Exists` → open as RawDiskReader (regular file = disk image, RawDiskReader.cs:60-61); else open `\\.\{letter}:` read-only. Detect FS from boot sector (FileSystemDetector).
2. QuickScan: dispatch by `FileSystemKind` — Ntfs→NtfsScanner, Fat32→Fat32Scanner, ExFat→ExFatScanner, else `NotSupportedException("Quick Scan supports NTFS, FAT32 and exFAT... Deep Scan (signature search) can still be used.")` (ScanEngine.cs:51-52).
3. After either scan, `HealthScorer.Score(f, new ExtentByteSource(f, _reader))` per file (ScanEngine.cs:55-59, 68-72).
**Prerequisites.** Admin for raw volume; image mode needs no admin (used by testlab/Test-OfflineImages.ps1:49).
**Tests.** No unit test instantiates ScanEngine; e2e covered by testlab scripts.
**Edge cases.** Fat16 is detected (FileSystemKind.Fat16) but has no scanner → Quick Scan throws the honest unsupported message. Disk Image mode is a placeholder button → info box "planned for the next release" (MainViewModel.cs:383-386) — README roadmap concurs (README.md:132).

### 2.2 Filesystem detection
**Behavior.** Boot-sector signature sniffing.
**Citations.** FileSystemDetector.cs:17-33.
**Algorithm.** OEM string at offset 3 (8 bytes): `"NTFS    "`→Ntfs, `"EXFAT   "`→ExFat; type string at 82 `StartsWith("FAT32")`→Fat32; at 54 `FAT16`/`FAT12`→Fat16; else Unknown. Requires full 512-byte read.
**Tests.** FileSystemDetectorTests (NtfsParserTests.cs:147-173): 4 facts incl. zero buffer → Unknown ("not misreported").

### 2.3 NTFS Quick Scan (deleted MFT records, path reconstruction, $Bitmap overwrite analysis)
**Behavior.** Streams the whole MFT, finds FILE records with in-use flag cleared, reconstructs original paths from parent-record chains, computes per-file overwritten-cluster fraction from $Bitmap.
**Citations.** NtfsScanner.cs:8-13 (class doc), NtfsBootSector.cs:17-46, MftRecord.cs:36-123, DataRunDecoder.cs:13-45.
**Algorithm.**
- Boot parse: BytesPerSector@11, SectorsPerCluster@13 (raw ≥0x81 encodes 2^(256−raw), NtfsBootSector.cs:42-46), TotalSectors@40 (i64), MftStartCluster@48, MftMirror@56, clusters-per-record@64 as sbyte (positive=clusters, negative n→2^−n bytes, :30-34). Sanity: bps 256..8192, spc>0, record 256..65536 else `InvalidDataException("implausible values (corrupted filesystem?)")` (:36-37).
- MFT location: parse record 0 at `MftStartCluster*bpc`, take its $DATA data runs as MFT extents (NtfsScanner.cs:126-141); empty → InvalidDataException.
- $Bitmap (record 6, read at `MftStart*bpc + 6*recordSize` assuming first MFT extent, :143-173): one bit per cluster, 1=allocated; size `min(DataSize, (TotalClusters+7)/8)`, sanity bound 512 MiB; failure → warn + disable overwrite analysis (OverwrittenFraction stays −1).
- Record parse (MftRecord.Parse): "FILE" magic; **USN fixup first** — verify last 2 bytes of each sector equal USN else reject as torn write (ApplyFixup, :138-156); flags@22 bit0=InUse bit1=IsDirectory; BaseRecordNumber@32 (48-bit mask); attribute walk from offset@20 to usedSize@24. Attributes: $STANDARD_INFORMATION(0x10) created@0/modified@8 FILETIME→UTC (≤0 or out-of-range→null, :158-163); $FILE_NAME(0x30) with namespace ranking Win32(1)/Win32AndDos(3) beat POSIX(0) beat DOS(2) (:86-89), parent record = first 8 bytes 48-bit, name UTF-16 at value+66, len at +64; $DATA(0x80) **unnamed only** (`nameLength == 0`, :99) — resident→`ResidentData` bytes; non-resident→real size@48 + data runs from runOffset@32.
- DataRunDecoder: header byte low nibble = length-field size, high nibble = offset-field size; 0x00 terminates; offsetSize 0 → sparse run `Lcn=-1`; LCN offsets are signed delta-accumulated; malformed/truncated input → stop silently, never throw (:26-44).
- Deleted filter: `!InUse && !IsDirectory && BaseRecordNumber==0 && FileName.Length>0 && HasData` (NtfsScanner.cs:75-77).
- Pass 1 collects `names[recordNo]=(name,parent)` for all named base records; pass 2 builds RecoverableFile with `BuildPath` (walk parent chain to root record 5, depth guard 64, self-reference guard, unknown ancestor → prefix `?\`, :196-209), extents `Lcn*bpc` length `clusters*bpc` (sparse runs skipped, :112-116), `OverwrittenFraction = MeasureOverwrittenFraction` (fraction of run clusters with bitmap bit set, −1 if unknown, :179-194).
- Progress: phase "Analyzing MFT", ~1 MiB record-aligned chunks (:53), FilesFound = deleted count; log summary line (:42, :121).
**UX.** Original names, paths, sizes, both timestamps surfaced; resident small files recovered byte-exact from the MFT itself (HealthScorer adds "exact recovery" note).
**Tests.** DataRunDecoderTests: single run, delta accumulation, negative delta sign-extension, sparse run Lcn=−1, garbage yields empty (NtfsParserTests.cs:9-55). MftRecordTests build a synthetic 1024-byte record with real fixup stamps: deleted record parsed w/ name+parent+resident data; InUse flag; non-FILE buffer→null; corrupted USN stamp→null (torn write) (:57-145).
**Edge cases.** Extension records (BaseRecord≠0) excluded — fragmented-into-extension files keep only base-record runs. Sequence numbers parsed (:45) but not used for staleness. $MFT mirror unused. $Bitmap assumed resident in first MFT extent (documented assumption, :148-149).

### 2.4 FAT32 Quick Scan (0xE5 entries, LFN, orphan/deleted directory harvesting)
**Behavior.** Loads the FAT, walks the live directory tree, collects deleted entries (first byte 0xE5) with LFN names, and harvests orphaned/deleted directories whose FAT chain was cleared.
**Citations.** Fat32Scanner.cs:9-17 (honest contiguity limitation in class doc), :32-53 boot parse, :71-80 FAT load, :85-183 WalkDirectory, :191-208 WalkDeletedDirectory, :211-259 HarvestDirectoryCluster.
**Algorithm.**
- Boot: bps@11, spc@13, reserved@14, numFats@16, totalSectors@32 (else @19), fatSectors@36, rootCluster@44; sanity bps 256..8192, spc>0, numFats>0 else InvalidDataException (:46-47). `ClusterToOffset(c)=DataOffset+(c−2)*bpc` (:55).
- FAT: all entries into `uint[]` masked 0x0FFFFFFF; cap 128M entries else InvalidDataException "FAT too large" (:71-80). `IsClusterFree(c) = _fat[c]==0` (:82-83).
- Directory walk follows the FAT chain (`cluster = _fat[cluster]`), cycle guard via visited set, chain guard 1M, depth cap 64 (:89-95). 32-byte entries; first byte 0x00 → end of directory (aborts cluster loop); attr 0x0F → LFN entry, seq = first & 0x1F (or −1 if 0xE5-deleted) with 13 UTF-16 chars gathered from offsets 1/14/28 (:274-283); deleted = first byte 0xE5; isDir = attr&0x10; volume label (attr&0x08) skipped; start cluster = high16@20<<16 | low16@26; size u32@28; FAT dates (lastAccess@18 ignored; created date@16/time@14, modified date@24/time@22) decoded 1980-based, Local→UTC, invalid→null (:305-314).
- LFN assembly: if all seqs>0 → order by seq; else (deleted, seq lost) → reverse disk order; concat + trim (:286-293). Deleted short name: destroyed first byte replaced with `_` (:295-303).
- Deleted file acceptance: `size>0`, cluster in [2, TotalClusters+2) (:144). Extent = single `(ClusterToOffset(start), size)` — **contiguity assumption**, HealthNote "FAT32: cluster chain lost on delete; contiguous layout assumed" (:158-160).
- Overwrite analysis: fraction of `ceil(size/bpc)` consecutive FAT entries ≠0 (MeasureReusedFraction, :261-272).
- **Orphan directory logic** (key differentiator): a live-looking directory entry whose first cluster is FREE in the FAT is an orphaned (deleted) tree → `WalkDeletedDirectory` (:131-139); a deleted (0xE5) directory with free first cluster → same (:173-178). `WalkDeletedDirectory` walks up to **256 contiguous clusters**; first cluster must begin with `.` or 0xE5 (:202); subsequent clusters must be free (else reused by live data → stop, :203); harvests **every** file entry regardless of its own deleted flag, because "Windows does not rewrite child entries when it deletes a whole tree" (:185-190, exFAT twin comment ExFatScanner.cs:96-100); stops at 0x00 terminator; nested deleted subdirs recursed (:234-238). Harvested files get note "recovered from deleted directory; contiguous layout assumed" (:254).
- Progress: phase "Scanning directories", BytesTotal=0 → indeterminate progress bar in UI (ScanProgress.Percent=0 when BytesTotal≤0, ScanProgress.cs:12; UI ScanIndeterminate, MainViewModel.cs:106).
**Tests.** No unit tests for Fat32Scanner (testlab covers e2e; Benchmark 14/14 exact, docs/BENCHMARK.md:26).
**Edge cases.** "."/".." skipped; size 0 files skipped; cluster range validated; `nextCluster:` label pattern for 0x00 early exit.

### 2.5 exFAT Quick Scan (deleted entry sets, NoFatChain exactness, allocation bitmap authority)
**Behavior.** Parses 32-byte directory entry sets (file + stream-extension + name entries), including deleted sets, using the allocation bitmap as the authoritative cluster state.
**Citations.** ExFatScanner.cs:9-14 (class doc), :29-51 boot, :74-76 type-code whitelist, :105-128 bitmap load, :143-219 WalkDirectory, :221-271 EmitEntry.
**Algorithm.**
- Boot: verify OEM "EXFAT   "; BytesPerSector = 1<<bs[108], SectorsPerCluster = 1<<bs[109]; FAT offset@80, FAT length@84, cluster-heap offset@88, cluster count@92, root cluster@96; sanity bps 256..8192, root ≥2 (:49-50).
- Entry-set state machine: `0x85` (live) / `0x05` (deleted) file entry → secondaryCount@1, isDir from attributes@4 (u16 & 0x10), created@8/modified@12 (u32 each); `0xC0`/`0x40` stream extension → NoFatChain = flags@1 bit 0x02, nameLength@3, firstCluster@20 (u32), dataLength@24 (i64); `0xC1`/`0x41` file-name entry → 15 UTF-16 chars at offset 2; emit when name complete (nameBuilder ≥ nameLength && haveStream) (:175-205). Deletion marker = in-use bit 0x80 cleared (0x85→0x05, 0xC0→0x40, 0xC1→0x41) (:10-11).
- `LooksLikeDirectoryCluster` whitelist of valid first type codes guards against straying into file data (:74-76, :167).
- Directory chaining: live dirs follow the FAT (`next in [2, 0xFFFFFFF7)`); without FAT assume contiguous `cluster++` (:208-216).
- Dir recursion (EmitEntry): live dir → recurse chainValid (unless NoFatChain); deleted/unreachable dir (cluster free per bitmap) → recurse with `treatAllAsDeleted=true`, bounded by `dirClusters = ceil(dataLength/bpc)` (:228-243). Same "children of a deleted tree are all recoverable" invariant as FAT32 (:96-100).
- File acceptance: deleted, `dataLength>0`, firstCluster in [2, ClusterCount+2) (:226, :244). Extent = single contiguous run; NoFatChain → note "file was stored contiguously (exact layout known)", else "FAT chain cleared on delete; contiguous layout assumed" (:257-262).
- Cluster allocation: **allocation bitmap preferred over FAT** (root entry type 0x81, firstCluster@20, length@24 i64; "FAT entries of deleted files are often left stale", :101-104, :130-141); FAT as fallback. Overwrite fraction over consecutive clusters (:273-284).
- Timestamps: FAT-packed u32 — date in high word, time in low word (:286-295).
**Tests.** None in xUnit; testlab e2e (Benchmark 14/14 exact quick, docs/BENCHMARK.md:28).
**Edge cases.** FAT load failure tolerated (warn, bitmap-only); bitmap length sanity 128 MiB; deleted dirs only recursed when first cluster free.

### 2.6 Deep Scan (streaming signature carving, 8 formats → 12 extensions)
**Behavior.** Streams the entire volume in 8 MiB chunks, tests each 512-byte sector for a signature, then measures the candidate by parsing the format's internal structure. Carved files get synthetic names; names/paths/timestamps are explicitly not recoverable.
**Citations.** DeepScanner.cs:8-17 (class doc), :36-107 Scan loop, :109-159 TryCarveAt; FormatSpecs.cs:26-48 registry.
**Algorithm.**
- Loop: read chunk (IOException → **skip one 8 MiB chunk and continue** — bad sectors tolerated, :59-65); for each sector offset: optional `IsOffsetAllocated(abs)` skip hook; `TryCarveAt`; on hit, skip past the carved file (sector-rounded) to avoid re-detecting embedded content, including jumping the outer position when the file spans chunks (:74-87).
- Signature match: exact header bytes per spec (JPEG FFD8FF, PNG 8-byte sig, GIF87a/GIF89a, %PDF-, PK\x03\x04, ID3, RIFF); MP4 via `LooksLikeMp4` (bytes 4-7 == "ftyp", FormatSpecs.cs:281-282); bare MP3 (no ID3) via `Mp3FrameLength(sector) > 0` sync-bit check (DeepScanner.cs:125-127).
- Acceptance: `measure != null && 0 < Length ≤ spec.MaxLength` (DeepScanner.cs:137). Per-spec MaxLength: JPEG 128 MiB, PNG 256 MiB, GIF 64 MiB, PDF 256 MiB, ZIP 1 GiB, MP4 8 GiB, MP3 512 MiB, WAV 4 GiB (FormatSpecs.cs:32-47).
- Naming: `carved_{sectorNumber:D10}.{ext}` (DeepScanner.cs:142); `OriginalPath=""`; `OverwrittenFraction=0` ("carved data was just read and parsed from disk", :146); notes: "structure validated during carving" or "header found but structure incomplete", plus "original name, path and timestamps are not recoverable from raw data" (:150-155).
- Progress: every 64 MiB or at end, phase "Deep scan (signature carving)" (:91-102). Pause via `pause?.WaitIfPaused(ct)` (:51).
**Validators (all defensive, bounded; "structure valid" only when header+layout+terminator check out, FormatSpecs.cs:21-25):**
- **JPEG** (:64-105): segment walk (fill bytes 0xFF, standalone markers RSTn/TEM skipped, segLen<2 reject) until SOS; then raw 1 MiB-buffer scan for FFD9 with 1-byte overlap for split markers; length = EOI end; sawSos required for StructureValid (EOI before SOS → valid=false). Comment: "Length-parsed APPn segments mean embedded EXIF thumbnails are skipped correctly" (:62-63).
- **PNG** (:108-128): chunk walk; first chunk must be IHDR len 13; type must be ASCII letters; IEND ends; CRC skipped (pos += 8+len+4).
- **GIF** (:131-176): logical screen; GCT size `3*2^((b&7)+1)`; extensions (0x21) and image descriptors (0x2C) with LCT + LZW min-code size; sub-block skip helper; trailer 0x3B.
- **PDF** (:181-207): scan for **last** `%%EOF` (incremental updates append new ones), stop when no new EOF within 8 MiB of the last; `startxref` presence → StructureValid, else note "no startxref found (may be truncated)".
- **ZIP/Office** (:220-278): scan for EOCD `PK\x05\x06`; comment length honored; **validate** central-directory pointer: bytes at `start+cdOffset` must be `PK\x01\x02` (or cdOffset==0xFFFFFFFF zip64) (:238-240); then `ClassifyZip`: ASCII scan of first 4 KiB (and last 64 KiB) for `word/`→docx, `xl/`→xlsx, `ppt/`→pptx, `[Content_Types].xml`→Office flag, else "zip"; unclear Office → note "application type unclear" (:254-278).
- **MP4/MOV** (:284-318): box walk; size==1 → 64-bit largesize@8; size==0 ("rest of file") → break (cannot determine end); box type must be alnum/space; `ftyp` required + ≥2 boxes; brand "qt" → ext mov; `moov`/`mdat` seen → StructureValid else note "likely truncated"; max 1000 boxes.
- **MP3** (:321-376): skip ID3v2 (syncsafe 7-bit size); frame walk using `Mp3FrameLength` — MPEG1/2/2.5 **Layer 3 only**, bitrate/sample-rate tables (:321-332), `factor 144 (MPEG1) / 72` + padding; **reject < 3 frames** ("too short to be confident it's really MP3 audio"); frames < 10 → StructureValid=false + note "very short audio stream".
- **WAV** (:379-388): "RIFF"+"WAVE" at offset 8; riffSize sanity 12..4 GiB; first chunk `fmt `/`JUNK`/`LIST` → valid; length = 8+riffSize.
**Tests.** FormatSpecTests (13 facts, FormatSpecTests.cs:14-157): exact lengths for all 8; JPEG truncated (no EOI) → null; JPEG + 500 B trailing garbage → stops at EOI; PNG corrupt IHDR type byte → null; ZIP classified docx/xlsx by content; MP3 2 frames → null; WAV broken "WAVE" → null.
**Edge cases / NOT FOUND IN CODE.**
- `DeepScanner.CarveText` property exists (DeepScanner.cs:23-24) but **no TXT spec exists in FormatSpecs.All** — TXT carving is absent, matching CHANGELOG.md:46 "TXT carving in Deep Scan is disabled by default (high noise)" but README.md:50 lists no TXT either. README CLAIM "disabled by default" = actually not implemented at all.
- `IsOffsetAllocated` hook (skip clusters allocated to live data) is **never wired** by ScanEngine/CLI/GUI — dormant feature (grep: only DeepScanner.cs:22 defines it; no assignment anywhere).
- Carved zip runs whose CD pointer fails validation are skipped entirely (return null) — no "header only" carve for ZIP.

### 2.7 Recovery health scoring (evidence-weighted confidence)
**Behavior.** Every result gets 0-100 `Confidence` + GREEN/YELLOW/RED `Health` + human-readable `HealthNotes`, computed only from measurable evidence; fully-overwritten files are forced RED.
**Citations.** HealthScorer.cs:8-15 (class doc), :16-86 Score, :88-125 ValidateContent.
**Algorithm.** Weights: cluster reuse 55% (unknown → flat 25 + note "Cluster allocation state unknown"), structural validation 35% (Valid +35 / HeaderOnly +15 / Mismatch +0 / NotApplicable +18 "neutral, do not punish or reward"), layout certainty 10% (resident data +10 "exact recovery"; contiguous-assumption note +3; fragmented +6; else +10). Bands: ≥75 Green, ≥40 Yellow, else Red (:73-79). Override: `reuse ≥ 0.99` → Red + Confidence ≤10 (:81-85).
- ValidateContent: carved files reuse carving notes (Valid if "structure validated" note present); metadata files matched by extension via FindSpec map (:127-138, jpg/jpeg/png/gif/pdf/zip/docx/xlsx/pptx/mp4/mov/m4v/m4a/mp3/wav); 16-byte header check (MP4 via ftyp; MP3 via frame sync); then full `spec.Measure(content, 0)`; `sizePlausible = |measure.Length − Size| ≤ max(4096, Size/10)` (:118-119); exceptions → HeaderOnly.
- Example evidence notes (user-visible strings): "No clusters reused by other files", "35% of clusters have been reused — that data is overwritten", "File signature and structure verified on disk", "Content does not match the expected file format (likely overwritten)", "File is fragmented (N extents)".
**Tests.** HealthScorerTests (EngineTests.cs:9-60): intact validated JPEG → Green ≥75; fully overwritten → Red ≤10; all-zeros content scores strictly below intact JPEG; unknown extension `.xyz` not punished as Red.
**Edge cases.** OverwrittenFraction −1 (bitmap unavailable) → 25+18+10 = 53 = Yellow midpoint, i.e. honest uncertainty.

### 2.8 Preview before recovery
**Behavior.** Best-effort preview read only from the (read-only) source; corrupted data yields partial preview or a clear failure reason.
**Citations.** PreviewProvider.cs:12-16 (class doc), :20-43 dispatch, :45-191 implementations.
**Algorithm / UX.**
- Dispatch by extension (:27-37): images (jpg/jpeg/png/gif/bmp/webp) → up to 32 MiB raw bytes, decoding deferred to UI ("a partially overwritten image may still render its upper portion", :52-53); text (txt/log/csv/md/xml/json/ini) → first 64 KiB UTF-8 with \0 stripped; pdf → first 256 KiB: version string, count of `/Type /Page` occurrences, "(Full rendering is available after recovery.)"; docx → ZipArchive over first 32 MiB, `word/document.xml` text extracted (tag-stripping state machine, 4000 chars); xlsx → worksheet names + `xl/sharedStrings.xml` sample (2000 chars); mp3/mp4/mov/wav/m4a → metadata card (name/size/dates/source, "Media preview is available after recovery"); zip/pptx → first 50 entry names; anything else → metadata card.
- Failure reasons are user-facing strings, e.g. "DOCX structure incomplete: word/document.xml missing.", "DOCX archive is damaged and cannot be opened.", "Data no longer looks like a PDF (content may be overwritten).", "No data available on disk for this file."
- UI: WPF BitmapImage, DecodePixelWidth=480, CacheOption.OnLoad, Frozen; decode failure → "Image data is too damaged to decode (...)" (MainViewModel.cs:267-283); stale-selection guard `if (SelectedFile != vm) return` (:264); preview runs on Task.Run off the UI thread (:263).
**Prerequisites.** Engine (volume handle) still open — after BackHome the handle is disposed and preview shows "Preview unavailable (volume closed)." (:259).
**Tests.** No dedicated unit tests (DOCX/XLSX paths exercised indirectly via SampleFiles in the test lab).

### 2.9 Safe recovery engine (destination validation, collision naming, SHA-256)
**Behavior.** Writes selected files to a destination folder; source stays read-only; destination on the source volume is blocked; same physical disk warns; names sanitized; collisions get " (n)" suffixes; SHA-256 computed during write.
**Citations.** RecoveryEngine.cs:14-18 (class doc), :24-37 CheckDestination, :45-86 Recover, :88-135 RecoverOne, :137-147 MakeUnique; PathSafety.cs:4-44.
**Algorithm.**
- `CheckDestination(sourceLetter, destFolder)` (static, pre-flight): dest root == source letter (OrdinalIgnoreCase) → **blocked** "Destination is on the SOURCE volume. Writing there can overwrite the very data you are trying to recover."; else map both to physical disk indices via `DiskEnumerator.GetPhysicalDiskIndex` (query-only handle, no admin) — equal → **warning** "Source and destination are on the same physical disk. Recover files to another drive whenever possible."; else no opinion.
- Pre-flight free-space check: `DriveInfo.AvailableFreeSpace < totalBytes` → IOException "Insufficient space on destination: need X MiB, have Y MiB." (:55-57).
- Per file: optional folder structure via `PathSafety.SanitizeRelativePath(OriginalPath)` (segments split, "."/".." dropped, each segment sanitized); filename via `SanitizeFileName`; `IsInsideRoot` containment re-check → failure item "Unsafe output path rejected." (:91-98).
- PathSafety.SanitizeFileName (:15-26): whitespace→"unnamed"; `..`→`_`; invalid filename chars / `/` / `\` / control chars <0x20 → `_`; trim + strip trailing dots; reserved DOS device names (CON, PRN, AUX, NUL, COM1-9, LPT1-9) prefixed `_`; 200-char cap.
- Collision: `MakeUnique` tries `stem (i).ext` i=1..9999, else `stem_{GUID:N}.ext` (:137-147).
- Write: `FileMode.CreateNew` (never overwrites), 1 MiB stream buffer, 4 MiB read buffer, incremental SHA-256 (`TransformBlock` per chunk) (:106-121). Status: 0 bytes written → Failed + **file deleted** + "No data could be read from the source extents."; `written ≥ target` → Recovered; else PartiallyRecovered + "Only X/Y bytes were readable." (:122-133).
- Per-file exceptions (except cancellation) → Failed item with `ex.Message`; progress phase `Recovering {FileName}` with byte totals (:59-83).
**UX.** CLI exit code 4 = blocked destination, warning printed (Program.cs:114-116); GUI: blocked → error MessageBox, warning → Yes/No "Continue anyway?", then a confirm dialog listing SOURCE/DEST/FILES, then completion box offering to open the destination in Explorer (MainViewModel.cs:318-361).
**Tests.** PathSafetyTests (EngineTests.cs:62-87): 6-case theory incl. `..\..\evil.exe`→`____evil.exe`, `CON`→`_CON`, `trailing...`→`trailing_`; traversal segments removed; IsInsideRoot rejects `C:\Windows\evil.dll` under root `C:\Recovered`.
**Edge cases.** Resident-data files write from memory (ExtentByteSource); zero-extent files fail cleanly; note: local variable `expected` (RecoveryEngine.cs:104) is computed but unused (dead code).

### 2.10 Recovery reports (text + JSON)
**Behavior.** Every recovery session writes a timestamped text and JSON report into the destination folder.
**Citations.** RecoveryReport.cs:9-61.
**Format.** `recovery-report-{yyyyMMdd-HHmmss}.txt` + `.json` sibling (:35, :58). Text: header (date, source volume, destination, duration hh:mm:ss, "Result: N recovered, M partial, K failed (total T)"), 72-char rule, per item: `[Status] name (N bytes, confidence P%)`, `-> output path`, `SHA-256: <hex>`, `Note: <error>` (:19-33). JSON mirrors with summary{recovered,partial,failed,total} and per-file {name, originalPath, size, status, output, sha256, confidence, health, error} (:38-57), indented.
**Tests.** Test lab excludes `recovery-report*` from hash comparison (Run-TestLab.ps1:112, Test-OfflineImages.ps1:52).

### 2.11 Progress / pause / cancel
**Behavior.** All long ops report ScanProgress and honor CancellationToken; Deep Scan additionally supports cooperative pause/resume; UI never freezes (async/await throughout).
**Citations.** ScanProgress.cs:4-26; PauseToken.cs:4-13 (ManualResetEventSlim, `WaitIfPaused(ct)` blocks the scan thread while observing cancellation); DeepScanner.cs:50-51; MainViewModel.cs:100-112, 155-164; .cursorrules:29 ("UI thread'i ... bloklama").
**Algorithm.** Percent = min(100, bytes/total); Speed = bytes/elapsed (only after 0.5 s); ETA = remaining/speed, null if >30 days or unknown (ScanProgress.cs:12-25). UI shows phase, %, indeterminate flag when BytesTotal ≤ 0 (FAT32/exFAT dir phase), elapsed, remaining, speed MB/s, files found (MainViewModel.cs:103-112). Cancel = CTS.Cancel (scan loops call `ct.ThrowIfCancellationRequested()` per chunk/cluster); BackHome also cancels and disposes the engine (:166-176).
**Edge cases.** Pause button only affects Deep Scan in practice (only DeepScanner polls PauseToken); Quick scanners support cancel only.

### 2.12 Error surfaces & logging
**Behavior.** Failures are surfaced with specific, actionable messages; a rolling daily log records operational metadata only.
**Citations.** RawDiskReader.cs:47-57 (Win32 error mapping: 5→UnauthorizedAccessException "Administrator rights are required for raw disk access.", 2/3→FileNotFoundException "may have been disconnected", 32→IOException "locked by another process", else generic with error code); Log.cs:8-51 (levels Debug/Info/Warn/Error, MinLevel=Info, format `yyyy-MM-dd HH:mm:ss.fff [LEVEL] [component] message`, file `%LOCALAPPDATA%\HalimRecovery\logs\halim-{yyyyMMdd}.log`, append under lock, all exceptions swallowed, OnMessage event hook); App.xaml.cs:12-21 (DispatcherUnhandledException → MessageBox + log + Handled=true; AppDomain fatal → log); Program.cs:45-56 (CLI: UnauthorizedAccessException → exit 3 + "Run this tool from an elevated (Administrator) prompt.", other → exit 1 + log); MainViewModel.cs:127-146 (scan errors: unauthorized → "Administrator rights required" box, NotSupported → "Unsupported filesystem" info box, generic → error box + log; status message line also updated).
**Privacy.** "Never log personal file contents, passwords or secrets — only operational metadata" (Log.cs:8-9; SECURITY.md:19).

### 2.13 Drive/volume selection UX & device enumeration
**Behavior.** Home screen volume combo (letter, label, [filesystem]) + refresh; SSD warning banner; default selection = first non-C volume else first.
**Citations.** MainViewModel.cs:47-62 (RefreshDrives; `SelectedVolume ??= ...FirstOrDefault(v => !v.DriveLetter.Equals("C"...)) ?? FirstOrDefault()`); MainWindow.xaml:28-51 (combo template `E: Label [NTFS]`, refresh ⟳ button, SSD banner text); DiskEnumerator.cs:23-119.
**Algorithm.** Physical disks via WMI `Win32_DiskDrive` (Index, Model, Size, MediaType, InterfaceType); volumes via `DriveInfo.GetDrives()` skipping CDRom/Network/NoRootDirectory (:69-70); volume→disk mapping via `IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS` (0x00560000) on a 0-access (query-only) handle — "Does not require admin" (:91-105, reads extent count@0, disk index@8); SSD via storage-namespace WMI `MSFT_PhysicalDisk.MediaType==4` (3=HDD, :107-119, unavailable → assume not SSD). GUI shows only the volume list (physical-disk view only in CLI `list-disks`).
**Health indicators.** No SMART/health telemetry; only the SSD/TRIM warning (README.md:98-106 "Halim Recovery shows an SSD warning and will honestly report what it finds").
**Tests.** None (hardware-dependent); exercised by CLI `list-disks` output format (Program.cs:58-67: `Disk N: Model — X GiB [SSD]` + per-volume `E: [NTFS] "Label" N MiB`).

### 2.14 Result presentation (grid, filters, sorting, confidence display)
**Behavior.** Results appear in a virtualized DataGrid sorted by confidence (descending) at load; filter combo + live substring search + smart filter; right-hand preview + health-details pane.
**Citations.** MainViewModel.cs:121-122 (OrderByDescending Confidence), :188-240 (FilterOptions + FilterPredicate); MainWindow.xaml:154-193 (grid columns: File Name 220, Original Path *, Size 80, Type 60, Date 120, Health 80, Conf. 55, Found by 80; EnableRowVirtualization + Recycling; Extended selection); FileItemVm.cs:11-38 (SizeText B/KB/MB/GB formatting, TypeText = ext or category, DateText local `yyyy-MM-dd HH:mm`, SourceText "Deep scan"/"Metadata", HealthText High/Partial/Low, HealthBrush green #37C871 / yellow #E8B33C / red #E05B5B, HealthTooltip = joined HealthNotes).
**Filters.** Combo: All / Images / Videos / Documents / Audio / Archives / High Confidence (Green) / Partial (Yellow) / Corrupted (Red) (MainViewModel.cs:188-189). SearchText live-filters name OR path substring (:234-236). No size filter; no explicit column-click sorting code (DataGrid default behavior only).
**UX.** Status overlay message bottom-left (e.g. "N recoverable file(s) found. Select files and press Recover.", MainWindow.xaml:210-213).

### 2.15 Offline natural-language smart search (TR + EN)
**Behavior.** Converts phrases like "tatil fotoğrafları geçen ay" / "invoice PDFs from 2025" into a deterministic FileFilter; purely rule-based, no cloud/AI.
**Citations.** NaturalQueryParser.cs:29-35 (class doc), :37-97 Parse, :99-106 SeasonRange; MainViewModel.cs:197-214 (ApplyNaturalQuery/Clear commands; Enter key binding MainWindow.xaml:137-139; status "Smart filter applied (rule-based, offline).").
**Algorithm.** Lowercased query → (1) category keywords TR/EN (fotoğraf/foto/resim/görsel/photo/picture/image → Image; video/film/movie/klip; müzik/ses/şarkı/music/audio/song; belge/doküman/document/evrak; arşiv/archive/sıkıştırılmış) (:44-53); (2) explicit extension mentions from a 14-extension list — if any, category is cleared ("extensions are more specific", :56-58); (3) time ranges: bugün/today → after today 00:00; dün/yesterday → [yesterday, today); "geçen hafta"/"last week" → after −14 d; "geçen ay"/"last month" → after −2 months; "geçen yıl"/"last year" → after −2 years (:61-70 — deliberately generous windows); (4) year mention 2000..now+1 with season keywords → yaz/summer [Jun 1, Sep 1), kış/winter [Dec y−1, Mar y), bahar/spring [Mar, Jun), sonbahar/güz/autumn/fall [Sep, Dec), plain year → calendar year (:73-80, :99-106); (5) leftover words (≥3 chars, not stop-words — large TR+EN stop list incl. inflected forms like "fotoğraflarını", "pdfleri" :83-90) → first one becomes `NameContains` (:91-94).
- FileFilter.Matches (:14-26): name-or-path contains; category; extension list; date = ModifiedUtc ?? CreatedUtc with null failing date-bounded filters; `MinHealth` uses enum ordering Green<Yellow<Red (`f.Health > MinHealth` → exclude). Note: NaturalQueryParser never sets MinHealth — that dimension is unused by NLQ.
**Tests.** NaturalQueryParserTests (EngineTests.cs:110-157) with fixed Now=2026-08-13: Turkish photo query → Image + ModifiedAfter + NameContains "tatil"; "fatura PDF'leri" → pdf extension + "fatura"; "2025 yazında ... videoları" → Video + [2025-06-01, 2025-09-01); FileFilter positive+negative match.

### 2.16 CLI
**Behavior.** `HalimRecoveryCli.exe` for scripting/test automation.
**Citations.** Program.cs:11-160.
**Commands.** `list-disks`; `quick-scan <drive> [--json out.json]`; `deep-scan <drive> [--json out.json]`; `recover <drive> --dest <folder> [--mode quick|deep] [--filter <name>] [--all]`; `gen-testfiles <dir> [--manifest out.json]` (:17-21, 39). Help banner includes admin/TRIM/same-drive notes (:13-27).
**Exit codes.** 0 OK; 1 unhandled error; 2 usage; 3 access denied (+elevation hint); 4 blocked destination; 5 recovery where every file failed (:45-56, :115, :142).
**Output.** Progress = single-line `\r` rewrite `% | files | MiB/s | elapsed` (:77-78); result table top-50 by confidence `[Health] Conf% Size Path\Name` + "... and N more" + TOTAL (:85-88); JSON export per file {Id, FileName, OriginalPath, Size, ext, Health, Confidence, Source, notes, CreatedUtc, ModifiedUtc, fragmented} (:92-99). `recover` default selection = non-Red files (all with `--all`) filtered by optional filename substring; `preserveFolderStructure: true`; report written; summary line (:128-142).
**Edge cases.** `gen-testfiles` uses SampleFiles.GenerateSet — 14 files (photo_holiday.jpg, screenshot.png, animation.gif, fatura_invoice.pdf, report.docx, budget.xlsx, slides.pptx, backup.zip, video_clip.mp4, song.mp3, sound.wav, notes.txt, subfolder\nested_doc.docx, subfolder\nested_photo.jpg) with random payloads → unique SHA-256 manifest (SampleFiles.cs:14-45). Builders emit structurally valid files (JPEG with escaped 0xFF payload so "the payload can't fake an EOI marker", :65-67; MP3 frames 0xFF 0xFB 0x90 0x00 = 128 kbps/44100 Hz → 417 B frames with fake-sync scrubbing, :159-172).

### 2.17 WPF GUI (dark theme, MVVM)
**Behavior.** Two-screen flow (Home → Results) + footer + support dialog.
**Citations.** MainWindow.xaml:1-215; App.xaml:5-135 (dark palette Bg #0F1216, Card #171B22, Accent #4F8EF7, health colors; styled Button/TextBox/ComboBox/DataGrid/ProgressBar); MainViewModel.cs:20-387; App.xaml.cs:6-25; app.manifest:8 (requireAdministrator); MainWindow.xaml.cs:8-19 (GridSelection wiring).
**Notable UX details.** Window 1180×760 (min 960×600), centered, app icon; home = numbered 1-2 wizard card ("1. Select the drive your files were deleted from", "2. Choose a scan type") + stop-using-drive tip; SSD amber banner (MainWindow.xaml:46-51); three scan buttons with Quick Scan styled primary and "Disk Image / Planned" third; footer with version + "Made with ❤️ by Halim Toklu" + Support button; support dialog = Ko-fi (opens browser), IBAN / USDT TRC20 / e-mail with copy-to-clipboard buttons and "copied" note (SupportDialog.xaml:1-54, .cs:10-29 — donation identifiers intentionally published by the author; values not reproduced here). Recovery flow dialogs listed in 2.9. Unhandled UI exception → MessageBox pointing at the log directory (App.xaml.cs:14-18).
**Anomaly.** CLI banner URL is truncated: `(c) Halim Toklu — https://github.com/` (Program.cs:15).

### 2.18 Session persistence, settings, i18n
**Behavior.**
- **Session persistence: NOT FOUND IN CODE.** No scan-results save/load, no settings file, no recent-destination memory; every run starts fresh (grep across src/ shows no Settings/Preferences/IsolatedStorage/Registry usage besides Log directory creation).
- **Settings: none.** Only `Log.MinLevel` (programmatic, Log.cs:15) and DeepScanner.CarveText/IsOffsetAllocated (init-only, unwired).
- **i18n: none at runtime** — UI strings are hardcoded English; however the natural-language search understands Turkish and English keywords (NaturalQueryParser.cs:43-90), and the Inno Setup installer ships English + Turkish wizard languages (HalimRecovery.iss:33-35). `.cursorrules` (repo-internal AI-agent instructions, Turkish) also documents bilingual intent but is tooling config, not app i18n.

### 2.19 Tests and their assertions
**Unit tests (xUnit, tests/HalimRecovery.Tests/, 33 facts):**
- NtfsParserTests.cs — DataRunDecoder (5): single run 0x21 header decode (len 0x18, LCN 0x5634); delta accumulation (0x100→0x120); negative delta sign extension (0xF0 → −0x10); sparse run Lcn=−1; garbage `{FF FF FF}` → empty (never throws). MftRecord (4, via synthetic record builder with real fixup stamps :60-109): deleted record parsed with name "vacation.jpg", parent=5, resident data; InUse flag honored; non-FILE buffer → null; corrupted sector-end USN → null (torn write). FileSystemDetector (4): NTFS/EXFAT/FAT32 detected from OEM/type strings; zero sector → Unknown.
- EngineTests.cs — HealthScorer (4, see 2.7); PathSafety (3 incl. 6-case InlineData theory, see 2.9); ExtentByteSource (2): resident read at offset; extents-missing file → Length 0; NaturalQueryParser (4, see 2.15).
- FormatSpecTests.cs (13, see 2.6) — driven through `FormatSpecs.All.First(name).Measure(new ByteArraySource(data), 0)`.
- csproj: xunit 2.9.2, Test.Sdk 17.12.0, coverlet.collector (Tests.csproj:11-14); references Core only.
**End-to-end test lab (PowerShell, requires admin, virtual disks only):**
- `Run-TestLab.ps1`: per FS in {NTFS, FAT32, exFAT} — diskpart create 256 MB expandable VHD, format label HALIMTEST, assign letter (auto-assign from Z.. if needed, :46-57); `gen-testfiles` + SHA-256 manifest; **Write-VolumeCache flush before delete (lazy-writer guard) and after delete (metadata flush)** (:88-96); permanent delete `Remove-Item -Recurse -Force` (no Recycle Bin); **deliberately no detach/reattach** — documented reason: remount writes System Volume Information/indexer files that "reuse the freshly freed directory clusters and destroy the deleted entries" on FAT32/exFAT (:98-102, echoed docs/BENCHMARK.md:48-54); quick-scan recovery via CLI `recover --mode quick --all`; hash-verified classification EXACT/PARTIAL/FAILED matched by leaf filename (deep-scan pass matched by hash since names are synthetic, :126-141); writes results/benchmark.{json,md} + transcript lab-run.log; VHD detached and deleted.
- `New-DebugImages.ps1`: persistent fixed 128 MB VHD fixtures with deleted files (detached after creation) for offline debugging.
- `Test-OfflineImages.ps1`: **no admin required** — extracts MBR partition 1 from the fixed VHD to a raw `.img` by reading LBA start@454 / sector count@458 (:28-44), runs `cli recover <img> --mode quick --all` (image-file mode of ScanEngine), verifies hashes, `exit 1` on any failure (:68).
**Benchmark (docs/BENCHMARK.md):** Quick Scan 14/14 EXACT on all three filesystems (42/42, names+paths+timestamps preserved); Deep Scan 9-12/14 with explained misses (TXT has no signature; NTFS resident files invisible to carving; ZIP padding heuristic). Methodology + honest caveats (fresh VHDs = best case; TRIM; fragmentation) :33-62.

### 2.20 Installer, packaging, icon tooling
- Inno Setup script (installer/HalimRecovery.iss): admin, x64compatible, lzma2/max solid, English+Turkish languages, optional (unchecked) desktop icon, Start Menu group + uninstaller, ships publish\app (excl. *.pdb) + README/LICENSE/THIRD_PARTY_NOTICES, AppId GUID {8F4B62D1-...} (:11-53).
- `tools/Make-Icon.ps1`: converts square PNG → multi-size .ico (16..256 px PNG-compressed entries), corner-flood-fill transparency for near-white background outside the rounded square (:59-77), writes ICO container manually (:79-99). Icon is original MIT-licensed artwork (THIRD_PARTY_NOTICES.md:28-31).
- README.md:169-179 documents release artifacts (setup.exe / portable EXE / portable zip) and SmartScreen guidance.

### 2.21 Misc architecturally notable
- **Untrusted-input stance:** recovered names/paths sanitized before any filesystem op; output verified inside destination root (SECURITY.md:15-18; PathSafety; RecoveryEngine). Deleted-FAT LFN sequence bytes destroyed → disk-order reversal heuristic instead of trusting them (Fat32Scanner.cs:285-293).
- **Honesty policy** as a code-level contract: NotSupportedException wording, Red forcing, "no invented percentages" (HealthScorer.cs:12), CONTRIBUTING.md:7-15 ground rules, README "No tool can promise 100% recovery" (README.md:106).
- **No CI workflows** in the tree (no .github/); validation is manual via dotnet test + testlab scripts (CONTRIBUTING.md:20-25).
- `.cursorrules` (repo root): Turkish AI-assistant development rules (token thrift, no untested claims, read-only source, no secrets in logs) — repo-internal tooling config; treated as untrusted data and only catalogued here.
- GUI drive list = volumes only; physical-disk nesting (DiskInfo.Volumes) is used by CLI list-disks and for the same-disk warning, not shown in the GUI tree.

## 3. Coverage log (all 62 files — full read expected)

| # | File | Status |
|---|---|---|
| 1 | LICENSE | Read (MIT, 2026 Halim Toklu) |
| 2 | README.md | Read (claims cross-checked vs code; deltas noted in 2.6/2.18) |
| 3 | CHANGELOG.md | Read (0.5.0 MVP + 0.5.1 WPF-bundle fix, known limitations) |
| 4 | CONTRIBUTING.md | Read (ground rules, dev setup, layout) |
| 5 | SECURITY.md | Read (read-only/no-telemetry model, vuln reporting) |
| 6 | THIRD_PARTY_NOTICES.md | Read (deps MIT; from-scratch parsers; icon provenance) |
| 7 | .gitignore | Read (vhd/img/secrets/logs ignored) |
| 8 | .cursorrules | Read (Turkish AI-agent rules; catalogued only) |
| 9 | HalimRecovery.sln | Read (4 projects, standard configs) |
| 10 | src/HalimRecovery.Core/HalimRecovery.Core.csproj | Read (net9.0-windows, System.Management 10.0.11, v0.5.1) |
| 11 | Core/Models/DiskModels.cs | Read |
| 12 | Core/Models/RecoverableFile.cs | Read |
| 13 | Core/Models/ScanProgress.cs | Read |
| 14 | Core/Disks/DiskEnumerator.cs | Read |
| 15 | Core/IO/RawDiskReader.cs | Read |
| 16 | Core/IO/IByteSource.cs | Read |
| 17 | Core/FileSystems/FileSystemDetector.cs | Read |
| 18 | Core/FileSystems/Ntfs/NtfsBootSector.cs | Read |
| 19 | Core/FileSystems/Ntfs/MftRecord.cs | Read |
| 20 | Core/FileSystems/Ntfs/DataRunDecoder.cs | Read |
| 21 | Core/FileSystems/Ntfs/NtfsScanner.cs | Read |
| 22 | Core/FileSystems/Fat32/Fat32Scanner.cs | Read |
| 23 | Core/FileSystems/ExFat/ExFatScanner.cs | Read |
| 24 | Core/Carving/FormatSpecs.cs | Read |
| 25 | Core/Carving/DeepScanner.cs | Read |
| 26 | Core/Health/HealthScorer.cs | Read |
| 27 | Core/Preview/PreviewProvider.cs | Read |
| 28 | Core/Recovery/RecoveryEngine.cs | Read |
| 29 | Core/Recovery/PathSafety.cs | Read |
| 30 | Core/Reporting/RecoveryReport.cs | Read |
| 31 | Core/Logging/Log.cs | Read |
| 32 | Core/Scanning/ScanEngine.cs | Read |
| 33 | Core/Scanning/PauseToken.cs | Read |
| 34 | Core/Search/NaturalQueryParser.cs | Read |
| 35 | Core/TestSupport/SampleFiles.cs | Read |
| 36 | src/HalimRecovery.Cli/HalimRecovery.Cli.csproj | Read |
| 37 | Cli/Program.cs | Read |
| 38 | src/HalimRecovery.App/HalimRecovery.App.csproj | Read |
| 39 | App/App.xaml | Read (dark palette + control styles) |
| 40 | App/App.xaml.cs | Read (global exception handlers, AppVersion) |
| 41 | App/AssemblyInfo.cs | Read (ThemeInfo boilerplate) |
| 42 | App/app.manifest | Read (requireAdministrator, Win10/11 GUID) |
| 43 | App/MainWindow.xaml | Read (full layout) |
| 44 | App/MainWindow.xaml.cs | Read (GridSelection wiring) |
| 45 | App/ViewModels/MainViewModel.cs | Read |
| 46 | App/ViewModels/FileItemVm.cs | Read |
| 47 | App/Views/SupportDialog.xaml | Read |
| 48 | App/Views/SupportDialog.xaml.cs | Read |
| 49 | App/Assets/app.ico | Binary asset — existence verified; described via Make-Icon.ps1 + THIRD_PARTY_NOTICES.md:28-31 (not text-readable) |
| 50 | assets/icon.png | Binary asset — same as above |
| 51 | docs/ARCHITECTURE.md | Read |
| 52 | docs/BENCHMARK.md | Read |
| 53 | installer/HalimRecovery.iss | Read |
| 54 | testlab/Run-TestLab.ps1 | Read |
| 55 | testlab/New-DebugImages.ps1 | Read |
| 56 | testlab/Test-OfflineImages.ps1 | Read |
| 57 | tools/Make-Icon.ps1 | Read |
| 58 | tests/HalimRecovery.Tests/HalimRecovery.Tests.csproj | Read |
| 59 | tests/.../NtfsParserTests.cs | Read |
| 60 | tests/.../EngineTests.cs | Read |
| 61 | tests/.../FormatSpecTests.cs | Read |
| 62 | tests/.../TestData.cs | Read |

(Line-count basis: 60 text files ≈ 5,930 lines + 2 binary assets = 62 tracked files.)

## 4. Harvest notes

**Strongest harvestable behaviors (re-implementable from this sheet alone):**
1. **Evidence-weighted health scoring** (55/35/10 weights, −1-unknown→25, ≥0.99 reuse forces Red≤10, HealthNotes as user-visible evidence journal) — HealthScorer.cs:16-86.
2. **NTFS deleted-record pipeline**: USN fixup validation → in-use flag test → $FILE_NAME namespace ranking → parent-chain path reconstruction with `?\` unknown-root marker → $Bitmap overwrite fraction — NtfsScanner.cs + MftRecord.cs.
3. **Orphan-directory harvesting** (FAT32/exFAT): "live-looking dir whose first cluster is free" → treat whole subtree as recoverable; children keep their entries because Windows never rewrites them — Fat32Scanner.cs:131-139/185-208.
4. **exFAT allocation-bitmap-as-authority** (FAT stale after delete) + NoFatChain exactness note — ExFatScanner.cs:101-141, 258-262.
5. **Structural carving validators** (esp. ZIP EOCD + central-directory pointer validation + Office classification; JPEG SOS/EOI with byte overlap; PDF last-%%EOF with 8 MiB window; MP3 frame-length walk with ≥3-frame confidence gate) — FormatSpecs.cs.
6. **Safe-recovery contract**: block same-volume dest, warn same-physical-disk (query-only extents IOCTL, no admin), name sanitization w/ reserved device names, ` (n)` collisions, CreateNew + incremental SHA-256, Failed files deleted — RecoveryEngine.cs.
7. **Offline TR/EN natural-language filter** with season ranges, stop-word list, extension-over-category precedence — NaturalQueryParser.cs.
8. **Hash-verified VHD test lab** with the double-flush and no-remount lessons (remount destroys FAT/exFAT deleted entries) — Run-TestLab.ps1:88-102, docs/BENCHMARK.md:48-54.
9. **Progress/ETA/pause/cancel model** (ScanProgress snapshot type, PauseToken, indeterminate when BytesTotal=0) — ScanProgress.cs/PauseToken.cs.
10. **Preview-from-source** (DOCX/XLSX/ZIP via in-memory ZipArchive over first 32 MiB; PDF header/page-count info; deferred image decode) — PreviewProvider.cs.

**Weaknesses / gaps (do not replicate blindly):**
- Contiguity assumption for FAT32/exFAT non-NoFatChain files (fragmented files recover wrong bytes silently — Yellow health only via +3 layout score); fragmented reconstruction explicitly not implemented (CHANGELOG.md:43-44, README.md:135).
- No dedup between Quick and Deep results; deep-scan lab counts "partial" = carved-but-not-matched.
- `DeepScanner.IsOffsetAllocated` / `CarveText` dead hooks; `FileFilter.MinHealth` never produced by the NLQ parser; `expected` dead variable in RecoverOne (RecoveryEngine.cs:104).
- NtfsScanner allocates a fresh `byte[recSize]` per record (GC pressure on multi-million-record MFTs); $Bitmap assumed in first MFT extent; MFT mirror unused; sequence numbers ignored.
- No session persistence, no settings, no runtime i18n, no size filter, no explicit sort controls, no SMART/health telemetry.
- No CI in tree; unit tests do not cover the three scanners end-to-end (delegated to admin-only testlab).

**Licensing observations.** MIT (2026); both runtime deps MIT; parsers written from scratch against public specs with an explicit no-GPL policy (docs/ARCHITECTURE.md:12-17, THIRD_PARTY_NOTICES.md:15-25, CONTRIBUTING.md:13-14) — clean reference for behavioral harvest; no copied code in this sheet regardless. README/SupportDialog publish personal donation coordinates (Ko-fi/IBAN/USDT/e-mail) by author intent — not reproduced in this sheet beyond noting their existence.
**README-vs-code deltas:** README:66 says `cd kurtarmak` (repo dir is `halim-recovery`); "TXT carving disabled by default" (CHANGELOG:46) = actually no TXT carver exists; image-file scanning (ScanEngine ctor) is implemented but undocumented in README usage; everything else checked (quick/deep, 3 FS, 12 carve formats, reports, CLI commands, installer, benchmark numbers) matches code.
