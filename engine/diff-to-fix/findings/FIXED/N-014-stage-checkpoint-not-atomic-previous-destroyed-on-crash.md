# MINOR N-014 — Stage checkpoint is NOT atomic; previous checkpoint destroyed on crash (truncate-then-write)

**Severity:** MINOR
**Module:** session, orchestrator, ui
**Parameters affected:** S34 (checkpoint — frequency, recovery behavior), D12 (session persistence format — atomicity), S36 (session lifecycle — save/load/resume semantics), F60 (bad-sector handling — crash recovery analog)
**Corpus references:** `re-workspace/artifacts/SaveScan/rsf-format--scan-session.md §5.5` (auto-saves use `auto_<volume>_<MM>_<DD>_<HH>_<MM>_<SS>.rsf` — a FRESH file per save with a timestamp name; older auto-saves preserved on disk); `:5.4.4` (og writer path: "seeks to EOF, writes magic+id+chain=0, remembers the header offset" — the og APPENDS records to EOF within a single save, but each save is a new file)
**Core references:** `src/ui/cr_ui.cpp:1131` (`s->ck_file = open_file(s->ck_path.c_str(), true)` where `open_file` at `:254-260` uses `_wfopen_s(&f, w.c_str(), L"wb")` — `"wb"` TRUNCATES the existing file to zero on open); `src/core/orchestrator/cr_scan.cpp:418-433` (`emit_checkpoint_()` does `seek(0)` then writes a fresh container header + records + finalize; previous checkpoint's bytes overwritten in place); `src/core/session/cr_crsf.cpp:172-200` (`Writer::finalize()` patches `record_count` and per-record `chain`/`crc` fields IN PLACE via seek+write; no `.tmp` + atomic rename) — audited commit `5d6da5f`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The core's stage checkpoint is written via `_wfopen_s(&f, w.c_str(), L"wb")`
which TRUNCATES the existing file to zero on open. If the process is
killed between `fopen("wb")` (truncate) and `finalize()` completing, the
checkpoint file is torn: the container header claims N records but the
chain fields are still zero. The previous good checkpoint is GONE
(truncated). The torn-tail salvage in `Reader::open()` recovers the
indexed prefix, but the user loses the unfinalized stage's progress
AND the previous auto-save's history. The og's pattern writes each
auto-save to a FRESH timestamped file, so a torn new file leaves the
previous auto-save intact.

## What the reference does

```text
// SaveScan/rsf-format--scan-session.md §5.5 (the og's pattern)
auto_<volume>_<MM>_<DD>_<HH>_<MM>_<SS>.rsf

A FRESH file per save with a timestamp name. Older auto-saves are
preserved on disk.

// §5.4.4 writer path:
"seeks to EOF, writes magic+id+chain=0, remembers the header offset"

The og APPENDS records to EOF within a single save, but each save
is a new file. Crash window: if killed mid-save, the new file is torn
but the PREVIOUS auto-save (different timestamp) is intact.
```

The og's pattern is crash-safe by accident: each save goes to a new
file, so a torn save doesn't destroy the previous good state.

## What our core does

```cpp
// src/ui/cr_ui.cpp:1131 (audited 5d6da5f)
s->ck_file = open_file(s->ck_path.c_str(), true);

// src/ui/cr_ui.cpp:254-260 (open_file implementation)
FILE* open_file(const std::string& path, bool writable) {
    FILE* f = nullptr;
    std::wstring w = utf8_to_utf16(path);
    if (writable) {
        _wfopen_s(&f, w.c_str(), L"wb");    // <-- TRUNCATES on open
    } else {
        _wfopen_s(&f, w.c_str(), L"rb");
    }
    return f;
}

// src/core/orchestrator/cr_scan.cpp:418-433 (emit_checkpoint_)
void emit_checkpoint_() {
    ck_file_->seek(0);
    cr::session::Writer w(*ck_file_);
    w.begin_container("scan_state");
    // ... write stage bits, counters, error map ...
    w.finalize();    // patches record_count, chain, crc IN PLACE
}

// src/core/session/cr_crsf.cpp:172-200 (Writer::finalize)
void Writer::finalize() {
    // Patch record_count at offset 0x10
    out_.seek(record_count_offset_);
    out_.write(&record_count_, 4);
    // Patch per-record chain + crc fields
    for (const auto& rec : records_) {
        out_.seek(rec.chain_offset_);
        out_.write(&rec.chain, 4);
        out_.seek(rec.crc_offset_);
        out_.write(&rec.crc, 4);
    }
    // NO fsync + atomic rename. The file is patched in place.
}
```

The checkpoint file is opened with `"wb"` (truncate), written in place,
and finalized via seek+write patches. No `.tmp` + atomic rename.

## Gap

The og's auto-save pattern preserves history (each save is a new
timestamped file; torn saves don't destroy the previous good state).
The core's checkpoint pattern overwrites the same file in place; torn
saves destroy the previous good state.

## Impact

- **Crash recovery:** if the process is killed during a checkpoint
  emission, the user loses the unfinalized stage's progress (the
  torn-tail salvage recovers the indexed prefix) AND the previous
  checkpoint's history (truncated). With the og's pattern, the user
  would have the previous auto-save (different timestamp) intact AND
  the new torn file (salvageable through the indexed prefix).
- **Functionally equivalent end-state for the user** (both sides
  recover the indexed prefix); the difference is the og preserves
  history and the core doesn't.
- **Parity benchmark:** the og's history preservation is a SUPERSET
  behavior; the core's overwrite-in-place is a parity gap.

## Reproduction / verification

- **Core evidence:** first-hand read of `cr_ui.cpp:1131`, `cr_ui.cpp:254-260`,
  `cr_scan.cpp:418-433`, `cr_crsf.cpp:172-200`. The `"wb"` truncate + in-place
  patch is explicit in the source.
- **Mitigation evidence:** first-hand read of `cr_crsf.cpp:240-276`
  `Reader::open()` torn-tail salvage (magic mismatch → `pos += CRSF_UNIT`
  skip-forward; `chain==0 && crc==0` unfinalized tail → stop indexing;
  count clamp to indexed size). BENCH-F1 verified.
- **Corpus evidence:** first-hand read of `SaveScan/rsf-format--scan-session.md
  §5.4.4, §5.5`.
- **Executed comparison:** not run — would require crashing the
  process mid-checkpoint and verifying recovery on both sides.

## Fix plan (for the engineering team)

1. **Architectural change:** write each checkpoint to a `.tmp` file
   (same directory), then atomically rename to the final name.
   Windows `MoveFileExW(MOVEFILE_REPLACE_EXISTING)` is atomic on the
   same volume.
2. **Files to modify:**
   - `src/core/session/cr_crsf.cpp:172-200` — `Writer::finalize()`:
     after the in-place patches, fsync the underlying file, then
     rename `<path>.tmp` → `<path>` (the temp file is what's been
     written; the rename makes it atomic).
   - `src/ui/cr_ui.cpp:1131` — `open_file()` for the checkpoint path:
     open `<path>.tmp` for writing (not `<path>` directly). The
     rename happens in `finalize()`.
   - `src/core/orchestrator/cr_scan.cpp:418-433` — `emit_checkpoint_()`:
     after `w.finalize()`, the rename has happened; the next reader
     opens `<path>` and sees the new content atomically.
3. **Algorithm/constant:** `MoveFileExW(MOVEFILE_REPLACE_EXISTING)` is
   the Win32 atomic rename primitive. On POSIX, `rename()` is atomic
   on the same filesystem.
4. **Tests to add:**
   - `Session.CheckpointAtomicityUnderCrash` — fixture: write a
     checkpoint, simulate a crash mid-finalize (close the file
     without writing the chain/crc patches), assert the previous
     good checkpoint is still readable. FAILS today.
   - `Session.CheckpointTmpFileCleanup` — assert the `.tmp` file is
     cleaned up after a successful rename.
   - `Session.CheckpointReaderHandlesAtomicRename` — assert the reader
     sees the new content immediately after the rename.
5. **Acceptance criteria:** the three tests above pass; the previous
   checkpoint is preserved on crash.

## Tests to add

- `Session.CheckpointAtomicityUnderCrash` (FAILS today)
- `Session.CheckpointTmpFileCleanup`
- `Session.CheckpointReaderHandlesAtomicRename`

## Related findings

- **S2-C-3 (this finding)** is the only session-atomicity finding. The
  torn-tail salvage (`cr_crsf.cpp:240-276`) already mitigates the
  worst case (the indexed prefix is recovered); the atomic-rename
  fix preserves the previous good state on top.

## Open questions

- Should the core also adopt the og's timestamped-auto-save pattern
  (preserve history across multiple checkpoints)? This is a UX
  decision (more disk usage, more recovery options) — out of scope for
  this finding. The atomic-rename fix is the minimum to match the
  og's crash-safety guarantee.

## Resolution

**Status:** FIXED
**Fixed in commit:** this session's N-014 atomic-checkpoint batch
**Verified by:** `CrUi.CheckpointPreviousGoodSurvivesTornEmit` (fail-before EXECUTED on the stash-isolated pre-fix tree with the new-field test isolated out: scan-2's "wb" open left the final path EMPTY — `{}` vs the good CRSF bytes, exactly the finding's destroyed-previous-checkpoint; pass-after byte-identical preservation) + `CrUi.CheckpointAtomicStagingAndCommit` (the committed final loads as a session container; no `.tmp` survives the scan) + `Checkpoint.CommitHookFiresAfterEveryFinalizeAndAbortsOnError` (commit fires exactly once per emitted checkpoint; a failing commit aborts the run with its own code and the checkpoint does not count as emitted) + full suite 704/704.
**Resolution notes:** The CheckpointSink gained a `commit` hook (nullable — in-memory sinks unchanged) that `emit_checkpoint_()` invokes after every successful `finalize()`. The cr_ui file adapter stages into `<path>.tmp` and commits via fflush + fclose + rename(tmp → final) + a fresh staging reopen (the rename runs on a CLOSED handle on Windows — MoveFileExW's FILE_SHARE_DELETE constraint avoided by construction); scan teardown and scan_close remove the transient `.tmp` (a torn never-committed emit is never trusted). Crash windows: before the first commit the FINAL holds the previous good checkpoint (pre-fix: truncated to zero at scan start); between commits the FINAL holds the last committed one; the rename itself is atomic per-platform. The og's crash-safety (rsf-format §5.5's fresh-file-per-save) is matched without adopting the timestamped-history pattern (the finding's own open question — a UX decision recorded as out of scope). The sink-triple contract comment in cr_scan.h updated to describe the staging + commit model; the torn-tail salvage (BENCH-F1) is unchanged and remains the reader-side backstop.
