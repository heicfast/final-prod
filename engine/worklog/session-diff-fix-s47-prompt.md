# Session 47 (S47) — the diff-to-fix parity-fix lane (SEVERE-first)

**Opened:** 2026-10-29 (utc+8) · **Lane:** S47 · **Operator:** the fixing agent per the user's mandate.
**Core at open:** `fa654673` (HEAD of main; the audited pin for the live board was `869da779`/`b26017a7` per
finding lineage — every finding is re-verified at the CURRENT tree before classification).
**Corpus at open:** `easeus-drw-research` @ `be0cfe9` (read-only ground truth).

## Mandate (token redacted)

Execute `diff-to-fix/`: for every finding (ACUTE → SEVERE → MAJOR → MINOR → INFORMATIONAL → TRACKED-OPEN),
independently verify (reproduce → re-cite corpus → classify CONFIRMED / ALREADY-FIXED / INVALID / SUPERSEDED),
then fix with tests (fail-before/pass-after), record the Resolution in the finding file, move it to
`findings/FIXED/`, update CROSS-REF-MATRIX / SUMMARY / modules / FIXED-LEDGER / docs, and push one
commit per finding. Rules: re-implement from understanding, never paste decompiled code; fix the cause;
no stubs/mocks/TODOs; safety hardenings preserved; pull frequently, never force-push, keep main green;
corpus wins over finding text; false findings marked INVALID with evidence, not "fixed" by breaking good code.

Board at open (filesystem truth, recounted this session): **1 ACUTE (A-014) · 9 SEVERE
(S-040 S-050 S-055 S-056 S-057 S-058 S-059 S-060 S-061) · 28 MAJOR · 36 MINOR · 7 INFO active; 64 FIXED; 6 tracked-open.**

## Session plan (SEVERE-first per the user's directive)

T1 pull+record ✅ · T2 read diff-to-fix ✅ · T3 read core ✅ · T4..T19 the verify→classify→fix loop in
priority order · T20 close-out: board docs, handoff.

## Worklog (chronological)

- **[T1]** Repos pulled; core at `fa654673`, corpus at `be0cfe9`. Audited-commit lineage recorded. Mandate saved.
- **[T2/T3]** diff-to-fix/ read (README, SUMMARY, all live finding files, module docs); core tree walked.
  Baseline build + full test suite next.

- **[f3e3a4b0]** A-014 FIXED (ACUTE): FAT time-word gate = the full DosDateTimeToFileTime accept set; 3 tests; finding+ledger updated.
- **[7cd8be5e]** S-055+S-056+N-171 FIXED (one carve patch): built-in-first collision rule executed, disabled-format toggle gates user signs, user-sign records count in carved_found; zero-skip restructure + stale-matches clear; 3 RED-before tests.
- **[486c8d8d]** S-058 FIXED (SEVERE): grading ext-table gate polarity corrected (NOT-in-table = the deep/twin population, verified vs FUN_1401bebe0 inequality comparator + :585888 control case); s5 evidence erratum recorded; 2 tests re-pinned.
- **[8917b012]** S-061 FIXED (SEVERE): ReFS/Btrfs insert-time High grade (FUN_14038a470 :605481-497); 2 tests.
- **[70d94f8c]** M-124 FIXED (MAJOR): EBR link 0x85 (Linux extended) joins the chain path; 1 RED-before test.

## Hot-window reconciliations (the parallel-lane ecosystem)

- The S45..S57 lanes pushed convergent landings for A-014/M-124/S-055/S-056/N-171/S-057/S-058/S-061/M-130/N-177/N-175/N-176/N-159/N-160/M-119/M-138(M-133)/N-158/M-115 while this lane worked. Three merge windows absorbed; all convergent seats resolved per SA-9 (remote code wins; this lane's records ride as convergence addenda).
- This lane's unique repairs across the windows: the M-138-fixture polarity re-base + the stale-name-lookup SEGFAULT (TwinKeyExt4NtfsNeverShift dereferenced null), the double N-171 validated tick, the crossed S-058 fixture/assertion merge, the committed conflict markers in modules/lost-partition.md, and the user-sign default-set test leak (UsersignTeardown RAII guard - the lha '2,2D6C68' sign made LzhWrongDashByteRejects fail in suite order).
- Lane rebranded S47 -> S58 (the MAJOR-first S47 and severe-first S51/S57 seats were taken by earlier pushes).
