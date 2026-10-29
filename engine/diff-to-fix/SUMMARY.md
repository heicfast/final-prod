# SUMMARY.md — executive summary (the S52-B severe-first convergence lane on top; the prior lanes below, newest-first)

## Session-52-B S52-B convergence-lane delta (2026-10-29 — the severe-first diff-to-fix execution lane; opened on `659f0f6c`, absorbed the S45..S59 hot windows at the rebase; corpus `be0cfe9`)

**The lane's independent A-014 verification converged with FOUR prior landings**
(S45 `862d79ef` / S51 / S47 / S56 — all recorded as addenda I..III in
findings/FIXED/A-014; this lane's **addendum IV** rides its own push): the three
zero-on-fail `DosDateTimeToFileTime` slots re-read first-hand
(FATSearchDLL:32135-54), the byte-equivalent gate built, RED→GREEN proven by the
stashed-header rebuild (6/6 FAILED pre-fix). Three UNIQUE supplements survive:
(1) the exFAT twin's own three zero-on-fail slots quoted directly
(exFATSearchDLL:45174-94 — the per-component evidence beyond the FUN_180002f40
structural gate; with the citation corrected: the exFAT access call passes a REAL
time word, the time-0 form is the FAT twin's alone);
(2) `FatEngine.AccessSlotStampsDateAtMidnightWhenTimeWordInvalid` — the third
slot's independence pin (the last of the three slots unpinned by the merged
battery);
(3) the 0x5A46 decode erratum extended to the live fixture comment
(test_fat_volume.cpp:169 `2022-01-06` → `2025-02-06`; the finding's own
mistranslation class, repeated by addendum I's first push).

**The 625a1e59 committed-red window repair (the 4th M-133/N-157 collision
instance — convergent with the window's own repair lanes):** main was RED at the
merge head (5 failing tests + compile-red, executed proof) — (1) the ext4
duplicate `SizeExceedsBlocksSetsDamagedFlag` (two lanes' N-177 landings merged
with the same TEST name; the S53 copy's unique deleted-path coverage preserved
by this lane's re-title `SizeExceedsBlocksMarkerOnDeletedInodesToo`); (2) the
M-134 grading e1/e2 unused rows; (3) the S-058 re-pin's crossed fixtures
(a.doc/zzqx inverted against the test's own assertions — the engine gate verified
NOT-IN-table-skips); (4) the N-171 validated double-tick (every user-sign record
double-counted carved_found on committed main; the erratum addendum in
findings/FIXED/N-171); (5) the user-sign registry leaks — twelve window tests
installed signs without teardown; the decisive leak: the default set's lha sign
`2,2D6C68` ("-lh" at +2) re-matched the LZH wrong-dash fixture
(`Carve.LzhWrongDashByteRejects` red in every full run since the window's first
landing — reproduced at `ff5b96ad`; the window's "suite green" claims ran
sub-filters). The 82-commit window's repair lanes converged on the same fixes
for items 1-4 and eleven of the teardowns — their landing is the fix of record
per SA-9; THIS lane's push carries the three remaining teardowns
(CarveStats.UserSignFilesCountInCarvedFound / UserSigns.RegistrationRoundTrip-
ProceedsToScan / UserSigns.ShortFragmentMatchesInCore), the access-slot pin, and
the A-014/N-171 record addenda. Verification: full suite green (linux-release),
the touched families ASan-clean.

**Board at this push (filesystem truth, re-counted at the merged tree): 55
active — 0 ACUTE · 3 SEVERE · 24 MAJOR · 26 MINOR · 2 INFO — + 4 tracked-open,
FIXED archive at 128 files.** Remaining SEVERE: S-040 (image create absent),
S-063 (NTFS MFT bootstrap fallback absent), S-065 (ext4 indirect ladder
unbounded-freeze/OOM) — the next fix order for the continuation lanes.

# SUMMARY.md — executive summary (the S55 continuation audit lane on top of the S47..S58 parallel window; the prior lanes below, newest-first)
# SUMMARY.md — executive summary (the S56 ACUTE-first fix lane on top; the prior lanes below, newest-first)

**S-065 FIXED (SEVERE — the ext4 indirect-ladder unbounded freeze/OOM):** the
ladder is now bounded by ceil(i_size/block_size) exactly as the og
(FUN_180005490 :3952-3954 re-verified first-hand), with the
physically-contiguous merge (:3986-3996), a 65536-extent hardening budget,
and the cancel token consulted at every indirection block (Volume gains
`cancel`). 4 tests RED->GREEN: PoisonDoubleIndirectBoundedBySize (273 vs 15
reads), PoisonTripleIndirectBoundedBySize (3000-cap blown vs 15),
StaleIndirectCancelResponsive (257 vs ≤1 poison reads),
ContiguousPhysicalBlocksMergedToSingleExtent (4 vs 1). Ext4 family 42/42;
suite 1444/1444; win gate 10/10; format clean. Trail: findings/FIXED/S-065 +
ledger row + ext4.md cell -> MATCH + matrix row + this block.

**Hot-window repairs this window:** the S-060/S-058 window residue (win-stub
grown for the metadata tail, the grading test's nested markers collapsed, the
preview leak closed, the _TRUNCATE format drift) + the S53's convergent
markers. Suite green throughout.
