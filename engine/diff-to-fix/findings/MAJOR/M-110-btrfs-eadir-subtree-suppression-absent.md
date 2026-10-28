# MAJOR M-110 — Btrfs "@eaDir" subtree suppression absent: the core emits Synology EA metadata subtrees the og deliberately never walks

**Severity:** MAJOR (user-visible result-set difference on every Synology/Btrfs volume scan — the M-018 divergence class, scoped to the Btrfs engine)
**Module:** btrfs (engine walk) / orchestration (result-set policy)
**Parameters affected:** B1 (result-set membership), A25 (tree walk logic — the descent filter), A26-adjacent (traversal pruning)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/BtrfsSearchDLL_decompiled.c:655-760` (FUN_180001cd0, the per-directory descent — the 7-byte `"@eaDir"[lVar6]` prefix compare at :704-711 with the skip-on-full-match fall-through); the driver call site :523 (FUN_180002120, the quick-scan driver — the only og Btrfs controller per I-003); the child-emit at :735 and entry-emit at :683 (the suppression is descent-only — the node itself is emitted, empty); the string census `raw-evidence ... rabin2_strings_all.txt:1911` (`@eaDir`). Full decode: `evidence/decompiled-quotes/s10f-btrfs-eadir-suppression.md`.
**Core references:** audited `44f043a5` (e56baac7 lineage). `src/core/btrfs/cr_btrfs_engine.cpp` — `scan_btrfs` :480 and the deep walk descend unconditionally; `grep -rn "eaDir" src/ include/ tests/` → zero hits (the read-agent's claim, re-run by the lead). The M-018 junk-filter pass `src/core/orchestrator/cr_filterfolder.cpp:76-113` does not and cannot reproduce the semantics: its pools (`cr_filterfolder_data.cpp`) contain no `@eaDir`, and the pass flags nodes rather than suppressing descent (an og-empty directory is not reachable by flagging).
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** none — zero `eadir` hits across the entire trail and the corpus artifacts (the filter is undocumented in the corpus's own btrfs deep-dives; this session's T3b function-level pass surfaced it, the lead re-read the decompile control flow first-hand).

## Summary

The og's Btrfs engine suppresses the descent into directories whose name starts with `@eaDir` (the 7-byte Synology EA-metadata directory name): such directories appear in the og's result tree but EMPTY — their children (the `SYNO@EA@…` EA entries) are never enqueued or emitted. The core has no equivalent filter anywhere: every Synology Btrfs volume scanned by the core floods the result tree with `@eaDir` subtrees of EA metadata files the og deliberately hides. This is the M-018 divergence class (default result-set junk policy the reference has and the core lacks) at engine scope, unregistered.

## What the reference does

`FUN_180001cd0` (BtrfsSearchDLL_decompiled.c:655) is the per-directory descent of the quick-scan walk. For each directory popped from the pending-descent list, if the entry type is directory (type word at entry+0x66 == 0x10) and the name pointer (entry+0x42) is non-null, the walk compares the name's first 7 bytes against the inline literal `"@eaDir"`; a full 7/7 match falls through WITHOUT the `goto LAB_180001e49` that performs the child walk — so the directory's children are never enqueued or emitted. Any first-byte mismatch jumps to the normal descent. The comparison is a PREFIX match with no terminator or length check (`@eaDirBackup` also suppresses). The node itself was already emitted (the child-emit at :735 / the entry-emit at :683 run before the pop-time check), so the og's presented tree shows the `@eaDir` directory empty. Full quoted decode: the evidence file §1.

## What our core does

`scan_btrfs` (cr_btrfs_engine.cpp:480) and the deep walk descend into every directory and emit every child unconditionally. `grep -rn "eaDir" src/ include/ tests/` is clean. The M-018 junk-filter pass flags nodes by name-pool membership (cr_filterfolder.cpp:76-113 — `NODE_FLAG_JUNK_FILTERED`, subtree kept); its pools are the Windows-junk sets (Config.Msi, $Windows.~BT, desktop.ini, System Volume Information, localized New Folder, BitLocker artifacts — cr_filterfolder_data.cpp) and contain no `@eaDir`; and its flag-not-hide semantics could not produce the og's empty-directory shape even if the name were added.

## Gap

Falsifiable claim: a Btrfs image containing `/photos/@eaDir/SYNO@EA@photos` (the standard Synology EA layout) scanned by the og produces a result tree where `photos/@eaDir` exists but has NO children; the same image scanned by the core (CLI `--json`, any query, recover-all) produces `photos/@eaDir/SYNO@EA@photos` plus every sibling EA entry.

## Impact

- **User-visible noise** on every Synology (and any @eaDir-bearing) Btrfs volume — the dominant real-world Btrfs deployment class for recovery tools.
- **Inventory/bench parity:** every og-vs-core comparison on such fixtures counts the EA subtrees as core-only entries — the exact mechanical-divergence class M-018 warned about, now at engine scope.
- **Recover-all writes EA metadata files** the og would never have offered.
- The og's filter is engine-side and prefix-based — a port must match the prefix semantics, not just the exact name.

## Reproduction / verification

- Decompiled evidence (strength 2): the FUN_180001cd0 walk quoted and control-flow decoded in `evidence/decompiled-quotes/s10f-btrfs-eadir-suppression.md` (read first-hand by the lead; the read-agent that surfaced it is recorded in the same file's header).
- Core-side: the grep census + the cr_filterfolder read (evidence file §4).
- Executed comparison: NOT run (no Synology fixture in this environment) — the decompile decode is instruction-decisive for the og side; a fixture-based execution is the fix's acceptance test (below).

## Fix plan (for the engineering team)

1. Add the descent suppression to the Btrfs walk: in `cr_btrfs_engine.cpp`'s directory-descent path, when a directory's name's first 7 bytes are exactly `@eaDir` (case-sensitive, no length/terminator check — og-exact), skip the child walk (emit the directory node itself, empty).
2. Register the prefix-match semantics explicitly (a `starts_with(u"@eaDir")` on the display name at the enqueue point); do NOT route through cr_filterfolder (different semantics: flag vs empty-subtree).
3. Add a PARITY-DELTAS row if the team decides the EA metadata should stay visible (a deliberate superset — then the filter belongs behind a flag); the og-exact default is suppression.
4. Tests: build a Synology-shaped fixture (nested dirs each containing `@eaDir/SYNO@EA@<name>` entries) — `BtrfsEaDirSubtreeSuppressed` (the @eaDir node present, children absent), `BtrfsEaDirPrefixMatch` (`@eaDirBackup` also suppressed), `BtrfsEaDirSiblingsUnaffected` (normal sibling dirs fully walked). Fail-before: the current core emits the subtree.
5. Acceptance: the fixture tree matches the og shape byte-for-byte at the JSON level; no other Btrfs result changes.

## Tests to add

- `Btrfs.EaDirSubtreeSuppressed` (fail-before/pass-after).
- `Btrfs.EaDirPrefixMatch` (`@eaDirBackup`, `@eaDir` exact, `@eaDirX`).
- `Btrfs.EaDirNodeItselfEmitted` (the og emits the empty directory node).
- `Btrfs.EaDirNotInFilterfolderPools` (guard: the name must not appear in the M-018 pools — different mechanism).

## Related findings

- M-018 (FIXED — the host-side junk-filter precedent; different mechanism and name set).
- I-003 (the og's single-controller Btrfs posture — the walk this filter lives in).

## Open questions

None. (Whether to ALSO suppress the `@eaDir` node itself is a deliberate-superset decision the fix plan registers; the og emits it empty.)

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's fix commit — the M-110 landing, session 39B)
**Verified by:** Btrfs.EaDirSubtreeSuppressed + Btrfs.EaDirNodeItselfEmitted (fail-before PROVEN by mutation: the suppression removed -> both RED with the EA children present; restored -> green) + Btrfs.EaDirExactMatchOnly (the semantics discriminator: @eaDirBackup descends — this test fails against a PREFIX-match implementation, the corrected decode's guard) + Btrfs.EaDirNotInFilterfolderPools (the mechanism boundary). Full ctest 1110/1110 (Linux release).
**Resolution notes:** SEMANTICS CORRECTED against the binary before landing: the finding (and its evidence file) decoded the compare as a PREFIX match ("@eaDirBackup also suppresses") — the instruction-level read disproves this. Disassembled first-hand this session (capstone over og-codebase/BtrfsSearchDLL.dll): the loop at 180001e30-180001e44 is `movzx cl,[rdx+rax] / inc rax / cmp cl,[r12+rax-1] / jne descend / cmp rax,7 / jne loop / jmp skip` with r12 = the literal at 0x180018580 = "@eaDir\0" — SEVEN bytes including the NUL terminator, so byte 6 of the candidate name must equal 0x00: the match is EXACT ("@eaDir" the 6-char name with its terminator), and "@eaDirBackup" mismatches at index 6 ('B' != 0x00) and descends normally. The decompile's `"@eaDir"[lVar6]` loop-7-times form is the classic strcmp-against-short-literal idiom; the finding's "no terminator check" reading missed that the 7th literal byte IS the terminator. The fix lands the exact-match semantics in cr_btrfs_engine.cpp's populate(): `if (de.name != u"@eaDir") populate(...)` — the directory node itself is emitted (empty, the og's shape: the entry-emit ran before the pop-time name check), the children (Synology's SYNO@EA@… EA entries) are never walked. NOT routed through cr_filterfolder (the M-018 pass flags by name-pool membership, subtree kept — different mechanism and shape; the not-in-pools guard test pins the boundary). The finding's fix-plan test list is corrected accordingly: the prefix cases invert (@eaDirBackup NOT suppressed). The fixture: build_synology_volume (the standard Synology layout /photos/@eaDir/SYNO@EA@photos + the @eaDirBackup discriminator + the unaffected sibling). Corpus citations: BtrfsSearchDLL_decompiled.c:655-760 (re-read first-hand) + the binary disassembly at 180001e0e-180001e44 (the instruction-decisive evidence, quoted in the commit). The evidence file s10f-btrfs-eadir-suppression.md's prefix-match paragraph is superseded by this decode.
