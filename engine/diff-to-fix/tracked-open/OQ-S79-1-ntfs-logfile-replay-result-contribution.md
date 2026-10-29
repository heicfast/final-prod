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
