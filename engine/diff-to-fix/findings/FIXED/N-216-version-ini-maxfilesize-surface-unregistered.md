# MINOR N-216 — config/version.ini's `MaxFileSize ` per-file cap surface (default 1 TiB, GiB units, trailing space in the key) has no core counterpart and its consumer is undecoded

**Severity:** MINOR
**Module:** orchestration (config layer) / query (possible size cap)
**Parameters affected:** #16 (filter model — a possible size predicate), #46 (documentation of the contract), S42-adjacent (the config-layer disposition family)
**Corpus references:** `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:337923-337925` (site 1: `FUN_1401d08e0("config/version.ini", <section>, "MaxFileSize ", 0x400)` → `param_1[0xc] = value << 0x1e` — the IDirectoryTree ctor's config), `:749901-749903` (site 2: `FUN_140478530("config/version.ini", <section>, "MaxFileSize ", 0x400)` → `*(longlong*)(param_1+0xa0) = value << 0x1e` — the scan-task config object, the same ctor that loads FilterFolder.ini's FileName at :749899); both read first-hand this session. The shipped `og-codebase/config/version.ini` carries NO MaxFileSize key → the 0x400 (1024) default applies → 1024 << 30 = 1 TiB.
**Core references:** audited `3799dee2`, re-verified LIVE at `271f5d06` — grep `MaxFileSize` over `src/` + `include/`: zero hits; the config-layer disposition family (`evidence/s42-config-layer-dispositions.md` §2: version.ini TreeType/scanext4 registered; MaxFileSize absent from the disposition)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og reads a per-file size cap — `MaxFileSize ` (note the trailing space in the profile key), default 1024, in GiB units (`value << 0x1e` = ×2^30) — from `config/version.ini` into two config objects: the directory-tree ctor (stored at +0x60-region field) and the scan-task config (stored at +0xa0). Default 1 TiB; the shipped ini omits the key so the default governs. The core has no equivalent surface, and the og's CONSUMER of the stored cap (what behavior it gates) was not decoded this session — the two store sites are decoded, the read sites are not.

## What the reference does

Site 1 (DRWUI:337923-337925), read first-hand:
```c
iVar1 = FUN_1401d08e0("config/version.ini",&DAT_140fc751c,"MaxFileSize ",0x400);
param_1[0xc] = (longlong)iVar1;
param_1[0xc] = param_1[0xc] << 0x1e;
```
Site 2 (DRWUI:749901-749903), read first-hand:
```c
iVar1 = FUN_140478530("config/version.ini",&DAT_1415d7dbc,"MaxFileSize ",0x400);
*(longlong *)(param_1 + 0xa0) = (longlong)iVar1;
*(longlong *)(param_1 + 0xa0) = *(longlong *)(param_1 + 0xa0) << 0x1e;
```
Both are profile-integer reads (section constant undecoded — two different DAT strings; default 0x400) with a GiB→bytes shift. Site 2's ctor is the scan-task config object that also loads FilterFolder.ini's FileName pool (DRWUI:749899) — the scan-side config family. Site 1's object is the results-tree/directory-tree ctor (the 0x5c0-allocation object, DRWUI:318937).

## What our core does

No `MaxFileSize` surface anywhere (grep zero hits). The core's config layer has no per-file size cap concept; the query filter model has size buckets (M-029) but no cap predicate.

## Gap

Stated falsifiably: the og carries a per-file size cap config (GiB units, default 1 TiB) in two config objects; the core has none. The og's cap's BEHAVIORAL effect is undecoded (no consumer of the +0x60/+0xa0 fields traced).

## Impact

Unknown-but-plausible: if the cap gates tree insertion or scan-side file acceptance, files above the configured size would be excluded from og results (a silent size-class exclusion). At the shipped default (1 TiB) the practical effect on real volumes is nil; the divergence only materializes on hosts that set the key or on >1 TiB files. Filed MINOR because the semantic is undecoded — the finding's value is registering the surface + the precise next decode.

## Reproduction / verification

Both DRWUI sites quoted above read first-hand this session; the shipped version.ini read (no MaxFileSize key — the TreeType/scanext4 keys the s42 disposition registered are present). Core grep at `3799dee2` and re-verified at `271f5d06`. Evidence: `evidence/s69-lane-sheets/S69-A-localdb.md` §D.3.

## Fix plan (for the engineering team)

1. First the decode (audit task, not engineering): trace the consumer of the two stored fields — in DRWUI, find reads of `+0x60`-family on the directory-tree object (ctor `FUN_1401d1210`) and `+0xa0` on the scan-task config (ctor `FUN_140478d10`) comparing against file sizes. Candidate hypotheses to falsify: (a) tree-insertion skip for files > cap; (b) scan-side accept gate; (c) display-only truncation.
2. If the consumer is behavioral and user-observable: implement or register the deliberate divergence in PARITY-DELTAS (the S42 config-layer family) with the decoded semantics.
3. If the consumer is display-only/OOS-UI: extend the PARITY-OG-CORE-INVENTORY version.ini row with the MaxFileSize disposition (the s42 disposition sheet §2's version.ini row).
4. Acceptance: the disposition names the consumer's semantics with a DRWUI cite; the core either matches the behavior or carries a registered deliberate divergence.

## Tests to add

Only after the consumer decode (a cap gate would take `Query.PerFileCapExcludes`-style tests; a display-only semantic takes none).

## Related findings

I-036/S42 config-layer registers (version.ini TreeType/scanext4 — the same file's dispositioned keys), M-073 (the scan-time filter family — a different input), N-132 (the display-config family).

## Open questions

The consumer of the cap fields (the +0x60/+0xa0 reads) — the precise next probe is a DRWUI cross-reference sweep on the two owning objects' field reads. The two section constants (`DAT_140fc751c`, `DAT_1415d7dbc`) are undecoded strings (both read from the same file/key; the section name does not change the semantic).

---

## Resolution

**Status:** FIXED
**Fixed in commit:** this session's N-216 landing (the S81 MINOR-first lane)
**Verified by:** the consumer decode executed this session (the finding's own open question — see below) + RED→GREEN: the three new tests fail as compile-errors on the pre-fix tree (`max_file_size` absent — the N-144 precedent's compile-RED form) and pass at the fixed tree (`FilterFolder.MaxFileSizeDefaultIsOneTebibyte` / `SizeCapHidesOversizeNode` — the 2 TiB node flags, the exact-cap boundary node stays, per the og's strict `cap < size` compare / `SizeCapHostOverrideRoundTrip`); the full suite **1621/1621** at the repaired tree.
**Resolution notes:** **The consumer was decoded this session — the finding's open question is closed:** the cap field at +0xa0 on the scan-task config (ctor FUN_140478d10, DRWUI:749901-749903) is read at **DRWUI:750172-175 inside the tree-builder that also consumes the FilterFolder.ini name pool** (the StrStrIA name gate at :750147-150 runs first in the same function): `lVar8 = FUN_140089170(param_2)` (the node's size accessor); `if (cap < size || size < 0) FUN_140088f40(param_2, 0)` — **the node is HIDDEN from the results tree** through the same visibility clear the junk-name path uses. So MaxFileSize is a tree-visibility size cap, not a scan-side accept gate and not a display truncation: hypothesis (a) of the fix plan (tree-insertion skip) is the nearest reading, with the precision that the mechanism is a visibility clear on the built tree beside the junk-name filter, and the negative-size half is a signed-accessor artifact (the u64 translation is the strict > compare alone). The shipped ini carries no key (read first-hand: og-codebase/config/version.ini — TreeType/scanext4 present, MaxFileSize absent), so the og's shipped posture is the 1 TiB default — which doubles as the corrupt-size guard (a torn metadata size claiming petabytes does not flood the tree). **Landing:** the predicate lands in `filterfolder` (the core's own tree-visibility filter pass, M-018's seam — the og consumes both gates in the same tree-builder): `max_file_size()` (default `0x400 << 30`), `set_max_file_size()` (the per-host tunable surface — the og's version.ini key), `size_exceeds_cap()`, and `apply()` flags oversize nodes with NODE_FLAG_JUNK_FILTERED (the presentation surfaces already skip flagged nodes — the og's visibility semantics). One deliberate core divergence registered in PARITY-DELTAS: `set_max_file_size(0)` = the predicate explicitly OFF (the og has no 0 semantics — its minimum is 1 GiB via the << 0x1e shift); the shipped-default path (the only og-executable path) is byte-parity.
