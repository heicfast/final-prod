# NTFS-RECOVER — Feature Context Sheet
> Agent F (Task 2-F), Phase-0. READ-ONLY extraction at HEAD 5072bbc729b293efc0b2070b38ad30c3b7e043cf, 0BSD. Date 2026-11-19. Every claim cites file:line.
> Repo: `mjgil-rust/ntfs-recover` (local: /home/z/my-project/repos/ntfs-recover), crate name `rust-ntfs-recover` v0.1.0. All 25 tracked files read in full. No git commands run, nothing executed.

## 0. Identity
- URL: https://github.com/mjgil-rust/ntfs-recover (per session brief; sources.txt confirms reference ecosystem). Branch `main`, HEAD 5072bbc.
- License: **BSD Zero Clause (0BSD)** — LICENSE:1-11. Harvest-safe: permission to use/copy/modify/distribute for any purpose. We still only take algorithms, not code.
- Layout (25 tracked files): 6 top-level modules + 10 parser modules + 4 docs + lockfile + manifest + test script + license + .gitignore.
- LOC: **4,005 lines of Rust** (main 388, scanner 566, rebuilder 391, extractor 349, repairer 858, types 107, parser/* 1,346 — includes ~15 inline unit tests). Docs: README 260, SPEC 613, DEBUGGING 182, sources.txt 109. README:160 claims "~3,500 lines" (understated).
- Dependencies (Cargo.toml:7-18): memmap2 0.9, rayon 1.10, clap 4 (derive), indicatif 0.17, serde+serde_json+bincode 1, glob 0.3, anyhow 1, log 0.4, env_logger 0.11. Release profile: opt-level 3 + LTO (Cargo.toml:20-22). No `unsafe` anywhere in src/ (verified by grep).
- Binary: `target/release/rust-ntfs-recover` (README:29).

## 1. Architecture
Four-phase pipeline, each phase resumable via a `.rntfs` scan index (README:11-21; main.rs:166-377):

| Phase | File | Role |
|---|---|---|
| 1 SCAN | src/scanner.rs:211-310 `scan_device` | brute-force whole-device record discovery, resumable index |
| 2 REBUILD | src/rebuilder.rs:21-155 `rebuild_tree` | parse attrs, dedup, parent-ref tree, path resolution |
| 3 EXTRACT | src/extractor.rs:25-123 `extract_files` | read runs/resident data, write output tree |
| 4 REPAIR | src/repairer.rs:28-170 `repair_mft` | rebuild $MFT run list, patch record 0 in-place |

Shared state in src/types.rs: `ScannedRecord` (disk_offset, mft_record_number, sequence_number u16, flags, base_record, raw_data Vec<u8> post-fixup; types.rs:7-20), `FileEntry` (types.rs:24-45), `DirectoryTree` (entries map, children map, root_record=5, orphaned vec; types.rs:49-69), `MftFragment` (logical_start, cluster_start, cluster_count; types.rs:83-90), `ScanResult` (geometry + fragments + records; types.rs:94-107).

Error model: two layers. Parser layer = `ParseError` enum with offsets (BufferOverflow/InvalidSignature/FixupMismatch/InvalidAttributeLength/InvalidDataRun/Generic; parser/error.rs:5-35) plus bounds-checked LE read helpers `read_u16_le…read_int_le` (error.rs:82-195). Orchestration layer = `anyhow::Result`. Design rule: **best-effort everywhere** — attribute iteration returns partial results on bad attribute (parser/attributes.rs:162-202), data-run decode returns partial runs on negative absolute offset (parser/data_runs.rs:84-89), fixup failure does not reject a record (parser/mft_record.rs:102-109: `let _ = apply_fixup_array(...)`).

## 2. MFT record brute-force scan

### 2.0 Record header layout (parser/mft_record.rs:50-95, SPEC.md:43-60)
```
0x00 4  "FILE"   0x04 2 usa_off   0x06 2 usa_size(words)   0x08 8 LSN
0x10 2 seq      0x12 2 links    0x14 2 first_attr_off      0x16 2 flags(0x01 in-use, 0x02 dir)
0x18 4 real_sz  0x1C 4 alloc_sz 0x20 8 base_record_ref    0x28 2 next_attr_id
0x2C 4 record# (NTFS 3.1+; read with unwrap_or(0))
```
`base_record_number()` masks low 48 bits (mft_record.rs:34-36); `is_base_record()` == reference==0 (mft_record.rs:38-40).

### 2.1 Geometry bootstrap (scanner.rs:217-228)
- Boot sector read from LBA 0, 512 bytes; `parse_boot_sector` (parser/boot_sector.rs:20-76) validates OEM "NTFS    " at 0x03, reads bytes/sector (0x0B), sectors/cluster (0x0D), MFT LCN (0x30), MFTMirr LCN (0x38), and the signed record-size encoding at 0x40: >0 → clusters-per-record × cluster_size; <0 → 2^(-value) (boot_sector.rs:46-53).
- Fallbacks when BPB unparseable (scanner.rs:219-222): sector=512 (DEFAULT_SECTOR_SIZE, scanner.rs:18), cluster=4096, record=1024 (DEFAULT_RECORD_SIZE, scanner.rs:17), mft_start_offset=0. `--cluster-size` overrides only the cluster value; note it also changes the computed `mft_start_offset = mft_start_cluster * cluster_size` (scanner.rs:222).

### 2.2 Stride & alignment (scanner.rs:314-358 `scan_chunk`)
The device is walked in 64 MB chunks (CHUNK_SIZE, scanner.rs:16), each processed by rayon `par_chunks(1 MB)` (scanner.rs:276-279). Within a sub-chunk: `pos` starts at 0 and advances **by `record_size`** (scanner.rs:355) — i.e. candidates are only examined at record-size-aligned offsets (typically every 1024 bytes; README:13 "every 1024-byte boundary"). **Single record-size strategy**: no multi-pass over 512/2048/4096 candidates; the size comes solely from BPB-or-default. Sub-chunk (1 MB) and chunk (64 MB) sizes are multiples of any power-of-two record size, so alignment is preserved across chunk seams (scanner.rs:16,275).

### 2.3 Accept/reject gate (scanner.rs:328 → parser/mft_record.rs:115-134 `validate_mft_record`)
1. `len >= 48` and bytes[0..4] == `FILE` (`has_file_signature`, mft_record.rs:44-47; magic const FILE_SIGNATURE mft_record.rs:5).
2. Allocated size (header 0x1C) sanity: reject only if `allocated > 65536`. The outer test `allocated == 0 || allocated > 4096` merely routes into the inner check, so **allocated==0 and 4097..=65536 are accepted** (mft_record.rs:119-127).
3. First-attribute offset (0x14) must be `>= 0x30` and `< allocated` (mft_record.rs:129-132).
- **NOT validated at scan time**: USA count vs record size, fixup signature match, sequence number, flags. Fixup is applied *after* acceptance, best-effort, and its error is discarded (mft_record.rs:99-112) — a record with a mismatching fixup is still stored, with sector-trailer bytes un-replaced.
- Fixup application (parser/fixup.rs:15-60 `apply_fixup_array`): `usa_size < 2` → no-op; else signature word at `usa_offset`, then for each of `usa_size-1` sectors: verify last-2-bytes of sector `(i+1)*sector_size - 2` equal the signature (else `FixupMismatch`), then overwrite those 2 bytes with the original word stored at `usa_offset + 2 + i*2`. Record shorter than expected → stop early (fixup.rs:31-36).

### 2.4 Record-number assignment (scanner.rs:332-339), in priority order:
1. If the scan-time MFT fragment map covers the disk offset → logical record number via `resolve_mft_record_number` (scanner.rs:158-176: cluster math against fragments, `logical_start + cluster_offset*recs_per_cluster + record_in_cluster`).
2. Else the header's own `mft_record_number` (0x2C, mft_record.rs:79) if non-zero.
3. Else `disk_offset / record_size` (positional guess).

### 2.5 Fragment map pre-pass (scanner.rs:61-156 `parse_mft_fragments`)
Reads record 0 at `mft_start_offset`, requires FILE signature, applies fixup (result ignored, scanner.rs:86), iterates attributes to find the first unnamed non-resident $DATA, decodes its runs; sparse runs advance logical numbering only; a run whose `cluster_offset > 0x1_0000_0000_0000` aborts the map as corrupt (scanner.rs:107-115). Fragments = (logical_start, cluster_start, cluster_count) with `recs_per_cluster = cluster_size / record_size`. This map may be partial/truncated — exactly the corruption case that `--reconstruct` later heals.

### 2.6 Resumable index format v4 (scanner.rs:21-45, 392-434)
64-byte header, magic `RNTFS004` (v3/v2 also loadable, scanner.rs:41-42,451-475; pre-magic "legacy" = whole-file bincode, scanner.rs:557-565), fields at fixed offsets: sector/cluster/record sizes, last_scanned_offset (18), device_size (26), total_records (34), mft_start_offset (38); then u32 fragment count + 24-byte fragments (cap 10,000, scanner.rs:376-378); then length-prefixed bincode `Vec<ScannedRecord>` batches. Flush every 1 GiB scanned (FLUSH_INTERVAL scanner.rs:19,285). Resume rejected if device size or geometry params mismatch (scanner.rs:466-469). Truncated batches tolerated (scanner.rs:493-518).

### 2.7 Parallelism & I/O
`rayon::ThreadPoolBuilder::num_threads(threads)` global (scanner.rs:244); default = `available_parallelism()` (main.rs:151-155). I/O is plain `File::seek`+`read` into a fresh 64 MB Vec per chunk, `truncate(n)` on short reads, `n==0` ends scan (scanner.rs:263-270) — **no mmap, no O_DIRECT, no readahead tuning** despite README:234. Device size via `seek(End(0))`, bail on 0 (scanner.rs:200-207).

Re-implementation sketch of the scan loop:
```
for chunk in device.chunks(64 MiB, resumable from header.last_scanned_offset):
    par_chunks(1 MiB).map(|sub| {
        pos = 0
        while pos + record_size <= sub.len():
            slice = sub[pos .. pos+record_size]
            if slice[0..4]=="FILE" and validate(slice):     # §2.3 gates
                raw = slice.to_vec(); hdr = parse_header(raw)
                apply_fixup(raw, hdr.usa_off, hdr.usa_size, sector_size)  # errors ignored
                mft_num = fragmap_lookup(offset) ?? hdr.record# (if ≠0) ?? offset/record_size
                emit ScannedRecord{offset, mft_num, hdr.seq, hdr.flags, hdr.base_record#, raw}
            pos += record_size
    })
    every 1 GiB: append bincode batch, patch header offset/count, flush
```

## 3. Directory-tree reconstruction
All in src/rebuilder.rs `rebuild_tree(scan, include_deleted)`.

### 3.1 Record triage (rebuilder.rs:31-55)
Extension records (`base_record != 0`, from header 0x20 lower 48 bits, mft_record.rs:34-36) are diverted to a side list; base records parsed regardless of deleted flag — comment: deleted directories are needed for path resolution on corrupted drives (rebuilder.rs:17-20,38-39).

### 3.2 Per-record parse `parse_record_to_entry` (rebuilder.rs:158-257)
- $STANDARD_INFORMATION (0x10): created/modified timestamps (rebuilder.rs:178-184; parser/standard_info.rs:13-29).
- $FILE_NAME (0x30): best-name selection among multiple $FILE_NAME attrs (hard links / DOS aliases) via "dominated" logic (rebuilder.rs:189-200): non-DOS beats DOS-only; Win32 (ns 1 or 3, filename.rs:42-44) beats non-Win32 → effective preference Win32/Win32+DOS > POSIX > DOS. `file_size` from $FILE_NAME real_size only if still 0 (rebuilder.rs:202-204).
- $DATA (0x80), **unnamed only** (`name_length == 0`, rebuilder.rs:211): flags → compressed 0x0001, encrypted 0x4000, sparse 0x8000 (rebuilder.rs:212-214); non-resident → `file_size = real_size`, decode runs from `data_run_bytes` slice (rebuilder.rs:216-224); resident → copy content bytes, size = content len (rebuilder.rs:225-230).
- Entry requires a $FILE_NAME, else record skipped (rebuilder.rs:237).

### 3.3 Duplicate handling / record reuse (rebuilder.rs:64-89)
For each mft_record_number with multiple candidates, sort by (alive desc, sequence_number desc) and keep the first — i.e. **in-use beats deleted, then highest sequence number** (matches README:211). If the winner has neither runs nor resident data and is not a directory, data runs are merged from all extension records whose `base_record` == that number (rebuilder.rs:78-86, `extract_data_runs_from_record` rebuilder.rs:260-282). NOTE: this is *not* $ATTRIBUTE_LIST-driven — see §8 (attribute_list.rs is dead code).

Parent links: `parent_record_number` = low 48 bits of the $FILE_NAME parent MFT reference; `parent_sequence` = high 16 bits (parser/filename.rs:62-64). **The parent sequence number is parsed but never compared** — stale-parent rejection by sequence does NOT exist in code (see §4). Parent→children map built for every entry (rebuilder.rs:104-109).

### 3.4 Path resolution (rebuilder.rs:285-357 `resolve_paths`/`build_path_cached`)
- Root is hard-coded **MFT record 5** → empty path (rebuilder.rs:291,329-333; types.rs:65).
- Memoized recursive walk with a `visited` HashSet per top-level walk: revisit → path prefix `__CYCLE__/<mft_num>` (rebuilder.rs:320-322) — cycles detected and broken, not looped.
- Parent entry missing from `entries` → `__ORPHANED__/<parent_mft_num>` becomes the prefix component (rebuilder.rs:324-327), so orphaned files land under `__ORPHANED__/<missing-parent-record>/<name>`.
- Self-parent (parent == own number) → path = own name only (rebuilder.rs:338-341).
- Orphans enumerated into `tree.orphaned` by scanning resolved paths for those markers (rebuilder.rs:360-375).

### 3.5 Deleted vs live filtering (rebuilder.rs:117-134)
After path resolution, if `!include_deleted`, entries with `is_deleted && !is_directory` are removed (flag 0x01 absent, rebuilder.rs:247), and children/orphan lists pruned — **deleted directories are always retained** for path continuity.

## 4. Sequence-number / record-reuse handling
- Sequence number (header 0x10, u16) is captured per scan record (types.rs:13) and per FileEntry (types.rs:26).
- Uses: (a) dedup tie-break "highest sequence wins" in tree rebuild (rebuilder.rs:67-74); (b) dedup tie-break in repair fragment reconstruction (repairer.rs:192-215, keeps highest sequence per record number).
- **NOT FOUND IN CODE**: validation of a child's `parent_sequence` against the parent entry's current sequence (stale-reference rejection), and any sequence check in `validate_mft_record`. The high-16-bit parent sequence is decoded (filename.rs:64) and then ignored. Consequence: after record reuse, a stale $FILE_NAME can attach a file to the wrong (reused) parent directory with no warning. This is a genuine gap a reimplementation should close.

## 5. $MFT run-list reconstruction + in-place patching
**VERDICT: fully implemented and battle-tested** (DEBUGGING.md documents a successful 2 TB repair). Two paths:

### 5a. Evidence gathering — `reconstruct_fragments` (repairer.rs:179-290)
Core insight (repairer.rs:174-177): for every record inside one physical MFT extent, `delta = disk_offset − mft_record_number × record_size` is constant. Algorithm:
1. Dedup by mft_record_number keeping highest sequence; skip extension records (base_record ≠ 0) and records beyond `max_record` (repairer.rs:195-215).
2. Group surviving records by delta (repairer.rs:227-231).
3. Each delta group → one fragment: min..max record numbers, logical range aligned outward to cluster multiples (`first − first%rpc`, `(last/rpc + 1)*rpc`), cluster_start = `(delta + logical_start*record_size)/cluster_size`, rejecting negative or non-cluster-aligned starts (repairer.rs:293-327 `make_fragment`).
4. Drop fragments < 4 clusters as stale-layout noise (repairer.rs:247-255).
5. Sort by logical_start; cap to `max_record` (cluster_count truncated via ceil-div, repairer.rs:260-275).
6. Merge fragments contiguous both logically and physically; overlapping ones are split/truncated (repairer.rs:330-369 `merge_adjacent_fragments`).

`max_record` derivation (main.rs:288-329): record 0's own $DATA header `end_vcn` (parser/attributes.rs NonResidentData, offset +0x18 within the attribute) is re-parsed from the scanned copy of record 0 → `max_rec = (end_vcn + 1) * recs_per_cluster − 1`. This is the $ATTRIBUTE_LIST awareness: when $MFT's $DATA spans record 0 + extension records (e.g. record 15 owns VCN ≥ 1,154,036), only record 0's VCN range is reconstructed so the untouched extension records keep their valid runs (DEBUGGING.md:36-48,172). If end_vcn can't be read, it reconstructs everything with a warning (main.rs:324-326).

### 5b. Encoding — `encode_data_runs` (parser/data_runs.rs:109-134)
Inverse of decode: header byte = (offset_bytes << 4) | length_bytes; length LE unsigned; offset LE **signed relative to previous run**; sparse runs = length only; 0x00 terminator. **Critical detail (harvest this)**: `min_bytes_unsigned` allocates one extra sign bit — `(bits + 1 + 7) / 8` (data_runs.rs:141-148) — because ntfs-3g's `ntfs_mapping_pairs_decompress` sign-extends run *lengths* from the MSB (DEBUGGING.md:83-141, SPEC.md:277-292). A count of 45,669 (0xB265) must be encoded in 3 bytes (`65 B2 00`), not 2, or ntfs-3g reads −19,867 → EIO. Offsets use the analogous `min_bytes_signed` (data_runs.rs:151-164).

### 5c. In-place patch — `patch_record_data_runs` (repairer.rs:467-588) — what bytes get rewritten where
Target: **MFT record 0's unnamed $DATA attribute run-list bytes only**, at byte offset `mft_start_cluster × cluster_size` on the partition (README:113 typical: 0x4000). Byte math: `runs_start = data_attr_offset + u16@attr+0x20 (data_runs_offset)`; `runs_space = u32@attr+0x04 (attr length) − data_runs_offset`; new bytes must satisfy `new_runs.len() ≤ runs_space` (repairer.rs:537-556). The attribute's own length, VCN fields, and size fields are NOT updated — only the bytes between `runs_start` and `runs_start+runs_space` (zeroed, then overwritten with the new encoding, terminator included since `encode_data_runs` appends 0x00, data_runs.rs:132).
1. Read existing record 0 (record_size bytes) at `scan.mft_start_offset`; require FILE signature (repairer.rs:479-486).
2. **Un-apply fixup**: restore USA-saved words into sector trailers, warning (not failing) on mismatch (repairer.rs:488-507).
3. Walk attributes manually (not via parser) to find the unnamed non-resident $DATA (type 0x80, non_resident==1, name_length==0); bail if absent (repairer.rs:509-535).
4. Compute `runs_space = data_attr_len − runs_offset`; **bail if new runs don't fit** — error text suggests a `--fit-record` flag that does not exist (see §8) (repairer.rs:549-556).
5. Zero the whole old runs area, then copy the new encoded runs in (repairer.rs:558-562). Everything else — record header, $ATTRIBUTE_LIST, $BITMAP, $FILE_NAME, $STANDARD_INFORMATION, attribute lengths/offsets — is preserved byte-for-byte.
6. **Re-apply fixup with a new USA signature**: `usa_sig+1` wrapping, mapping 0xFFFE→0x0001 (avoids 0x0000/0xFFFF), stored at usa_offset, sector trailers replaced, originals saved into USA entries (repairer.rs:570-585).
7. **NOT written**: boot-sector BPB fields (read-only input, boot_sector.rs / scanner.rs:180-198), extension records (e.g. record 15), $MFTMirr unless `--fix-mirror` (which writes the same patched record-0 bytes at `mft_mirror_start_cluster * cluster_size` after its own backup; repairer.rs:154-163).

Safety & preconditions before any write:
- Non-empty fragment map, else bail (repairer.rs:34-40); `mft_start_offset != 0` (boot sector must have been readable) else bail (repairer.rs:42-47); boot sector must parse at repair time (repairer.rs:49-50); record 0 on disk must carry FILE signature; unnamed non-resident $DATA must exist; runs must fit.
- Interactive **"Type 'yes'"** confirmation, skipped only for dry-run (main.rs:358-374).
- **Backup first**: raw record_size bytes of original record 0 → `mft_record0_backup_<unix_secs>.bin` (or `--backup` path); mirror gets `mftmirr_record0_backup_*.bin` (repairer.rs:142-147,154-159; timestamp fn repairer.rs:837-844). Restore recipe (dd bs=1024 seek=<mft_start/1024>) in README:109-115 and SPEC.md:314-319.
- `--dry-run`: prints repair plan, fragment table, hex dump of first 128 bytes, writes nothing (repairer.rs:117-140).
- Write path: `OpenOptions::write(true)`, seek, `write_all`, `flush` (no fsync) (repairer.rs:822-834).
- A complete from-scratch record-0 builder `build_mft_record_0` exists (repairer.rs:590-796: minimal $STANDARD_INFORMATION + $FILE_NAME "$MFT" parent=5 + non-resident $DATA with computed end_vcn/allocated/real sizes + USA 0x0001) but is **dead code** — never called; DEBUGGING.md:49-59 records that this "Attempt 1" approach failed because it drops $ATTRIBUTE_LIST/$BITMAP. The lesson "never rebuild record 0 from scratch; patch in-place" is encoded as the shipped design.

### 5d. Repair flow in main.rs (269-377)
Load scan → optionally `--reconstruct` (compare original vs reconstructed fragment maps via `print_fragment_comparison`, repairer.rs:371-445, with `+` markers for new fragments) → confirm → `repair_mft`. Without `--reconstruct`, repair uses whatever fragment map the scanner got from record 0's own (possibly truncated) runs — the README:152 note "required for corrupted runs".

Post-repair story (docs): run `ntfsfix` to sync $MFTMirr/clear journal, then mount ntfs-3g ro (README:93-97,165-167; DEBUGGING.md:142-165).

## 6. File extraction (attributes/runs)
- Attributes parsed anywhere: 0x10, 0x20 (parser only), 0x30, 0x80 (attributes.rs:5-8). Others ignored. **ADS (named $DATA) NOT implemented** — filtered out by `name_length == 0` checks (rebuilder.rs:211, scanner.rs:93); only the unnamed stream is recovered.
- Non-resident reads (extractor.rs:158-204): per run, `disk_offset = cluster_offset × cluster_size`, read up to `min(run_bytes, remaining)` in ≤1 MB chunks, honoring `file_size` truncation across runs; unexpected EOF mid-run → warn and continue with next run (partial data kept).
- Sparse runs: zero-filled up to min(run size, remaining) via a reused 1 MB zero buffer (extractor.rs:168-180).
- Resident data: copied verbatim from the stored post-fixup record bytes (extractor.rs:153-157).
- Compressed/encrypted/sparse files are flagged (list output columns `D C E S`, extractor.rs:216-223) but not decoded (README:225-227 limitation, honored).
- Filters (extractor.rs:229-256): size min/max + glob patterns matched against filename or full path. Size parser `parse_size` accepts 1K/1M/1G/1B/bare (extractor.rs:305-328, unit-tested).
- Output: `full_path` (or filename if unresolved) sanitized per component (`/ \ : * ? " < > | \0` → `_`, trailing dots trimmed, `.`/`..` dropped; extractor.rs:259-273); conflicts get `_1.._9999` suffixes (extractor.rs:276-302); `--no-dirs` flattens; files sorted by first data-run cluster to minimize seeks (extractor.rs:47-53); progress bar per file; failures counted, not fatal (extractor.rs:97-111).

## 7. CLI/UX
clap derive, subcommands `scan|list|extract|repair` (main.rs:55-149); global `-s/--scan-file` (default ./scan.rntfs), `--cluster-size`, `--threads` (default CPUs), `-v` (main.rs:30-53). Extract: `-o`, repeatable `-p` globs, `--min-size/--max-size`, `--list-only`, `--no-dirs`, `--include-deleted`, `--dry-run`. Repair: `--dry-run`, `--backup`, `--fix-mirror`, `--reconstruct`. List supports pattern/size/include-deleted filters and uses a dummy `/dev/null` device path because listing short-circuits before any device read (main.rs:185-218, extractor.rs:67-70).
Logging: env_logger, `info` default, `debug` with `-v` (main.rs:160-164). Progress: indicatif byte bar with ETA during scan (scanner.rs:246-250), per-file bar during extract (extractor.rs:86-92). Orchestration: `scan` standalone; `list` requires existing scan file; `extract` scans first if no scan file (main.rs:233-245); `repair` requires existing scan (main.rs:279-280).

## 8. Tests/fixtures
- 15 inline `#[cfg(test)]` unit tests: mft_record header parse + validate (mft_record.rs:136-176), fixup round-trip (fixup.rs:62-103), boot sector parse (boot_sector.rs:78-118), $FILE_NAME parse (filename.rs:111-140), 8 data-run decode/encode round-trips incl. sparse, negative relative offsets, 2 TB-scale cluster numbers (data_runs.rs:181-306), `parse_size` + `sanitize_filename` (extractor.rs:330-349).
- Integration fixture: tests/create_test_image.sh (54 lines) — 32 MB `mkfs.ntfs -Q -s 512 -c 4096` image, populated with resident `readme.txt` + 16 KB/64 KB/128 KB/256 KB/512 KB random binaries in Documents/Photos/Music; requires root to loop-mount. **No corruption step and no automated assertions** — verification is manual eyeballing (README:245-256).
- NOT FOUND IN CODE: unit tests for `rebuild_tree`, `reconstruct_fragments`, `patch_record_data_runs`, cycle/orphan behavior, resume logic — SPEC.md:593-601 promises "unit tests for directory tree reconstruction" and a "corrupt the MFT, verify recovery" integration test; neither exists.

## 9. README-vs-code truth table
| README claim | Status |
|---|---|
| 4 phases; resumable `.rntfs` v4 index (README:11-21,193-199) | IMPLEMENTED (scanner.rs:21-45,230-241) |
| Scan every 1024-B boundary, 64 MB chunks, rayon 1 MB sub-chunks (README:13,203) | IMPLEMENTED (scanner.rs:16,275-279,326-355); stride is `record_size` from BPB, not literally 1024 |
| Boot-sector fallback defaults 512/4096 (README:220) | IMPLEMENTED (scanner.rs:219-222); but repair bails when mft_start_offset==0 (repairer.rs:42-47) |
| Duplicate records: in-use > deleted, then highest seq (README:211) | IMPLEMENTED (rebuilder.rs:66-75) |
| Orphans → `__ORPHANED__/`, cycles → `__CYCLE__/` (README:212-213) | IMPLEMENTED (rebuilder.rs:320-327) — orphan dir name embeds the *missing parent's* record number |
| Extension records for large/fragmented files (README:216) | PARTIAL — merged via base-record scan only (rebuilder.rs:78-86); `$ATTRIBUTE_LIST` parser exists but is never called (attribute_list.rs:31, zero call sites) |
| Repair patches only $DATA runs, preserves all other attrs; detects multi-record MFT via $ATTRIBUTE_LIST VCN boundary (README:19,99-107) | IMPLEMENTED (repairer.rs:467-588; main.rs:288-329 reads end_vcn from record 0's $DATA, not by parsing $ATTRIBUTE_LIST itself) |
| Backup of record 0 + dd restore (README:109-115,154) | IMPLEMENTED (repairer.rs:142-147,808-820) |
| `--fix-mirror` (README:155) | IMPLEMENTED (repairer.rs:154-163) — writes patched record 0 over MFTMirr's record 0 |
| Sparse runs zero-filled; resident data; compressed/encrypted flagged not decoded; filename conflicts `_N`; sanitization (README:214-219) | IMPLEMENTED (extractor.rs:153-180,259-302; rebuilder.rs:212-214) |
| "memmap2 — Memory-mapped I/O" (README:234, Cargo.toml:8) | **NOT FOUND IN CODE** — no `memmap`/`Mmap` usage in src/; plain `File::read` into Vec (scanner.rs:266-270). Dependency declared+locked but unused |
| MFT fragment map from record 0 for logical numbering (README:204) | IMPLEMENTED (scanner.rs:61-176) |
| No `unsafe`, no panics on untrusted data (README:191) | VERIFIED for `unsafe` (grep zero hits); parser is panic-free by construction (bounds-checked reads) |
| ~500 MB/s NVMe target (README:207) | CLAIM ONLY — no benchmark code |
| LZNT1 decompression / EFS / $INDEX_* / journal (README:223-228 not done) | CORRECT — absent |
| `--fit-record` (repairer.rs:729-736 error text) | **GHOST FLAG** — mentioned in an error message and as an unused `RepairOptions.fit_record` field (repairer.rs:19, main.rs:355) but not a CLI arg and not implemented |
| SPEC.md testing strategy (tree-rebuild unit tests, corrupt-MFT integration test, benchmark; SPEC:593-601) | NOT FOUND IN CODE (see §8) |

## 10. Coverage log (all 25 tracked files, full read)
| File | LOC | Read | Notes |
|---|---|---|---|
| src/main.rs | 388 | ✔ | CLI, orchestration, repair confirmation, end_vcn probe |
| src/scanner.rs | 566 | ✔ | scan loop, fragment map, .rntfs v4/resume |
| src/rebuilder.rs | 391 | ✔ | tree rebuild, dedup, paths, orphans |
| src/extractor.rs | 349 | ✔ | extraction, filters, sanitize, tests |
| src/repairer.rs | 858 | ✔ | reconstruct_fragments, patch_record_data_runs, dead build_mft_record_0 |
| src/types.rs | 107 | ✔ | shared structs |
| src/parser/mod.rs | 9 | ✔ | module list |
| src/parser/error.rs | 195 | ✔ | ParseError + LE read helpers |
| src/parser/mft_record.rs | 176 | ✔ | header, validate gate, tests |
| src/parser/fixup.rs | 103 | ✔ | USA apply, test |
| src/parser/boot_sector.rs | 118 | ✔ | BPB, test |
| src/parser/attributes.rs | 202 | ✔ | attr headers, best-effort iteration |
| src/parser/filename.rs | 140 | ✔ | $FILE_NAME, namespaces, test |
| src/parser/data_runs.rs | 306 | ✔ | decode/encode + sign-bit rule, 8 tests |
| src/parser/standard_info.rs | 29 | ✔ | timestamps |
| src/parser/attribute_list.rs | 68 | ✔ | parser exists, **never called** |
| README.md | 260 | ✔ | claims audited (§9) |
| SPEC.md | 613 | ✔ | design doc incl. ntfs-3g sign-extension rationale |
| DEBUGGING.md | 182 | ✔ | real 2 TB repair war story; 3 root causes |
| sources.txt | 109 | ✔ | reference repo list (RecuperaBit, ntfs-3g, libfsntfs…) |
| Cargo.toml | 22 | ✔ | deps; memmap2 unused |
| Cargo.lock | 656 | ✔ | skimmed package list (matches manifest) |
| LICENSE | 11 | ✔ | 0BSD |
| tests/create_test_image.sh | 54 | ✔ | fixture generator, no assertions |
| .gitignore | 1 | ✔ | /target, *_backup* |

No credentials/tokens found anywhere (sources.txt and DEBUGGING.md contain only public URLs and device paths like /dev/nvme1n1p1).

## 11. Harvest notes
**Strongest algorithms to port (behavior only, 0BSD permits but we re-implement):**
1. **Delta-grouping MFT extent reconstruction** — `delta = disk_offset − record# × record_size` constant per extent; group→min/max→cluster-align→drop <4 clusters→merge (repairer.rs:174-369). Elegant, needs no boot-sector trust beyond geometry.
2. **Sign-extension-safe run-list encoder** — lengths need a spare sign bit because ntfs-3g decodes them signed (`65 B2 00` not `65 B2`); the single most transferable bug-lesson in the repo (data_runs.rs:136-164; DEBUGGING.md:83-141).
3. **In-place $DATA run patch with fixup round-trip** — un-fixup → zero+replace runs inside existing attribute → bump USA signature (0xFFFE→1 wrap) → re-fixup; never rebuild record 0 (loses $ATTRIBUTE_LIST/$BITMAP) (repairer.rs:467-588; DEBUGGING.md:49-59).
4. **VCN-boundary capping for multi-record $MFT** — reconstruct only record 0's `end_vcn` range so untouched extension records stay valid (main.rs:288-329; repairer.rs:260-275).
5. **Record-number triple fallback** — fragment-map logical number → header number → positional offset/record_size (scanner.rs:332-339).
6. **Deleted-dirs-for-paths + post-hoc pruning** — keep deleted directories during path resolution, drop deleted files afterwards unless requested (rebuilder.rs:17-20,117-134).
7. **Partial-result discipline** — attribute iteration, run decoding, and batch reading all return what parsed so far instead of failing (attributes.rs:162-202; data_runs.rs:84-89; scanner.rs:493-518).

**Weaknesses / gaps to fix in our reimplementation:**
- No parent-sequence (stale-reference) validation (§4); no USA-count/fixup validation in the scan gate (§2) — false positives possible from stale "FILE"-magic bytes inside file content at aligned offsets.
- Single record-size scan; no 512/2048/4096 multi-pass or unaligned-record fallback if BPB lies.
- `$ATTRIBUTE_LIST` ignored at runtime (extension merging is heuristic); ADS (named streams) dropped entirely.
- Repair offers no automatic `--fit-record` despite error text; no fsync after the 1 KiB write; mirror "fix" writes record 0 only.
- No automated tests for the tree/repair core; integration fixture lacks a corruption+assert step.
- Unused `memmap2` dependency; README/Cargo suggest mmap I/O that doesn't exist.
