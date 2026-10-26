# MAJOR M-012 — FAT mirror mode: core consults the mirror on BAD, accepts mirror answers unscrutinized, and cannot scan mirror-only (FAT1 unreadable)

**Severity:** MAJOR (damaged-volume result divergence; whole-volume scan failure class)
**Module:** fat
**Parameters affected:** B2 (recovery on damaged volumes), A27 (fragment handling), S35 (error recovery)
**Corpus references:** `FATSearchDLL_decompiled.c` — factory `FUN_180006140 :3809-3950` (children +8 FAT1, +0x10 FAT2); `CDoubleFATTable::Next` `FUN_180041de0 :54216-54248` (fallback only on 0; mirror scrutiny: shifted-copy reject, cross-link reject, contiguity accept; BAD/EOC mirror answers not believed when primary said FREE; raw unscrutinized answers when child1 is NULL); BAD canonicalization `FUN_180041c80 :54135` (`if (uVar1 == 0xfff7) return 0xffffff7;`), `FUN_1800416f0 :53798-53804` (FAT12 0xFF7 raw)
**Core references:** `src/core/fat/cr_chain.cpp:83-86` (FAT1 read failure → ERR_READ_FAILED), `:103` (mirror consulted when `v == 0 || v == CHAIN_BAD`) — audited commit `e93decf`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's double-FAT policy: trust the primary FAT whenever its answer is non-zero (including BAD); fall back to the mirror **only** when the primary answers FREE/OOB; when the mirror is consulted, **scrutinize** its answer (reject shifted-copies and cross-links, accept only contiguity-consistent answers; never believe mirror BAD/EOC when the primary said FREE); and when the primary FAT is unreadable entirely, scan on the raw mirror. The core: consults the mirror on **BAD too**, accepts any non-zero non-BAD mirror answer unconditionally, and fails the whole volume when FAT1 cannot be read.

## What the reference does

`FUN_180041de0`:

```
:54216  if (child1 && (uVar2 = child1->Next(c), uVar2 != 0)) return uVar2;   // primary wins when non-zero
:54226-54248  mirror scrutiny: backward v: reject if Next(c+1)==v+1 (shifted copy), reject if
              Next(v-1)==v (cross-link), accept iff Next(v)==v+1; forward v: accept unless
              v ∈ {0x0FFFFFF7, 0x0FFFFFF8, 0xFFFFFFFF} (mirror BAD/EOC not believed after primary FREE)
child1 == NULL → mirror answer returned RAW (mirror-only volume scan)
```

BAD is a non-zero answer (canonicalized per flavor), so a primary BAD **never** triggers fallback.

## What our core does

```cpp
// cr_chain.cpp:103
if (mirror_ && (v == 0 || v == CHAIN_BAD)) { ... consult mirror, accept any non-zero non-BAD ... }
// cr_chain.cpp:83-86
if (!read_exact(... primary ...)) return ERR_READ_FAILED;   // no mirror-only mode
```

## Gap

Falsifiable claims: (a) a cluster whose primary FAT says BAD but whose mirror holds a live link: og treats the chain as broken at BAD; core re-links through the mirror (different content recovered); (b) a mirror answer that is a shifted copy of the primary (classic FAT1/FAT2 mismatch from partial writes): og rejects the mirror answer; core follows it (bogus chains); (c) a device whose FAT1 region is unreadable: og scans the volume on the raw mirror; core fails the whole scan.

## Impact

Damaged FAT volumes are the product's core scenario; the three sub-divergences change recovered content and, in the mirror-only case, whether the volume scans at all.

## Reproduction / verification

Decompile-verified with full quotes (agent S2-B; `evidence/decompiled-quotes/fat-exfat-session2.md`); core lines verified first-hand. Sub-case (c) is directly bench-testable with a device that fails reads over the FAT1 region.

## Fix plan

1. `cr_chain.cpp:103` — consult the mirror **only when the primary answer is 0** (drop `|| v == CHAIN_BAD`).
2. Add the og's mirror scrutiny: on backward answers reject shifted-copies (`mirror->next(c+1) == v+1`) and cross-links (`mirror->next(v-1) == v`), accept only `mirror->next(v) == v+1`; on forward answers, do not accept mirror BAD/EOC values when the primary said FREE.
3. Mirror-only mode: on primary load failure, construct the table from the mirror at `reserved + spf` and continue the scan (mark `mirror_only` in scan stats).

## Tests to add

- `Fat.PrimaryBadNotRelinkedThroughMirror` — (a).
- `Fat.MirrorShiftedCopyRejected` / `Fat.MirrorCrossLinkRejected` — (b).
- `Fat.MirrorOnlyScan` — FAT1 unreadable, FAT2 intact: volume scans, chains resolve (fails before).

## Related findings

- M-011 (alternate-region retry on load failure — the factory-level complement of the mirror-only mode) · N-002 (EOC family semantics, mechanism-corrected this session)

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** `cfae986`
**Verified by:** FatEngine.MirrorShiftedCopyRejected (fail-before FAILED: the shifted-copy answer was followed; pass-after: rejected, and the contiguity-consistent backward answer IS believed in the same test), FatEngine.MirrorCrossLinkRejected (fail-before FAILED; pass-after: rejected), FatEngine.MirrorOnlyScanFat1Unreadable (fail-before FAILED: ERR_READ_FAILED killed the whole scan; pass-after: loads from the mirror, chains resolve, the full scan succeeds with stats.mirror_only_scan == 1). Sub-case (a) (primary BAD not re-linked) was already landed by the N-022 fix (mirror consult on primary FREE only) and is pinned by its own tests.
**Resolution notes:** FatTable::mirror_consult_ implements the og's scrutiny verbatim (FUN_180041de0 :54226-48 re-read first-hand): consulted only when the primary answered FREE; backward answers (alt < cluster) reject shifted copies (mirror(c+1) == alt+1) and cross-links (mirror(alt-1) == alt), believing only contiguity (mirror(alt) == alt+1); forward answers reject mirror BAD/EOC claims (a mirror is not believed when the primary said FREE); the previous accept-any-non-zero-non-BAD policy followed torn FAT1/FAT2 pairs into bogus chains. Mirror-only mode: when FAT1 cannot be read, the table is constructed from the mirror at reserved+spf with no second-mirror consult (the og's child1==NULL shape, raw mirror answers) and the scan proceeds; stats.mirror_only_scan records it. The og's raw-mirror answers skip scrutiny by construction (no primary exists to disagree) - the core's mirror-only table serves entry_at directly, same shape.
