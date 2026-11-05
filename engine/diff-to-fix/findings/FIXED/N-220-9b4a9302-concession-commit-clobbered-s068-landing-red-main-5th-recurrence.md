# MINOR N-220 — the M-120/N-210/N-217/N-219 class's FIFTH recurrence: `9b4a9302` (the S-068 "concession" trail commit) replaced the landing's implementation with the conceding lane's own variant — its commit message claims "No code change rides this commit — the tree equals the racing window's plus this trail record" while carrying a 143-line rewrite of `cr_fat_quick.cpp` — main went behaviorally RED (2 FAT tests) at the push

**Severity:** MINOR (the class convention; the repair landed in the same session — but the recurrence is a NEW SHAPE for the class: not committed conflict markers and not a build break, a *semantic* clobber that compiles clean and only the landing's own tests can catch — the worst kind, because every gate except the test suite is blind to it)
**Module:** audit-trail integrity + fat (the M-120/N-210/N-217/N-219 class; the S-068 surface)
**Parameters affected:** Q68 (audit-trail truth), B2 (recovery correctness — the S-068 re-base machinery was silently disabled for every volume whose last-good hint sits under 0x310000 clusters), CI truth
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/FATSearchDLL_decompiled.c` :14752-14756 (the PRIMARY pre-walk re-base: `if (((*param_8 == '\0') && (uVar5 < 0x10000)) && (uVar5 < DAT_1800728c8)) { *param_7 = (DAT_1800728c8 & 0xffffffffffff0000) + uVar5; *param_8 = '\x01'; }` — THREE gates, NO big-volume gate) and :14826-14848 (the separate 0xE5 second-chance retry leg: `if (*param_8 == '\0') { if ((0x310000 < DAT_1800728c8) && (uVar2 = FUN_180012c00(... re-based ...), *param_8 != '\0')) { adopt + write-back + hint update } }` — the 0x310000 gate fires only on a FIRST-WALK FAILURE, a different population). Both re-read first-hand at the repair.
**Core references:** found at core `0cf6142d` (origin/main HEAD at this lane's pull). `9b4a9302`'s tree vs `ad1bcb9e`'s: the concession commit's diff rewrites `emit_entry`'s re-base gate to `ctx.last_good_first > 0x310000` (the conceding lane's variant) while the landing's gate — and the corpus's primary site — carry no such gate. The landing's tests (`FatEngine.DeletedHigh16ZeroRebasesFromHint`, `FatEngine.LastGoodHintUpdatesOnlyOnFileWalks`, both in `tests/unit/test_fat_volume.cpp`, anchor hint 0x10000) went RED: `stats.high16_rebased` 0 ≠ 1, the torn extent at the raw low cluster (710144 ≠ 34264576), `fat_origin_cluster` 291 ≠ 65827. Verified first-hand: the full ctest run at the pulled HEAD, 2 failures.
**Status:** FIXED (found and fixed in the same session, per §8's discovered-divergence rule)

## Summary

The S-068 arbitration's concession commit pushed the conceding lane's implementation INSTEAD of the landing it conceded to. The commit message's own trail record says the opposite ("No code change rides this commit — the tree equals the racing window's plus this trail record"), and the message's own delta analysis ("This lane's variant gated the re-base on the finding's fix-plan hint > 0x310000 (point 3); the landed reading attributes the gate to the :14834 retry leg — ... the landed reading is the closer site-2 decode") documents that the CONCEDED variant is the one that landed in the tree. The consequence: the re-base machinery silently stopped firing on every volume whose last-good hint sits below 0x310000 clusters — the entire sub-~1.5 GiB-cluster population — while the tests that prove the machinery stayed RED on main.

## The two-site decode (this finding's contribution, first-hand)

The corpus carries TWO re-base sites, not one — the two racing lanes had each decoded a different one:

- **The primary pre-walk re-base (:14752-14756):** fires under exactly three gates (not-already-assumed, value < 0x10000, value < hint) with NO big-volume gate. This is the site the deleted-dentry emission path rides — the landing `ad1bcb9e` modeled it.
- **The 0xE5 second-chance retry leg (:14826-14848):** the `0x310000 < DAT_1800728c8` gate, firing only after a FIRST-WALK FAILURE — a re-base of the walked result, a different population. The conceding variant collapsed this gate onto the primary site.

The S82 addendum's open question 2 ("the landed reading is the closer site-2 decode") was correct; the concession commit's tree contradicted its own record.

## Resolution

**Status:** FIXED
**Fixed in commit:** `cd3479e7` (the racing repair — the conceding lane restored both files to the landed `ad1bcb9e` implementation; this lane's independent minimal-delta repair conceded per SA-9 and is recorded below)
**Verified by:** first-hand at this lane's tree: the two RED tests green (`FatEngine.DeletedHigh16ZeroRebasesFromHint`: high16_rebased 1, the extent at the TRUE chain 34264576, fat_origin_cluster 65827, PartialRisk; `FatEngine.LastGoodHintUpdatesOnlyOnFileWalks`: the directory dentry never clobbers the hint), FAT family 92/92, full suite 1657/1657 at `0cf6142d`+repair; the racing `cd3479e7` restore verified byte-equivalent to the landing (`efd037e4` blob identity).
**Resolution notes:** Two lanes independently found and repaired the clobber in the same window (the racing `cd3479e7` byte-restore keeps the seat per SA-9; this lane's variant — removing only the `> 0x310000` conjunct while keeping the conceding tree's refinements — was withdrawn unpushed: its hint-update position (live-walks only) is a semantic delta from the landing's any-successful-non-directory-walk position, and the landing's position is the closer decode of :13757-13758's walk-driver site). The class's fifth recurrence extends its shape: a concession commit that lands the conceding variant against its own message. The gates that catch the class are now: the conflict-marker lint (markers), `-Werror` (interleaved code), and ONLY the full test suite (semantic clobbers) — this recurrence compiled clean and passed every lint, which is why the repair protocol for pulled-red main must always start with the full ctest run, not the build. The two-site decode is recorded here for the next audit; S-068's finding Resolution stands unchanged (the landing's semantics are what the tree carries again).

## Convergence addendum (the S84 lane's independent corroboration, recorded at the merge — 2026-10-30)

A second lane (the S84 fix lane, opened at `cac42582`) independently
found, corpus-verified, and repaired the clobber in the same window —
its finding was filed as M-157 (the false-message severity reading)
before the pull revealed this lane's N-220 had already landed; the
M-157 seat is conceded per SA-9 and its content merged here. The
corroboration record:

- **The identical corpus decode.** Both lanes read the PRIMARY site
  (:14752-14756) as the three-gate conjunction with no big-volume gate,
  and located the `0x310000 < DAT_1800728c8` comparison at the retry
  sites only (:14834 and :30184 — the S84 lane additionally censused
  the second FAT16-family twin site). Both lanes reproduced the two RED
  tests first-hand at the pulled HEAD before any edit.
- **The identical repair gate set.** The S84 variant restored the same
  primary conjunction (`deleted && Fat32 && reconstruct_deleted_chains
  && first >= 2 && first < 0x10000 && first < hint`) and the
  both-branches hint update.
- **The one implementation delta (conceded, recorded for the next
  audit):** the S84 variant KEPT three of the clobbering rewrite's
  probe refinements the byte-restore drops — the explicit `rebased`
  bounds check, the `!broken && !cycle` probe refusal, and the adoption
  log line. The byte-restore (the seat) is implicitly safe on the
  bounds (a non-empty walked chain implies an in-range cluster) and its
  adoption gate `!cancelled && !clusters.empty() && raw(rebased) != 0`
  matches the landed Resolution's own decode of the success-flag class.
  If a future audit finds the broken-chain adoption shape matters (a
  torn re-based chain adopted on its partial run), the stricter probe
  is the recorded next step.
- **The class lesson both lanes reached independently:** a concession
  commit that lands the conceding variant against its own message is a
  semantic clobber that passes the marker lint AND `-Werror` — only the
  full test suite catches it. Trail commits must be diffstat-checked
  against their own messages before push.

## Corroboration addendum (a third independent lane, same two-site decode)

A convergent repair lane (rebasing over this window) independently reached
the identical arbitration from a clean corpus read — the same two sites
(:14752-14756 primary pre-walk re-base, three gates, no big-volume gate;
:14826-14848 the 0x310000-gated second-chance retry leg, a different
population), the same RED evidence (high16_rebased 0, the extent at the raw
low cluster, fat_origin_cluster 291), and the same repair (the gate
restored to the corpus-true primary form; the retry-leg shape recorded as
the unmodeled sibling). Three independent lanes, one decode, one verdict —
the arbitration is as settled as the corpus allows. The convergent lane's
finding file was folded here per the no-duplication convention; its
format-drift sweep of rrecov.cpp/cr_ui.cpp landed in the same window.
