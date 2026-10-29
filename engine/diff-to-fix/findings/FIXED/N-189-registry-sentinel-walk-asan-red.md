# MINOR N-189 — (discovered in fix session S59) two registry walks in test_carve.cpp use a null-terminator idiom on a count-based table — ASan global-buffer-overflow red on main

**Severity:** MINOR (test-side only; production code is unaffected)
**Module:** carve (test suite) / registry contract
**Parameters affected:** none (no engine behavior) — CI green gate (Linux ASan lane)
**Corpus references:** none — a core-internal contract defect (the table contract is `registry()` + `registry_size()`, cr_signatures.h:512-513: "The compile-time table (one entry per FormatId, order = FormatId)"; `kTable` in `src/core/carve/cr_registry.cpp:117` carries exactly `FMT_COUNT` rows and no sentinel row)
**Core references:** `tests/unit/test_carve.cpp:8134` (S-054's `TsM2TsCapIsTheOg200MB` pin) and `:8168` (`TsStreamClampedAt200MB` disable-list builder) — both `for (const Signature* s = reg; s->ext; ++s)`; `src/core/carve/cr_registry.cpp:117` (`kTable`, ASan-reported size 13024 bytes) + `:1562-1567` (`registry()`/`registry_size()`); discovered at core `98079347` (this lane) — the parallel window repaired it at f8f01513
**Status:** FIXED

## Summary

The S-054 pin test and its companion fixture-builder walk the compile-time
signature table with a null-`ext` sentinel idiom (`s->ext` as the loop
condition). The table contract is count-based: `kTable` has exactly
`registry_size() == FMT_COUNT` rows and NO terminator row, so after the last
real row the loop reads `s->ext` from the memory that follows the global —
an 8-byte out-of-bounds read that AddressSanitizer flags as a
global-buffer-overflow ("located 8 bytes after global variable 'kTable'
defined in cr_registry.cpp:117"). The whole Linux ASan test lane aborts on
the read (the suite never completes), which means main is red on the
sanitizer gate the project mandates for every push.

## What the reference does

Not a parity question — a core-internal contract mismatch. The header
contract (cr_signatures.h:511-513) documents the table as one entry per
FormatId with `registry_size()` as the enumeration bound; every production
walk (`cr_registry.cpp:1577`, `:1621` — `validate_registry` and the format
census) indexes by count. Only the two test-side walks used the sentinel
idiom.

## What our core does

The two test walks read one element past the table. Verified by execution
at `98079347`:

```
$ ./build/linux-gcc/tests/cr_tests
==3169==ERROR: AddressSanitizer: global-buffer-overflow on address ...
READ of size 8 ... in Carve_TsM2TsCapIsTheOg200MB_Test::TestBody()
  tests/unit/test_carve.cpp:8134
0x... is located 8 bytes after global variable 'kTable' defined in
  'src/core/carve/cr_registry.cpp:117' of size 13024
```

## Gap

The suite aborts before reaching any later test under ASan — every finding
fix this session needs the suite runnable. No engine defect: the
production walks are count-based.

## Impact

CI red on the mandated sanitizer lane; latent test-suite hazard only.
Recovery results unaffected.

## Reproduction / verification

- Executed first-hand this session (session-45 lane open, core `98079347`):
  the ASan abort quoted above; `--gtest_filter='Carve.TsM2TsCapIsTheOg200MB'`
  reproduces it standalone.
- The S-054 lane landed the pin at the pre-merge tree; the terminator walk
  was introduced in the test body itself (`git log -S "s->ext; ++s"
  tests/unit/test_carve.cpp`).

## Fix plan (for the engineering team)

1. Replace both sentinel walks with count-based walks over
   `registry_size()` (the same shape every production walk uses).
2. Keep the assertions unchanged — only the iteration contract is wrong.

## Tests to add

The fixed tests are their own proof: the suite must run to completion under
ASan with the registry pin still asserting the og's 200 MB cap
(`TsM2TsCapIsTheOg200MB` green, not aborting).

## Related findings

- S-054 (FIXED — the pin test this defect rode in on)
- The 09490fea "repair(tests)" precedent (a committed-main red closed by a
  test reconstruction)

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** f8f01513 (the parallel lane's convergent landing — the fix of record per SA-9; this lane's byte-equivalent b938c8e5 dropped at the merge)
**Verified by:** the full Linux ASan suite runs to completion at the fixed tree
(`cr_tests` no longer aborts; `Carve.TsM2TsCapIsTheOg200MB` and
`Carve.TsStreamClampedAt200MB` both green — the S-054 pins still hold).
**Resolution notes:** Both walks converted to `registry_size()`-bounded
index loops (the pre-merge test_carve.cpp:8134 and :8168 sites). No production
code changed — the engine's count-based table contract was already correct at
every production walk (cr_registry.cpp:1577/:1621). The S-054 regression
pins (200 MB cap on both TS rows) assert identically after the fix.

### Convergence addendum (session-59 lane, 2026-10-29)

This lane discovered and fixed the defect independently at core `98079347`
(b938c8e5, first local commit — RED proven by the ASan abort at the lane-open
baseline; GREEN proven by the full-suite completion: 1161 passed / 0 failed /
85 env-skipped). At the hot-window merge the parallel lane's landing
**f8f01513** ("repair(tests): the S-054 TS-pin red main closed") was already
on main — the same two walks, the same count-based form, the same class
reading (their cite: the M-133/N-157 class). Per SA-9 earlier-push-wins,
**f8f01513 is the fix of record**; this lane's local repair was dropped as
the byte-equivalent duplicate (the superset rule) and this finding file —
renumbered N-186→N-189 at the same merge (the remote's N-160 window took N-186
for the renumbered N-142-rsf; N-187/N-188 likewise taken) — remains the
dedicated audit record for a defect the winning lane repaired without a
standalone finding file. The draft's session ID (S45) also collided with the
parallel window's session numbering; this lane rebranded Session-59 at the
merge.
