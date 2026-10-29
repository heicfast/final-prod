# OQ-S79-1 — the og's $LogFile transaction-journal machinery (load + RCDR walk at volume open) has zero core counterpart; the walk's contribution to the RESULT SET is undecoded — the blocker for any parity filing

**Status:** OPEN (tracked) · **Filed:** session-79 continuation audit lane (renumbered TWICE at the push-rebases per SA-9 (S70 → S77 → S79; the racing fix lanes took S70..S76 then their own S77/S78)) · **Core at filing:** `3799dee2` · **Corpus:** `be0cfe9`

## The verified half (first-hand this session)

The og's NTFS volume-open loads and walks the $LogFile at BOTH open sites — `NTFSSearchDLL_decompiled.c` :753-755 (the first open path) and :31653-31655 (the second):

```c
FUN_18001cf20((undefined1 *)(param_1 + 0x260), *(longlong **)(param_1 + 0x98), local_1208,
              (uint)local_1220, *(longlong *)(param_1 + 0xa0));   /* load */
uVar5 = FUN_18001d460(param_1 + 0x260);                            /* walk */
*(undefined1 *)(param_1 + 0x409) = uVar5;                          /* the walk's success flag */
```

- **FUN_18001cf20** (:22856+, the loader): reads the $LogFile's $DATA via the device, computes the restart-page offsets, validates the restart pages through `FUN_18001cdd0` at the primary offset and again at `+0x800` (the second-restart flag at +0x29), handles the 0x200/0x800 sector-size duality, and stages the log buffer. The `param_5 != 0` path (:22878-79) carries an explicit size override — the "≤128 MB" staging bound family.
- **FUN_18001d460** (:23100+, the walk driver): gates on the loaded state, checks the RCDR signature `*(int *)(lVar2 + 0x2000) == 0x44524352` (:23132), extracts the restart-specified walk start, and invokes **FUN_180029dd0** (the record walker) three ways (:23133 forward from the restart position, :23155 the flag-form, :23157 the fallback from 0x4000). FUN_180029dd0 (:33577+) walks the RCDR log-record pages, parsing the restart/client-entry pairs (the `local_48`/`local_40` fields against the page header at +8/+0x20) and accumulating into its out-params.

**The core has none of this machinery** (grep "LogFile" across src/ + include/: zero hits). The structural absence is certain.

## The open question (the blocker)

What does the walked journal CONTRIBUTE to the og's result set? The candidates, in the order the trail's own patterns suggest:

1. **Recently-deleted file records** — the RCDR walk harvests create/delete operations whose MFT records are gone/reused (the complement to the USN harvest's tail window, N-039's family): harvested entries emitted as lost/deleted nodes, possibly with runlists adopted through the same-name machinery (FUN_18001d690's family — the S-012 rescue lane).
2. **Volume-state-only contributions** — the walk feeds internal flags (the +0x409 success flag, the clean-unmount posture) consumed by later validation gates, with NO result-set surface.
3. **The $MFT-record reconstruction source** — the journal's undo/redo data replayed to reconstruct torn MFT records (the classic NTFS "recovery" semantics), feeding the walk's damaged-record repair.

The decode needed to discriminate: **FUN_180029dd0's out-params' consumers** — where the accumulated records land (the +0x260 context's fields and their readers), and whether any reader emits into the result sink (the CNTFSFileProperties creation path) versus only mutating volume state. The follow-the-data walk starts at the +0x260 context object's field reads after :31655.

## Why tracked-open, not a finding

The user SOP: "Never assert a reference behavior you have not verified." The structural absence is verified; the behavioral gap (what files/results the journal yields — the thing a fix would port) is not. A finding filed today would have to guess between candidates 1-3 for its Impact section — exactly the false-positive class the SOP prices above a missing finding.

## The next session's probe list (precise)

1. Read every field access on the +0x260 log context after the :31655 walk (the follow-the-data walk).
2. Decode FUN_180029dd0's `param_5`/`param_6` out-params' shapes (the accumulated record list) and find their readers.
3. If an emission site exists: file the finding with the population fixture (a volume with a recently-deleted file whose MFT record is reused, journal intact) and the expected delta vs the core. If no emission site exists: file the INFORMATIONAL register (the dead-machinery family — the I-040/I-041 precedent) and close this OQ.
4. Cross-check the corpus artifacts first: `re-workspace/artifacts/SearchDLL/ntfs-engine-internals.md` and `ntfs-engine--deep-dive.md` have no $LogFile row (grep verified this session) — the decode is genuinely fresh territory.

## Related findings

- N-039 (FIXED — the USN $J tail-window harvest; the sibling journal surface that WAS decoded and ported).
- S-012 (FIXED — the two-pass architecture + the rescue chain; the same-name adoption machinery candidate 1 would consume).
- M-151 (this session — the truncated-MFT run-growth probe: the OTHER post-load volume-open salvage the og runs).

---

## Resolution (the S86 lane, 2026-10-30 — the probe list executed in full; the OQ closes)

**Status:** RESOLVED — candidate 2 (volume-state-only), in fact PROVABLY DEAD
machinery: no emission site exists anywhere in the og's chain. The follow-
the-data walk completed this session, every probe first-hand:

1. **The walk driver FUN_18001d460 (:23100-23160) discards its own
   outputs.** Its two out-params (`&local_res10`, `local_res8` —
   `undefined4 local_res8[2]`, both frame locals) are passed to EVERY
   FUN_180029dd0 call and NEVER READ after the final call; the staged
   log buffer is FREED at the driver's end (`free(puVar1[4]);
   puVar1[5]=0; puVar1[6]=0`, :23146-23150); the function's only
   persistent effect is the success flag at ctx+0x28 (:23158).
2. **The success flag is never read.** The first open site stores it at
   volume+0x409 (:756) — a corpus-wide grep finds exactly ONE write and
   ZERO reads; the volume-clone path (:40403-40405) copies it (and the
   second site's +0x208 twin) into the clone (+0x51/+0x41) — the clone's
   copies have no readers either.
3. **The record walker's accumulation is state-internal.** FUN_180029dd0
   (:33577+) assembles page fragments into the state's buffer
   (state[0xc..0xe]); the per-record handler FUN_18002a0f0 (:33741+)
   builds the chkdntfs-style RECOVERY TABLES in the state — the
   transaction table (state+0x14, FUN_18002ae10), the 0x16/0x15-pair
   table (state+0x2c, FUN_18002af50), the page table (state+0x1c,
   FUN_18002b090, with the page-number computation
   `(sector_size^2>>10) * *(int*)(rec+0x48) + *(u16*)(rec+0x44)>>1`),
   the two client-id maps (state+0x20/+0x24, FUN_18002ac20), the byte
   counter (state[0xb]) — then RESETS the accumulation buffer
   (state[0xc..0xe]=0). The tables persist in the state object but
   NOTHING after the volume open queries them: the only LSN-table
   lookup sites (FUN_18002abb0) are INSIDE the walker itself (:33777/
   :33780).
4. **No result-sink call anywhere in the chain**: no CNTFSFileProperties
   creation, no tree add, no node emit — the corpus artifacts carry no
   $LogFile row (the OQ's own grep, re-verified).

The interpretation: the og's $LogFile machinery is inherited MOUNT-PATH
code (the shared NTFS library's chkdntfs-style recovery: transaction
table, dirty-page table, client tables) that runs at volume open, builds
its tables, frees the log buffer, and is never consulted again — the
scan's results come from the MFT walk (the S-012 two-pass architecture),
not from the journal. Per the OQ's own probe list item 3, second half:
**the INFORMATIONAL register is filed (I-049, the dead-machinery family —
the I-040/I-041 precedent) and this OQ closes.** The core's zero
counterpart is CORRECT — porting dead machinery would be the
invented-path anti-pattern the mandate forbids.
