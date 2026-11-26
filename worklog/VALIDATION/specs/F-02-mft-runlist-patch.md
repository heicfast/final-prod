# F-02 Implementation Spec — $MFT Run-List Reconstruction + In-Place Patch

> Wave 2 / matrix row F-02 (P1). Source: ntfs-recover @ `5072bbc7` (0BSD).
> Policy: re-implemented from this documented behavior; no Rust code copied
> (standing policy is stricter than 0BSD requires). Extracted 2026-11-26
> with file:line citations against the pinned reference clone. Companion
> sheets: `worklog/NTFS-RECOVER-FEATURES.md` §5.

Battle-tested reference case: 2TB NTFS volume, sector=512, cluster=4096,
record=1024, MFT at cluster 4 (reference DEBUGGING.md:3-11).

## 1. Delta-grouping fragment reconstruction

Reference: `src/repairer.rs:179-290` (`reconstruct_fragments`).

Inputs: scanned MFT records (each: disk_offset, mft_record_number,
sequence_number, base_record), cluster_size, record_size, optional
max_record (from record 0's $DATA end_vcn, see §1.5).

1. **Dedup/filter**: skip extension records (`base_record != 0`); skip
   records with `mft_record_number > max_record` when set; deduplicate by
   record number keeping the copy with the highest sequence number.
2. **Delta grouping** (signed 64-bit): `delta = disk_offset −
   (mft_record_number × record_size)`. All records of one physical MFT
   extent share the same delta (disk offsets are linear in record number
   within an extent). Group by delta.
3. **Fragment creation** per delta group, spanning min..max record number,
   with `rpc = cluster_size / record_size`:
   - `logical_start = first_mft − (first_mft mod rpc)` (floor-align),
   - `logical_end_excl = ((last_mft / rpc) + 1) × rpc` (ceil-align),
   - `cluster_count = (logical_end_excl − logical_start) / rpc`,
   - `disk_offset_start = delta + logical_start × record_size`; drop the
     fragment if negative,
   - `cluster_start = disk_offset_start / cluster_size`; require
     `disk_offset_start mod cluster_size == 0`, else warn and drop.
   Gaps inside a group (missing/overwritten records) do NOT split the
   fragment — only min/max matter.
4. **Drop tiny fragments**: discard `cluster_count < 4` (stale records
   from a previous MFT layout; 16 records at default geometry).
5. **Cap to record 0's VCN range** (`src/main.rs:288-349`): locate
   scanned record 0 (`record_number==0 && base_record==0`), parse its
   attributes, take the unnamed non-resident $DATA `end_vcn`, then
   `max_record = (end_vcn + 1) × rpc − 1`. Sort fragments by
   logical_start; `max_logical_end = max_record + 1`; a fragment whose end
   exceeds it is truncated (`cluster_count = ceil((max_logical_end −
   logical_start)/rpc)`; drop if 0). Without record 0 in the scan:
   reconstruct uncapped with a warning. The cap is ESSENTIAL when $MFT
   $DATA is split across record 0 + $ATTRIBUTE_LIST extension records
   (reference case: record 15 held VCN 1,154,036-1,408,847; record 0 held
   VCN 0-1,154,035 — DEBUGGING.md:27-47).
6. **Merge** (sorted by logical_start; `src/repairer.rs:330-369`):
   - contiguous logically AND physically (`frag.logical_start ==
     prev_end_logical && frag.cluster_start == prev_end_cluster`) →
     extend previous fragment's cluster_count;
   - overlap (`frag.logical_start < prev_end_logical`) →
     `overlap_clusters = (prev_end_logical − frag.logical_start)/rpc`;
     if the new fragment extends beyond, append a trimmed fragment at
     `prev_end_logical` with `cluster_start += overlap_clusters`,
     `cluster_count −= overlap_clusters`; fully contained → skip;
   - else gap → append as-is.

## 2. Data-run encoding (sign-extension-safe)

Reference: `src/parser/data_runs.rs:102-164`.

Header byte per run: high nibble = offset-field byte count, low nibble =
length-field byte count. Then length bytes (LE, unsigned), then offset
bytes (LE, two's complement). Offsets are RELATIVE to the previous run's
absolute cluster offset (first run relative to 0). Sparse runs: offset
nibble 0 (length only). `0x00` header terminates; the patch path also
zero-fills all remaining run space (redundant termination).

**Sign-extension rule (the critical bug the reference fixed):** ntfs-3g
decodes both fields by sign-extending the most-significant byte
(DEBUGGING.md:99-117). A length of 45,669 (0xB265) encoded in 2 bytes
decodes as −19,867 → `ntfs_mapping_pairs_decompress` fails EIO. Encoder
must guarantee the MSB's bit 7 is 0:

- Length (logically unsigned): `len_bytes = ceil((bit_length(value) + 1) / 8)`
  — the +1 spares the sign bit.
- Offset (signed): positive → `ceil((bit_length + 1)/8)`; negative →
  `ceil((significant_bits + 1)/8)` counting from the leading ones.

## 3. Record-0 in-place patch

Reference: `src/repairer.rs:467-588` (`patch_record_data_runs`).

1. Read `record_size` bytes at `mft_start_offset` (from boot sector).
   Bail unless magic is `FILE`.
2. **Un-fixup**: `usa_offset` = u16@0x04, `usa_size` = u16@0x06, `usa_sig`
   = u16 at `usa_offset`. For each of `num_sectors = record_size /
   sector_size` sectors: original word lives at `usa_offset + 2 + i*2`;
   the word at `sector_end = (i+1)*sector_size − 2` must equal `usa_sig`
   (mismatch = warn, continue); restore the original word into
   `sector_end`.
3. Walk attributes from u16@0x14: type u32@pos, length u32@pos+4; stop at
   type 0xFFFFFFFF or invalid length. Find: type 0x80, non-resident flag
   (byte@pos+0x08)==1, name length (byte@pos+0x09)==0 (the unnamed
   non-resident $DATA).
4. `runs_offset` = u16@(attr+0x20); `runs_start = attr_pos +
   runs_offset`; `runs_space = attr_length − runs_offset`.
5. **Refuse** if new encoded runs exceed `runs_space` — the record is
   never restructured. Attribute length and header fields (start_vcn,
   end_vcn, allocated/real/initialized sizes) are NOT modified; the §1.5
   cap keeps total clusters consistent with the preserved end_vcn.
6. Zero all `runs_space` bytes, then copy the new run bytes in.
7. **Re-fixup**: `new_sig = usa_sig + 1` (wrapping), mapping 0xFFFE →
   0x0001 so the signature is never 0x0000 or 0xFFFF. Write at
   `usa_offset`; for each sector, save the current `sector_end` word into
   USA entry i, then write `new_sig` at `sector_end`.
8. Caller writes the buffer back at `mft_start_offset`.

## 4. Safety protocol

Reference: `src/main.rs:269-377`, `src/repairer.rs:28-170`.

Order: load scan file (mandatory) → `--reconstruct` recomputes fragments
and prints an original-vs-reconstructed comparison → if not dry-run,
stderr warning + prompt `Type 'yes' to continue`; anything else aborts →
repair_mft. Inside repair_mft: bail on empty fragment map,
`mft_start_offset==0`, or unreadable boot sector; print full plan;
dry-run writes NOTHING (prints "would write", hex-dumps first 128 bytes);
real run backs up the original record 0 FIRST (default
`mft_record0_backup_<unix-secs>.bin`; raw record_size bytes), then
writes. `--fix-mirror` additionally backs up and writes the same patched
record at the $MFTMirr offset. Restore procedure documented via dd.

## 5. Documented failure: rebuilding record 0 from scratch

Constructing a fresh record 0 ($STANDARD_INFORMATION + $FILE_NAME +
$DATA) FAILED: the new record lacked $ATTRIBUTE_LIST (pointing to the
extension record's continuation runs) and $BITMAP; the driver saw only
4.6M of 5.6M records. Lesson: **never rebuild record 0 from scratch;
always patch in-place**, preserving every other attribute and all header
fields byte-for-byte (reference DEBUGGING.md:49-59,171).

## 6. Constants

- Min fragment: 4 clusters (= 16 records @ sector 512 / cluster 4096 /
  record 1024; `rpc = 4`).
- USA signature bump: +1 wrapping, 0xFFFE→0x0001, never 0x0000/0xFFFF.
- Run field widths: length 1-8 bytes, offset 0-8 bytes;
  `bytes = ceil((bits+1)/8)`.
- Scanner sanity cap: cluster_offset > 2^44 treated as corruption.
- Scale proof: 2TB, 5,635,394 records, cap at 4,616,143, final patch =
  53 runs / 1,154,036 clusters; repair + `ntfsfix` + read-only ntfs-3g
  mount succeeded.
