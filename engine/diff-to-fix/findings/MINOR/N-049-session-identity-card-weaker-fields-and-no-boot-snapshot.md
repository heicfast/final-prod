# MINOR N-049 — Session identity card weaker than og record-3: no firmware/WWN/interface fields, CRC32 boot hash instead of the 512-byte snapshot

**Severity:** MINOR
**Module:** session
**Parameters affected:** D12 (session persistence format — identity-card field coverage), S36 (session lifecycle — resume/re-materialization), B9 (timestamp/identity semantics)
**Corpus references:** `re-workspace/artifacts/SaveScan/rsf-format--scan-session.md:228-242` (record 3 "device identity card": disk model +0x004 padded, serial +0x108, **firmware revision +0x206**, **WWN/serial-2 +0x30A**, **interface description +0x414** ("USB Device"), **byte[512] MBR/BOOT SECTOR SNAPSHOT at +0x518** — "full first-sector image incl. partition table; 452 contiguous code bytes observed + 0x55AA magic"; "enough hardware + boot-sector context to (a) verify on restore that the same volume is being re-attached, and (b) **re-materialize the partition context without re-enumerating the disk**")
**Core references:** `include/cr/session/cr_records.h:49-64` (`VolumeInfo` identity card: model[48] + serial[32] + **u32 boot_hash (CRC32 of the first 512 bytes)** — "the volume identity proper; the resume path compares it and warns on mismatch"); no firmware/WWN/interface fields; no sector snapshot anywhere in the record set
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Both sides persist a device identity card for resume verification, but the og's record-3 is materially richer: firmware revision, WWN/second-serial, interface description, and a full 512-byte boot-sector snapshot (supporting partition-context re-materialization). The core stores model + serial + a CRC32 of the boot sector — verify-only, weaker fingerprint, no re-materialization.

## What the reference does

Record 3 (rsf-format--scan-session.md:228-242) is a fixed-layout device identity card: model (+0x004), serial (+0x108), firmware revision (+0x206), WWN/serial-2 (+0x30A), interface description (+0x414), and the full first-sector image (+0x518, 512 bytes incl. the partition table). The artifact states its two purposes: same-volume verification on restore, and re-materializing the partition context without re-enumerating the disk.

## What our core does

`VolumeInfo` (cr_records.h:49-64) carries model[48], serial[32], and `boot_hash` (CRC32 of the first 512 bytes at the slice start); the resume path compares and warns on mismatch. No firmware/WWN/interface fields; no snapshot. The partition context on resume is re-derived from the device, not from the session.

## Gap

Falsifiable claim: for the same disk, the og's session can (a) distinguish two disks with identical model+serial but different firmware (the core cannot — its card lacks those axes), and (b) rebuild the partition context from the session blob alone; the core's session can neither.

## Impact

Edge-case identity precision (cloned/imaged disks sharing model+serial — the CRC32 boot hash does most of the discrimination work, so false-accept risk is low) and a re-materialization capability gap with no current consumer (our resume always re-attaches a device). Practical impact today: low; recorded for the data-model ledger and any future offline-session tooling.

## Reproduction / verification

Corpus-derived (record-3 artifact first-cited this session — quotes above with offsets). Core side read in-source at the audited commit.

## Fix plan (for the engineering team)

1. Extend `VolumeInfo`'s v2 tail (append-only; v1 sessions deserialize with zeros and skip verification — the existing pattern at cr_records.h:50-52): `char firmware[32]`, `char wwn[32]`, `char interface_[32]`, and a `boot_snapshot[512]` (or a new REC record to keep VolumeInfo lean).
2. Populate from the same enumeration data that fills model/serial (add firmware/WWN/interface to the win enumeration struct — `STORAGE_DEVICE_DESCRIPTOR` provides them).
3. Keep the CRC32 compare as the fast path; use the snapshot only for (a) diagnostics on mismatch (show WHAT changed) and (b) optional partition-context re-materialization if offline tooling ever needs it.
4. Tests: `Session.IdentityCardFirmwareWwnInterface` (fixture with distinct firmware), `Session.BootSnapshotRoundTrip`.
5. Acceptance: a v3 session file carries the extended card; v1/v2 sessions still load (zeros → skip).

## Tests to add

- `Session.IdentityCardFirmwareWwnInterface`
- `Session.BootSnapshotRoundTrip`

## Related findings

M-041 (session-format field family — the record-4/node side of the same ledger); N-014 (checkpoint atomicity — same module); I-012 lineage (rsf artifacts previously uncited; this session first-cites them).

## Open questions

None material — whether the og's restore actually re-materializes partition context from the snapshot in ALL paths vs. warn-and-refuse is corpus-ambiguous ("(a) and (b)" both stated); the finding's field-coverage claims are not affected by that ambiguity.
