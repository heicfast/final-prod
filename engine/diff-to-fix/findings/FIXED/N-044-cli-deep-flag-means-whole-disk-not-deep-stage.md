# MINOR N-044 — CLI `--deep` flag semantics: sets whole-disk lost-partition discovery, not the deep stage — naming hazard against the og's "Deep Scan" toggle

**Severity:** MINOR
**Module:** cli
**Parameters affected:** P24 (CLI exit codes/semantics — flag semantics), P37-adjacent (stage model surface), P68 (documentation accuracy — usage text)
**Corpus references:** `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:917068-917262` — the og's "Deep Scan" = the complete-searcher set (NTFS/FAT/exFAT/ZFS/HFS/ReFS + RAW; never Btrfs) run after quick; `re-workspace/artifacts/UIUX/` scan-phase captures (the Deep Scan toggle semantics)
**Core references:** `src/app/rrecov.cpp:509-512` — `--deep` sets `cfg.whole_disk = true` (lost-partition discovery); the deep ENGINE stage is default-on (`run_deep`), `--quick-only` disables it (`:513-515`); usage text `:6`/`:1601` does not disambiguate; audited commit `63a9167`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

In the og's UI, "Deep Scan" means running the complete-searcher engine pass after quick. In the core's CLI, `--deep` does NOT mean that — the deep engine pass is always on by default — it turns on whole-disk lost-partition discovery (`cfg.whole_disk`). A user or parity-tester mapping the og's Deep Scan toggle onto `--deep` gets a different scan (lost-partition discovery added) while believing they enabled the deep pass (which was already on). The usage text does not disambiguate.

## What the reference does

Deep Scan toggle → complete-searcher set + RAW after the quick pass (Btrfs excluded —
its complete factory is a NULL stub; see I-003/I-016). No lost-partition-discovery
meaning attaches to the toggle (lost-partition recovery is a separate product flow —
corpus architecture/04).

## What our core does

`rrecov.cpp:509-512`:

```c
if (has_flag(argc, argv, i, L"--deep")) { cfg.whole_disk = true; continue; }
if (has_flag(argc, argv, i, L"--quick-only")) { cfg.run_deep = false; cfg.run_carve = false; ... }
```

The deep engine stage runs by default; `--deep` = whole-disk discovery. CLI note at
`:570` even warns "--deep on a volume: lost-partition discovery needs --disk" —
confirming the meaning, but the flag NAME still collides with the og's term.

## Gap

Falsifiable: `rrecov scan --disk N` (no flag) already runs the deep stage; adding
`--deep` changes behavior by adding whole-disk discovery only. A mapping table from
og's "Deep Scan" to `--deep` is wrong.

## Impact

Parity-test and bench-script authoring errors (the bench harness's scan modes —
quick/deep/deepcarve — already model it correctly; ad-hoc users will not). Cosmetic
for recovery results (the default posture is a superset), but a real trap for anyone
comparing against the og's scan modes.

## Reproduction / verification

Direct code read (`:509-515`, usage text) this session; og semantics from the
decompile + UIUX captures (cited). Not an execution-dependent claim.

## Fix plan (for the engineering team)

1. Cheapest: rename the flag `--whole-disk` (keeping `--deep` as a deprecated alias
   that prints a deprecation note for one release), and fix the usage text to state
   "deep stage is always on; --quick-only disables it".
2. **Files to modify**: `src/app/rrecov.cpp` (flag table + usage text),
   `docs/testing.md`/bench scripts referencing `--deep` (check `bench/ours/run-ours.ps1`
   scan-mode matrix — it uses its own mode names, verify no collision).
3. **Tests to add**: `Cli.DeepFlagSetsWholeDiskOnly` (config assert: run_deep unchanged,
   whole_disk set), `Cli.DefaultRunsDeepStage` (config assert).
4. **Acceptance criteria**: usage text names all three knobs accurately; no flag name
   collides with an og term with different semantics.

## Tests to add

`Cli.DeepFlagSetsWholeDiskOnly`, `Cli.DefaultRunsDeepStage`.

## Related findings

- **I-003** — Btrfs deep-scan superset (the core's deep set differs from the og's by
  design there).
- **N-020** — CLI doc drift family.

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** this session's cli-minors batch
**Verified by:** `ScanConfig.DeepStageDefaultsOnAndWholeDiskOff` (config asserts: run_deep default-on and untouched by the alias; whole_disk opt-in - exactly the semantics the usage text now states). The rrecov flag mapping + usage text are Windows-lane surfaces (MSVC compile + the import gate; no automated flag-contract harness exists for the app layer - recorded honestly here).
**Resolution notes:** per the fix plan's cheapest option: the flag is now `--whole-disk` (the honest name - it enables lost-partition discovery); `--deep` kept one release as a deprecated alias that prints a note stating what it actually does and that the deep stage is always on (--quick-only disables it). Usage text names all three knobs accurately (deep stage always on / --quick-only disables / --whole-disk = lost-partition discovery); the header comment, the volume-target note, and the bench script comment updated. No flag name collides with an og term carrying different semantics anymore.
