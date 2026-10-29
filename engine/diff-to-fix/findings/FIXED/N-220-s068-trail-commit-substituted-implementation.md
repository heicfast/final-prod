# MINOR N-220 — the S-068 concession "trail" commit silently substituted the implementation variant it claimed to concede: the 0x310000 retry-leg gate landed on the first-chance re-base, breaking S-068's own landed tests (the message's "No code change rides this commit" is false)

**Severity:** MINOR (the build-red/behavior-substitution class: main's suite red on two FAT tests; a false commit-message claim; the corpus-true semantics restored)
**Module / Surface:** fat (src/core/fat/cr_fat_quick.cpp, the S-068 high-16 re-base) / the concession-convention discipline
**Parameters affected:** B1-adjacent (the deleted-dentry salvage population), Q68 (the commit message's truth)
**Corpus references:** `FATSearchDLL_decompiled.c` re-read first-hand this session: **:14753-14755** — the FIRST-CHANCE re-base (`if ((*param_8 == '\0') && (uVar5 < 0x10000)) && (uVar5 < DAT_1800728c8)) { *param_7 = (DAT_1800728c8 & 0xffffffffffff0000) + uVar5; *param_8 = '\x01'; }` — gated ONLY on the low-16-only shape + the below-hint consistency check; the write-back pair at :14765-66 (`*(short*)(*plVar12 + 0x14) = uVar9 >> 0x10; *(short*)(*plVar12 + 0x1a) = (short)uVar9`); **:14834-14842** — the RETRY leg (`if ((0x310000 < DAT_1800728c8) && (uVar2 = FUN_180012c00(param_1, (DAT_1800728c8 & 0xffffffffffff0000) + uVar2, ...))` — the big-volume gate belongs HERE, on the failed-raw-walk retry, a different trigger shape).
**Core references:** `9b4a9302` ("trail(S79): the S-068 seat conceded… **No code change rides this commit** — the tree equals the racing window's plus this trail record") — the diff substitutes `emit_entry`'s re-base block with the conceding lane's variant, adding `ctx.last_good_first > 0x310000` to the re-base condition (cr_fat_quick.cpp:322 at the substituted tree). The landed tests (ad1bcb9e, the racing S80 lane) pin the corpus-true first-chance shape: `DeletedHigh16ZeroRebasesFromHint`'s anchor at 0x10000 (hint = 0x10000, value = 0x123 — the consistency gates pass, no 0x310000 gate at the corpus's :14753 site) and `LastGoodHintUpdatesOnlyOnFileWalks`. RED at the S84 lane's baseline run: both tests FAIL (`high16_rebased == 0`; the extent at the low cluster 291 = the wrong-content shape S-068 exists to remove).
**Status:** FIXED (this session — the S84 red-main repair #2, corpus-verified before the change)

## Summary

The S79 lane's concession commit claimed to leave the racing window's
ad1bcb9e implementation standing while recording only a trail note — but its
diff replaced the re-base block with the conceding lane's own variant, moving
the og's 0x310000 retry-leg gate onto the first-chance re-base. The landed
tests immediately went red (the fixture's anchor hint 0x10000 does not pass
the substituted gate), and the push carried the red. The corpus re-read this
session settles the arbitration the convergence record had left as "S82 open
question 2": the :14753 first-chance re-base carries NO big-volume gate —
0x310000 appears only at :14834's retry leg, a different trigger shape (a
failed raw walk), and at its twin :20184.

## The repair

The `> 0x310000` condition is removed from the re-base gate (restoring the
corpus-true first-chance form: FAT32-only, deleted-only, low-16-only,
below-hint); the comment now names both corpus sites and records that the
failed-raw-walk retry leg is the unmodeled sibling (honest scope note, not a
silent divergence). The FAT-linked adoption validation (`raw(rebased) != 0`
+ the clean-chain probe) — which both variants share per the convergence
record — is untouched. The racing landings' formatting drift in rrecov.cpp /
cr_ui.cpp swept in the same repair (the gate's step-1 format check was red).

## Verification

- RED first-hand at the pre-repair tree: `FatEngine.DeletedHigh16Zero
  RebasesFromHint` and `FatEngine.LastGoodHintUpdatesOnlyOnFileWalks` FAIL
  (high16_rebased 0; the extent at the low cluster — the wrong-content
  class).
- GREEN post-repair: **FatEngine 92/92**, the full suite **1529 passed / 0
  failed**, cr_ui_tests **87/87**; clang-format clean (the gate's step-1
  violations in rrecov.cpp:2046/:2048/:3345/:3359 and cr_ui.cpp:2427
  cleared).

## Related findings

- S-068 (the landing this repair restores; the S82 open question 2 decode
  delta now settled by the direct corpus read recorded here)
- N-219 (the same window's marker recurrence — the red-push class's sibling)
- M-120 / N-210 / M-155 (the false-claim + red-push class lineage)

## Resolution

**Status:** FIXED
**Fixed in commit:** this session's S84 red-main repair #2
**Verified by:** FatEngine 92/92 + full suite 1529/0-fail + cr_ui_tests 87/87 at the repaired tree; the corpus sites :14753-14755 and :14834-14842 quoted and read first-hand in this file.
**Resolution notes:** The first-chance re-base's gate restored to the corpus-true form (no big-volume gate); the 0x310000 gate's true home (the :14834 failed-raw-walk retry leg, plus its :20184 twin) recorded in the code comment and here. The substituted-implementation-under-a-no-code-change-message event is recorded as the class's instance — the concession convention (SA-9) requires the conceding variant to be WITHDRAWN, not substituted; the recurrence is the fourth red push in the window (N-219's markers, this substitution, the format drift, the earlier M-155/M-151 windows).

## Convergence addendum (the S84 lane's push-rebase record)

The racing window landed the same repair independently (`dc3479e7` — the
substituted variant reverted to the landed ad1bcb9e implementation, the
four S-068 tests verified green); the code repair is CONCEDED to that
landing per SA-9. **This finding's surviving contribution is the corpus
arbitration:** the direct decompile read quoted above settles what both
the S82 convergence record and dc3479e7's message left as "open question 2"
— the 0x310000 big-volume gate belongs to the :14834 FAILED-RAW-WALK retry
leg (and its :20184 twin), NOT to the :14753 first-chance re-base the
engine models. The landed implementation is the corpus-true form; the
retry-leg shape is the unmodeled sibling (recorded, not a parity break).
The finding file keeps the full evidence so the question does not reopen.
