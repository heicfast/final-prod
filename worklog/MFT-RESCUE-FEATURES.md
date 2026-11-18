# MFT-RESCUE — Feature Context Sheet
> Agent G (Task 2-G), Phase-0. READ-ONLY extraction at HEAD 9b0e5ecc9d24ae9918f83e3a81e4a8616f610a22, MIT. Date 2026-11-19. Every claim cites file:line.
> Repo is 8 tracked files / ~1,154 Python LOC. Many mission focus areas ($LogFile, $UsnJrnl, slack carving, deleted/live categorization) are **NOT FOUND IN CODE** — this sheet documents what actually exists and marks the gaps precisely.

## 0. Identity (URL, SHA, layout, LOC, license)

- URL: `github.com/hasithahaththella/mft-rescue` (README:45 shows a placeholder `<you>` clone URL; repo dir name + session metadata are the identity source).
- SHA: `9b0e5ecc9d24ae9918f83e3a81e4a8616f610a22`, branch `main` (verified by reading `.git/HEAD` → `ref: refs/heads/main` and `.git/refs/heads/main` → the SHA; no git commands run).
- Layout (all 8 tracked files):
  ```
  LICENSE                    21 lines  (MIT, Copyright (c) 2026 Hasitha Haththella)
  README.md                 175 lines
  .gitignore                 32 lines  (ignores *.img, *.progress.json, *.log, recovered/)
  recovery/__init__.py        2 lines  (docstring + __version__ = "0.1.0")
  recovery/image_disk.py    307 lines  (Windows raw-device opener + imager)
  recovery/ntfs_recover.py  655 lines  (MFT parser + recovery driver)
  tests/__init__.py           0 lines  (empty)
  tests/test_ntfs.py        190 lines  (11 unittest methods, all synthetic)
  ```
- Python LOC total: 1,154 (incl. tests). Stdlib only: `argparse, logging, os, re, struct, sys, time, ctypes, json, dataclasses, pathlib, unittest` — zero third-party deps (README:33 claim is accurate).
- Target platform: Windows raw-device paths via ctypes/kernel32 (`image_disk.py:52-96`), but file-mode (`.img`) paths work anywhere (`image_disk.py:111-115`); raw paths raise `RuntimeError` off-Windows (`image_disk.py:95-96`).
- No CI, no setup.py/pyproject, no requirements file, no fixtures directory.

## 1. Architecture (module map, data structures, error model)

Two modules, one dependency edge:

- `recovery.image_disk` — standalone imager; also exports `RawWinDevice`, the read-only source handle reused by the parser (`ntfs_recover.py:38`: `from .image_disk import RawWinDevice  # reuse the read-only Windows opener`).
- `recovery.ntfs_recover` — MFT scanner/parser + 3-pass recovery driver; imports only `RawWinDevice` from the sibling.

`RawWinDevice` (`image_disk.py:88-157`) — the single I/O abstraction:
- Raw path (`\\.\…`): `CreateFileW(GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN)` (`image_disk.py:97-105`); size via `IOCTL_DISK_GET_LENGTH_INFO` (`image_disk.py:117-127`); `seek`→`SetFilePointerEx` (`:133-140`), `read`→`ReadFile` (`:142-151`), `close`→`CloseHandle` (`:153-157`).
- File path: plain `open(path, "rb")` + seek-to-end for size (`image_disk.py:111-115`).
- **No write method exists on the class** — read-only by construction.

Data structures (`ntfs_recover.py:123-159`, all dataclasses):
- `FileNameAttr(parent_ref, name, namespace, real_size, alloc_size, created, modified, accessed)` — one per $FILE_NAME attribute.
- `DataAttr(name, resident, resident_bytes, real_size, alloc_size, init_size, runs, compressed)` — one per $DATA; `runs` is `[(length_in_clusters, lcn_or_None)]`, `None` = sparse.
- `MftRecord(rec_num, in_use, is_dir, seq, base_ref, names[], data_streams[], si_modified, si_created, si_accessed, raw_offset)` — `base_ref` masked to 48-bit record number (`ntfs_recover.py:192`); sequence number of the ref is discarded.
- `Node(rec_num, name, parent, is_dir, record)` (`ntfs_recover.py:297-303`) — **dead**: defined, never instantiated.

Core functions: `_sanitize_name` (:61), `_apply_fixup` (:66), `_decode_runlist` (:91), `_parse_attr_header` (:162 — **dead code, never called**; `parse_record` inlines its own header logic at :198-216), `parse_record` (:180), `_pick_name` (:306), `_build_paths` (:320), `_ext_of` (:354), `_ft_to_unix` (:361), `_read_runs` (:368), `_detect_record_size` (:392), `scan_mft` (:420), `_matches_filters` (:484), `recover` (:496), `main` (:626). Imager: `_same_volume` (`image_disk.py:162`), `_refuse_unsafe_output` (:169), `_load_progress`/`_save_progress` (:183/:192), `image` (:198), `main` (:293).

Error model: per-record/per-file `try/except` with skip-and-log (attribute parse failures silently swallowed `ntfs_recover.py:287-288`; file recovery failures counted `:610-612`); scan-level read errors skip 1 MiB and continue (`:436-439`); imager read errors trigger sector-granularity salvage (`image_disk.py:248-269`); fatal misconfigurations raise `SystemExit` (same-volume refusal). Return code is always `0` from `recover`/`image` — failures are logged, never reflected in exit status (`ntfs_recover.py:623`, `image_disk.py:287`).

## 2. Deleted-record detection

**Status: IMPLEMENTED (flag parse only) — no dedicated deleted-file workflow.**

- Flags read from record offset `0x16`: `FLAG_RECORD_IN_USE = 0x0001`, `FLAG_RECORD_IS_DIRECTORY = 0x0002` (`ntfs_recover.py:50-51`); classification is exactly `in_use = bool(flags & FLAG_RECORD_IN_USE)` (`:187`). A record with the bit clear is "deleted" in NTFS semantics; the code stores this in `MftRecord.in_use` (`:194`).
- No heuristics beyond the flag: no sequence-number staleness analysis, no $Bitmap cross-check, no "zeroed-record" checks. The 16-bit sequence at `0x10` (`:191`) and parent-ref sequences are parsed but never validated.
- Deleted records are **not filtered out** — they flow through the same recovery pipeline as live ones. The only behavioral use of `in_use` is dedup preference during the scan: a record slot is replaced only if the new hit is in-use (`scan_mft`, `ntfs_recover.py:454`: `if rec.rec_num not in records or rec.in_use:`), so a live record wins over a stale deleted one occupying the same record number.
- There is no CLI switch to select live-only or deleted-only output; `--ext/--name/--max-size` filters apply to both classes identically (`:561-574`).
- Re-implementer note: "recovering deleted files" here is a *side effect* of keeping every parseable aligned FILE record, not a feature path.

## 3. $LogFile analysis/replay

**Status: NOT FOUND IN CODE.** No occurrence of `$LogFile`, `LoggedUtilityStream`, restart-page, or transaction/undo parsing anywhere in the 8 files (grep over the whole tree confirms; the only "USN" hits are the Update Sequence Number in fixup code, e.g. `ntfs_recover.py:76`, and test comments `tests/test_ntfs.py:54,79`). The README does not claim it either — the mission item simply has no counterpart in this repo. No replay semantics, no analyze-only mode, nothing to harvest here.

## 4. $UsnJrnl analysis

**Status: NOT FOUND IN CODE.** No `$UsnJrnl`/`$Extend` parsing, no USN record (`USN_RECORD_V2`) structures, no reason-code or file-activity history derivation (grep confirms zero relevant hits; README makes no such claim). Nothing to harvest.

## 5. MFT slack-space carving

**Status: NOT FOUND IN CODE (aligned scan only).**

- `scan_mft` walks the source in 16 MiB buffers and tests **only `record_size`-aligned offsets** for the `FILE` signature: `for local in range(0, len(buf) - record_size + 1, record_size)` (`ntfs_recover.py:445`). Unaligned "FILE" hits (records embedded in slack, in $DATA content, or after a record-size change) are never examined.
- `_detect_record_size` (`:392-417`) does do an unaligned signature search, but only to *measure* the dominant 1024/2048/4096 spacing over the first 64 MiB (heuristic: hit must have bytes-allocated ∈ {1024,2048,4096} at `+0x1C`, ≥4 hits, mode of valid diffs, fallback 1024 — `:406-417`).
- No gap analysis between end-of-used-attributes (`bytes used` at `0x18`) and record end, no carving of partial records, no validation of embedded records beyond the normal `parse_record` path.
- Corrupted-record rejection exists but is coarse: fixup USN mismatch (`:84-85`), bad header bounds (`:68-79`, `:181-184`), attribute length sanity (`:203-204`).
- Consequence: deleted records still resident in their aligned MFT slots are found; anything living in slack is invisible to this tool.

## 6. Parent-reference / path reconstruction

**Status: IMPLEMENTED — `_build_paths` (`ntfs_recover.py:320-351`) + `_pick_name` (:306-317).**

- Parent link: each $FILE_NAME's first 8 bytes hold the parent MFT reference; masked to the 48-bit record number (`ntfs_recover.py:229`). Name selection prefers namespaces in order `NS_WIN32_DOS(3) > NS_WIN32(1) > NS_POSIX(0) > NS_DOS(2)`, first-seen per namespace via `setdefault` (`:310-315`), falling back to `names[0]` (`:316-317`).
- Root: record number 5 hard-coded, resolves to `""` (`:330-332`).
- Recursive `resolve(rec_num, seen)`:
  - memoized via `paths` dict (`:325-327`);
  - **cycle safety**: `if rec_num in seen: return f"__orphans__/cycle_{rec_num}"` (`:327-328`);
  - missing parent record → `__orphans__/missing_{rec_num}` (`:334-335`);
  - record with no parseable $FILE_NAME → `__orphans__/noname_{rec_num}` (`:337-338`);
  - self-parent or parent 0 → `__orphans__/{name}` (`:341-342`);
  - else `parent_path/name` joined with `/` (`:344-345`).
- Names sanitized before joining: `INVALID_NAME_CHARS = re.compile(r'[<>:"/\\|?*\x00-\x1f]')` replaced with `_`, trailing `". "` stripped, empty→`_` (`:58-63`).
- Deleted parents: because deleted records are retained in the `records` map (§2), chains through deleted parents/directories resolve normally — only *missing* (unparseable/absent) parents orphan.
- Extension records (multi-record files): $DATA streams and $FILE_NAMEs of records with `base_ref != 0` are merged into their base record after the scan (`:470-477`); note the base lookup uses only the masked record number with **no sequence-number match**, so a stale extension could theoretically merge into a reused base slot.
- Orphan paths produced by early returns (cycle/missing/noname) are **not memoized** (`paths[rec_num] = p` only executes on the success path, `:346`), so they are recomputed per query — O(n·depth) worst case on orphan-heavy volumes. Deep trees also risk Python recursion limits (resolution is recursive, not iterative).
- Driver fallbacks: dry-run uses `paths.get(rn, f"__orphans__/{rn}")` (`:542`); recover pass uses `f"__orphans__/rec_{rn}"` (`:566`) — three different orphan naming schemes across the codebase, and none matches the README's claimed `__orphans__/<record_number>/<name>` (README:28; see §12).

## 7. Live/deleted/orphaned enumeration

**Status: PARTIAL — volume scan + path build exist; there is no categorical live/deleted/orphan report.**

- `scan_mft` returns `dict[rec_num -> MftRecord]` for base records only (`ntfs_recover.py:420-481`): linear aligned scan, 16 MiB buffers, per-5 s progress log (`:462-468`), read-error skip of 1 MiB (alignment preserved since 1 MiB is a multiple of all supported record sizes, `:436-439`), summary log "base records=%d, extensions=%d, raw FILE hits=%d" (`:479-480`).
- The only classification surfaced is the stats line after path building: count of directories vs files-with-$DATA (`:532-534`: "Indexed %d directories, %d files with $DATA"). `in_use` is never reported, aggregated, or used to bucket output.
- Orphaned entries are implicitly categorized by path prefix only (`__orphans__/...` per §6); there is no machine-readable output (no JSON/CSV), no per-record listing with flags, no "live vs deleted" column anywhere.
- Dry-run listing (`:536-552`) prints at most 50 matching `path (size)` lines and explicitly states totals are "not counted exhaustively" (`:551`).
- Re-implementer note: the enumeration substrate (records dict + paths dict + `raw_offset` per record, `:451`) is sufficient to build such a report, but this repo never does.

## 8. The ddrescue-style imager (as implemented)

**Status: IMPLEMENTED (single-pass + inline sector salvage + checkpoint/resume). "ddrescue-style" is loose: there is no second retry pass and no mapfile re-read of failed ranges.**

`image()` (`image_disk.py:198-290`):
- Chunked sequential copy: default `CHUNK = 1 << 20` (1 MiB), `--chunk` override (`:37`, `:299`). Source opened via `RawWinDevice` (read-only, §9); output pre-created at full size (seek-to-`total-1`, write one zero byte — "sparse if FS supports it", `:222-227`) then opened `r+b`.
- **Bad-sector isolation**: on a chunk read `OSError`, the chunk is retried **sector-by-sector** at `SECTOR = 512` granularity (`:248-269`): each readable 512-byte sector is salvaged and written; failing sectors are zero-filled and appended to `bad_ranges` as `[start, end)` byte offsets (`:258-261`); a "Salvaged X/Y bytes" warning is logged (`:269`). Short reads are zero-padded (`:243-244`, `:256-257`). This is the docstring's "similar to ddrescue's first pass" (`:11`).
- **Map persistence**: progress state at `<out>.progress.json` = `{"offset": int, "bad": [[s,e],...], "size": total}` (`:279`), written atomically (tmp file + `os.replace`, `:192-195`). The `bad` list is the de-facto bad-block map; the human-readable `.log` additionally records chunk-level error offsets (`:250`).
- **Resume**: `--resume` loads the JSON and continues from `state["offset"]`, preserving `bad_ranges` and recomputing `bad_bytes` (`:214-219`, `:236`). Resume re-opens the existing image `r+b` (`:221`). **Resume never revisits previously-failed ranges** — no ddrescue-style `-r3` retry phase; bad areas stay zero-filled forever.
- Checkpoint cadence: saved when ≥5 s elapsed or at completion (`:272`, `:279`) — **not** per chunk as README:153 claims (§12).
- Progress logging: percentage, GiB progress, MiB/s rate, cumulative bad bytes every 5 s (`:271-281`); final "DONE. %d bytes imaged. %d bad bytes zero-filled across %d ranges." (`:283-286`).
- Safety: `_refuse_unsafe_output` before opening source (`:200`, see §9); note `out.parent.mkdir` happens *before* the refusal check (`:199-200`).

## 9. Read-only posture + safety

**Status: IMPLEMENTED for the source; the posture is structural, not policy-configurable.**

- Source handle can only read: `RawWinDevice` exposes `size/seek/read/close` and opens raw devices with `GENERIC_READ` + `FILE_SHARE_READ|FILE_SHARE_WRITE` (so other tools aren't blocked) + `FILE_FLAG_SEQUENTIAL_SCAN`, never a write access mask (`image_disk.py:97-105`); file sources use `"rb"` (`:112`). No write API exists anywhere on the class (`:88-157`). `FILE_FLAG_NO_BUFFERING` is defined (`:46`) but **never used**.
- Same-volume refusal, two independent implementations:
  - imager `_refuse_unsafe_output` (`image_disk.py:169-180`): file sources compared via `Path.resolve().drive` (`_same_volume`, `:162-166`); raw `\\.\X:` sources compare the drive letter to the output's resolved drive. Raises `SystemExit` with "Refusing: …".
  - recovery `recover()` inline check, deliberately FIRST — before any mkdir/log I/O (`ntfs_recover.py:500-505`): only fires when `source.startswith("\\\\.\\")`; compares `source[4]` to `out.drive`.
- Holes/edges (all verifiable in code):
  - `recover()` performs **no** same-volume check for file (image) sources — only the imager does; recovering from `F:\img.img` to `F:\out` is not refused by `ntfs_recover`.
  - The `--log` path is never checked against the source volume in either module (defaults are safe: `<out>/recovery.log` `ntfs_recover.py:646`, `<out>.log` `image_disk.py:302`).
  - Drive-letter comparison only: no volume-serial / mount-point / junction resolution; `\\.\PhysicalDrive1` makes `source[4] == "P"` (`ntfs_recover.py:502`), which can false-refuse an output on `P:` (conservative direction, but a bug).
  - Imager runs `out.parent.mkdir` before refusing (`image_disk.py:199-200`).
- README safety prose (README:11, 22-24, 147-154) matches the constants and share flags; see §12 for the "verified by tests" overclaim.
- Resource nit: `recover()` dry-run returns at `:552` without `src.close()` (close only at `:622`).

## 10. CLI/UX, outputs

Two `python -m` entrypoints (argparse; `main(argv)` in both, `sys.exit(main())` under `__main__`):

- `recovery.image_disk` (`image_disk.py:293-303`): `--source` (required; `\\.\E:`, `\\.\PhysicalDrive1`, or `.img`), `--out` (required), `--log` (default `<out>.log`), `--resume` (flag), `--chunk` (default 1 MiB). Matches README table (README:110-118).
- `recovery.ntfs_recover` (`ntfs_recover.py:626-651`): `--source`, `--out`, `--log` (default `<out>/recovery.log`), `--cluster-size` (4096), `--record-size` (0 = auto-detect, fallback 1024), `--ext` (comma list, lowercased, dots stripped, `:641-643`), `--name` (regex compiled case-insensitive, `:523`), `--max-size` (default 2 GiB), `--dry-run`. Matches README:120-132.
- Logging: `logging.basicConfig(INFO, "%(asctime)s %(levelname)s %(message)s")` with a UTF-8 `FileHandler` plus `StreamHandler` in both drivers (`ntfs_recover.py:508-512`, `image_disk.py:202-206`). 5-second progress heartbeats in scan/recover/imager loops. Output files keep original paths (with `/`→`os.sep` translation, `ntfs_recover.py:576`) and get mtime/atime restored from $STANDARD_INFORMATION/$FILE_NAME FILETIMEs via `os.utime` (`:597-606`; FILETIME→Unix epoch offset 116444736000000000, `:361-365`).
- Data-extraction semantics worth copying: default (unnamed) $DATA stream preferred, else first stream (`:571`); resident content truncated to `real_size` (`:586`); non-resident read via `_read_runs` honoring `real_size`, sparse runs and read errors zero-filled (`:368-389`); compressed/sparse streams written raw and flagged only at DEBUG level (`:588-589` — README's "flagged in the log" is debug-level only, §12); over-`--max-size` files skipped and counted (`:572-574`).
- No JSON/CSV machine-readable report; no interactive UI; PowerShell-flavored docs (README:44-106).

## 11. Tests/fixtures

`tests/test_ntfs.py` — 11 methods, 5 classes, all in-memory synthetic (no disk access, `tests/test_ntfs.py:1`):

- `FixupTests` (:89-113): USA restore round-trip (tails at 510/1022 replaced from USA slots, :90-103); mismatched USN → `_apply_fixup` returns False (:105-113).
- `RunlistTests` (:116-133): single run len=8 lcn=0x100 (:117-121); relative negative delta (second run at lcn 8, :123-127); sparse run `off_size == 0` → `(5, None)` (:129-133).
- `SanitizeTests` (:136-142): `"a/b:c?.txt"` → `"a_b_c_.txt"`; `"..."` → `"_"`.
- `ParseRecordTests` (:144-165): full synthetic record round-trip via `_build_record` helper (:19-86, which builds header, USA at 0x30 with 2 sectors, one resident $FILE_NAME + one resident $DATA, ATTR_END terminator) asserting rec_num=42, in_use=True, is_dir=False, name, parent_ref=5, resident payload bytes; rejection of bad signature and short buffer (:161-165).
- `SafetyTests` (:168-186): `recover(source=r"\\.\E:", out=Path(r"E:\out"), …, dry_run=True)` must raise `SystemExit` — the only driver-level test.
- **Coverage gaps vs README claims**: no non-resident $DATA `parse_record` test (only the `_decode_runlist` unit tests), no `_build_paths`/orphan/cycle test, no `_detect_record_size` test, no `_read_runs` test, no imager (`image`) test, no resume/progress test. README:162-164 claims tests cover "non-resident `$DATA` with runlists" — the runlist *decoder* is tested, the non-resident *record parse* is not.
- Runner: `python -m unittest discover -s tests -v` (README:159); imports as package `recovery.*` (tests/test_ntfs.py:7), so run from repo root.

## 12. README-vs-code truth table

| README claim (line) | Code reality | Verdict |
|---|---|---|
| Read-only source, never written (README:11, 22-24) | `RawWinDevice` GENERIC_READ-only, no write API (`image_disk.py:97-105,88-157`) | TRUE |
| Output refused if on source volume (README:23-24, 151-152) | Imager: both file+raw sources (`image_disk.py:169-180`); recovery: raw sources only (`ntfs_recover.py:501-505`); `--log` unchecked; drive-letter comparison only | PARTIAL |
| USA fixup, resident + non-resident $DATA, runlists, parent-ref rebuild (README:25-26) | `_apply_fixup` :66-88, `parse_record` :180-292, `_decode_runlist` :91-118, `_build_paths` :320-351 | TRUE |
| Orphans placed under `__orphans__/<record_number>/<name>` (README:27-28) | Actual schemes: `__orphans__/{name}` (:342), `__orphans__/missing_{n}` (:335), `__orphans__/noname_{n}` (:338), `__orphans__/cycle_{n}` (:328), `__orphans__/rec_{n}` (:566), `__orphans__/{n}` (:542) — none match the README shape | FALSE (format mismatch) |
| Filtering by `--ext` / `--name` regex on basename (README:29-30) | `_matches_filters` :484-493, `--ext` :641-643, `--name` :523,635 | TRUE |
| Imager: 1 MiB chunks, bad regions zero-filled + offsets logged, JSON checkpoint, `--resume` (README:31-32) | `image_disk.py:37,248-269,279,214-219` | TRUE |
| Zero third-party deps, Python 3.10+ (README:33, 37) | stdlib-only imports throughout | TRUE |
| CLI tables incl. defaults (README:110-132) | `image_disk.py:293-303`, `ntfs_recover.py:626-651` — all defaults match | TRUE |
| Auto-detect cluster/record geometry, fallback 1024 (README:128, 137-139) | record size auto-detect :392-417; **cluster size is NOT auto-detected** — fixed default 4096 (:631), README:137-139 implies both auto-detect | PARTIAL |
| Compressed/encrypted/sparse written raw and "flagged in the log" (README:140-142) | written raw TRUE (:588-590); flagging is `log.debug` only (:589) — invisible at default INFO level | PARTIAL |
| ADS skipped, only unnamed $DATA (README:143) | `next((d for d in rec.data_streams if d.name == ""), …)` :571 (falls back to first stream if no unnamed — slight deviation) | MOSTLY TRUE |
| "Imager checkpoints progress every chunk" (README:153) | Checkpoint at ≥5 s intervals or completion (`image_disk.py:272,279`) | FALSE |
| "Recovery logs every write with the originating MFT record number" (README:153-154) | No per-file success log; periodic aggregate stats only (`ntfs_recover.py:614-621`); failures logged by path, not record number (:611) | FALSE |
| "Safety properties verified by tests" (README:147) | Only same-drive refusal tested (`tests/test_ntfs.py:168-186`); read-only handle and checkpointing untested | PARTIAL |
| Tests cover "non-resident $DATA with runlists" etc. (README:162-164) | Runlist decoder tested; non-resident record parsing, paths, imager not tested | PARTIAL |
| $LogFile / $UsnJrnl / slack carving | Not claimed, not implemented | N/A (absent) |

## 13. Coverage log (all 8 files — full read expected)

| File | Lines | Read status |
|---|---|---|
| `README.md` | 175 | Full read; used for §12 truth table |
| `LICENSE` | 21 | Full read; MIT, (c) 2026 Hasitha Haththella |
| `.gitignore` | 32 | Full read; ignores images/progress/log/recovered artifacts |
| `recovery/__init__.py` | 2 | Full read; version 0.1.0 |
| `recovery/image_disk.py` | 307 | Full read; all functions analyzed (§1, 8, 9, 10) |
| `recovery/ntfs_recover.py` | 655 | Full read; all functions analyzed (§1-7, 9-10) |
| `tests/__init__.py` | 0 | Empty (verified via read attempt) |
| `tests/test_ntfs.py` | 190 | Full read; 11 methods catalogued (§11) |

Additional verification reads (not tracked content): `.git/HEAD` (branch main), `.git/refs/heads/main` (SHA match). Greps confirmed absences: no `$LogFile`/`$UsnJrnl`/slack/two-pass strings; dead symbols `_parse_attr_header`, `Node`, `ATTR_INDEX_ROOT`, `ATTR_ATTRIBUTE_LIST`, `FILE_FLAG_NO_BUFFERING` each have exactly one defining occurrence and zero uses.

## 14. Harvest notes (strongest algorithms, weaknesses, licensing)

**Strongest behaviors worth re-implementing (behavioral spec, not code):**
1. USA fixup with strict USN verification as a corruption gate (`ntfs_recover.py:66-88`) — cheap, rejects torn records.
2. Aligned single-pass MFT scan with base/extension record split and post-scan merge of extension $DATA/$FILE_NAME into bases (`:420-481`).
3. Path reconstruction with cycle detection, four distinct orphan classes, and namespace-priority name selection (Win32+DOS > Win32 > POSIX > DOS) (`:306-351`).
4. Imager's inline sector-granularity salvage inside a failed 1 MiB chunk + atomic JSON checkpoint (`image_disk.py:192-195, 248-269, 279`) — good resume ergonomics even without a retry phase.
5. Runlist decoder with sparse-run support (`off_size == 0` → `(len, None)`) and signed relative LCN deltas (`ntfs_recover.py:91-118`); zero-fill semantics for sparse/errored runs in `_read_runs` (`:368-389`).
6. Read-only-by-construction device wrapper: no write verb exists at all (`image_disk.py:88-157`).

**Weaknesses / gaps (do not inherit):**
- No deleted/live categorization, no $LogFile/$UsnJrnl, no slack carving, no attribute-list ($ATTRIBUTE_LIST) handling — constant defined, never parsed, so highly fragmented files whose attributes live in other records via $ATTRIBUTE_LIST are missed (extension merge only follows `base_ref`).
- base_ref merge ignores sequence numbers (stale-extension hazard); fixup hardcodes 512-byte sectors (no 4Kn); cluster size never auto-detected despite README implication; orphan paths unmemoized (perf) and inconsistently named across five schemes; recursion depth unbounded.
- Safety checks: file-source recovery lacks same-volume refusal; `--log` unchecked; drive-letter-only comparison; imager mkdir-before-refusal; `\\.\PhysicalDriveN` misparsed as drive letter "P" in the recovery check (`ntfs_recover.py:501-505`).
- Exit codes always 0; per-file audit trail claimed but absent; dry-run leaks the source handle; compressed-stream flagging only at DEBUG.
- Test suite covers primitives well but no driver paths beyond the refusal test.

**Licensing:** MIT, Copyright (c) 2026 Hasitha Haththella (`LICENSE:1-3`). Harvest is behavioral (algorithms/semantics as described above); re-implementation must not copy expression. No credentials/tokens found in any file. No instructions-embedded-in-repo anomalies encountered; repo content treated strictly as data.
