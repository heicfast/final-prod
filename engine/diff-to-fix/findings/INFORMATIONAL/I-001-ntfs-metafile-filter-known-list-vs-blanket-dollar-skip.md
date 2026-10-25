# INFORMATIONAL I-001 — Metafile filter is a known-20 list, not the og's blanket `$`-skip

**Severity:** INFORMATIONAL (deliberate, documented deviation — reviewed and accepted)
**Module:** ntfs
**Parameters affected:** B1 (detection correctness — result-set membership), B5 (orphan reconstruction)
**Corpus references:** `re-workspace/artifacts/SearchDLL/ntfs-engine-algorithms-v2--deleted-viability.md` §1 (filter line `name[0]=='$' → Release+skip`, decompile :21194, agent-B verified); `re-workspace/artifacts/CORRECTIONS.md` row 2 (FUN_180041970 = base-record test)
**Core references:** `src/core/ntfs/cr_mftwalk.cpp:249–262` (`is_metafile_name` + rationale comment), `:585–643` (filter, $UsnJrnl capture, $Recycle.Bin mapping) — audited commit `67193b6`
**Status:** ACKNOWLEDGED in `f74a5dd` (verified independently by the session-2 audit: blanket `$`-skip at FUN_18001acc0 :21186 re-read first-hand; pin test UserFileNamedDollarNotHidden green in 313/313 ASan; PARITY-DELTAS D-12; future work must PRESERVE this divergence)

## Summary

The og skips every record whose name starts with `$` — which silently hides the entire
`$RECYCLE.BIN` and `$Extend` subtrees (their files are `$I…`/`$R…`-named) and any user file
whose name begins with `$`. Our core skips exactly the 20 volume metafiles and surfaces
recycle-bin deletions (remapped via $I/$R pairing). This is a deliberate, code-documented
improvement; it is recorded here because it makes our result set a strict superset of the og's
on volumes with recycle-bin content.

## What the reference does

Decompiled filter (FUN_18001acc0, :21186–21194, agent-B verified):

```
filters (props flags u16@+0x23, name str@+0x11):
   (flags & 4) || name==NULL || name[0]=='$' → Release+skip
```

The blanket `name[0]=='$'` drop applies to every record in the deleted pass, including
`$RECYCLE.BIN`-resident files and user files named e.g. `$notes.txt`.

## What our core does

`cr_mftwalk.cpp:249–262`:

```cpp
/* volume metafiles only - NOT every '$' name (the old blanket skip
 * swallowed $Recycle.Bin/$Extend subtree content: recycle deletions
 * vanished from the results; user files named '$...' also vanished) */
static bool is_metafile_name(const std::u16string& n) {
    static const std::u16string kMeta[] = {
        u"$MFT", u"$MFTMirr", u"$LogFile", u"$Volume", u"$AttrDef", u"$Bitmap", u"$Boot",
        u"$BadClus", u"$Secure", u"$UpCase", u"$Extend", u"$Quota", u"$ObjId", u"$Reparse",
        u"$RmMetadata", u"$UsnJrnl", u"$SDH", u"$SII", u"$Transact", u"$Config"};
```

plus the $I/$R recycle remap (`:620–643`, `:808–816`) and the deferred unmatched-$R phase
(`:929–942`).

## Gap

Falsifiable claim: for a volume containing recycle-bin deletions, the og result set has no
entries for them and our core has remapped entries (original name, size, deletion time from
$I, `DeletedState::Deleted`). For a user file named `$x.txt`, og hides it; core shows it.

## Impact

- User-visible superset (better recovery outcomes; recycle-bin is a primary user scenario).
- Comparative benchmarks must account for it: og-side inventories cannot contain these files,
  so our recall on recycle scenarios can only be scored against the seed corpus ground truth,
  not against og output.
- No correctness risk: the recycle remap is evidence-based ($I metadata), not inferred.

## Reproduction / verification

- Corpus-derived: decompile quote (filter line) — the og has no recycle-remap counterpart
  anywhere in the NTFS artifacts (B-ntfs.md full read).
- Core-derived: code quotes above; tests `test_ntfs_volume.cpp` recycle cases (agent-D test
  inventory: recycle $I/$R remap incl. out-of-order records — covered).

## Fix plan

No code change required. Actions:

1. Register this deviation in `docs/parity/PARITY-DELTAS.md` as ACCEPTED with this finding ID
   if not already listed (the PARITY-DELTAS D-01..D-45 register was not re-verified for this
   row during this audit wave — check before duplicating).
2. In bench metrics, tag recycle-originated nodes (they carry the remap provenance) so
   og-comparison excludes them explicitly rather than by luck.

## Tests to add

- `test_ntfs_volume.cpp::User_File_Named_Dollar_Not_Hidden`: active file `$notes.txt` in a
  user directory → emitted (already true; pin it as a regression guard for anyone
  "fixing" parity by restoring the blanket skip).
- Bench-side: assert recycle-originated nodes are excluded from og-comparison sets.

## Related findings

- S-001 (same result-set-superset family, opposite direction of intent).

## Open questions

None.


## Resolution

**Status:** ACKNOWLEDGED (deliberate, recovery-positive superset — preserved)
**Verified by:** first-hand re-verification this session: the blanket
`name[0]=='$'` skip exists in the og loop (FUN_18001acc0 :21186, read
directly); no recycle-remap counterpart exists in the corpus artifacts; the
rationale (recycle deletions + user '$…' files surface) is correct and
code-documented. Pin test `NtfsVolume.UserFileNamedDollarNotHidden` added
(active `$notes.txt` must surface — guards against anyone "fixing" parity by
restoring the blanket skip). Already registered: PARITY-DELTAS D-12 (known-
metafiles + $I/$R remap). Future work must PRESERVE this divergence.
