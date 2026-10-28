# MAJOR M-123 — GPT header validation laxity: the core accepts headers the og's analyzer rejects (no reserved==0, no exact header_size, no position/backup/usable-range gates)

**Severity:** MAJOR
**Module:** table (GPT parse)
**Parameters affected:** detection correctness (1), on-disk struct validation (20), false-positive rate (3)
**Corpus references:** `ghidra-mo/Partition_decompiled.c:4805-4856` (`FUN_1800078d0` — the og's GPT header verifier, quote below)
**Core references:** `src/core/table/cr_table_parse.cpp:29-70` (`load_gpt_side`); audited at core `8d0d88b8`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's GPT header verifier requires: signature, revision 0x10000, **header_size exactly
0x5C (92)**, **reserved == 0**, position consistency (primary: current_lba==1 and
backup_lba==sector_count−1; backup: mirrored), usable-range sanity (first_usable <
last_usable < sector_count), entries_lba OUTSIDE the usable range, entry_size == 0x80,
then the CRC over 92 bytes. The core's `load_gpt_side` checks signature, entry_size==128,
entry-count cap, header_size∈[92,512] and the two CRCs — none of the position/reserved/
usable-range gates. The core trusts headers the og routes to its salvage/backup ladder
(e.g. a truncated-image GPT whose last_usable ≥ sector_count passes as a healthy primary).

## What the reference does

`FUN_1800078d0` (Partition_decompiled.c, ghidra-mo:4813-4847):

```c
4813  if ((((*param_2 == 0x5452415020494645) && ((int)param_2[1] == 0x10000)) &&
4814      (*(int *)((longlong)param_2 + 0xc) == 0x5c)) && (*(int *)((longlong)param_2 + 0x14) == 0)) {
4815    if (param_3 == '\0') {
4816      bVar4 = param_2[3] == 1;                          /* primary: current==1 */
4817    } else {
4818      bVar4 = param_2[3] == *(longlong *)(param_1 + 8) + -1;  /* backup: current==last */
4819    }
4821    if (bVar4) {
4822      if (param_3 == '\0') {
4823        bVar4 = param_2[4] == *(longlong *)(param_1 + 8) + -1; /* primary: backup==last */
4824      } else {
4826        bVar4 = param_2[4] == 1;                        /* backup: backup==1 */
4827      }
4828      if (((((bVar4) && ((ulonglong)param_2[5] < *(ulonglong *)(param_1 + 8))) &&
4829           (((ulonglong)param_2[5] < (ulonglong)param_2[6] &&
4830            ((ulonglong)param_2[6] < *(ulonglong *)(param_1 + 8))))) &&
4831          (((ulonglong)param_2[9] < (ulonglong)param_2[5] ||
4832           ((ulonglong)param_2[6] < (ulonglong)param_2[9])))) &&
4833         (*(int *)((longlong)param_2 + 0x54) == 0x80)) {
```

then the 0x5C-byte CRC (:4834-4845; failure → "Checksum GPT HEADER sector failed." →
0xFFFFFFFF). Any structural violation → "Verify GPT HEADER sector failed." → the header
is untrusted → the caller proceeds to the salvage/backup ladder.

## What our core does

`cr_table_parse.cpp:29-70` (`load_gpt_side`): signature, `entry_size != 128 → reject`,
entry_count∈[1, 8192] (1 MiB cap), `header_size < 92 || > sec.size() → reject`, header
CRC over header_size bytes, entries CRC, and an entries-array-inside-disk bound. No
reserved field check, no header_size==92 exactness (a CRC-valid 96-byte header passes),
no current/backup LBA position checks, no first/last_usable bounds, no
entries-outside-usable-range check.

## Gap

For any GPT header with reserved≠0, header_size≠92 (CRC-consistent), wrong
current/backup LBA, or last_usable ≥ device sector count: og → primary untrusted
(salvage/backup ladder); core → primary trusted, entries emitted, `gpt_primary_ok=true`.

## Impact

Detection-accuracy divergence on damaged/truncated/hand-crafted images: a 4 TB GPT disk
imaged to a 2 TB file (last_usable beyond the device) reports a healthy GPT in the core
and a damaged-but-salvageable GPT in the og. The acceptance decision also governs WHICH
copy's entries are emitted when both copies are readable and disagree (the core prefers a
laxly-validated primary). Internal flags today, but the emitted volume set is the
user-visible output.

## Reproduction / verification

Verified first-hand this session (lead auditor): the og verifier read directly at
Partition_decompiled.c(ghidra-mo):4805-4856 (quote above — every gate decoded); the core
read directly at cr_table_parse.cpp:29-70. No PARITY-DELTAS row covers GPT header
validation (D-04 is geometry-only; grep GPT across PARITY-DELTAS: no header-validation
delta).

## Fix plan (for the engineering team)

1. Port the og's gate set into `load_gpt_side` (or a `validate_gpt_header` helper it
   calls): header_size == 92, reserved == 0, position pair (current/backup mirrored by
   side), usable-range bounds (first_usable < last_usable < sector_count), entries_lba
   outside [first_usable, last_usable]. Keep the core's existing stricter items
   (entry_count cap, entries-inside-disk) — they are the LA-7 registration's deliberate
   strictness.
2. File: `src/core/table/cr_table_parse.cpp`.
3. Order: structural gates BEFORE the CRC (cheap rejects first, og order).
4. Tests: below (each gate, plus the truncated-image fixture).
5. Doc: none beyond the module table rows.
6. Acceptance criteria: the og-derived fixture set (each gate violated individually)
   routes to salvage/backup exactly as the og does; healthy headers unchanged; the
   existing GPT tests stay green.

## Tests to add

- `Gpt.HeaderSizeNot92Rejected` (CRC-valid 96-byte header)
- `Gpt.ReservedNonzeroRejected`
- `Gpt.PrimaryCurrentLbaWrongRejected` / `Gpt.BackupLbaWrongRejected`
- `Gpt.UsableRangeBeyondDeviceRejected` (the truncated-image case)
- `Gpt.EntriesLbaInsideUsableRangeRejected`
- `Gpt.TruncatedImageFallsToBackup` (end-to-end: 4 TB GPT imaged to 2 TB → backup ladder)

## Related findings

- M-132 (the backup-probe position asymmetry — the ladder this finding's rejects flow into)
- N-161 (the entry-span underflow in the same file)
- N-164 (the deliberate-strictness registration for the core's extra gates)

## Open questions

None.
