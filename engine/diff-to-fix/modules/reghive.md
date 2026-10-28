# modules/reghive.md — the offline registry-hive module audit verdicts (the regf parser + the BitLocker FVEAutoUnlock harvest + the offline-OS profile)

Reference: the og's offline BitLocker chain — `bitlockerhelp.mo` mounts the target
volume's own SYSTEM hive ("<root>\Windows\System32\config\SYSTEM" —
`bitlockerhelp_decompiled.c:3128-3155`) and reads
`\ControlSet001\Control\FVEAutoUnlock\<volume-GUID>\Data` binary blobs, feeding the
same unlock flow as the live registry (`DRWService_decompiled.c:104-140`); the og's
DeviceManager walk (the offline OS profile) fills the 0x418-byte CPartitionBootInfo
record (`DeviceManager_decompiled.c:62034-62330`). Corpus artifacts:
`re-workspace/artifacts/DRWService-exe--bitlocker-service.md:44,88`,
`config-files/` BitLocker chain rows.

Core (audited at the session-11B tree `b37fabab`-lineage): `src/core/reghive/`
(cr_reghive.cpp 474 LOC + cr_offlineos.cpp 196 LOC) + `include/cr/reghive/` +
`tests/unit/test_reghive.cpp` (632 LOC). Pure portable parser — no OS API (the og's
RegLib is an in-house parser too); strictly read-only, bounds-checked at every
offset, size-capped (records ≤ kFveMaxRecords, blobs ≤ kFveMaxBlobBytes).

**Scope note (I-030's packaging rule):** this row SUMMARIZES already-scored
coverage — S-014 (the module's own finding, FIXED), M-082 (the offline-OS profile
half, FIXED at `1f714884`), D-16 (the BitLocker legitimate-unlock-only family), and
the M-082 PARITY-DELTAS row (`:3177+`). No new audit is opened here.

## Behavioral

- **B-x detection/unlock semantics** — **S-014 — FIXED**: the offline
  FVEAutoUnlock harvest reads the dead system's SYSTEM hive exactly as the og's
  bitlockerhelp does (the ControlSet001 walk, the per-volume-GUID Data blobs →
  temp .BEK → `-KeyFilePath`); the S-014 auto-unlock enumeration's oversized-Data
  skip pin verified (`394a1020`). MATCH-with-registration on the unlock family:
  **D-16** — detect + legitimate Windows unlock only, **no FVEK/XTS decryption**
  (the og's in-product OpenSSL `EVP_aes_*_xts` decrypt is a REGISTERED exclusion;
  F13 safety).
- **B-x the offline-OS profile (M-082, FIXED `1f714884`)** —
  `reghive::offlineos::profile_os()` reproduces the DeviceManager walk
  corpus-exact: the 8 CurrentVersion identity fields, the drive-letter-gated
  SystemRoot, the `\MountedDevices\DosDevices\<x>:` 12-byte system-partition
  location, the `\Select\Current` ControlSet resolution (`%03d` rendering), the
  ProductOptions classification — same fields, same gates, same Select-awareness
  (NOT the FVE harvest's hardcoded 001). **Registered staging boundary** (the
  M-082 PARITY-DELTAS row, `:3177+`): C++ API only, no cr_ui export yet — the
  recorded follow-up consumer is boot-fix auto-locate (cr_bootfix taking the BCD
  path from the profile's MountedDevices location); the ABI lands with that flow.
  ACCEPTED (layering-faithful staging).

## Constants and format

- **regf constants** — MATCH: REGF_HEADER_SIZE 0x1000, CHECKSUM_OFF 0x1FC,
  ROOT_CELL_OFF 0x24, BINS_SIZE_OFF 0x28, CELL_ALIGN 8, HBIN_HEADER_SIZE 0x20 —
  all corpus-decoded, cited in cr_reghive.cpp at their use sites (the S-014
  landing's own per-constant verification).

## Safety

- **Read-only posture** — MATCH (the module's own contract: no writes, no OS API,
  bounds-checked, size-capped; the og's parser is equally read-only on this path).
- **F13 BitLocker posture** — the D-16 family: no bypass, no decryption (see
  B-x above).

## Tests

- `tests/unit/test_reghive.cpp` (632 LOC) — the regf walk, the FVE harvest, the
  offline-OS profile fields (the M-082 landing's RED→GREEN set + the oversized-Data
  skip pin).

## Related findings and registrations

S-014 (FIXED) · M-082 (FIXED `1f714884`) · D-16 (PARITY-DELTAS `:422`) · the
M-082 staging row (PARITY-DELTAS `:3177+`) · M-078 (the unlock chain's turn-off
tail — a different module's finding, cross-referenced) · N-121 (the boot-fix
ESP consumer — the profile's recorded follow-up).
