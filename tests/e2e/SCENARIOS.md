# SCENARIOS — the e2e matrix, row by row

Every row's ground truth: the seed manifest (`engine/bench/seed/generate_seed.ps1`
corpus v2.0.0) + the recorded deletion method. Expected recoverability cites the
repo's recorded measurements (engine/.github/scenarios/benchmark-matrix.json,
engine/bench/baseline-candidate.json, engine/bench/root-causes.json) — never
optimism. A gate never tests a documented engine gap as if it were a promise.

## The corpus (identical for every row)

- 23 real-magic formats: jpg, png, heic, tiff, cr2, nef, arw, dng, mp4, mov,
  avi, mkv, pdf, zip, 7z, rar5, sqlite, pst, txt, bin, docx/xlsx/pptx
  (PK-class) — real headers/footers over deterministic PRNG bodies
  (seed `0x5EEDC0DE`).
- Directory classes: `small/`, `tiny/`, `media/`, `big/`, `manysmall/`,
  `nested/` (5+ levels), `longpath/` (>260 chars), `unicode/` (CJK + special
  characters), `frag/` + `fraghigh/` (deliberately interleaved writes),
  `ads/`, `hardlinks/`, `sparse/`, plus the `keep/` control group that is
  never deleted.
- Size classes: <4 KB (`tiny`), 4 KB–1 MB, 1–50 MB, 50–500 MB, >500 MB
  (`big` class in the full profile).
- Attributes: read-only, hidden, system, NTFS ADS, NTFS hard links, NTFS
  sparse, fragmentation classes contiguous/fragmented/highly fragmented.
- Manifest per file: rel path, size, SHA-256, fixed timestamps (2026-01-15/
  16/17), attributes, extents (FSCTL_GET_RETRIEVAL_POINTERS), MFT record
  numbers, `keep` flag.

## The deletion modes (engine/bench/scenarios/apply-scenario.ps1)

| Mode | Method | Expected recoverability (recorded evidence) |
|---|---|---|
| `recycle_bin` | shell delete via VB `SendToRecycleBin` (read-only bit cleared + retried, recorded), bin NOT emptied | NTFS deepcarve hash-recall **0.0006** on the 10,216-file bench corpus (run 42019994804) — the `$R`-name/pairing semantics are the quality workstream's gap |
| `permanent_delete` | `Remove-Item -Force` (no bin) | NTFS deepcarve hash-recall **0.428** (run 42019994804; floor 0.35); exFAT **0.0** (quality gap); FAT32 0–0.3% (GAP-S6-1/S6-2) |
| `unlink_native` | native `DeleteFileW` P/Invoke | same on-disk end state as permanent_delete; parity is the recorded expectation |
| `folder_delete` | whole seed tree shell-deleted to the bin, then `Clear-RecycleBin -Force` | bin-emptied NTFS deepcarve hash-recall **0.0739** (run 42019994804) — the `$I`/`$R` metadata is itself deleted; payload recovery is carve-dependent |
| `quick_format` | `Format-Volume` (metadata reinit, no zero-fill) | KNOWN ENGINE GAP GAP-S6-4 (name-only remnant scan, 0% vs og 51.5%) — recorded ungated; the row still exercises the full format-recovery path |
| `full_format` | `Format-Volume -Full` (zero-fill) | recall 0 is physics, not a defect; the row proves honest survival (exit 0, empty tree) |
| `delete_partition` | `Remove-Partition` (GPT/MBR entry removed, data sectors untouched; diskpart `delete partition override` fallback) | GATE = lost partition DISCOVERED (≥1) + recover completes; file recall discovery-limited (43.0% measured, run 40073093997-smoke); the lost-partition APPLY probe follows |
| `decay_50/80/100` | permanent delete → fill volume to N% with recorded PRNG fillers → delete fillers | decay_50 NTFS measured **96.4%** (floor 0.85); the decay_80 cliff is an allocation-hint sector-reuse effect (GAP-S6-5) — judged against apply-scenario's physical-ceiling probe, ungated |
| `partial_overwrite` | permanent delete → 0xAA over the first 8 sectors (4 KiB) of each deleted file's first extent via locked-volume raw IO | the overwritten files CANNOT hash-match by construction; the recorded recall covers the remainder — honesty about physics |
| `mft_corrupt` | permanent delete → zero the deleted files' MFT records (+adjacent) → XOR 0xFF the first 4 KiB of `$Bitmap` (NTFS only) | structured NTFS recovery destroyed by construction; the carve-only salvage path is what this row exercises |

## The full matrix (45 rows)

Per filesystem the 11 non-NTFS-only modes; `mft_corrupt` is NTFS-only
(apply-scenario refuses non-NTFS with a recorded NOT-COMPARABLE, so the row
does not exist for other filesystems). NTFS rows: recycle, permdel, unlink,
folder, qkfmt, fullfmt, partdel (discovery), decay50 (floor 0.85), decay80,
decay100, partovr, mftcorrupt — 12 rows. exFAT / FAT32 / ReFS: the same 11
rows minus mftcorrupt. FAT32's disk is MBR (the partition-delete variant);
the others GPT. ReFS rows run on their own shards ("where the runner supports
it": a runner that cannot format ReFS fails the shard with a diagnostic).

Scan modes: `all` = quick + deep + deepcarve per lane (the partition-delete
rows run deepcarve only — the lost-partition surface). Every row runs BOTH
lanes (CLI + API) with the divergence check.

## The smoke matrix (4 rows, PR-triggered, CLI only)

`SM-NTFS-PERMDEL` (quick), `SM-EXFAT-RECYCLE` (quick), `SM-FAT32-QKFMT`
(deepcarve), `SM-PART-DELETE` (deepcarve + the discovery gate on a disposable
GPT disk). Small dedicated disks (~1–1.5 GiB each); the API-lane skip is the
recorded design reason ("smoke profile is CLI-only by design") — recorded in
every verdict.json + divergence.json.

## Execution-completeness gates (every row, hard)

- CLI: every scan mode exits 0 and produces its session; recover exits 0 or 2
  (2 = honest partial); the transport assertion (direct raw I/O) holds.
- API: harness exit 0/2; `api-result.json` reports `transport: direct`.
- Inventory + metrics computed for both lanes; divergence check green.
- Floors: only where recorded evidence supports them (NTFS permdel 0.35,
  NTFS decay50 0.85); everything else observe-only with the citation.
- Discovery rows: lost partitions ≥ 1 + the APPLY probe (dry-run exit 0; real
  apply recorded with partition-before/after).
