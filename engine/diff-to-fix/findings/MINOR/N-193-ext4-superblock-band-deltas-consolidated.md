# MINOR N-193 — ext4 superblock validator acceptance-band deltas (consolidated): inode_size==0, log_bs 7..20, journal_inum gating, desc_size combos — four band differences on garbage SBs, both directions, unregistered

**Severity:** MINOR
**Module:** ext4 (superblock validation)
**Parameters affected:** B3 (garbage-SB acceptance band), C20 (struct decode bands), Q68 (registration completeness)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/EXT4SearchDLL_decompiled.c` — the validator region :3232-3239 (log_bs sign-check form; inode_size gate `(rev==0) || (short)inode_size != 0`), :3365-3367 (journal_inum copied only when feature_compat HAS_JOURNAL bit 2), :3397-3405 (desc_size clamped to [32,64] unconditionally)
**Core references:** audited `34d6cf10` — `src/core/ext4/cr_ext4_engine.cpp:148-149` (`log_bs > 6` reject), `:172-174` (inode_size==0 → 128 fallback accept), `:177-179` (journal_inum read unconditionally, 0 → 8), `:191-199` (desc_size 64 only when 64BIT else 32)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Four superblock acceptance-band differences between the og validator and the core's, all on garbage/corrupt SBs (near-zero real-volume impact), consolidated so each deliberate choice can be registered rather than stay accidental: (1) inode_size==0 with rev≥1 — og rejects, core falls back to 128 and accepts; (2) log_block_size 7..20 — og accepts (128 KiB..1 GiB blocks pass its int-sign form), core rejects above 6; (3) journal_inum — og reads it only when HAS_JOURNAL is set, core reads unconditionally and defaults to 8 (probing inode 8 on non-journal volumes); (4) desc_size — og clamps to [32,64] unconditionally, core picks 64-iff-64BIT (differs only on invalid combos).

## What the reference does

The four og forms with their sites (above; each located in the validator/mount regions this session — the agent's coverage log carries the line ranges; the log_bs sign-check and the journal_inum gate were re-read directly).

## What our core does

The four core forms quoted above at `34d6cf10`.

## Gap

Per band, falsifiable on synthetic SBs: e.g. rev=1 + inode_size=0 → og reject / core accept; log_bs=7 → og accept (128 KiB) / core reject; a non-journal ext2 whose inode 8 starts with jbd2-looking bytes → core could salvage from a non-journal (the jsb magic gate usually filters).

## Impact

Garbage-SB acceptance-band noise, both directions; the real deliverable is registering the four deliberate choices (each core form is defensible — the fallbacks are hardening or kernel-faithful — but unregistered).

## Reproduction / verification

Corpus-decompile evidence (the four og forms with sites; two re-read directly, two from the deep-dive coverage log with the line ranges recorded). Core side: all four quoted at `34d6cf10`.

## Fix plan (for the engineering team)

1. Register the four bands in PARITY-DELTAS (one D-row, four sub-items: core-stricter on log_bs; core-tolerant on inode_size fallback; the journal_inum unconditional-read stance; the desc_size 64BIT-form stance) — each with the one-line rationale.
2. No behavior change recommended (every core form is defensible; the og's log_bs 7..20 acceptance is an og defect class — 1 GiB block sizes are nonsense).
3. Files: `docs/PARITY-DELTAS.md`.

## Tests to add

- `Ext4.SbBandsPinned` (the four edges asserted as deliberate: inode_size=0→128 accept; log_bs=7 reject; journal_inum=0→8 on non-journal; 64BIT+ds=32→64).

## Related findings

- M-144 (the ext4 caps — the capacity end of the same validator's band questions).
- I-045 (the og-defect non-adoption register — this finding's log_bs band belongs in the same family conversation).

## Open questions

None.
