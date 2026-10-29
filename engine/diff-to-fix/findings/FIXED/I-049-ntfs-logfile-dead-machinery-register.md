# INFORMATIONAL I-049 — the og's $LogFile transaction-journal machinery is dead weight in the scan path: the walk builds the chkdntfs recovery tables, frees the log, and is never consulted again — the core's zero counterpart is the correct deliberate non-adoption

**Severity:** INFORMATIONAL (the dead-machinery register — the I-040/I-041
precedent; resolved from tracked-open OQ-S79-1, this session)
**Module / Surface:** ntfs (the volume-open path's $LogFile load + RCDR walk)
**Parameters affected:** B2-adjacent (no result contribution exists to port), S35-adjacent (no error-recovery ladder rung exists), Q68 (this register is the doc truth)
**Corpus references:** `NTFSSearchDLL_decompiled.c` — the two open sites (:753-756 the first, :31653-31655 the second); the loader FUN_18001cf20 (:22856+); the walk driver FUN_18001d460 (:23100-23160); the record walker FUN_180029dd0 (:33577+); the per-record handler FUN_18002a0f0 (:33741+); the volume clone (:40403-40405). All read first-hand this session (objdump cross-checks on NTFSSearchDLL.dll where Ghidra's typing was lossy).
**Core references:** the deliberate absence — grep "LogFile" across src/ + include/: zero hits (the OQ's verification, re-confirmed at this session's HEAD). The core's NTFS results come from the MFT walk (S-012's two-pass architecture) exactly as the og's do.
**Status:** ACKNOWLEDGED — no code change; the non-adoption is deliberate and correct

## Summary

The og loads and walks the $LogFile at BOTH NTFS volume-open sites. The
full follow-the-data decode (OQ-S79-1's probe list, executed this
session) proves the machinery is DEAD in the scan path: the walk
driver's only outputs are two frame-local out-params it never reads, the
staged log buffer is freed at the walk's end, the success flag is
written and cloned but never read (one write, zero readers, corpus-wide),
and the per-record handler's tables — the transaction table, the
0x16/0x15-pair table, the page table, the two client-id maps — persist
in the state object with no post-open querier (the only lookup sites
are inside the walker itself). No result-sink call exists anywhere in
the chain: no CNTFSFileProperties creation, no tree add, no node emit.

## What the reference does (the decode, for the record)

The inherited mount-path code: the loader (FUN_18001cf20) reads the
$LogFile's $DATA, computes and validates the restart pages (the
0x200/0x800 sector-size duality, the second-restart flag), and stages
the log buffer with the ≤128 MB bound family. The walk driver
(FUN_18001d460) gates on the loaded state, checks the RCDR signature at
+0x2000, extracts the restart-specified walk start, and invokes the
record walker three ways (forward from the restart position, the flag
form, the 0x4000 fallback). The walker assembles page fragments; the
per-record handler builds the chkdntfs recovery tables (the transaction
table at state+0x14, the 0x16/0x15 prepare/commit pair table at
state+0x2c, the page table at state+0x1c with the
`(sector_size²>>10)·*(int*)(rec+0x48) + *(u16*)(rec+0x44)>>1`
page-number computation, the client-id maps at state+0x20/+0x24) —
then resets the accumulation buffer. The driver frees the staged
buffer, stores the success flag at ctx+0x28, and returns. The volume
clone copies the flag; nothing reads it. The scan proceeds to the MFT
walk; the journal is never consulted again.

## What our core does

Nothing on this surface — by decision. The core's NTFS results come
from the MFT walk (S-012's two-pass architecture + the rescue chain),
the same source the og's results actually come from. Porting the
$LogFile table machinery would be the invented-path anti-pattern the
mandate forbids ("removing an invented path the reference does not have
is required"; conversely, no path the reference does not effectively
have shall be added).

## Why this is a register, not a fix

The OQ's three candidates are now discriminated: candidate 1
(recently-deleted records from the journal) — REFUTED (no emission
site); candidate 3 ($MFT reconstruction from replay) — REFUTED (no
replay consumer; the tables are built and abandoned); candidate 2
(volume-state-only) — CONFIRMED, and stronger: the state itself is dead
(one write, zero reads). A fix would add dead code; the register
documents the evidence so no future session re-derives the question.

## Reproduction / verification

Every claim above is a first-hand corpus read this session (the
function sites cited; the zero-readers greps corpus-wide on
NTFSSearchDLL_decompiled.c). The core absence re-verified at this
session's HEAD (grep "LogFile" zero).

## Related findings

- OQ-S79-1 (RESOLVED — this register is its resolution; the file retired
  to findings/FIXED/ with the full probe record).
- N-039 (FIXED — the USN $J tail-window harvest; the sibling journal
  surface that WAS result-bearing and IS ported).
- S-012 (FIXED — the two-pass architecture; the actual result source).
- I-040 / I-041 (the dead-machinery register family precedent).
