# MAJOR M-130 — the FAT dentry shape-validator family (byte-histogram gate, dir-size gate, size/chain-XOR gate, ext-byte gate, walk-stop semantics) is not ported: the core emits garbage dentries the og rejects

**Severity:** MAJOR
**Module:** fat (dentry validation)
**Parameters affected:** detection correctness (1), false-positive rate (3), tree walk logic (26)
**Corpus references:** `FATSearchDLL_decompiled.c:44263-44375` (`FUN_1800365a0` — the shape validator, gates (c)-(f) below), `:8792-8793` (validator failure = directory-walk break — end-of-directory semantics), 9 call sites across the walk family (census in the lane sheet)
**Core references:** the only ported sub-gate: date-sanity (config-gated, default OFF — N-010c lineage); no histogram/size/chain/ext gates in `src/core/fat/cr_fat_quick.cpp` or `cr_lzn.cpp`; audited at core `8d0d88b8`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og runs every candidate dentry through a shape validator before emission, enforcing:
(c) extension-tail byte > '?' → reject; (d) size/chain XOR consistency —
`(chain≠0) == (size@+0x1C==0)` mismatch → reject; (e) directory with `size≠0 ∧
first_cluster_high≠0` → reject; (f) a byte-histogram corruption gate — any byte value
appearing more than 9 times in the 21-byte metadata window (+0x0B..+0x1F) → reject; plus
volume-label reject and walk-stop semantics at several sites (a failed validation BREAKS
the directory walk as if end-of-directory). The core ports none of (c)-(f) — only the
date-sanity sub-gate exists (default OFF). Garbage dentries the og suppresses surface as
file results in the core.

## What the reference does

`FUN_1800365a0` (FAT:44263-44375), decoded by lane E:

- (c) ext-tail gate `:44295-96`: the last extension byte > `'?'` (0x3F) → reject —
  erased/uninitialized ext tails (0xE5-0xFF bytes) never surface;
- (d) size/chain XOR `:44344-48`: `(first_cluster != 0) == (size == 0)` → reject — a
  zero-size file with a chain, or a sized file with no chain, is inconsistent;
- (e) dir gate `:44350`: directory entries with `size != 0 && high-cluster-word != 0` →
  reject;
- (f) histogram `:44356-69`: counts byte frequencies in the dentry's +0x0B..+0x1F window
  (21 bytes: times/date/startclus/size fields); ANY value > 9 occurrences → reject — the
  main guard against erased-tail patterns (e.g. 12×0xFF) and stale-0xFF sectors;
- at `:8792-93` and sibling sites, a failed validation `break`s the walk (the validator
  doubles as the end-of-directory heuristic on corrupt directories).

## What our core does

`cr_fat_quick.cpp` walks dentries with: the deleted-marker/LFN machinery, the date
sub-gate (N-010c, config-gated default OFF), and chain-following at read time — no
ext-tail gate, no size/chain consistency gate, no dir gate, no histogram gate, no
walk-stop semantics tied to validation. A torn root sector's garbage dentry (e.g.
12×0xFF in the metadata window) is emitted as a result.

## Gap

For dentry shapes the og's validator rejects (classes (c)-(f)), the core emits results.
Falsifiable: plant a dentry with ext tail byte 0xFF (c), or 12×0xFF in +0x0B..+0x1F (f),
or size=0 with first_cluster≠0 (d) → og: not in results (and the walk stops at (f)-class
corruption); core: a file result appears.

## Impact

False-positive rate on corrupt/erased FAT volumes — the recovery target population: the
core's result set carries garbage entries the og suppresses, inflating counts (and the
S-049 namer names them), polluting the "found" signal users triage by. The missing
walk-stop semantics also changes directory-boundary behavior on corrupt directories
(the og stops at the first invalid dentry; the core continues into garbage).

## Reproduction / verification

Corpus-derived (lane E first-hand decode of FUN_1800365a0 with per-gate line cites
above; the walk-break site read at :8792-93). Core-derived (grep: no histogram/ext-tail/
size-chain gates in the FAT sources; only the N-010c date gate exists, default OFF).
No PARITY-DELTAS row covers the validator family (N-010c's registration covers the date
sub-gate only).

## Fix plan (for the engineering team)

1. Port the validator as a shared FAT dentry gate: `dentry_shape_ok(const Dentry*)`
   implementing (c), (d), (e), (f) with the exact thresholds (`'?'` tail bound, XOR
   rule, dir rule, >9 repeats over the 21-byte window). Wire it into the walk sites the
   og gates (the 9-site census in the lane sheet — start with the root/orphan-registry
   walk, the deepest-impact site).
2. Walk-stop semantics: decode + port per site (the og `break`s at some sites, skips at
   others — the lane's site census distinguishes them; port the break semantics only
   where the og breaks, else the core loses trailing valid entries the og also loses —
   parity means matching the loss).
3. Files: `src/core/fat/cr_fat_quick.cpp` (+ `cr_lzn.cpp` if the LFN path needs the
   gate), one new helper in `include/cr/fat/`.
4. Tests: below (one per gate + the walk-stop semantics + the volume-label reject).
5. Acceptance criteria: on the gate-violating fixtures, zero results where the og yields
   zero; on valid volumes, result sets unchanged (the existing FAT corpus tests green);
   the erased-tail fixture (12×0xFF) stops the walk exactly where the og stops.

## Tests to add

- `Fat.ExtTailByteBeyondQuestionmarkRejected` (fails today)
- `Fat.SizeChainXorRejected` (zero-size-with-chain AND sized-no-chain)
- `Fat.DirSizeAndHighClusterRejected`
- `Fat.HistogramTenRepeatsRejected` (12×0xFF window; boundary 9 vs 10)
- `Fat.ValidatorFailureStopsWalkAtGatedSites` (the :8792 semantics)
- `Fat.VolumeLabelRejected` (regression: already handled? pin it)

## Related findings

- N-172 / N-175 / N-176 (the same walk's name-family gates — smaller legs)
- N-010c lineage (the date sub-gate precedent for config-gating)

## Open questions

The per-site break-vs-skip census needs the 9 call sites re-verified during the fix (the
lane sheet lists them; the fix plan's step 2 depends on it). The gates themselves are
fully decoded.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's M-130 landing, session-52 lane — post-push SHA in the ledger row and worklog entry)
**Verified by:** `FatEngine.DentryAttrReservedBitsStopWalk` / `DentryErasedPatternQuadStopsWalk` / `DentryHistogramEleventhRepeatStopsWalk` / `DentryImpureLabelStopsWalk` (all four RED pre-fix — the garbage dentries surfaced as results and the walk continued; GREEN post-fix — the walk stops at the corrupt slot, the trailing dentries lost exactly like the og), `FatEngine.DentryHistogramTenRepeatsStillEmit` (the 10-vs-11 boundary pin), `FatEngine.PureVolumeLabelSkippedWalkContinues` (the label pin), `FatEngine.SizeChainXorSkippedButWalkContinues` / `DirSizeAndHighClusterSkipped` (the emission-skip classes: not emitted, walk continues — the skip-not-break distinction decoded from the two og walk families). Suite 1322/1322 (linux-release); the FAT+telemetry families ASan-clean.
**Resolution notes:** The validator ported as two helpers in include/cr/fat/cr_fat_wire.h — `fat_dentry_walk_ok` (the param_2=0 directory-walk set, wired as a per-slot walk-BREAK at cr_fat_quick.cpp's walk_dir_entries: reserved attribute bits, the erased/repeated-pattern quad, impure volume-label shapes, the byte histogram) and `fat_dentry_shape_ok` (the param_2=1 emission-walk set, wired as a pre-emit SKIP: the size/chain XOR consistency and the dir size+high-cluster shape). Three corpus-wins corrections to the finding's decode, each re-read in the raw decompile this session: (1) the ":44295-96 ext-tail byte > '?'" gate is the ATTRIBUTE byte — the compare is on the u32 at +0x08 whose top byte is the attribute (the walk sites pass the dentry as BOTH context slots, verified at :8788-89 and :42944-45); the reserved-bits semantics are the same garbage-rejection class but a different field; (2) the histogram boundary is 10-pass/11-reject (the pre-increment compare `9 < count` fires on the 11th occurrence), not ">9 occurrences"; (3) the size/chain XOR and dir gates run ONLY at the emission walk (:42948, FUN_180034db0, param_2=1) — the directory walks (:8792/:53647) never see them, so their failure is a SKIP not a walk-stop (the port separates the two classes accordingly). Additional gate decoded and ported that the finding missed: the erased/repeated-pattern quad (ext+attr quad == ntres/tenths/creation-time quad, != 0x20 — rejects the all-zero dentry and uniform-byte garbage, :44293-94) and the label-purity rule (volume bit set but attr ∉ {0x08, 0x28} with hidden clear → walk stops — which also upgrades the FAT-10 0x0F-with-stray-cluster class from skip to break, the og's own semantics). The date/time sub-gates stay config-gated per the N-010c lineage (the og's FUN_1800368e0 time gate is behind its own config flag; the always-on emission-level time-word zeroing is A-014's landing). One fixture corrected per the S-042 fixtures-from-spec rule: test_telemetry's build_fat16_wide planted cluster-2/size-0 dentries — exactly the illegal XOR class; the fixture now plants sized streams. The `shape_rejected` counter added to FatScanStats (additive).
