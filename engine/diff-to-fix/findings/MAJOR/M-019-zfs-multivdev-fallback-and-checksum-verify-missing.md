# MAJOR M-019 — ZFS: og attempts multi-vdev reads with a first-entry fallback; the core refuses — and the og verifies fletcher4 checksums with next-DVA retry, which the core lacks entirely

**Severity:** MAJOR (mirror members: partial content loss; striped pools: silent garbage acceptance)
**Module:** zfs
**Parameters affected:** B1 (detection/result-set), B2 (content correctness), A25 (algorithms), D15-adjacent
**Corpus references:** `ZfsSearchDLL_decompiled.c` — DVA vdev resolution `FUN_1800168b0 :19471` + **fallback `:19544-19556` (no registry match → returns the FIRST entry — the scanned disk — never fails)**; read offset `:17996 local_628 = (u32*)((dva_w1 + 0x2000) * 0x200)` (DVA offset + 4 MiB vdev skip); fletcher4 verify `:18093-18111` (4-accumulator loop), 32-byte memcmp `:18117`, gate `(uVar22 == 7) || (uVar22 == 0)` `:18076`; on failure **retry next DVA copy** `:18129 FUN_180014360(..., iVar8+1)` else drop `:18139-141`; debug "checksum can't pass" `:18120-26`
**Core references:** `src/core/zfs/cr_zfs_engine.cpp:408-410` (`if (vdev != 0) return false; /* single-disk resolvable only */`), `:1105-1106` (`stats.unsupported_vdevs`), every read `read_block(bp, 0, …)` with no checksum verify and no copy retry (`:489/:533/:584/:591/:602/:961`) — audited commit `e93decf`
**Status:** READ-ONLY FINDING — no fix applied. Resolves the multi-vdev half of tracked-open OQ-T6 (see `resolved/tracked-open/`).

## Summary

For a block pointer whose DVA names a vdev other than the scanned disk, the og resolves the vdev registry and — on no match — **falls back to the first registry entry (the scanned disk)** and reads anyway; it never refuses. Mirror members (vdev id 0) read correctly; stripe members yield their local halves; raidz yields checksum failures which are **detected** (fletcher4 + 32-byte compare) and retried against the next DVA copy before the block is dropped. The core refuses every non-zero vdev outright, and reads every block with **no checksum verification and no DVA-copy retry** — so silent corruption (or a wrong-block read) is indistinguishable from good data in core output.

## What the reference does

Quotes above. Consequences by topology (decompile-derived): mirror → full correct content from the local member; striped → correct content for blocks whose DVA names the scanned member, garbage-checked failures elsewhere; raidz → detected checksum failures → next-DVA → drop.

## What our core does

Refusal at `:408-410` (mirror members ARE accepted today only because the mirror vdev is id 0); no fletcher4, no retry — `read_block(bp, 0, …)` trusts every buffer.

## Gap

Falsifiable claims: (a) a mirror pool scanned from member 2: og recovers full trees; core recovers full trees too (id 0) — MATCH today only by accident of the id; (b) a **striped** pool: og recovers the local member's blocks (checksum-verified); core refuses every foreign-vdev block silently (`unsupported_vdevs` counter only) and, for the blocks it does read, has no checksum gate — a torn read enters results as if valid; (c) any pool where a block's first DVA is foreign but its second DVA is local: og retries DVA 2 and recovers the block; core never tries.

## Impact

ZFS topology coverage (B1) and content trust (B2). The checksum gap also weakens healthy-volume guarantees: the og's drop-on-verify-fail is a data-integrity gate the core does not have at all (session-1's module file line "B3 = M checksum-aware reads" is wrong for the core — corrected in `modules/zfs.md`).

## Reproduction / verification

Decompile-verified with full quotes (agent S2-C2; `evidence/decompiled-quotes/btrfs-zfs-refs-session2.md`); core lines verified first-hand.

## Fix plan

1. Replace the `vdev != 0` refusal with the og's resolution: registry lookup by vdev id (registry = the vdevs discovered in the label — for single-disk labels, id 0), fallback to the scanned-disk stream on no match; read at `dva_offset + 4 MiB` (the `+0x2000) * 0x200` encoding, already matched by the core's asize decode).
2. Add fletcher4 verification for checksum types {7, 0} (ON/INHERIT) with the 32-byte stored-compare; on failure, if the block pointer has more DVAs, retry the next copy; only then drop the block (count `checksum_drops`).
3. Keep the current refusal for genuinely unreadable topologies in the stats, but only after the fallback read + checksum path has had its chance (og parity).

## Tests to add

- `Zfs.StripedPoolLocalBlocksRecovered` — two-disk stripe fixture: local-member blocks recovered, foreign dropped with counters (fails before: all foreign refused with no read).
- `Zfs.ChecksumFailDropsAndRetriesNextDva` — corrupted first copy: og-parity retry/drop behavior (no code path before).
- `Zfs.MirrorSecondMemberScan` — pins the accidental MATCH at id 0 as an intentional test.

## Related findings

- OQ-T6 resolution (label probing: core superset, og front-label only — registered I-010) · I-010 (scope of label-field supersets)

## Open questions

The og's pool display-name source (it never reads the label "name" field; `root_dataset` drives enumeration) — tracked in OQ-S2M-2.
