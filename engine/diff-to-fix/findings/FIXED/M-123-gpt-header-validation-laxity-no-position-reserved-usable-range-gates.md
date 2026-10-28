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

## Resolution

**Status:** FIXED
**Fixed in commit:** `10620f04` (the S47 MAJOR-first fix lane)
**Verified by:** Gpt.OgGateSetHealthyHeaderTrusted (the control: both flags true) · Gpt.HeaderSizeNot92Rejected (RED pre-fix: the CRC-valid 96-byte header was trusted) · Gpt.ReservedNonzeroRejected (RED) · Gpt.WrongRevisionRejected (RED — a gate the finding's summary listed via the og quote; the core never checked revision) · Gpt.PrimaryCurrentLbaWrongRejected (RED) · Gpt.BackupLbaWrongRejected (RED) · Gpt.UsableRangeBeyondDeviceRejected (RED — the truncated-image case) · Gpt.EntriesLbaInsideUsableRangeRejected (RED) · Gpt.TruncatedImageFallsToLadderNotTrustedPrimary (RED pre-fix: gpt_primary_ok true on a header describing a bigger disk; post-fix untrusted + salvage recovers the entries, the og's exact ladder); the existing GPT tests green after the fixture re-shape; full suite 1178/1178.
**Resolution notes:** The og's verifier (FUN_1800078d0, Partition_decompiled.c:4813-4847 — re-read first-hand this session before the fix) landed in `load_gpt_side` in og order, structural gates BEFORE the CRC: signature, revision == 0x00010000, header_size == 92 exactly, reserved == 0, the position pair mirrored by side (primary: current==1 && backup==dev-last; backup: current==dev-last && backup==1), usable-range bounds (first_usable < sector_count, first < last, last < sector_count), entries_start_lba OUTSIDE [first_usable, last_usable], entry_size == 128 — then the 92-byte header CRC, then the entries CRC. A `bool backup_side` parameter was added to `load_gpt_side` (the og's param_3) and all four call sites updated. The core's deliberate strictness kept per N-164: the entry-count 1 MiB cap and the entries-array-inside-disk bound (the og verifier has neither). The salvage path (`salvage_primary_entries`) is INTENTIONALLY unchanged — it is the og's step-1 ladder (FUN_180006e20:4530-4560) whose own gates are signature + entry geometry + entries CRC, so a structurally-invalid primary now routes to salvage exactly as the og's does. Five stale fixtures re-shaped to real-world og-valid GPT geometry (entries at LBA2, first_usable=34, last_usable=disk-34): Gpt.ValidTable, the BadHeaderCrcDetected fixture (which now carries the full position shape so its rejection exercises the entries-CRC gate, not a structural short-circuit), make_gpt_disk (the salvage family), Gpt.EspMsrLinuxBiosbootLdmClassification, the shared build_two_volume_gpt_disk, and the LDM make_gpt_dynamic_disk — these fixtures were themselves og-INVALID (entries array inside the usable range), a fact the finding implied but did not enumerate; the re-shape is the fixture-side half of the landing. No safety hardening touched; no benchmark (correctness-only).
