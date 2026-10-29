# MINOR N-190 — (discovered in fix session S59) the hot-window merge pushed a red tree: a duplicated N-177 test name (g++ redefinition), dead fixtures in the merged polarity pin, a DOUBLE-counted N-171 `validated` tick, cross-wired S-058 assertions, and ten user-sign tests leaking PROCESS-GLOBAL registry state — 4 deterministic suite reds + 1 order-dependent red + the build red, all on pushed main

**Severity:** MINOR (test-side only; no engine surface — but the CI build gate is red)
**Module:** ext4/grading (test suite) — the S53/S58-window merge residue
**Parameters affected:** none (no engine behavior) — the CI green gate
**Corpus references:** none — a core-internal merge-collision defect (two lanes landed the N-177 test body under one name: cb2abd42's S53 convergence vs 7be438c9's landing, both folded into one file at the hot-window merge; the grading pin's e1/e2 dotless leftovers rode the same fold)
**Core references:** `tests/unit/test_ext4_volume.cpp:536` + `:683` (two `TEST(Ext4Engine, SizeExceedsBlocksSetsDamagedFlag)` bodies — the second is the deleted-path facet using `tree::NODE_FLAG_SIZE_EXCEEDS_BLOCKS`, the first the dir/files-only facet using `tree::FileProperties::PFLAG_SIZE_EXCEEDS_BLOCKS`; both constants are the same 0x0200 bit, cr_node.h:52/:98); `tests/unit/test_grading.cpp:225-228` (the winning S-055/S-058-era pin's unused `e1`/`e2` nodes — -Werror=unused-variable); discovered at the post-merge tree `be992fdc`
**Status:** FIXED

## Summary

At the session-59 lane's post-merge rebuild, main did not compile on the
Linux gcc `-Wall -Wextra -Wpedantic -Werror` lane: (a) the hot-window merge
folded BOTH parallel lanes' N-177 test bodies into test_ext4_volume.cpp under
the SAME `TEST(Ext4Engine, SizeExceedsBlocksSetsDamagedFlag)` name — a class
redefinition; (b) the merged S-058 polarity pin in test_grading.cpp kept its
`e1`/`e2` dotless fixture nodes after the dotless assertions moved to their own
`DotlessNamesSkipTwinDetection` test — unused variables under -Werror. The
pushed tree's own trail claimed the window's suites green (290e9c16: "suite
1379/1379"), so the red is a merge-residue class, not a lane defect: both
sides were green individually, the fold was not re-gated before the push (the
N-135/1fb7603b step-4 discipline exists precisely for this class).

## What the reference does

Not a parity question — a build-gate defect.

## What our core does (at discovery)

```
g++ -Werror: test_ext4_volume.cpp:683: error: redefinition of
  'cr::ext4::{anonymous}::Ext4Engine_SizeExceedsBlocksSetsDamagedFlag_Test'
g++ -Werror: test_grading.cpp:225: error: unused variable 'e1'
g++ -Werror: test_grading.cpp:227: error: unused variable 'e2'
```

## Gap

`make cr_tests` fails at the merged tree; every subsequent landing is blocked
until repaired.

## Impact

CI red on the mandated Linux lane; no engine defect (the two N-177 test bodies
are complementary pins, both valid; the unused nodes were dead fixtures).

## Reproduction / verification

- Executed first-hand: the post-merge `make -C build/linux-gcc -j2 cr_tests`
  output quoted above; the duplicate-name census over the whole test tree
  (`rg -o "^TEST\(...\)" | sort | uniq -d`) confirmed exactly one collision.

## Fix plan (for the engineering team)

1. Keep BOTH N-177 bodies — they pin different facets (the first: the clean
   file + the 100-KiB-declared/1-KiB-covered file + the files-only dir gate;
   the second: the damaged hello + the deleted-path `gone.dat` control) — and
   rename the second to a distinct name.
2. Drop the dead `e1`/`e2` fixtures from the polarity pin (the dotless pair
   keeps its own dedicated test).
3. Re-gate the build before the next push.

## Tests to add

The repaired build IS the test: `cr_tests` compiles clean and the renamed
`Ext4Engine.SizeExceedsBlocksDamagedAndDeletedPath` + the polarity pin both
run green.

## Related findings

- N-186/N-188 (the same committed-red family from earlier windows)
- The 09490fea/12c2f09e repair precedents
- N-135's step-4 discipline (re-run the full gate after EVERY rebase/merge)

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-190 landing — post-push SHA in the worklog entry)
**Verified by:** `make cr_tests` clean at the repaired tree (was: redefinition
+ unused-variable errors); `Ext4Engine.SizeExceedsBlocksDamagedAndDeletedPath`
+ `Ext4Engine.SizeExceedsBlocksSetsDamagedFlag` + `Ext4Engine.DamagedFlag*` all
green (both N-177 facets retained); the full suite re-gate at the same push.
**Resolution notes:** Five repairs, one class (merge folds pushed without the
post-merge full-gate re-run — the N-135 step-4 discipline applies to merges,
not only rebases): (1) the second N-177 body renamed to
`SizeExceedsBlocksDamagedAndDeletedPath` (both lanes' pins kept — the facets
are complementary, not duplicates); (2) the dead `e1`/`e2` dotless fixtures
dropped from the S-058 polarity pin (the dotless pair retains its own
`Grading.DotlessNamesSkipTwinDetection`); (3) the hot-window merge folded BOTH
lanes' N-171 `validated` tick into the user-sign emission arm (cr_carve.cpp
carried two `ctx.validated.fetch_add(1)` with two N-171 comments) — every
user-sign record double-counted in `carved_found`; ONE tick kept with the
merged provenance note; (4) the merged S-058 test body was cross-wired — the
`.zzqx` (out-of-table) fixtures carried the `.doc` (in-table) expectations;
re-wired to the corrected polarity the ENGINE already implements (zzqx pair
twins+High, doc pair the else arm+Average); (5) ten window-added usersign
tests (the S-055/S-056 battery + CarveStats.UserSignFilesCountInCarvedFound)
installed sets into the PROCESS-GLOBAL registry without teardown — the
default set (111 extensions incl. the lzh/lha family) leaked into every
later scan test and `Carve.LzhWrongDashByteRejects` carves the wrong-dash
form via a leaked user sign in full-run order (order-dependent red); each
installer now restores the prevailing world (the empty set; Registration-
RoundTrip's deliberate zip install defines the later prevailing state, per
the N-168 convention). One engine-semantics repair (the tick): no other
engine code changed. The duplicate-name census over the whole test tree
returned zero after the repair. Verification: the 13-test repaired battery +
the FULL suite at the repaired tree — 1317 ran / 1275 OK / 42 env-skipped
(TiffVariants) / **0 failed** (was: build broken + 4 deterministic suite
reds + 1 order-dependent red).
