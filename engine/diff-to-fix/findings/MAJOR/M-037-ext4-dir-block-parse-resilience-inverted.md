# MAJOR M-037 — ext4 directory-block parse resilience is inverted vs og (skip-vs-stop on both error classes)

**Severity:** MAJOR
**Module:** ext4
**Parameters affected:** A25 (engine algorithm steps — dir parse), B1 (detection under corruption — salvage surface), S35 (error recovery)
**Corpus references:** EXT4SearchDLL_decompiled.c :4146-4168 — invalid rec_len/type → **skip entry, continue** (:4146-4163); `inode == 0` → **stop parsing the block** (:4155); type must be 1..7 (:4164-4165); `rec_len >= (name_len+11)&~3` (:4166-4167); inode# > inodes_count → skip (:4168).
**Core references:** `src/core/ext4/cr_ext4_engine.cpp:391-394` (invalid rec_len → **break, drop rest of block**), `:396` (inode==0 → **continue past**), `:412` (majority-sane gate), no file_type gate; our bounds check is `name_len+8 <= rec_len` (differs from the og's rounding rule). Audited commit `7fd00e0`.
**Status:** READ-ONLY FINDING — no fix applied
> **Integration note (2026-10-26):** filed by the session-3 *storage/census*
> parallel leg against baseline `7fd00e0`; renumbered from its local `M-021` during the multi-leg reconciliation (IDs `S-016..S-022`,
> `M-024..M-030`, `N-033..N-044`, `I-013..I-016` were taken by the legs that
> landed on main first). Cross-references to parallel-leg findings were
> rewritten in the same pass.


## Summary

Both engines' damage-tolerance policies are inverted relative to each other on both error classes: a structurally invalid dirent makes the og skip-and-continue while we drop the block remainder; an inode-0 hole makes the og stop the block while we continue past it. On a dir block with one corrupted rec_len mid-block, the og recovers the later entries and we lose them; on a block with an inode-0 hole followed by live entries, we recover them and the og does not.

## What the reference does

Quoted rules — skip-on-structural-invalid, stop-on-inode-0, type gate 1..7, rec_len floor with 4-byte rounding including the 11-byte overhead.

## What our core does

Break-on-invalid (drops remainder), continue-on-inode-0, majority-sane gate, no type gate, simpler rec_len floor.

## Gap

Opposite salvage surfaces on damaged directory blocks (both directions). Falsifiable: a block with corrupted rec_len at entry 2 of 8 → og emits entries 3..8, we emit 1 only; a block with inode-0 at entry 2 → we emit 3..8, og stops.

## Impact

Different recovered file sets on damaged directories — the exact scenario the engine exists for. Each policy wins different cases; neither dominates.

## Fix plan

1. Split the policies: adopt skip-and-continue for structurally invalid dirents (rec_len/type/bounds failures) — recover more.
2. Keep our continue-past-inode-0 (recovers more; document as deliberate superset, I-014 (parallel leg register)-style) OR match the og's stop if parity benches demand it — recommendation: keep the superset, pin with a test.
3. Add the og's type gate (1..7) as a skip condition, and align the rec_len floor to the og's rounding rule.

## Tests to add

- `Ext4.CorruptRecLenSkipsEntryNotBlock` — mid-block corruption; entries after it recovered (fails today).
- `Ext4.InodeZeroHoleContinues` — pin the superset with a comment citing the og's stop.
- `Ext4.FileTypeGateSkipsInvalid` — type 0/8 entries skipped.

## Related findings

I-014 (parallel leg register) (E-notes: our other ext4 supersets), S-025/S-026/S-027 (same engine).

## Open questions

None — both policies are quoted.
