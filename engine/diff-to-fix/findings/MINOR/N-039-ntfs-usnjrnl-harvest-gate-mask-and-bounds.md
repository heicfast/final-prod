# MINOR N-039 — NTFS $UsnJrnl harvest: og reason-gate is `reason & 0x200` (compression-change bit, harvest-all flag dead); core gates on true FILE_DELETE 0x20000 + bounds/shape deltas

**Severity:** MINOR (grouped — behavioral, but a secondary name-evidence path where the core is the correct side; two-sided result-set effect documented below)
**Module:** ntfs
**Parameters affected:** B4 (deleted-file classification — USN evidence path), B1 (result-set membership), A25 (engine steps — $J chunk walk), C23 (constants — reason mask), Q65 (test debt — USN_HARVEST_CAP boundary, already tracked)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c` — gate `FUN_1800196d0 :20098` (`reason & 0x200`, harvest-all byte `+0x80` read there only); ctor `FUN_180018570 :19371` (harvest-all := 0); iter `FUN_180019570 :19993` (`:20023` rlen>0x200 break, `:20024` 4-byte zero skip, no version check); chunker `FUN_1800193c0 :19938-19941` (10 MiB chunks, no total cap); locator `FUN_1800190d0 :19868/19879-80` (`$UsnJrnl` + `$J` named stream); wiring `:757-761`, `:12588-12624`; consolidated quotes in `diff-to-fix/evidence/decompiled-quotes/s3-ntfs-usn-gate.md`
**Core references:** `include/cr/ntfs/cr_mftwalk.h:61-65` (`USN_REASON_FILE_DELETE = 0x00020000`), `src/core/ntfs/cr_mftwalk.cpp:24-25` (64 MiB cap, resync limit), `:975-1099` (harvest: V2+V3 validation `:1020-1038`, gate `:1048`, `$`-skip `:1054`, dedup `:1060-1081`, root_lost emit `:1091-1096`); fixture `tests/unit/test_ntfs_volume.cpp:30-33, 1074-1075`; prior registration `docs/PARITY-DELTAS.md` D-11 — audited commit `34b3441`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's $UsnJrnl harvest gate is `reason & 0x200` — per the public winioctl.h table that
is `USN_REASON_COMPRESSION_CHANGE`, not `USN_REASON_FILE_DELETE` (0x00020000); the
`||`-combined harvest-all byte (`+0x80`) is write-once-zero and has no setter anywhere in
the decompile, so the 0x200 mask is the entire effective gate. The core gates on the true
FILE_DELETE bit (0x20000). This completes the dead S3-NTFS agent's probe: its premise
("winioctl FILE_DELETE=0x200") was inverted — the core is the correct side; the probe's
executed output stands as core-behavior evidence with the interpretation corrected.
Secondary bounds/shape deltas: og caps in-chunk records at 512 bytes (names ≥227 chars
stall a chunk) and validates no record version, while the og walks the WHOLE journal
(the core mines only the first 64 MiB of the stream).

## What the reference does

Gate (FUN_1800196d0, USN_RECORD_V2 offsets: reason@+0x28):

```c
:20098  if ((*(char *)(param_1 + 0x80) != '\0') || ((*(uint *)(param_2 + 0x28) & 0x200) != 0)) {
             // harvest entry: name(ANSI), fileref, parentref, ts, attrs → vector
```

- The harvest-all byte: only reference in the entire decompile is this read; ctor writes
  0 (`:19371`); neither initializer (`FUN_180018740 :19422`, `FUN_180019a20 :20217`)
  writes it → permanently 0.
- Record iteration (FUN_180019570): `if (0x200 < *puVar1) break;` — RecordLength > 512
  abandons the rest of the 10 MiB chunk; zero words skipped 4 bytes at a time; **no
  MajorVersion/MinorVersion check** (V2 offsets applied to any data); tail partials
  carried in a 1 MiB buffer.
- Chunk walk (FUN_1800193c0): `0xa00000`-byte chunks over the full `$J` runlist —
  **no total cap**.
- Wiring: located via `$UsnJrnl` strcmp (`:19868`) + `L"$J"` named stream
  (`:19879-80`); harvested entries become CNTFSFileProperties (name from ANSI string,
  times, record#) inserted into a record-keyed collection (`:12588-12624`).
- Consumer shape notes: names converted UTF-16→ANSI (`FUN_180006610/6f0`); directory
  records additionally enter a second map (`:20104-20110`); no `$`-name filter observed
  in the harvest/consumer code read.

## What our core does

- Gate `cr_mftwalk.cpp:1048`: `reason & USN_REASON_FILE_DELETE` with
  `USN_REASON_FILE_DELETE = 0x00020000` (`cr_mftwalk.h:65`) — the true winioctl.h
  FILE_DELETE; DELETE-only policy documented in the header comment (`:61-64`).
- Validation: `rlen >= 64 && major in {2,3}` (+ minor==0 for V2) with V3's +0x10 shifted
  layout (`:1020-1038`); no 512-byte rlen cap (names up to 512 bytes = 256 chars cover
  NTFS's 255-char maximum); 8-byte resync bounded by `USN_RESYNC_LIMIT` (`:1026-1029`).
- Stream: materialized in VCN order **capped at 64 MiB** (`USN_HARVEST_CAP`, `:24`,
  `:980-981`) — on journals larger than 64 MiB only the first 64 MiB is mined.
- Emission: `$`-prefixed names skipped (`:1054`), UTF-16 preserved, dedup against
  structured records/(parent,name) pairs + in-journal dedup (`:1060-1081`), emitted
  under `root_lost` as MetadataOnly/Deleted (`:1091-1096`).

## Gap

Falsifiable claims (same `$J`, both engines run):
1. **Gate mask, two-sided.** A record with reason `0x80020000` (true Windows delete:
   FILE_DELETE|CLOSE) → og does NOT harvest (bit 0x200 clear); core harvests. A record
   with reason `0x80000200` (CLOSE|COMPRESSION_CHANGE — not a deletion) → og harvests;
   core does not.
2. **512-byte record cap.** A delete record for a ≥227-char name (V2 rlen > 512) in the
   middle of a chunk → og abandons the rest of that 10 MiB chunk; core harvests it.
3. **64 MiB stream cap.** A `$J` stream > 64 MiB with delete records beyond the first
   64 MiB → og harvests them; core does not (VCN-order cap; on a wrapped journal the
   tail holds the newest records).
4. **V3 records** (128-bit refs): og applies V2 offsets (reason read from the parent-ref
   high half) → gate effectively never matches correctly; core parses V3 natively.

## Impact

- **Recovery correctness:** the core's gate matches what Windows actually writes on
  deletion; the og's harvest is mis-gated (fires on compression-change records, misses
  true deletes) — i.e., the og's "USN journal cross-check" is near-inert for its
  documented purpose on real volumes. The core recovers journal-evidenced deleted names
  the og cannot. No correctness risk on the core side.
- **Result-set parity:** two-sided. Inventories diverge on journals containing
  compression-change records (og-only entries) and on >64 MiB journals (og-only entries
  beyond the core's cap). Og-parity benchmarks must exclude/annotate the USN evidence set.
- **User-observable:** core users see journal-evidenced names the og never surfaces;
  og users would see compression-changed files mislabeled as deleted-evidence names.
- **Do NOT "fix" parity by copying 0x200** — that would replicate an og defect.

## Reproduction / verification

- Decompiled evidence (strength 2): all quotes first-hand, file:line cited; the
  harvest-all dead-flag claim verified by exhaustive mask search (`+ 0x80) =` → 3 hits,
  all other classes; `char *)(… + 0x80)` → single hit, the gate read). Consolidated in
  `evidence/decompiled-quotes/s3-ntfs-usn-gate.md`.
- Executed evidence (strength 1, prior agent): `evidence/executions/s3-ntfs-usn-reason/`
  — core HEAD 34b3441 harvests the `0x20100` (contains 0x20000) record and rejects the
  `0x80000200` record. The probe's own interpretation is corrected by this finding: its
  "realdelete" record was CLOSE|COMPRESSION_CHANGE, NOT a Windows delete record.
- SDK semantics: winioctl.h values recalled from the public SDK (no local copy — flagged
  in the evidence file §6); the mechanical gate divergence does not depend on them.

## Fix plan (for the engineering team)

1. **Keep the 0x20000 gate** — register the og's 0x200 mask as an og defect in
   `docs/PARITY-DELTAS.md` (extend D-11: "og gate decoded this session = 0x200
   (compression-change); harvest-all flag dead code — our FILE_DELETE gate is deliberate
   correctness, not parity drift").
2. **Test-comment fix (cosmetic):** `tests/unit/test_ntfs_volume.cpp:30-33` — the
   comment claims `0x00000100` is "winioctl.h USN_REASON_FILE_CREATE"; the SDK value is
   `0x00010000` (0x100 is USN_REASON_HARD_LINK_CHANGE). Relabel the fixture constant
   (e.g. `kFixtureReasonBit = 0x100`) so no future agent re-derives the dead probe's
   premise.
3. **Journal cap direction (small code change, optional):** for `$J` streams > 64 MiB,
   mine the TAIL of the stream (newest records) instead of the head — the journal is
   circular and recent deletions are the recovery-relevant ones. Alternatively raise the
   cap and keep a bytes-probe budget.
4. **Contract:** note in the USN harvest doc block that og parity is intentionally NOT
   bitwise (gate, rlen cap, version validation, ANSI conversion all differ).

## Tests to add

- `NtfsVolume.UsnHarvestTrueDeleteReason0x80020000` — a `0x80020000`-reason record IS
  harvested (would fail if anyone copies the og's 0x200 mask). PASSES today.
- `NtfsVolume.UsnHarvestRejectsCompressionChangeOnly` — a `0x80000200`-reason record is
  NOT harvested. PASSES today.
- `NtfsVolume.UsnHarvestLongNameRecord` — delete record with a 240-char name (rlen
  ~540 > 512) is harvested (og would stall). PASSES today.
- `NtfsVolume.UsnHarvestCapMinesTail` (fails today — pins fix-plan item 3).

## Related findings

- S-012 (pass architecture — the USN harvest is a post-walk evidence phase in the core;
  the og harvests during the same engine run) · D-11 PARITY-DELTAS (existing ACCEPTED
  registration this finding sharpens with the first og-side gate decode) · Q65 test-debt
  note (USN_HARVEST_CAP boundary, module file) · dead-agent probe disposition recorded
  in `evidence/executions/s3-ntfs-usn-reason/` + evidence file §6.

## Open questions

- The og consumer at `:12588-12624` inserts harvested props into a record-keyed
  collection; whether a later stage filters `$`-names or re-parents by the dir map
  (`:20104-20110`) is not decoded (scan-side effect marginal; next artifact: the
  collection consumer after `FUN_180031070`).
