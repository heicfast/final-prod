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
