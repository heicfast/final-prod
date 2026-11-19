# named-feature-fixture engine-flow: the 44391734922 red, root-caused

**Date**: 2026-11-19 · **Session**: red-main repair + F-01 closed-loop push
**Product HEAD at start**: `f4dcc3e8` (v0.9.6) · **HEAD at handoff**: `20efb62a`

## What this session found and fixed

### 1. ui-ci red at HEAD (run 44474393997's ancestor 44391744949)

The M31 repair-page test (`renders File Repair truthfully`) failed on
`findByText(/no repairable problems were found/i)` with the dry-run
skeleton still in the DOM. Root cause (measured, not guessed): the page
awaits TWO serial mock latencies — device enumeration `latency(700,200)`
then the dry-run `latency(600,200)` (it fires only after a device
exists) — mean ~1300 ms, worst ~1600 ms, while RTL's `findByText`
default budget is 1000 ms. The local suite passed (fast Linux, 859 ms
test wall) and the Windows+coverage runner exceeded it. Evidence:
`ui/tests/timing-budget-proof.ts` (the distribution: 97.2% of trials
exceed 1000 ms). Fix: explicit `{ timeout: 4000 }` on the two awaiting
assertions (mean+3σ ≈ 1790 ms + poll interval + commit overhead, honest
margin). The undo-log assertion also converted from `getByText` to
`findByText` (same chain). Local: 225/225 vitest, tsc, eslint green.

Follow-ups the CI caught in my own commit: prettier formatting on the
new proof file (run 44474393997) and knip's unused-files check flagging
the standalone script (run 44475009006) — fixed with `pnpm exec
prettier --write` and a one-file knip `ignore` entry.

### 2. The named-feature-fixture engine-flow red (runs 10–12, canonical 44391734922)

Pulled both jobs' artifacts (VHDX + scan.crsf + recovery-report +
verify-report) and drove the evidence to THREE root causes:

**(a) ENGINE, F-01b — the `.` volume root became a `_` directory.**
The NTFS walk parents the volume root as a real in-use directory NAMED
`.` (`cr_mftwalk.cpp:1185` "its parent reference is itself"). The
classic recover flow accidentally survived it (Win32 path
normalization collapses `.`), but the F-01 plan walk SANITIZES every
component and `sanitize_component_sf(".")` → `_` — every real-volume
reconstruction nested under a phantom `_\` (the run's output:
`reconstructed/_/fixture/Docs/...`). The failure cascaded into I2 (all
20 EXACT rows "missing" at their expected rel), I3 (collision
candidates sit 4 path parts deep, the verifier accepts 2) and the
deleted-directory rows. Fix: `.`/`..` are REFERENCE names — transparent
components in both walk forms (tree + snapshot), for in-use dirs,
deleted dirs and deleted-dir emission (a deleted `.` never becomes an
item; children surface at its rel). 4 new unit tests
(`VolumeRootDotIsTransparent`, `DeletedDotDirNeverAnItem`,
`DotDotDirIsTransparent`, `SnapshotDotRootTransparent`); ExtractPlan
suite 10/10.

**(b) FIXTURE, D-V2-DURABILITY — the v2 recreate's data was lost on
detach.** Parsed `scan.crsf` (the full CRSF RESULT_TREE grammar,
re-assembling deflate chunks) and re-assembled the deleted nodes'
extents at the volume base (+0x400000): the stash copy
`.nffx/draft-v2.txt` reads its TRUE bytes (b0c28603) from the VHDX, but
the v2 recreate at `Collisions/draft.txt` reads 4096 ZERO bytes at its
own extent (ad7facb… = sha256 of 4096 zeros) — the engine read exactly
what the disk holds; the copy's data never persisted. The v2 recreate
was the ONLY mid-apply cached write (`File.Copy`) on a volume whose
cache this lane's own history proved unflushable (runs
44354788432/44355793332). Fix: the recreate goes through a
WriteThrough FileStream with `Flush(true)` and a sha-verify before the
delete (recorded in the apply-log).

**(c) FIXTURE, D-ROOT-01 + D-PROBE-01 — rooting and MFT-record reuse.**
The fixture seeded at `X:\fixture`, so volume-relative paths (what the
engine reconstructs) carried a `fixture/` component the manifest never
described; the fixture now seeds at the volume root `X:\`. The flush
probe's post-deletion create had taken `Docs/report.docx`'s freed MFT
record (the file's recovery was silently eaten — the missing I2 row);
the probe's record is now allocated BEFORE any deletion (only its 1-byte
write-through + delete stay last). The probe and the stash are now
DECLARED manifest rows (26 rows total) so I4 accountability is exact.

**Also found + fixed (display truth):** the engine printed
`(VHDX, MBR, 2.00 GB)` over the run's GPT disk — `classify_lba0` needs
LBA1 (`EFI PART` at +0x200) but the rrecov caller read only 512 bytes.
The og's documented 0x1000-byte read restored (`engine/src/app/rrecov.cpp`).

### Verification state at handoff

- Engine suite (Linux, gcc, ASan, suite-by-suite): **1682 passed / 0
  failed**; `FatChain.CycleGuardPerf` excluded locally — its 3.4 GB
  synthetic FAT image exceeds this sandbox's 4 GB RAM (OOM-kill 137);
  Windows CI owns it (my diff touches no FAT code). 16 fixture-dependent
  tests skip by design (image files absent).
- Commits: `be7966c5` (the three-defect fix set), `1a1f6877` (null-safe
  `$MAGIC[$null]` for the magic-less probe row + prettier), `20efb62a`
  (knip ignore for the standalone proof script).
- Lanes in flight at handoff: engine-ci(73)/bridge(68) on `be7966c5`;
  ui-ci(129)/ui-e2e(114) on `20efb62a`; **named-feature-fixture(14) on
  `1a1f6877` — the F-01 closed-loop validation gate (fixture job +
  engine-flow scan+reconstruct+verify over the new fixture design)**.
  Its verdict is the next session's first check; a red means a new
  root-cause pass over its artifacts (the lane uploads them always).

### Open items (precise scope)

1. **F-01 gate**: named-feature-fixture run 14's engine-flow verdict;
   on green, curate evidence into `worklog/VALIDATION/fixtures/named-feature/`
   (manifest + verify-report + VHDX sha) and mark the gate closed.
2. ui-ci 129 / ui-e2e 114 / engine-ci 73 / bridge 68 verdicts — the
   knip/prettier fixes should hold; any red is a new integration miss.
3. Remaining P1s per FEATURE-MATRIX: F-02 ($MFT run-list rebuild),
   F-03 (parent-seq validation), F-12 (backup boot-sector lattice),
   F-18/F-19 (carve breadth + fragmentation reassembly), F-28 (FAT
   orphan subtree harvest), F-37 (imaging presets UX).
4. `worklog/PARITY.md` + `worklog/FOLLOW-UP.md` (deliverables 6/4) not
   yet written — the matrix rows are all tracked in
   `worklog/FEATURE-MATRIX.md`; PARITY resolves each to
   IMPLEMENTED / DELIBERATELY-DIVERGENT / NOT-APPLICABLE once the P1
   set is done or scoped.
5. RC on clean Windows (gate 8) — after the fixture gate closes.
