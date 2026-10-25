# MINOR N-028 — HFS+ result-sink and fork minors (session-2 group)

**Severity:** MINOR (grouped) · **Module:** hfsplus · **Status:** READ-ONLY FINDINGS — no fix applied
**Parameters affected:** B3, B5, B7, C20
**Audited commit:** `e93decf` · corpus `03ad46c`

From the session-2 HFS+ decompile read (agent S2-G2; quotes in `evidence/decompiled-quotes/ext4-hfsplus-session2.md`):

**N-015a — Result sink drops dotfiles and "HFS+ Private Data".** og `FUN_1800057d0 :3714-3718`: `if ((*_Str == '.') || (iVar5 == DAT_18002a36c /*17*/ && strstr(_Str,"HFS+ Private Data") != NULL)) { destroy; return 1; }` — dot-prefixed names and the hardlink metadata directory are removed from results. Core has no name filter (`cr_hfsplus_engine.cpp` — grep clean). Falsifiable: a volume with `.DS_Store` or `HFS+ Private Data` shows both in core results, neither in og. Caveat: this is one result sink (vtable-reached); other og scan modes may differ — replicate for the equivalent sink only. Fix: filter `.*` and the 17-char literal at emission; register the choice. Test: `HfsPlus.DotfileAndPrivateDataFiltered`.

**N-015b — Resource forks are a core superset.** og reads the data fork only for normal files (resource fork referenced solely inside the UF_COMPRESSED branch, `FUN_180009940 :7666-7680`; no `..namedfork` string anywhere). Core surfaces `..namedfork/rsrc` child nodes (`cr_hfsplus_engine.cpp:535-557`). Deliberate superset candidate — either register in `docs/parity/PARITY-DELTAS.md` or gate behind a flag for og-parity scans (needed for inventory comparisons either way). Test: bench comparison filter or flag-off mode.

**N-015c — `'prdf'` (0x66647270) records folderized.** og `FUN_180009940 :7687-7689`: a file record whose `userInfo.fileType` bytes are `p,r,d,f` is treated as a folder (folder bit + kind 0x10, forks not parsed). Core dispatches on record type only (`:263-281`). Purpose of the constant undecoded (quoted verbatim); replicate for parity. Test: `HfsPlus.PrdfRecordFolderized`.

**Related:** S-002 (the decmpfs finding these came from — see its session-2 update) · S-003 (journal salvage)

**Open questions:** N-015c's semantic purpose (Finder alias?) — one-line comment when pinned.

## Resolution

**Status:** FIXED (N-015a/c) · REGISTERED (N-015b — D-53) · N-005/N-038 fixed alongside (own files)
**Fixed in commit:** this session's HFS+ batch (cr_hfsplus_engine.cpp: sink filter at both record sources; prdf folderization; ownerFlags-bit-0x20 fork selection; block-size bound; D-53)
**Verified by:** fail-before/pass-after on the pre-batch source (worktree at `3928f33`): `HfsPlus.DotfileAndPrivateDataFiltered` (dotfiles/Private-Data emitted before, filtered after), `HfsPlus.PrdfRecordFolderized` (file-emitted before, folderized after), `HfsPlus.RecordBit29SelectsResourceFork` (data-fork bytes before, rsrc-fork content after) — 3/3 fail before, pass after (stats-field assertions stripped for the pre-fix compile; the full tests pass post-fix). Suite 392/392.
**Resolution notes:** All three sub-items verified first-hand in the decompile this session (FUN_1800057d0 :3714-3718 sink; FUN_180009940 :7665-7689 fork gate + prdf). N-015a: the filter covers the live walk AND the freed-node orphan sweep (the og's single result callback is source-global). N-015b: the rsrc-child superset stays registered (D-53) — unmarked files keep data-fork content; N-038's marked files now emit the rsrc fork AS the node content (the og's own behavior), which composes. N-015c: fileType compared BE at +0x30 (the og's LE load 0x66647270 of disk bytes "prdf"); purpose still undecoded — the finding's one-line-comment-when-pinned note stays open in the corpus question, the behavior is pinned.
